// OcctProxy.cpp - C++/CLI 브리지의 유일한 번역 단위.
//
// 모든 public ref 클래스는 아래 헤더들에 인라인으로 정의되어 있으며,
// C# 쪽에서는 OcctProxy.OcctInfo, OcctShape, ShapeFactory, ShapeIO, OcctViewer로 보인다.
//
// 여기서 링크되는 OCCT 툴킷 목록은 OcctProxy.vcxproj의 <AdditionalDependencies>에 있다.

#include "Common.h"
#include "OcctShape.h"
#include "ShapeFactory.h"
#include "ShapeIO.h"
#include "OcctViewer.h"
