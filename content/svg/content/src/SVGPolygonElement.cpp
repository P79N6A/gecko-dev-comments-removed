




#include "mozilla/dom/SVGPolygonElement.h"
#include "mozilla/dom/SVGPolygonElementBinding.h"
#include "gfxContext.h"
#include "SVGContentUtils.h"

DOMCI_NODE_DATA(SVGPolygonElement, mozilla::dom::SVGPolygonElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Polygon)

namespace mozilla {
namespace dom {

JSObject*
SVGPolygonElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGPolygonElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}




NS_IMPL_ADDREF_INHERITED(SVGPolygonElement,SVGPolygonElementBase)
NS_IMPL_RELEASE_INHERITED(SVGPolygonElement,SVGPolygonElementBase)

NS_INTERFACE_TABLE_HEAD(SVGPolygonElement)
  NS_NODE_INTERFACE_TABLE4(SVGPolygonElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGPolygonElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPolygonElement)
NS_INTERFACE_MAP_END_INHERITING(SVGPolygonElementBase)




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
