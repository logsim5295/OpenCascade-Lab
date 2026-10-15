using System.Windows;
using OcctLab.Runtime;

namespace OcctLab.Viewer;

public partial class App : Application
{
    // WPF가 생성한 Main()보다 먼저 실행된다: MainWindow가 OcctProxy 타입을 로드하기 전에
    // OCCT DLL 폴더를 PATH에 넣는다.
    static App()
    {
        try
        {
            OcctEnvironment.Initialize();
        }
        catch (OcctNotFoundException ex)
        {
            MessageBox.Show(ex.Message, "OCCT not found", MessageBoxButton.OK, MessageBoxImage.Error);
            Environment.Exit(2);
        }
    }

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);
        DispatcherUnhandledException += (_, args) =>
        {
            MessageBox.Show(args.Exception.Message, args.Exception.GetType().Name,
                MessageBoxButton.OK, MessageBoxImage.Error);
            args.Handled = true;
        };
    }
}
