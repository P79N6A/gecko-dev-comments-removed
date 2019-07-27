



























#include "win_xp_wifiScanner.h"
#include "nsWifiAccessPoint.h"
#include <windows.h>
#include <winioctl.h>
#include <wlanapi.h>
#include <string>
#include <vector>


#define NDIS_STATUS_INVALID_LENGTH   ((NDIS_STATUS)0xC0010014L)
#define NDIS_STATUS_BUFFER_TOO_SHORT ((NDIS_STATUS)0xC0010016L)

namespace {

const int kInitialBufferSize = 2 << 12;  
const int kMaximumBufferSize = 2 << 20;  


const int kStringLength = 512;


typedef DWORD (WINAPI* WlanOpenHandleFunction)(DWORD dwClientVersion,
                                               PVOID pReserved,
                                               PDWORD pdwNegotiatedVersion,
                                               PHANDLE phClientHandle);


typedef DWORD (WINAPI* WlanEnumInterfacesFunction)(
    HANDLE hClientHandle,
    PVOID pReserved,
    PWLAN_INTERFACE_INFO_LIST* ppInterfaceList);


typedef DWORD (WINAPI* WlanGetNetworkBssListFunction)(
    HANDLE hClientHandle,
    const GUID* pInterfaceGuid,
    const  PDOT11_SSID pDot11Ssid,
    DOT11_BSS_TYPE dot11BssType,
    BOOL bSecurityEnabled,
    PVOID pReserved,
    PWLAN_BSS_LIST* ppWlanBssList
);


typedef VOID (WINAPI* WlanFreeMemoryFunction)(PVOID pMemory);


typedef DWORD (WINAPI* WlanCloseHandleFunction)(HANDLE hClientHandle,
                                                PVOID pReserved);


bool UndefineDosDevice(const std::string& device_name);
bool DefineDosDeviceIfNotExists(const std::string& device_name);
HANDLE GetFileHandle(const std::string& device_name);

int PerformQuery(HANDLE adapter_handle, std::vector<char>& buffer, DWORD* bytes_out);
bool ResizeBuffer(size_t requested_size, std::vector<char>& buffer);


bool GetSystemDirectory(std::string* path);

bool ConvertToAccessPointData(const NDIS_WLAN_BSSID& data, nsWifiAccessPoint* access_point_data);
int GetDataFromBssIdList(const NDIS_802_11_BSSID_LIST& bss_id_list,
                         int list_size,
                         nsCOMArray<nsWifiAccessPoint>& outData);
} 

class WindowsNdisApi
{
public:
  virtual ~WindowsNdisApi();
  static WindowsNdisApi* Create();
  virtual bool GetAccessPointData(nsCOMArray<nsWifiAccessPoint>& outData);

private:
  static bool GetInterfacesNDIS(std::vector<std::string>& interface_service_names_out);
  
  explicit WindowsNdisApi(std::vector<std::string>* interface_service_names);
  bool GetInterfaceDataNDIS(HANDLE adapter_handle, nsCOMArray<nsWifiAccessPoint>& outData);
  
  std::vector<std::string> interface_service_names_;
  std::vector<char> _buffer;
};


WindowsNdisApi::WindowsNdisApi(
    std::vector<std::string>* interface_service_names)
    : _buffer(kInitialBufferSize) {
  interface_service_names_.swap(*interface_service_names);
}

WindowsNdisApi::~WindowsNdisApi() {
}

WindowsNdisApi* WindowsNdisApi::Create() {
  std::vector<std::string> interface_service_names;
  if (GetInterfacesNDIS(interface_service_names)) {
    return new WindowsNdisApi(&interface_service_names);
  }
  return NULL;
}

