







#ifndef nsCSSRules_h_
#define nsCSSRules_h_

#include "mozilla/Attributes.h"
#include "mozilla/Move.h"

#include "mozilla/MemoryReporting.h"
#include "mozilla/css/GroupRule.h"
#include "nsIDOMCSSConditionRule.h"
#include "nsIDOMCSSCounterStyleRule.h"
#include "nsIDOMCSSFontFaceRule.h"
#include "nsIDOMCSSFontFeatureValuesRule.h"
#include "nsIDOMCSSGroupingRule.h"
#include "nsIDOMCSSMediaRule.h"
#include "nsIDOMCSSMozDocumentRule.h"
#include "nsIDOMCSSSupportsRule.h"
#include "nsIDOMMozCSSKeyframeRule.h"
#include "nsIDOMMozCSSKeyframesRule.h"
#include "nsAutoPtr.h"
#include "nsCSSProperty.h"
#include "nsCSSValue.h"
#include "nsIDOMCSSCharsetRule.h"
#include "nsTArray.h"
#include "nsDOMCSSDeclaration.h"
#include "Declaration.h"
#include "nsIDOMCSSPageRule.h"
#include "StyleRule.h"
#include "gfxFontFeatures.h"

class nsMediaList;

namespace mozilla {

class ErrorResult;

namespace css {

class MediaRule MOZ_FINAL : public GroupRule,
                            public nsIDOMCSSMediaRule
{
public:
  MediaRule(uint32_t aLineNumber, uint32_t aColumnNumber);
private:
  MediaRule(const MediaRule& aCopy);
  ~MediaRule();
public:

  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  virtual void SetStyleSheet(mozilla::CSSStyleSheet* aSheet); 
  virtual int32_t GetType() const;
  virtual already_AddRefed<Rule> Clone() const;
  virtual nsIDOMCSSRule* GetDOMRule()
  {
    return this;
  }
  virtual nsIDOMCSSRule* GetExistingDOMRule()
  {
    return this;
  }

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSGROUPINGRULE

  
  NS_DECL_NSIDOMCSSCONDITIONRULE

  
  NS_DECL_NSIDOMCSSMEDIARULE

  
  virtual bool UseForPresentation(nsPresContext* aPresContext,
                                    nsMediaQueryResultCacheKey& aKey);

  
  nsresult SetMedia(nsMediaList* aMedia);
  
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE;

protected:
  void AppendConditionText(nsAString& aOutput);

  nsRefPtr<nsMediaList> mMedia;
};

class DocumentRule MOZ_FINAL : public GroupRule,
                               public nsIDOMCSSMozDocumentRule
{
public:
  DocumentRule(uint32_t aLineNumber, uint32_t aColumnNumber);
private:
  DocumentRule(const DocumentRule& aCopy);
  ~DocumentRule();
public:

  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  virtual int32_t GetType() const;
  virtual already_AddRefed<Rule> Clone() const;
  virtual nsIDOMCSSRule* GetDOMRule()
  {
    return this;
  }
  virtual nsIDOMCSSRule* GetExistingDOMRule()
  {
    return this;
  }

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSGROUPINGRULE

  
  NS_DECL_NSIDOMCSSCONDITIONRULE

  
  NS_DECL_NSIDOMCSSMOZDOCUMENTRULE

  
  virtual bool UseForPresentation(nsPresContext* aPresContext,
                                    nsMediaQueryResultCacheKey& aKey);

  enum Function {
    eURL,
    eURLPrefix,
    eDomain,
    eRegExp
  };

  struct URL {
    Function func;
    nsCString url;
    URL *next;

    URL() : next(nullptr) {}
    URL(const URL& aOther)
      : func(aOther.func)
      , url(aOther.url)
      , next(aOther.next ? new URL(*aOther.next) : nullptr)
    {
    }
    ~URL();
  };

  void SetURLs(URL *aURLs) { mURLs = aURLs; }

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE;

protected:
  void AppendConditionText(nsAString& aOutput);

  nsAutoPtr<URL> mURLs; 
};

} 
} 


class nsCSSFontFaceRule;
class nsCSSFontFaceStyleDecl MOZ_FINAL : public nsICSSDeclaration
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMCSSSTYLEDECLARATION_HELPER
  NS_DECL_NSICSSDECLARATION
  virtual already_AddRefed<mozilla::dom::CSSValue>
  GetPropertyCSSValue(const nsAString& aProp, mozilla::ErrorResult& aRv)
    MOZ_OVERRIDE;
  using nsICSSDeclaration::GetPropertyCSSValue;

  nsCSSFontFaceStyleDecl()
  {
    SetIsDOMBinding();
  }

  virtual nsINode *GetParentObject() MOZ_OVERRIDE;
  virtual void IndexedGetter(uint32_t aIndex, bool& aFound, nsAString& aPropName) MOZ_OVERRIDE;

  nsresult GetPropertyValue(nsCSSFontDesc aFontDescID,
                            nsAString & aResult) const;

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

