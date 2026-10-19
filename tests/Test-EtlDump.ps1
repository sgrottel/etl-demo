param(
    [Parameter(Mandatory = $true)]
    [string]$EtlDumpPath
)

. "$PSScriptRoot\Common.ps1"

$dump = (Get-Item -LiteralPath $EtlDumpPath).FullName
Assert-Trace ([IO.Path]::GetExtension($dump) -eq '.exe' -and (Test-Path -LiteralPath $dump -PathType Leaf)) `
    "Expected an EtlDump executable: '$dump'."

$managedEvents = @("WriteEventStringCS -- ( 'msg': 'Start' (System.String) ) -- Task: '0'")
for ($a = 0; $a -lt 4; $a++)
{
    for ($b = 1; $b -le 3; $b++)
    {
        $managedEvents += "WriteEventIntsCS -- ( 'i1': '$a' (System.Int32) 'i2': '$b' (System.Int32) ) -- Task: '0'"
    }
}
$managedEvents += "WriteEventStringCS -- ( 'msg': 'End' (System.String) ) -- Task: '0'"

$nativeEvents = foreach ($pair in @(@(1, 5), @(1, 2), @(6, 5), @(3, 2), @(3, 4), @(5, 6), @(6, 1), @(6, 4), @(5, 5), @(6, 5)))
{
    "WriteEventInts -- ( 'i1': '$($pair[0])' (System.Int32) 'i2': '$($pair[1])' (System.Int32) ) -- Task: '0'"
}
$fixtures = [ordered]@{
    'DemoAppCS.etl' = $managedEvents
    'MinimalApp.etl' = $nativeEvents
}

$directory = Join-Path ([IO.Path]::GetTempPath()) ('EtlDumpTest-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $directory | Out-Null
$passed = $false
try
{
    foreach ($name in $fixtures.Keys)
    {
        $fixtureDirectory = Join-Path $directory ([IO.Path]::GetFileNameWithoutExtension($name))
        New-Item -ItemType Directory -Path $fixtureDirectory | Out-Null
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination (Join-Path $fixtureDirectory 'etl-demo.etl')

        $output = Invoke-TestExecutable $dump $fixtureDirectory 'dump'
        Assert-Trace (-not [string]::IsNullOrWhiteSpace($output)) "EtlDump returned no events for $name."
        $actual = @($output.TrimEnd([char[]]"`r`n") -split '\r?\n')
        $expected = $fixtures[$name]
        Assert-Trace ($actual.Count -eq $expected.Count) `
            "Expected $($expected.Count) events for $name, got $($actual.Count)."
        for ($index = 0; $index -lt $expected.Count; $index++)
        {
            Assert-Trace ($actual[$index] -ceq $expected[$index]) `
                "$name event $($index + 1) differs.`nExpected: $($expected[$index])`nActual:   $($actual[$index])"
        }
        Write-Host "PASS: $name ($($expected.Count) events)"
    }
    $passed = $true
}
finally
{
    if ($passed)
    {
        Remove-Item -LiteralPath $directory -Recurse -Force
    }
    else
    {
        Write-Warning "EtlDump test failed. Process logs are retained in '$directory'."
    }
}
