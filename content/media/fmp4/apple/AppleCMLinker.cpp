





#include <dlfcn.h>

#include "AppleCMLinker.h"
#include "MainThreadUtils.h"
#include "mozilla/ArrayUtils.h"
#include "nsCocoaFeatures.h"
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

#define LINK_FUNC(func) typeof(CM ## func) CM ## func;
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

  const char* dlnames[] =
    { "/System/Library/Frameworks/CoreMedia.framework/CoreMedia",
      "/System/Library/PrivateFrameworks/CoreMedia.framework/CoreMedia" };
  bool dlfound = false;
  for (size_t i = 0; i < ArrayLength(dlnames); i++) {
    if ((sLink = dlopen(dlnames[i], RTLD_NOW | RTLD_LOCAL))) {
      dlfound = true;
      break;
    }
  }
  if (!dlfound) {
    NS_WARNING("Couldn't load CoreMedia framework");
    goto fail;
  }

  if (nsCocoaFeatures::OnLionOrLater()) {
#define LINK_FUNC2(func)                                       \
  func = (typeof(func))dlsym(sLink, #func);                    \
  if (!func) {                                                 \
    NS_WARNING("Couldn't load CoreMedia function " #func );    \
    goto fail;                                                 \
  }
#define LINK_FUNC(func) LINK_FUNC2(CM ## func)
#include "AppleCMFunctions.h"
#undef LINK_FUNC
#undef LINK_FUNC2

    skPropExtensionAtoms =
      GetIOConst("kCMFormatDescriptionExtension_SampleDescriptionExtensionAtoms");

    skPropFullRangeVideo =
      GetIOConst("kCMFormatDescriptionExtension_FullRangeVideo");

  } else {
#define LINK_FUNC2(cm, fig)                                    \
  cm = (typeof(cm))dlsym(sLink, #fig);                         \
  if (!cm) {                                                   \
    NS_WARNING("Couldn't load CoreMedia function " #fig );     \
    goto fail;                                                 \
  }
#define LINK_FUNC(func) LINK_FUNC2(CM ## func, Fig ## func)
#include "AppleCMFunctions.h"
#undef LINK_FUNC
#undef LINK_FUNC2

    skPropExtensionAtoms =
      GetIOConst("kFigFormatDescriptionExtension_SampleDescriptionExtensionAtoms");
  }

  if (!skPropExtensionAtoms) {
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
    sLinkStatus = LinkStatus_INIT;
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
