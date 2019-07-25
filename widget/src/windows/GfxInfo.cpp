





































#include <windows.h>
#include "gfxWindowsPlatform.h"
#include "GfxInfo.h"
#include "nsUnicharUtils.h"
#include "mozilla/FunctionTimer.h"

#if defined(MOZ_CRASHREPORTER) && defined(MOZ_ENABLE_LIBXUL)
#include "nsExceptionHandler.h"
#include "nsICrashReporter.h"
#define NS_CRASHREPORTER_CONTRACTID "@mozilla.org/toolkit/crash-reporter;1"
#include "nsIPrefService.h"
#endif


using namespace mozilla::widget;

NS_IMPL_ISUPPORTS1(GfxInfo, nsIGfxInfo)



nsresult
GfxInfo::GetD2DEnabled(PRBool *aEnabled)
{
  *aEnabled = gfxWindowsPlatform::GetPlatform()->GetRenderMode() == gfxWindowsPlatform::RENDER_DIRECT2D;
  return NS_OK;
}

nsresult
GfxInfo::GetDWriteEnabled(PRBool *aEnabled)
{
  *aEnabled = gfxWindowsPlatform::GetPlatform()->DWriteEnabled();
  return NS_OK;
}



static const nsresult GetKeyValue(const WCHAR* keyLocation, const WCHAR* keyName, nsAString& destString, int type)
{
  HKEY key;
  DWORD dwcbData;
  DWORD dValue;
  DWORD resultType;
  LONG result;
  nsresult retval = NS_OK;

  result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyLocation, 0, KEY_QUERY_VALUE, &key);
  if (result != ERROR_SUCCESS) {
    return NS_ERROR_FAILURE;
  }

  switch (type) {
    case REG_DWORD: {
      
      dwcbData = sizeof(dValue);
      result = RegQueryValueExW(key, keyName, NULL, &resultType, (LPBYTE)&dValue, &dwcbData);
      if (result == ERROR_SUCCESS && resultType == REG_DWORD) {
        dValue = dValue / 1024 / 1024;
        destString.AppendInt(PRInt32(dValue));
      } else {
        retval = NS_ERROR_FAILURE;
      }
      break;
    }
    case REG_MULTI_SZ: {
      
      WCHAR wCharValue[1024];
      dwcbData = sizeof(wCharValue);

      result = RegQueryValueExW(key, keyName, NULL, &resultType, (LPBYTE)wCharValue, &dwcbData);
      if (result == ERROR_SUCCESS && resultType == REG_MULTI_SZ) {
        
        bool isValid = false;

        DWORD strLen = dwcbData/sizeof(wCharValue[0]);
        for (DWORD i = 0; i < strLen; i++) {
          if (wCharValue[i] == '\0') {
            if (i < strLen - 1 && wCharValue[i + 1] == '\0') {
              isValid = true;
              break;
            } else {
              wCharValue[i] = ' ';
            }
          }
        }

        
        wCharValue[strLen-1] = '\0';

        if (isValid)
          destString = wCharValue;

      } else {
        retval = NS_ERROR_FAILURE;
      }

      break;
    }
  }
  RegCloseKey(key);

  return retval;
}




static const void normalizeDriverId(nsString& driverid) {
  ToUpperCase(driverid);
  PRInt32 rev = driverid.Find(NS_LITERAL_CSTRING("&REV_"));
  if (rev != -1) {
    driverid.Cut(rev, driverid.Length());
  }
}









