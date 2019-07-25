





































#include <windows.h>
#include <setupapi.h>
#include "gfxWindowsPlatform.h"
#include "GfxInfo.h"
#include "GfxInfoWebGL.h"
#include "nsUnicharUtils.h"
#include "nsPrintfCString.h"
#include "mozilla/FunctionTimer.h"
#include "prenv.h"
#include "prprf.h"
#include "GfxDriverInfo.h"
#include "mozilla/Preferences.h"

#if defined(MOZ_CRASHREPORTER)
#include "nsExceptionHandler.h"
#include "nsICrashReporter.h"
#define NS_CRASHREPORTER_CONTRACTID "@mozilla.org/toolkit/crash-reporter;1"
#endif


using namespace mozilla::widget;

#ifdef DEBUG
NS_IMPL_ISUPPORTS_INHERITED1(GfxInfo, GfxInfoBase, nsIGfxInfoDebug)
#endif

static const PRUint32 allWindowsVersions = 0xffffffff;
static const PRUint64 allDriverVersions = 0xffffffffffffffffULL;

static const PRUint32 vendorIntel = 0x8086;

static const PRUint32 vendorNVIDIA = 0x10de;

static const PRUint32 vendorAMD = 0x1022;
static const PRUint32 vendorATI = 0x1002;

#define V(a,b,c,d) GFX_DRIVER_VERSION(a,b,c,d)


GfxInfo::GfxInfo()
  : mAdapterVendorID(0),
    mAdapterDeviceID(0),
    mWindowsVersion(0)
{
}



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

