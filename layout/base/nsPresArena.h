






#ifndef nsPresArena_h___
#define nsPresArena_h___

#include "nscore.h"
#include "nsQueryFrame.h"

#include "mozilla/StandardInteger.h"

struct nsArenaMemoryStats;







#ifdef DEBUG_TRACEMALLOC_PRESARENA
#define PRESARENA_MUST_FREE_DURING_DESTROY true
#else
#define PRESARENA_MUST_FREE_DURING_DESTROY false
#endif

class nsPresArena {
public:
  nsPresArena();
  ~nsPresArena();

  
  NS_HIDDEN_(void*) AllocateBySize(size_t aSize);
  NS_HIDDEN_(void)  FreeBySize(size_t aSize, void* aPtr);

  
  
  NS_HIDDEN_(void*) AllocateByFrameID(nsQueryFrame::FrameIID aID, size_t aSize);
  NS_HIDDEN_(void)  FreeByFrameID(nsQueryFrame::FrameIID aID, void* aPtr);

  enum ObjectID {
    nsLineBox_id = nsQueryFrame::NON_FRAME_MARKER,
    nsRuleNode_id,
    nsStyleContext_id,
    nsFrameList_id,

    
    
    
    
    
    
    NON_OBJECT_MARKER = 0x40000000
  };

  
  
  NS_HIDDEN_(void*) AllocateByObjectID(ObjectID aID, size_t aSize);
  NS_HIDDEN_(void)  FreeByObjectID(ObjectID aID, void* aPtr);

  



  void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                           nsArenaMemoryStats* aArenaStats);

  






  static uintptr_t GetPoisonValue();

private:
  struct State;
  State* mState;
};

#endif