protected:
  ~nsCSSFontFaceStyleDecl() {}

  friend class nsCSSFontFaceRule;
#define CSS_FONT_DESC(name_, method_) nsCSSValue m##method_;
#include "nsCSSFontDescList.h"
#undef CSS_FONT_DESC

  static nsCSSValue nsCSSFontFaceStyleDecl::* const Fields[];
  inline nsCSSFontFaceRule* ContainingRule();
  inline const nsCSSFontFaceRule* ContainingRule() const;

private:
  
  
  
  void* operator new(size_t size) CPP_THROW_NEW;
};

class nsCSSFontFaceRule MOZ_FINAL : public mozilla::css::Rule,
                                    public nsIDOMCSSFontFaceRule
{
public:
  nsCSSFontFaceRule(uint32_t aLineNumber, uint32_t aColumnNumber)
    : mozilla::css::Rule(aLineNumber, aColumnNumber) {}

  nsCSSFontFaceRule(const nsCSSFontFaceRule& aCopy)
    
    : mozilla::css::Rule(aCopy), mDecl(aCopy.mDecl) {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsCSSFontFaceRule,
                                                         mozilla::css::Rule)

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  DECL_STYLE_RULE_INHERIT

  virtual int32_t GetType() const MOZ_OVERRIDE;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSFONTFACERULE

  void SetDesc(nsCSSFontDesc aDescID, nsCSSValue const & aValue);
  void GetDesc(nsCSSFontDesc aDescID, nsCSSValue & aValue);

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

protected:
  ~nsCSSFontFaceRule() {}

  friend class nsCSSFontFaceStyleDecl;
  nsCSSFontFaceStyleDecl mDecl;
};



struct nsFontFaceRuleContainer {
  nsRefPtr<nsCSSFontFaceRule> mRule;
  uint8_t mSheetType;
};

inline nsCSSFontFaceRule*
nsCSSFontFaceStyleDecl::ContainingRule()
{
  return reinterpret_cast<nsCSSFontFaceRule*>
    (reinterpret_cast<char*>(this) - offsetof(nsCSSFontFaceRule, mDecl));
}

inline const nsCSSFontFaceRule*
nsCSSFontFaceStyleDecl::ContainingRule() const
{
  return reinterpret_cast<const nsCSSFontFaceRule*>
    (reinterpret_cast<const char*>(this) - offsetof(nsCSSFontFaceRule, mDecl));
}

class nsCSSFontFeatureValuesRule MOZ_FINAL :
                                       public mozilla::css::Rule,
                                       public nsIDOMCSSFontFeatureValuesRule
{
public:
  nsCSSFontFeatureValuesRule(uint32_t aLineNumber, uint32_t aColumnNumber)
    : mozilla::css::Rule(aLineNumber, aColumnNumber) {}

  nsCSSFontFeatureValuesRule(const nsCSSFontFeatureValuesRule& aCopy)
    
    : mozilla::css::Rule(aCopy),
      mFamilyList(aCopy.mFamilyList),
      mFeatureValues(aCopy.mFeatureValues) {}

  NS_DECL_ISUPPORTS

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  DECL_STYLE_RULE_INHERIT

  virtual int32_t GetType() const MOZ_OVERRIDE;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSFONTFEATUREVALUESRULE

  const mozilla::FontFamilyList& GetFamilyList() { return mFamilyList; }
  void SetFamilyList(const mozilla::FontFamilyList& aFamilyList);

  void AddValueList(int32_t aVariantAlternate,
                    nsTArray<gfxFontFeatureValueSet::ValueList>& aValueList);

  const nsTArray<gfxFontFeatureValueSet::FeatureValues>& GetFeatureValues()
  {
    return mFeatureValues;
  }

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

protected:
  ~nsCSSFontFeatureValuesRule() {}

  mozilla::FontFamilyList mFamilyList;
  nsTArray<gfxFontFeatureValueSet::FeatureValues> mFeatureValues;
};

namespace mozilla {
namespace css {

class CharsetRule MOZ_FINAL : public Rule,
                              public nsIDOMCSSCharsetRule
{
public:
  CharsetRule(const nsAString& aEncoding,
              uint32_t aLineNumber, uint32_t aColumnNumber);
private:
  
  CharsetRule(const CharsetRule& aCopy);
  ~CharsetRule() {}

public:
  NS_DECL_ISUPPORTS

  DECL_STYLE_RULE_INHERIT

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  virtual int32_t GetType() const;
  virtual already_AddRefed<Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_IMETHOD GetEncoding(nsAString& aEncoding) MOZ_OVERRIDE;
  NS_IMETHOD SetEncoding(const nsAString& aEncoding) MOZ_OVERRIDE;

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  nsString  mEncoding;
};

} 
} 

