









































#ifndef nsICSSGroupRule_h___
#define nsICSSGroupRule_h___

#include "nsICSSRule.h"
#include "nsCOMArray.h"

class nsPresContext;
class nsMediaQueryResultCacheKey;


#define NS_ICSS_GROUP_RULE_IID     \
{0x4d5e7eca, 0x433e, 0x491a, {0xb2, 0x26, 0x39, 0xa2, 0x00, 0x39, 0x0e, 0xa1}}


class nsICSSGroupRule : public nsICSSRule {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_GROUP_RULE_IID)

  NS_IMETHOD  AppendStyleRule(nsICSSRule* aRule) = 0;

  NS_IMETHOD  StyleRuleCount(PRInt32& aCount) const = 0;
  NS_IMETHOD  GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const = 0;

  typedef nsCOMArray<nsICSSRule>::nsCOMArrayEnumFunc RuleEnumFunc;
  NS_IMETHOD_(PRBool) EnumerateRulesForwards(RuleEnumFunc aFunc, void * aData) const = 0;

  




  NS_IMETHOD  DeleteStyleRuleAt(PRUint32 aIndex) = 0;
  NS_IMETHOD  InsertStyleRulesAt(PRUint32 aIndex,
                                 nsCOMArray<nsICSSRule>& aRules) = 0;
  NS_IMETHOD  ReplaceStyleRule(nsICSSRule* aOld, nsICSSRule* aNew) = 0;

  NS_IMETHOD_(PRBool) UseForPresentation(nsPresContext* aPresContext,
                                         nsMediaQueryResultCacheKey& aKey) = 0;
   
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSGroupRule, NS_ICSS_GROUP_RULE_IID)

#endif 
