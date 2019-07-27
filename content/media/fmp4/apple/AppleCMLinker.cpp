





#include <dlfcn.h>

#include "AppleCMLinker.h"
#include "MainThreadUtils.h"
#include "nsDebug.h"

#ifdef PR_LOGGING
PRLogModuleInfo* GetAppleMediaLog();
#define LOG(...) PR_LOG(GetAppleMediaLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

namespace mozilla {

AppleCMLinker::LinkStatus
AppleCMLinker::sLinkStatus = LinkStatus_INIT;

void* AppleCMLinker::sLink = nullptr;
nsrefcnt AppleCMLinker::sRefCount = 0;
CFStringRef AppleCMLinker::skPropExtensionAtoms = nullptr;
CFStringRef AppleCMLinker::skPropFullRangeVideo = nullptr;

#define LINK_FUNC(func) typeof(func) func;
#include "AppleCMFunctions.h"
#undef LINK_FUNC

 bool
AppleCMLinker::Link()
{
  
  
  
  MOZ_ASSERT(NS_IsMainThread());
  ++sRefCount;

  if (sLinkStatus) {
    return sLinkStatus == LinkStatus_SUCCEEDED;
  }

  const char* dlname =
    "/System/Library/Frameworks/CoreMedia.framework/CoreMedia";
  if (!(sLink = dlopen(dlname, RTLD_NOW | RTLD_LOCAL))) {
    NS_WARNING("Couldn't load CoreMedia framework");
    goto fail;
  }

#define LINK_FUNC(func)                                        \
  func = (typeof(func))dlsym(sLink, #func);                    \
  if (!func) {                                                 \
    NS_WARNING("Couldn't load CoreMedia function " #func ); \
    goto fail;                                                 \
  }
#include "AppleCMFunctions.h"
#undef LINK_FUNC

  
  skPropExtensionAtoms =
    GetIOConst("kCMFormatDescriptionExtension_SampleDescriptionExtensionAtoms");

  skPropFullRangeVideo =
    GetIOConst("kCMFormatDescriptionExtension_FullRangeVideo");

  if (!skPropExtensionAtoms || !skPropFullRangeVideo) {
    goto fail;
  }

  LOG("Loaded CoreMedia framework.");
  sLinkStatus = LinkStatus_SUCCEEDED;
  return true;

fail:
  Unlink();

  sLinkStatus = LinkStatus_FAILED;
  return false;
}

 void
AppleCMLinker::Unlink()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sRefCount > 0, "Unbalanced Unlink()");
  --sRefCount;
  if (sLink && sRefCount < 1) {
    LOG("Unlinking CoreMedia framework.");
    dlclose(sLink);
    sLink = nullptr;
  }
}

 CFStringRef
AppleCMLinker::GetIOConst(const char* symbol)
{
  CFStringRef* address = (CFStringRef*)dlsym(sLink, symbol);
  if (!address) {
    return nullptr;
  }

  return *address;
}

} 
