using System.Globalization;
using OcctLab.Runtime;
using OcctProxy;

namespace OcctLab.Cli;

/// <summary>
/// 헤드리스 모델링 시연: 기본 도형 -> 불리언 -> 필렛 -> 측정 -> 내보내기 -> 다시 가져오기.
/// 여기 코드는 모두 순수 C#이며, OCCT 호출은 C++/CLI OcctProxy 어셈블리를 거친다.
/// </summary>
internal static class Demo
{
    public static void Run(string outputDir)
    {
        var inv = CultureInfo.InvariantCulture;
        OcctInfo.SetConsoleOutput(false);   // OCCT가 stdout에 찍는 STEP 변환 통계를 끈다
        Console.WriteLine($"OCCT version      : {OcctInfo.Version}");
        Console.WriteLine($"OCCT binaries     : {OcctEnvironment.RuntimeDirectories[0]}  ({OcctEnvironment.Configuration})");
        Console.WriteLine();

        // 1. 기본 도형 --------------------------------------------------------
        using var box = ShapeFactory.MakeBox(100, 60, 40);
        using var hole = ShapeFactory.MakeCylinder(50, 30, -5, 0, 0, 1, radius: 15, height: 50);
        Console.WriteLine($"Box               : {box}");
        Console.WriteLine($"Cylinder          : {hole}");

        // 2. 불리언 + 필렛 ---------------------------------------------------
        using var plate = ShapeFactory.Cut(box, hole);
        using var rounded = ShapeFactory.FilletAllEdges(plate, 3.0);
        Console.WriteLine($"Box - Cylinder    : {plate}");
        Console.WriteLine($"Filleted (r=3)    : {rounded}");
        Console.WriteLine();

        // 3. 측정 -------------------------------------------------------------
        var bbox = rounded.GetBoundingBox();
        var com = rounded.CenterOfMass();
        Console.WriteLine($"Valid (BRepCheck) : {rounded.IsValid()}");
        Console.WriteLine($"Volume            : {rounded.Volume.ToString("F3", inv)}  (box {box.Volume.ToString("F1", inv)}, hole {(Math.PI * 15 * 15 * 40).ToString("F1", inv)})");
        Console.WriteLine($"Surface area      : {rounded.SurfaceArea.ToString("F3", inv)}");
        Console.WriteLine($"Bounding box      : {bbox}");
        Console.WriteLine($"Center of mass    : ({com[0].ToString("F3", inv)}, {com[1].ToString("F3", inv)}, {com[2].ToString("F3", inv)})");
        Console.WriteLine();

        // 4. 내보내기 ---------------------------------------------------------
        Directory.CreateDirectory(outputDir);
        var stepPath = Path.Combine(outputDir, "plate.step");
        var brepPath = Path.Combine(outputDir, "plate.brep");
        var stlPath = Path.Combine(outputDir, "plate.stl");
        ShapeIO.ExportStep(rounded, stepPath);
        ShapeIO.ExportBrep(rounded, brepPath);
        ShapeIO.ExportStl(rounded, stlPath, linearDeflection: 0.05, ascii: false);
        foreach (var p in new[] { stepPath, brepPath, stlPath })
        {
            Console.WriteLine($"Exported          : {p}  ({new FileInfo(p).Length:N0} bytes)");
        }
        Console.WriteLine();

        // 5. 왕복 검증 (내보낸 STEP 다시 읽기) -------------------------------
        using var reloaded = ShapeIO.ImportStep(stepPath);
        var delta = Math.Abs(reloaded.Volume - rounded.Volume);
        Console.WriteLine($"Re-imported STEP  : {reloaded}");
        Console.WriteLine($"Volume difference : {delta.ToString("E2", inv)}  -> {(delta < 1e-3 ? "OK" : "MISMATCH")}");

        // 6. 오류 처리: OCCT 실패는 OcctException으로 드러난다 ---------------
        try
        {
            ShapeFactory.FilletAllEdges(box, 1000.0);
        }
        catch (OcctException ex)
        {
            Console.WriteLine($"Expected failure  : {ex.Message}");
        }
    }
}