nsresult
GfxInfo::GetAzureEnabled(PRBool *aEnabled)
{
  *aEnabled = PR_FALSE;

  PRBool d2dEnabled = 
    gfxWindowsPlatform::GetPlatform()->GetRenderMode() == gfxWindowsPlatform::RENDER_DIRECT2D;

  if (d2dEnabled) {
    PRBool azure = PR_FALSE;
    nsresult rv = mozilla::Preferences::GetBool("gfx.canvas.azure.enabled", &azure);

    if (NS_SUCCEEDED(rv) && azure) {
      *aEnabled = PR_TRUE;
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetDWriteVersion(nsAString & aDwriteVersion)
{
  gfxWindowsPlatform::GetDLLVersion(L"dwrite.dll", aDwriteVersion);
  return NS_OK;
}

#define PIXEL_STRUCT_RGB  1
#define PIXEL_STRUCT_BGR  2


NS_IMETHODIMP
GfxInfo::GetCleartypeParameters(nsAString & aCleartypeParams)
{
  nsTArray<ClearTypeParameterInfo> clearTypeParams;

  gfxWindowsPlatform::GetPlatform()->GetCleartypeParams(clearTypeParams);
  PRUint32 d, numDisplays = clearTypeParams.Length();
  bool displayNames = (numDisplays > 1);
  bool foundData = false;
  nsString outStr;
  WCHAR valStr[256];

  for (d = 0; d < numDisplays; d++) {
    ClearTypeParameterInfo& params = clearTypeParams[d];

    if (displayNames) {
      swprintf_s(valStr, NS_ARRAY_LENGTH(valStr),
                 L"%s [ ", params.displayName.get());
      outStr.Append(valStr);
    }

    if (params.gamma >= 0) {
      foundData = true;
      swprintf_s(valStr, NS_ARRAY_LENGTH(valStr),
                 L"Gamma: %d ", params.gamma);
      outStr.Append(valStr);
    }

    if (params.pixelStructure >= 0) {
      foundData = true;
      if (params.pixelStructure == PIXEL_STRUCT_RGB ||
          params.pixelStructure == PIXEL_STRUCT_BGR)
      {
        swprintf_s(valStr, NS_ARRAY_LENGTH(valStr),
                   L"Pixel Structure: %s ",
                   (params.pixelStructure == PIXEL_STRUCT_RGB ?
                      L"RGB" : L"BGR"));
      } else {
        swprintf_s(valStr, NS_ARRAY_LENGTH(valStr),
                   L"Pixel Structure: %d ", params.pixelStructure);
      }
      outStr.Append(valStr);
    }

    if (params.clearTypeLevel >= 0) {
      foundData = true;
      swprintf_s(valStr, NS_ARRAY_LENGTH(valStr),
                 L"ClearType Level: %d ", params.clearTypeLevel);
      outStr.Append(valStr);
    }

    if (params.enhancedContrast >= 0) {
      foundData = true;
      swprintf_s(valStr, NS_ARRAY_LENGTH(valStr),
                 L"Enhanced Contrast: %d ", params.enhancedContrast);
      outStr.Append(valStr);
    }

    if (displayNames) {
      outStr.Append(L"] ");
    }
  }

  if (foundData) {
    aCleartypeParams.Assign(outStr);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}



static nsresult GetKeyValue(const WCHAR* keyLocation, const WCHAR* keyName, nsAString& destString, int type)
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




static void normalizeDriverId(nsString& driverid) {
  ToUpperCase(driverid);
  PRInt32 rev = driverid.Find(NS_LITERAL_CSTRING("&REV_"));
  if (rev != -1) {
    driverid.Cut(rev, driverid.Length());
  }
}


typedef HDEVINFO (WINAPI*SetupDiGetClassDevsWFunc)(
  CONST GUID *ClassGuid,
  PCWSTR Enumerator,
  HWND hwndParent,
  DWORD Flags
);
typedef BOOL (WINAPI*SetupDiEnumDeviceInfoFunc)(
  HDEVINFO DeviceInfoSet,
  DWORD MemberIndex,
  PSP_DEVINFO_DATA DeviceInfoData
);
typedef BOOL (WINAPI*SetupDiGetDeviceRegistryPropertyWFunc)(
  HDEVINFO DeviceInfoSet,
  PSP_DEVINFO_DATA DeviceInfoData,
  DWORD Property,
  PDWORD PropertyRegDataType,
  PBYTE PropertyBuffer,
  DWORD PropertyBufferSize,
  PDWORD RequiredSize
);
typedef BOOL (WINAPI*SetupDiDestroyDeviceInfoListFunc)(
  HDEVINFO DeviceInfoSet
);









#define DEVICE_KEY_PREFIX L"\\Registry\\Machine\\"
nsresult
GfxInfo::Init()
{
  NS_TIME_FUNCTION;

  nsresult rv = GfxInfoBase::Init();

  DISPLAY_DEVICEW displayDevice;
  displayDevice.cb = sizeof(displayDevice);
  int deviceIndex = 0;

  mDeviceKeyDebug = NS_LITERAL_STRING("PrimarySearch");

  while (EnumDisplayDevicesW(NULL, deviceIndex, &displayDevice, 0)) {
    if (displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
      mDeviceKeyDebug = NS_LITERAL_STRING("NullSearch");
      break;
    }
    deviceIndex++;
  }

  
  if (wcsnlen(displayDevice.DeviceKey, NS_ARRAY_LENGTH(displayDevice.DeviceKey))
      == NS_ARRAY_LENGTH(displayDevice.DeviceKey)) {
    
    return rv;
  }

  mDeviceKeyDebug = displayDevice.DeviceKey;

  
  
  
  if (_wcsnicmp(displayDevice.DeviceKey, DEVICE_KEY_PREFIX, NS_ARRAY_LENGTH(DEVICE_KEY_PREFIX)-1) != 0)
    return rv;

  
  mDeviceKey = displayDevice.DeviceKey + NS_ARRAY_LENGTH(DEVICE_KEY_PREFIX)-1;

  mDeviceID = displayDevice.DeviceID;
  mDeviceString = displayDevice.DeviceString;


  HMODULE setupapi = LoadLibraryW(L"setupapi.dll");

  if (setupapi) {
    SetupDiGetClassDevsWFunc setupGetClassDevs = (SetupDiGetClassDevsWFunc)
      GetProcAddress(setupapi, "SetupDiGetClassDevsW");
    SetupDiEnumDeviceInfoFunc setupEnumDeviceInfo = (SetupDiEnumDeviceInfoFunc)
      GetProcAddress(setupapi, "SetupDiEnumDeviceInfo");
    SetupDiGetDeviceRegistryPropertyWFunc setupGetDeviceRegistryProperty = (SetupDiGetDeviceRegistryPropertyWFunc)
      GetProcAddress(setupapi, "SetupDiGetDeviceRegistryPropertyW");
    SetupDiDestroyDeviceInfoListFunc setupDestroyDeviceInfoList = (SetupDiDestroyDeviceInfoListFunc)
      GetProcAddress(setupapi, "SetupDiDestroyDeviceInfoList");

    if (setupGetClassDevs &&
        setupEnumDeviceInfo &&
        setupGetDeviceRegistryProperty &&
        setupDestroyDeviceInfoList) {
      
      HDEVINFO devinfo = setupGetClassDevs(NULL, mDeviceID.get(), NULL,
                                           DIGCF_PRESENT | DIGCF_PROFILE | DIGCF_ALLCLASSES);

      if (devinfo != INVALID_HANDLE_VALUE) {
        HKEY key;
        LONG result;
        WCHAR value[255];
        DWORD dwcbData;
        SP_DEVINFO_DATA devinfoData;
        DWORD memberIndex = 0;

        devinfoData.cbSize = sizeof(devinfoData);
        NS_NAMED_LITERAL_STRING(driverKeyPre, "System\\CurrentControlSet\\Control\\Class\\");
        
        while (setupEnumDeviceInfo(devinfo, memberIndex++, &devinfoData)) {
          
          if (setupGetDeviceRegistryProperty(devinfo,
                                             &devinfoData,
                                             SPDRP_DRIVER,
                                             NULL,
                                             (PBYTE)value,
                                             sizeof(value),
                                             NULL)) {
            nsAutoString driverKey(driverKeyPre);
            driverKey += value;
            result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, driverKey.BeginReading(), 0, KEY_QUERY_VALUE, &key);
            if (result == ERROR_SUCCESS) {
              
              dwcbData = sizeof(value);
              result = RegQueryValueExW(key, L"DriverVersion", NULL, NULL, (LPBYTE)value, &dwcbData);
              if (result == ERROR_SUCCESS)
                mDriverVersion = value;
              dwcbData = sizeof(value);
              result = RegQueryValueExW(key, L"DriverDate", NULL, NULL, (LPBYTE)value, &dwcbData);
              if (result == ERROR_SUCCESS)
                mDriverDate = value;
              RegCloseKey(key);
              break;
            }
          }
        }

        setupDestroyDeviceInfoList(devinfo);
      }
    }

    FreeLibrary(setupapi);
  }

  const char *spoofedDriverVersionString = PR_GetEnv("MOZ_GFX_SPOOF_DRIVER_VERSION");
  if (spoofedDriverVersionString) {
    mDriverVersion.AssignASCII(spoofedDriverVersionString);
  }

  const char *spoofedVendor = PR_GetEnv("MOZ_GFX_SPOOF_VENDOR_ID");
  if (spoofedVendor) {
     PR_sscanf(spoofedVendor, "%x", &mAdapterVendorID);
  } else {
    nsAutoString vendor(mDeviceID);
    ToUpperCase(vendor);
    PRInt32 start = vendor.Find(NS_LITERAL_CSTRING("VEN_"));
    if (start != -1) {
      vendor.Cut(0, start + strlen("VEN_"));
      vendor.Truncate(4);
    }
    nsresult err;
    mAdapterVendorID = vendor.ToInteger(&err, 16);
  }

  mHasDriverVersionMismatch = PR_FALSE;
  if (mAdapterVendorID == vendorIntel) {
    
    
    
    PRBool is64bitApp = sizeof(void*) == 8;
    const PRUnichar *dllFileName = is64bitApp
                                 ? L"igd10umd64.dll"
                                 : L"igd10umd32.dll";
    nsString dllVersion;
    gfxWindowsPlatform::GetDLLVersion((PRUnichar*)dllFileName, dllVersion);

    PRUint64 dllNumericVersion = 0, driverNumericVersion = 0;
    ParseDriverVersion(dllVersion, &dllNumericVersion);
    ParseDriverVersion(mDriverVersion, &driverNumericVersion);

    
    
    
    if (dllNumericVersion != driverNumericVersion)
      mHasDriverVersionMismatch = PR_TRUE;
  }

  const char *spoofedDevice = PR_GetEnv("MOZ_GFX_SPOOF_DEVICE_ID");
  if (spoofedDevice) {
    PR_sscanf(spoofedDevice, "%x", &mAdapterDeviceID);
  } else {
    nsAutoString device(mDeviceID);
    ToUpperCase(device);
    PRInt32 start = device.Find(NS_LITERAL_CSTRING("&DEV_"));
    if (start != -1) {
      device.Cut(0, start + strlen("&DEV_"));
      device.Truncate(4);
    }
    nsresult err;
    mAdapterDeviceID = device.ToInteger(&err, 16);
  }

  const char *spoofedWindowsVersion = PR_GetEnv("MOZ_GFX_SPOOF_WINDOWS_VERSION");
  if (spoofedWindowsVersion) {
    PR_sscanf(spoofedWindowsVersion, "%x", &mWindowsVersion);
  } else {
    mWindowsVersion = gfxWindowsPlatform::WindowsOSVersion();
  }

  AddCrashReportAnnotations();

  return rv;
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
  *aAdapterVendorID = mAdapterVendorID;
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID(PRUint32 *aAdapterDeviceID)
{
  *aAdapterDeviceID = mAdapterDeviceID;
  return NS_OK;
}

#if defined(MOZ_CRASHREPORTER)



static void
CheckForCiscoVPN() {
  LONG result;
  HKEY key;
  
  result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Cisco Systems\\VPN Client", 0, KEY_QUERY_VALUE, &key);
  if (result == ERROR_SUCCESS) {
    RegCloseKey(key);
    CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING("Cisco VPN\n"));
  }
}
#endif

void
GfxInfo::AddCrashReportAnnotations()
{
#if defined(MOZ_CRASHREPORTER)
  CheckForCiscoVPN();

  nsCAutoString deviceIDString, vendorIDString;
  PRUint32 deviceID, vendorID;
  nsAutoString adapterDriverVersionString;

  GetAdapterDeviceID(&deviceID);
  GetAdapterVendorID(&vendorID);
  GetAdapterDriverVersion(adapterDriverVersionString);

  deviceIDString.AppendPrintf("%04x", deviceID);
  vendorIDString.AppendPrintf("%04x", vendorID);

  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterVendorID"),
      vendorIDString);
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterDeviceID"),
      deviceIDString);
  
  

  nsCAutoString note;
  
  note.AppendPrintf("AdapterVendorID: %04x, ", vendorID);
  note.AppendPrintf("AdapterDeviceID: %04x, ", deviceID);
  note.AppendPrintf("AdapterDriverVersion: ");
  note.Append(NS_LossyConvertUTF16toASCII(adapterDriverVersionString));

  if (vendorID == 0) {
      
      note.Append(", ");
      note.AppendWithConversion(mDeviceID);
      note.Append(", ");
      note.AppendWithConversion(mDeviceKeyDebug);
  }
  note.Append("\n");

  CrashReporter::AppendAppNotesToCrashReport(note);

#endif
}

#define V(a,b,c,d) GFX_DRIVER_VERSION(a,b,c,d)

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


static const PRUint32 deviceFamilyIntelBlockDirect2D[] = {
    0x2982, 
    0x2983, 
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


static const PRUint32 deviceFamilyNvidiaBlockD3D9Layers[] = {
    0x00f3, 
    0x0146, 
    0x014f, 
    0x0161, 
    0x0162, 
    0x0163, 
    0x0164, 
    0x0167, 
    0x0168, 
    0x0169, 
    0x0221, 
    0x0222, 
    0x0240, 
    0x0241, 
    0x0242, 
    0x0244, 
    0x0245, 
    0x0247, 
    0x03d0, 
    0x03d1, 
    0x03d2, 
    0x03d5, 
    0
};

static const GfxDriverInfo gDriverInfo[] = {
  



  


  GfxDriverInfo( DRIVER_OS_WINDOWS_XP,
    vendorNVIDIA, GfxDriverInfo::allDevices,
    GfxDriverInfo::allFeatures, nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION,
    DRIVER_LESS_THAN, V(6,14,12,5721), "257.21" ),
  GfxDriverInfo( DRIVER_OS_WINDOWS_VISTA,
    vendorNVIDIA, GfxDriverInfo::allDevices,
    GfxDriverInfo::allFeatures, nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION,
    DRIVER_LESS_THAN, V(8,17,12,5721), "257.21" ),
  GfxDriverInfo( DRIVER_OS_WINDOWS_7,
    vendorNVIDIA, GfxDriverInfo::allDevices,
    GfxDriverInfo::allFeatures, nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION,
    DRIVER_LESS_THAN, V(8,17,12,5721), "257.21" ),

  


  GfxDriverInfo( DRIVER_OS_ALL,
    vendorNVIDIA, (GfxDeviceFamily) deviceFamilyNvidiaBlockD3D9Layers,
    nsIGfxInfo::FEATURE_DIRECT3D_9_LAYERS, nsIGfxInfo::FEATURE_BLOCKED_DEVICE,
    DRIVER_LESS_THAN, allDriverVersions ),

  


  GfxDriverInfo( DRIVER_OS_ALL,
    vendorATI, GfxDriverInfo::allDevices,
    GfxDriverInfo::allFeatures, nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION,
    DRIVER_LESS_THAN, V(8,741,0,0), "10.6" ),
  GfxDriverInfo( DRIVER_OS_ALL,
    vendorAMD, GfxDriverInfo::allDevices,
    GfxDriverInfo::allFeatures, nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION,
    DRIVER_LESS_THAN, V(8,741,0,0), "10.6" ),

  





  GfxDriverInfo( DRIVER_OS_ALL,
    vendorATI, GfxDriverInfo::allDevices,
    nsIGfxInfo::FEATURE_OPENGL_LAYERS, nsIGfxInfo::FEATURE_DISCOURAGED,
    DRIVER_LESS_THAN, allDriverVersions ),
  GfxDriverInfo( DRIVER_OS_ALL,
    vendorATI, GfxDriverInfo::allDevices,
    nsIGfxInfo::FEATURE_WEBGL_OPENGL, nsIGfxInfo::FEATURE_DISCOURAGED,
    DRIVER_LESS_THAN, allDriverVersions ),
  GfxDriverInfo( DRIVER_OS_ALL,
    vendorAMD, GfxDriverInfo::allDevices,
    nsIGfxInfo::FEATURE_OPENGL_LAYERS, nsIGfxInfo::FEATURE_DISCOURAGED,
    DRIVER_LESS_THAN, allDriverVersions ),
  GfxDriverInfo( DRIVER_OS_ALL,
    vendorAMD, GfxDriverInfo::allDevices,
    nsIGfxInfo::FEATURE_WEBGL_OPENGL, nsIGfxInfo::FEATURE_DISCOURAGED,
    DRIVER_LESS_THAN, allDriverVersions ),

  



  


  GfxDriverInfo( DRIVER_OS_ALL,
    vendorIntel, (GfxDeviceFamily) deviceFamilyIntelBlockDirect2D,
    nsIGfxInfo::FEATURE_DIRECT2D, nsIGfxInfo::FEATURE_BLOCKED_DEVICE,
    DRIVER_LESS_THAN, allDriverVersions ),

  



#define IMPLEMENT_INTEL_DRIVER_BLOCKLIST(winVer, devFamily, driverVer) \
  GfxDriverInfo( winVer,                                               \
    vendorIntel, (GfxDeviceFamily) devFamily,                          \
    GfxDriverInfo::allFeatures, nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION,           \
    DRIVER_LESS_THAN, driverVer ),

  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_XP, deviceFamilyIntelGMA500,   V(6,14,11,1018))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_XP, deviceFamilyIntelGMA900,   V(6,14,10,4764))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_XP, deviceFamilyIntelGMA950,   V(6,14,10,4926))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_XP, deviceFamilyIntelGMA3150,  V(6,14,10,5260))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_XP, deviceFamilyIntelGMAX3000, V(6,14,10,5218))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_XP, deviceFamilyIntelGMAX4500HD, V(6,14,10,5284))

  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_VISTA, deviceFamilyIntelGMA500,   V(7,14,10,1006))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_VISTA, deviceFamilyIntelGMA900,   allDriverVersions)
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_VISTA, deviceFamilyIntelGMA950,   V(7,14,10,1504))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_VISTA, deviceFamilyIntelGMA3150,  V(7,14,10,2124))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_VISTA, deviceFamilyIntelGMAX3000, V(7,15,10,1666))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_VISTA, deviceFamilyIntelGMAX4500HD, V(8,15,10,2202))

  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_7, deviceFamilyIntelGMA500,   V(5,0,0,2026))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_7, deviceFamilyIntelGMA900,   allDriverVersions)
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_7, deviceFamilyIntelGMA950,   V(8,15,10,1930))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_7, deviceFamilyIntelGMA3150,  V(8,14,10,2117))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_7, deviceFamilyIntelGMAX3000, V(8,15,10,1930))
  IMPLEMENT_INTEL_DRIVER_BLOCKLIST(DRIVER_OS_WINDOWS_7, deviceFamilyIntelGMAX4500HD, V(8,15,10,2202))

  
  GfxDriverInfo( DRIVER_OS_ALL,
    vendorIntel, GfxDriverInfo::allDevices,
    nsIGfxInfo::FEATURE_OPENGL_LAYERS, nsIGfxInfo::FEATURE_DISCOURAGED,
    DRIVER_LESS_THAN, allDriverVersions ),
  GfxDriverInfo( DRIVER_OS_ALL,
    vendorIntel, GfxDriverInfo::allDevices,
    nsIGfxInfo::FEATURE_WEBGL_OPENGL, nsIGfxInfo::FEATURE_DISCOURAGED,
    DRIVER_LESS_THAN, allDriverVersions ),

  GfxDriverInfo()
};

