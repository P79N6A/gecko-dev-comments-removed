









































#ifndef mozilla_css_GroupRule_h__
#define mozilla_css_GroupRule_h__

#include "nsCSSRule.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"

class nsPresContext;
class nsMediaQueryResultCacheKey;

namespace mozilla {
namespace css {

class GroupRuleRuleList;



class GroupRule : public nsCSSRule
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
  void AppendStyleRule(nsICSSRule* aRule);

  PRInt32 StyleRuleCount() const { return mRules.Count(); }
  nsICSSRule* GetStyleRuleAt(PRInt32 aIndex) const;

  typedef nsCOMArray<nsICSSRule>::nsCOMArrayEnumFunc RuleEnumFunc;
  PRBool EnumerateRulesForwards(RuleEnumFunc aFunc, void * aData) const;

  




  nsresult DeleteStyleRuleAt(PRUint32 aIndex);
  nsresult InsertStyleRulesAt(PRUint32 aIndex,
                              nsCOMArray<nsICSSRule>& aRules);
  nsresult ReplaceStyleRule(nsICSSRule *aOld, nsICSSRule *aNew);

  virtual PRBool UseForPresentation(nsPresContext* aPresContext,
                                    nsMediaQueryResultCacheKey& aKey) = 0;

protected:
  
  nsresult AppendRulesToCssText(nsAString& aCssText);
  
  nsresult GetParentRule(nsIDOMCSSRule** aParentRule);

  
  
  nsIDOMCSSRuleList* GetCssRules();
  nsresult InsertRule(const nsAString & aRule, PRUint32 aIndex,
                      PRUint32* _retval);
  nsresult DeleteRule(PRUint32 aIndex);

  nsCOMArray<nsICSSRule> mRules;
  nsRefPtr<GroupRuleRuleList> mRuleCollection; 
};

} 
} 

#endif 
