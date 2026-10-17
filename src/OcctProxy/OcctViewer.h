// OcctViewer.h - Win32 윈도 핸들(HWND)에 바인딩되는 OpenGL 3D 뷰어.
#pragma once

#include "Common.h"
#include "OcctShape.h"

#include <Aspect_DisplayConnection.hxx>
#include <Aspect_GradientFillMethod.hxx>
#include <Aspect_TypeOfTriedronPosition.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <WNT_Window.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <NCollection_List.hxx>
#include <AIS_DisplayMode.hxx>
#include <AIS_SelectionScheme.hxx>
#include <AIS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <TopLoc_Location.hxx>

namespace OcctProxy
{
  public enum class DisplayMode
  {
    Wireframe = AIS_WireFrame,
    Shaded    = AIS_Shaded
  };

  public enum class ViewOrientation
  {
    Front, Back, Top, Bottom, Left, Right, Isometric
  };

  /// OcctViewer가 소유하는 네이티브 OCCT 핸들 묶음.
  /// (관리 형식 안에 네이티브 형식을 중첩할 수 없으므로 네임스페이스 범위에 둔다.)
  struct OcctViewerNative
  {
    Handle(Graphic3d_GraphicDriver) Driver;
    Handle(V3d_Viewer)              Viewer;
    Handle(V3d_View)                View;
    Handle(AIS_InteractiveContext)  Context;
  };

  /// 윈도 하나에 대한 V3d_Viewer / V3d_View / AIS_InteractiveContext를 캡슐화한다.
  /// WinForms 컨트롤에서 viewer.Init(control.Handle)로 생성한다.
  public ref class OcctViewer
  {
  public:
    OcctViewer() : myNative(new OcctViewerNative()) {}
    ~OcctViewer() { this->!OcctViewer(); }

    /// Dispose()와 GC Finalizer 양쪽에서 호출된다. 두 번 호출되어도 안전해야 한다.
    /// (NCollection_Haft를 멤버로 쓰면 Finalizer 실행 순서가 보장되지 않아 Haft가 먼저
    ///  정리된 뒤 널 핸들을 참조하는 NullReferenceException이 났다. 그래서 네이티브
    ///  핸들 4개를 이 클래스가 직접 소유하는 하나의 구조체로 묶어 관리한다.)
    !OcctViewer()
    {
      OcctViewerNative* aNative = myNative;
      myNative = nullptr;
      if (aNative == nullptr) return;

      if (!aNative->Context.IsNull()) { aNative->Context->RemoveAll(false); }
      if (!aNative->View.IsNull())    { aNative->View->Remove(); }
      aNative->Context.Nullify();
      aNative->View.Nullify();
      aNative->Viewer.Nullify();
      aNative->Driver.Nullify();
      delete aNative;
    }

    /// 주어진 HWND에 OpenGL 컨텍스트를 생성한다. 실패하면 false를 반환.
    bool Init(System::IntPtr hwnd)
    {
      if (myNative == nullptr) { throw gcnew System::ObjectDisposedException("OcctViewer"); }
      OCCT_TRY
        Handle(Aspect_DisplayConnection) aDisplay = new Aspect_DisplayConnection();
        Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver(aDisplay, false);
        aDriver->ChangeOptions().buffersNoSwap = false;
        if (!aDriver->InitContext())
        {
          return false;
        }
        myDriver() = aDriver;

        myViewer() = new V3d_Viewer(aDriver);
        myViewer()->SetDefaultLights();
        myViewer()->SetLightOn();

        myView() = myViewer()->CreateView();
        Handle(WNT_Window) aWindow = new WNT_Window(reinterpret_cast<HWND>(hwnd.ToPointer()));
        myView()->SetWindow(aWindow);
        if (!aWindow->IsMapped())
        {
          aWindow->Map();
        }
        myView()->SetBgGradientColors(Quantity_Color(0.30, 0.33, 0.40, Quantity_TOC_sRGB),
                                      Quantity_Color(0.08, 0.09, 0.12, Quantity_TOC_sRGB),
                                      Aspect_GradientFillMethod_Vertical, false);
        myView()->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.08, V3d_ZBUFFER);

        myContext() = new AIS_InteractiveContext(myViewer());
        myContext()->SetDisplayMode(AIS_Shaded, false);

        myView()->MustBeResized();
        myView()->Redraw();
        return true;
      OCCT_CATCH
    }

