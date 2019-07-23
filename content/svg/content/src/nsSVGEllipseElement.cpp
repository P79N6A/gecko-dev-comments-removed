






































#include "nsSVGPathGeometryElement.h"
#include "nsIDOMSVGEllipseElement.h"
#include "nsSVGLength2.h"
#include "nsGkAtoms.h"
#include "nsSVGUtils.h"

typedef nsSVGPathGeometryElement nsSVGEllipseElementBase;

class nsSVGEllipseElement : public nsSVGEllipseElementBase,
                            public nsIDOMSVGEllipseElement
{
protected:
  friend nsresult NS_NewSVGEllipseElement(nsIContent **aResult,
                                         nsINodeInfo *aNodeInfo);
  nsSVGEllipseElement(nsINodeInfo *aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGELLIPSEELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGEllipseElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGEllipseElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGEllipseElementBase::)

  
  virtual void ConstructPath(cairo_t *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

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

NS_INTERFACE_MAP_BEGIN(nsSVGEllipseElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGEllipseElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGEllipseElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGEllipseElementBase)




nsSVGEllipseElement::nsSVGEllipseElement(nsINodeInfo *aNodeInfo)
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
                              NS_ARRAY_LENGTH(sLengthInfo));
}




void
nsSVGEllipseElement::ConstructPath(cairo_t *aCtx)
{
  float x, y, rx, ry;

  GetAnimatedLengthValues(&x, &y, &rx, &ry, nsnull);

  if (rx > 0.0f && ry > 0.0f) {
    cairo_save(aCtx);
    cairo_translate(aCtx, x, y);
    cairo_scale(aCtx, rx, ry);
    cairo_arc(aCtx, 0, 0, 1, 0, 2 * M_PI);
    cairo_restore(aCtx);
  }
}
