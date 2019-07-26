




#include "mozilla/dom/SVGPolygonElement.h"
#include "mozilla/dom/SVGPolygonElementBinding.h"
#include "gfxContext.h"
#include "SVGContentUtils.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Polygon)

namespace mozilla {
namespace dom {

JSObject*
SVGPolygonElement::WrapNode(JSContext *aCx)
{
  return SVGPolygonElementBinding::Wrap(aCx, this);
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

void
SVGPolygonElement::ConstructPath(gfxContext *aCtx)
{
  SVGPolygonElementBase::ConstructPath(aCtx);
  
  
  aCtx->ClosePath();
}

} 
} 