    property bool IsInitialized { bool get() { return HasView(); } }

    // ---- 윈도 연동 ---------------------------------------------------------

    /// 컨트롤의 Resize 이벤트에서 호출.
    void Resize()
    {
      if (HasView()) { myView()->MustBeResized(); }
    }

    /// 컨트롤의 Paint 이벤트에서 호출.
    void Redraw()
    {
      if (HasView()) { myView()->Redraw(); }
    }

    // ---- 장면 --------------------------------------------------------------

    /// 지정한 RGB 색상(각 성분 0..1)으로 형상을 표시한다.
    void Display(OcctShape^ shape, double r, double g, double b, bool update)
    {
      if (shape == nullptr || shape->IsNull || !HasContext()) return;
      OCCT_TRY
        Handle(AIS_Shape) anAis = new AIS_Shape(shape->Native());
        anAis->SetColor(Quantity_Color(r, g, b, Quantity_TOC_sRGB));
        myContext()->Display(anAis, update ? true : false);
      OCCT_CATCH
    }

    void Display(OcctShape^ shape)
    {
      Display(shape, 0.75, 0.75, 0.78, true);
    }

    void EraseAll()
    {
      if (!HasContext()) return;
      myContext()->RemoveAll(true);
    }

    void FitAll()
    {
      if (!HasView()) return;
      myView()->FitAll(0.01, false);
      myView()->ZFitAll();
      myView()->Redraw();
    }

    /// 표시 중인 모든 객체(및 기본값)를 와이어프레임 또는 셰이디드로 전환한다.
    void SetDisplayMode(DisplayMode mode)
    {
      if (!HasContext()) return;
      OCCT_TRY
        const int aMode = static_cast<int>(mode);
        myContext()->SetDisplayMode(aMode, false);
        NCollection_List<Handle(AIS_InteractiveObject)> aList;
        myContext()->DisplayedObjects(aList);
        for (NCollection_List<Handle(AIS_InteractiveObject)>::Iterator anIt(aList); anIt.More(); anIt.Next())
        {
          myContext()->SetDisplayMode(anIt.Value(), aMode, false);
        }
        myContext()->UpdateCurrentViewer();
      OCCT_CATCH
    }

    void SetBackgroundColor(double r, double g, double b)
    {
      if (!HasView()) return;
      myView()->SetBgGradientStyle(Aspect_GradientFillMethod_None, false);
      myView()->SetBackgroundColor(Quantity_Color(r, g, b, Quantity_TOC_sRGB));
      myView()->Redraw();
    }

    void SetOrientation(ViewOrientation orientation)
    {
      if (!HasView()) return;
      switch (orientation)
      {
        case ViewOrientation::Front:  myView()->SetProj(V3d_Yneg); break;
        case ViewOrientation::Back:   myView()->SetProj(V3d_Ypos); break;
        case ViewOrientation::Top:    myView()->SetProj(V3d_Zpos); break;
        case ViewOrientation::Bottom: myView()->SetProj(V3d_Zneg); break;
        case ViewOrientation::Left:   myView()->SetProj(V3d_Xneg); break;
        case ViewOrientation::Right:  myView()->SetProj(V3d_Xpos); break;
        default:                      myView()->SetProj(V3d_XposYnegZpos); break;
      }
      FitAll();
    }

    // ---- 마우스 상호작용 (컨트롤의 픽셀 좌표) ---------------------------------

    void StartRotation(int x, int y)
    {
      if (HasView()) { myView()->StartRotation(x, y); }
    }

    void Rotate(int x, int y)
    {
      if (HasView()) { myView()->Rotation(x, y); }
    }

