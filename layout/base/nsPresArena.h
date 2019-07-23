











































#ifndef nsPresArena_h___
#define nsPresArena_h___

#include "nscore.h"

class nsPresArena {
public:
  nsPresArena();
  ~nsPresArena();

  
  NS_HIDDEN_(void*) Allocate(size_t aSize);
  NS_HIDDEN_(void)  Free(size_t aSize, void* aPtr);

private:
  struct State;
  State* mState;
#ifdef DEBUG
  PRUint32 mAllocCount;
#endif
};

#endif
