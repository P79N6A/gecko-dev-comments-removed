




































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
  DRIVER_OS_WINDOWS_2000,
  DRIVER_OS_WINDOWS_XP,
  DRIVER_OS_WINDOWS_SERVER_2003,
  DRIVER_OS_WINDOWS_VISTA,
  DRIVER_OS_WINDOWS_7,
  DRIVER_OS_LINUX,
  DRIVER_OS_OS_X_10_5,
  DRIVER_OS_OS_X_10_6,
  DRIVER_OS_OS_X_10_7,
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
  DRIVER_UNKNOWN_COMPARISON
};

enum DeviceFamily {
  IntelGMA500,
  IntelGMA900,
  IntelGMA950,
  IntelGMA3150,
  IntelGMAX3000,
  IntelGMAX4500HD,
  NvidiaBlockD3D9Layers
};


typedef PRUint32* GfxDeviceFamily;

struct GfxDriverInfo
{
  
  
  GfxDriverInfo(OperatingSystem os, PRUint32 vendor, GfxDeviceFamily devices,
                PRInt32 feature, PRInt32 featureStatus, VersionComparisonOp op,
                PRUint64 driverVersion, const char *suggestedVersion = nsnull,
                bool ownDevices = false);

  GfxDriverInfo();
  GfxDriverInfo(const GfxDriverInfo&);
  ~GfxDriverInfo();

  OperatingSystem mOperatingSystem;

  PRUint32 mAdapterVendor;
  static PRUint32 allAdapterVendors;

  GfxDeviceFamily mDevices;
  static GfxDeviceFamily allDevices;

  
  
  bool mDeleteDevices;

  
  PRInt32 mFeature;
  static PRInt32 allFeatures;

  
  PRInt32 mFeatureStatus;

  VersionComparisonOp mComparisonOp;

  
  PRUint64 mDriverVersion;
  PRUint64 mDriverVersionMax;
  static PRUint64 allDriverVersions;

  static PRUint32 vendorIntel;
  static PRUint32 vendorNVIDIA;
  static PRUint32 vendorAMD;
  static PRUint32 vendorATI;

  const char *mSuggestedVersion;

  static const GfxDeviceFamily GetDeviceFamily(DeviceFamily id);
};

#define GFX_DRIVER_VERSION(a,b,c,d) \
  ((PRUint64(a)<<48) | (PRUint64(b)<<32) | (PRUint64(c)<<16) | PRUint64(d))

inline bool
ParseDriverVersion(nsAString& aVersion, PRUint64 *aNumericVersion)
{
  int a, b, c, d;
  
  if (sscanf(NS_LossyConvertUTF16toASCII(aVersion).get(),
             "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
    return false;
  if (a < 0 || a > 0xffff) return false;
  if (b < 0 || b > 0xffff) return false;
  if (c < 0 || c > 0xffff) return false;
  if (d < 0 || d > 0xffff) return false;

  *aNumericVersion = GFX_DRIVER_VERSION(a, b, c, d);
  return true;
}

}
}

#endif 
