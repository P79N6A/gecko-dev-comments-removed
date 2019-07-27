



#ifndef ReferrerPolicy_h__
#define ReferrerPolicy_h__

#include "nsStringGlue.h"
#include "nsIHttpChannel.h"

namespace mozilla { namespace net {

enum ReferrerPolicy {
  
  RP_No_Referrer                 = nsIHttpChannel::REFERRER_POLICY_NO_REFERRER,

  
  RP_Origin                      = nsIHttpChannel::REFERRER_POLICY_ORIGIN,

  
  RP_No_Referrer_When_Downgrade  = nsIHttpChannel::REFERRER_POLICY_NO_REFERRER_WHEN_DOWNGRADE,
  RP_Default                     = nsIHttpChannel::REFERRER_POLICY_NO_REFERRER_WHEN_DOWNGRADE,

  
  RP_Origin_When_Crossorigin     = nsIHttpChannel::REFERRER_POLICY_ORIGIN_WHEN_XORIGIN,

  
  RP_Unsafe_URL                  = nsIHttpChannel::REFERRER_POLICY_UNSAFE_URL,

  
  RP_Unset                       = nsIHttpChannel::REFERRER_POLICY_NO_REFERRER_WHEN_DOWNGRADE
};

inline ReferrerPolicy
ReferrerPolicyFromString(const nsAString& content)
{
  
  
  if (content.LowerCaseEqualsLiteral("never") ||
      content.LowerCaseEqualsLiteral("no-referrer")) {
    return RP_No_Referrer;
  }
  if (content.LowerCaseEqualsLiteral("origin")) {
    return RP_Origin;
  }
  if (content.LowerCaseEqualsLiteral("default") ||
      content.LowerCaseEqualsLiteral("no-referrer-when-downgrade")) {
    return RP_No_Referrer_When_Downgrade;
  }
  if (content.LowerCaseEqualsLiteral("origin-when-cross-origin") ||
      content.LowerCaseEqualsLiteral("origin-when-crossorigin")) {
    return RP_Origin_When_Crossorigin;
  }
  if (content.LowerCaseEqualsLiteral("always") ||
      content.LowerCaseEqualsLiteral("unsafe-url")) {
    return RP_Unsafe_URL;
  }
  
  return RP_No_Referrer;

}

inline bool
IsValidReferrerPolicy(const nsAString& content)
{
  return content.LowerCaseEqualsLiteral("never")
      || content.LowerCaseEqualsLiteral("no-referrer")
      || content.LowerCaseEqualsLiteral("origin")
      || content.LowerCaseEqualsLiteral("default")
      || content.LowerCaseEqualsLiteral("no-referrer-when-downgrade")
      || content.LowerCaseEqualsLiteral("origin-when-cross-origin")
      || content.LowerCaseEqualsLiteral("origin-when-crossorigin")
      || content.LowerCaseEqualsLiteral("always")
      || content.LowerCaseEqualsLiteral("unsafe-url");
}

} } 

#endif
