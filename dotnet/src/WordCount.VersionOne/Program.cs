using WordCount.Cli;

namespace WordCount.VersionOne
{
    internal class Program
    {
        internal static int Main(string[] arguments) =>
            CommandLine.Invoke(arguments, (file) =>
            {
                //var whitespace = new HashSet<byte>
                //{
                //        (byte)' ', (byte)'\t', (byte)'\n', (byte)'\r', (byte)'\v', (byte)'\f'
                //};

                static bool IsWhitespace(byte c)
                {
                    switch (c)
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

                bool previousWhitespace = true;
                int total = 0;

                using var fileStream = new FileStream(
                    file.FullName,
                    FileMode.Open,
                    FileAccess.Read,
                    FileShare.Read,
                    bufferSize: 1 << 20,
                    options: FileOptions.SequentialScan);
                //using (FileStream fs = File.OpenRead(file.FullName))
                int b;
                while ((b = fileStream.ReadByte()) != -1)
                {
                    //bool currentWhitespace = whitespace.Contains((byte)b);
                    bool currentWhitespace = IsWhitespace((byte)b);
                    if (!currentWhitespace && previousWhitespace)
                        total++;
                    previousWhitespace = currentWhitespace;
                }

                Console.WriteLine(total);
            });
    }
}