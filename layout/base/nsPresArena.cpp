








#include "nsPresArena.h"
#include "nsCRT.h"
#include "nsDebug.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "prinit.h"
#include "prlog.h"
#include "nsArenaMemoryStats.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsPrintfCString.h"

#ifdef MOZ_CRASHREPORTER
#include "nsICrashReporter.h"
#endif

#include "mozilla/StandardInteger.h"
#include "mozilla/MemoryChecking.h"





#define ALIGN_SHIFT 3
#define PL_ARENA_CONST_ALIGN_MASK ((uintptr_t(1) << ALIGN_SHIFT) - 1)
#include "plarena.h"

#ifdef _WIN32
# include <windows.h>
#elif !defined(__OS2__)
# include <unistd.h>
# include <sys/mman.h>
# ifndef MAP_ANON
#  ifdef MAP_ANONYMOUS
#   define MAP_ANON MAP_ANONYMOUS
#  else
#   error "Don't know how to get anonymous memory"
#  endif
# endif
#endif


static const size_t ARENA_PAGE_SIZE = 8192;









#ifdef _WIN32
static void *
ReserveRegion(uintptr_t region, uintptr_t size)
{
  return VirtualAlloc((void *)region, size, MEM_RESERVE, PAGE_NOACCESS);
}

static void
ReleaseRegion(void *region, uintptr_t size)
{
  VirtualFree(region, size, MEM_RELEASE);
}

static bool
ProbeRegion(uintptr_t region, uintptr_t size)
{
  SYSTEM_INFO sinfo;
  GetSystemInfo(&sinfo);
  if (region >= (uintptr_t)sinfo.lpMaximumApplicationAddress &&
      region + size >= (uintptr_t)sinfo.lpMaximumApplicationAddress) {
    return true;
  } else {
    return false;
  }
}

static uintptr_t
GetDesiredRegionSize()
{
  SYSTEM_INFO sinfo;
  GetSystemInfo(&sinfo);
  return sinfo.dwAllocationGranularity;
}

#define RESERVE_FAILED 0

#elif defined(__OS2__)
static void *
ReserveRegion(uintptr_t region, uintptr_t size)
{
  
  
  return (void*)0xFFFD0000;
}

static void
ReleaseRegion(void *region, uintptr_t size)
{
  return;
}

static bool
ProbeRegion(uintptr_t region, uintptr_t size)
{
  
  
  return false;
}

static uintptr_t
GetDesiredRegionSize()
{
  
  return 0x1000;
}

#define RESERVE_FAILED 0

#else 

