









































#ifndef nsCSSRules_h_
#define nsCSSRules_h_

#include "nsCSSRule.h"
#include "nsICSSGroupRule.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsIDOMCSSMediaRule.h"
#include "nsIDOMCSSMozDocumentRule.h"
#include "nsString.h"

class CSSGroupRuleRuleListImpl;
class nsMediaList;

#define DECL_STYLE_RULE_INHERIT  \
NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const; \
NS_IMETHOD SetStyleSheet(nsICSSStyleSheet* aSheet); \
NS_IMETHOD SetParentRule(nsICSSGroupRule* aRule); \
NS_IMETHOD GetDOMRule(nsIDOMCSSRule** aDOMRule); \
NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);



class nsCSSGroupRule : public nsCSSRule, public nsICSSGroupRule
{
protected:
  nsCSSGroupRule();
  nsCSSGroupRule(const nsCSSGroupRule& aCopy);
  ~nsCSSGroupRule();

  
  DECL_STYLE_RULE_INHERIT

  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

public:
  
  NS_IMETHOD AppendStyleRule(nsICSSRule* aRule);
  NS_IMETHOD StyleRuleCount(PRInt32& aCount) const;
  NS_IMETHOD GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const;
  NS_IMETHOD EnumerateRulesForwards(RuleEnumFunc aFunc, void * aData) const;
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

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSMEDIARULE

  
  NS_IMETHOD_(PRBool) UseForPresentation(nsPresContext* aPresContext);

  
  nsresult SetMedia(nsMediaList* aMedia);
  
protected:
  nsCOMPtr<nsMediaList> mMedia;
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

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSMOZDOCUMENTRULE

  
  NS_IMETHOD_(PRBool) UseForPresentation(nsPresContext* aPresContext);

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
    ~URL() { delete next; }
  };

  void SetURLs(URL *aURLs) { mURLs = aURLs; }

protected:
  nsAutoPtr<URL> mURLs; 
};


#endif 
