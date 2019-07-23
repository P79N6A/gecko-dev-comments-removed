













































#include "nsPresArena.h"
#include "nsCRT.h"
#include "nsDebug.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "prmem.h"
#include "prinit.h"
#include "prlog.h"





#define ALIGN_SHIFT 3
#define PL_ARENA_CONST_ALIGN_MASK ((PRUword(1) << ALIGN_SHIFT) - 1)
#include "plarena.h"

#ifdef _WIN32
# include <windows.h>
#else
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

#ifndef DEBUG_TRACEMALLOC_PRESARENA


static const size_t ARENA_PAGE_SIZE = 4096;









#ifdef _WIN32
static void *
ReserveRegion(PRUword region, PRUword size)
{
  return VirtualAlloc((void *)region, size, MEM_RESERVE, PAGE_NOACCESS);
}

static void
ReleaseRegion(void *region, PRUword size)
{
  VirtualFree(region, size, MEM_RELEASE);
}

static bool
ProbeRegion(PRUword region, PRUword size)
{
  SYSTEM_INFO sinfo;
  GetSystemInfo(&sinfo);
  if (region >= (PRUword)sinfo.lpMaximumApplicationAddress &&
      region + size >= (PRUword)sinfo.lpMaximumApplicationAddress) {
    return true;
  } else {
    return false;
  }
}

static PRUword
GetDesiredRegionSize()
{
  SYSTEM_INFO sinfo;
  GetSystemInfo(&sinfo);
  return sinfo.dwAllocationGranularity;
}

#define RESERVE_FAILED 0

#else 

static void *
ReserveRegion(PRUword region, PRUword size)
{
  return mmap((caddr_t)region, size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
}

static void
ReleaseRegion(void *region, PRUword size)
{
  munmap((caddr_t)region, size);
}

static bool
ProbeRegion(PRUword region, PRUword size)
{
  if (madvise((caddr_t)region, size, MADV_NORMAL)) {
    return true;
  } else {
    return false;
  }
}

static PRUword
GetDesiredRegionSize()
{
  return sysconf(_SC_PAGESIZE);
}

#define RESERVE_FAILED MAP_FAILED

#endif 

static PRUword ARENA_POISON;
static PRCallOnceType ARENA_POISON_guard;

PR_STATIC_ASSERT(sizeof(PRUword) == 4 || sizeof(PRUword) == 8);
PR_STATIC_ASSERT(sizeof(PRUword) == sizeof(void *));

static PRStatus
ARENA_POISON_init()
{
  PRUword rgnsize = GetDesiredRegionSize();

  if (sizeof(PRUword) == 8) {
    
    
    
    ARENA_POISON =
      (((PRUword(0x7FFFFFFFu) << 31) << 1 | PRUword(0xF0DEAFFFu))
       & ~(rgnsize-1))
      + rgnsize/2 - 1;
    return PR_SUCCESS;

  } else {
    
    
    PRUword candidate = (0xF0DEAFFF & ~(rgnsize-1));
    PRUword step = rgnsize;
    int direction = +1;
    PRUword limit = candidate + 1024*rgnsize;
    while (candidate < limit) {
      void *result = ReserveRegion(candidate, rgnsize);
      if (result == (void *)candidate) {
        
        ARENA_POISON = candidate + rgnsize/2 - 1;
        return PR_SUCCESS;

      } else {
        if (result != RESERVE_FAILED)
          ReleaseRegion(result, rgnsize);

        if (ProbeRegion(candidate, rgnsize)) {
          
          ARENA_POISON = candidate + rgnsize/2 - 1;
          return PR_SUCCESS;
        }
      }

      candidate += step*direction;
      step = step + rgnsize;
      direction = -direction;
    }

    NS_RUNTIMEABORT("no usable poison region identified");
    return PR_FAILURE;
  }
}





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
  

  PRBool KeyEquals(KeyTypePointer const aKey) const
  { return mKey == aKey; }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  { return NS_INT32_TO_PTR(aKey); }

  static PLDHashNumber HashKey(KeyTypePointer aKey)
  { return NS_PTR_TO_INT32(aKey); }

  enum { ALLOW_MEMMOVE = PR_FALSE };
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
        for (; p < limit; p += sizeof(PRUword)) {
          NS_ABORT_IF_FALSE(*reinterpret_cast<PRUword*>(p) == ARENA_POISON,
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
    for (; p < limit; p += sizeof(PRUword)) {
      *reinterpret_cast<PRUword*>(p) = ARENA_POISON;
    }

    list->mEntries.AppendElement(aPtr);
  }
};

#else



struct nsPresArena::State
{
  void* Allocate(PRUint32 , size_t aSize)
  {
    return PR_Malloc(aSize);
  }

  void Free(PRUint32 , void* aPtr)
  {
    PR_Free(aPtr);
  }
};

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
