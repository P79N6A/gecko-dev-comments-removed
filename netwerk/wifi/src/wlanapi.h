
































typedef DWORD (WINAPI *WlanOpenHandleFunction)(DWORD dwClientVersion,
                                               PVOID pReserved,
                                               PDWORD pdwNegotiatedVersion,
                                               PHANDLE phClientHandle);



typedef enum _WLAN_INTERFACE_STATE {
  WLAN_INTERFACE_STATE_UNUSED
} WLAN_INTERFACE_STATE;

typedef struct _WLAN_INTERFACE_INFO {
  GUID InterfaceGuid;
  WCHAR strInterfaceDescription[256];
  WLAN_INTERFACE_STATE isState;
} WLAN_INTERFACE_INFO, *PWLAN_INTERFACE_INFO;

typedef struct _WLAN_INTERFACE_INFO_LIST {
  DWORD dwNumberOfItems;
  DWORD dwIndex;
  WLAN_INTERFACE_INFO InterfaceInfo[1];
} WLAN_INTERFACE_INFO_LIST, *PWLAN_INTERFACE_INFO_LIST;

typedef DWORD (WINAPI *WlanEnumInterfacesFunction)(
    HANDLE hClientHandle,
    PVOID pReserved,
    PWLAN_INTERFACE_INFO_LIST *ppInterfaceList);



#define DOT11_SSID_MAX_LENGTH 32

typedef struct _DOT11_SSID {
  ULONG uSSIDLength;
  UCHAR ucSSID[DOT11_SSID_MAX_LENGTH];
} DOT11_SSID, *PDOT11_SSID;

typedef UCHAR DOT11_MAC_ADDRESS[6];

typedef enum _DOT11_BSS_TYPE {
  DOT11_BSS_TYPE_UNUSED
} DOT11_BSS_TYPE;

typedef enum _DOT11_PHY_TYPE {
  DOT11_PHY_TYPE_UNUSED
} DOT11_PHY_TYPE;

#define DOT11_RATE_SET_MAX_LENGTH (126)

typedef struct _WLAN_RATE_SET {
  ULONG uRateSetLength;
  USHORT usRateSet[DOT11_RATE_SET_MAX_LENGTH];
} WLAN_RATE_SET, *PWLAN_RATE_SET;

typedef struct _WLAN_BSS_ENTRY {
  DOT11_SSID dot11Ssid;
  ULONG uPhyId;
  DOT11_MAC_ADDRESS dot11Bssid;
  DOT11_BSS_TYPE dot11BssType;
  DOT11_PHY_TYPE dot11BssPhyType;
  LONG lRssi;
  ULONG uLinkQuality;
  BOOLEAN bInRegDomain;
  USHORT usBeaconPeriod;
  ULONGLONG ullTimestamp;
  ULONGLONG ullHostTimestamp;
  USHORT usCapabilityInformation;
  ULONG ulChCenterFrequency;
  WLAN_RATE_SET wlanRateSet;
  ULONG ulIeOffset;
  ULONG ulIeSize;
} WLAN_BSS_ENTRY, *PWLAN_BSS_ENTRY;

typedef struct _WLAN_BSS_LIST {
  DWORD dwTotalSize;
  DWORD dwNumberOfItems;
  
  
  WLAN_BSS_ENTRY wlanBssEntries[1];
} WLAN_BSS_LIST, *PWLAN_BSS_LIST;

typedef DWORD (WINAPI *WlanGetNetworkBssListFunction)(
    HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    const  PDOT11_SSID pDot11Ssid,
    DOT11_BSS_TYPE dot11BssType,
    BOOL bSecurityEnabled,
    PVOID pReserved,
    PWLAN_BSS_LIST *ppWlanBssList
);



typedef VOID (WINAPI *WlanFreeMemoryFunction)(PVOID pMemory);



typedef DWORD (WINAPI *WlanCloseHandleFunction)(HANDLE hClientHandle,
                                                PVOID pReserved);


