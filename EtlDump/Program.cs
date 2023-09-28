using Microsoft.Diagnostics.Tracing;

namespace EtlDump
{
    internal class Program
    {

        static void Main(string[] args)
        {
            using (var source = new ETWTraceEventSource(@"etl-demo.etl"))
            {
                // setup the callbacks
                source.Dynamic.All += (TraceEvent ev) => {
                    if (ev.ProviderName != "EtlProviderApp") return;

                    Console.Write($"{ev.EventName} -- (");

                    foreach (string name in ev.PayloadNames)
                    {
                        Console.Write($" '{name}': '{ev.PayloadByName(name)}' ({ev.PayloadByName(name).GetType()})");
                    }

                    int t = (int)ev.Task;
                    Console.WriteLine($" ) -- Task: '{t}'");
                };

                // iterate over the file, calling the callbacks.  
                source.Process();
            }
        }

    }
}