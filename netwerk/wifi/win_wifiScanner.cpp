



#include "nsWifiAccessPoint.h"
#include "win_wifiScanner.h"


#include "win_wlanLibrary.h"

#define DOT11_BSS_TYPE_UNUSED static_cast<DOT11_BSS_TYPE>(0)

class InterfaceScanCallbackData {
public:
  InterfaceScanCallbackData(uint32_t numInterfaces)
    : mCurrentlyScanningInterfaces(numInterfaces)
  {
    mAllInterfacesDoneScanningEvent =
      ::CreateEvent(nullptr,  
                    TRUE,     
                    FALSE,    
                    nullptr); 
    MOZ_ASSERT(NULL != mAllInterfacesDoneScanningEvent);
  }

  ~InterfaceScanCallbackData()
  {
    ::CloseHandle(mAllInterfacesDoneScanningEvent);
  }

  void
  OnInterfaceScanComplete()
  {
    uint32_t val = ::InterlockedDecrement(&mCurrentlyScanningInterfaces);
    if (!val) {
      ::SetEvent(mAllInterfacesDoneScanningEvent);
    }
  }

  void
  WaitForAllInterfacesToFinishScanning(uint32_t msToWait)
  {
    ::WaitForSingleObject(mAllInterfacesDoneScanningEvent,
                          msToWait);
  }

private:
  volatile uint32_t mCurrentlyScanningInterfaces;
  HANDLE mAllInterfacesDoneScanningEvent;
};

static void
OnScanComplete(PWLAN_NOTIFICATION_DATA data, PVOID context)
{
  if (WLAN_NOTIFICATION_SOURCE_ACM != data->NotificationSource) {
    return;
  }

  if (wlan_notification_acm_scan_complete != data->NotificationCode &&
      wlan_notification_acm_scan_fail != data->NotificationCode) {
    return;
  }

  InterfaceScanCallbackData* cbData =
    reinterpret_cast<InterfaceScanCallbackData*>(context);
  cbData->OnInterfaceScanComplete();
}

WinWifiScanner::WinWifiScanner()
{
  
  
  
  
  
  
  mWlanLibrary = WinWLANLibrary::Load();
  MOZ_ASSERT(mWlanLibrary);
}

WinWifiScanner::~WinWifiScanner()
{
}

nsresult
WinWifiScanner::GetAccessPointsFromWLAN(nsCOMArray<nsWifiAccessPoint> &accessPoints)
{
  accessPoints.Clear();

  
  
  if (!mWlanLibrary) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  WLAN_INTERFACE_INFO_LIST *interface_list = nullptr;
  if (ERROR_SUCCESS !=
      (*mWlanLibrary->GetWlanEnumInterfacesPtr())(mWlanLibrary->GetWLANHandle(),
                                                  nullptr,
                                                  &interface_list)) {
    return NS_ERROR_FAILURE;
  }

  
  ScopedWLANObject scopedInterfaceList(mWlanLibrary, interface_list);

  if (!interface_list->dwNumberOfItems) {
    return NS_OK;
  }

  InterfaceScanCallbackData cbData(interface_list->dwNumberOfItems);

  DWORD wlanNotifySource;
  if (ERROR_SUCCESS !=
      (*mWlanLibrary->GetWlanRegisterNotificationPtr())(
                                  mWlanLibrary->GetWLANHandle(),
                                  WLAN_NOTIFICATION_SOURCE_ACM,
                                  TRUE,
                                  (WLAN_NOTIFICATION_CALLBACK)OnScanComplete,
                                  &cbData,
                                  NULL,
                                  &wlanNotifySource)) {
    return NS_ERROR_FAILURE;
  }

  
  for (unsigned int i = 0; i < interface_list->dwNumberOfItems; ++i) {
    if (ERROR_SUCCESS !=
        (*mWlanLibrary->GetWlanScanPtr())(
                    mWlanLibrary->GetWLANHandle(),
                    &interface_list->InterfaceInfo[i].InterfaceGuid,
                    NULL,
                    NULL,
                    NULL)) {
      cbData.OnInterfaceScanComplete();
    }
  }

  
  
  
  cbData.WaitForAllInterfacesToFinishScanning(5000);

  
  
  
  (*mWlanLibrary->GetWlanRegisterNotificationPtr())(
                              mWlanLibrary->GetWLANHandle(),
                              WLAN_NOTIFICATION_SOURCE_NONE,
                              TRUE,
                              NULL,
                              NULL,
                              NULL,
                              &wlanNotifySource);

  
  for (uint32_t i = 0; i < interface_list->dwNumberOfItems; ++i) {
    WLAN_BSS_LIST *bss_list;
    if (ERROR_SUCCESS !=
        (*mWlanLibrary->GetWlanGetNetworkBssListPtr())(
                           mWlanLibrary->GetWLANHandle(),
                           &interface_list->InterfaceInfo[i].InterfaceGuid,
                           nullptr,  
                           DOT11_BSS_TYPE_UNUSED,
                           false,    
                           nullptr,  
                           &bss_list)) {
      continue;
    }

    
    ScopedWLANObject scopedBssList(mWlanLibrary, bss_list);

    
    for (int j = 0; j < static_cast<int>(bss_list->dwNumberOfItems); ++j) {
      nsWifiAccessPoint* ap = new nsWifiAccessPoint();
      if (!ap) {
        continue;
      }

      const WLAN_BSS_ENTRY bss_entry = bss_list->wlanBssEntries[j];
      ap->setMac(bss_entry.dot11Bssid);
      ap->setSignal(bss_entry.lRssi);
      ap->setSSID(reinterpret_cast<char const*>(bss_entry.dot11Ssid.ucSSID),
                  bss_entry.dot11Ssid.uSSIDLength);

      accessPoints.AppendObject(ap);
    }
  }

  return NS_OK;
}
