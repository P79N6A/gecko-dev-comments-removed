





#include <dlfcn.h>

#include "AppleVTLinker.h"
#include "MainThreadUtils.h"
#include "mozilla/ArrayUtils.h"
#include "nsDebug.h"

#ifdef PR_LOGGING
PRLogModuleInfo* GetAppleMediaLog();
#define LOG(...) PR_LOG(GetAppleMediaLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

namespace mozilla {

AppleVTLinker::LinkStatus
AppleVTLinker::sLinkStatus = LinkStatus_INIT;

void* AppleVTLinker::sLink = nullptr;
nsrefcnt AppleVTLinker::sRefCount = 0;
CFStringRef AppleVTLinker::skPropEnableHWAccel = nullptr;
CFStringRef AppleVTLinker::skPropUsingHWAccel = nullptr;

#define LINK_FUNC(func) typeof(func) func;
#include "AppleVTFunctions.h"
#undef LINK_FUNC

 bool
AppleVTLinker::Link()
{
  
  
  
  MOZ_ASSERT(NS_IsMainThread());
  ++sRefCount;

  if (sLinkStatus) {
    return sLinkStatus == LinkStatus_SUCCEEDED;
  }

  const char* dlnames[] =
    { "/System/Library/Frameworks/VideoToolbox.framework/VideoToolbox",
      "/System/Library/PrivateFrameworks/VideoToolbox.framework/VideoToolbox" };
  bool dlfound = false;
  for (size_t i = 0; i < ArrayLength(dlnames); i++) {
    if ((sLink = dlopen(dlnames[i], RTLD_NOW | RTLD_LOCAL))) {
      dlfound = true;
      break;
    }
  }
  if (!dlfound) {
    NS_WARNING("Couldn't load VideoToolbox framework");
    goto fail;
  }

#define LINK_FUNC(func)                                        \
  func = (typeof(func))dlsym(sLink, #func);                    \
  if (!func) {                                                 \
    NS_WARNING("Couldn't load VideoToolbox function " #func ); \
    goto fail;                                                 \
  }
#include "AppleVTFunctions.h"
#undef LINK_FUNC

  
  skPropEnableHWAccel =
    GetIOConst("kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder");
  skPropUsingHWAccel =
    GetIOConst("kVTDecompressionPropertyKey_UsingHardwareAcceleratedVideoDecoder");

  LOG("Loaded VideoToolbox framework.");
  sLinkStatus = LinkStatus_SUCCEEDED;
  return true;

fail:
  Unlink();

  sLinkStatus = LinkStatus_FAILED;
  return false;
}

 void
AppleVTLinker::Unlink()
{
  
  
  
  
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sRefCount > 0, "Unbalanced Unlink()");
  --sRefCount;
  if (sLink && sRefCount < 1) {
    LOG("Unlinking VideoToolbox framework.");
    dlclose(sLink);
    sLink = nullptr;
    skPropEnableHWAccel = nullptr;
    skPropUsingHWAccel = nullptr;
    sLinkStatus = LinkStatus_INIT;
  }
}

 CFStringRef
AppleVTLinker::GetIOConst(const char* symbol)
{
  CFStringRef* address = (CFStringRef*)dlsym(sLink, symbol);
  if (!address) {
    return nullptr;
  }

  return *address;
}

} 