#define DEVICE_KEY_PREFIX L"\\Registry\\Machine\\"
void
GfxInfo::Init()
{
  NS_TIME_FUNCTION;

  DISPLAY_DEVICEW displayDevice;
  displayDevice.cb = sizeof(displayDevice);
  int deviceIndex = 0;

  while (EnumDisplayDevicesW(NULL, deviceIndex, &displayDevice, 0)) {
    if (displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
      break;
    deviceIndex++;
  }

  
  
  if (wcsncmp(displayDevice.DeviceKey, DEVICE_KEY_PREFIX, NS_ARRAY_LENGTH(DEVICE_KEY_PREFIX)-1) != 0)
    return;

  
  if (wcsnlen(displayDevice.DeviceKey, NS_ARRAY_LENGTH(displayDevice.DeviceKey))
      == NS_ARRAY_LENGTH(displayDevice.DeviceKey)) {
    
    return;
  }

  
  mDeviceKey = displayDevice.DeviceKey + NS_ARRAY_LENGTH(DEVICE_KEY_PREFIX)-1;

  mDeviceID = displayDevice.DeviceID;
  mDeviceString = displayDevice.DeviceString;


  HKEY key, subkey;
  LONG result, enumresult;
  DWORD index = 0;
  WCHAR subkeyname[64];
  WCHAR value[128];
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


  AddCrashReportAnnotations();
}


NS_IMETHODIMP
GfxInfo::GetAdapterDescription(nsAString & aAdapterDescription)
{
  aAdapterDescription = mDeviceString;
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM(nsAString & aAdapterRAM)
{
  if (NS_FAILED(GetKeyValue(mDeviceKey.BeginReading(), L"HardwareInformation.MemorySize", aAdapterRAM, REG_DWORD)))
    aAdapterRAM = L"Unknown";
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver(nsAString & aAdapterDriver)
{
  if (NS_FAILED(GetKeyValue(mDeviceKey.BeginReading(), L"InstalledDisplayDrivers", aAdapterDriver, REG_MULTI_SZ)))
    aAdapterDriver = L"Unknown";
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion(nsAString & aAdapterDriverVersion)
{
  aAdapterDriverVersion = mDriverVersion;
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate(nsAString & aAdapterDriverDate)
{
  aAdapterDriverDate = mDriverDate;
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID(PRUint32 *aAdapterVendorID)
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


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID(PRUint32 *aAdapterDeviceID)
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

void
GfxInfo::AddCrashReportAnnotations()
{
#if defined(MOZ_CRASHREPORTER) && defined(MOZ_ENABLE_LIBXUL)
  nsCAutoString deviceIDString, vendorIDString;
  PRUint32 deviceID, vendorID;

  GetAdapterDeviceID(&deviceID);
  GetAdapterVendorID(&vendorID);

  deviceIDString.AppendPrintf("%04x", deviceID);
  vendorIDString.AppendPrintf("%04x", vendorID);

  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterVendorID"),
      vendorIDString);
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterDeviceID"),
      deviceIDString);

  

  nsCAutoString note;
  
  note.AppendPrintf("AdapterVendorID: %04x, ", vendorID);
  note.AppendPrintf("AdapterDeviceID: %04x\n", deviceID);

  CrashReporter::AppendAppNotesToCrashReport(note);

#endif
}

enum VersionComparisonOp {
  DRIVER_LESS_THAN,             
  DRIVER_LESS_THAN_OR_EQUAL,    
  DRIVER_GREATER_THAN,          
  DRIVER_GREATER_THAN_OR_EQUAL, 
  DRIVER_EQUAL,                 
  DRIVER_NOT_EQUAL,             
  DRIVER_BETWEEN_EXCLUSIVE,     
  DRIVER_BETWEEN_INCLUSIVE,     
  DRIVER_BETWEEN_INCLUSIVE_START 
};

typedef const PRUint32 *GfxDeviceFamily;

struct GfxDriverInfo {
  PRUint32 windowsVersion;

  PRUint32 vendor;
  GfxDeviceFamily devices;

  PRInt32 feature;
  PRInt32 featureStatus;

  VersionComparisonOp op;

  
  PRUint64 version;
  PRUint64 versionMax;
};

static const PRUint32 allWindowsVersions = 0xffffffff;
static const PRInt32  allFeatures = -1;
static const PRUint32 *allDevices = (PRUint32*) nsnull;
static const PRUint64 allDriverVersions = 0xffffffffffffffffULL;


static const PRUint32 vendorIntel = 0x8086;





#define V(a,b,c,d)   ((PRUint64(a)<<48) | (PRUint64(b)<<32) | (PRUint64(c)<<16) | PRUint64(d))

static const PRUint32 deviceFamilyIntelGMA500[] = {
    0x8108, 
    0x8109, 
    0
};

static const PRUint32 deviceFamilyIntelGMA900[] = {
    0x2582, 
    0x2782, 
    0x2592, 
    0x2792, 
    0
};

static const PRUint32 deviceFamilyIntelGMA950[] = {
    0x2772, 
    0x2776, 
    0x27A2, 
    0x27A6, 
    0x27AE, 
    0
};

static const PRUint32 deviceFamilyIntelGMA3150[] = {
    0xA001, 
    0xA002, 
    0xA011, 
    0xA012, 
    0
};

static const PRUint32 deviceFamilyIntelGMAX3000[] = {
    0x2972, 
    0x2973, 
    0x2982, 
    0x2983, 
    0x2992, 
    0x2993, 
    0x29A2, 
    0x29A3, 
    0x29B2, 
    0x29B3, 
    0x29C2, 
    0x29C3, 
    0x29D2, 
    0x29D3, 
    0x2A02, 
    0x2A03, 
    0x2A12, 
    0x2A13, 
    0
};

static const PRUint32 deviceFamilyIntelGMAX4500HD[] = {
    0x2A42, 
    0x2A43, 
    0x2E42, 
    0x2E43, 
    0x2E92, 
    0x2E93, 
    0x2E32, 
    0x2E33, 
    0x2E22, 
    0x2E23, 
    0x2E12, 
    0x2E13, 
    0x0042, 
    0x0046, 
    0x0102, 
    0x0106, 
    0x0112, 
    0x0116, 
    0x0122, 
    0x0126, 
    0x010A, 
    0x0080, 
    0
};

static const GfxDriverInfo driverInfo[] = {
  



  



#define IMPLEMENT_INTEL_DRIVER_BLOCKLIST(winVer, devFamily, driverVer) \
  { winVer,                                                            \
    vendorIntel, devFamily,                                            \
    allFeatures, nsIGfxInfo::FEATURE_BLOCKED,                          \
    DRIVER_LESS_THAN, driverVer },

  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsXP, deviceFamilyIntelGMA500,   V(6,14,11,1018))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsXP, deviceFamilyIntelGMA900,   V(6,14,10,4764))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsXP, deviceFamilyIntelGMA950,   V(6,14,10,4926))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsXP, deviceFamilyIntelGMA3150,  V(6,14,10,5260))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsXP, deviceFamilyIntelGMAX3000, V(6,14,10,5218))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsXP, deviceFamilyIntelGMAX4500HD, V(6,14,10,5284))

  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsVista, deviceFamilyIntelGMA500,   V(7,14,10,1006))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsVista, deviceFamilyIntelGMA900,   allDriverVersions)
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsVista, deviceFamilyIntelGMA950,   V(7,14,10,1504))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsVista, deviceFamilyIntelGMA3150,  V(7,14,10,2124))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsVista, deviceFamilyIntelGMAX3000, V(7,15,10,1666))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindowsVista, deviceFamilyIntelGMAX4500HD, V(8,15,10,2182))

  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindows7, deviceFamilyIntelGMA500,   V(5,0,0,2026))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindows7, deviceFamilyIntelGMA900,   allDriverVersions)
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindows7, deviceFamilyIntelGMA950,   V(8,15,10,1930))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindows7, deviceFamilyIntelGMA3150,  V(8,14,10,2117))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindows7, deviceFamilyIntelGMAX3000, V(8,15,10,1930))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(gfxWindowsPlatform::kWindows7, deviceFamilyIntelGMAX4500HD, V(8,15,10,2182))

  
  { allWindowsVersions,
    vendorIntel, allDevices,
    nsIGfxInfo::FEATURE_OPENGL_LAYERS, nsIGfxInfo::FEATURE_NOT_SUGGESTED,
    DRIVER_LESS_THAN, allDriverVersions },
  { allWindowsVersions,
    vendorIntel, allDevices,
    nsIGfxInfo::FEATURE_WEBGL_OPENGL, nsIGfxInfo::FEATURE_NOT_SUGGESTED,
    DRIVER_LESS_THAN, allDriverVersions },

  



  



  { 0, 0, allDevices, 0 }
};

