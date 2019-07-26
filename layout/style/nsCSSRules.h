







#ifndef nsCSSRules_h_
#define nsCSSRules_h_

#include "mozilla/Attributes.h"

#include "mozilla/css/GroupRule.h"
#include "mozilla/Preferences.h"
#include "nsIDOMCSSConditionRule.h"
#include "nsIDOMCSSFontFaceRule.h"
#include "nsIDOMCSSGroupingRule.h"
#include "nsIDOMCSSMediaRule.h"
#include "nsIDOMCSSMozDocumentRule.h"
#include "nsIDOMCSSSupportsRule.h"
#include "nsIDOMMozCSSKeyframeRule.h"
#include "nsIDOMMozCSSKeyframesRule.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsAutoPtr.h"
#include "nsCSSProperty.h"
#include "nsCSSValue.h"
#include "nsIDOMCSSCharsetRule.h"
#include "nsTArray.h"
#include "nsDOMCSSDeclaration.h"
#include "Declaration.h"
#include "nsIDOMCSSPageRule.h"
#include "StyleRule.h"

class nsMediaList;

namespace mozilla {

class ErrorResult;

namespace css {

class MediaRule MOZ_FINAL : public GroupRule,
                            public nsIDOMCSSMediaRule
{
public:
  MediaRule();
private:
  MediaRule(const MediaRule& aCopy);
  ~MediaRule();
public:

  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

  
  virtual void SetStyleSheet(nsCSSStyleSheet* aSheet); 
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
  
  virtual NS_MUST_OVERRIDE size_t
    SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

protected:
  void AppendConditionText(nsAString& aOutput);

  nsRefPtr<nsMediaList> mMedia;
};

class DocumentRule MOZ_FINAL : public GroupRule,
                               public nsIDOMCSSMozDocumentRule
{
public:
  DocumentRule();
private:
  DocumentRule(const DocumentRule& aCopy);
  ~DocumentRule();
public:

  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
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

  virtual NS_MUST_OVERRIDE size_t
    SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

protected:
  void AppendConditionText(nsAString& aOutput);

  nsAutoPtr<URL> mURLs; 
};

} 
} 


class nsCSSFontFaceRule;
class nsCSSFontFaceStyleDecl : public nsICSSDeclaration
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

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap);

protected:
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
  nsCSSFontFaceRule() {}

  nsCSSFontFaceRule(const nsCSSFontFaceRule& aCopy)
    
    : mozilla::css::Rule(aCopy), mDecl(aCopy.mDecl) {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsCSSFontFaceRule,
                                                         mozilla::css::Rule)

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

  
  DECL_STYLE_RULE_INHERIT

  virtual int32_t GetType() const;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSFONTFACERULE

  void SetDesc(nsCSSFontDesc aDescID, nsCSSValue const & aValue);
  void GetDesc(nsCSSFontDesc aDescID, nsCSSValue & aValue);

  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE;

protected:
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

namespace mozilla {
namespace css {

class CharsetRule MOZ_FINAL : public Rule,
                              public nsIDOMCSSCharsetRule
{
public:
  CharsetRule(const nsAString& aEncoding);
private:
  
  CharsetRule(const CharsetRule& aCopy);
  ~CharsetRule() {}

public:
  NS_DECL_ISUPPORTS

  DECL_STYLE_RULE_INHERIT

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

  
  virtual int32_t GetType() const;
  virtual already_AddRefed<Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_IMETHOD GetEncoding(nsAString& aEncoding);
  NS_IMETHOD SetEncoding(const nsAString& aEncoding);

  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

private:
  nsString  mEncoding;
};

} 
} 

class nsCSSKeyframeRule;

