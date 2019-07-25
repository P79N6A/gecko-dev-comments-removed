








































#ifndef nsHTMLCSSStyleSheet_h_
#define nsHTMLCSSStyleSheet_h_

#include "nsIStyleSheet.h"
#include "nsIStyleRuleProcessor.h"

class nsHTMLCSSStyleSheet : public nsIStyleSheet,
                            public nsIStyleRuleProcessor {
public:
  nsHTMLCSSStyleSheet();

  NS_DECL_ISUPPORTS

  nsresult Init(nsIURI* aURL, nsIDocument* aDocument);
  void Reset(nsIURI* aURL);

  
  virtual nsIURI* GetSheetURI() const;
  virtual nsIURI* GetBaseURI() const;
  virtual void GetTitle(nsString& aTitle) const;
  virtual void GetType(nsString& aType) const;
  virtual PRBool HasRules() const;
  virtual PRBool IsApplicable() const;
  virtual void SetEnabled(PRBool aEnabled);
  virtual PRBool IsComplete() const;
  virtual void SetComplete();
  virtual nsIStyleSheet* GetParentSheet() const;  
  virtual nsIDocument* GetOwningDocument() const;
  virtual void SetOwningDocument(nsIDocument* aDocument);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual void RulesMatching(ElementRuleProcessorData* aData);
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData);
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData);
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData);
#endif
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData);
  virtual PRBool HasDocumentStateDependentStyle(StateRuleProcessorData* aData);
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData);
  virtual PRBool MediumFeaturesChanged(nsPresContext* aPresContext);

private: 
  
  nsHTMLCSSStyleSheet(const nsHTMLCSSStyleSheet& aCopy); 
  nsHTMLCSSStyleSheet& operator=(const nsHTMLCSSStyleSheet& aCopy); 

protected:
  nsCOMPtr<nsIURI> mURL;
  nsIDocument*     mDocument;
};

#endif 