bool WindowsNdisApi::GetAccessPointData(nsCOMArray<nsWifiAccessPoint>& outData) {
  int interfaces_failed = 0;
  int interfaces_succeeded = 0;

  for (int i = 0; i < static_cast<int>(interface_service_names_.size()); ++i) {
    
    if (!DefineDosDeviceIfNotExists(interface_service_names_[i])) {
      continue;
    }

    
    
    HANDLE adapter_handle = GetFileHandle(interface_service_names_[i]);
    if (adapter_handle == INVALID_HANDLE_VALUE) {
      continue;
    }

    
    if (GetInterfaceDataNDIS(adapter_handle, outData)) {
      ++interfaces_succeeded;
    } else {
      ++interfaces_failed;
    }

    
    CloseHandle(adapter_handle);
    UndefineDosDevice(interface_service_names_[i]);
  }

  
  
  return interfaces_succeeded > 0 || interfaces_failed == 0;
}

bool WindowsNdisApi::GetInterfacesNDIS(std::vector<std::string>& interface_service_names_out) {
  HKEY network_cards_key = NULL;
  if (RegOpenKeyEx(
      HKEY_LOCAL_MACHINE,
      "Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards",
      0,
      KEY_READ,
      &network_cards_key) != ERROR_SUCCESS) {
    return false;
  }
  if (!network_cards_key) {
    return false;
  }

  for (int i = 0; ; ++i) {
    TCHAR name[kStringLength];
    DWORD name_size = kStringLength;
    FILETIME time;
    if (RegEnumKeyEx(network_cards_key,
                     i,
                     name,
                     &name_size,
                     NULL,
                     NULL,
                     NULL,
                     &time) != ERROR_SUCCESS) {
      break;
    }
    HKEY hardware_key = NULL;
    if (RegOpenKeyEx(network_cards_key, name, 0, KEY_READ, &hardware_key) !=
        ERROR_SUCCESS) {
      break;
    }
    if (!hardware_key) {
      return false;
    }

    TCHAR service_name[kStringLength];
    DWORD service_name_size = kStringLength;
    DWORD type = 0;
    if (RegQueryValueEx(hardware_key,
                        "ServiceName",
                        NULL,
                        &type,
                        reinterpret_cast<LPBYTE>(service_name),
                        &service_name_size) == ERROR_SUCCESS) {
      interface_service_names_out.push_back(service_name);
    }
    RegCloseKey(hardware_key);
  }

  RegCloseKey(network_cards_key);
  return true;
}

bool WindowsNdisApi::GetInterfaceDataNDIS(HANDLE adapter_handle,
                                          nsCOMArray<nsWifiAccessPoint>& outData) {
  DWORD bytes_out;
  int result;

  while (true) {
    bytes_out = 0;
    result = PerformQuery(adapter_handle, _buffer, &bytes_out);
    if (result == ERROR_GEN_FAILURE ||  
        result == ERROR_INSUFFICIENT_BUFFER ||
        result == ERROR_MORE_DATA ||
        result == NDIS_STATUS_INVALID_LENGTH ||
        result == NDIS_STATUS_BUFFER_TOO_SHORT) {
      
      
      size_t newSize;
      if (bytes_out > static_cast<DWORD>(_buffer.size())) {
        newSize = bytes_out;
      } else {
        newSize = _buffer.size() * 2;
      }
      if (!ResizeBuffer(newSize, _buffer)) {
        return false;
      }
    } else {
      
      break;
    }
  }

  if (result == ERROR_SUCCESS) {
    NDIS_802_11_BSSID_LIST* bssid_list =
        reinterpret_cast<NDIS_802_11_BSSID_LIST*>(&_buffer[0]);
    GetDataFromBssIdList(*bssid_list, _buffer.size(), outData);
  }

  return true;
}

