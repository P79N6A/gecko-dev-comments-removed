





#ifndef __FFmpegRuntimeLinker_h__
#define __FFmpegRuntimeLinker_h__

extern "C" {
#pragma GCC visibility push(default)
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#pragma GCC visibility pop
}

#include "nsAutoPtr.h"

namespace mozilla
{

class FFmpegRuntimeLinker
{
public:
  static bool Link();
  static void Unlink();

private:
  static void* sLinkedLibs[];

  static enum LinkStatus {
    LinkStatus_INIT = 0,
    LinkStatus_FAILED,
    LinkStatus_SUCCEEDED
  } sLinkStatus;
};

#define AV_FUNC(lib, func) extern typeof(func)* func;
#include "FFmpegFunctionList.h"
#undef AV_FUNC
}

#endif 
