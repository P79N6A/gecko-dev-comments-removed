





#include <dlfcn.h>

#include "AppleVDALinker.h"
#include "MainThreadUtils.h"
#include "nsDebug.h"

#ifdef PR_LOGGING
PRLogModuleInfo* GetAppleMediaLog();
#define LOG(...) PR_LOG(GetAppleMediaLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

namespace mozilla {

AppleVDALinker::LinkStatus
AppleVDALinker::sLinkStatus = LinkStatus_INIT;

void* AppleVDALinker::sLink = nullptr;
nsrefcnt AppleVDALinker::sRefCount = 0;
CFStringRef AppleVDALinker::skPropWidth = nullptr;
CFStringRef AppleVDALinker::skPropHeight = nullptr;
CFStringRef AppleVDALinker::skPropSourceFormat = nullptr;
CFStringRef AppleVDALinker::skPropAVCCData = nullptr;

#define LINK_FUNC(func) typeof(func) func;
#include "AppleVDAFunctions.h"
#undef LINK_FUNC

 bool
AppleVDALinker::Link()
{
  
  
  
  MOZ_ASSERT(NS_IsMainThread());
  ++sRefCount;

  if (sLinkStatus) {
    return sLinkStatus == LinkStatus_SUCCEEDED;
  }

  const char* dlname =
    "/System/Library/Frameworks/VideoDecodeAcceleration.framework/VideoDecodeAcceleration";

  if (!(sLink = dlopen(dlname, RTLD_NOW | RTLD_LOCAL))) {
    NS_WARNING("Couldn't load VideoDecodeAcceleration framework");
    goto fail;
  }

#define LINK_FUNC(func)                                                   \
  func = (typeof(func))dlsym(sLink, #func);                               \
  if (!func) {                                                            \
    NS_WARNING("Couldn't load VideoDecodeAcceleration function " #func ); \
    goto fail;                                                            \
  }
#include "AppleVDAFunctions.h"
#undef LINK_FUNC

  skPropWidth = GetIOConst("kVDADecoderConfiguration_Width");
  skPropHeight = GetIOConst("kVDADecoderConfiguration_Height");
  skPropSourceFormat = GetIOConst("kVDADecoderConfiguration_SourceFormat");
  skPropAVCCData = GetIOConst("kVDADecoderConfiguration_avcCData");

  if (!skPropWidth || !skPropHeight || !skPropSourceFormat || !skPropAVCCData) {
    goto fail;
  }

  LOG("Loaded VideoDecodeAcceleration framework.");
  sLinkStatus = LinkStatus_SUCCEEDED;
  return true;

fail:
  Unlink();

  sLinkStatus = LinkStatus_FAILED;
  return false;
}

 void
AppleVDALinker::Unlink()
{
  
  
  
  
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sRefCount > 0, "Unbalanced Unlink()");
  --sRefCount;
  if (sLink && sRefCount < 1) {
    LOG("Unlinking VideoToolbox framework.");
    dlclose(sLink);
    sLink = nullptr;
    skPropWidth = nullptr;
    skPropHeight = nullptr;
    skPropSourceFormat = nullptr;
    skPropAVCCData = nullptr;
  }
}

 CFStringRef
AppleVDALinker::GetIOConst(const char* symbol)
{
  CFStringRef* address = (CFStringRef*)dlsym(sLink, symbol);
  if (!address) {
    return nullptr;
  }

  return *address;
}

} 
