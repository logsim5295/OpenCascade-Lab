// OcctShape.h - TopoDS_Shape를 감싸는 관리 래퍼.
#pragma once

#include "Common.h"

#include <TopoDS_Shape.hxx>
#include <TopAbs.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <NCollection_IndexedMap.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

namespace OcctProxy
{
  /// 모델 좌표계 기준의 축 정렬 바운딩 박스.
  public value struct BoundingBox
  {
    double XMin, YMin, ZMin, XMax, YMax, ZMax;

    property double SizeX { double get() { return XMax - XMin; } }
    property double SizeY { double get() { return YMax - YMin; } }
    property double SizeZ { double get() { return ZMax - ZMin; } }

    virtual System::String^ ToString() override
    {
      return System::String::Format(System::Globalization::CultureInfo::InvariantCulture,
        "[{0:F3}, {1:F3}, {2:F3}] - [{3:F3}, {4:F3}, {5:F3}]", XMin, YMin, ZMin, XMax, YMax, ZMax);
    }
  };

  /// 위상 형상 타입 (TopAbs_ShapeEnum과 1:1 대응).
  public enum class ShapeType
  {
    Compound  = TopAbs_COMPOUND,
    CompSolid = TopAbs_COMPSOLID,
    Solid     = TopAbs_SOLID,
    Shell     = TopAbs_SHELL,
    Face      = TopAbs_FACE,
    Wire      = TopAbs_WIRE,
    Edge      = TopAbs_EDGE,
    Vertex    = TopAbs_VERTEX,
    Shape     = TopAbs_SHAPE
  };

  /// 네이티브 TopoDS_Shape에 대한 관리 핸들. 형상의 힙 복사본을 소유한다
  /// (TopoDS_Shape 자체가 가벼운 참조 카운팅 핸들이므로 복사 비용은 작다).
  public ref class OcctShape
  {
  internal:
    OcctShape(const TopoDS_Shape& theShape)
      : myShape(new TopoDS_Shape(theShape)) {}

    /// 다른 프록시 클래스에서 네이티브 형상에 접근할 때 사용.
    const TopoDS_Shape& Native() { return *myShape; }

  public:
    OcctShape() : myShape(new TopoDS_Shape()) {}
    ~OcctShape() { this->!OcctShape(); }
    !OcctShape()
    {
      if (myShape != nullptr)
      {
        delete myShape;
        myShape = nullptr;
      }
    }

    property bool IsNull { bool get() { return myShape == nullptr || myShape->IsNull(); } }

    property ShapeType Type
    {
      ShapeType get() { return IsNull ? ShapeType::Shape : static_cast<ShapeType>(myShape->ShapeType()); }
    }

    property int NbSolids   { int get() { return Count(TopAbs_SOLID); } }
    property int NbShells   { int get() { return Count(TopAbs_SHELL); } }
    property int NbFaces    { int get() { return Count(TopAbs_FACE); } }
    property int NbEdges    { int get() { return Count(TopAbs_EDGE); } }
    property int NbVertices { int get() { return Count(TopAbs_VERTEX); } }

    /// 형상에 포함된 모든 솔리드의 부피 (모델 단위^3).
    property double Volume
    {
      double get()
      {
        if (IsNull) return 0.0;
        OCCT_TRY
          GProp_GProps aProps;
          BRepGProp::VolumeProperties(*myShape, aProps);
          return aProps.Mass();
        OCCT_CATCH
      }
    }

    /// 모든 면의 총 표면적 (모델 단위^2).
    property double SurfaceArea
    {
      double get()
      {
        if (IsNull) return 0.0;
        OCCT_TRY
          GProp_GProps aProps;
          BRepGProp::SurfaceProperties(*myShape, aProps);
          return aProps.Mass();
        OCCT_CATCH
      }
    }

    /// 부피 가중 질량 중심을 (x, y, z)로 반환.
    array<double>^ CenterOfMass()
    {
      array<double>^ aResult = gcnew array<double>(3);
      if (IsNull) return aResult;
      OCCT_TRY
        GProp_GProps aProps;
        BRepGProp::VolumeProperties(*myShape, aProps);
        gp_Pnt aCenter = aProps.CentreOfMass();
        aResult[0] = aCenter.X();
        aResult[1] = aCenter.Y();
        aResult[2] = aCenter.Z();
        return aResult;
      OCCT_CATCH
    }

    BoundingBox GetBoundingBox()
    {
      BoundingBox aBox = {};
      if (IsNull) return aBox;
      OCCT_TRY
        Bnd_Box aBnd;
        BRepBndLib::Add(*myShape, aBnd, false);
        if (!aBnd.IsVoid())
        {
          aBnd.Get(aBox.XMin, aBox.YMin, aBox.ZMin, aBox.XMax, aBox.YMax, aBox.ZMax);
        }
        return aBox;
      OCCT_CATCH
    }

    /// BRepCheck_Analyzer 실행 (위상 / 기하 유효성 검사).
    bool IsValid()
    {
      if (IsNull) return false;
      OCCT_TRY
        BRepCheck_Analyzer anAnalyzer(*myShape);
        return anAnalyzer.IsValid() == true;
      OCCT_CATCH
    }

    virtual System::String^ ToString() override
    {
      if (IsNull) return "OcctShape(null)";
      return System::String::Format(System::Globalization::CultureInfo::InvariantCulture,
        "OcctShape({0}: {1} solids, {2} faces, {3} edges, {4} vertices)",
        Type.ToString(), NbSolids, NbFaces, NbEdges, NbVertices);
    }

  private:
    int Count(TopAbs_ShapeEnum theType)
    {
      if (IsNull) return 0;
      NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> aMap;
      TopExp::MapShapes(*myShape, theType, aMap);
      return aMap.Extent();
    }

    TopoDS_Shape* myShape;
  };
}