static bool
ParseDriverVersion(nsAString& aVersion, PRUint64 *aNumericVersion)
{
  int a, b, c, d;
  
  if (sscanf(nsPromiseFlatCString(NS_LossyConvertUTF16toASCII(aVersion)).get(),
             "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
    return false;
  if (a < 0 || a > 0xffff) return false;
  if (b < 0 || b > 0xffff) return false;
  if (c < 0 || c > 0xffff) return false;
  if (d < 0 || d > 0xffff) return false;

  *aNumericVersion = V(a, b, c, d);
  return true;
}

NS_IMETHODIMP
GfxInfo::GetFeatureStatus(PRInt32 aFeature, PRInt32 *aStatus)
{
  PRInt32 status = nsIGfxInfo::FEATURE_STATUS_UNKNOWN;

  PRUint32 adapterVendor = 0;
  PRUint32 adapterDeviceID = 0;
  nsAutoString adapterDriverVersionString;
  if (NS_FAILED(GetAdapterVendorID(&adapterVendor)) ||
      NS_FAILED(GetAdapterDeviceID(&adapterDeviceID)) ||
      NS_FAILED(GetAdapterDriverVersion(adapterDriverVersionString)))
  {
    return NS_ERROR_FAILURE;
  }

  PRUint64 driverVersion;
  if (!ParseDriverVersion(adapterDriverVersionString, &driverVersion)) {
    return NS_ERROR_FAILURE;
  }

  const GfxDriverInfo *info = &driverInfo[0];
  while (info->windowsVersion) {

    if (info->windowsVersion != allWindowsVersions &&
        info->windowsVersion != gfxWindowsPlatform::WindowsOSVersion())
    {
      info++;
      continue;
    }

    if (info->vendor != adapterVendor) {
      info++;
      continue;
    }

    if (info->devices != allDevices) {
        bool deviceMatches = false;
        for (const PRUint32 *devices = info->devices; *devices; ++devices) {
            if (*devices == adapterDeviceID) {
                deviceMatches = true;
                break;
            }
        }

        if (!deviceMatches) {
            info++;
            continue;
        }
    }

    bool match = false;

    switch (info->op) {
    case DRIVER_LESS_THAN:
      match = driverVersion < info->version;
      break;
    case DRIVER_LESS_THAN_OR_EQUAL:
      match = driverVersion <= info->version;
      break;
    case DRIVER_GREATER_THAN:
      match = driverVersion > info->version;
      break;
    case DRIVER_GREATER_THAN_OR_EQUAL:
      match = driverVersion >= info->version;
      break;
    case DRIVER_EQUAL:
      match = driverVersion == info->version;
      break;
    case DRIVER_NOT_EQUAL:
      match = driverVersion != info->version;
      break;
    case DRIVER_BETWEEN_EXCLUSIVE:
      match = driverVersion > info->version && driverVersion < info->versionMax;
      break;
    case DRIVER_BETWEEN_INCLUSIVE:
      match = driverVersion >= info->version && driverVersion <= info->versionMax;
      break;
    case DRIVER_BETWEEN_INCLUSIVE_START:
      match = driverVersion >= info->version && driverVersion < info->versionMax;
      break;
    default:
      NS_WARNING("Bogus op in GfxDriverInfo");
      break;
    }

    if (match) {
      if (info->feature == allFeatures ||
          info->feature == aFeature)
      {
        status = info->featureStatus;
        break;
      }
    }

    info++;
  }

  *aStatus = status;
  return NS_OK;
}

