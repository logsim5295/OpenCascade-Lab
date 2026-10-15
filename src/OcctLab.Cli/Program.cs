using System.Globalization;
using OcctLab.Runtime;

namespace OcctLab.Cli;

internal static class Program
{
    private static int Main(string[] args)
    {
        // OcctProxy 타입이 JIT 컴파일되기 전에 반드시 실행해야 한다 (OcctEnvironment 주석 참고).
        try
        {
            OcctEnvironment.Initialize();
        }
        catch (OcctNotFoundException ex)
        {
            Console.Error.WriteLine(ex.Message);
            return 2;
        }

        var outputDir = args.Length > 0 ? args[0] : Path.Combine(Environment.CurrentDirectory, "output");
        try
        {
            Demo.Run(outputDir);
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"FAILED: {ex.GetType().Name}: {ex.Message}");
            return 1;
        }
    }
}
