




#ifndef _nsAccCache_H_
#define _nsAccCache_H_

#include "xpcAccessibleDocument.h"








template <class T>
static PLDHashOperator
ClearCacheEntry(const void* aKey, nsRefPtr<T>& aAccessible, void* aUserArg)
{
  NS_ASSERTION(aAccessible, "Calling ClearCacheEntry with a nullptr pointer!");
  if (aAccessible)
    aAccessible->Shutdown();

  return PL_DHASH_REMOVE;
}





static void
ClearCache(mozilla::a11y::AccessibleHashtable& aCache)
{
  aCache.Enumerate(ClearCacheEntry<mozilla::a11y::Accessible>, nullptr);
}

#endif
