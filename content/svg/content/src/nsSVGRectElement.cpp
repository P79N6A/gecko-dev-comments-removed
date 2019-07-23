






































#include "nsSVGPathGeometryElement.h"
#include "nsIDOMSVGRectElement.h"
#include "nsSVGLength2.h"
#include "nsGkAtoms.h"
#include "gfxContext.h"

typedef nsSVGPathGeometryElement nsSVGRectElementBase;

class nsSVGRectElement : public nsSVGRectElementBase,
                         public nsIDOMSVGRectElement
{
protected:
  friend nsresult NS_NewSVGRectElement(nsIContent **aResult,
                                       nsINodeInfo *aNodeInfo);
  nsSVGRectElement(nsINodeInfo* aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGRECTELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGRectElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGRectElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGRectElementBase::)

  
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  virtual LengthAttributesInfo GetLengthInfo();
 
  enum { X, Y, WIDTH, HEIGHT, RX, RY };
  nsSVGLength2 mLengthAttributes[6];
  static LengthInfo sLengthInfo[6];
};

nsSVGElement::LengthInfo nsSVGRectElement::sLengthInfo[6] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::rx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::ry, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Rect)




NS_IMPL_ADDREF_INHERITED(nsSVGRectElement,nsSVGRectElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGRectElement,nsSVGRectElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGRectElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGRectElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGRectElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGRectElementBase)




nsSVGRectElement::nsSVGRectElement(nsINodeInfo *aNodeInfo)
  : nsSVGRectElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGRectElement)





NS_IMETHODIMP nsSVGRectElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP nsSVGRectElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP nsSVGRectElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP nsSVGRectElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}


NS_IMETHODIMP nsSVGRectElement::GetRx(nsIDOMSVGAnimatedLength * *aRx)
{
  return mLengthAttributes[RX].ToDOMAnimatedLength(aRx, this);
}


NS_IMETHODIMP nsSVGRectElement::GetRy(nsIDOMSVGAnimatedLength * *aRy)
{
  return mLengthAttributes[RY].ToDOMAnimatedLength(aRy, this);
}




nsSVGElement::LengthAttributesInfo
nsSVGRectElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}




void
nsSVGRectElement::ConstructPath(gfxContext *aCtx)
{
  float x, y, width, height, rx, ry;

  GetAnimatedLengthValues(&x, &y, &width, &height, &rx, &ry, nsnull);

  

  if (width <= 0 || height <= 0 || ry < 0 || rx < 0)
    return;

  
  if (rx == 0 && ry == 0) {
    aCtx->Rectangle(gfxRect(x, y, width, height));
    return;
  }

  
  float halfWidth  = width/2;
  float halfHeight = height/2;
  if (rx > halfWidth)
    rx = halfWidth;
  if (ry > halfHeight)
    ry = halfHeight;

  



  PRBool hasRx = HasAttr(kNameSpaceID_None, nsGkAtoms::rx);
  PRBool hasRy = HasAttr(kNameSpaceID_None, nsGkAtoms::ry);
  if (hasRx && !hasRy)
    ry = rx;
  else if (hasRy && !hasRx)
    rx = ry;

  

  if (rx > halfWidth)
    rx = ry = halfWidth;
  else if (ry > halfHeight)
    rx = ry = halfHeight;

  
  
  
  const float magic = 4*(sqrt(2.)-1)/3;
  const float magic_x = magic*rx;
  const float magic_y = magic*ry;

  aCtx->MoveTo(gfxPoint(x + rx, y));
  aCtx->LineTo(gfxPoint(x + width - rx, y));

  aCtx->CurveTo(gfxPoint(x + width - rx + magic_x, y),
                gfxPoint(x + width, y + ry - magic_y),
                gfxPoint(x + width, y + ry));

  aCtx->LineTo(gfxPoint(x + width, y + height - ry));

  aCtx->CurveTo(gfxPoint(x + width, y + height - ry + magic_y),
                gfxPoint(x + width - rx + magic_x, y+height),
                gfxPoint(x + width - rx, y + height));

  aCtx->LineTo(gfxPoint(x + rx, y + height));

  aCtx->CurveTo(gfxPoint(x + rx - magic_x, y + height),
                gfxPoint(x, y + height - ry + magic_y),
                gfxPoint(x, y + height - ry));

  aCtx->LineTo(gfxPoint(x, y + ry));

  aCtx->CurveTo(gfxPoint(x, y + ry - magic_y),
                gfxPoint(x + rx - magic_x, y),
                gfxPoint(x + rx, y));

  aCtx->ClosePath();
}
