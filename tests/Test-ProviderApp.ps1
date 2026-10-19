param(
    [Parameter(Mandatory = $true)]
    [string]$AppPath,
    [Parameter(Mandatory = $true)]
    [string]$EtlDumpPath
)

. "$PSScriptRoot\Common.ps1"

$check = {
    param([string]$Output, [string]$AppOutput)

    $lines = @($AppOutput -split '\r?\n' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $previousIndex = -1
    foreach ($message in @('Setup', 'Here we go', 'Teardown', 'Done.'))
    {
        Assert-Trace (@($lines -ceq $message).Count -eq 1) "Missing or duplicated console message: $message"
        $index = [array]::IndexOf($lines, $message)
        Assert-Trace ($index -gt $previousIndex) "Console message out of order: $message"
        $previousIndex = $index
    }
    Assert-Trace (@($lines -ceq 'Start ProcessTrace...').Count -eq 1) 'Missing real-time consumer startup message.'
    Assert-Trace (@($lines -ceq 'End ProcessTrace.').Count -eq 1) 'Missing real-time consumer completion message.'
    Assert-Trace ([array]::IndexOf($lines, 'End ProcessTrace.') -gt [array]::IndexOf($lines, 'Start ProcessTrace...')) `
        'Real-time consumer console messages are out of order.'

    $received = @($lines -match '^Received `WriteEventInts` ')
    Assert-Trace ($received.Count -eq 6) "Expected 6 observed numeric events on the console, got $($received.Count)."
    foreach ($event in $received)
    {
        Assert-Trace ($event -match '^Received `WriteEventInts` \[[0-9]{1,2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}_[0-9]{3}_[0-9]{3}\]: ') `
            "Invalid observer console timestamp: $event"
    }
    foreach ($pair in @(@(1, 15), @(2, 27), @(4, 42), @(6, 666), @(8, 8015)))
    {
        $pattern = "\bi1 = $($pair[0]);\s+i2 = $($pair[1]);$"
        Assert-Trace (@($received -match $pattern).Count -eq 1) "Missing or invalid observed console pair ($($pair -join ', '))."
    }
    Assert-Trace (@($received -match '\bf = 4[.,]2;\s+i1 = 7;\s+i2 = 79;$').Count -eq 1) `
        'The alternate event schema was not decoded correctly on the console.'

    $events = @(Get-TraceEvents $Output 'WriteEventInts')
    Assert-Trace ($events.Count -eq 6) "Expected 6 numeric events, got $($events.Count)."
    foreach ($pair in @(@(1, 15), @(2, 27), @(4, 42), @(6, 666), @(8, 8015)))
    {
        $pattern = "'i1': '$($pair[0])' \(System.Int32\) 'i2': '$($pair[1])' \(System.Int32\)"
        Assert-Trace (@($events -match $pattern).Count -eq 1) "Missing or invalid integer pair ($($pair -join ', '))."
    }

    Assert-Trace (@($events -match "'f': '4[.,]2' \(System.Single\) 'i1': '7' \(System.Int64\) 'i2': '79' \(System.Int64\)").Count -eq 1) `
        'The alternate schema sharing the WriteEventInts name was not decoded correctly.'

    $strings = @(Get-TraceEvents $Output 'WriteEventIntAndStr')
    Assert-Trace ($strings.Count -eq 2) "Expected 2 integer/string events, got $($strings.Count)."
    Assert-Trace ($strings[0] -match "'i1': '3' \(System.Int32\) 'str': 'woof\?' \(System.String\)") 'Invalid first integer/string payload.'
    Assert-Trace ($strings[1] -match "'i1': '5' \(System.Int32\) 'str': 'woof!' \(System.String\)") 'Invalid second integer/string payload.'

    $orderedEvents = @($Output -split '\r?\n' | Where-Object { $_ -match '^WriteEvent(?:Ints|IntAndStr) -- \(' })
    Assert-Trace ($orderedEvents.Count -eq 8) "Expected 8 provider events, got $($orderedEvents.Count)."

    for ($index = 0; $index -lt 8; $index++)
    {
        $value = $index + 1
        Assert-Trace ($orderedEvents[$index] -match "'i1': '$value' \(System.Int(?:32|64)\)") `
            "Provider events are out of order at event $value."
        $expectedTask = if ($value -eq 4) { 1 } else { 0 }
        Assert-Trace ($orderedEvents[$index] -match " -- Task: '$expectedTask'$") `
            "Expected EtlDump to report Task $expectedTask for provider event ${value}: $($orderedEvents[$index])"
    }
}

Invoke-TraceTest -AppPath $AppPath -EtlDumpPath $EtlDumpPath -Check $check
