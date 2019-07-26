





#ifndef mozilla_dom_quota_utilities_h__
#define mozilla_dom_quota_utilities_h__

#include "mozilla/dom/quota/QuotaCommon.h"

BEGIN_QUOTA_NAMESPACE

inline void
IncrementUsage(uint64_t* aUsage, uint64_t aDelta)
{
  
  if ((UINT64_MAX - *aUsage) < aDelta) {
    NS_WARNING("Usage exceeds the maximum!");
    *aUsage = UINT64_MAX;
  }
  else {
    *aUsage += aDelta;
  }
}

inline bool
PatternMatchesOrigin(const nsACString& aPatternString, const nsACString& aOrigin)
{
  
  return StringBeginsWith(aOrigin, aPatternString);
}

END_QUOTA_NAMESPACE

#endif 
