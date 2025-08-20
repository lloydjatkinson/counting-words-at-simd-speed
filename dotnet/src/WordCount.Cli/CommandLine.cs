using System.Diagnostics.CodeAnalysis;
using System.IO;

using static System.Runtime.InteropServices.JavaScript.JSType;

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

        public static void LogError(string error)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.Error.WriteLine(error);
            Console.ResetColor();
        }

        public static int Invoke(string[] arguments, Action<FileInfo> run)
        {
            if (!TryParse(arguments, out var file, out var error))
            {
                LogError(error);
                return 1;
            }

            if (!file.Exists)
            {
                LogError($"File not found: {file.FullName}");
                return 1;
            }

            try
            {
                run(file);
                return 0;
            }
            catch (Exception exception)
            {
                LogError(exception.Message);
                return 1;
            }
        }
    }
}