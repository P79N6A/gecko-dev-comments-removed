









































#ifndef nsCSSRules_h_
#define nsCSSRules_h_

#include "mozilla/css/GroupRule.h"
#include "nsIDOMCSSMediaRule.h"
#include "nsIDOMCSSMozDocumentRule.h"
#include "nsIDOMCSSFontFaceRule.h"
#ifdef MOZ_CSS_ANIMATIONS
#include "nsIDOMMozCSSKeyframeRule.h"
#include "nsIDOMMozCSSKeyframesRule.h"
#endif
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsICSSRuleList.h"
#include "nsAutoPtr.h"
#include "nsCSSProperty.h"
#include "nsCSSValue.h"
#include "nsIDOMCSSCharsetRule.h"
#include "nsTArray.h"
#include "nsDOMCSSDeclaration.h"
#include "Declaration.h"

namespace mozilla {
namespace css {
class StyleRule;
}
}

class nsMediaList;

namespace mozilla {
namespace css {

class NS_FINAL_CLASS MediaRule : public GroupRule,
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
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual void SetStyleSheet(nsCSSStyleSheet* aSheet); 
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<Rule> Clone() const;
  virtual nsIDOMCSSRule* GetDOMRule()
  {
    return this;
  }

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSMEDIARULE

  
  virtual PRBool UseForPresentation(nsPresContext* aPresContext,
                                    nsMediaQueryResultCacheKey& aKey);

  
  nsresult SetMedia(nsMediaList* aMedia);
  
protected:
  nsRefPtr<nsMediaList> mMedia;
};

class NS_FINAL_CLASS DocumentRule : public GroupRule,
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
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<Rule> Clone() const;
  virtual nsIDOMCSSRule* GetDOMRule()
  {
    return this;
  }

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSMOZDOCUMENTRULE

  
  virtual PRBool UseForPresentation(nsPresContext* aPresContext,
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

    URL() : next(nsnull) {}
    URL(const URL& aOther)
      : func(aOther.func)
      , url(aOther.url)
      , next(aOther.next ? new URL(*aOther.next) : nsnull)
    {
    }
    ~URL();
  };

  void SetURLs(URL *aURLs) { mURLs = aURLs; }

protected:
  nsAutoPtr<URL> mURLs; 
};

} 
} 


class nsCSSFontFaceRule;
class nsCSSFontFaceStyleDecl : public nsIDOMCSSStyleDeclaration
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMCSSSTYLEDECLARATION

  nsresult GetPropertyValue(nsCSSFontDesc aFontDescID,
                            nsAString & aResult NS_OUTPARAM) const;

protected:
  friend class nsCSSFontFaceRule;
  nsCSSValue mFamily;
  nsCSSValue mStyle;
  nsCSSValue mWeight;
  nsCSSValue mStretch;
  nsCSSValue mSrc;
  nsCSSValue mUnicodeRange;
  nsCSSValue mFontFeatureSettings;
  nsCSSValue mFontLanguageOverride;

  static nsCSSValue nsCSSFontFaceStyleDecl::* const Fields[];  
  inline nsCSSFontFaceRule* ContainingRule();
  inline const nsCSSFontFaceRule* ContainingRule() const;

private:
  
  
  
  void* operator new(size_t size) CPP_THROW_NEW;
};

class NS_FINAL_CLASS nsCSSFontFaceRule : public mozilla::css::Rule,
                                         public nsIDOMCSSFontFaceRule
{
public:
  nsCSSFontFaceRule() {}

  nsCSSFontFaceRule(const nsCSSFontFaceRule& aCopy)
    
    : mozilla::css::Rule(aCopy), mDecl(aCopy.mDecl) {}

  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  DECL_STYLE_RULE_INHERIT

  virtual PRInt32 GetType() const;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSFONTFACERULE

  void SetDesc(nsCSSFontDesc aDescID, nsCSSValue const & aValue);
  void GetDesc(nsCSSFontDesc aDescID, nsCSSValue & aValue);

protected:
  friend class nsCSSFontFaceStyleDecl;
  nsCSSFontFaceStyleDecl mDecl;
};



struct nsFontFaceRuleContainer {
  nsRefPtr<nsCSSFontFaceRule> mRule;
  PRUint8 mSheetType;
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

class NS_FINAL_CLASS CharsetRule : public Rule,
                                   public nsIDOMCSSCharsetRule
{
public:
  CharsetRule(const nsAString& aEncoding);
private:
  
  CharsetRule(const CharsetRule& aCopy);
  ~CharsetRule() {}

public:
  NS_DECL_ISUPPORTS_INHERITED

  DECL_STYLE_RULE_INHERIT

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_IMETHOD GetEncoding(nsAString& aEncoding);
  NS_IMETHOD SetEncoding(const nsAString& aEncoding);

private:
  nsString  mEncoding;
};

} 
} 

#ifdef MOZ_CSS_ANIMATIONS
class nsCSSKeyframeRule;

class NS_FINAL_CLASS nsCSSKeyframeStyleDeclaration
                         : public nsDOMCSSDeclaration
{
public:
  nsCSSKeyframeStyleDeclaration(nsCSSKeyframeRule *aRule);
  virtual ~nsCSSKeyframeStyleDeclaration();

  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent);
  void DropReference() { mRule = nsnull; }
  virtual mozilla::css::Declaration* GetCSSDeclaration(PRBool aAllocate);
  virtual nsresult SetCSSDeclaration(mozilla::css::Declaration* aDecl);
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv);
  virtual nsIDocument* DocToUpdate();

  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  virtual nsINode *GetParentObject()
  {
    return nsnull;
  }

protected:
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

  
  
  nsCSSKeyframeRule *mRule;
};

class NS_FINAL_CLASS nsCSSKeyframeRule : public mozilla::css::Rule,
                                         public nsIDOMMozCSSKeyframeRule
{
public:
  
  nsCSSKeyframeRule(nsTArray<float> aKeys,
                    nsAutoPtr<mozilla::css::Declaration> aDeclaration)
    : mDeclaration(aDeclaration)
  {
    mKeys.SwapElements(aKeys);
  }
private:
  nsCSSKeyframeRule(const nsCSSKeyframeRule& aCopy);
  ~nsCSSKeyframeRule();
public:
  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  DECL_STYLE_RULE_INHERIT
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMMOZCSSKEYFRAMERULE

  const nsTArray<float>& GetKeys() const     { return mKeys; }
  mozilla::css::Declaration* Declaration()   { return mDeclaration; }

  void ChangeDeclaration(mozilla::css::Declaration* aDeclaration);

private:
  nsAutoTArray<float, 1>                     mKeys;
  nsAutoPtr<mozilla::css::Declaration>       mDeclaration;
  
  nsRefPtr<nsCSSKeyframeStyleDeclaration>    mDOMDeclaration;
};

class NS_FINAL_CLASS nsCSSKeyframesRule : public mozilla::css::GroupRule,
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
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<mozilla::css::Rule> Clone() const;
  virtual nsIDOMCSSRule* GetDOMRule()
  {
    return this;
  }

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMMOZCSSKEYFRAMESRULE

  
  virtual PRBool UseForPresentation(nsPresContext* aPresContext,
                                    nsMediaQueryResultCacheKey& aKey);

  const nsString& GetName() { return mName; }

private:
  PRUint32 FindRuleIndexForKey(const nsAString& aKey);

  nsString                                   mName;
};
#endif

#endif 
