






































#include "mozilla/Util.h"

#include "nsSVGPathGeometryElement.h"
#include "nsIDOMSVGEllipseElement.h"
#include "nsSVGLength2.h"
#include "nsGkAtoms.h"
#include "nsSVGUtils.h"
#include "gfxContext.h"

using namespace mozilla;

typedef nsSVGPathGeometryElement nsSVGEllipseElementBase;

class nsSVGEllipseElement : public nsSVGEllipseElementBase,
                            public nsIDOMSVGEllipseElement
{
protected:
  friend nsresult NS_NewSVGEllipseElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGEllipseElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGELLIPSEELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGEllipseElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGEllipseElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGEllipseElementBase::)

  
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:

  virtual LengthAttributesInfo GetLengthInfo();

  enum { CX, CY, RX, RY };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

nsSVGElement::LengthInfo nsSVGEllipseElement::sLengthInfo[4] =
{
  { &nsGkAtoms::cx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::cy, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::rx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::ry, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Ellipse)




NS_IMPL_ADDREF_INHERITED(nsSVGEllipseElement,nsSVGEllipseElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGEllipseElement,nsSVGEllipseElementBase)

DOMCI_NODE_DATA(SVGEllipseElement, nsSVGEllipseElement)

NS_INTERFACE_TABLE_HEAD(nsSVGEllipseElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGEllipseElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGEllipseElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGEllipseElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGEllipseElementBase)




nsSVGEllipseElement::nsSVGEllipseElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGEllipseElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGEllipseElement)





NS_IMETHODIMP nsSVGEllipseElement::GetCx(nsIDOMSVGAnimatedLength * *aCx)
{
  return mLengthAttributes[CX].ToDOMAnimatedLength(aCx, this);
}


NS_IMETHODIMP nsSVGEllipseElement::GetCy(nsIDOMSVGAnimatedLength * *aCy)
{
  return mLengthAttributes[CY].ToDOMAnimatedLength(aCy, this);
}


NS_IMETHODIMP nsSVGEllipseElement::GetRx(nsIDOMSVGAnimatedLength * *aRx)
{
  return mLengthAttributes[RX].ToDOMAnimatedLength(aRx, this);
}


NS_IMETHODIMP nsSVGEllipseElement::GetRy(nsIDOMSVGAnimatedLength * *aRy)
{
  return mLengthAttributes[RY].ToDOMAnimatedLength(aRy, this);
}




nsSVGElement::LengthAttributesInfo
nsSVGEllipseElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}




void
nsSVGEllipseElement::ConstructPath(gfxContext *aCtx)
{
  float x, y, rx, ry;

  GetAnimatedLengthValues(&x, &y, &rx, &ry, nsnull);

  if (rx > 0.0f && ry > 0.0f) {
    aCtx->Save();
    aCtx->Translate(gfxPoint(x, y));
    aCtx->Scale(rx, ry);
    aCtx->Arc(gfxPoint(0, 0), 1, 0, 2 * M_PI);
    aCtx->Restore();
  }
}
