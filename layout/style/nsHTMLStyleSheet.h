











































#ifndef nsHTMLStyleSheet_h_
#define nsHTMLStyleSheet_h_

#include "nsIStyleSheet.h"
#include "nsIStyleRuleProcessor.h"
#include "nsIStyleRule.h"
#include "pldhash.h"
#include "nsCOMPtr.h"
#include "nsColor.h"
class nsMappedAttributes;

class nsHTMLStyleSheet : public nsIStyleSheet, public nsIStyleRuleProcessor {
public:
  nsHTMLStyleSheet(void);
  nsresult Init();

  NS_DECL_ISUPPORTS

  
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
  virtual void SetOwningDocument(nsIDocument* aDocumemt);
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
                                   PRBool* aRulesChanged);

  nsresult Init(nsIURI* aURL, nsIDocument* aDocument);
  void Reset(nsIURI* aURL);
  nsresult SetLinkColor(nscolor aColor);
  nsresult SetActiveLinkColor(nscolor aColor);
  nsresult SetVisitedLinkColor(nscolor aColor);

  
  already_AddRefed<nsMappedAttributes>
    UniqueMappedAttributes(nsMappedAttributes* aMapped);
  void DropMappedAttributes(nsMappedAttributes* aMapped);


private: 
  
  nsHTMLStyleSheet(const nsHTMLStyleSheet& aCopy); 
  nsHTMLStyleSheet& operator=(const nsHTMLStyleSheet& aCopy); 

  ~nsHTMLStyleSheet();

  class HTMLColorRule;
  friend class HTMLColorRule;
  class HTMLColorRule : public nsIStyleRule {
  public:
    HTMLColorRule() {}

    NS_DECL_ISUPPORTS

    
    virtual void MapRuleInfoInto(nsRuleData* aRuleData);
  #ifdef DEBUG
    virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
  #endif

    nscolor             mColor;
  };


  class GenericTableRule;
  friend class GenericTableRule;
  class GenericTableRule: public nsIStyleRule {
  public:
    GenericTableRule() {}

    NS_DECL_ISUPPORTS

    
    virtual void MapRuleInfoInto(nsRuleData* aRuleData);
  #ifdef DEBUG
    virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
  #endif
  };

  
  class TableTHRule;
  friend class TableTHRule;
  class TableTHRule: public GenericTableRule {
  public:
    TableTHRule() {}

    virtual void MapRuleInfoInto(nsRuleData* aRuleData);
  };

  nsCOMPtr<nsIURI>        mURL;
  nsIDocument*            mDocument;
  nsRefPtr<HTMLColorRule> mLinkRule;
  nsRefPtr<HTMLColorRule> mVisitedRule;
  nsRefPtr<HTMLColorRule> mActiveRule;
  nsRefPtr<HTMLColorRule> mDocumentColorRule;
  nsRefPtr<TableTHRule>   mTableTHRule;

  PLDHashTable            mMappedAttrTable;
};


nsresult
NS_NewHTMLStyleSheet(nsHTMLStyleSheet** aInstancePtrResult, nsIURI* aURL, 
                     nsIDocument* aDocument);

nsresult
NS_NewHTMLStyleSheet(nsHTMLStyleSheet** aInstancePtrResult);

#endif 
