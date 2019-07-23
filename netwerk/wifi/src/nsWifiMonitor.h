






































#include "nsIWifiMonitor.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsAutoLock.h"
#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsCOMArray.h"
#include "nsIWifiMonitor.h"
#include "prmon.h"
#include "prlog.h"
#include "nsIObserver.h"
#include "nsTArray.h"

#ifndef __nsWifiMonitor__
#define __nsWifiMonitor__

#if defined(PR_LOGGING)
extern PRLogModuleInfo *gWifiMonitorLog;
#endif
#define LOG(args)     PR_LOG(gWifiMonitorLog, PR_LOG_DEBUG, args)

class nsWifiListener
{
 public:

  nsWifiListener(nsIWifiListener* aListener)
  {
    mListener = aListener;
    mHasSentData = PR_FALSE;
  }
  ~nsWifiListener() {}
  
  nsCOMPtr<nsIWifiListener> mListener;
  PRBool mHasSentData;
};

class nsWifiMonitor : nsIRunnable, nsIWifiMonitor, nsIObserver
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWIFIMONITOR
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIOBSERVER

  nsWifiMonitor();

 private:
  ~nsWifiMonitor();

  nsresult DoScan();

#if defined(XP_MACOSX)
  nsresult DoScanWithCoreWLAN();
  nsresult DoScanOld();
#endif

  PRBool mKeepGoing;
  nsCOMPtr<nsIThread> mThread;

  nsTArray<nsWifiListener> mListeners;

  PRMonitor* mMonitor;

};

#endif
