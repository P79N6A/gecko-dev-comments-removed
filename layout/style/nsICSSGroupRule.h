









































#ifndef nsICSSGroupRule_h
#define nsICSSGroupRule_h

#include "nsICSSRule.h"
#include "nsCOMArray.h"

class nsPresContext;
class nsMediaQueryResultCacheKey;

#define NS_ICSS_GROUP_RULE_IID \
{ 0xf1e3d96b, 0xe381, 0x4533, \
  { 0xa6, 0x5e, 0xa5, 0x31, 0xba, 0xca, 0x93, 0x62 } }


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
