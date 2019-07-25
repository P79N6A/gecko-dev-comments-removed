









































#ifndef mozilla_css_GroupRule_h__
#define mozilla_css_GroupRule_h__

#include "mozilla/css/Rule.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"

class nsPresContext;
class nsMediaQueryResultCacheKey;

namespace mozilla {
namespace css {

class GroupRuleRuleList;



class GroupRule : public Rule
{
protected:
  GroupRule();
  GroupRule(const GroupRule& aCopy);
  virtual ~GroupRule();
public:

  
  DECL_STYLE_RULE_INHERIT_NO_DOMRULE
  virtual void SetStyleSheet(nsCSSStyleSheet* aSheet);

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

public:
  void AppendStyleRule(Rule* aRule);

  PRInt32 StyleRuleCount() const { return mRules.Count(); }
  Rule* GetStyleRuleAt(PRInt32 aIndex) const;

  typedef nsCOMArray<Rule>::nsCOMArrayEnumFunc RuleEnumFunc;
  PRBool EnumerateRulesForwards(RuleEnumFunc aFunc, void * aData) const;

  




  nsresult DeleteStyleRuleAt(PRUint32 aIndex);
  nsresult InsertStyleRulesAt(PRUint32 aIndex,
                              nsCOMArray<Rule>& aRules);
  nsresult ReplaceStyleRule(Rule *aOld, Rule *aNew);

  virtual PRBool UseForPresentation(nsPresContext* aPresContext,
                                    nsMediaQueryResultCacheKey& aKey) = 0;

protected:
  
  nsresult AppendRulesToCssText(nsAString& aCssText);
  
  nsresult GetParentRule(nsIDOMCSSRule** aParentRule);

  
  
  nsIDOMCSSRuleList* GetCssRules();
  nsresult InsertRule(const nsAString & aRule, PRUint32 aIndex,
                      PRUint32* _retval);
  nsresult DeleteRule(PRUint32 aIndex);

  nsCOMArray<Rule> mRules;
  nsRefPtr<GroupRuleRuleList> mRuleCollection; 
};

} 
} 

#endif 
