






































#include <mach-o/dyld.h>
#include <dlfcn.h>
#include <unistd.h>

#include "osx_wifi.h"

#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsWifiMonitor.h"
#include "nsWifiAccessPoint.h"

#include "nsIProxyObjectManager.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"

nsresult
nsWifiMonitor::DoScan()
{
  void *apple_80211_library = dlopen(
      "/System/Library/PrivateFrameworks/Apple80211.framework/Apple80211",
      RTLD_LAZY);
  if (!apple_80211_library)
    return NS_ERROR_NOT_AVAILABLE;

  WirelessContextPtr wifi_context_;

  WirelessAttachFunction WirelessAttach_function_ = reinterpret_cast<WirelessAttachFunction>(dlsym(apple_80211_library, "WirelessAttach"));
  WirelessScanSplitFunction WirelessScanSplit_function_ = reinterpret_cast<WirelessScanSplitFunction>(dlsym(apple_80211_library, "WirelessScanSplit"));
  WirelessDetachFunction WirelessDetach_function_ = reinterpret_cast<WirelessDetachFunction>(dlsym(apple_80211_library, "WirelessDetach"));
  
  if (!WirelessAttach_function_ || !WirelessScanSplit_function_ || !WirelessDetach_function_) {
    dlclose(apple_80211_library);
    return NS_ERROR_NOT_AVAILABLE;
  }

  if ((*WirelessAttach_function_)(&wifi_context_, 0) != noErr) {
    dlclose(apple_80211_library);
    return NS_ERROR_FAILURE;
  }

  
    
  nsCOMArray<nsWifiAccessPoint> lastAccessPoints;
  nsCOMArray<nsWifiAccessPoint> accessPoints;

  do {
    accessPoints.Clear();

    CFArrayRef managed_access_points = NULL;
    CFArrayRef adhoc_access_points = NULL;
    
    if ((*WirelessScanSplit_function_)(wifi_context_,
                                      &managed_access_points,
                                      &adhoc_access_points,
                                      0) != noErr) {
      continue;
    }
    
    if (managed_access_points == NULL) {
      continue;
    }

    int accessPointsCount = CFArrayGetCount(managed_access_points);

    for (int i = 0; i < accessPointsCount; ++i) {

      nsWifiAccessPoint* ap = new nsWifiAccessPoint();
      if (!ap)
        continue;

      const WirelessNetworkInfo *access_point_info =
        reinterpret_cast<const WirelessNetworkInfo*>(CFDataGetBytePtr(reinterpret_cast<const CFDataRef>(CFArrayGetValueAtIndex(managed_access_points, i))));

      ap->setMac(access_point_info->macAddress);

      
      ap->setSignal(access_point_info->signal);

      ap->setSSID(reinterpret_cast<const char*>(access_point_info->name),
                  access_point_info->nameLen);

      accessPoints.AppendObject(ap);
    }

    PRBool accessPointsChanged = !AccessPointsEqual(accessPoints, lastAccessPoints);
    nsCOMArray<nsIWifiListener> currentListeners;

    {
      nsAutoMonitor mon(mMonitor);
      
      for (PRUint32 i = 0; i < mListeners.Length(); i++) {
        if (!mListeners[i].mHasSentData || accessPointsChanged) {
          mListeners[i].mHasSentData = PR_TRUE;
          currentListeners.AppendObject(mListeners[i].mListener);
        }
      }
    }
    
    ReplaceArray(lastAccessPoints, accessPoints);

    if (currentListeners.Count() > 0)
    {
      PRUint32 resultCount = lastAccessPoints.Count();
      nsIWifiAccessPoint** result = static_cast<nsIWifiAccessPoint**> (nsMemory::Alloc(sizeof(nsIWifiAccessPoint*) * resultCount));
      if (!result) {
        dlclose(apple_80211_library);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      for (PRUint32 i = 0; i < resultCount; i++)
        result[i] = lastAccessPoints[i];

      for (PRInt32 i = 0; i < currentListeners.Count(); i++) {
        
        LOG(("About to send data to the wifi listeners\n"));
        
        nsCOMPtr<nsIWifiListener> proxy;
        nsCOMPtr<nsIProxyObjectManager> proxyObjMgr = do_GetService("@mozilla.org/xpcomproxy;1");
        proxyObjMgr->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                       NS_GET_IID(nsIWifiListener),
                                       currentListeners[i],
                                       NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                       getter_AddRefs(proxy));
        if (!proxy) {
          LOG(("There is no proxy available.  this should never happen\n"));
        }
        else
        {
          nsresult rv = proxy->OnChange(result, resultCount);
          LOG( ("... sent %d\n", rv));
        }
      }

      nsMemory::Free(result);
    }

    
    LOG(("waiting on monitor\n"));
    
    nsAutoMonitor mon(mMonitor);
    mon.Wait(PR_SecondsToInterval(60));
  }
  while (mKeepGoing == PR_TRUE);
  
  (*WirelessDetach_function_)(wifi_context_);
  
  dlclose(apple_80211_library);
  
  return NS_OK;
}
