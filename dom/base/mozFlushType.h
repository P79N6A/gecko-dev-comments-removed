




#ifndef mozFlushType_h___
#define mozFlushType_h___








enum mozFlushType {
  Flush_None             = 0, 
  Flush_Content          = 1, 
  Flush_ContentAndNotify = 2, 


  Flush_Style            = 3, 
  Flush_Frames           = Flush_Style,
  Flush_InterruptibleLayout = 4, 


  Flush_Layout           = 5, 

  Flush_Display          = 6  
};

namespace mozilla {

struct ChangesToFlush {
  ChangesToFlush(mozFlushType aFlushType, bool aFlushAnimations)
    : mFlushType(aFlushType)
    , mFlushAnimations(aFlushAnimations)
  {}

  mozFlushType mFlushType;
  bool mFlushAnimations;
};

} 

#endif 
