using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Diagnosers;

namespace WordCount.Benchmarks.Micro
{
    /// <summary>
    /// Compares a HashSet lookup against a switch statement for checking
    /// whether a byte matches a small set of values. This optimization
    /// provides a significant speedup in VersionOne by removing the overhead
    /// of a method call.
    /// </summary>
    [SimpleJob]
    [MemoryDiagnoser]
    [HardwareCounters(
        HardwareCounter.BranchMispredictions,
        HardwareCounter.BranchInstructions)]
    public class HashSetComparedToSwitch
    {
        private readonly HashSet<byte> whitespace = new HashSet<byte>
        {
            (byte)' ', (byte)'\t', (byte)'\n', (byte)'\r', (byte)'\v', (byte)'\f'
        };

        private bool IsWhitespaceUsingSwitch(byte character)
        {
            switch (character)
            {
                case (byte)' ':
                case (byte)'\n':
                case (byte)'\r':
                case (byte)'\t':
                case (byte)'\v':
                case (byte)'\f':
                    return true;
                default:
                    return false;
            }
        }

        [Params(1_000, 10_000, 100_000, 1_000_000)]
        public int Size { get; set; }

        private byte[] data = Array.Empty<byte>();

        [GlobalSetup]
        public void Setup()
        {
            this.data = new byte[this.Size];
            var rnd = new Random(42); // deterministic

            var alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
            var ws = new[] { (byte)' ', (byte)'\n', (byte)'\r', (byte)'\t', (byte)'\v', (byte)'\f' };

            int i = 0;
            while (i < this.data.Length)
            {
                int wordLen = 1 + rnd.Next(30);

                for (int k = 0; k < wordLen && i < this.data.Length; k++)
                {
                    this.data[i++] = (byte)alphabet[rnd.Next(alphabet.Length)];
                }

                if (i < this.data.Length)
                {
                    this.data[i++] = ws[rnd.Next(ws.Length)];
                }
            }
        }


        private bool IsWhitespaceUsingHashSet(byte character) => this.whitespace.Contains(character);

        [Benchmark]
        public int HashSet()
        {
            int count = 0;
            for (int i = 0; i < this.data.Length; i++)
                if (this.IsWhitespaceUsingHashSet(this.data[i]))
                    count++;
            return count;
        }

        [Benchmark]
        public int Switch()
        {
            int count = 0;
            for (int i = 0; i < this.data.Length; i++)
                if (this.IsWhitespaceUsingSwitch(this.data[i]))
                    count++;
            return count;
        }
    }
}