



































#include "nsIDOMSVGSymbolElement.h"
#include "nsSVGStylableElement.h"
#include "nsSVGViewBox.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsGkAtoms.h"

using namespace mozilla;
typedef nsSVGStylableElement nsSVGSymbolElementBase;

class nsSVGSymbolElement : public nsSVGSymbolElementBase,
                           public nsIDOMSVGFitToViewBox,
                           public nsIDOMSVGSymbolElement
{
protected:
  friend nsresult NS_NewSVGSymbolElement(nsIContent **aResult,
                                         already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGSymbolElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSYMBOLELEMENT
  NS_DECL_NSIDOMSVGFITTOVIEWBOX

  
  NS_FORWARD_NSIDOMNODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  virtual nsSVGViewBox *GetViewBox();
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();

  nsSVGViewBox mViewBox;
  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Symbol)




NS_IMPL_ADDREF_INHERITED(nsSVGSymbolElement,nsSVGSymbolElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGSymbolElement,nsSVGSymbolElementBase)

DOMCI_NODE_DATA(SVGSymbolElement, nsSVGSymbolElement)

NS_INTERFACE_TABLE_HEAD(nsSVGSymbolElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGSymbolElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGFitToViewBox,
                           nsIDOMSVGSymbolElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGSymbolElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGSymbolElementBase)




nsSVGSymbolElement::nsSVGSymbolElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGSymbolElementBase(aNodeInfo)
{
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGSymbolElement)





NS_IMETHODIMP nsSVGSymbolElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  return mViewBox.ToDOMAnimatedRect(aViewBox, this);
}


NS_IMETHODIMP
nsSVGSymbolElement::GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio
                                           **aPreserveAspectRatio)
{
  return mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(aPreserveAspectRatio, this);
}




NS_IMETHODIMP_(bool)
nsSVGSymbolElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFEFloodMap,
    sFillStrokeMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sGraphicsMap,
    sLightingEffectsMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
   };

  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGSymbolElementBase::IsAttributeMapped(name);
}




nsSVGViewBox *
nsSVGSymbolElement::GetViewBox()
{
  return &mViewBox;
}

SVGAnimatedPreserveAspectRatio *
nsSVGSymbolElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}
