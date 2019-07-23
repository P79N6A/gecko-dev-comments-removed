





































#include "nsSVGPolyElement.h"
#include "nsIDOMSVGPolygonElement.h"

typedef nsSVGPolyElement nsSVGPolygonElementBase;

class nsSVGPolygonElement : public nsSVGPolygonElementBase,
                            public nsIDOMSVGPolygonElement
{
protected:
  friend nsresult NS_NewSVGPolygonElement(nsIContent **aResult,
                                          nsINodeInfo *aNodeInfo);
  nsSVGPolygonElement(nsINodeInfo* aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGPOLYGONELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGPolygonElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGPolygonElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGPolygonElementBase::)

  
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(cairo_t *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Polygon)




NS_IMPL_ADDREF_INHERITED(nsSVGPolygonElement,nsSVGPolygonElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGPolygonElement,nsSVGPolygonElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGPolygonElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPolygonElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPolygonElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGPolygonElementBase)




nsSVGPolygonElement::nsSVGPolygonElement(nsINodeInfo* aNodeInfo)
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

    endMark->angle = nsSVGUtils::AngleBisect(angle, endMark->angle);
    startMark->angle = nsSVGUtils::AngleBisect(angle, startMark->angle);
  }
}

void
nsSVGPolygonElement::ConstructPath(cairo_t *aCtx)
{
  nsSVGPolygonElementBase::ConstructPath(aCtx);
  
  
  cairo_close_path(aCtx);
}


