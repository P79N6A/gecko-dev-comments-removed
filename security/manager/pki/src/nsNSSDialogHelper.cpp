







































#include "nsNSSDialogHelper.h"
#include "nsIWindowWatcher.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"

const char* nsNSSDialogHelper::kDefaultOpenWindowParam = "centerscreen,chrome,modal,titlebar";

nsresult
nsNSSDialogHelper::openDialog(
    nsIDOMWindowInternal *window,
    const char *url,
    nsISupports *params)
{
  nsresult rv;
  nsCOMPtr<nsIWindowWatcher> windowWatcher = 
           do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsIDOMWindowInternal *parent = window;

  nsCOMPtr<nsIDOMWindowInternal> activeParent;
  if (!parent) {
    nsCOMPtr<nsIDOMWindow> active;
    windowWatcher->GetActiveWindow(getter_AddRefs(active));
    if (active) {
      active->QueryInterface(NS_GET_IID(nsIDOMWindowInternal), getter_AddRefs(activeParent));
      parent = activeParent;
    }
  }

  nsCOMPtr<nsIDOMWindow> newWindow;
  rv = windowWatcher->OpenWindow(parent,
                                 url,
                                 "_blank",
                                 nsNSSDialogHelper::kDefaultOpenWindowParam,
                                 params,
                                 getter_AddRefs(newWindow));
  return rv;
}

