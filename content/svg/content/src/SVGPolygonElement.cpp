




#include "mozilla/dom/SVGPolygonElement.h"
#include "mozilla/dom/SVGPolygonElementBinding.h"
#include "gfxContext.h"
#include "SVGContentUtils.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Polygon)

namespace mozilla {
namespace dom {

JSObject*
SVGPolygonElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGPolygonElementBinding::Wrap(aCx, aScope, this);
}




NS_IMPL_ISUPPORTS_INHERITED3(SVGPolygonElement, SVGPolygonElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)




SVGPolygonElement::SVGPolygonElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGPolygonElementBase(aNodeInfo)
{

}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGPolygonElement)




void
SVGPolygonElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks)
{
  nsSVGPolyElement::GetMarkPoints(aMarks);
  if (aMarks->Length() > 0) {
    nsSVGMark *endMark = &aMarks->ElementAt(aMarks->Length()-1);
    nsSVGMark *startMark = &aMarks->ElementAt(0);
    float angle = atan2(startMark->y - endMark->y, startMark->x - endMark->x);

    endMark->angle = SVGContentUtils::AngleBisect(angle, endMark->angle);
    startMark->angle = SVGContentUtils::AngleBisect(angle, startMark->angle);
    
    
    
    aMarks->AppendElement(nsSVGMark(startMark->x, startMark->y, startMark->angle));
  }
}

void
SVGPolygonElement::ConstructPath(gfxContext *aCtx)
{
  SVGPolygonElementBase::ConstructPath(aCtx);
  
  
  aCtx->ClosePath();
}

} 
} 
