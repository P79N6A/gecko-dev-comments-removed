




#include "mozilla/dom/SVGSwitchElement.h"
#include "nsSVGUtils.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/SVGSwitchElementBinding.h"

class nsIFrame;

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Switch)

namespace mozilla {
namespace dom {

JSObject*
SVGSwitchElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGSwitchElementBinding::Wrap(aCx, aScope, this);
}




NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(SVGSwitchElement,
                                                  SVGSwitchElementBase)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mActiveChild)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(SVGSwitchElement,
                                                SVGSwitchElementBase)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mActiveChild)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(SVGSwitchElement,SVGSwitchElementBase)
NS_IMPL_RELEASE_INHERITED(SVGSwitchElement,SVGSwitchElementBase)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(SVGSwitchElement)
  NS_NODE_INTERFACE_TABLE3(SVGSwitchElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement)
NS_INTERFACE_MAP_END_INHERITING(SVGSwitchElementBase)




SVGSwitchElement::SVGSwitchElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGSwitchElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}

void
SVGSwitchElement::MaybeInvalidate()
{
  
  
  

  nsIContent *newActiveChild = FindActiveChild();

  if (newActiveChild == mActiveChild) {
    return;
  }

  nsIFrame *frame = GetPrimaryFrame();
  if (frame) {
    nsSVGUtils::InvalidateBounds(frame, false);
    nsSVGUtils::ScheduleReflowSVG(frame);
  }

  mActiveChild = newActiveChild;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGSwitchElement)




nsresult
SVGSwitchElement::InsertChildAt(nsIContent* aKid,
                                uint32_t aIndex,
                                bool aNotify)
{
  nsresult rv = SVGSwitchElementBase::InsertChildAt(aKid, aIndex, aNotify);
  if (NS_SUCCEEDED(rv)) {
    MaybeInvalidate();
  }
  return rv;
}

void
SVGSwitchElement::RemoveChildAt(uint32_t aIndex, bool aNotify)
{
  SVGSwitchElementBase::RemoveChildAt(aIndex, aNotify);
  MaybeInvalidate();
}




NS_IMETHODIMP_(bool)
SVGSwitchElement::IsAttributeMapped(const nsIAtom* name) const
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

  return FindAttributeDependence(name, map) ||
    SVGSwitchElementBase::IsAttributeMapped(name);
}




nsIContent *
SVGSwitchElement::FindActiveChild() const
{
  bool allowReorder = AttrValueIs(kNameSpaceID_None,
                                    nsGkAtoms::allowReorder,
                                    nsGkAtoms::yes, eCaseMatters);

  const nsAdoptingString& acceptLangs =
    Preferences::GetLocalizedString("intl.accept_languages");

  if (allowReorder && !acceptLangs.IsEmpty()) {
    int32_t bestLanguagePreferenceRank = -1;
    nsIContent *bestChild = nullptr;
    for (nsIContent* child = nsINode::GetFirstChild();
         child;
         child = child->GetNextSibling()) {

      if (!child->IsElement()) {
        continue;
      }
      nsCOMPtr<SVGTests> tests(do_QueryInterface(child));
      if (tests) {
        if (tests->PassesConditionalProcessingTests(
                            SVGTests::kIgnoreSystemLanguage)) {
          int32_t languagePreferenceRank =
              tests->GetBestLanguagePreferenceRank(acceptLangs);
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
        }
      } else if (!bestChild) {
         bestChild = child;
      }
    }
    return bestChild;
  }

  for (nsIContent* child = nsINode::GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (!child->IsElement()) {
      continue;
    }
    nsCOMPtr<SVGTests> tests(do_QueryInterface(child));
    if (!tests || tests->PassesConditionalProcessingTests(&acceptLangs)) {
      return child;
    }
  }
  return nullptr;
}

} 
} 

