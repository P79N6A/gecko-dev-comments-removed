











































#ifndef nsPresArena_h___
#define nsPresArena_h___

#include "nscore.h"
#include "nsQueryFrame.h"







#ifdef DEBUG_TRACEMALLOC_PRESARENA
#define PRESARENA_MUST_FREE_DURING_DESTROY PR_TRUE
#else
#define PRESARENA_MUST_FREE_DURING_DESTROY PR_FALSE
#endif

class nsPresArena {
public:
  nsPresArena();
  ~nsPresArena();

  
  NS_HIDDEN_(void*) AllocateBySize(size_t aSize);
  NS_HIDDEN_(void)  FreeBySize(size_t aSize, void* aPtr);

  
  
  NS_HIDDEN_(void*) AllocateByCode(nsQueryFrame::FrameIID aCode, size_t aSize);
  NS_HIDDEN_(void)  FreeByCode(nsQueryFrame::FrameIID aCode, void* aPtr);

private:
  struct State;
  State* mState;
};

#endif