static OperatingSystem
WindowsVersionToOperatingSystem(PRInt32 aWindowsVersion)
{
  switch(aWindowsVersion) {
    case gfxWindowsPlatform::kWindows2000:
      return DRIVER_OS_WINDOWS_2000;
    case gfxWindowsPlatform::kWindowsXP:
      return DRIVER_OS_WINDOWS_XP;
    case gfxWindowsPlatform::kWindowsServer2003:
      return DRIVER_OS_WINDOWS_SERVER_2003;
    case gfxWindowsPlatform::kWindowsVista:
      return DRIVER_OS_WINDOWS_VISTA;
    case gfxWindowsPlatform::kWindows7:
      return DRIVER_OS_WINDOWS_7;
    case gfxWindowsPlatform::kWindowsUnknown:
    default:
      return DRIVER_OS_UNKNOWN;
    };
}

nsresult
GfxInfo::GetFeatureStatusImpl(PRInt32 aFeature, PRInt32 *aStatus, nsAString & aSuggestedDriverVersion, GfxDriverInfo* aDriverInfo )
{
  *aStatus = nsIGfxInfo::FEATURE_NO_INFO;
  aSuggestedDriverVersion.SetIsVoid(PR_TRUE);

  PRInt32 status = nsIGfxInfo::FEATURE_NO_INFO;

  PRUint32 adapterVendor = 0;
  PRUint32 adapterDeviceID = 0;
  nsAutoString adapterDriverVersionString;
  if (NS_FAILED(GetAdapterVendorID(&adapterVendor)) ||
      NS_FAILED(GetAdapterDeviceID(&adapterDeviceID)) ||
      NS_FAILED(GetAdapterDriverVersion(adapterDriverVersionString)))
  {
    return NS_ERROR_FAILURE;
  }

  if (adapterVendor != vendorIntel &&
      adapterVendor != vendorNVIDIA &&
      adapterVendor != vendorAMD &&
      adapterVendor != vendorATI &&
      
      
      
      adapterVendor != 0xabcd &&
      adapterVendor != 0xdcba &&
      adapterVendor != 0xabab &&
      adapterVendor != 0xdcdc)
  {
    *aStatus = FEATURE_BLOCKED_DEVICE;
    return NS_OK;
  }

  PRUint64 driverVersion;
  if (!ParseDriverVersion(adapterDriverVersionString, &driverVersion)) {
    return NS_ERROR_FAILURE;
  }
  
  if (aFeature == FEATURE_DIRECT3D_9_LAYERS &&
      mWindowsVersion < gfxWindowsPlatform::kWindowsXP)
  {
    *aStatus = FEATURE_BLOCKED_OS_VERSION;
    return NS_OK;
  }

  
  
  if (aFeature == FEATURE_WEBGL_ANGLE &&
      gfxWindowsPlatform::IsOptimus())
  {
    *aStatus = FEATURE_BLOCKED_DEVICE;
    return NS_OK;
  }

  OperatingSystem os = WindowsVersionToOperatingSystem(mWindowsVersion);

  
  
  if (os == DRIVER_OS_WINDOWS_SERVER_2003)
    os = DRIVER_OS_WINDOWS_XP;

  const GfxDriverInfo *info;
  if (aDriverInfo)
    info = aDriverInfo;
  else
    info = &gDriverInfo[0];

  if (mHasDriverVersionMismatch) {
    if (aFeature == nsIGfxInfo::FEATURE_DIRECT3D_10_LAYERS ||
        aFeature == nsIGfxInfo::FEATURE_DIRECT3D_10_1_LAYERS ||
        aFeature == nsIGfxInfo::FEATURE_DIRECT2D)
    {
      *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DRIVER_VERSION;
      return NS_OK;
    }
  }

  
  
  
  if (os == DRIVER_OS_WINDOWS_XP &&
      adapterVendor == vendorNVIDIA &&
      adapterDeviceID == 0x0861 && 
      driverVersion == V(6,14,11,7756))
  {
    return NS_OK;
  }

  while (info->mOperatingSystem) {

    if (info->mOperatingSystem != DRIVER_OS_ALL &&
        info->mOperatingSystem != os)
    {
      info++;
      continue;
    }

    if (info->mAdapterVendor != GfxDriverInfo::allAdapterVendors &&
        info->mAdapterVendor != adapterVendor) {
      info++;
      continue;
    }

    if (info->mDevices != GfxDriverInfo::allDevices) {
        bool deviceMatches = false;
        for (const PRUint32 *devices = info->mDevices; *devices; ++devices) {
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

    switch (info->mComparisonOp) {
    case DRIVER_LESS_THAN:
      match = driverVersion < info->mDriverVersion;
      break;
    case DRIVER_LESS_THAN_OR_EQUAL:
      match = driverVersion <= info->mDriverVersion;
      break;
    case DRIVER_GREATER_THAN:
      match = driverVersion > info->mDriverVersion;
      break;
    case DRIVER_GREATER_THAN_OR_EQUAL:
      match = driverVersion >= info->mDriverVersion;
      break;
    case DRIVER_EQUAL:
      match = driverVersion == info->mDriverVersion;
      break;
    case DRIVER_NOT_EQUAL:
      match = driverVersion != info->mDriverVersion;
      break;
    case DRIVER_BETWEEN_EXCLUSIVE:
      match = driverVersion > info->mDriverVersion && driverVersion < info->mDriverVersionMax;
      break;
    case DRIVER_BETWEEN_INCLUSIVE:
      match = driverVersion >= info->mDriverVersion && driverVersion <= info->mDriverVersionMax;
      break;
    case DRIVER_BETWEEN_INCLUSIVE_START:
      match = driverVersion >= info->mDriverVersion && driverVersion < info->mDriverVersionMax;
      break;
    default:
      NS_WARNING("Bogus op in GfxDriverInfo");
      break;
    }

    if (match) {
      if (info->mFeature == GfxDriverInfo::allFeatures ||
          info->mFeature == aFeature)
      {
        status = info->mFeatureStatus;
        break;
      }
    }

    info++;
  }

  *aStatus = status;

  if (status == FEATURE_BLOCKED_DRIVER_VERSION) {
      if (info->mSuggestedVersion) {
          aSuggestedDriverVersion.AppendPrintf("%s", info->mSuggestedVersion);
      } else if (info->mComparisonOp == DRIVER_LESS_THAN &&
                 info->mDriverVersion != allDriverVersions)
      {
          aSuggestedDriverVersion.AppendPrintf("%lld.%lld.%lld.%lld",
                                               (info->mDriverVersion & 0xffff000000000000) >> 48,
                                               (info->mDriverVersion & 0x0000ffff00000000) >> 32,
                                               (info->mDriverVersion & 0x00000000ffff0000) >> 16,
                                               (info->mDriverVersion & 0x000000000000ffff));
      }
  }
  
  return NS_OK;
}

#ifdef DEBUG




NS_IMETHODIMP GfxInfo::SpoofVendorID(PRUint32 aVendorID)
{
  mAdapterVendorID = aVendorID;
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofDeviceID(PRUint32 aDeviceID)
{
  mAdapterDeviceID = aDeviceID;
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofDriverVersion(const nsAString & aDriverVersion)
{
  mDriverVersion = aDriverVersion;
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofOSVersion(PRUint32 aVersion)
{
  mWindowsVersion = aVersion;
  return NS_OK;
}

#endif
