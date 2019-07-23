



































#include "nsSVGClipPathElement.h"
#include "nsGkAtoms.h"
#include "nsSVGEnum.h"

NS_IMPL_NS_NEW_SVG_ELEMENT(ClipPath)




NS_IMPL_ADDREF_INHERITED(nsSVGClipPathElement,nsSVGClipPathElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGClipPathElement,nsSVGClipPathElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGClipPathElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGClipPathElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGClipPathElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGClipPathElementBase)




nsSVGClipPathElement::nsSVGClipPathElement(nsINodeInfo *aNodeInfo)
  : nsSVGClipPathElementBase(aNodeInfo)
{
}


nsresult
nsSVGClipPathElement::Init()
{
  nsresult rv = nsSVGClipPathElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  
  static struct nsSVGEnumMapping gUnitMap[] = {
    {&nsGkAtoms::objectBoundingBox, nsIDOMSVGClipPathElement::SVG_CPUNITS_OBJECTBOUNDINGBOX},
    {&nsGkAtoms::userSpaceOnUse, nsIDOMSVGClipPathElement::SVG_CPUNITS_USERSPACEONUSE},
    {nsnull, 0}
  };

  
  {
    nsCOMPtr<nsISVGEnum> units;
    rv = NS_NewSVGEnum(getter_AddRefs(units),
                       nsIDOMSVGClipPathElement::SVG_CPUNITS_USERSPACEONUSE,
                       gUnitMap);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(mClipPathUnits), units);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::clipPathUnits, mClipPathUnits);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return NS_OK;
}


NS_IMETHODIMP nsSVGClipPathElement::GetClipPathUnits(nsIDOMSVGAnimatedEnumeration * *aClipPathUnits)
{
  *aClipPathUnits = mClipPathUnits;
  NS_IF_ADDREF(*aClipPathUnits);
  return NS_OK;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGClipPathElement)

