param(
    [Parameter(Mandatory = $true)]
    [string]$AppPath,
    [Parameter(Mandatory = $true)]
    [string]$EtlDumpPath
)

. "$PSScriptRoot\Common.ps1"

$check = {
    param([string]$Output, [string]$AppOutput)

    $events = @(Get-TraceEvents $Output 'WriteEventInts')
    Assert-Trace ($events.Count -eq 10) "Expected 10 dice-pair events, got $($events.Count)."
    $lines = @($AppOutput -split '\r?\n' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    Assert-Trace ($lines.Count -eq 12) "Expected a header, 10 dice rounds, and a completion message, got $($lines.Count) console lines."
    Assert-Trace ($lines[0] -ceq 'I will now roll 10x 2x D6 dice:') 'Missing dice console header.'
    Assert-Trace ($lines[-1] -ceq 'done.') 'Missing console completion message.'
    for ($round = 0; $round -lt 10; $round++)
    {
        $event = $events[$round]
        Assert-Trace ($event -match "^WriteEventInts -- \( 'i1': '[1-6]' \(System.Int32\) 'i2': '[1-6]' \(System.Int32\) \) -- Task: '0'$") `
            "Invalid dice values, payload types, or task: $event"
        Assert-Trace ($lines[$round + 1] -match "^\s+${round}: \[([1-6])\] \[([1-6])\]$") `
            "Invalid console dice round ${round}: $($lines[$round + 1])"
        $first = $Matches[1]
        $second = $Matches[2]
        Assert-Trace ($event -match "'i1': '$first' \(System.Int32\) 'i2': '$second' \(System.Int32\)") `
            "Console dice round $round does not match its trace event."
    }
}

Invoke-TraceTest -AppPath $AppPath -EtlDumpPath $EtlDumpPath -Check $check
