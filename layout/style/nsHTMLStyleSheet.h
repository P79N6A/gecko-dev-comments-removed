











































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
  NS_IMETHOD SetOwningDocument(nsIDocument* aDocumemt);
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
  nsresult Reset(nsIURI* aURL);
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

    
    NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
  #ifdef DEBUG
    NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
  #endif

    nscolor             mColor;
  };


  class GenericTableRule;
  friend class GenericTableRule;
  class GenericTableRule: public nsIStyleRule {
  public:
    GenericTableRule() {}

    NS_DECL_ISUPPORTS

    
    NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
  #ifdef DEBUG
    NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
  #endif
  };

  
  class TableTHRule;
  friend class TableTHRule;
  class TableTHRule: public GenericTableRule {
  public:
    TableTHRule() {}

    NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
  };

  nsIURI*              mURL;
  nsIDocument*         mDocument;
  HTMLColorRule*       mLinkRule;
  HTMLColorRule*       mVisitedRule;
  HTMLColorRule*       mActiveRule;
  HTMLColorRule*       mDocumentColorRule;
  TableTHRule*         mTableTHRule;

  PLDHashTable         mMappedAttrTable;
};


nsresult
NS_NewHTMLStyleSheet(nsHTMLStyleSheet** aInstancePtrResult, nsIURI* aURL, 
                     nsIDocument* aDocument);

nsresult
NS_NewHTMLStyleSheet(nsHTMLStyleSheet** aInstancePtrResult);

#endif 
