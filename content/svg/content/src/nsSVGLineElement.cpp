






































#include "mozilla/Util.h"

#include "nsSVGPathGeometryElement.h"
#include "nsIDOMSVGLineElement.h"
#include "nsSVGLength2.h"
#include "nsGkAtoms.h"
#include "gfxContext.h"

using namespace mozilla;

typedef nsSVGPathGeometryElement nsSVGLineElementBase;

class nsSVGLineElement : public nsSVGLineElementBase,
                         public nsIDOMSVGLineElement
{
protected:
  friend nsresult NS_NewSVGLineElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGLineElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGLINEELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGLineElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGLineElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGLineElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const;

  
  virtual bool IsMarkable() { return true; }
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:

  virtual LengthAttributesInfo GetLengthInfo();

  enum { X1, Y1, X2, Y2 };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

nsSVGElement::LengthInfo nsSVGLineElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x1, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::y1, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::x2, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::y2, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Line)




NS_IMPL_ADDREF_INHERITED(nsSVGLineElement,nsSVGLineElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGLineElement,nsSVGLineElementBase)

DOMCI_NODE_DATA(SVGLineElement, nsSVGLineElement)

NS_INTERFACE_TABLE_HEAD(nsSVGLineElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGLineElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTests,
                           nsIDOMSVGLineElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGLineElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGLineElementBase)




nsSVGLineElement::nsSVGLineElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGLineElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGLineElement)





NS_IMETHODIMP nsSVGLineElement::GetX1(nsIDOMSVGAnimatedLength * *aX1)
{
  return mLengthAttributes[X1].ToDOMAnimatedLength(aX1, this);
}


NS_IMETHODIMP nsSVGLineElement::GetY1(nsIDOMSVGAnimatedLength * *aY1)
{
  return mLengthAttributes[Y1].ToDOMAnimatedLength(aY1, this);
}


NS_IMETHODIMP nsSVGLineElement::GetX2(nsIDOMSVGAnimatedLength * *aX2)
{
  return mLengthAttributes[X2].ToDOMAnimatedLength(aX2, this);
}


NS_IMETHODIMP nsSVGLineElement::GetY2(nsIDOMSVGAnimatedLength * *aY2)
{
  return mLengthAttributes[Y2].ToDOMAnimatedLength(aY2, this);
}




NS_IMETHODIMP_(bool)
nsSVGLineElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sMarkersMap
  };
  
  return FindAttributeDependence(name, map) ||
    nsSVGLineElementBase::IsAttributeMapped(name);
}




nsSVGElement::LengthAttributesInfo
nsSVGLineElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}




void
nsSVGLineElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks) {
  float x1, y1, x2, y2;

  GetAnimatedLengthValues(&x1, &y1, &x2, &y2, nsnull);

  float angle = atan2(y2 - y1, x2 - x1);

  aMarks->AppendElement(nsSVGMark(x1, y1, angle));
  aMarks->AppendElement(nsSVGMark(x2, y2, angle));
}

void
nsSVGLineElement::ConstructPath(gfxContext *aCtx)
{
  float x1, y1, x2, y2;

  GetAnimatedLengthValues(&x1, &y1, &x2, &y2, nsnull);

  aCtx->MoveTo(gfxPoint(x1, y1));
  aCtx->LineTo(gfxPoint(x2, y2));
}
