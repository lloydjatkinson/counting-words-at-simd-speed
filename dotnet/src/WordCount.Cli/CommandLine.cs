using System.Diagnostics.CodeAnalysis;
using System.IO;

namespace WordCount.Cli
{
    public static class CommandLine
    {
        public static bool TryParse(
            string[] arguments,
            [NotNullWhen(true)] out FileInfo? file,
            [NotNullWhen(false)] out string? error)
        {
            if (arguments is ["--file", var value] && !string.IsNullOrWhiteSpace(value))
            {
                file = new FileInfo(value);
                error = null;
                return true;
            }

            file = null;
            error = "Option --file is required (usage: --file <path>).";
            return false;
        }

        public static int Invoke(string[] arguments, Action<FileInfo> run)
        {
            if (!TryParse(arguments, out var file, out var error))
            {
                Console.Error.WriteLine(error);
                return 1;
            }

            if (!file.Exists)
            {
                Console.Error.WriteLine($"File not found: {file.FullName}");
                return 1;
            }

            try
            {
                run(file);
                return 0;
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Error.WriteLine(ex.Message);
                Console.ResetColor();
                return 1;
            }
        }
    }
}