




#include "prtypes.h"
#include "nsString.h"

#ifndef __mozilla_widget_GfxDriverInfo_h__
#define __mozilla_widget_GfxDriverInfo_h__

#define V(a,b,c,d) GFX_DRIVER_VERSION(a,b,c,d)


#define APPEND_TO_DRIVER_BLOCKLIST(os, vendor, devices, feature, featureStatus, driverComparator, driverVersion, suggestedVersion) \
    mDriverInfo->AppendElement(GfxDriverInfo(os, vendor, devices, feature, featureStatus, driverComparator, driverVersion, suggestedVersion))
#define APPEND_TO_DRIVER_BLOCKLIST2(os, vendor, devices, feature, featureStatus, driverComparator, driverVersion) \
    mDriverInfo->AppendElement(GfxDriverInfo(os, vendor, devices, feature, featureStatus, driverComparator, driverVersion))

namespace mozilla {
namespace widget {

enum OperatingSystem {
  DRIVER_OS_UNKNOWN = 0,
  DRIVER_OS_WINDOWS_XP,
  DRIVER_OS_WINDOWS_SERVER_2003,
  DRIVER_OS_WINDOWS_VISTA,
  DRIVER_OS_WINDOWS_7,
  DRIVER_OS_WINDOWS_8,
  DRIVER_OS_LINUX,
  DRIVER_OS_OS_X_10_5,
  DRIVER_OS_OS_X_10_6,
  DRIVER_OS_OS_X_10_7,
  DRIVER_OS_OS_X_10_8,
  DRIVER_OS_ANDROID,
  DRIVER_OS_ALL
};

enum VersionComparisonOp {
  DRIVER_LESS_THAN,             
  DRIVER_LESS_THAN_OR_EQUAL,    
  DRIVER_GREATER_THAN,          
  DRIVER_GREATER_THAN_OR_EQUAL, 
  DRIVER_EQUAL,                 
  DRIVER_NOT_EQUAL,             
  DRIVER_BETWEEN_EXCLUSIVE,     
  DRIVER_BETWEEN_INCLUSIVE,     
  DRIVER_BETWEEN_INCLUSIVE_START, 
  DRIVER_COMPARISON_IGNORED
};

enum DeviceFamily {
  IntelGMA500,
  IntelGMA900,
  IntelGMA950,
  IntelGMA3150,
  IntelGMAX3000,
  IntelGMAX4500HD,
  NvidiaBlockD3D9Layers,
  RadeonX1000,
  Geforce7300GT,
  DeviceFamilyMax
};

enum DeviceVendor {
  VendorAll,
  VendorIntel,
  VendorNVIDIA,
  VendorAMD,
  VendorATI,
  DeviceVendorMax
};


typedef nsTArray<nsString> GfxDeviceFamily;

struct GfxDriverInfo
{
  
  
  GfxDriverInfo(OperatingSystem os, nsAString& vendor, GfxDeviceFamily* devices,
                int32_t feature, int32_t featureStatus, VersionComparisonOp op,
                uint64_t driverVersion, const char *suggestedVersion = nullptr,
                bool ownDevices = false);

  GfxDriverInfo();
  GfxDriverInfo(const GfxDriverInfo&);
  ~GfxDriverInfo();

  OperatingSystem mOperatingSystem;

  nsString mAdapterVendor;

  static GfxDeviceFamily* const allDevices;
  GfxDeviceFamily* mDevices;

  
  
  bool mDeleteDevices;

  
  int32_t mFeature;
  static int32_t allFeatures;

  
  int32_t mFeatureStatus;

  VersionComparisonOp mComparisonOp;

  
  uint64_t mDriverVersion;
  uint64_t mDriverVersionMax;
  static uint64_t allDriverVersions;

  const char *mSuggestedVersion;

  static const GfxDeviceFamily* GetDeviceFamily(DeviceFamily id);
  static GfxDeviceFamily* mDeviceFamilies[DeviceFamilyMax];

  static const nsAString& GetDeviceVendor(DeviceVendor id);
  static nsAString* mDeviceVendors[DeviceVendorMax];
};

#define GFX_DRIVER_VERSION(a,b,c,d) \
  ((uint64_t(a)<<48) | (uint64_t(b)<<32) | (uint64_t(c)<<16) | uint64_t(d))

inline bool
ParseDriverVersion(nsAString& aVersion, uint64_t *aNumericVersion)
{
#if defined(XP_WIN)
  int a, b, c, d;
  
  if (sscanf(NS_LossyConvertUTF16toASCII(aVersion).get(),
             "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
    return false;
  if (a < 0 || a > 0xffff) return false;
  if (b < 0 || b > 0xffff) return false;
  if (c < 0 || c > 0xffff) return false;
  if (d < 0 || d > 0xffff) return false;

  *aNumericVersion = GFX_DRIVER_VERSION(a, b, c, d);
#elif defined(ANDROID)
  
  
  *aNumericVersion = atoi(NS_LossyConvertUTF16toASCII(aVersion).get());
#endif
  return true;
}

}
}

#endif 
