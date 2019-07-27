





#include "mozilla/dom/SVGPolygonElement.h"
#include "mozilla/dom/SVGPolygonElementBinding.h"
#include "mozilla/gfx/2D.h"
#include "SVGContentUtils.h"

using namespace mozilla;
using namespace mozilla::gfx;

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Polygon)

namespace mozilla {
namespace dom {

JSObject*
SVGPolygonElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return SVGPolygonElementBinding::Wrap(aCx, this, aGivenProto);
}




SVGPolygonElement::SVGPolygonElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGPolygonElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGPolygonElement)




void
SVGPolygonElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks)
{
  nsSVGPolyElement::GetMarkPoints(aMarks);

  if (aMarks->IsEmpty() || aMarks->LastElement().type != nsSVGMark::eEnd) {
    return;
  }

  nsSVGMark *endMark = &aMarks->LastElement();
  nsSVGMark *startMark = &aMarks->ElementAt(0);
  float angle = atan2(startMark->y - endMark->y, startMark->x - endMark->x);

  endMark->type = nsSVGMark::eMid;
  endMark->angle = SVGContentUtils::AngleBisect(angle, endMark->angle);
  startMark->angle = SVGContentUtils::AngleBisect(angle, startMark->angle);
  
  
  
  aMarks->AppendElement(nsSVGMark(startMark->x, startMark->y, startMark->angle,
                                  nsSVGMark::eEnd));
}

already_AddRefed<Path>
SVGPolygonElement::BuildPath(PathBuilder* aBuilder)
{
  const SVGPointList &points = mPoints.GetAnimValue();

  if (points.IsEmpty()) {
    return nullptr;
  }

  aBuilder->MoveTo(points[0]);
  for (uint32_t i = 1; i < points.Length(); ++i) {
    aBuilder->LineTo(points[i]);
  }

  aBuilder->Close();

  return aBuilder->Finish();
}

} 
} 
