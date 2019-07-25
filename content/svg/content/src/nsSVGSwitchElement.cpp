



































#include "nsSVGFeatures.h"
#include "nsSVGSwitchElement.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsSVGUtils.h"
#include "mozilla/Preferences.h"

using namespace mozilla;





NS_IMPL_NS_NEW_SVG_ELEMENT(Switch)





NS_IMPL_CYCLE_COLLECTION_CLASS(nsSVGSwitchElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsSVGSwitchElement,
                                                  nsSVGSwitchElementBase)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mActiveChild)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsSVGSwitchElement,
                                                nsSVGSwitchElementBase)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mActiveChild)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(nsSVGSwitchElement,nsSVGSwitchElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGSwitchElement,nsSVGSwitchElementBase)

DOMCI_NODE_DATA(SVGSwitchElement, nsSVGSwitchElement)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsSVGSwitchElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGSwitchElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGSwitchElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGSwitchElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGSwitchElementBase)




nsSVGSwitchElement::nsSVGSwitchElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGSwitchElementBase(aNodeInfo)
{
}

void
nsSVGSwitchElement::MaybeInvalidate()
{
  
  
  

  if (FindActiveChild() == mActiveChild) {
    return;
  }

  nsIFrame *frame = GetPrimaryFrame();
  if (frame) {
    nsISVGChildFrame* svgFrame = do_QueryFrame(frame);
    if (svgFrame) {
      nsSVGUtils::UpdateGraphic(svgFrame);
    }
  }
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




nsIContent *
nsSVGSwitchElement::FindActiveChild() const
{
  PRBool allowReorder = AttrValueIs(kNameSpaceID_None,
                                    nsGkAtoms::allowReorder,
                                    nsGkAtoms::yes, eCaseMatters);

  const nsAdoptingString& acceptLangs =
    Preferences::GetLocalizedString("intl.accept_languages");

  PRUint32 count = GetChildCount();

  if (allowReorder && !acceptLangs.IsEmpty()) {
    PRInt32 bestLanguagePreferenceRank = -1;
    nsIContent *bestChild = nsnull;
    for (PRUint32 i = 0; i < count; i++) {
      nsIContent *child = GetChildAt(i);
      if (nsSVGFeatures::PassesConditionalProcessingTests(
            child, nsSVGFeatures::kIgnoreSystemLanguage)) {
        nsAutoString value;
        if (child->GetAttr(kNameSpaceID_None, nsGkAtoms::systemLanguage,
                           value)) {
          PRInt32 languagePreferenceRank =
            nsSVGFeatures::GetBestLanguagePreferenceRank(value, acceptLangs);
          switch (languagePreferenceRank) {
          case 0:
            
            return child;
          case -1:
            
            break;
          default:
            if (bestLanguagePreferenceRank == -1 ||
                languagePreferenceRank < bestLanguagePreferenceRank) {
              bestLanguagePreferenceRank = languagePreferenceRank;
              bestChild = child;
            }
            break;
          }
        } else if (!bestChild) {
          bestChild = child;
        }
      }
    }
    return bestChild;
  }

  for (PRUint32 i = 0; i < count; i++) {
    nsIContent *child = GetChildAt(i);
    if (nsSVGFeatures::PassesConditionalProcessingTests(child, &acceptLangs)) {
      return child;
    }
  }
  return nsnull;
}
