# Etl Demo
Demo app `ProviderApp.exe` pushing some example non-trivial trace events.

### Projects
- `EtlDump` - a minimal CSharp application to read a Windows Trace Logging (etl) file.
- `ProviderApp` - a Cpp application emitting some trace events, some with non-trivial payload configurations

### Further information
- The [Event Tracing for Windows (ETW)](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/event-tracing-for-windows) enables the consistent, straightforward capture of kernel and application events for performance analysis and behavior analysis.
- [Windows Performance Analyzer (WPA)](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/windows-performance-analyzer) presents the information that ETW collects in graphs and tables.
- [Windows Performance Recorder (WPR)](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/windows-performance-recorder) records system and application events, writing event trace logs (ETL).

This project can be used to reproduce specific setups in a small repeatable manner, to test further analysis tools.


## How-to Record an Event Trace

When you just run `ProviderApp.exe` it will print a couple of lines out to the console, and it will also write a couple of trace events.
To record these trace events, you can use the [Windows Performance Recorder (WPR)](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/windows-performance-recorder).

You can start the recording using the provided profile file `etl-demo.wprp`.

Run the Powershell script file [`record-wpr.ps1`](.\record-wpr.ps1) in an elevated shell to record an ETL file automatically.


## How-to Build
You need Visual Studio 2022 or newer, with the Desktop CSharp tools and the Desktop Cpp tools installed.
Community edition should work just fine.

Open the solution [`EtlDemo.sln`](.\EtlDemo.sln) in Visual Studio.

You should be able to build the project right away.

Restoring Nuget packets from some projects might need to be triggered explicitly if building fails.

If you are having trouble, you might need to install a Windows SDK or adjust the respective parameters in the project settings.


## License
This project is freely available as open source under the terms of the [MIT License](.\LICENSE)

> Copyright (c) 2023-2024 Sebastian Grottel
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.