class nsCSSKeyframeRule;

class nsCSSKeyframeStyleDeclaration MOZ_FINAL : public nsDOMCSSDeclaration
{
public:
  explicit nsCSSKeyframeStyleDeclaration(nsCSSKeyframeRule *aRule);

  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent) MOZ_OVERRIDE;
  void DropReference() { mRule = nullptr; }
  virtual mozilla::css::Declaration* GetCSSDeclaration(bool aAllocate) MOZ_OVERRIDE;
  virtual nsresult SetCSSDeclaration(mozilla::css::Declaration* aDecl) MOZ_OVERRIDE;
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv) MOZ_OVERRIDE;
  virtual nsIDocument* DocToUpdate() MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsCSSKeyframeStyleDeclaration,
                                                         nsICSSDeclaration)

  virtual nsINode* GetParentObject() MOZ_OVERRIDE;

protected:
  virtual ~nsCSSKeyframeStyleDeclaration();

  
  
  nsCSSKeyframeRule *mRule;
};

class nsCSSKeyframeRule MOZ_FINAL : public mozilla::css::Rule,
                                    public nsIDOMMozCSSKeyframeRule
{
public:
  
  nsCSSKeyframeRule(InfallibleTArray<float>& aKeys,
                    nsAutoPtr<mozilla::css::Declaration>&& aDeclaration,
                    uint32_t aLineNumber, uint32_t aColumnNumber)
    : mozilla::css::Rule(aLineNumber, aColumnNumber)
    , mDeclaration(mozilla::Move(aDeclaration))
  {
    mKeys.SwapElements(aKeys);
  }
private:
  nsCSSKeyframeRule(const nsCSSKeyframeRule& aCopy);
  ~nsCSSKeyframeRule();
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsCSSKeyframeRule, nsIStyleRule)

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  DECL_STYLE_RULE_INHERIT
  virtual int32_t GetType() const MOZ_OVERRIDE;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMMOZCSSKEYFRAMERULE

  const nsTArray<float>& GetKeys() const     { return mKeys; }
  mozilla::css::Declaration* Declaration()   { return mDeclaration; }

  void ChangeDeclaration(mozilla::css::Declaration* aDeclaration);

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

  void DoGetKeyText(nsAString &aKeyText) const;

private:
  nsTArray<float>                            mKeys;
  nsAutoPtr<mozilla::css::Declaration>       mDeclaration;
  
  nsRefPtr<nsCSSKeyframeStyleDeclaration>    mDOMDeclaration;
};

class nsCSSKeyframesRule MOZ_FINAL : public mozilla::css::GroupRule,
                                     public nsIDOMMozCSSKeyframesRule
{
public:
  nsCSSKeyframesRule(const nsSubstring& aName,
                     uint32_t aLineNumber, uint32_t aColumnNumber)
    : mozilla::css::GroupRule(aLineNumber, aColumnNumber)
    , mName(aName)
  {
  }
private:
  nsCSSKeyframesRule(const nsCSSKeyframesRule& aCopy);
  ~nsCSSKeyframesRule();
public:
  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  virtual int32_t GetType() const;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;
  virtual nsIDOMCSSRule* GetDOMRule()
  {
    return this;
  }
  virtual nsIDOMCSSRule* GetExistingDOMRule()
  {
    return this;
  }

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMMOZCSSKEYFRAMESRULE

  
  virtual bool UseForPresentation(nsPresContext* aPresContext,
                                    nsMediaQueryResultCacheKey& aKey) MOZ_OVERRIDE;

  const nsString& GetName() { return mName; }

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

private:
  uint32_t FindRuleIndexForKey(const nsAString& aKey);

  nsString                                   mName;
};

class nsCSSPageRule;

class nsCSSPageStyleDeclaration MOZ_FINAL : public nsDOMCSSDeclaration
{
public:
  explicit nsCSSPageStyleDeclaration(nsCSSPageRule *aRule);

  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent) MOZ_OVERRIDE;
  void DropReference() { mRule = nullptr; }
  virtual mozilla::css::Declaration* GetCSSDeclaration(bool aAllocate) MOZ_OVERRIDE;
  virtual nsresult SetCSSDeclaration(mozilla::css::Declaration* aDecl) MOZ_OVERRIDE;
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv) MOZ_OVERRIDE;
  virtual nsIDocument* DocToUpdate() MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsCSSPageStyleDeclaration,
                                                         nsICSSDeclaration)

  virtual nsINode *GetParentObject() MOZ_OVERRIDE;

