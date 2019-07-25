









































#ifndef nsICSSGroupRule_h___
#define nsICSSGroupRule_h___

#include "nsICSSRule.h"
#include "nsCOMArray.h"

class nsIAtom;
class nsPresContext;
class nsMediaQueryResultCacheKey;


#define NS_ICSS_GROUP_RULE_IID     \
{0xd0f07b17, 0xe9c7, 0x4a54, {0xaa, 0xa4, 0x62, 0x24, 0x25, 0xb3, 0x8d, 0x27}}

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
