Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-Trace
{
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition)
    {
        throw $Message
    }
}

function Get-TraceEvents
{
    param(
        [string]$Output,
        [string]$Name
    )

    $pattern = '^' + [regex]::Escape($Name) + ' -- \('
    $Output -split '\r?\n' | Where-Object { $_ -match $pattern }
}

function Invoke-TestExecutable
{
    param(
        [string]$Path,
        [string]$Directory,
        [string]$LogName,
        [int]$StopAfterSeconds = 0,
        [scriptblock]$OnStarted,
        [scriptblock]$BeforeStop
    )

    $stdoutPath = Join-Path $Directory "$LogName.stdout.txt"
    $stderrPath = Join-Path $Directory "$LogName.stderr.txt"
    $process = Start-Process -FilePath $Path -WorkingDirectory $Directory -NoNewWindow `
        -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath -PassThru
    try
    {
        if ($OnStarted)
        {
            & $OnStarted $process
        }

        if ($StopAfterSeconds -gt 0)
        {
            if ($process.WaitForExit($StopAfterSeconds * 1000))
            {
                throw "$LogName exited before the timed stop (exit code $($process.ExitCode))."
            }

            if ($BeforeStop)
            {
                & $BeforeStop $process
            }

            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
        }
        else
        {
            if (-not $process.WaitForExit(30000))
            {
                throw "$LogName did not exit within 30 seconds."
            }

            $process.WaitForExit()
            if ($process.ExitCode -ne 0)
            {
                throw "$LogName exited with code $($process.ExitCode). See '$stderrPath'."
            }
        }

        $stderr = Get-Content -LiteralPath $stderrPath -Raw
        if (-not [string]::IsNullOrWhiteSpace($stderr))
        {
            throw "$LogName wrote to stderr: $stderr"
        }

        $stdout = Get-Content -LiteralPath $stdoutPath -Raw
        if ($stdout -match '(?i)\b(failed|error)\b')
        {
            throw "$LogName reported an error: $stdout"
        }

        return $stdout
    }
    finally
    {
        if (-not $process.HasExited)
        {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
        }
        $process.Dispose()
    }
}

function Invoke-WprCommand
{
    param(
        [string]$Path,
        [string[]]$Arguments
    )

    $output = & $Path @Arguments 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0)
    {
        throw "WPR $($Arguments -join ' ') failed (exit code $LASTEXITCODE): $output"
    }
    Write-Host $output.Trim()
}

function Invoke-TraceTest
{
    param(
        [string]$AppPath,
        [string]$EtlDumpPath,
        [scriptblock]$Check,
        [int]$StopAfterSeconds = 0,
        [ValidateRange(0, 30)]
        [int]$TraceStartDelaySeconds = 0
    )

    if ($TraceStartDelaySeconds -gt 0 -and $StopAfterSeconds -le 0)
    {
        throw 'Starting the app before the trace requires a timed stop.'
    }

    $app = (Get-Item -LiteralPath $AppPath).FullName
    $dump = (Get-Item -LiteralPath $EtlDumpPath).FullName
    foreach ($path in @($app, $dump))
    {
        if ([IO.Path]::GetExtension($path) -ne '.exe' -or -not (Test-Path -LiteralPath $path -PathType Leaf))
        {
            throw "Expected an executable file: '$path'."
        }
    }

    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator))
    {
        throw 'ETW capture requires an elevated PowerShell session. Run this test as administrator.'
    }

    $wpr = (Get-Command wpr.exe -CommandType Application | Select-Object -First 1).Source
    $profile = Join-Path (Split-Path $PSScriptRoot -Parent) 'etl-demo.wprp'
    $instance = 'EtlDemoTest-' + [guid]::NewGuid().ToString('N')
    $directory = Join-Path ([IO.Path]::GetTempPath()) $instance
    New-Item -ItemType Directory -Path $directory | Out-Null
    $traceState = @{ Started = $false }
    $tracePath = Join-Path $directory 'etl-demo.etl'
    $appStdoutPath = Join-Path $directory 'app.stdout.txt'
    $startTrace = {
        Invoke-WprCommand $wpr @('-start', $profile, '-filemode', '-instancename', $instance)
        $traceState.Started = $true
    }
    $stopTrace = {
        Invoke-WprCommand $wpr @('-stop', $tracePath, '-skipPdbGen', '-instancename', $instance)
        $traceState.Started = $false
    }
    $passed = $false

    try
    {
        if ($TraceStartDelaySeconds -gt 0)
        {
            $onStarted = {
                param($process)

                if ($process.WaitForExit($TraceStartDelaySeconds * 1000))
                {
                    throw "The app exited before tracing started (exit code $($process.ExitCode))."
                }
                $traceState.BeforeStartOutput = Get-Content -LiteralPath $appStdoutPath -Raw
                & $startTrace
            }
            $beforeStop = {
                param($process)

                $traceState.BeforeStopOutput = Get-Content -LiteralPath $appStdoutPath -Raw
                & $stopTrace
                $traceState.AfterStopOutput = Get-Content -LiteralPath $appStdoutPath -Raw
                if ($process.WaitForExit(2000))
                {
                    throw "The app exited after tracing stopped (exit code $($process.ExitCode))."
                }
            }
            $appOutput = Invoke-TestExecutable $app $directory 'app' $StopAfterSeconds `
                -OnStarted $onStarted -BeforeStop $beforeStop
        }
        else
        {
            & $startTrace
            $appOutput = Invoke-TestExecutable $app $directory 'app' $StopAfterSeconds
            & $stopTrace
        }
        Write-Host $appOutput

        # EtlDump reads etl-demo.etl from its working directory.
        $output = Invoke-TestExecutable $dump $directory 'dump'
        Assert-Trace (-not [string]::IsNullOrWhiteSpace($output)) 'EtlDump returned no events.'
        Write-Host $output
        & $Check $output $appOutput $traceState

        $passed = $true
        Write-Host "PASS: $app"
    }
    finally
    {
        try
        {
            if ($traceState.Started)
            {
                Invoke-WprCommand $wpr @('-cancel', '-instancename', $instance)
            }
        }

        finally
        {
            if ($passed)
            {
                Remove-Item -LiteralPath $directory -Recurse -Force
            }
            else
            {
                Write-Warning "Test failed. Trace and process logs are retained in '$directory'."
            }
        }
    }
}

