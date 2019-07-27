





#include <dlfcn.h>

#include "FFmpegRuntimeLinker.h"
#include "mozilla/ArrayUtils.h"
#include "FFmpegLog.h"

#define NUM_ELEMENTS(X) (sizeof(X) / sizeof((X)[0]))

namespace mozilla
{

FFmpegRuntimeLinker::LinkStatus FFmpegRuntimeLinker::sLinkStatus =
  LinkStatus_INIT;

struct AvFormatLib
{
  const char* Name;
  PlatformDecoderModule* (*Factory)();
  uint32_t Version;
};

template <int V> class FFmpegDecoderModule
{
public:
  static PlatformDecoderModule* Create();
};

static const AvFormatLib sLibs[] = {
  { "libavformat.so.55", FFmpegDecoderModule<55>::Create, 55 },
  { "libavformat.so.54", FFmpegDecoderModule<54>::Create, 54 },
  { "libavformat.so.53", FFmpegDecoderModule<53>::Create, 53 },
};

void* FFmpegRuntimeLinker::sLinkedLib = nullptr;
const AvFormatLib* FFmpegRuntimeLinker::sLib = nullptr;

#define AV_FUNC(func, ver) void (*func)();
#define LIBAVCODEC_ALLVERSION
#include "FFmpegFunctionList.h"
#undef LIBAVCODEC_ALLVERSION
#undef AV_FUNC

 bool
FFmpegRuntimeLinker::Link()
{
  if (sLinkStatus) {
    return sLinkStatus == LinkStatus_SUCCEEDED;
  }

  for (size_t i = 0; i < ArrayLength(sLibs); i++) {
    const AvFormatLib* lib = &sLibs[i];
    sLinkedLib = dlopen(lib->Name, RTLD_NOW | RTLD_LOCAL);
    if (sLinkedLib) {
      if (Bind(lib->Name, lib->Version)) {
        sLib = lib;
        sLinkStatus = LinkStatus_SUCCEEDED;
        return true;
      }
      
      Unlink();
    }
  }

  FFMPEG_LOG("H264/AAC codecs unsupported without [");
  for (size_t i = 0; i < ArrayLength(sLibs); i++) {
    FFMPEG_LOG("%s %s", i ? "," : "", sLibs[i].Name);
  }
  FFMPEG_LOG(" ]\n");

  Unlink();

  sLinkStatus = LinkStatus_FAILED;
  return false;
}

 bool
FFmpegRuntimeLinker::Bind(const char* aLibName, uint32_t Version)
{
#define LIBAVCODEC_ALLVERSION
#define AV_FUNC(func, ver)                                                     \
  if (ver == 0 || ver == Version) {                                            \
    if (!(func = (typeof(func))dlsym(sLinkedLib, #func))) {                    \
      FFMPEG_LOG("Couldn't load function " #func " from %s.", aLibName);       \
      return false;                                                            \
    }                                                                          \
  }
#include "FFmpegFunctionList.h"
#undef AV_FUNC
#undef LIBAVCODEC_ALLVERSION
  return true;
}

 PlatformDecoderModule*
FFmpegRuntimeLinker::CreateDecoderModule()
{
  PlatformDecoderModule* module = sLib->Factory();
  return module;
}

 void
FFmpegRuntimeLinker::Unlink()
{
  if (sLinkedLib) {
    dlclose(sLinkedLib);
    sLinkedLib = nullptr;
    sLib = nullptr;
    sLinkStatus = LinkStatus_INIT;
  }
}

} 
