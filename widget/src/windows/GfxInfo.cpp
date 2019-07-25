





































#include <windows.h>
#include "gfxWindowsPlatform.h"
#include "GfxInfo.h"
#include "nsUnicharUtils.h"
#include "mozilla/FunctionTimer.h"

using namespace mozilla::widget;

NS_IMPL_ISUPPORTS1(GfxInfo, nsIGfxInfo)

nsresult GfxInfo::GetD2DEnabled(PRBool *aEnabled)
{
  *aEnabled = gfxWindowsPlatform::GetPlatform()->GetRenderMode() == gfxWindowsPlatform::RENDER_DIRECT2D;
  return NS_OK;
}

nsresult GfxInfo::GetDWriteEnabled(PRBool *aEnabled)
{
  *aEnabled = gfxWindowsPlatform::GetPlatform()->DWriteEnabled();
  return NS_OK;
}



static nsresult GetKeyValue(const TCHAR* keyLocation, const TCHAR* keyName, nsAString& destString, int type)
{
  HKEY key;
  DWORD dwcbData;
  WCHAR wCharValue[1024];
  TCHAR tCharValue[1024];
  DWORD dValue;
  LONG result;
  nsresult retval = NS_OK;

  result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyLocation, 0, KEY_QUERY_VALUE, &key);
  if (result != ERROR_SUCCESS) {
    return NS_ERROR_FAILURE;
  }

  switch (type) {
    case REG_DWORD: {
      
      dwcbData = sizeof(dValue);
      result = RegQueryValueExW(key, keyName, NULL, NULL, (LPBYTE)&dValue, &dwcbData);
      if (result != ERROR_SUCCESS) {
        retval = NS_ERROR_FAILURE;
      }
      dValue = dValue / 1024 / 1024;
      destString.AppendInt(static_cast<PRInt32>(dValue));
      break;
    }
    case REG_MULTI_SZ: {
      
      dwcbData = sizeof(tCharValue);
      result = RegQueryValueExW(key, keyName, NULL, NULL, (LPBYTE)tCharValue, &dwcbData);
      if (result != ERROR_SUCCESS) {
        retval = NS_ERROR_FAILURE;
      }
      
      for (DWORD i = 0, len = dwcbData/sizeof(tCharValue[0]); i < len; i++) {
        if (tCharValue[i] == '\0')
          tCharValue[i] = ' ';
      }
      destString = tCharValue;
      break;
    }
  }
  RegCloseKey(key);

  return retval;
}




static void normalizeDriverId(nsString& driverid) {
  ToUpperCase(driverid);
  PRInt32 rev = driverid.Find(NS_LITERAL_CSTRING("&REV_"));
  if (rev != -1) {
    driverid.Cut(rev, driverid.Length());
  }
}