function Assert-ContinuousConsoleOutput
{
    param(
        [string]$AppOutput,
        [string]$TraceOutput,
        [switch]$LateTrace,
        [hashtable]$TraceState
    )

    $lines = @($AppOutput -split '\r?\n' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    Assert-Trace (@($lines -ceq 'Rolling Dice...').Count -eq 1) 'Missing or duplicated dice startup message.'
    Assert-Trace (@($lines -ceq 'Press any key to leave.').Count -eq 1) 'Missing or duplicated keyboard prompt.'
    Assert-Trace (@($lines -ceq '# Event Provider ENABLE').Count -ge 1) 'Missing provider enable notification.'

    $consoleStats = [Collections.Generic.List[string]]::new()
    $previous = @(0L, 0L, 0L, 0L, 0L, 0L)
    $previousTotal = 0L
    foreach ($line in $lines)
    {
        # Other trace sessions can generate any number of these notifications.
        if ($line -cmatch '^# Event Provider (ENABLE|DISABLE|CAPTURE STATE)$' -or
            $line -ceq 'Rolling Dice...' -or $line -ceq 'Press any key to leave.')
        {
            continue
        }

        Assert-Trace ($line -cmatch '^Dice stats\.  1: ([0-9]+)  2: ([0-9]+)  3: ([0-9]+)  4: ([0-9]+)  5: ([0-9]+)  6: ([0-9]+)$') `
            "Unexpected or malformed console output: $line"
        $current = @(1..6 | ForEach-Object { [long]$Matches[$_] })
        $total = 0L
        for ($index = 0; $index -lt 6; $index++)
        {
            Assert-Trace ($current[$index] -ge $previous[$index]) 'Console dice counters decreased.'
            $total += $current[$index]
        }
        Assert-Trace ($total -gt $previousTotal) 'Console dice statistics did not advance.'
        $consoleStats.Add(($current -join ','))
        $previous = $current
        $previousTotal = $total
    }
    Assert-Trace ($consoleStats.Count -ge 1) 'Missing console dice statistics.'

    $firstIndex = -1
    $lastIndex = -1
    foreach ($event in @(Get-TraceEvents $TraceOutput 'DiceStats'))
    {
        $current = @(0L, 0L, 0L, 0L, 0L, 0L)
        for ($die = 1; $die -le 6; $die++)
        {
            Assert-Trace ($event -match "'D$die': '([0-9]+)' \(System.UInt32\)") "Invalid trace counter D$die."
            $current[$die - 1] = [long]$Matches[1]
        }
        $index = $consoleStats.IndexOf(($current -join ','))
        Assert-Trace ($index -gt $lastIndex) "Trace statistics are missing or out of order in console output: $event"
        if ($firstIndex -lt 0)
        {
            $firstIndex = $index
        }
        $lastIndex = $index
    }
    Assert-Trace ($firstIndex -ge 0) 'No trace statistics could be compared with console output.'

    if ($LateTrace)
    {
        Assert-Trace ($null -ne $TraceState) 'Missing console snapshots at trace boundaries.'
        $beforeStart = @($TraceState.BeforeStartOutput -split '\r?\n')
        $beforeStop = @($TraceState.BeforeStopOutput -split '\r?\n')
        $afterStop = @($TraceState.AfterStopOutput -split '\r?\n')
        $previousEnables = @($beforeStart -ceq '# Event Provider ENABLE').Count
        Assert-Trace (@($beforeStop -ceq '# Event Provider ENABLE').Count -gt $previousEnables) `
            'No new provider enable notification appeared during the capture window.'
        # DISABLE is not guaranteed while another session still enables the provider.
        Assert-Trace (@($beforeStart -match '^Dice stats\.').Count -ge 1) 'Missing console statistics before tracing started.'
        Assert-Trace ($consoleStats.Count -gt @($afterStop -match '^Dice stats\.').Count) `
            'The app did not print new statistics after tracing stopped.'
        Assert-Trace ($firstIndex -gt 0) 'Missing console statistics from before the capture window.'
        Assert-Trace ($lastIndex -lt $consoleStats.Count - 1) 'Missing console statistics from after the capture window.'
    }
}
