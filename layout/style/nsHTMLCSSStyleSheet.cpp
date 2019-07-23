









































#include "nsIHTMLCSSStyleSheet.h"
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



class HTMLCSSStyleSheetImpl : public nsIHTMLCSSStyleSheet,
                              public nsIStyleRuleProcessor {
public:
  HTMLCSSStyleSheetImpl();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD Init(nsIURI* aURL, nsIDocument* aDocument);
  NS_IMETHOD Reset(nsIURI* aURL);
  NS_IMETHOD GetSheetURI(nsIURI** aSheetURL) const;
  NS_IMETHOD GetBaseURI(nsIURI** aBaseURL) const;
  NS_IMETHOD GetTitle(nsString& aTitle) const;
  NS_IMETHOD GetType(nsString& aType) const;
  NS_IMETHOD_(PRBool) HasRules() const;

  NS_IMETHOD GetApplicable(PRBool& aApplicable) const;
  
  NS_IMETHOD SetEnabled(PRBool aEnabled);

  NS_IMETHOD GetComplete(PRBool& aComplete) const;
  NS_IMETHOD SetComplete();

  
  NS_IMETHOD GetParentSheet(nsIStyleSheet*& aParent) const;  
  NS_IMETHOD GetOwningDocument(nsIDocument*& aDocument) const;
  NS_IMETHOD SetOwningDocument(nsIDocument* aDocument);

  
  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData);

  NS_IMETHOD RulesMatching(PseudoElementRuleProcessorData* aData);

  NS_IMETHOD RulesMatching(AnonBoxRuleProcessorData* aData);

  NS_IMETHOD RulesMatching(PseudoRuleProcessorData* aData);

  NS_IMETHOD HasStateDependentStyle(StateRuleProcessorData* aData,
                                    nsReStyleHint* aResult);

  virtual nsReStyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData);
  NS_IMETHOD MediumFeaturesChanged(nsPresContext* aPresContext,
                                  PRBool* aResult);

#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

private: 
  
  HTMLCSSStyleSheetImpl(const HTMLCSSStyleSheetImpl& aCopy); 
  HTMLCSSStyleSheetImpl& operator=(const HTMLCSSStyleSheetImpl& aCopy); 

protected:
  virtual ~HTMLCSSStyleSheetImpl();

protected:
  nsIURI*         mURL;
  nsIDocument*    mDocument;
};


HTMLCSSStyleSheetImpl::HTMLCSSStyleSheetImpl()
  : nsIHTMLCSSStyleSheet(),
    mRefCnt(0),
    mURL(nsnull),
    mDocument(nsnull)
{
}

HTMLCSSStyleSheetImpl::~HTMLCSSStyleSheetImpl()
{
  NS_RELEASE(mURL);
}

NS_IMPL_ISUPPORTS3(HTMLCSSStyleSheetImpl,
                   nsIHTMLCSSStyleSheet,
                   nsIStyleSheet,
                   nsIStyleRuleProcessor)

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::RulesMatching(ElementRuleProcessorData* aData)
{
  nsIContent* content = aData->mContent;
  
  if (content) {
    
    nsICSSStyleRule* rule = content->GetInlineStyleRule();
    if (rule)
      aData->mRuleWalker->Forward(rule);

#ifdef MOZ_SMIL
    rule = content->GetSMILOverrideStyleRule();
    if (rule)
      aData->mRuleWalker->Forward(rule);
#endif 
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::RulesMatching(AnonBoxRuleProcessorData* aData)
{
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::RulesMatching(PseudoRuleProcessorData* aData)
{
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::Init(nsIURI* aURL, nsIDocument* aDocument)
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


NS_IMETHODIMP
HTMLCSSStyleSheetImpl::HasStateDependentStyle(StateRuleProcessorData* aData,
                                              nsReStyleHint* aResult)
{
  *aResult = nsReStyleHint(0);
  return NS_OK;
}


nsReStyleHint
HTMLCSSStyleSheetImpl::HasAttributeDependentStyle(AttributeRuleProcessorData* aData)
{
  return nsReStyleHint(0);
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::MediumFeaturesChanged(nsPresContext* aPresContext,
                                             PRBool* aRulesChanged)
{
  *aRulesChanged = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP 
HTMLCSSStyleSheetImpl::Reset(nsIURI* aURL)
{
  NS_IF_RELEASE(mURL);
  mURL = aURL;
  NS_ADDREF(mURL);

  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetSheetURI(nsIURI** aSheetURL) const
{
  NS_IF_ADDREF(mURL);
  *aSheetURL = mURL;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetBaseURI(nsIURI** aBaseURL) const
{
  NS_IF_ADDREF(mURL);
  *aBaseURL = mURL;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetTitle(nsString& aTitle) const
{
  aTitle.AssignLiteral("Internal HTML/CSS Style Sheet");
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetType(nsString& aType) const
{
  aType.AssignLiteral("text/html");
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
HTMLCSSStyleSheetImpl::HasRules() const
{
  
  return PR_TRUE;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetApplicable(PRBool& aApplicable) const
{
  aApplicable = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::SetEnabled(PRBool aEnabled)
{ 
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetComplete(PRBool& aComplete) const
{
  aComplete = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::SetComplete()
{
  return NS_OK;
}


NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetParentSheet(nsIStyleSheet*& aParent) const
{
  aParent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetOwningDocument(nsIDocument*& aDocument) const
{
  NS_IF_ADDREF(mDocument);
  aDocument = mDocument;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::SetOwningDocument(nsIDocument* aDocument)
{
  mDocument = aDocument;
  return NS_OK;
}

#ifdef DEBUG
void HTMLCSSStyleSheetImpl::List(FILE* out, PRInt32 aIndent) const
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


nsresult
NS_NewHTMLCSSStyleSheet(nsIHTMLCSSStyleSheet** aInstancePtrResult,
                        nsIURI* aURL, nsIDocument* aDocument)
{
  nsresult rv;
  nsIHTMLCSSStyleSheet* sheet;
  if (NS_FAILED(rv = NS_NewHTMLCSSStyleSheet(&sheet)))
    return rv;

  if (NS_FAILED(rv = sheet->Init(aURL, aDocument))) {
    NS_RELEASE(sheet);
    return rv;
  }

  *aInstancePtrResult = sheet;
  return NS_OK;
}

nsresult
NS_NewHTMLCSSStyleSheet(nsIHTMLCSSStyleSheet** aInstancePtrResult)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  HTMLCSSStyleSheetImpl*  it = new HTMLCSSStyleSheetImpl();

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(it);
  *aInstancePtrResult = it;
  return NS_OK;
}
