








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

using namespace mozilla;
using namespace mozilla::dom;

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

NS_IMPL_ISUPPORTS(nsHTMLCSSStyleSheet, nsIStyleRuleProcessor)

 void
nsHTMLCSSStyleSheet::RulesMatching(ElementRuleProcessorData* aData)
{
  ElementRulesMatching(aData->mPresContext, aData->mElement,
                       aData->mRuleWalker);
}

void
nsHTMLCSSStyleSheet::ElementRulesMatching(nsPresContext* aPresContext,
                                          Element* aElement,
                                          nsRuleWalker* aRuleWalker)
{
  
  css::StyleRule* rule = aElement->GetInlineStyleRule();
  if (rule) {
    rule->RuleMatched();
    aRuleWalker->Forward(rule);
  }

  rule = aElement->GetSMILOverrideStyleRule();
  if (rule) {
    if (aPresContext->IsProcessingRestyles() &&
        !aPresContext->IsProcessingAnimationStyleChange()) {
      
      
      
      aPresContext->PresShell()->RestyleForAnimation(aElement,
                                                     eRestyle_Self);
    } else {
      
      
      rule->RuleMatched();
      aRuleWalker->Forward(rule);
    }
  }
}

 void
nsHTMLCSSStyleSheet::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  if (nsCSSPseudoElements::PseudoElementSupportsStyleAttribute(aData->mPseudoType)) {
    MOZ_ASSERT(aData->mPseudoElement,
        "If pseudo element is supposed to support style attribute, it must "
        "have a pseudo element set");

    
    css::StyleRule* rule = aData->mPseudoElement->GetInlineStyleRule();
    if (rule) {
      rule->RuleMatched();
      aData->mRuleWalker->Forward(rule);
    }
  }
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

 nsRestyleHint
nsHTMLCSSStyleSheet::HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData)
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
SizeOfCachedStyleAttrsEntryExcludingThis(nsStringHashKey::KeyType& aKey,
                                         MiscContainer* const& aData,
                                         mozilla::MallocSizeOf aMallocSizeOf,
                                         void* userArg)
{
  
  
  return aKey.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
}

 size_t
nsHTMLCSSStyleSheet::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  
  
  
  return mCachedStyleAttrs.SizeOfExcludingThis(SizeOfCachedStyleAttrsEntryExcludingThis,
                                               aMallocSizeOf,
                                               nullptr);
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
