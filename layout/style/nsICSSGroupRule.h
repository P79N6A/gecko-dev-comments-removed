









































#ifndef nsICSSGroupRule_h___
#define nsICSSGroupRule_h___

#include "nsICSSRule.h"
#include "nsCOMArray.h"

class nsPresContext;
class nsMediaQueryResultCacheKey;


#define NS_ICSS_GROUP_RULE_IID     \
{0x388222c0, 0xcb76, 0x4a01, {0x99, 0x88, 0x8c, 0xd2, 0x8e, 0x66, 0x69, 0x0e}}


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
