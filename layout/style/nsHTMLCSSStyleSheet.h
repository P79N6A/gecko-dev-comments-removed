








































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
  nsresult Reset(nsIURI* aURL);

  
  virtual already_AddRefed<nsIURI> GetSheetURI() const;
  virtual already_AddRefed<nsIURI> GetBaseURI() const;
  virtual void GetTitle(nsString& aTitle) const;
  virtual void GetType(nsString& aType) const;
  virtual PRBool HasRules() const;
  virtual PRBool IsApplicable() const;
  virtual void SetEnabled(PRBool aEnabled);
  virtual PRBool IsComplete() const;
  virtual void SetComplete();
  virtual already_AddRefed<nsIStyleSheet> GetParentSheet() const;  
  virtual already_AddRefed<nsIDocument> GetOwningDocument() const;
  virtual void SetOwningDocument(nsIDocument* aDocument);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData);
  NS_IMETHOD RulesMatching(PseudoElementRuleProcessorData* aData);
  NS_IMETHOD RulesMatching(AnonBoxRuleProcessorData* aData);
#ifdef MOZ_XUL
  NS_IMETHOD RulesMatching(XULTreeRuleProcessorData* aData);
#endif
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData);
  virtual PRBool HasDocumentStateDependentStyle(StateRuleProcessorData* aData);
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData);
  NS_IMETHOD MediumFeaturesChanged(nsPresContext* aPresContext,
                                  PRBool* aResult);

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
