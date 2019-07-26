










#ifndef nsHTMLStyleSheet_h_
#define nsHTMLStyleSheet_h_

#include "nsAutoPtr.h"
#include "nsColor.h"
#include "nsCOMPtr.h"
#include "nsIStyleRule.h"
#include "nsIStyleRuleProcessor.h"
#include "nsIStyleSheet.h"
#include "pldhash.h"
#include "mozilla/Attributes.h"

class nsMappedAttributes;

class nsHTMLStyleSheet MOZ_FINAL : public nsIStyleSheet,
                                   public nsIStyleRuleProcessor
{
public:
  nsHTMLStyleSheet(nsIURI* aURL, nsIDocument* aDocument);

  NS_DECL_ISUPPORTS

  
  virtual nsIURI* GetSheetURI() const;
  virtual nsIURI* GetBaseURI() const MOZ_OVERRIDE;
  virtual void GetTitle(nsString& aTitle) const MOZ_OVERRIDE;
  virtual void GetType(nsString& aType) const MOZ_OVERRIDE;
  virtual bool HasRules() const MOZ_OVERRIDE;
  virtual bool IsApplicable() const MOZ_OVERRIDE;
  virtual void SetEnabled(bool aEnabled) MOZ_OVERRIDE;
  virtual bool IsComplete() const MOZ_OVERRIDE;
  virtual void SetComplete() MOZ_OVERRIDE;
  virtual nsIStyleSheet* GetParentSheet() const MOZ_OVERRIDE;  
  virtual nsIDocument* GetOwningDocument() const MOZ_OVERRIDE;
  virtual void SetOwningDocument(nsIDocument* aDocumemt) MOZ_OVERRIDE;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) MOZ_OVERRIDE;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) MOZ_OVERRIDE;
#endif
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) MOZ_OVERRIDE;
  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  size_t DOMSizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  void Reset(nsIURI* aURL);
  nsresult SetLinkColor(nscolor aColor);
  nsresult SetActiveLinkColor(nscolor aColor);
  nsresult SetVisitedLinkColor(nscolor aColor);

  
  already_AddRefed<nsMappedAttributes>
    UniqueMappedAttributes(nsMappedAttributes* aMapped);
  void DropMappedAttributes(nsMappedAttributes* aMapped);

private: 
  nsHTMLStyleSheet(const nsHTMLStyleSheet& aCopy) MOZ_DELETE;
  nsHTMLStyleSheet& operator=(const nsHTMLStyleSheet& aCopy) MOZ_DELETE;

  ~nsHTMLStyleSheet();

  class HTMLColorRule;
  friend class HTMLColorRule;
  class HTMLColorRule MOZ_FINAL : public nsIStyleRule {
  public:
    HTMLColorRule() {}

    NS_DECL_ISUPPORTS

    
    virtual void MapRuleInfoInto(nsRuleData* aRuleData);
  #ifdef DEBUG
    virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
  #endif

    nscolor             mColor;
  };

  
  nsresult ImplLinkColorSetter(nsRefPtr<HTMLColorRule>& aRule, nscolor aColor);

  class GenericTableRule;
  friend class GenericTableRule;
  class GenericTableRule : public nsIStyleRule {
  public:
    GenericTableRule() {}
    virtual ~GenericTableRule() {}

    NS_DECL_ISUPPORTS

    
    virtual void MapRuleInfoInto(nsRuleData* aRuleData) = 0;
  #ifdef DEBUG
    virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
  #endif
  };

  
  class TableTHRule;
  friend class TableTHRule;
  class TableTHRule MOZ_FINAL : public GenericTableRule {
  public:
    TableTHRule() {}

    virtual void MapRuleInfoInto(nsRuleData* aRuleData);
  };

  
  class TableQuirkColorRule MOZ_FINAL : public GenericTableRule {
  public:
    TableQuirkColorRule() {}

    virtual void MapRuleInfoInto(nsRuleData* aRuleData);
  };

  nsCOMPtr<nsIURI>        mURL;
  nsIDocument*            mDocument;
  nsRefPtr<HTMLColorRule> mLinkRule;
  nsRefPtr<HTMLColorRule> mVisitedRule;
  nsRefPtr<HTMLColorRule> mActiveRule;
  nsRefPtr<TableQuirkColorRule> mTableQuirkColorRule;
  nsRefPtr<TableTHRule>   mTableTHRule;

  PLDHashTable            mMappedAttrTable;
};

#endif 
