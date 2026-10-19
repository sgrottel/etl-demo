param(
    [Parameter(Mandatory = $true)]
    [string]$AppPath,
    [Parameter(Mandatory = $true)]
    [string]$EtlDumpPath
)

. "$PSScriptRoot\Common.ps1"

$check = {
    param([string]$Output, [string]$AppOutput)

    Assert-Trace ([string]::IsNullOrWhiteSpace($AppOutput)) 'DemoAppCS should not write console output.'

    $messages = @(Get-TraceEvents $Output 'WriteEventStringCS')
    Assert-Trace ($messages.Count -eq 2) "Expected 2 message events, got $($messages.Count)."
    Assert-Trace ($messages[0] -match "'msg': 'Start' \(System.String\)") 'Missing initial Start message.'
    Assert-Trace ($messages[1] -match "'msg': 'End' \(System.String\)") 'Missing final End message.'

    $events = @(Get-TraceEvents $Output 'WriteEventIntsCS')
    Assert-Trace ($events.Count -eq 12) "Expected 12 integer-pair events, got $($events.Count)."
    for ($a = 0; $a -lt 4; $a++)
    {
        for ($b = 1; $b -le 3; $b++)
        {
            $event = $events[$a * 3 + $b - 1]
            Assert-Trace ($event -match "'i1': '$a' \(System.Int32\) 'i2': '$b' \(System.Int32\)") `
                "Expected integer pair ($a, $b), got: $event"
        }
    }
}

Invoke-TraceTest -AppPath $AppPath -EtlDumpPath $EtlDumpPath -Check $check
