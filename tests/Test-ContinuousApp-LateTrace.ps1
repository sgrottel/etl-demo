param(
    [Parameter(Mandatory = $true)]
    [string]$AppPath,
    [Parameter(Mandatory = $true)]
    [string]$EtlDumpPath,
    [ValidateRange(3, 30)]
    [int]$RunSeconds = 3
)

. "$PSScriptRoot\Common.ps1"

$check = {
    param([string]$Output, [string]$AppOutput, [hashtable]$TraceState)

    Assert-ContinuousConsoleOutput -AppOutput $AppOutput -TraceOutput $Output -LateTrace -TraceState $TraceState

    $messages = @(Get-TraceEvents $Output 'Message')
    Assert-Trace ($messages.Count -eq 0) 'Start/End messages should be outside the capture window.'

    $previous = $null
    $sinceStats = @(0, 0, 0, 0, 0, 0)
    $rollCount = 0
    $statsCount = 0

    foreach ($event in $Output -split '\r?\n')
    {
        if ($event -match '^DieRolled -- \(')
        {
            Assert-Trace ($event -match "^DieRolled -- \( 'value': '([1-6])' \(System.Int32\) \) -- Task: '0'$") `
                "Invalid die value or payload type: $event"
            $sinceStats[[int]$Matches[1] - 1]++
            $rollCount++
        }
        elseif ($event -match '^DiceStats -- \(')
        {
            $current = @(0L, 0L, 0L, 0L, 0L, 0L)
            for ($die = 1; $die -le 6; $die++)
            {
                Assert-Trace ($event -match "'D$die': '([0-9]+)' \(System.Int32\)") "Missing or invalid counter D$die."
                $current[$die - 1] = [long]$Matches[1]
                if ($null -ne $previous)
                {
                    $delta = $current[$die - 1] - $previous[$die - 1]
                    Assert-Trace ($delta -eq $sinceStats[$die - 1]) "Counter D$die does not match rolls between statistics."
                }
            }

            if ($null -eq $previous)
            {
                $lifetimeTotal = ($current | Measure-Object -Sum).Sum
                Assert-Trace ($lifetimeTotal -gt $rollCount) 'Expected statistics to include rolls from before tracing started.'
            }
            $previous = $current
            $sinceStats = @(0, 0, 0, 0, 0, 0)
            $statsCount++
        }
    }

    Assert-Trace ($rollCount -ge 10) "Expected at least 10 recorded die rolls, got $rollCount."
    Assert-Trace ($statsCount -ge 2) "Expected at least 2 statistics events to check counter increases, got $statsCount."
}

Invoke-TraceTest -AppPath $AppPath -EtlDumpPath $EtlDumpPath -Check $check `
    -StopAfterSeconds $RunSeconds -TraceStartDelaySeconds 2
