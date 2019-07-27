





#ifndef AppleVDALinker_h
#define AppleVDALinker_h

extern "C" {
#pragma GCC visibility push(default)
#include "VideoDecodeAcceleration/VDADecoder.h"
#pragma GCC visibility pop
}

#include "nscore.h"

namespace mozilla {

class AppleVDALinker
{
public:
  static bool Link();
  static void Unlink();
  static CFStringRef skPropWidth;
  static CFStringRef skPropHeight;
  static CFStringRef skPropSourceFormat;
  static CFStringRef skPropAVCCData;

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
#include "AppleVDAFunctions.h"
#undef LINK_FUNC

} 

#endif 
