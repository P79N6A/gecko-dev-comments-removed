





#ifndef AppleVTLinker_h
#define AppleVTLinker_h

extern "C" {
#pragma GCC visibility push(default)
#include "VideoToolbox/VideoToolbox.h"
#pragma GCC visibility pop
}

#include "nscore.h"

namespace mozilla {

class AppleVTLinker
{
public:
  static bool Link();
  static void Unlink();

private:
  static void* sLink;
  static nsrefcnt sRefCount;

  static enum LinkStatus {
    LinkStatus_INIT = 0,
    LinkStatus_FAILED,
    LinkStatus_SUCCEEDED
  } sLinkStatus;
};

#define LINK_FUNC(func) extern typeof(func)* func;
#include "AppleVTFunctions.h"
#undef LINK_FUNC

} 

#endif 
