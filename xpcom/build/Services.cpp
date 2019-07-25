





































#include "mozilla/Services.h"
#include "nsComponentManager.h"
#include "nsIIOService.h"
#include "nsIDirectoryService.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIChromeRegistry.h"
#include "nsIObserverService.h"
#include "nsNetCID.h"
#include "nsObserverService.h"
#include "nsXPCOMPrivate.h"
#include "nsIStringBundle.h"
#include "nsIToolkitChromeRegistry.h"
#include "nsIXULOverlayProvider.h"
#include "IHistory.h"
#include "nsIXPConnect.h"

using namespace mozilla;
using namespace mozilla::services;





#define MOZ_SERVICE(NAME, TYPE, CONTRACT_ID)                            \
  static TYPE* g##NAME = nsnull;                                        \
                                                                        \
  already_AddRefed<TYPE>                                                \
  mozilla::services::Get##NAME()                                        \
  {                                                                     \
    if (!g##NAME) {                                                     \
      nsCOMPtr<TYPE> os = do_GetService(CONTRACT_ID);                   \
      g##NAME = os.forget().get();                                      \
    }                                                                   \
    NS_IF_ADDREF(g##NAME);                                              \
    return g##NAME;                                                     \
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
