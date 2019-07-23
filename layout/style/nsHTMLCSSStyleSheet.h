








































#ifndef nsHTMLCSSStyleSheet_h_
#define nsHTMLCSSStyleSheet_h_

#include "nsIHTMLCSSStyleSheet.h"
#include "nsIStyleRuleProcessor.h"

class nsHTMLCSSStyleSheet : public nsIHTMLCSSStyleSheet,
                            public nsIStyleRuleProcessor {
public:
  nsHTMLCSSStyleSheet();

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

#ifdef MOZ_XUL
  NS_IMETHOD RulesMatching(XULTreeRuleProcessorData* aData);
#endif

  virtual nsReStyleHint HasStateDependentStyle(StateRuleProcessorData* aData);

  virtual nsReStyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData);
  NS_IMETHOD MediumFeaturesChanged(nsPresContext* aPresContext,
                                  PRBool* aResult);

#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

private: 
  
  nsHTMLCSSStyleSheet(const nsHTMLCSSStyleSheet& aCopy); 
  nsHTMLCSSStyleSheet& operator=(const nsHTMLCSSStyleSheet& aCopy); 

protected:
  virtual ~nsHTMLCSSStyleSheet();

protected:
  nsIURI*         mURL;
  nsIDocument*    mDocument;
};

#endif 
