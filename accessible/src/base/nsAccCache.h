





































#ifndef _nsAccCache_H_
#define _nsAccCache_H_

#include "nsRefPtrHashtable.h"
#include "nsCycleCollectionParticipant.h"

class nsIAccessNode;








template <class T>
static PLDHashOperator
ClearCacheEntry(const void* aKey, nsRefPtr<T>& aAccessNode, void* aUserArg)
{
  NS_ASSERTION(aAccessNode, "Calling ClearCacheEntry with a NULL pointer!");
  if (aAccessNode)
    aAccessNode->Shutdown();

  return PL_DHASH_REMOVE;
}




template <class T>
static void
ClearCache(nsRefPtrHashtable<nsVoidPtrHashKey, T> & aCache)
{
  aCache.Enumerate(ClearCacheEntry<T>, nsnull);
}




template <class T>
static PLDHashOperator
CycleCollectorTraverseCacheEntry(const void *aKey, T *aAccessNode,
                                 void *aUserArg)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aUserArg);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "accessible cache entry");

  nsISupports *supports = static_cast<nsIAccessNode*>(aAccessNode);
  cb->NoteXPCOMChild(supports);
  return PL_DHASH_NEXT;
}




template <class T>
static void
CycleCollectorTraverseCache(nsRefPtrHashtable<nsVoidPtrHashKey, T> & aCache,
                            nsCycleCollectionTraversalCallback *aCallback)
{
  aCache.EnumerateRead(CycleCollectorTraverseCacheEntry<T>, aCallback);
}

#endif
