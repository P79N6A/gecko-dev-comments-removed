



#ifndef nsArenaMemoryStats_h
#define nsArenaMemoryStats_h

#define FRAME_ID_STAT_FIELD(classname) mArena##classname

struct nsArenaMemoryStats {
#define FRAME_ID(classname) size_t FRAME_ID_STAT_FIELD(classname);
#include "nsFrameIdList.h"
#undef FRAME_ID
  size_t mOther;
};

#endif 