    /// 픽셀 델타만큼 화면을 이동(팬)한다.
    void Pan(int dx, int dy)
    {
      if (HasView()) { myView()->Pan(dx, dy); }
    }

    /// 지정한 픽셀을 중심으로 줌한다. factor > 1이면 확대, < 1이면 축소.
    void ZoomAt(int x, int y, double factor)
    {
      if (!HasView()) return;
      myView()->StartZoomAtPoint(x, y);
      // ZoomAtPoint는 픽셀 델타로부터 계수를 구하므로 factor를 델타로 환산한다.
      const int aDelta = static_cast<int>((factor - 1.0) * 100.0);
      myView()->ZoomAtPoint(x, y, x + aDelta, y);
    }

    /// 커서 아래 엔티티를 동적으로 하이라이트한다.
    void MoveTo(int x, int y)
    {
      if (!HasContext()) return;
      myContext()->MoveTo(x, y, myView(), true);
    }

    /// 감지된(하이라이트된) 엔티티를 선택하고 기존 선택을 대체한다.
    void Select()
    {
      if (!HasContext()) return;
      myContext()->SelectDetected(AIS_SelectionScheme_Replace);
      myContext()->UpdateCurrentViewer();
    }

    /// 감지된 엔티티를 현재 선택에서 토글한다.
    void ShiftSelect()
    {
      if (!HasContext()) return;
      myContext()->SelectDetected(AIS_SelectionScheme_XOR);
      myContext()->UpdateCurrentViewer();
    }

    property int SelectedCount
    {
      int get() { return !HasContext() ? 0 : myContext()->NbSelected(); }
    }

    void SetSelectedColor(double r, double g, double b)
    {
      if (!HasContext()) return;
      Quantity_Color aColor(r, g, b, Quantity_TOC_sRGB);
      for (myContext()->InitSelected(); myContext()->MoreSelected(); myContext()->NextSelected())
      {
        myContext()->SetColor(myContext()->SelectedInteractive(), aColor, false);
      }
      myContext()->UpdateCurrentViewer();
    }

    void RemoveSelected()
    {
      if (!HasContext()) return;
      NCollection_List<Handle(AIS_InteractiveObject)> aList;
      for (myContext()->InitSelected(); myContext()->MoreSelected(); myContext()->NextSelected())
      {
        aList.Append(myContext()->SelectedInteractive());
      }
      myContext()->ClearSelected(false);
      for (NCollection_List<Handle(AIS_InteractiveObject)>::Iterator anIt(aList); anIt.More(); anIt.Next())
      {
        myContext()->Remove(anIt.Value(), false);
      }
      myContext()->UpdateCurrentViewer();
    }

    /// 선택된 모든 객체를 월드 좌표계 기준으로 (dx, dy, dz)만큼 이동한다.
    /// AIS 객체의 로컬 변환(LocalTransformation)만 바뀌며 원본 TopoDS_Shape는 그대로다.
    void TranslateSelected(double dx, double dy, double dz)
    {
      if (!HasContext()) return;
      OCCT_TRY
        gp_Trsf aDelta;
        aDelta.SetTranslation(gp_Vec(dx, dy, dz));

        // 선택 목록을 먼저 복사한다 (RemoveSelected와 같은 패턴).
        NCollection_List<Handle(AIS_InteractiveObject)> aList;
        for (myContext()->InitSelected(); myContext()->MoreSelected(); myContext()->NextSelected())
        {
          aList.Append(myContext()->SelectedInteractive());
        }

        for (NCollection_List<Handle(AIS_InteractiveObject)>::Iterator anIt(aList); anIt.More(); anIt.Next())
        {
          const Handle(AIS_InteractiveObject)& anObj = anIt.Value();
          // aDelta * 기존 변환: 기존 변환을 먼저 적용한 뒤 월드 축 방향으로 이동한다.
          gp_Trsf aNew = aDelta * anObj->LocalTransformation();
          // SetLocation은 로컬 변환 설정과 함께 선택 엔티티/동적 하이라이트도 갱신한다.
          myContext()->SetLocation(anObj, TopLoc_Location(aNew));
        }
        myContext()->UpdateCurrentViewer();
      OCCT_CATCH
    }

