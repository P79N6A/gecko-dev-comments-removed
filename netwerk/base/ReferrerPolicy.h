



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


const char kRPS_Never[]                       = "never";
const char kRPS_No_Referrer[]                 = "no-referrer";


const char kRPS_Origin[]                      = "origin";


const char kRPS_Default[]                     = "default";
const char kRPS_No_Referrer_When_Downgrade[]  = "no-referrer-when-downgrade";


const char kRPS_Origin_When_Cross_Origin[]    = "origin-when-cross-origin";
const char kRPS_Origin_When_Crossorigin[]     = "origin-when-crossorigin";


const char kRPS_Always[]                      = "always";
const char kRPS_Unsafe_URL[]                  = "unsafe-url";

inline ReferrerPolicy
ReferrerPolicyFromString(const nsAString& content)
{
  
  
  if (content.LowerCaseEqualsLiteral(kRPS_Never) ||
      content.LowerCaseEqualsLiteral(kRPS_No_Referrer)) {
    return RP_No_Referrer;
  }
  if (content.LowerCaseEqualsLiteral(kRPS_Origin)) {
    return RP_Origin;
  }
  if (content.LowerCaseEqualsLiteral(kRPS_Default) ||
      content.LowerCaseEqualsLiteral(kRPS_No_Referrer_When_Downgrade)) {
    return RP_No_Referrer_When_Downgrade;
  }
  if (content.LowerCaseEqualsLiteral(kRPS_Origin_When_Cross_Origin) ||
      content.LowerCaseEqualsLiteral(kRPS_Origin_When_Crossorigin)) {
    return RP_Origin_When_Crossorigin;
  }
  if (content.LowerCaseEqualsLiteral(kRPS_Always) ||
      content.LowerCaseEqualsLiteral(kRPS_Unsafe_URL)) {
    return RP_Unsafe_URL;
  }
  
  return RP_No_Referrer;

}

inline bool
IsValidReferrerPolicy(const nsAString& content)
{
  return content.LowerCaseEqualsLiteral(kRPS_Never)
      || content.LowerCaseEqualsLiteral(kRPS_No_Referrer)
      || content.LowerCaseEqualsLiteral(kRPS_Origin)
      || content.LowerCaseEqualsLiteral(kRPS_Default)
      || content.LowerCaseEqualsLiteral(kRPS_No_Referrer_When_Downgrade)
      || content.LowerCaseEqualsLiteral(kRPS_Origin_When_Cross_Origin)
      || content.LowerCaseEqualsLiteral(kRPS_Origin_When_Crossorigin)
      || content.LowerCaseEqualsLiteral(kRPS_Always)
      || content.LowerCaseEqualsLiteral(kRPS_Unsafe_URL);
}

inline bool
IsValidAttributeReferrerPolicy(const nsAString& aContent)
{
  
  
  return aContent.LowerCaseEqualsLiteral(kRPS_No_Referrer)
      || aContent.LowerCaseEqualsLiteral(kRPS_Origin)
      || aContent.LowerCaseEqualsLiteral(kRPS_Unsafe_URL);
}

inline ReferrerPolicy
AttributeReferrerPolicyFromString(const nsAString& aContent)
{
  
  if (aContent.IsEmpty()) {
    return RP_Unset;
  }
  
  
  if (IsValidAttributeReferrerPolicy(aContent)) {
    return ReferrerPolicyFromString(aContent);
  }
  
  
  return RP_No_Referrer;
}

} 
} 

#endif
