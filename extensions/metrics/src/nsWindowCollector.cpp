





































#include "nsWindowCollector.h"
#include "nsMetricsService.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsDocShellCID.h"
#include "nsAutoPtr.h"
#include "nsITimer.h"
#include "nsComponentManagerUtils.h"

nsWindowCollector::nsWindowCollector()
{
}

nsWindowCollector::~nsWindowCollector()
{
}

NS_IMPL_ISUPPORTS2(nsWindowCollector, nsIMetricsCollector, nsIObserver)

NS_IMETHODIMP
nsWindowCollector::OnAttach()
{
  nsCOMPtr<nsIObserverService> obsSvc =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ENSURE_STATE(obsSvc);

  nsresult rv = obsSvc->AddObserver(this, NS_WEBNAVIGATION_CREATE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->AddObserver(this, NS_CHROME_WEBNAVIGATION_CREATE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->AddObserver(this, "domwindowopened", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->AddObserver(this, "domwindowclosed", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->AddObserver(this, NS_METRICS_WEBNAVIGATION_DESTROY, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->AddObserver(this,
                           NS_METRICS_CHROME_WEBNAVIGATION_DESTROY, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsWindowCollector::OnDetach()
{
  nsCOMPtr<nsIObserverService> obsSvc =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ENSURE_STATE(obsSvc);

  nsresult rv = obsSvc->RemoveObserver(this, NS_WEBNAVIGATION_CREATE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->RemoveObserver(this, NS_CHROME_WEBNAVIGATION_CREATE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->RemoveObserver(this, "domwindowopened");
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->RemoveObserver(this, "domwindowclosed");
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->RemoveObserver(this, NS_METRICS_WEBNAVIGATION_DESTROY);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->RemoveObserver(this, NS_METRICS_CHROME_WEBNAVIGATION_DESTROY);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsWindowCollector::OnNewLog()
{
  return NS_OK;
}

struct WindowOpenClosure
{
  WindowOpenClosure(nsISupports *subj, nsWindowCollector *coll)
      : subject(subj), collector(coll) { }

  nsCOMPtr<nsISupports> subject;
  nsRefPtr<nsWindowCollector> collector;
};

 void
nsWindowCollector::WindowOpenCallback(nsITimer *timer, void *closure)
{
  WindowOpenClosure *wc = NS_STATIC_CAST(WindowOpenClosure *, closure);
  wc->collector->LogWindowOpen(timer, wc->subject);

  delete wc;
}

void
nsWindowCollector::LogWindowOpen(nsITimer *timer, nsISupports *subject)
{
  mWindowOpenTimers.RemoveElement(timer);
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(subject);

  if (!window) {
    return;
  }

  nsCOMPtr<nsIDOMWindowInternal> opener;
  window->GetOpener(getter_AddRefs(opener));

  nsCOMPtr<nsIWritablePropertyBag2> properties;
  nsMetricsUtils::NewPropertyBag(getter_AddRefs(properties));
  if (!properties) {
    return;
  }

  if (opener) {
    properties->SetPropertyAsUint32(NS_LITERAL_STRING("opener"),
                                    nsMetricsService::GetWindowID(opener));
  }

  properties->SetPropertyAsUint32(NS_LITERAL_STRING("windowid"),
                                  nsMetricsService::GetWindowID(window));

  properties->SetPropertyAsACString(NS_LITERAL_STRING("action"),
                                    NS_LITERAL_CSTRING("open"));

  nsMetricsService *ms = nsMetricsService::get();
  if (ms) {
    ms->LogEvent(NS_LITERAL_STRING("window"), properties);
  }
}

NS_IMETHODIMP
nsWindowCollector::Observe(nsISupports *subject,
                           const char *topic,
                           const PRUnichar *data)
{
  nsCOMPtr<nsIWritablePropertyBag2> properties;
  nsresult rv = nsMetricsUtils::NewPropertyBag(getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsPIDOMWindow> window;
  nsCString action;

  if (strcmp(topic, NS_WEBNAVIGATION_CREATE) == 0 ||
      strcmp(topic, NS_CHROME_WEBNAVIGATION_CREATE) == 0) {
    
    action.Assign("create");

    nsCOMPtr<nsIDocShellTreeItem> item = do_QueryInterface(subject);
    NS_ENSURE_STATE(item);

    window = do_GetInterface(subject);
    NS_ENSURE_STATE(window);

    
    
    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    item->GetParent(getter_AddRefs(parentItem));
    nsCOMPtr<nsPIDOMWindow> parentWindow = do_GetInterface(parentItem);
    if (parentWindow) {
      PRUint32 id = nsMetricsService::GetWindowID(parentWindow);
      rv = properties->SetPropertyAsUint32(NS_LITERAL_STRING("parent"), id);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (strcmp(topic, NS_CHROME_WEBNAVIGATION_CREATE) == 0) {
      rv = properties->SetPropertyAsBool(NS_LITERAL_STRING("chrome"), PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (nsMetricsUtils::IsSubframe(item)) {
      rv = properties->SetPropertyAsBool(NS_LITERAL_STRING("subframe"),
                                         PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else if (strcmp(topic, "domwindowopened") == 0) {
    
    

    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
    NS_ENSURE_STATE(timer);

    WindowOpenClosure *wc = new WindowOpenClosure(subject, this);
    NS_ENSURE_TRUE(wc, NS_ERROR_OUT_OF_MEMORY);

    rv = timer->InitWithFuncCallback(nsWindowCollector::WindowOpenCallback,
                                     wc, 0, nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);

    mWindowOpenTimers.AppendElement(timer);
  } else if (strcmp(topic, "domwindowclosed") == 0) {
    
    action.Assign("close");
    window = do_QueryInterface(subject);
  } else if (strcmp(topic, NS_METRICS_WEBNAVIGATION_DESTROY) == 0 ||
             strcmp(topic, NS_METRICS_CHROME_WEBNAVIGATION_DESTROY) == 0) {
    
    action.Assign("destroy");
    window = do_GetInterface(subject);

    nsCOMPtr<nsIDocShellTreeItem> item = do_QueryInterface(subject);
    NS_ENSURE_STATE(item);
    if (nsMetricsUtils::IsSubframe(item)) {
      rv = properties->SetPropertyAsBool(NS_LITERAL_STRING("subframe"),
                                         PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (window) {
    rv = properties->SetPropertyAsUint32(NS_LITERAL_STRING("windowid"),
                                         nsMetricsService::GetWindowID(window));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = properties->SetPropertyAsACString(NS_LITERAL_STRING("action"),
                                           action);
    NS_ENSURE_SUCCESS(rv, rv);

    nsMetricsService *ms = nsMetricsService::get();
    NS_ENSURE_STATE(ms);
    rv = ms->LogEvent(NS_LITERAL_STRING("window"), properties);
  }

  return rv;
}