    /// 선택된 모든 객체를 각자의 중심을 지나는 월드 축 (ax, ay, az) 둘레로 angleDeg(도)만큼 회전한다.
    /// 회전 중심은 원본 형상 바운딩 박스의 중심을 현재 로컬 변환으로 옮긴 월드 좌표다.
    /// (AIS_Shape가 아니거나 박스가 비어 있으면 로컬 변환의 이동 성분을 중심으로 쓴다.)
    /// AIS 객체의 로컬 변환(LocalTransformation)만 바뀌며 원본 TopoDS_Shape는 그대로다.
    void RotateSelected(double ax, double ay, double az, double angleDeg)
    {
      if (!HasContext()) return;
      if (ax == 0.0 && ay == 0.0 && az == 0.0) { throw gcnew OcctException("회전 축이 영벡터입니다."); }
      OCCT_TRY
        const gp_Dir anAxis(ax, ay, az);
        const double anAngle = angleDeg * M_PI / 180.0;

        // 선택 목록을 먼저 복사한다 (RemoveSelected와 같은 패턴).
        NCollection_List<Handle(AIS_InteractiveObject)> aList;
        for (myContext()->InitSelected(); myContext()->MoreSelected(); myContext()->NextSelected())
        {
          aList.Append(myContext()->SelectedInteractive());
        }

        for (NCollection_List<Handle(AIS_InteractiveObject)>::Iterator anIt(aList); anIt.More(); anIt.Next())
        {
          const Handle(AIS_InteractiveObject)& anObj = anIt.Value();
          const gp_Trsf anOld = anObj->LocalTransformation();

          // 회전 중심: 원본 형상의 바운딩 박스 중심(로컬 좌표)을 현재 변환으로 월드 좌표에 옮긴다.
          gp_Pnt aCenter(anOld.TranslationPart());
          Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(anObj);
          if (!aShape.IsNull() && !aShape->Shape().IsNull())
          {
            Bnd_Box aBox;
            BRepBndLib::Add(aShape->Shape(), aBox, false);
            if (!aBox.IsVoid())
            {
              double aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;
              aBox.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
              aCenter = gp_Pnt((aXmin + aXmax) * 0.5, (aYmin + aYmax) * 0.5, (aZmin + aZmax) * 0.5)
                          .Transformed(anOld);
            }
          }

          gp_Trsf aDelta;
          aDelta.SetRotation(gp_Ax1(aCenter, anAxis), anAngle);
          // aDelta * 기존 변환: 기존 변환을 먼저 적용한 뒤 중심을 지나는 월드 축 둘레로 회전한다.
          gp_Trsf aNew = aDelta * anOld;
          // SetLocation은 로컬 변환 설정과 함께 선택 엔티티/동적 하이라이트도 갱신한다.
          myContext()->SetLocation(anObj, TopLoc_Location(aNew));
        }
        myContext()->UpdateCurrentViewer();
      OCCT_CATCH
    }

    /// 스크린샷을 저장한다 (확장자에 따라 png/bmp/jpg).
    bool Dump(System::String^ path)
    {
      if (!HasView()) return false;
      OCCT_TRY
        myView()->Redraw();
        return myView()->Dump(ToAscii(path).ToCString()) == true;
      OCCT_CATCH
    }

  private:
    OcctViewerNative* myNative;

    // 모든 public 메서드는 HasView()/HasContext()로 먼저 확인한 뒤 아래 접근자를 쓴다.
    bool HasView()    { return myNative != nullptr && !myNative->View.IsNull(); }
    bool HasContext() { return myNative != nullptr && !myNative->Context.IsNull(); }

    Handle(Graphic3d_GraphicDriver)& myDriver()  { return myNative->Driver; }
    Handle(V3d_Viewer)&              myViewer()  { return myNative->Viewer; }
    Handle(V3d_View)&                myView()    { return myNative->View; }
    Handle(AIS_InteractiveContext)&  myContext() { return myNative->Context; }
  };
}
