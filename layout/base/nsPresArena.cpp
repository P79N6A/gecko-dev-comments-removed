













































#include "nsPresArena.h"
#include "nsCRT.h"
#include "nsDebug.h"
#include "prmem.h"





#ifndef DEBUG_TRACEMALLOC_PRESARENA





#define ALIGN_SHIFT 3
#define PL_ARENA_CONST_ALIGN_MASK ((PRUword(1) << ALIGN_SHIFT) - 1)
#include "plarena.h"


static const size_t MAX_RECYCLED_SIZE = 400;



static const size_t NUM_RECYCLERS = MAX_RECYCLED_SIZE >> ALIGN_SHIFT;


static const size_t ARENA_PAGE_SIZE = 4096;

struct nsPresArena::State {
  void*       mRecyclers[NUM_RECYCLERS];
  PLArenaPool mPool;

  State()
  {
    PL_INIT_ARENA_POOL(&mPool, "PresArena", ARENA_PAGE_SIZE);
    memset(mRecyclers, 0, sizeof(mRecyclers));
  }

  ~State()
  {
    PL_FinishArenaPool(&mPool);
  }

  void* Allocate(size_t aSize)
  {
    void* result = nsnull;

    
    aSize = PL_ARENA_ALIGN(&mPool, aSize);

    
    if (aSize <= MAX_RECYCLED_SIZE) {
      const size_t index = (aSize >> ALIGN_SHIFT) - 1;
      result = mRecyclers[index];
      if (result) {
        
        void* next = *((void**)result);
        mRecyclers[index] = next;
      }
    }

    if (!result) {
      
      PL_ARENA_ALLOCATE(result, &mPool, aSize);
    }

    return result;
  }

  void Free(size_t aSize, void* aPtr)
  {
    
    aSize = PL_ARENA_ALIGN(&mPool, aSize);

    
    if (aSize <= MAX_RECYCLED_SIZE) {
      const size_t index = (aSize >> ALIGN_SHIFT) - 1;
      void* currentTop = mRecyclers[index];
      mRecyclers[index] = aPtr;
      *((void**)aPtr) = currentTop;
    }
#if defined DEBUG_dbaron || defined DEBUG_zack
    else {
      fprintf(stderr,
              "WARNING: nsPresArena::FreeFrame leaking chunk of %lu bytes.\n",
              aSize);
    }
#endif
  }
};

#else


struct nsPresArena::State
{
  void* Allocate(size_t aSize)
  {
    return PR_Malloc(aSize);
  }

  void Free(size_t , void* aPtr)
  {
    PR_Free(aPtr);
  }
};

#endif 


nsPresArena::nsPresArena()
  : mState(new nsPresArena::State())
#ifdef DEBUG
  , mAllocCount(0)
#endif
{}

nsPresArena::~nsPresArena()
{
#ifdef DEBUG
  NS_ASSERTION(mAllocCount == 0,
               "Some PresArena objects were not freed");
#endif
  delete mState;
}

void*
nsPresArena::Allocate(size_t aSize)
{
  NS_ABORT_IF_FALSE(aSize > 0, "PresArena cannot allocate zero bytes");
  void* result = mState->Allocate(aSize);
#ifdef DEBUG
  if (result)
    mAllocCount++;
#endif
  return result;
}

void
nsPresArena::Free(size_t aSize, void* aPtr)
{
  NS_ABORT_IF_FALSE(aSize > 0, "PresArena cannot free zero bytes");
#ifdef DEBUG
  
  
  memset(aPtr, 0xdd, aSize);
  mAllocCount--;
#endif
  mState->Free(aSize, aPtr);
}
