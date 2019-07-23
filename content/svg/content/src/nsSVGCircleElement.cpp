





































#include "nsSVGPathGeometryElement.h"
#include "nsIDOMSVGCircleElement.h"
#include "nsSVGLength2.h"
#include "nsGkAtoms.h"
#include "nsSVGUtils.h"
#include "gfxContext.h"

typedef nsSVGPathGeometryElement nsSVGCircleElementBase;

class nsSVGCircleElement : public nsSVGCircleElementBase,
                           public nsIDOMSVGCircleElement
{
protected:
  friend nsresult NS_NewSVGCircleElement(nsIContent **aResult,
                                         nsINodeInfo *aNodeInfo);
  nsSVGCircleElement(nsINodeInfo *aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGCIRCLEELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGCircleElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGCircleElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGCircleElementBase::)

  
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  virtual LengthAttributesInfo GetLengthInfo();

  enum { CX, CY, R };
  nsSVGLength2 mLengthAttributes[3];
  static LengthInfo sLengthInfo[3];
};

nsSVGElement::LengthInfo nsSVGCircleElement::sLengthInfo[3] =
{
  { &nsGkAtoms::cx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::cy, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::r, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::XY }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Circle)




NS_IMPL_ADDREF_INHERITED(nsSVGCircleElement,nsSVGCircleElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGCircleElement,nsSVGCircleElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGCircleElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGCircleElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGCircleElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGCircleElementBase)




nsSVGCircleElement::nsSVGCircleElement(nsINodeInfo *aNodeInfo)
  : nsSVGCircleElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGCircleElement)





NS_IMETHODIMP nsSVGCircleElement::GetCx(nsIDOMSVGAnimatedLength * *aCx)
{
  return mLengthAttributes[CX].ToDOMAnimatedLength(aCx, this);
}


NS_IMETHODIMP nsSVGCircleElement::GetCy(nsIDOMSVGAnimatedLength * *aCy)
{
  return mLengthAttributes[CY].ToDOMAnimatedLength(aCy, this);
}


NS_IMETHODIMP nsSVGCircleElement::GetR(nsIDOMSVGAnimatedLength * *aR)
{
  return mLengthAttributes[R].ToDOMAnimatedLength(aR, this);
}




nsSVGElement::LengthAttributesInfo
nsSVGCircleElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}




void
nsSVGCircleElement::ConstructPath(gfxContext *aCtx)
{
  float x, y, r;

  GetAnimatedLengthValues(&x, &y, &r, nsnull);

  if (r > 0.0f)
    aCtx->Arc(gfxPoint(x, y), r, 0, 2*M_PI);
}
