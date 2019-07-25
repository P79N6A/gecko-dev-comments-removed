













































#include "nsPresArena.h"
#include "nsCRT.h"
#include "nsDebug.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "prmem.h"
#include "prinit.h"
#include "prlog.h"

#ifdef MOZ_CRASHREPORTER
#include "nsICrashReporter.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsPrintfCString.h"
#endif

#include "mozilla/StandardInteger.h"





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
  return mmap((caddr_t)region, size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
}

static void
ReleaseRegion(void *region, uintptr_t size)
{
  munmap((caddr_t)region, size);
}

static bool
ProbeRegion(uintptr_t region, uintptr_t size)
{
  if (madvise((caddr_t)region, size, MADV_NORMAL)) {
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
                            nsPrintfCString(17, "%.16llx", PRUint64(rgnbase)));
    cr->AnnotateCrashReport(NS_LITERAL_CSTRING("FramePoisonSize"),
                            nsPrintfCString("%lu", PRUint32(rgnsize)));
  }
#endif
  return PR_SUCCESS;
}

#ifndef DEBUG_TRACEMALLOC_PRESARENA




namespace {

class FreeList : public PLDHashEntryHdr
{
public:
  typedef PRUint32 KeyType;
  nsTArray<void *> mEntries;
  size_t mEntrySize;

protected:
  typedef const void* KeyTypePointer;
  KeyTypePointer mKey;

  FreeList(KeyTypePointer aKey) : mEntrySize(0), mKey(aKey) {}
  

  bool KeyEquals(KeyTypePointer const aKey) const
  { return mKey == aKey; }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  { return NS_INT32_TO_PTR(aKey); }

  static PLDHashNumber HashKey(KeyTypePointer aKey)
  { return NS_PTR_TO_INT32(aKey); }

  enum { ALLOW_MEMMOVE = false };
  friend class nsTHashtable<FreeList>;
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

  ~State()
  {
    PL_FinishArenaPool(&mPool);
  }

  void* Allocate(PRUint32 aCode, size_t aSize)
  {
    NS_ABORT_IF_FALSE(aSize > 0, "PresArena cannot allocate zero bytes");

    
    aSize = PL_ARENA_ALIGN(&mPool, aSize);

    
    
    FreeList* list = mFreeLists.PutEntry(aCode);
    if (!list) {
      return nsnull;
    }

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
#ifdef DEBUG
      {
        char* p = reinterpret_cast<char*>(result);
        char* limit = p + list->mEntrySize;
        for (; p < limit; p += sizeof(uintptr_t)) {
          NS_ABORT_IF_FALSE(*reinterpret_cast<uintptr_t*>(p) == ARENA_POISON,
                            "PresArena: poison overwritten");
        }
      }
#endif
      return result;
    }

    
    PL_ARENA_ALLOCATE(result, &mPool, aSize);
    return result;
  }

  void Free(PRUint32 aCode, void* aPtr)
  {
    
    FreeList* list = mFreeLists.GetEntry(aCode);
    NS_ABORT_IF_FALSE(list, "no free list for pres arena object");
    NS_ABORT_IF_FALSE(list->mEntrySize > 0, "PresArena cannot free zero bytes");

    char* p = reinterpret_cast<char*>(aPtr);
    char* limit = p + list->mEntrySize;
    for (; p < limit; p += sizeof(uintptr_t)) {
      *reinterpret_cast<uintptr_t*>(p) = ARENA_POISON;
    }

    list->mEntries.AppendElement(aPtr);
  }

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
  {
    size_t n = aMallocSizeOf(this);

    
    
    
    const PLArena *arena = mPool.first.next;
    while (arena) {
      n += aMallocSizeOf(arena);
      arena = arena->next;
    }
    return n;
  }
};

size_t
nsPresArena::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return mState ? mState->SizeOfIncludingThis(aMallocSizeOf) : 0;
}

#else




struct nsPresArena::State
{

  State()
  {
    PR_CallOnce(&ARENA_POISON_guard, ARENA_POISON_init);
  }

  void* Allocate(PRUint32 , size_t aSize)
  {
    return PR_Malloc(aSize);
  }

  void Free(PRUint32 , void* aPtr)
  {
    PR_Free(aPtr);
  }
};

size_t
nsPresArena::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return 0;
}

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
  return mState->Allocate(PRUint32(aSize) |
                          PRUint32(nsQueryFrame::NON_FRAME_MARKER),
                          aSize);
}

void
nsPresArena::FreeBySize(size_t aSize, void* aPtr)
{
  mState->Free(PRUint32(aSize) |
               PRUint32(nsQueryFrame::NON_FRAME_MARKER), aPtr);
}

void*
nsPresArena::AllocateByCode(nsQueryFrame::FrameIID aCode, size_t aSize)
{
  return mState->Allocate(aCode, aSize);
}

void
nsPresArena::FreeByCode(nsQueryFrame::FrameIID aCode, void* aPtr)
{
  mState->Free(aCode, aPtr);
}

 uintptr_t
nsPresArena::GetPoisonValue()
{
  return ARENA_POISON;
}
