










#ifndef nsHTMLStyleSheet_h_
#define nsHTMLStyleSheet_h_

#include "nsAutoPtr.h"
#include "nsColor.h"
#include "nsCOMPtr.h"
#include "nsIStyleRule.h"
#include "nsIStyleRuleProcessor.h"
#include "pldhash.h"
#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "nsString.h"

class nsIDocument;
class nsMappedAttributes;

class nsHTMLStyleSheet final : public nsIStyleRuleProcessor
{
public:
  explicit nsHTMLStyleSheet(nsIDocument* aDocument);

  void SetOwningDocument(nsIDocument* aDocument);

  NS_DECL_ISUPPORTS

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) override;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) override;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) override;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) override;
#endif
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) override;
  virtual nsRestyleHint HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData) override;
  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) override;
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) override;
  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) override;
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE override;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE override;
  size_t DOMSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  void Reset();
  nsresult SetLinkColor(nscolor aColor);
  nsresult SetActiveLinkColor(nscolor aColor);
  nsresult SetVisitedLinkColor(nscolor aColor);

  
  already_AddRefed<nsMappedAttributes>
    UniqueMappedAttributes(nsMappedAttributes* aMapped);
  void DropMappedAttributes(nsMappedAttributes* aMapped);

  nsIStyleRule* LangRuleFor(const nsString& aLanguage);

private: 
  nsHTMLStyleSheet(const nsHTMLStyleSheet& aCopy) = delete;
  nsHTMLStyleSheet& operator=(const nsHTMLStyleSheet& aCopy) = delete;

  ~nsHTMLStyleSheet();

  class HTMLColorRule;
  friend class HTMLColorRule;
  class HTMLColorRule final : public nsIStyleRule {
  private:
    ~HTMLColorRule() {}
  public:
    HTMLColorRule() {}

    NS_DECL_ISUPPORTS

    
    virtual void MapRuleInfoInto(nsRuleData* aRuleData) override;
  #ifdef DEBUG
    virtual void List(FILE* out = stdout, int32_t aIndent = 0) const override;
  #endif

    nscolor             mColor;
  };

  
  nsresult ImplLinkColorSetter(nsRefPtr<HTMLColorRule>& aRule, nscolor aColor);

  class GenericTableRule;
  friend class GenericTableRule;
  class GenericTableRule : public nsIStyleRule {
  protected:
    virtual ~GenericTableRule() {}
  public:
    GenericTableRule() {}

    NS_DECL_ISUPPORTS

    
    virtual void MapRuleInfoInto(nsRuleData* aRuleData) override = 0;
  #ifdef DEBUG
    virtual void List(FILE* out = stdout, int32_t aIndent = 0) const override;
  #endif
  };

  
  class TableTHRule;
  friend class TableTHRule;
  class TableTHRule final : public GenericTableRule {
  public:
    TableTHRule() {}

    virtual void MapRuleInfoInto(nsRuleData* aRuleData) override;
  };

  
  class TableQuirkColorRule final : public GenericTableRule {
  public:
    TableQuirkColorRule() {}

    virtual void MapRuleInfoInto(nsRuleData* aRuleData) override;
  };

public: 

  
  
  class LangRule final : public nsIStyleRule {
  private:
    ~LangRule() {}
  public:
    explicit LangRule(const nsSubstring& aLang) : mLang(aLang) {}

    NS_DECL_ISUPPORTS

    
    virtual void MapRuleInfoInto(nsRuleData* aRuleData) override;
  #ifdef DEBUG
    virtual void List(FILE* out = stdout, int32_t aIndent = 0) const override;
  #endif

    nsString mLang;
  };

private:
  nsIDocument*            mDocument;
  nsRefPtr<HTMLColorRule> mLinkRule;
  nsRefPtr<HTMLColorRule> mVisitedRule;
  nsRefPtr<HTMLColorRule> mActiveRule;
  nsRefPtr<TableQuirkColorRule> mTableQuirkColorRule;
  nsRefPtr<TableTHRule>   mTableTHRule;

  PLDHashTable            mMappedAttrTable;
  PLDHashTable            mLangRuleTable;
};

#endif 
