








































#include "windows.h"
#include "wlanapi.h"

#include <ntddndis.h>
#include "winioctl.h"
#include "stdlib.h"

#include "nsWifiMonitor.h"
#include "nsWifiAccessPoint.h"

#include "nsIProxyObjectManager.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"

using namespace mozilla;

nsresult
nsWifiMonitor::DoScan()
{
    HINSTANCE wlan_library = LoadLibrary("Wlanapi.dll");
    if (!wlan_library)
      return NS_ERROR_NOT_AVAILABLE;

    WlanOpenHandleFunction WlanOpenHandle = (WlanOpenHandleFunction) GetProcAddress(wlan_library, "WlanOpenHandle");
    WlanEnumInterfacesFunction WlanEnumInterfaces = (WlanEnumInterfacesFunction) GetProcAddress(wlan_library, "WlanEnumInterfaces");
    WlanGetNetworkBssListFunction WlanGetNetworkBssList = (WlanGetNetworkBssListFunction) GetProcAddress(wlan_library, "WlanGetNetworkBssList");
    WlanFreeMemoryFunction WlanFreeMemory = (WlanFreeMemoryFunction) GetProcAddress(wlan_library, "WlanFreeMemory");
    WlanCloseHandleFunction WlanCloseHandle = (WlanCloseHandleFunction) GetProcAddress(wlan_library, "WlanCloseHandle");

    if (!WlanOpenHandle ||
        !WlanEnumInterfaces ||
        !WlanGetNetworkBssList ||
        !WlanFreeMemory ||
        !WlanCloseHandle)
      return NS_ERROR_FAILURE;

    

    nsCOMArray<nsWifiAccessPoint> lastAccessPoints;
    nsCOMArray<nsWifiAccessPoint> accessPoints;

    do {
      accessPoints.Clear();

      
      DWORD negotiated_version;
      HANDLE wlan_handle = NULL;
      
      
      
      static const int kXpWlanClientVersion = 1;
      if ((*WlanOpenHandle)(kXpWlanClientVersion,
                            NULL,
                            &negotiated_version,
                            &wlan_handle) != ERROR_SUCCESS) {
        return NS_ERROR_NOT_AVAILABLE;
      }

      
      if (!wlan_handle)
        return NS_ERROR_FAILURE;

      
      WLAN_INTERFACE_INFO_LIST *interface_list = NULL;
      if ((*WlanEnumInterfaces)(wlan_handle, NULL, &interface_list) != ERROR_SUCCESS) {
        
        (*WlanCloseHandle)(wlan_handle, NULL);
        return NS_ERROR_FAILURE;
      }

      
      for (int i = 0; i < static_cast<int>(interface_list->dwNumberOfItems); ++i) {

        WLAN_BSS_LIST *bss_list;
        HRESULT rv = (*WlanGetNetworkBssList)(wlan_handle,
                                              &interface_list->InterfaceInfo[i].InterfaceGuid,
                                              NULL,   
                                              DOT11_BSS_TYPE_UNUSED,
                                              false,  
                                              NULL,   
                                              &bss_list);
        if (rv != ERROR_SUCCESS) {
          continue;
        }

        for (int j = 0; j < static_cast<int>(bss_list->dwNumberOfItems); ++j) {

          nsWifiAccessPoint* ap = new nsWifiAccessPoint();
          if (!ap)
            continue;

          const WLAN_BSS_ENTRY bss_entry = bss_list->wlanBssEntries[j];

          ap->setMac(bss_entry.dot11Bssid);
          ap->setSignal(bss_entry.lRssi);
          ap->setSSID((char*) bss_entry.dot11Ssid.ucSSID,
                      bss_entry.dot11Ssid.uSSIDLength);

          accessPoints.AppendObject(ap);
        }
        (*WlanFreeMemory)(bss_list);
      }

      
      (*WlanFreeMemory)(interface_list);

      
      (*WlanCloseHandle)(wlan_handle, NULL);


      PRBool accessPointsChanged = !AccessPointsEqual(accessPoints, lastAccessPoints);
      nsCOMArray<nsIWifiListener> currentListeners;

      {
        ReentrantMonitorAutoEnter mon(mReentrantMonitor);

        for (PRUint32 i = 0; i < mListeners.Length(); i++) {
          if (!mListeners[i].mHasSentData || accessPointsChanged) {
            mListeners[i].mHasSentData = PR_TRUE;
            currentListeners.AppendObject(mListeners[i].mListener);
          }
        }
      }

      ReplaceArray(lastAccessPoints, accessPoints);

      if (currentListeners.Count() > 0) {
        PRUint32 resultCount = lastAccessPoints.Count();
        nsIWifiAccessPoint** result = static_cast<nsIWifiAccessPoint**> (nsMemory::Alloc(sizeof(nsIWifiAccessPoint*) * resultCount));
        if (!result)
          return NS_ERROR_OUT_OF_MEMORY;

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

      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      mon.Wait(PR_SecondsToInterval(60));
    }
    while (mKeepGoing);

    return NS_OK;
}
