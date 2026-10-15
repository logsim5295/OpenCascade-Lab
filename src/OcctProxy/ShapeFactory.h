// ShapeFactory.h - 기본 도형, 불리언, 필렛, 변환.
#pragma once

#include "Common.h"
#include "OcctShape.h"

#include <cmath>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepBuilderAPI_Transform.hxx>

namespace OcctProxy
{
  // 순수 네이티브 헬퍼. BRepFilletAPI_* 객체는 과정렬(over-aligned) 멤버를 가지므로,
  // 그런 지역 변수를 갖는 함수는 /clr 아래에서 네이티브로 컴파일된다. 이는 함수의
  // 시그니처와 본문에 관리 타입이 전혀 없을 때만 허용된다.
  namespace Native
  {
    inline TopoDS_Shape FilletAllEdges(const TopoDS_Shape& theShape, double theRadius, bool& theIsDone)
    {
      BRepFilletAPI_MakeFillet aFillet(theShape);
      for (TopExp_Explorer anExp(theShape, TopAbs_EDGE); anExp.More(); anExp.Next())
      {
        aFillet.Add(theRadius, TopoDS::Edge(anExp.Current()));
      }
      aFillet.Build();
      theIsDone = aFillet.IsDone();
      return theIsDone ? aFillet.Shape() : TopoDS_Shape();
    }

    inline TopoDS_Shape ChamferAllEdges(const TopoDS_Shape& theShape, double theDistance, bool& theIsDone)
    {
      BRepFilletAPI_MakeChamfer aChamfer(theShape);
      for (TopExp_Explorer anExp(theShape, TopAbs_EDGE); anExp.More(); anExp.Next())
      {
        aChamfer.Add(theDistance, TopoDS::Edge(anExp.Current()));
      }
      aChamfer.Build();
      theIsDone = aChamfer.IsDone();
      return theIsDone ? aChamfer.Shape() : TopoDS_Shape();
    }
  }

