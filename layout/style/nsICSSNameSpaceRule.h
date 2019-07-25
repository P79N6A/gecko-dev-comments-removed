






































#ifndef nsICSSNameSpaceRule_h___
#define nsICSSNameSpaceRule_h___

#include "nsICSSRule.h"


class nsIAtom;


#define NS_ICSS_NAMESPACE_RULE_IID     \
{0xc2116a9f, 0x370d, 0x458a, {0x80, 0xc1, 0xc6, 0x44, 0x50, 0x89, 0xa8, 0xb7}}

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
