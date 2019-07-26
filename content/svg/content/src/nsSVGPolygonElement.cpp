




#include "nsSVGPolyElement.h"
#include "nsIDOMSVGPolygonElement.h"
#include "gfxContext.h"
#include "SVGContentUtils.h"

typedef nsSVGPolyElement nsSVGPolygonElementBase;

class nsSVGPolygonElement : public nsSVGPolygonElementBase,
                            public nsIDOMSVGPolygonElement
{
protected:
  friend nsresult NS_NewSVGPolygonElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGPolygonElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGPOLYGONELEMENT

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGPolygonElementBase::)

  
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Polygon)




NS_IMPL_ADDREF_INHERITED(nsSVGPolygonElement,nsSVGPolygonElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGPolygonElement,nsSVGPolygonElementBase)

DOMCI_NODE_DATA(SVGPolygonElement, nsSVGPolygonElement)

NS_INTERFACE_TABLE_HEAD(nsSVGPolygonElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGPolygonElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTests,
                           nsIDOMSVGPolygonElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPolygonElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGPolygonElementBase)




nsSVGPolygonElement::nsSVGPolygonElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGPolygonElementBase(aNodeInfo)
{

}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGPolygonElement)




void
nsSVGPolygonElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks)
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
nsSVGPolygonElement::ConstructPath(gfxContext *aCtx)
{
  nsSVGPolygonElementBase::ConstructPath(aCtx);
  
  
  aCtx->ClosePath();
}


