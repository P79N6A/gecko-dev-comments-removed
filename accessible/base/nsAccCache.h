




#ifndef _nsAccCache_H_
#define _nsAccCache_H_

#include "xpcAccessibleDocument.h"








template <class T>
static PLDHashOperator
ClearCacheEntry(const void* aKey, nsRefPtr<T>& aAccessible, void* aUserArg)
{
  NS_ASSERTION(aAccessible, "Calling ClearCacheEntry with a nullptr pointer!");
  if (aAccessible && !aAccessible->IsDefunct())
    aAccessible->Shutdown();

  return PL_DHASH_REMOVE;
}

template <class T>
static PLDHashOperator
UnbindCacheEntryFromDocument(const void* aKey, nsRefPtr<T>& aAccessible,
                             void* aUserArg)
{
  MOZ_ASSERT(aAccessible && !aAccessible->IsDefunct());
  aAccessible->Document()->UnbindFromDocument(aAccessible);

  return PL_DHASH_REMOVE;
}





template <class T>
static void
ClearCache(nsRefPtrHashtable<nsPtrHashKey<const void>, T>& aCache)
{
  aCache.Enumerate(ClearCacheEntry<T>, nullptr);
}

#endif
