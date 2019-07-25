





































#include "mozilla/Services.h"
#include "nsComponentManager.h"
#include "nsIIOService.h"
#include "nsIDirectoryService.h"
#include "nsIAccessibilityService.h"
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
  NS_COM already_AddRefed<TYPE>                                         \
  mozilla::services::Get##NAME()                                        \
  {                                                                     \
    if (!g##NAME) {                                                     \
      nsCOMPtr<TYPE> os = do_GetService(CONTRACT_ID);                   \
      g##NAME = os.forget().get();                                      \
    }                                                                   \
    NS_IF_ADDREF(g##NAME);                                              \
    return g##NAME;                                                     \
  }

#include "ServiceList.h"
#undef MOZ_SERVICE




void 
mozilla::services::Shutdown()
{
  gXPCOMShuttingDown = PR_TRUE;
#define MOZ_SERVICE(NAME, TYPE, CONTRACT_ID) NS_IF_RELEASE(g##NAME);
#include "ServiceList.h"
#undef MOZ_SERVICE
}
