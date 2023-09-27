#
# Record ETL file with WPR
#

if (-Not (Get-Command wpr.exe -ErrorAction SilentlyContinue))
{
	Write-Error "WPR.exe -- Windows Performance Recorder -- not found."
	exit
}

$Env:Path += ";."
$etldemo = (Get-Command EtlProviderApp.exe -ErrorAction SilentlyContinue)
if (-Not $etldemo)
{
	Write-Error "EtlProviderApp.exe not found. Please run this script from within one of the binary directories."
	exit
}

Write-Host "Starting Trace..."
(wpr -cancel 2>&1) | Out-Null
wpr.exe -start (Join-Path $PSScriptRoot "etl-demo.wprp")

Write-Host "Running App..."
& $etldemo

Write-Host "Completing Trace..."
wpr -stop "etl-demo.etl"

Write-Host "Done."
