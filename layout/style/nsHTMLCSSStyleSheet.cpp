









































#include "nsHTMLCSSStyleSheet.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIURL.h"
#include "nsCSSPseudoElements.h"
#include "nsIStyleRule.h"
#include "nsIFrame.h"
#include "nsICSSStyleRule.h"
#include "nsIStyleRuleProcessor.h"
#include "nsPresContext.h"
#include "nsIDocument.h"
#include "nsCOMPtr.h"
#include "nsRuleWalker.h"
#include "nsRuleData.h"
#include "nsRuleProcessorData.h"
#include "mozilla/dom/Element.h"

using namespace mozilla::dom;

nsHTMLCSSStyleSheet::nsHTMLCSSStyleSheet()
  : mRefCnt(0),
    mURL(nsnull),
    mDocument(nsnull)
{
}

nsHTMLCSSStyleSheet::~nsHTMLCSSStyleSheet()
{
  NS_RELEASE(mURL);
}

NS_IMPL_ISUPPORTS2(nsHTMLCSSStyleSheet,
                   nsIStyleSheet,
                   nsIStyleRuleProcessor)

NS_IMETHODIMP
nsHTMLCSSStyleSheet::RulesMatching(ElementRuleProcessorData* aData)
{
  Element* element = aData->mElement;

  
  nsICSSStyleRule* rule = element->GetInlineStyleRule();
  if (rule) {
    rule->RuleMatched();
    aData->mRuleWalker->Forward(rule);
  }

#ifdef MOZ_SMIL
  rule = element->GetSMILOverrideStyleRule();
  if (rule) {
    if (aData->mPresContext->IsProcessingRestyles() &&
        !aData->mPresContext->IsProcessingAnimationStyleChange()) {
      
      
      
      aData->mPresContext->PresShell()->RestyleForAnimation(element);
    } else {
      
      
      rule->RuleMatched();
      aData->mRuleWalker->Forward(rule);
    }
  }
#endif 

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::RulesMatching(AnonBoxRuleProcessorData* aData)
{
  return NS_OK;
}

#ifdef MOZ_XUL
NS_IMETHODIMP
nsHTMLCSSStyleSheet::RulesMatching(XULTreeRuleProcessorData* aData)
{
  return NS_OK;
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
  NS_ADDREF(mURL);
  return NS_OK;
}


nsRestyleHint
nsHTMLCSSStyleSheet::HasStateDependentStyle(StateRuleProcessorData* aData)
{
  return nsRestyleHint(0);
}

PRBool
nsHTMLCSSStyleSheet::HasDocumentStateDependentStyle(StateRuleProcessorData* aData)
{
  return PR_FALSE;
}


nsRestyleHint
nsHTMLCSSStyleSheet::HasAttributeDependentStyle(AttributeRuleProcessorData* aData)
{
  
  
  if (aData->mAttrHasChanged && aData->mAttribute == nsGkAtoms::style) {
    return eRestyle_Self;
  }

  return nsRestyleHint(0);
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::MediumFeaturesChanged(nsPresContext* aPresContext,
                                           PRBool* aRulesChanged)
{
  *aRulesChanged = PR_FALSE;
  return NS_OK;
}


nsresult
nsHTMLCSSStyleSheet::Reset(nsIURI* aURL)
{
  NS_IF_RELEASE(mURL);
  mURL = aURL;
  NS_ADDREF(mURL);

  return NS_OK;
}

already_AddRefed<nsIURI>
nsHTMLCSSStyleSheet::GetSheetURI() const
{
  NS_IF_ADDREF(mURL);
  return mURL;
}

already_AddRefed<nsIURI>
nsHTMLCSSStyleSheet::GetBaseURI() const
{
  NS_IF_ADDREF(mURL);
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

PRBool
nsHTMLCSSStyleSheet::HasRules() const
{
  
  return PR_TRUE;
}

PRBool
nsHTMLCSSStyleSheet::GetApplicable() const
{
  return PR_TRUE;
}

void
nsHTMLCSSStyleSheet::SetEnabled(PRBool aEnabled)
{ 
}

PRBool
nsHTMLCSSStyleSheet::GetComplete() const
{
  return PR_TRUE;
}

void
nsHTMLCSSStyleSheet::SetComplete()
{
}


already_AddRefed<nsIStyleSheet>
nsHTMLCSSStyleSheet::GetParentSheet() const
{
  return nsnull;
}

already_AddRefed<nsIDocument>
nsHTMLCSSStyleSheet::GetOwningDocument() const
{
  NS_IF_ADDREF(mDocument);
  return mDocument;
}

void
nsHTMLCSSStyleSheet::SetOwningDocument(nsIDocument* aDocument)
{
  mDocument = aDocument;
}

#ifdef DEBUG
void nsHTMLCSSStyleSheet::List(FILE* out, PRInt32 aIndent) const
{
  
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("HTML CSS Style Sheet: ", out);
  nsCAutoString urlSpec;
  mURL->GetSpec(urlSpec);
  if (!urlSpec.IsEmpty()) {
    fputs(urlSpec.get(), out);
  }
  fputs("\n", out);
}
#endif
