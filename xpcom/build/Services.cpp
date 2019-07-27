





#include "mozilla/Likely.h"
#include "mozilla/Services.h"
#include "nsComponentManager.h"
#include "nsIObserverService.h"
#include "nsNetCID.h"
#include "nsObserverService.h"
#include "nsXPCOMPrivate.h"
#if !defined(MOZILLA_XPCOMRT_API)
#include "nsIIOService.h"
#include "nsIDirectoryService.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIChromeRegistry.h"
#include "nsIStringBundle.h"
#include "nsIToolkitChromeRegistry.h"
#include "nsIXULOverlayProvider.h"
#include "IHistory.h"
#include "nsIXPConnect.h"
#include "inIDOMUtils.h"
#include "nsIPermissionManager.h"
#include "nsIServiceWorkerManager.h"
#include "nsIAsyncShutdown.h"
#include "nsIUUIDGenerator.h"
#include "nsIGfxInfo.h"
#endif 

using namespace mozilla;
using namespace mozilla::services;





#define MOZ_SERVICE(NAME, TYPE, CONTRACT_ID)                            \
  static TYPE* g##NAME = nullptr;                                       \
                                                                        \
  already_AddRefed<TYPE>                                                \
  mozilla::services::Get##NAME()                                        \
  {                                                                     \
    if (MOZ_UNLIKELY(gXPCOMShuttingDown)) {                             \
      return nullptr;                                                   \
    }                                                                   \
    if (!g##NAME) {                                                     \
      nsCOMPtr<TYPE> os = do_GetService(CONTRACT_ID);                   \
      os.swap(g##NAME);                                                 \
    }                                                                   \
    nsCOMPtr<TYPE> ret = g##NAME;                                       \
    return ret.forget();                                                \
  }                                                                     \
  NS_EXPORT_(already_AddRefed<TYPE>)                                    \
  mozilla::services::_external_Get##NAME()                              \
  {                                                                     \
    return Get##NAME();                                                 \
  }

#include "ServiceList.h"
#undef MOZ_SERVICE




void
mozilla::services::Shutdown()
{
  gXPCOMShuttingDown = true;
#define MOZ_SERVICE(NAME, TYPE, CONTRACT_ID) NS_IF_RELEASE(g##NAME);
#include "ServiceList.h"
#undef MOZ_SERVICE
}
