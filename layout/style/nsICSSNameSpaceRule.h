






































#ifndef nsICSSNameSpaceRule_h___
#define nsICSSNameSpaceRule_h___

#include "nsICSSRule.h"

class nsIAtom;


#define NS_ICSS_NAMESPACE_RULE_IID     \
{0x153392d4, 0x90bb, 0x424e, {0xb7, 0x37, 0x1b, 0xf5, 0xbc, 0x77, 0x53, 0x94}}


class nsICSSNameSpaceRule : public nsICSSRule {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_NAMESPACE_RULE_IID)

  NS_IMETHOD  GetPrefix(nsIAtom*& aPrefix) const = 0;
  NS_IMETHOD  SetPrefix(nsIAtom* aPrefix) = 0;

  NS_IMETHOD  GetURLSpec(nsString& aURLSpec) const = 0;
  NS_IMETHOD  SetURLSpec(const nsString& aURLSpec) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSNameSpaceRule, NS_ICSS_NAMESPACE_RULE_IID)

nsresult
NS_NewCSSNameSpaceRule(nsICSSNameSpaceRule** aInstancePtrResult, 
                       nsIAtom* aPrefix, const nsString& aURLSpec);

#endif 