namespace {
#define uint8 unsigned char

bool ConvertToAccessPointData(const NDIS_WLAN_BSSID& data, nsWifiAccessPoint* access_point_data)
{
  access_point_data->setMac(data.MacAddress);
  access_point_data->setSignal(data.Rssi);
  
  const unsigned char* ssid = data.Ssid.Ssid;
  size_t len = data.Ssid.SsidLength;
  access_point_data->setSSID(reinterpret_cast<const char*>(ssid), len);
  return true;
}

int GetDataFromBssIdList(const NDIS_802_11_BSSID_LIST& bss_id_list,
                         int list_size,
                         nsCOMArray<nsWifiAccessPoint>& outData)
{
  
  int found = 0;
  const uint8* iterator = reinterpret_cast<const uint8*>(&bss_id_list.Bssid[0]);
  const uint8* end_of_buffer =
      reinterpret_cast<const uint8*>(&bss_id_list) + list_size;
  for (int i = 0; i < static_cast<int>(bss_id_list.NumberOfItems); ++i) {
    const NDIS_WLAN_BSSID *bss_id =
        reinterpret_cast<const NDIS_WLAN_BSSID*>(iterator);
    
    if (bss_id->Length < sizeof(NDIS_WLAN_BSSID) ||
        iterator + bss_id->Length > end_of_buffer) {
      break;
    }
    nsWifiAccessPoint* ap = new nsWifiAccessPoint();
    if (ConvertToAccessPointData(*bss_id, ap)) {
      outData.AppendObject(ap);
      ++found;
    }
    
    iterator += bss_id->Length;
  }
  return found;
}


bool UndefineDosDevice(const std::string& device_name) {
  
  std::string target_path = "\\Device\\" + device_name;
  return DefineDosDevice(
      DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE,
      device_name.c_str(),
      target_path.c_str()) == TRUE;
}

bool DefineDosDeviceIfNotExists(const std::string& device_name) {
  
  std::string target_path = "\\Device\\" + device_name;

  TCHAR target[kStringLength];
  if (QueryDosDevice(device_name.c_str(), target, kStringLength) > 0 &&
      target_path.compare(target) == 0) {
    
    return true;
  }

  if (GetLastError() != ERROR_FILE_NOT_FOUND) {
    return false;
  }

  if (!DefineDosDevice(DDD_RAW_TARGET_PATH,
                       device_name.c_str(),
                       target_path.c_str())) {
    return false;
  }

  
  return QueryDosDevice(device_name.c_str(), target, kStringLength) > 0 &&
      target_path.compare(target) == 0;
}

HANDLE GetFileHandle(const std::string& device_name) {
  
  
  std::string formatted_device_name = "\\\\.\\" + device_name;

  return CreateFile(formatted_device_name.c_str(),
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,  
                    0,  
                    OPEN_EXISTING,
                    0,  
                    INVALID_HANDLE_VALUE);
}

int PerformQuery(HANDLE adapter_handle,
                 std::vector<char>& buffer,
                 DWORD* bytes_out) {
  DWORD oid = OID_802_11_BSSID_LIST;
  if (!DeviceIoControl(adapter_handle,
                       IOCTL_NDIS_QUERY_GLOBAL_STATS,
                       &oid,
                       sizeof(oid),
                       &buffer[0],
                       buffer.size(),
                       bytes_out,
                       NULL)) {
    return GetLastError();
  }
  return ERROR_SUCCESS;
}

bool ResizeBuffer(size_t requested_size, std::vector<char>& buffer) {
  if (requested_size > kMaximumBufferSize) {
    buffer.resize(kInitialBufferSize);
    return false;
  }

  buffer.resize(requested_size);
  return true;
}

} 


nsresult
WinXPWifiScanner::GetAccessPointsFromWLAN(nsCOMArray<nsWifiAccessPoint> &accessPoints)
{
  if (!mImplementation) {
    mImplementation = WindowsNdisApi::Create();
    if (!mImplementation) {
      return NS_ERROR_FAILURE;
    }
  }

  accessPoints.Clear();
  bool isOk = mImplementation->GetAccessPointData(accessPoints);
  if (!isOk) {
    mImplementation = 0;
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}