#define DEVICE_KEY_PREFIX L"\\Registry\\Machine\\"
void GfxInfo::Init()
{
  NS_TIME_FUNCTION;

  DISPLAY_DEVICE lpDisplayDevice;
  lpDisplayDevice.cb = sizeof(lpDisplayDevice);
  int deviceIndex = 0;

  while (EnumDisplayDevices(NULL, deviceIndex, &lpDisplayDevice, 0)) {
    if (lpDisplayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
      break;
    deviceIndex++;
  }

  
  if (wcsncmp(lpDisplayDevice.DeviceKey, DEVICE_KEY_PREFIX, wcslen(DEVICE_KEY_PREFIX)) != 0)
    return;

  
  size_t i;
  for (i = 0; i < sizeof(lpDisplayDevice.DeviceKey); i++) {
    if (lpDisplayDevice.DeviceKey[i] == L'\0')
      break;
  }

  if (i == sizeof(lpDisplayDevice.DeviceKey)) {
      
      return;
  }

  
  mDeviceKey = lpDisplayDevice.DeviceKey + wcslen(DEVICE_KEY_PREFIX);

  mDeviceID = lpDisplayDevice.DeviceID;
  mDeviceString = lpDisplayDevice.DeviceString;


  HKEY key, subkey;
  LONG result, enumresult;
  DWORD index = 0;
  TCHAR subkeyname[64];
  TCHAR value[128];
  DWORD dwcbData = sizeof(subkeyname);

  
  result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Control\\Class\\{4D36E968-E325-11CE-BFC1-08002BE10318}", 
                        0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &key);
  if (result != ERROR_SUCCESS) {
    return;
  }

  nsAutoString wantedDriverId(mDeviceID);
  normalizeDriverId(wantedDriverId);

  while ((enumresult = RegEnumKeyExW(key, index, subkeyname, &dwcbData, NULL, NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS) {
    result = RegOpenKeyExW(key, subkeyname, 0, KEY_QUERY_VALUE, &subkey);
    if (result == ERROR_SUCCESS) {
      dwcbData = sizeof(value);
      result = RegQueryValueExW(subkey, L"MatchingDeviceId", NULL, NULL, (LPBYTE)value, &dwcbData);
      if (result == ERROR_SUCCESS) {
        nsAutoString matchingDeviceId(value);
        normalizeDriverId(matchingDeviceId);
        if (wantedDriverId.Find(matchingDeviceId) > -1) {
          
          result = RegQueryValueExW(subkey, L"DriverVersion", NULL, NULL, (LPBYTE)value, &dwcbData);
          if (result == ERROR_SUCCESS)
            mDriverVersion = value;
          result = RegQueryValueExW(subkey, L"DriverDate", NULL, NULL, (LPBYTE)value, &dwcbData);
          if (result == ERROR_SUCCESS)
            mDriverDate = value;
          break;
        }
      }
      RegCloseKey(subkey);
    }
    index++;
    dwcbData = sizeof(subkeyname);
  }

  RegCloseKey(key);
}


NS_IMETHODIMP GfxInfo::GetAdapterDescription(nsAString & aAdapterDescription)
{
  aAdapterDescription = mDeviceString;
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::GetAdapterRAM(nsAString & aAdapterRAM)
{
  if (NS_FAILED(GetKeyValue(mDeviceKey.BeginReading(), L"HardwareInformation.MemorySize", aAdapterRAM, REG_DWORD)))
    aAdapterRAM = L"Unknown";
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::GetAdapterDriver(nsAString & aAdapterDriver)
{
  if (NS_FAILED(GetKeyValue(mDeviceKey.BeginReading(), L"InstalledDisplayDrivers", aAdapterDriver, REG_MULTI_SZ)))
    aAdapterDriver = L"Unknown";
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::GetAdapterDriverVersion(nsAString & aAdapterDriverVersion)
{
  aAdapterDriverVersion = mDriverVersion;
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::GetAdapterDriverDate(nsAString & aAdapterDriverDate)
{
  aAdapterDriverDate = mDriverDate;
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::GetAdapterVendorID(PRUint32 *aAdapterVendorID)
{
  nsAutoString vendor(mDeviceID);
  ToUpperCase(vendor);
  PRInt32 start = vendor.Find(NS_LITERAL_CSTRING("VEN_"));
  if (start != -1) {
    vendor.Cut(0, start + strlen("VEN_"));
    vendor.Truncate(4);
  }
  nsresult err;
  *aAdapterVendorID = vendor.ToInteger(&err, 16);
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::GetAdapterDeviceID(PRUint32 *aAdapterDeviceID)
{
  nsAutoString device(mDeviceID);
  ToUpperCase(device);
  PRInt32 start = device.Find(NS_LITERAL_CSTRING("&DEV_"));
  if (start != -1) {
    device.Cut(0, start + strlen("&DEV_"));
    device.Truncate(4);
  }
  nsresult err;
  *aAdapterDeviceID = device.ToInteger(&err, 16);
  return NS_OK;
}