class nsCSSKeyframeStyleDeclaration MOZ_FINAL : public nsDOMCSSDeclaration
{
public:
  nsCSSKeyframeStyleDeclaration(nsCSSKeyframeRule *aRule);
  virtual ~nsCSSKeyframeStyleDeclaration();

  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent) MOZ_OVERRIDE;
  void DropReference() { mRule = nullptr; }
  virtual mozilla::css::Declaration* GetCSSDeclaration(bool aAllocate) MOZ_OVERRIDE;
  virtual nsresult SetCSSDeclaration(mozilla::css::Declaration* aDecl) MOZ_OVERRIDE;
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv) MOZ_OVERRIDE;
  virtual nsIDocument* DocToUpdate() MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsCSSKeyframeStyleDeclaration,
                                                         nsICSSDeclaration)

  virtual nsINode* GetParentObject();

protected:
  
  
  nsCSSKeyframeRule *mRule;
};

class nsCSSKeyframeRule MOZ_FINAL : public mozilla::css::Rule,
                                    public nsIDOMMozCSSKeyframeRule
{
public:
  
  nsCSSKeyframeRule(InfallibleTArray<float>& aKeys,
                    nsAutoPtr<mozilla::css::Declaration> aDeclaration)
    : mDeclaration(aDeclaration)
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
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

  
  DECL_STYLE_RULE_INHERIT
  virtual int32_t GetType() const;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMMOZCSSKEYFRAMERULE

  const nsTArray<float>& GetKeys() const     { return mKeys; }
  mozilla::css::Declaration* Declaration()   { return mDeclaration; }

  void ChangeDeclaration(mozilla::css::Declaration* aDeclaration);

  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE;

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
  nsCSSKeyframesRule(const nsSubstring& aName)
    : mName(aName)
  {
  }
private:
  nsCSSKeyframesRule(const nsCSSKeyframesRule& aCopy);
  ~nsCSSKeyframesRule();
public:
  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
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
                                    nsMediaQueryResultCacheKey& aKey);

  const nsString& GetName() { return mName; }

  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE;

private:
  uint32_t FindRuleIndexForKey(const nsAString& aKey);

  nsString                                   mName;
};

class nsCSSPageRule;

class nsCSSPageStyleDeclaration MOZ_FINAL : public nsDOMCSSDeclaration
{
public:
  nsCSSPageStyleDeclaration(nsCSSPageRule *aRule);
  virtual ~nsCSSPageStyleDeclaration();

  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent) MOZ_OVERRIDE;
  void DropReference() { mRule = nullptr; }
  virtual mozilla::css::Declaration* GetCSSDeclaration(bool aAllocate) MOZ_OVERRIDE;
  virtual nsresult SetCSSDeclaration(mozilla::css::Declaration* aDecl) MOZ_OVERRIDE;
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv) MOZ_OVERRIDE;
  virtual nsIDocument* DocToUpdate() MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsCSSPageStyleDeclaration,
                                                         nsICSSDeclaration)

  virtual nsINode *GetParentObject();

protected:
  
  
  nsCSSPageRule *mRule;
};

class nsCSSPageRule MOZ_FINAL : public mozilla::css::Rule,
                                public nsIDOMCSSPageRule
{
public:
  
  nsCSSPageRule(nsAutoPtr<mozilla::css::Declaration> aDeclaration)
    : mDeclaration(aDeclaration),
      mImportantRule(nullptr)
  {
  }
private:
  nsCSSPageRule(const nsCSSPageRule& aCopy);
  ~nsCSSPageRule();
public:
  NS_DECL_ISUPPORTS

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

  
  DECL_STYLE_RULE_INHERIT
  virtual int32_t GetType() const;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSPAGERULE

  mozilla::css::Declaration* Declaration()   { return mDeclaration; }

  void ChangeDeclaration(mozilla::css::Declaration* aDeclaration);

  mozilla::css::ImportantRule* GetImportantRule();

  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
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
  CSSSupportsRule(bool aConditionMet, const nsString& aCondition);
  CSSSupportsRule(const CSSSupportsRule& aCopy);

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
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

  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  static bool PrefEnabled()
  {
    return Preferences::GetBool("layout.css.supports-rule.enabled");
  }

protected:
  bool mUseGroup;
  nsString mCondition;
};

} 

#endif 
