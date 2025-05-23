using System.Diagnostics.Tracing;

namespace DemoAppCS
{
    internal class Program
    {
        static void Main(string[] args)
        {
            MyEventSource.eventz.WriteEventStringCS("Start");
            for (int a = 0; a < 4; ++a)
            {
                for (int b = 1; b <= 3; ++b)
                {
                    MyEventSource.eventz.WriteEventIntsCS(a, b);
                }
            }
            MyEventSource.eventz.WriteEventStringCS("End");
        }
    }

    [EventSource(Name = "EtlProviderApp")]
    public sealed class MyEventSource : EventSource
    {
        public static readonly MyEventSource eventz = new MyEventSource();

        public MyEventSource() : base(EventSourceSettings.EtwSelfDescribingEventFormat)
        {
        }

        [Event(1, Level = EventLevel.Verbose)]
        public void WriteEventIntsCS(int i1, int i2)
        {
            WriteEvent(1, i1, i2);
        }

        [Event(2, Level = EventLevel.Verbose)]
        public void WriteEventStringCS(string msg)
        {
            WriteEvent(2, msg);
        }
    }
}
