








#ifndef nsPresArena_h___
#define nsPresArena_h___

#include "mozilla/MemoryChecking.h" 
#include "mozilla/MemoryReporting.h"
#include <stdint.h>
#include "nscore.h"
#include "nsQueryFrame.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "plarena.h"

struct nsArenaMemoryStats;

class nsPresArena {
public:
  nsPresArena();
  ~nsPresArena();

  enum ObjectID {
    nsLineBox_id = nsQueryFrame::NON_FRAME_MARKER,
    nsRuleNode_id,
    nsStyleContext_id,
    nsFrameList_id,

    







    NON_OBJECT_MARKER = 0x40000000
  };

  


  NS_HIDDEN_(void*) AllocateBySize(size_t aSize)
  {
    return Allocate(uint32_t(aSize) | uint32_t(NON_OBJECT_MARKER), aSize);
  }
  NS_HIDDEN_(void) FreeBySize(size_t aSize, void* aPtr)
  {
    Free(uint32_t(aSize) | uint32_t(NON_OBJECT_MARKER), aPtr);
  }

  



  NS_HIDDEN_(void*) AllocateByFrameID(nsQueryFrame::FrameIID aID, size_t aSize)
  {
    return Allocate(aID, aSize);
  }
  NS_HIDDEN_(void) FreeByFrameID(nsQueryFrame::FrameIID aID, void* aPtr)
  {
    Free(aID, aPtr);
  }

  



  NS_HIDDEN_(void*) AllocateByObjectID(ObjectID aID, size_t aSize)
  {
    return Allocate(aID, aSize);
  }
  NS_HIDDEN_(void) FreeByObjectID(ObjectID aID, void* aPtr)
  {
    Free(aID, aPtr);
  }

  



  void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                              nsArenaMemoryStats* aArenaStats);

private:
  NS_HIDDEN_(void*) Allocate(uint32_t aCode, size_t aSize);
  NS_HIDDEN_(void) Free(uint32_t aCode, void* aPtr);

  
  
  class FreeList : public PLDHashEntryHdr
  {
  public:
    typedef uint32_t KeyType;
    nsTArray<void *> mEntries;
    size_t mEntrySize;
    size_t mEntriesEverAllocated;

    typedef const void* KeyTypePointer;
    KeyTypePointer mKey;

    FreeList(KeyTypePointer aKey)
    : mEntrySize(0), mEntriesEverAllocated(0), mKey(aKey) {}
    

    bool KeyEquals(KeyTypePointer const aKey) const
    { return mKey == aKey; }

    static KeyTypePointer KeyToPointer(KeyType aKey)
    { return NS_INT32_TO_PTR(aKey); }

    static PLDHashNumber HashKey(KeyTypePointer aKey)
    { return NS_PTR_TO_INT32(aKey); }

    enum { ALLOW_MEMMOVE = false };
  };

#if defined(MOZ_HAVE_MEM_CHECKS)
  static PLDHashOperator UnpoisonFreeList(FreeList* aEntry, void*);
#endif
  static PLDHashOperator FreeListEnumerator(FreeList* aEntry, void* aData);
  static size_t SizeOfFreeListEntryExcludingThis(FreeList* aEntry,
                                                 mozilla::MallocSizeOf aMallocSizeOf,
                                                 void*);

  nsTHashtable<FreeList> mFreeLists;
  PLArenaPool mPool;
};

#endif
