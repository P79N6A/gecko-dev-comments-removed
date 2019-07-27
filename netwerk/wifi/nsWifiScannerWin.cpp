



#include "nsWifiMonitor.h"


#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"
#include "nsServiceManagerUtils.h"
#include "nsWifiAccessPoint.h"
#include "win_wifiScanner.h"

using namespace mozilla;











nsresult
nsWifiMonitor::DoScan()
{
    if (!mWinWifiScanner) {
      mWinWifiScanner = new WinWifiScanner();
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
