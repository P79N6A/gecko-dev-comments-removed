








#include "nsHTMLCSSStyleSheet.h"
#include "mozilla/css/StyleRule.h"
#include "nsIStyleRuleProcessor.h"
#include "nsPresContext.h"
#include "nsIDocument.h"
#include "nsCOMPtr.h"
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
  : mDocument(nullptr)
{
}

nsHTMLCSSStyleSheet::~nsHTMLCSSStyleSheet()
{
  
  
  mCachedStyleAttrs.Enumerate(ClearAttrCache, nullptr);
}

NS_IMPL_ISUPPORTS2(nsHTMLCSSStyleSheet,
                   nsIStyleSheet,
                   nsIStyleRuleProcessor)

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

nsresult
nsHTMLCSSStyleSheet::Init(nsIURI* aURL, nsIDocument* aDocument)
{
  NS_PRECONDITION(aURL && aDocument, "null ptr");
  if (! aURL || ! aDocument)
    return NS_ERROR_NULL_POINTER;

  if (mURL || mDocument)
    return NS_ERROR_ALREADY_INITIALIZED;

  mDocument = aDocument; 
  mURL = aURL;
  mCachedStyleAttrs.Init();
  return NS_OK;
}


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
nsHTMLCSSStyleSheet::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return 0;
}

 size_t
nsHTMLCSSStyleSheet::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
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

void
nsHTMLCSSStyleSheet::Reset(nsIURI* aURL)
{
  mURL = aURL;
}

 nsIURI*
nsHTMLCSSStyleSheet::GetSheetURI() const
{
  return mURL;
}

 nsIURI*
nsHTMLCSSStyleSheet::GetBaseURI() const
{
  return mURL;
}

 void
nsHTMLCSSStyleSheet::GetTitle(nsString& aTitle) const
{
  aTitle.AssignLiteral("Internal HTML/CSS Style Sheet");
}

 void
nsHTMLCSSStyleSheet::GetType(nsString& aType) const
{
  aType.AssignLiteral("text/html");
}

 bool
nsHTMLCSSStyleSheet::HasRules() const
{
  
  return true;
}

 bool
nsHTMLCSSStyleSheet::IsApplicable() const
{
  return true;
}

 void
nsHTMLCSSStyleSheet::SetEnabled(bool aEnabled)
{ 
}

 bool
nsHTMLCSSStyleSheet::IsComplete() const
{
  return true;
}

 void
nsHTMLCSSStyleSheet::SetComplete()
{
}


 nsIStyleSheet*
nsHTMLCSSStyleSheet::GetParentSheet() const
{
  return nullptr;
}

 nsIDocument*
nsHTMLCSSStyleSheet::GetOwningDocument() const
{
  return mDocument;
}

 void
nsHTMLCSSStyleSheet::SetOwningDocument(nsIDocument* aDocument)
{
  mDocument = aDocument;
}

#ifdef DEBUG
 void
nsHTMLCSSStyleSheet::List(FILE* out, int32_t aIndent) const
{
  
  for (int32_t index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("HTML CSS Style Sheet: ", out);
  nsAutoCString urlSpec;
  mURL->GetSpec(urlSpec);
  if (!urlSpec.IsEmpty()) {
    fputs(urlSpec.get(), out);
  }
  fputs("\n", out);
}
#endif
