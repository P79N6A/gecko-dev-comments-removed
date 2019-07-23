









































#ifndef nsICSSStyleRuleDOMWrapper_h_
#define nsICSSStyleRuleDOMWrapper_h_

#include "nsIDOMCSSStyleRule.h"



#define NS_ICSS_STYLE_RULE_DOM_WRAPPER_IID \
{0x476a4290, 0x1194, 0x4099, {0x8f, 0x2d, 0xa1, 0xcc, 0xc9, 0xbd, 0xd6, 0x76}}

class nsICSSStyleRuleDOMWrapper : public nsIDOMCSSStyleRule {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_STYLE_RULE_DOM_WRAPPER_IID)

  NS_IMETHOD GetCSSStyleRule(nsICSSStyleRule** aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSStyleRuleDOMWrapper,
                              NS_ICSS_STYLE_RULE_DOM_WRAPPER_IID)

#endif 