  /// 가장 흔히 쓰는 OCCT 모델링 알고리즘을 .NET에 노출하는 정적 팩토리.
  /// 길이는 모델 단위(관례상 mm), 각도는 도(degree) 단위.
  public ref class ShapeFactory abstract sealed
  {
  public:
    // ---- 기본 도형 ---------------------------------------------------------

    /// 한 모서리가 원점에 있고 +X/+Y/+Z 방향으로 뻗는 박스.
    static OcctShape^ MakeBox(double dx, double dy, double dz)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeBox(dx, dy, dz).Shape());
      OCCT_CATCH
    }

    /// 한 모서리가 (x, y, z)에 있는 박스.
    static OcctShape^ MakeBox(double x, double y, double z, double dx, double dy, double dz)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeBox(gp_Pnt(x, y, z), dx, dy, dz).Shape());
      OCCT_CATCH
    }

    /// 밑면 중심이 원점에 있고 +Z 방향으로 세워진 원기둥.
    static OcctShape^ MakeCylinder(double radius, double height)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeCylinder(radius, height).Shape());
      OCCT_CATCH
    }

    /// 밑면 중심이 (x, y, z)이고 축 방향이 (dirX, dirY, dirZ)인 원기둥.
    static OcctShape^ MakeCylinder(double x, double y, double z,
                                   double dirX, double dirY, double dirZ,
                                   double radius, double height)
    {
      OCCT_TRY
        gp_Ax2 anAxis(gp_Pnt(x, y, z), gp_Dir(dirX, dirY, dirZ));
        return gcnew OcctShape(BRepPrimAPI_MakeCylinder(anAxis, radius, height).Shape());
      OCCT_CATCH
    }

    static OcctShape^ MakeSphere(double radius)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeSphere(radius).Shape());
      OCCT_CATCH
    }

    static OcctShape^ MakeSphere(double x, double y, double z, double radius)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeSphere(gp_Pnt(x, y, z), radius).Shape());
      OCCT_CATCH
    }

    /// +Z 방향 원뿔: 밑면 반지름 radius1, 윗면 반지름 radius2 (0이면 뾰족한 꼭짓점).
    static OcctShape^ MakeCone(double radius1, double radius2, double height)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeCone(radius1, radius2, height).Shape());
      OCCT_CATCH
    }

    /// +Z 축을 도는 토러스: majorRadius는 중심에서 튜브 중심까지 거리, minorRadius는 튜브 반지름.
    static OcctShape^ MakeTorus(double majorRadius, double minorRadius)
    {
      OCCT_TRY
        return gcnew OcctShape(BRepPrimAPI_MakeTorus(majorRadius, minorRadius).Shape());
      OCCT_CATCH
    }

    // ---- 불리언 연산 -------------------------------------------------------

    static OcctShape^ Fuse(OcctShape^ a, OcctShape^ b)
    {
      CheckArgs(a, b);
      OCCT_TRY
        BRepAlgoAPI_Fuse anOp(a->Native(), b->Native());
        return FromBoolean(anOp, "Fuse");
      OCCT_CATCH
    }

    /// a에서 b를 뺀 결과를 반환.
    static OcctShape^ Cut(OcctShape^ a, OcctShape^ b)
    {
      CheckArgs(a, b);
      OCCT_TRY
        BRepAlgoAPI_Cut anOp(a->Native(), b->Native());
        return FromBoolean(anOp, "Cut");
      OCCT_CATCH
    }

    static OcctShape^ Common(OcctShape^ a, OcctShape^ b)
    {
      CheckArgs(a, b);
      OCCT_TRY
        BRepAlgoAPI_Common anOp(a->Native(), b->Native());
        return FromBoolean(anOp, "Common");
      OCCT_CATCH
    }

    // ---- 국부 연산 ---------------------------------------------------------

    /// 형상의 모든 엣지에 일정한 반지름으로 필렛을 적용한다.
    static OcctShape^ FilletAllEdges(OcctShape^ shape, double radius)
    {
      CheckArg(shape);
      OCCT_TRY
        bool isDone = false;
        TopoDS_Shape aResult = Native::FilletAllEdges(shape->Native(), radius, isDone);
        if (!isDone)
        {
          throw gcnew OcctException("Fillet failed (radius too large for the geometry?)");
        }
        return gcnew OcctShape(aResult);
      OCCT_CATCH
    }

    /// 형상의 모든 엣지에 일정한 거리로 챔퍼를 적용한다.
    static OcctShape^ ChamferAllEdges(OcctShape^ shape, double distance)
    {
      CheckArg(shape);
      OCCT_TRY
        bool isDone = false;
        TopoDS_Shape aResult = Native::ChamferAllEdges(shape->Native(), distance, isDone);
        if (!isDone)
        {
          throw gcnew OcctException("Chamfer failed (distance too large for the geometry?)");
        }
        return gcnew OcctShape(aResult);
      OCCT_CATCH
    }

    // ---- 변환 --------------------------------------------------------------

    static OcctShape^ Translate(OcctShape^ shape, double dx, double dy, double dz)
    {
      CheckArg(shape);
      OCCT_TRY
        gp_Trsf aTrsf;
        aTrsf.SetTranslation(gp_Vec(dx, dy, dz));
        return gcnew OcctShape(BRepBuilderAPI_Transform(shape->Native(), aTrsf, true).Shape());
      OCCT_CATCH
    }

    /// (px, py, pz)를 지나고 방향이 (dx, dy, dz)인 축을 중심으로 회전한다.
    static OcctShape^ Rotate(OcctShape^ shape,
                             double px, double py, double pz,
                             double dx, double dy, double dz,
                             double angleDegrees)
    {
      CheckArg(shape);
      OCCT_TRY
        gp_Trsf aTrsf;
        aTrsf.SetRotation(gp_Ax1(gp_Pnt(px, py, pz), gp_Dir(dx, dy, dz)),
                          angleDegrees * 3.14159265358979323846 / 180.0);
        return gcnew OcctShape(BRepBuilderAPI_Transform(shape->Native(), aTrsf, true).Shape());
      OCCT_CATCH
    }

    /// 원점을 기준으로 균일하게 크기를 조절한다.
    static OcctShape^ Scale(OcctShape^ shape, double factor)
    {
      CheckArg(shape);
      OCCT_TRY
        gp_Trsf aTrsf;
        aTrsf.SetScale(gp_Pnt(0.0, 0.0, 0.0), factor);
        return gcnew OcctShape(BRepBuilderAPI_Transform(shape->Native(), aTrsf, true).Shape());
      OCCT_CATCH
    }

    /// 여러 형상을 하나의 컴파운드로 묶는다 (불리언 연산은 수행하지 않음).
    static OcctShape^ MakeCompound(System::Collections::Generic::IEnumerable<OcctShape^>^ shapes)
    {
      if (shapes == nullptr) throw gcnew System::ArgumentNullException("shapes");
      OCCT_TRY
        BRep_Builder aBuilder;
        TopoDS_Compound aCompound;
        aBuilder.MakeCompound(aCompound);
        for each (OcctShape^ aShape in shapes)
        {
          if (aShape != nullptr && !aShape->IsNull)
          {
            aBuilder.Add(aCompound, aShape->Native());
          }
        }
        return gcnew OcctShape(aCompound);
      OCCT_CATCH
    }

  private:
    static void CheckArg(OcctShape^ shape)
    {
      if (shape == nullptr || shape->IsNull) throw gcnew System::ArgumentException("Shape is null");
    }

    static void CheckArgs(OcctShape^ a, OcctShape^ b)
    {
      if (a == nullptr || a->IsNull) throw gcnew System::ArgumentException("First shape is null");
      if (b == nullptr || b->IsNull) throw gcnew System::ArgumentException("Second shape is null");
    }

    static OcctShape^ FromBoolean(BRepAlgoAPI_BooleanOperation& theOp, const char* theName)
    {
      theOp.Build();
      if (!theOp.IsDone() || theOp.HasErrors())
      {
        throw gcnew OcctException(System::String::Concat("Boolean ", gcnew System::String(theName), " failed"));
      }
      return gcnew OcctShape(theOp.Shape());
    }
  };
}
