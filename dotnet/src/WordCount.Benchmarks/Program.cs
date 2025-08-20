using BenchmarkDotNet.Running;

namespace WordCount.Benchmarks
{
    internal class Program
    {
        public static void Main(string[] arguments) =>
            BenchmarkSwitcher.FromAssembly(typeof(Program).Assembly).Run(arguments);
    }
}