protected:
  virtual ~nsCSSPageStyleDeclaration();

  
  
  nsCSSPageRule *mRule;
};

class nsCSSPageRule MOZ_FINAL : public mozilla::css::Rule,
                                public nsIDOMCSSPageRule
{
public:
  
  nsCSSPageRule(nsAutoPtr<mozilla::css::Declaration>&& aDeclaration,
                uint32_t aLineNumber, uint32_t aColumnNumber)
    : mozilla::css::Rule(aLineNumber, aColumnNumber)
    , mDeclaration(mozilla::Move(aDeclaration))
    , mImportantRule(nullptr)
  {
  }
private:
  nsCSSPageRule(const nsCSSPageRule& aCopy);
  ~nsCSSPageRule();
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsCSSPageRule, nsIDOMCSSPageRule)

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  DECL_STYLE_RULE_INHERIT
  virtual int32_t GetType() const MOZ_OVERRIDE;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSPAGERULE

  mozilla::css::Declaration* Declaration()   { return mDeclaration; }

  void ChangeDeclaration(mozilla::css::Declaration* aDeclaration);

  mozilla::css::ImportantRule* GetImportantRule();

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
private:
  nsAutoPtr<mozilla::css::Declaration>    mDeclaration;
  
  nsRefPtr<nsCSSPageStyleDeclaration>     mDOMDeclaration;
  nsRefPtr<mozilla::css::ImportantRule>   mImportantRule;
};

namespace mozilla {

class CSSSupportsRule : public css::GroupRule,
                        public nsIDOMCSSSupportsRule
{
public:
  CSSSupportsRule(bool aConditionMet, const nsString& aCondition,
                  uint32_t aLineNumber, uint32_t aColumnNumber);
  CSSSupportsRule(const CSSSupportsRule& aCopy);

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  virtual int32_t GetType() const;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;
  virtual bool UseForPresentation(nsPresContext* aPresContext,
                                  nsMediaQueryResultCacheKey& aKey);
  virtual nsIDOMCSSRule* GetDOMRule()
  {
    return this;
  }
  virtual nsIDOMCSSRule* GetExistingDOMRule()
  {
    return this;
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSGROUPINGRULE

  
  NS_DECL_NSIDOMCSSCONDITIONRULE

  
  NS_DECL_NSIDOMCSSSUPPORTSRULE

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

protected:
  virtual ~CSSSupportsRule();

  bool mUseGroup;
  nsString mCondition;
};

} 

class nsCSSCounterStyleRule MOZ_FINAL : public mozilla::css::Rule,
                                        public nsIDOMCSSCounterStyleRule
{
public:
  explicit nsCSSCounterStyleRule(const nsAString& aName,
                                 uint32_t aLineNumber, uint32_t aColumnNumber)
    : mozilla::css::Rule(aLineNumber, aColumnNumber)
    , mName(aName)
    , mGeneration(0)
  {
  }

private:
  nsCSSCounterStyleRule(const nsCSSCounterStyleRule& aCopy);
  ~nsCSSCounterStyleRule();

public:
  NS_DECL_ISUPPORTS

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  DECL_STYLE_RULE_INHERIT
  virtual int32_t GetType() const MOZ_OVERRIDE;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSCOUNTERSTYLERULE

  
  
  static bool CheckDescValue(int32_t aSystem,
                             nsCSSCounterDesc aDescID,
                             const nsCSSValue& aValue);

  const nsString& GetName() const { return mName; }

  uint32_t GetGeneration() const { return mGeneration; }

  int32_t GetSystem() const;
  const nsCSSValue& GetSystemArgument() const;

  const nsCSSValue& GetDesc(nsCSSCounterDesc aDescID) const
  {
    NS_ABORT_IF_FALSE(aDescID >= 0 && aDescID < eCSSCounterDesc_COUNT,
                      "descriptor ID out of range");
    return mValues[aDescID];
  }

  void SetDesc(nsCSSCounterDesc aDescID, const nsCSSValue& aValue);

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

private:
  typedef NS_STDCALL_FUNCPROTO(nsresult, Getter, nsCSSCounterStyleRule,
                               GetSymbols, (nsAString&));
  static const Getter kGetters[];

  nsresult GetDescriptor(nsCSSCounterDesc aDescID, nsAString& aValue);
  nsresult SetDescriptor(nsCSSCounterDesc aDescID, const nsAString& aValue);

  nsString   mName;
  nsCSSValue mValues[eCSSCounterDesc_COUNT];
  uint32_t   mGeneration;
};

#endif 
