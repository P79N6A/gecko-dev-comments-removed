





#include "nsNSSDialogHelper.h"
#include "nsIWindowWatcher.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "mozilla/dom/ScriptSettings.h"

static const char kOpenDialogParam[] = "centerscreen,chrome,modal,titlebar";
static const char kOpenWindowParam[] = "centerscreen,chrome,titlebar";

nsresult
nsNSSDialogHelper::openDialog(
    nsIDOMWindow *window,
    const char *url,
    nsISupports *params,
    bool modal)
{
#ifdef MOZ_WIDGET_GONK
  
  
  return NS_ERROR_NOT_IMPLEMENTED;
#endif

  nsresult rv;
  nsCOMPtr<nsIWindowWatcher> windowWatcher = 
           do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDOMWindow> parent = window;

  if (!parent) {
    windowWatcher->GetActiveWindow(getter_AddRefs(parent));
  }

  
  
  
  
  MOZ_ASSERT(!strncmp("chrome://", url, strlen("chrome://")));
  mozilla::dom::AutoNoJSAPI nojsapi;

  nsCOMPtr<nsIDOMWindow> newWindow;
  rv = windowWatcher->OpenWindow(parent,
                                 url,
                                 "_blank",
                                 modal
                                 ? kOpenDialogParam
                                 : kOpenWindowParam,
                                 params,
                                 getter_AddRefs(newWindow));
  return rv;
}

