





#ifndef AppleCMLinker_h
#define AppleCMLinker_h

extern "C" {
#pragma GCC visibility push(default)
#include <CoreMedia/CoreMedia.h>
#pragma GCC visibility pop
}

#include "nscore.h"

namespace mozilla {

class AppleCMLinker
{
public:
  static bool Link();
  static void Unlink();
  static CFStringRef skPropExtensionAtoms;
  static CFStringRef skPropFullRangeVideo;

private:
  static void* sLink;
  static nsrefcnt sRefCount;

  static enum LinkStatus {
    LinkStatus_INIT = 0,
    LinkStatus_FAILED,
    LinkStatus_SUCCEEDED
  } sLinkStatus;

  static CFStringRef GetIOConst(const char* symbol);
};

#define LINK_FUNC(func) extern typeof(func)* func;
#include "AppleCMFunctions.h"
#undef LINK_FUNC

} 

#endif 
