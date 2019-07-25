






































#ifndef nsICSSNameSpaceRule_h___
#define nsICSSNameSpaceRule_h___

#include "nsICSSRule.h"

class nsIAtom;


#define NS_ICSS_NAMESPACE_RULE_IID     \
{0xec064d33, 0xa6f1, 0x459c, {0x8f, 0x31, 0x82, 0x89, 0xee, 0xd8, 0x77, 0xde}}


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
