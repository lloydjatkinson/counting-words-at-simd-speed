using WordCount.Cli;

namespace WordCount.VersionTwo
{
    internal class Program
    {
        internal static int Main(string[] arguments) =>
            CommandLine.Invoke(arguments, static (file) =>
            {
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
                    bufferSize: 4096,
                    options: FileOptions.SequentialScan);
                //using (FileStream fs = File.OpenRead(file.FullName))
                int @byte;
                while ((@byte = fileStream.ReadByte()) != -1)
                {
                    //bool currentWhitespace = whitespace.Contains((byte)byte);
                    bool currentWhitespace = IsWhitespace((byte)@byte);
                    if (!currentWhitespace && previousWhitespace)
                    {
                        total++;
                    }
                    previousWhitespace = currentWhitespace;
                }

                Console.WriteLine(total);
            });
    }
}