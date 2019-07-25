









#ifndef nsICSSStyleRuleDOMWrapper_h_
#define nsICSSStyleRuleDOMWrapper_h_

#include "nsIDOMCSSStyleRule.h"



#define NS_ICSS_STYLE_RULE_DOM_WRAPPER_IID \
{0xcee1bbb6, 0x0a32, 0x4cf3, {0x8d, 0x42, 0xba, 0x39, 0x38, 0xe9, 0xec, 0xaa}}


class nsICSSStyleRuleDOMWrapper : public nsIDOMCSSStyleRule {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_STYLE_RULE_DOM_WRAPPER_IID)

  NS_IMETHOD GetCSSStyleRule(mozilla::css::StyleRule** aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSStyleRuleDOMWrapper,
                              NS_ICSS_STYLE_RULE_DOM_WRAPPER_IID)

#endif 
