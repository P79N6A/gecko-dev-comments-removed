








#ifndef nsHTMLCSSStyleSheet_h_
#define nsHTMLCSSStyleSheet_h_

#include "mozilla/Attributes.h"

#include "nsIStyleSheet.h"
#include "nsIStyleRuleProcessor.h"

class nsHTMLCSSStyleSheet MOZ_FINAL : public nsIStyleSheet,
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
  virtual bool HasRules() const;
  virtual bool IsApplicable() const;
  virtual void SetEnabled(bool aEnabled);
  virtual bool IsComplete() const;
  virtual void SetComplete();
  virtual nsIStyleSheet* GetParentSheet() const;  
  virtual nsIDocument* GetOwningDocument() const;
  virtual void SetOwningDocument(nsIDocument* aDocument);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

  
  virtual void RulesMatching(ElementRuleProcessorData* aData);
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData);
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData);
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData);
#endif
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData);
  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData);
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData);
  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext);
  virtual NS_MUST_OVERRIDE size_t
    SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE;
  virtual NS_MUST_OVERRIDE size_t
    SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE;

private: 
  nsHTMLCSSStyleSheet(const nsHTMLCSSStyleSheet& aCopy) MOZ_DELETE;
  nsHTMLCSSStyleSheet& operator=(const nsHTMLCSSStyleSheet& aCopy) MOZ_DELETE;

protected:
  nsCOMPtr<nsIURI> mURL;
  nsIDocument*     mDocument;
};

#endif 
