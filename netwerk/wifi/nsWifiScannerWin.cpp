



#include "nsWifiMonitor.h"


#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"
#include "nsServiceManagerUtils.h"
#include "nsWifiAccessPoint.h"
#include "win_wifiScanner.h"
#include "win_xp_wifiScanner.h"
#include "mozilla/WindowsVersion.h"

using namespace mozilla;











nsresult
nsWifiMonitor::DoScan()
{
    if (!mWinWifiScanner) {
      if (IsWin2003OrLater()) {
        mWinWifiScanner = new WinWifiScanner();
        LOG(("Using Windows 2003+ wifi scanner."));
      } else {
        mWinWifiScanner = new WinXPWifiScanner();
        LOG(("Using Windows XP wifi scanner."));
      }

      if (!mWinWifiScanner) {
        
        return NS_ERROR_FAILURE;
      }
    }

    

    nsCOMArray<nsWifiAccessPoint> lastAccessPoints;
    nsCOMArray<nsWifiAccessPoint> accessPoints;

    do {
      accessPoints.Clear();
      nsresult rv = mWinWifiScanner->GetAccessPointsFromWLAN(accessPoints);
      if (NS_FAILED(rv)) {
        return rv;
      }

      bool accessPointsChanged = !AccessPointsEqual(accessPoints, lastAccessPoints);
      ReplaceArray(lastAccessPoints, accessPoints);

      rv = CallWifiListeners(lastAccessPoints, accessPointsChanged);
      NS_ENSURE_SUCCESS(rv, rv);

      
      LOG(("waiting on monitor\n"));

      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      if (mKeepGoing) {
          mon.Wait(PR_SecondsToInterval(kDefaultWifiScanInterval));
      }
    }
    while (mKeepGoing);

    return NS_OK;
}
