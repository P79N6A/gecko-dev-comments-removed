









































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
  nsIContent* content = aData->mContent;

  
  nsICSSStyleRule* rule = content->GetInlineStyleRule();
  if (rule) {
    rule->RuleMatched();
    aData->mRuleWalker->Forward(rule);
  }

#ifdef MOZ_SMIL
  rule = content->GetSMILOverrideStyleRule();
  if (rule) {
    if (aData->mPresContext->IsProcessingRestyles() &&
        !aData->mPresContext->IsProcessingAnimationStyleChange()) {
      
      
      
      aData->mPresContext->PresShell()->RestyleForAnimation(aData->mContent);
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

NS_IMETHODIMP
nsHTMLCSSStyleSheet::GetSheetURI(nsIURI** aSheetURL) const
{
  NS_IF_ADDREF(mURL);
  *aSheetURL = mURL;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::GetBaseURI(nsIURI** aBaseURL) const
{
  NS_IF_ADDREF(mURL);
  *aBaseURL = mURL;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::GetTitle(nsString& aTitle) const
{
  aTitle.AssignLiteral("Internal HTML/CSS Style Sheet");
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::GetType(nsString& aType) const
{
  aType.AssignLiteral("text/html");
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsHTMLCSSStyleSheet::HasRules() const
{
  
  return PR_TRUE;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::GetApplicable(PRBool& aApplicable) const
{
  aApplicable = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::SetEnabled(PRBool aEnabled)
{ 
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::GetComplete(PRBool& aComplete) const
{
  aComplete = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::SetComplete()
{
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLCSSStyleSheet::GetParentSheet(nsIStyleSheet*& aParent) const
{
  aParent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::GetOwningDocument(nsIDocument*& aDocument) const
{
  NS_IF_ADDREF(mDocument);
  aDocument = mDocument;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCSSStyleSheet::SetOwningDocument(nsIDocument* aDocument)
{
  mDocument = aDocument;
  return NS_OK;
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
