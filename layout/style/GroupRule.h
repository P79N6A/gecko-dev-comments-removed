









































#ifndef mozilla_css_GroupRule_h__
#define mozilla_css_GroupRule_h__

#include "nsICSSRule.h"
#include "nsCSSRule.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"

class nsPresContext;
class nsMediaQueryResultCacheKey;

namespace mozilla {
namespace css {

class GroupRuleRuleList;



class GroupRule : public nsCSSRule, public nsICSSRule
{
protected:
  GroupRule();
  GroupRule(const GroupRule& aCopy);
  virtual ~GroupRule();

protected:
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
public:

  
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  
  DECL_STYLE_RULE_INHERIT_NO_DOMRULE

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

public:
  NS_IMETHOD  AppendStyleRule(nsICSSRule* aRule);

  NS_IMETHOD  StyleRuleCount(PRInt32& aCount) const;
  NS_IMETHOD  GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const;

  typedef nsCOMArray<nsICSSRule>::nsCOMArrayEnumFunc RuleEnumFunc;
  NS_IMETHOD_(PRBool) EnumerateRulesForwards(RuleEnumFunc aFunc, void * aData) const;

  




  NS_IMETHOD  DeleteStyleRuleAt(PRUint32 aIndex);
  NS_IMETHOD  InsertStyleRulesAt(PRUint32 aIndex,
                                 nsCOMArray<nsICSSRule>& aRules);
  NS_IMETHOD  ReplaceStyleRule(nsICSSRule *aOld, nsICSSRule *aNew);

  NS_IMETHOD_(PRBool) UseForPresentation(nsPresContext* aPresContext,
                                         nsMediaQueryResultCacheKey& aKey) = 0;

protected:
  
  nsresult AppendRulesToCssText(nsAString& aCssText);
  
  nsresult GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet);
  nsresult GetParentRule(nsIDOMCSSRule** aParentRule);

  
  
  nsresult GetCssRules(nsIDOMCSSRuleList* *aRuleList);
  nsresult InsertRule(const nsAString & aRule, PRUint32 aIndex,
                      PRUint32* _retval);
  nsresult DeleteRule(PRUint32 aIndex);

  nsCOMArray<nsICSSRule> mRules;
  nsRefPtr<GroupRuleRuleList> mRuleCollection; 
};

} 
} 

#endif 
