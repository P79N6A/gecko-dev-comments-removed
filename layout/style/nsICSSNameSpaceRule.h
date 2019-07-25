






































#ifndef nsICSSNameSpaceRule_h
#define nsICSSNameSpaceRule_h

#include "nsICSSRule.h"

class nsIAtom;

#define NS_ICSS_NAMESPACE_RULE_IID \
{ 0x9be32bb3, 0x5729, 0x4853, \
  { 0x87, 0x29, 0x9b, 0x46, 0x69, 0xad, 0x82, 0x1b } }


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