static void *
ReserveRegion(uintptr_t region, uintptr_t size)
{
  return mmap(reinterpret_cast<void*>(region), size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
}

static void
ReleaseRegion(void *region, uintptr_t size)
{
  munmap(region, size);
}

static bool
ProbeRegion(uintptr_t region, uintptr_t size)
{
  if (madvise(reinterpret_cast<void*>(region), size, MADV_NORMAL)) {
    return true;
  } else {
    return false;
  }
}

static uintptr_t
GetDesiredRegionSize()
{
  return sysconf(_SC_PAGESIZE);
}

#define RESERVE_FAILED MAP_FAILED

#endif 

PR_STATIC_ASSERT(sizeof(uintptr_t) == 4 || sizeof(uintptr_t) == 8);
PR_STATIC_ASSERT(sizeof(uintptr_t) == sizeof(void *));

static uintptr_t
ReservePoisonArea(uintptr_t rgnsize)
{
  if (sizeof(uintptr_t) == 8) {
    
    
    
    return
      (((uintptr_t(0x7FFFFFFFu) << 31) << 1 | uintptr_t(0xF0DEAFFFu))
       & ~(rgnsize-1));

  } else {
    
    uintptr_t candidate = (0xF0DEAFFF & ~(rgnsize-1));
    void *result = ReserveRegion(candidate, rgnsize);
    if (result == (void *)candidate) {
      
      return candidate;
    }

    
    
    if (ProbeRegion(candidate, rgnsize)) {
      
      if (result != RESERVE_FAILED)
        ReleaseRegion(result, rgnsize);
      return candidate;
    }

    
    
    if (result != RESERVE_FAILED) {
      return uintptr_t(result);
    }

    
    
    result = ReserveRegion(0, rgnsize);
    if (result != RESERVE_FAILED) {
      return uintptr_t(result);
    }

    NS_RUNTIMEABORT("no usable poison region identified");
    return 0;
  }
}

static uintptr_t ARENA_POISON;
static PRCallOnceType ARENA_POISON_guard;

static PRStatus
ARENA_POISON_init()
{
  uintptr_t rgnsize = GetDesiredRegionSize();
  uintptr_t rgnbase = ReservePoisonArea(rgnsize);

  if (rgnsize == 0) 
    return PR_FAILURE;

  ARENA_POISON = rgnbase + rgnsize/2 - 1;

#ifdef MOZ_CRASHREPORTER
  nsCOMPtr<nsICrashReporter> cr =
    do_GetService("@mozilla.org/toolkit/crash-reporter;1");
  bool enabled;
  if (cr && NS_SUCCEEDED(cr->GetEnabled(&enabled)) && enabled) {
    cr->AnnotateCrashReport(NS_LITERAL_CSTRING("FramePoisonBase"),
                            nsPrintfCString("%.16llx", uint64_t(rgnbase)));
    cr->AnnotateCrashReport(NS_LITERAL_CSTRING("FramePoisonSize"),
                            nsPrintfCString("%lu", uint32_t(rgnsize)));
  }
#endif
  return PR_SUCCESS;
}

#ifndef DEBUG_TRACEMALLOC_PRESARENA




namespace {

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

}

struct nsPresArena::State {
  nsTHashtable<FreeList> mFreeLists;
  PLArenaPool mPool;

  State()
  {
    mFreeLists.Init();
    PL_INIT_ARENA_POOL(&mPool, "PresArena", ARENA_PAGE_SIZE);
    PR_CallOnce(&ARENA_POISON_guard, ARENA_POISON_init);
  }

#if defined(MOZ_HAVE_MEM_CHECKS)
  static PLDHashOperator UnpoisonFreeList(FreeList* aEntry, void*)
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

  ~State()
  {
#if defined(MOZ_HAVE_MEM_CHECKS)
    mFreeLists.EnumerateEntries(UnpoisonFreeList, nullptr);
#endif
    PL_FinishArenaPool(&mPool);
  }

  void* Allocate(uint32_t aCode, size_t aSize)
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
          NS_ABORT_IF_FALSE(val == ARENA_POISON,
                            nsPrintfCString("PresArena: poison overwritten; "
                                            "wanted %.16llx "
                                            "found %.16llx "
                                            "errors in bits %.16llx",
                                            uint64_t(ARENA_POISON),
                                            uint64_t(val),
                                            uint64_t(ARENA_POISON ^ val)
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

  void Free(uint32_t aCode, void* aPtr)
  {
    
    FreeList* list = mFreeLists.GetEntry(aCode);
    NS_ABORT_IF_FALSE(list, "no free list for pres arena object");
    NS_ABORT_IF_FALSE(list->mEntrySize > 0, "PresArena cannot free zero bytes");

    char* p = reinterpret_cast<char*>(aPtr);
    char* limit = p + list->mEntrySize;
    for (; p < limit; p += sizeof(uintptr_t)) {
      *reinterpret_cast<uintptr_t*>(p) = ARENA_POISON;
    }

    MOZ_MAKE_MEM_NOACCESS(aPtr, list->mEntrySize);
    list->mEntries.AppendElement(aPtr);
  }

  static size_t SizeOfFreeListEntryExcludingThis(FreeList* aEntry,
                                                 nsMallocSizeOfFun aMallocSizeOf,
                                                 void *)
  {
    return aEntry->mEntries.SizeOfExcludingThis(aMallocSizeOf);
  }

  size_t SizeOfIncludingThisFromMalloc(nsMallocSizeOfFun aMallocSizeOf) const
  {
    size_t n = aMallocSizeOf(this);
    n += PL_SizeOfArenaPoolExcludingPool(&mPool, aMallocSizeOf);
    n += mFreeLists.SizeOfExcludingThis(SizeOfFreeListEntryExcludingThis,
                                        aMallocSizeOf);
    return n;
  }

  struct EnumerateData {
    nsArenaMemoryStats* stats;
    size_t total;
  };

  static PLDHashOperator FreeListEnumerator(FreeList* aEntry, void* aData)
  {
    EnumerateData* data = static_cast<EnumerateData*>(aData);
    
    
    
    
    
    size_t totalSize = aEntry->mEntrySize * aEntry->mEntriesEverAllocated;
    size_t* p;

    switch (NS_PTR_TO_INT32(aEntry->mKey)) {
#define FRAME_ID(classname)                                        \
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

  void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                           nsArenaMemoryStats* aArenaStats)
  {
    
    
    
    
    
    
    
    
    
    

    size_t mallocSize = SizeOfIncludingThisFromMalloc(aMallocSizeOf);
    EnumerateData data = { aArenaStats, 0 };
    mFreeLists.EnumerateEntries(FreeListEnumerator, &data);
    aArenaStats->mOther = mallocSize - data.total;
  }
};

void
nsPresArena::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                 nsArenaMemoryStats* aArenaStats)
{
  mState->SizeOfIncludingThis(aMallocSizeOf, aArenaStats);
}

#else




struct nsPresArena::State
{

  State()
  {
    PR_CallOnce(&ARENA_POISON_guard, ARENA_POISON_init);
  }

  void* Allocate(uint32_t , size_t aSize)
  {
    return moz_malloc(aSize);
  }

  void Free(uint32_t , void* aPtr)
  {
    moz_free(aPtr);
  }
};

void
nsPresArena::SizeOfExcludingThis(nsMallocSizeOfFun, nsArenaMemoryStats*)
{}

#endif 


nsPresArena::nsPresArena()
  : mState(new nsPresArena::State())
{}

nsPresArena::~nsPresArena()
{
  delete mState;
}

void*
nsPresArena::AllocateBySize(size_t aSize)
{
  return mState->Allocate(uint32_t(aSize) | uint32_t(NON_OBJECT_MARKER),
                          aSize);
}

void
nsPresArena::FreeBySize(size_t aSize, void* aPtr)
{
  mState->Free(uint32_t(aSize) | uint32_t(NON_OBJECT_MARKER), aPtr);
}

void*
nsPresArena::AllocateByFrameID(nsQueryFrame::FrameIID aID, size_t aSize)
{
  return mState->Allocate(aID, aSize);
}

void
nsPresArena::FreeByFrameID(nsQueryFrame::FrameIID aID, void* aPtr)
{
  mState->Free(aID, aPtr);
}

void*
nsPresArena::AllocateByObjectID(ObjectID aID, size_t aSize)
{
  return mState->Allocate(aID, aSize);
}

void
nsPresArena::FreeByObjectID(ObjectID aID, void* aPtr)
{
  mState->Free(aID, aPtr);
}

 uintptr_t
nsPresArena::GetPoisonValue()
{
  return ARENA_POISON;
}
