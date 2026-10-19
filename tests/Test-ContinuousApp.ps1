param(
    [Parameter(Mandatory = $true)]
    [string]$AppPath,
    [Parameter(Mandatory = $true)]
    [string]$EtlDumpPath,
    [ValidateRange(2, 30)]
    [int]$RunSeconds = 3
)

. "$PSScriptRoot\Common.ps1"

$check = {
    param([string]$Output, [string]$AppOutput)

    Assert-ContinuousConsoleOutput -AppOutput $AppOutput -TraceOutput $Output

    $messages = @(Get-TraceEvents $Output 'Message')
    Assert-Trace (@($messages -match "'message': 'Start' \(System.String\)").Count -eq 1) 'Expected one Start message.'

    $rolls = @(Get-TraceEvents $Output 'DieRolled')
    Assert-Trace ($rolls.Count -ge 10) "Expected at least 10 die rolls, got $($rolls.Count)."
    foreach ($roll in $rolls)
    {
        Assert-Trace ($roll -match "^DieRolled -- \( 'value': '[1-6]' \(System.Int32\) \) -- Task: '0'$") `
            "Invalid die value or payload type: $roll"
    }

    $stats = @(Get-TraceEvents $Output 'DiceStats')
    Assert-Trace ($stats.Count -ge 1) 'Missing periodic dice statistics.'
    $total = 0
    for ($die = 1; $die -le 6; $die++)
    {
        Assert-Trace ($stats[-1] -match "'D$die': '([0-9]+)' \(System.Int32\)") "Missing or invalid counter D$die."
        $count = [int]$Matches[1]
        $observed = @($rolls -match "'value': '$die'").Count
        Assert-Trace ($count -le $observed) "Counter D$die exceeds the observed rolls."
        $total += $count
    }
    Assert-Trace ($total -ge 10 -and $total -le $rolls.Count) 'Dice statistics do not agree with the recorded rolls.'
}

# The timed stop terminates the process, so an End event is not expected.
Invoke-TraceTest -AppPath $AppPath -EtlDumpPath $EtlDumpPath -Check $check -StopAfterSeconds $RunSeconds
