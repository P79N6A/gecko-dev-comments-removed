






































#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMCID.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsWifiMonitor.h"

#include "nsIProxyObjectManager.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "mozilla/Services.h"

using namespace mozilla;

#if defined(PR_LOGGING)
PRLogModuleInfo *gWifiMonitorLog;
#endif

NS_IMPL_THREADSAFE_ISUPPORTS3(nsWifiMonitor,
                              nsIRunnable,
                              nsIObserver,
                              nsIWifiMonitor)

nsWifiMonitor::nsWifiMonitor()
: mKeepGoing(PR_TRUE)
, mReentrantMonitor("nsWifiMonitor.mReentrantMonitor")
{
#if defined(PR_LOGGING)
  gWifiMonitorLog = PR_NewLogModule("WifiMonitor");
#endif

  nsCOMPtr<nsIObserverService> obsSvc = mozilla::services::GetObserverService();
  if (obsSvc)
    obsSvc->AddObserver(this, "xpcom-shutdown", PR_FALSE);

  LOG(("@@@@@ wifimonitor created\n"));
}

nsWifiMonitor::~nsWifiMonitor()
{
}

NS_IMETHODIMP
nsWifiMonitor::Observe(nsISupports *subject, const char *topic,
                     const PRUnichar *data)
{
  if (!strcmp(topic, "xpcom-shutdown")) {
    LOG(("Shutting down\n"));
    mKeepGoing = PR_FALSE;

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mon.Notify();
  }
  return NS_OK;
}


NS_IMETHODIMP nsWifiMonitor::StartWatching(nsIWifiListener *aListener)
{
  if (!aListener)
    return NS_ERROR_NULL_POINTER;

  nsresult rv = NS_OK;
  if (!mThread) {
    rv = NS_NewThread(getter_AddRefs(mThread), this);
    if (NS_FAILED(rv))
      return rv;
  }

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  mKeepGoing = PR_TRUE;

  mListeners.AppendElement(nsWifiListener(aListener));

  
  mon.Notify();
  return NS_OK;
}

NS_IMETHODIMP nsWifiMonitor::StopWatching(nsIWifiListener *aListener)
{
  if (!aListener)
    return NS_ERROR_NULL_POINTER;

  LOG(("removing listener\n"));

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  for (PRUint32 i = 0; i < mListeners.Length(); i++) {

    if (mListeners[i].mListener == aListener) {
      mListeners.RemoveElementAt(i);
      break;
    }
  }

  if (mListeners.Length() == 0) {
    mKeepGoing = PR_FALSE;
    mon.Notify();
    mThread = nsnull;
  }

  return NS_OK;
}


NS_IMETHODIMP nsWifiMonitor::Run()
{
  LOG(("@@@@@ wifi monitor run called\n"));

  nsresult rv = DoScan();

  if (mKeepGoing && NS_FAILED(rv)) {

    nsCOMPtr<nsIProxyObjectManager> proxyObjMgr = do_GetService("@mozilla.org/xpcomproxy;1");

    
    for (PRUint32 i = 0; i < mListeners.Length(); i++) {
      LOG(("About to send error to a listener\n"));

      nsCOMPtr<nsIWifiListener> proxy;
      proxyObjMgr->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                     NS_GET_IID(nsIWifiListener),
                                     mListeners[i].mListener,
                                     NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                     getter_AddRefs(proxy));

      if (proxy) {
        proxy->OnError(rv);
        LOG( ("... sent %d\n", rv));
      }
    }
  }

  return NS_OK;
}
