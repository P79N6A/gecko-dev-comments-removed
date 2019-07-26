







#ifndef nsCycleCollectionNoteChild_h__
#define nsCycleCollectionNoteChild_h__

#include "nsCycleCollectionTraversalCallback.h"
#include "mozilla/Likely.h"

enum {
  CycleCollectionEdgeNameArrayFlag = 1
};


void
CycleCollectionNoteEdgeNameImpl(nsCycleCollectionTraversalCallback& aCallback,
                                const char* aName,
                                uint32_t aFlags = 0);


MOZ_ALWAYS_INLINE void
CycleCollectionNoteEdgeName(nsCycleCollectionTraversalCallback& aCallback,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  if (MOZ_UNLIKELY(aCallback.WantDebugInfo())) {
    CycleCollectionNoteEdgeNameImpl(aCallback, aName, aFlags);
  }
}

#endif 
