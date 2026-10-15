using System.Windows.Forms;
using OcctProxy;
// 이 프로젝트에서 WPF(System.Windows.*) using은 전역이므로, 여기서 쓰려는 WinForms/GDI 타입을 명시적으로 고정한다.
using Color = System.Drawing.Color;
using Control = System.Windows.Forms.Control;
using MouseEventArgs = System.Windows.Forms.MouseEventArgs;
using Point = System.Drawing.Point;

namespace OcctLab.Viewer;

/// <summary>
/// <see cref="OcctViewer"/>를 소유하는 WinForms 컨트롤 (이 컨트롤의 HWND 위에 OpenGL 뷰가 그려진다).
/// 마우스: 왼쪽 드래그 = 회전, 가운데/오른쪽 드래그 = 이동, 휠 = 줌, 클릭 = 선택, Shift+클릭 = 다중 선택.
/// </summary>
public sealed class OcctViewControl : Control
{
    private Point _last;
    private bool _rotating;
    private bool _panning;
    private bool _moved;

    public OcctViewer Viewer { get; } = new();

    public event EventHandler? SelectionChanged;

    public OcctViewControl()
    {
        SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.Opaque | ControlStyles.Selectable, true);
        SetStyle(ControlStyles.OptimizedDoubleBuffer | ControlStyles.SupportsTransparentBackColor, false);
        BackColor = Color.Black;
        TabStop = true;
    }

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        if (DesignMode || Viewer.IsInitialized) return;
        if (!Viewer.Init(Handle))
        {
            throw new InvalidOperationException("Failed to create the OCCT OpenGL view (is a GPU/OpenGL driver available?).");
        }
    }

    protected override void OnPaintBackground(PaintEventArgs pevent)
    {
        // 클라이언트 영역 전체를 OCCT가 그린다.
    }

    protected override void OnPaint(PaintEventArgs e) => Viewer.Redraw();

    protected override void OnResize(EventArgs e)
    {
        base.OnResize(e);
        Viewer.Resize();
        Invalidate();
    }

    protected override void OnMouseDown(MouseEventArgs e)
    {
        base.OnMouseDown(e);
        Focus();
        _last = e.Location;
        _moved = false;
        if (e.Button == MouseButtons.Left)
        {
            _rotating = true;
            Viewer.StartRotation(e.X, e.Y);
        }
        else if (e.Button is MouseButtons.Middle or MouseButtons.Right)
        {
            _panning = true;
        }
    }

    protected override void OnMouseMove(MouseEventArgs e)
    {
        base.OnMouseMove(e);
        if (_rotating)
        {
            Viewer.Rotate(e.X, e.Y);
            _moved = true;
        }
        else if (_panning)
        {
            Viewer.Pan(e.X - _last.X, _last.Y - e.Y);
            _last = e.Location;
            _moved = true;
        }
        else
        {
            Viewer.MoveTo(e.X, e.Y);
        }
    }

    protected override void OnMouseUp(MouseEventArgs e)
    {
        base.OnMouseUp(e);
        if (e.Button == MouseButtons.Left && !_moved)
        {
            Viewer.MoveTo(e.X, e.Y);
            if ((ModifierKeys & Keys.Shift) == Keys.Shift) Viewer.ShiftSelect();
            else Viewer.Select();
            SelectionChanged?.Invoke(this, EventArgs.Empty);
        }
        _rotating = false;
        _panning = false;
    }

    protected override void OnMouseWheel(MouseEventArgs e)
    {
        base.OnMouseWheel(e);
        Viewer.ZoomAt(e.X, e.Y, e.Delta > 0 ? 1.15 : 1.0 / 1.15);
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            Viewer.Dispose();
        }
        base.Dispose(disposing);
    }
}
