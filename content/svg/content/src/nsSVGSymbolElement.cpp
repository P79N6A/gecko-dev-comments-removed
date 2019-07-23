



































#include "nsIDOMSVGSymbolElement.h"
#include "nsSVGStylableElement.h"
#include "nsSVGAnimatedPreserveAspectRatio.h"
#include "nsSVGPreserveAspectRatio.h"
#include "nsIDOMSVGRect.h"
#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsSVGRect.h"
#include "nsSVGAnimatedRect.h"
#include "nsGkAtoms.h"

typedef nsSVGStylableElement nsSVGSymbolElementBase;

class nsSVGSymbolElement : public nsSVGSymbolElementBase,
                           public nsIDOMSVGFitToViewBox,
                           public nsIDOMSVGSymbolElement
{
protected:
  friend nsresult NS_NewSVGSymbolElement(nsIContent **aResult,
                                         nsINodeInfo *aNodeInfo);
  nsSVGSymbolElement(nsINodeInfo* aNodeInfo);
  nsresult Init();

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSYMBOLELEMENT
  NS_DECL_NSIDOMSVGFITTOVIEWBOX

  
  NS_FORWARD_NSIDOMNODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  
  NS_IMETHODIMP_(PRBool) IsAttributeMapped(const nsIAtom* name) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  nsCOMPtr<nsIDOMSVGAnimatedRect> mViewBox;
  nsCOMPtr<nsIDOMSVGAnimatedPreserveAspectRatio> mPreserveAspectRatio;
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Symbol)




NS_IMPL_ADDREF_INHERITED(nsSVGSymbolElement,nsSVGSymbolElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGSymbolElement,nsSVGSymbolElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGSymbolElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGFitToViewBox)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGSymbolElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGSymbolElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGSymbolElementBase)




nsSVGSymbolElement::nsSVGSymbolElement(nsINodeInfo *aNodeInfo)
  : nsSVGSymbolElementBase(aNodeInfo)
{
}


nsresult
nsSVGSymbolElement::Init()
{
  nsresult rv = nsSVGSymbolElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);


  
  {
    nsCOMPtr<nsIDOMSVGRect> viewbox;
    rv = NS_NewSVGRect(getter_AddRefs(viewbox));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedRect(getter_AddRefs(mViewBox), viewbox);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::viewBox, mViewBox);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGPreserveAspectRatio> preserveAspectRatio;
    rv = NS_NewSVGPreserveAspectRatio(getter_AddRefs(preserveAspectRatio));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedPreserveAspectRatio(
                                          getter_AddRefs(mPreserveAspectRatio),
                                          preserveAspectRatio);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::preserveAspectRatio,
                           mPreserveAspectRatio);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return NS_OK;
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGSymbolElement)





NS_IMETHODIMP nsSVGSymbolElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  *aViewBox = mViewBox;
  NS_ADDREF(*aViewBox);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGSymbolElement::GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio * *aPreserveAspectRatio)
{
  *aPreserveAspectRatio = mPreserveAspectRatio;
  NS_ADDREF(*aPreserveAspectRatio);
  return NS_OK;
}




NS_IMETHODIMP_(PRBool)
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
