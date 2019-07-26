




#ifndef _nsAccCache_H_
#define _nsAccCache_H_

#include "nsIAccessible.h"
#include "nsRefPtrHashtable.h"
#include "nsCycleCollectionParticipant.h"








template <class T>
static PLDHashOperator
ClearCacheEntry(const void* aKey, nsRefPtr<T>& aAccessible, void* aUserArg)
{
  NS_ASSERTION(aAccessible, "Calling ClearCacheEntry with a NULL pointer!");
  if (aAccessible)
    aAccessible->Shutdown();

  return PL_DHASH_REMOVE;
}





static void
ClearCache(mozilla::a11y::AccessibleHashtable& aCache)
{
  aCache.Enumerate(ClearCacheEntry<mozilla::a11y::Accessible>, nullptr);
}




template <class T>
static PLDHashOperator
CycleCollectorTraverseCacheEntry(const void *aKey, T *aAccessible,
                                 void *aUserArg)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aUserArg);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "accessible cache entry");

  nsISupports *supports = static_cast<nsIAccessible*>(aAccessible);
  cb->NoteXPCOMChild(supports);
  return PL_DHASH_NEXT;
}





static void
CycleCollectorTraverseCache(mozilla::a11y::AccessibleHashtable& aCache,
                            nsCycleCollectionTraversalCallback *aCallback)
{
  aCache.EnumerateRead(CycleCollectorTraverseCacheEntry<mozilla::a11y::Accessible>,
                       aCallback);
}

#endif
