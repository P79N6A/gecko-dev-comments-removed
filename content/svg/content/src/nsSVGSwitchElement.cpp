



































#include "nsSVGSwitchElement.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsSVGUtils.h"

PRBool 
NS_SVG_PassesConditionalProcessingTests(nsIContent *aContent);





NS_IMPL_NS_NEW_SVG_ELEMENT(Switch)





NS_IMPL_ADDREF_INHERITED(nsSVGSwitchElement,nsSVGSwitchElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGSwitchElement,nsSVGSwitchElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGSwitchElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGSwitchElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGSwitchElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGSwitchElementBase)




nsSVGSwitchElement::nsSVGSwitchElement(nsINodeInfo *aNodeInfo)
  : nsSVGSwitchElementBase(aNodeInfo)
{
}

void
nsSVGSwitchElement::MaybeInvalidate()
{
  
  
  

  PRUint32 count = GetChildCount();
  for (PRUint32 i = 0; i < count; i++) {
    nsIContent * child = GetChildAt(i);
    if (child->IsNodeOfType(nsINode::eELEMENT) &&
        NS_SVG_PassesConditionalProcessingTests(child)) {

      if (mActiveChild == child) {
        return;
      }

      nsIFrame *frame = GetPrimaryFrame();
      if (frame) {
        nsISVGChildFrame* svgFrame = nsnull;

        CallQueryInterface(frame, &svgFrame);
        if (svgFrame) {
          nsSVGUtils::UpdateGraphic(svgFrame);
        }
      }
      return;
    }
  }
}

void
nsSVGSwitchElement::UpdateActiveChild()
{
  PRUint32 count = GetChildCount();
  for (PRUint32 i = 0; i < count; i++) {
    nsIContent * child = GetChildAt(i);
    if (child->IsNodeOfType(nsINode::eELEMENT) &&
        NS_SVG_PassesConditionalProcessingTests(child)) {
      mActiveChild = child;
      return;
    }
  }
  mActiveChild = nsnull;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGSwitchElement)




nsresult
nsSVGSwitchElement::InsertChildAt(nsIContent* aKid,
                                  PRUint32 aIndex,
                                  PRBool aNotify)
{
  nsresult rv = nsSVGSwitchElementBase::InsertChildAt(aKid, aIndex, aNotify);
  if (NS_SUCCEEDED(rv)) {
    MaybeInvalidate();
  }
  return rv;
}

nsresult
nsSVGSwitchElement::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
  nsresult rv = nsSVGSwitchElementBase::RemoveChildAt(aIndex, aNotify);
  if (NS_SUCCEEDED(rv)) {
    MaybeInvalidate();
  }
  return rv;
}
 



NS_IMETHODIMP_(PRBool)
nsSVGSwitchElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sLightingEffectsMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGSwitchElementBase::IsAttributeMapped(name);
}
