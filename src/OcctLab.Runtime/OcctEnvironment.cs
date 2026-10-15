using System.Reflection;

namespace OcctLab.Runtime;

/// <summary>
/// 네이티브 OCCT 바이너리를 찾지 못했을 때 던져진다.
/// </summary>
public sealed class OcctNotFoundException : Exception
{
    public OcctNotFoundException(string message) : base(message) { }
}

/// <summary>
/// 네이티브 OCCT DLL(TKernel.dll, TKOpenGl.dll, freetype.dll, ...)이 있는 폴더를 PATH 앞에
/// 추가하여 현재 프로세스에서 로드할 수 있게 한다.
///
/// 중요: JIT가 OcctProxy.dll의 타입을 건드리기 전에 <see cref="Initialize"/>를 호출해야 한다.
/// OcctProxy.dll이 로드되면 Windows 로더가 네이티브 임포트를 해석하므로, 이 호출은
/// OcctProxy 타입을 직접 참조하지 않는 메서드(예: Program.Main) 안에 두어야 한다.
/// </summary>
public static class OcctEnvironment
{
    private static readonly object Gate = new();
    private static bool _initialized;

    /// <summary>PATH 앞에 추가된 폴더 목록 (첫 번째 폴더에 TKernel.dll이 있다).</summary>
    public static IReadOnlyList<string> RuntimeDirectories { get; private set; } = Array.Empty<string>();

    /// <summary>OCCT 패키지 루트 (inc/와 win64/를 포함하는 폴더).</summary>
    public static string? OcctRoot { get; private set; }

    /// <summary>"Debug" 또는 "Release": 이 빌드가 링크된 OCCT 바이너리(bind/ 또는 bin/) 구분.</summary>
    public static string Configuration { get; private set; } = "Release";

    public static void Initialize()
    {
        lock (Gate)
        {
            if (_initialized) return;

            var meta = typeof(OcctEnvironment).Assembly
                .GetCustomAttributes<AssemblyMetadataAttribute>()
                .Where(a => a.Value is not null)
                .ToDictionary(a => a.Key, a => a.Value!, StringComparer.OrdinalIgnoreCase);

            Configuration = meta.GetValueOrDefault("OcctConfiguration", "Release");
            var dirs = ResolveDirectories(meta);

            var occtBin = dirs.FirstOrDefault(d => File.Exists(Path.Combine(d, "TKernel.dll")));
            if (occtBin is null)
            {
                throw new OcctNotFoundException(
                    "OCCT native binaries (TKernel.dll) were not found." + Environment.NewLine +
                    "  - Run  scripts/setup-occt.ps1  to download OCCT V8.0.1 into third_party/, or" + Environment.NewLine +
                    "  - set  OCCT_ROOT  to a folder containing win64\\vc14\\bin (and OCCT_RUNTIME_DIRS for 3rd-party DLLs)." + Environment.NewLine +
                    "Searched: " + (dirs.Count == 0 ? "(nothing)" : string.Join("; ", dirs)));
            }

            var currentPath = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
            var newPath = string.Join(Path.PathSeparator, dirs.Append(currentPath));
            Environment.SetEnvironmentVariable("PATH", newPath);

            OcctRoot = Path.GetFullPath(Path.Combine(occtBin, "..", "..", ".."));
            if (string.IsNullOrEmpty(Environment.GetEnvironmentVariable("CASROOT")))
            {
                Environment.SetEnvironmentVariable("CASROOT", OcctRoot);
            }

            RuntimeDirectories = dirs;
            _initialized = true;
        }
    }

    private static List<string> ResolveDirectories(Dictionary<string, string> meta)
    {
        var candidates = new List<string>();
        var binFolder = string.Equals(Configuration, "Debug", StringComparison.OrdinalIgnoreCase) ? "bind" : "bin";

        // 1) 사용자가 환경 변수로 명시적으로 지정한 경로.
        var envRoot = Environment.GetEnvironmentVariable("OCCT_ROOT");
        if (!string.IsNullOrWhiteSpace(envRoot))
        {
            candidates.Add(Path.Combine(envRoot, "win64", "vc14", binFolder));
        }
        var envDirs = Environment.GetEnvironmentVariable("OCCT_RUNTIME_DIRS");
        if (!string.IsNullOrWhiteSpace(envDirs))
        {
            candidates.AddRange(Split(envDirs));
        }

        // 2) Directory.Build.props에서 빌드 시점에 구워 넣은 값.
        if (meta.TryGetValue("OcctRuntimeDirs", out var baked))
        {
            candidates.AddRange(Split(baked));
        }

        // 3) 폴백: exe 위치에서 상위로 올라가며 third_party/occt를 찾는다 (저장소를 옮긴 뒤에 유용).
        for (var dir = new DirectoryInfo(AppContext.BaseDirectory); dir is not null; dir = dir.Parent)
        {
            var probe = Path.Combine(dir.FullName, "third_party", "occt", "win64", "vc14", binFolder);
            if (Directory.Exists(probe))
            {
                candidates.Add(probe);
                var thirdParty = Path.Combine(dir.FullName, "third_party", "3rdparty");
                if (Directory.Exists(thirdParty))
                {
                    candidates.AddRange(Directory.GetDirectories(thirdParty)
                        .Select(p => Path.Combine(p, "bin"))
                        .Where(Directory.Exists));
                }
                break;
            }
        }

        return candidates
            .Select(p => Path.GetFullPath(p.Trim()))
            .Where(Directory.Exists)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private static IEnumerable<string> Split(string list) =>
        list.Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
}
