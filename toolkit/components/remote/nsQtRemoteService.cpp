








































#include "nsQtRemoteService.h"

#include <X11/Xatom.h> 
#include <stdlib.h>

#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsPIDOMWindow.h"
#include "nsIGenericFactory.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsIWeakReference.h"
#include "nsIWidget.h"
#include "nsIAppShellService.h"
#include "nsAppShellCID.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "prprf.h"
#include "prenv.h"
#include "nsCRT.h"

#ifdef MOZ_WIDGET_GTK2

#endif

#ifdef MOZ_XUL_APP
#include "nsICommandLineRunner.h"
#include "nsXULAppAPI.h"
#else
#include "nsISuiteRemoteService.h"
#endif

#define MOZILLA_VERSION_PROP   "_MOZILLA_VERSION"
#define MOZILLA_LOCK_PROP      "_MOZILLA_LOCK"
#define MOZILLA_COMMAND_PROP   "_MOZILLA_COMMAND"
#define MOZILLA_RESPONSE_PROP  "_MOZILLA_RESPONSE"
#define MOZILLA_USER_PROP      "_MOZILLA_USER"
#define MOZILLA_PROFILE_PROP   "_MOZILLA_PROFILE"
#define MOZILLA_PROGRAM_PROP   "_MOZILLA_PROGRAM"
#define MOZILLA_COMMANDLINE_PROP "_MOZILLA_COMMANDLINE"

#ifdef IS_BIG_ENDIAN
#define TO_LITTLE_ENDIAN32(x) \
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | \
    (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#else
#define TO_LITTLE_ENDIAN32(x) (x)
#endif

#ifdef MOZ_XUL_APP
const unsigned char kRemoteVersion[] = "5.1";
#else
const unsigned char kRemoteVersion[] = "5.0";
#endif

NS_IMPL_ISUPPORTS2(nsQtRemoteService,
                   nsIRemoteService,
                   nsIObserver)

NS_IMETHODIMP
nsQtRemoteService::Startup(const char* aAppName, const char* aProfileName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}





















NS_IMETHODIMP
nsQtRemoteService::RegisterWindow(nsIDOMWindow* aWindow)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsQtRemoteService::Shutdown()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsQtRemoteService::Observe(nsISupports* aSubject,
                            const char *aTopic,
                            const PRUnichar *aData)
{
  return NS_OK;
}


#define NS_REMOTESERVICE_CID \
  { 0xc0773e90, 0x5799, 0x4eff, { 0xad, 0x3, 0x3e, 0xbc, 0xd8, 0x56, 0x24, 0xac } }

NS_GENERIC_FACTORY_CONSTRUCTOR(nsQtRemoteService)

static const nsModuleComponentInfo components[] =
{
  { "Remote Service",
    NS_REMOTESERVICE_CID,
    "@mozilla.org/toolkit/remote-service;1",
    nsQtRemoteServiceConstructor
  }
};

NS_IMPL_NSGETMODULE(RemoteServiceModule, components)
