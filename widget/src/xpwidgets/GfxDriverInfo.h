




































#include "prtypes.h"
#include "nsString.h"

#ifndef __mozilla_widget_GfxDriverInfo_h__
#define __mozilla_widget_GfxDriverInfo_h__

#define V(a,b,c,d) GFX_DRIVER_VERSION(a,b,c,d)

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
    0x0222, 
    0x0240, 
    0x0241, 
    0x0244, 
    0x0245, 
    0x0247, 
    0x03d0, 
    0x03d1, 
    0x03d2, 
    0x03d5, 
    0
};


typedef PRUint32 *GfxDeviceFamily;

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
