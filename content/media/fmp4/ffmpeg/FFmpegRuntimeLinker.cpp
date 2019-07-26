





#include <dlfcn.h>

#include "nsDebug.h"

#include "FFmpegRuntimeLinker.h"


#include "FFmpegDecoderModule.h"

#define NUM_ELEMENTS(X) (sizeof(X) / sizeof((X)[0]))

#define LIBAVCODEC 0
#define LIBAVFORMAT 1
#define LIBAVUTIL 2

namespace mozilla
{

FFmpegRuntimeLinker::LinkStatus FFmpegRuntimeLinker::sLinkStatus =
  LinkStatus_INIT;

static const char * const sLibNames[] = {
  "libavcodec.so.53", "libavformat.so.53", "libavutil.so.51",
};

void* FFmpegRuntimeLinker::sLinkedLibs[NUM_ELEMENTS(sLibNames)] = {
  nullptr, nullptr, nullptr
};

#define AV_FUNC(lib, func) typeof(func) func;
#include "FFmpegFunctionList.h"
#undef AV_FUNC

 bool
FFmpegRuntimeLinker::Link()
{
  if (sLinkStatus) {
    return sLinkStatus == LinkStatus_SUCCEEDED;
  }

  for (uint32_t i = 0; i < NUM_ELEMENTS(sLinkedLibs); i++) {
    if (!(sLinkedLibs[i] = dlopen(sLibNames[i], RTLD_NOW | RTLD_LOCAL))) {
      NS_WARNING("Couldn't link ffmpeg libraries.");
      goto fail;
    }
  }

#define AV_FUNC(lib, func)                                                     \
  func = (typeof(func))dlsym(sLinkedLibs[lib], #func);                         \
  if (!func) {                                                                 \
    NS_WARNING("Couldn't load FFmpeg function " #func ".");                    \
    goto fail;                                                                 \
  }
#include "FFmpegFunctionList.h"
#undef AV_FUNC

  sLinkStatus = LinkStatus_SUCCEEDED;
  return true;

fail:
  Unlink();

  sLinkStatus = LinkStatus_FAILED;
  return false;
}

 void
FFmpegRuntimeLinker::Unlink()
{
  FFMPEG_LOG("Unlinking ffmpeg libraries.");
  for (uint32_t i = 0; i < NUM_ELEMENTS(sLinkedLibs); i++) {
    if (sLinkedLibs[i]) {
      dlclose(sLinkedLibs[i]);
      sLinkedLibs[i] = nullptr;
    }
  }
}

} 
