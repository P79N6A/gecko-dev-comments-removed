












#define ALIGN_SHIFT 3
#define PL_ARENA_CONST_ALIGN_MASK ((uintptr_t(1) << ALIGN_SHIFT) - 1)
#include "plarena.h"



#include "nsPresArena.h"

#include "mozilla/Poison.h"
#include "nsDebug.h"
#include "nsArenaMemoryStats.h"
#include "nsPrintfCString.h"


static const size_t ARENA_PAGE_SIZE = 8192;

nsPresArena::nsPresArena()
{
  PL_INIT_ARENA_POOL(&mPool, "PresArena", ARENA_PAGE_SIZE);
}

nsPresArena::~nsPresArena()
{
#if defined(MOZ_HAVE_MEM_CHECKS)
  mFreeLists.EnumerateEntries(UnpoisonFreeList, nullptr);
#endif
  PL_FinishArenaPool(&mPool);
}

void*
nsPresArena::Allocate(uint32_t aCode, size_t aSize)
{
  NS_ABORT_IF_FALSE(aSize > 0, "PresArena cannot allocate zero bytes");

  
  aSize = PL_ARENA_ALIGN(&mPool, aSize);

  
  
  FreeList* list = mFreeLists.PutEntry(aCode);

  nsTArray<void*>::index_type len = list->mEntries.Length();
  if (list->mEntrySize == 0) {
    NS_ABORT_IF_FALSE(len == 0, "list with entries but no recorded size");
    list->mEntrySize = aSize;
  } else {
    NS_ABORT_IF_FALSE(list->mEntrySize == aSize,
                      "different sizes for same object type code");
  }

  void* result;
  if (len > 0) {
    
    result = list->mEntries.ElementAt(len - 1);
    list->mEntries.RemoveElementAt(len - 1);
#if defined(DEBUG)
    {
      MOZ_MAKE_MEM_DEFINED(result, list->mEntrySize);
      char* p = reinterpret_cast<char*>(result);
      char* limit = p + list->mEntrySize;
      for (; p < limit; p += sizeof(uintptr_t)) {
        uintptr_t val = *reinterpret_cast<uintptr_t*>(p);
        NS_ABORT_IF_FALSE(val == mozPoisonValue(),
                          nsPrintfCString("PresArena: poison overwritten; "
                                          "wanted %.16llx "
                                          "found %.16llx "
                                          "errors in bits %.16llx",
                                          uint64_t(mozPoisonValue()),
                                          uint64_t(val),
                                          uint64_t(mozPoisonValue() ^ val)
                                          ).get());
      }
    }
#endif
    MOZ_MAKE_MEM_UNDEFINED(result, list->mEntrySize);
    return result;
  }

  
  list->mEntriesEverAllocated++;
  PL_ARENA_ALLOCATE(result, &mPool, aSize);
  if (!result) {
    NS_RUNTIMEABORT("out of memory");
  }
  return result;
}

void
nsPresArena::Free(uint32_t aCode, void* aPtr)
{
  
  FreeList* list = mFreeLists.GetEntry(aCode);
  NS_ABORT_IF_FALSE(list, "no free list for pres arena object");
  NS_ABORT_IF_FALSE(list->mEntrySize > 0, "PresArena cannot free zero bytes");

  mozWritePoison(aPtr, list->mEntrySize);

  MOZ_MAKE_MEM_NOACCESS(aPtr, list->mEntrySize);
  list->mEntries.AppendElement(aPtr);
}

struct EnumerateData {
  nsArenaMemoryStats* stats;
  size_t total;
};

#if defined(MOZ_HAVE_MEM_CHECKS)
 PLDHashOperator
nsPresArena::UnpoisonFreeList(FreeList* aEntry, void*)
{
  nsTArray<void*>::index_type len;
  while ((len = aEntry->mEntries.Length())) {
    void* result = aEntry->mEntries.ElementAt(len - 1);
    aEntry->mEntries.RemoveElementAt(len - 1);
    MOZ_MAKE_MEM_UNDEFINED(result, aEntry->mEntrySize);
  }
  return PL_DHASH_NEXT;
}
#endif

 PLDHashOperator
nsPresArena::FreeListEnumerator(FreeList* aEntry, void* aData)
{
  EnumerateData* data = static_cast<EnumerateData*>(aData);
  
  
  
  
  
  size_t totalSize = aEntry->mEntrySize * aEntry->mEntriesEverAllocated;
  size_t* p;

  switch (NS_PTR_TO_INT32(aEntry->mKey)) {
#define FRAME_ID(classname)                                      \
    case nsQueryFrame::classname##_id:                           \
      p = &data->stats->FRAME_ID_STAT_FIELD(classname);          \
      break;
#include "nsFrameIdList.h"
#undef FRAME_ID
  case nsLineBox_id:
    p = &data->stats->mLineBoxes;
    break;
  case nsRuleNode_id:
    p = &data->stats->mRuleNodes;
    break;
  case nsStyleContext_id:
    p = &data->stats->mStyleContexts;
    break;
  default:
    return PL_DHASH_NEXT;
  }

  *p += totalSize;
  data->total += totalSize;

  return PL_DHASH_NEXT;
}

void
nsPresArena::AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                    nsArenaMemoryStats* aArenaStats)
{
  
  
  
  
  
  
  
  
  
  

  size_t mallocSize = PL_SizeOfArenaPoolExcludingPool(&mPool, aMallocSizeOf);
  mallocSize += mFreeLists.SizeOfExcludingThis(aMallocSizeOf);

  EnumerateData data = { aArenaStats, 0 };
  mFreeLists.EnumerateEntries(FreeListEnumerator, &data);
  aArenaStats->mOther += mallocSize - data.total;
}
