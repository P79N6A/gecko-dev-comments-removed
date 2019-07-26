








#include "nsHTMLCSSStyleSheet.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/css/StyleRule.h"
#include "nsIStyleRuleProcessor.h"
#include "nsPresContext.h"
#include "nsRuleWalker.h"
#include "nsRuleProcessorData.h"
#include "mozilla/dom/Element.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"

using namespace mozilla::dom;
namespace css = mozilla::css;

namespace {

PLDHashOperator
ClearAttrCache(const nsAString& aKey, MiscContainer*& aValue, void*)
{
  
  
  MOZ_ASSERT(aValue->mType == nsAttrValue::eCSSStyleRule);

  aValue->mValue.mCSSStyleRule->SetHTMLCSSStyleSheet(nullptr);
  aValue->mValue.mCached = 0;

  return PL_DHASH_REMOVE;
}

} 

nsHTMLCSSStyleSheet::nsHTMLCSSStyleSheet()
{
}

nsHTMLCSSStyleSheet::~nsHTMLCSSStyleSheet()
{
  
  
  mCachedStyleAttrs.Enumerate(ClearAttrCache, nullptr);
}

NS_IMPL_ISUPPORTS1(nsHTMLCSSStyleSheet, nsIStyleRuleProcessor)

 void
nsHTMLCSSStyleSheet::RulesMatching(ElementRuleProcessorData* aData)
{
  Element* element = aData->mElement;

  
  css::StyleRule* rule = element->GetInlineStyleRule();
  if (rule) {
    rule->RuleMatched();
    aData->mRuleWalker->Forward(rule);
  }

  rule = element->GetSMILOverrideStyleRule();
  if (rule) {
    if (aData->mPresContext->IsProcessingRestyles() &&
        !aData->mPresContext->IsProcessingAnimationStyleChange()) {
      
      
      
      aData->mPresContext->PresShell()->RestyleForAnimation(element,
                                                            eRestyle_Self);
    } else {
      
      
      rule->RuleMatched();
      aData->mRuleWalker->Forward(rule);
    }
  }
}

 void
nsHTMLCSSStyleSheet::RulesMatching(PseudoElementRuleProcessorData* aData)
{
}

 void
nsHTMLCSSStyleSheet::RulesMatching(AnonBoxRuleProcessorData* aData)
{
}

#ifdef MOZ_XUL
 void
nsHTMLCSSStyleSheet::RulesMatching(XULTreeRuleProcessorData* aData)
{
}
#endif


 nsRestyleHint
nsHTMLCSSStyleSheet::HasStateDependentStyle(StateRuleProcessorData* aData)
{
  return nsRestyleHint(0);
}

 bool
nsHTMLCSSStyleSheet::HasDocumentStateDependentStyle(StateRuleProcessorData* aData)
{
  return false;
}


 nsRestyleHint
nsHTMLCSSStyleSheet::HasAttributeDependentStyle(AttributeRuleProcessorData* aData)
{
  
  
  if (aData->mAttrHasChanged && aData->mAttribute == nsGkAtoms::style) {
    return eRestyle_Self;
  }

  return nsRestyleHint(0);
}

 bool
nsHTMLCSSStyleSheet::MediumFeaturesChanged(nsPresContext* aPresContext)
{
  return false;
}

 size_t
nsHTMLCSSStyleSheet::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return 0;
}

 size_t
nsHTMLCSSStyleSheet::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

void
nsHTMLCSSStyleSheet::CacheStyleAttr(const nsAString& aSerialized,
                                    MiscContainer* aValue)
{
  mCachedStyleAttrs.Put(aSerialized, aValue);
}

void
nsHTMLCSSStyleSheet::EvictStyleAttr(const nsAString& aSerialized,
                                    MiscContainer* aValue)
{
#ifdef DEBUG
  {
    NS_ASSERTION(aValue = mCachedStyleAttrs.Get(aSerialized),
                 "Cached value does not match?!");
  }
#endif
  mCachedStyleAttrs.Remove(aSerialized);
}

MiscContainer*
nsHTMLCSSStyleSheet::LookupStyleAttr(const nsAString& aSerialized)
{
  return mCachedStyleAttrs.Get(aSerialized);
}
