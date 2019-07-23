









































#ifndef nsCSSRules_h_
#define nsCSSRules_h_

#include "nsCSSRule.h"
#include "nsICSSGroupRule.h"
#include "nsIDOMCSSMediaRule.h"
#include "nsIDOMCSSMozDocumentRule.h"
#include "nsIDOMCSSFontFaceRule.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsAutoPtr.h"
#include "nsCSSProperty.h"
#include "nsCSSValue.h"

class CSSGroupRuleRuleListImpl;
class nsMediaList;

#define DECL_STYLE_RULE_INHERIT_NO_DOMRULE  \
NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const; \
NS_IMETHOD SetStyleSheet(nsICSSStyleSheet* aSheet); \
NS_IMETHOD SetParentRule(nsICSSGroupRule* aRule); \
NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);

#define DECL_STYLE_RULE_INHERIT  \
DECL_STYLE_RULE_INHERIT_NO_DOMRULE \
nsIDOMCSSRule* GetDOMRuleWeak(nsresult* aResult);



class nsCSSGroupRule : public nsCSSRule, public nsICSSGroupRule
{
protected:
  nsCSSGroupRule();
  nsCSSGroupRule(const nsCSSGroupRule& aCopy);
  ~nsCSSGroupRule();

  
  DECL_STYLE_RULE_INHERIT_NO_DOMRULE

  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

public:
  
  NS_IMETHOD AppendStyleRule(nsICSSRule* aRule);
  NS_IMETHOD StyleRuleCount(PRInt32& aCount) const;
  NS_IMETHOD GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const;
  NS_IMETHOD_(PRBool) EnumerateRulesForwards(RuleEnumFunc aFunc, void * aData) const;
  NS_IMETHOD DeleteStyleRuleAt(PRUint32 aIndex);
  NS_IMETHOD InsertStyleRulesAt(PRUint32 aIndex,
                                nsCOMArray<nsICSSRule>& aRules);
  NS_IMETHOD ReplaceStyleRule(nsICSSRule *aOld, nsICSSRule *aNew);

protected:
  
  nsresult AppendRulesToCssText(nsAString& aCssText);
  
  nsresult GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet);
  nsresult GetParentRule(nsIDOMCSSRule** aParentRule);

  
  
  nsresult GetCssRules(nsIDOMCSSRuleList* *aRuleList);
  nsresult InsertRule(const nsAString & aRule, PRUint32 aIndex,
                      PRUint32* _retval);
  nsresult DeleteRule(PRUint32 aIndex);

  nsCOMArray<nsICSSRule> mRules;
  CSSGroupRuleRuleListImpl* mRuleCollection;
};

class nsCSSMediaRule : public nsCSSGroupRule,
                       public nsIDOMCSSMediaRule
{
public:
  nsCSSMediaRule();
  nsCSSMediaRule(const nsCSSMediaRule& aCopy);
  virtual ~nsCSSMediaRule();

  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  NS_IMETHOD SetStyleSheet(nsICSSStyleSheet* aSheet); 
  NS_IMETHOD GetType(PRInt32& aType) const;
  NS_IMETHOD Clone(nsICSSRule*& aClone) const;
  nsIDOMCSSRule* GetDOMRuleWeak(nsresult *aResult)
  {
    *aResult = NS_OK;
    return this;
  }

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSMEDIARULE

  
  NS_IMETHOD_(PRBool) UseForPresentation(nsPresContext* aPresContext,
                                         nsMediaQueryResultCacheKey& aKey);

  
  nsresult SetMedia(nsMediaList* aMedia);
  
protected:
  nsRefPtr<nsMediaList> mMedia;
};

class nsCSSDocumentRule : public nsCSSGroupRule,
                          public nsIDOMCSSMozDocumentRule
{
public:
  nsCSSDocumentRule(void);
  nsCSSDocumentRule(const nsCSSDocumentRule& aCopy);
  virtual ~nsCSSDocumentRule(void);

  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  NS_IMETHOD GetType(PRInt32& aType) const;
  NS_IMETHOD Clone(nsICSSRule*& aClone) const;
  nsIDOMCSSRule* GetDOMRuleWeak(nsresult *aResult)
  {
    *aResult = NS_OK;
    return this;
  }

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSMOZDOCUMENTRULE

  
  NS_IMETHOD_(PRBool) UseForPresentation(nsPresContext* aPresContext,
                                         nsMediaQueryResultCacheKey& aKey);

  enum Function {
    eURL,
    eURLPrefix,
    eDomain
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

  static nsCSSValue nsCSSFontFaceStyleDecl::* const Fields[];  
  inline nsCSSFontFaceRule* ContainingRule();
  inline const nsCSSFontFaceRule* ContainingRule() const;

private:
  
  
  
  void* operator new(size_t size) CPP_THROW_NEW;
};

class nsCSSFontFaceRule : public nsCSSRule,
                          public nsICSSRule,
                          public nsIDOMCSSFontFaceRule
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  DECL_STYLE_RULE_INHERIT

  NS_IMETHOD GetType(PRInt32& aType) const;
  NS_IMETHOD Clone(nsICSSRule*& aClone) const;

  
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

#endif 
