








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
#include "RestyleManager.h"

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
    RestyleManager* restyleManager = aPresContext->RestyleManager();
    if (!restyleManager->SkipAnimationRules()) {
      
      
      rule->RuleMatched();
      aRuleWalker->Forward(rule);
    }
  }
}

void
nsHTMLCSSStyleSheet::PseudoElementRulesMatching(Element* aPseudoElement,
                                                nsCSSPseudoElements::Type
                                                  aPseudoType,
                                                nsRuleWalker* aRuleWalker)
{
  MOZ_ASSERT(nsCSSPseudoElements::
               PseudoElementSupportsStyleAttribute(aPseudoType));
  MOZ_ASSERT(aPseudoElement);

  
  css::StyleRule* rule = aPseudoElement->GetInlineStyleRule();
  if (rule) {
    rule->RuleMatched();
    aRuleWalker->Forward(rule);
  }
}

 void
nsHTMLCSSStyleSheet::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  if (nsCSSPseudoElements::PseudoElementSupportsStyleAttribute(aData->mPseudoType)) {
    MOZ_ASSERT(aData->mPseudoElement,
        "If pseudo element is supposed to support style attribute, it must "
        "have a pseudo element set");

    PseudoElementRulesMatching(aData->mPseudoElement, aData->mPseudoType,
                               aData->mRuleWalker);
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
    return eRestyle_StyleAttribute;
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
    NS_ASSERTION(aValue == mCachedStyleAttrs.Get(aSerialized),
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
