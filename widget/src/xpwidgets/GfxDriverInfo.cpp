




































#include "GfxDriverInfo.h"
#include "nsIGfxInfo.h"

using namespace mozilla::widget;

PRUint32 GfxDriverInfo::allAdapterVendors = 0;
PRInt32 GfxDriverInfo::allFeatures = 0;
PRUint64 GfxDriverInfo::allDriverVersions = ~(PRUint64(0));

PRUint32 GfxDriverInfo::vendorIntel = 0x8086;
PRUint32 GfxDriverInfo::vendorNVIDIA = 0x10de;
PRUint32 GfxDriverInfo::vendorAMD = 0x1022;
PRUint32 GfxDriverInfo::vendorATI = 0x1002;

GfxDeviceFamily GfxDriverInfo::allDevices = nsnull;

GfxDriverInfo::GfxDriverInfo()
  : mOperatingSystem(DRIVER_OS_UNKNOWN),
    mAdapterVendor(allAdapterVendors),
    mDevices(allDevices),
    mDeleteDevices(false),
    mFeature(allFeatures),
    mFeatureStatus(nsIGfxInfo::FEATURE_NO_INFO),
    mComparisonOp(DRIVER_UNKNOWN_COMPARISON),
    mDriverVersion(0),
    mDriverVersionMax(0),
    mSuggestedVersion(nsnull)
{}

GfxDriverInfo::GfxDriverInfo(OperatingSystem os, PRUint32 vendor,
                             GfxDeviceFamily devices,
                             PRInt32 feature, PRInt32 featureStatus,
                             VersionComparisonOp op,
                             PRUint64 driverVersion,
                             const char *suggestedVersion ,
                             bool ownDevices )
  : mOperatingSystem(os),
    mAdapterVendor(vendor),
    mDevices(devices),
    mDeleteDevices(ownDevices),
    mFeature(feature),
    mFeatureStatus(featureStatus),
    mComparisonOp(op),
    mDriverVersion(driverVersion),
    mDriverVersionMax(0),
    mSuggestedVersion(suggestedVersion)
{}

GfxDriverInfo::GfxDriverInfo(const GfxDriverInfo& aOrig)
  : mOperatingSystem(aOrig.mOperatingSystem),
    mAdapterVendor(aOrig.mAdapterVendor),
    mFeature(aOrig.mFeature),
    mFeatureStatus(aOrig.mFeatureStatus),
    mComparisonOp(aOrig.mComparisonOp),
    mDriverVersion(aOrig.mDriverVersion),
    mDriverVersionMax(aOrig.mDriverVersionMax),
    mSuggestedVersion(aOrig.mSuggestedVersion)
{
  
  
  if (aOrig.mDeleteDevices) {
    PRUint32 count = 0;
    const PRUint32 *device = aOrig.mDevices;
    while (*device) {
      count++;
      device++;
    }

    mDevices = new PRUint32[count + 1];
    memcpy(mDevices, aOrig.mDevices, sizeof(PRUint32) * (count + 1));
  } else {
    mDevices = aOrig.mDevices;
  }

  mDeleteDevices = aOrig.mDeleteDevices;
}

GfxDriverInfo::~GfxDriverInfo()
{
  if (mDeleteDevices)
    delete[] mDevices;
}

const GfxDeviceFamily GfxDriverInfo::GetDeviceFamily(DeviceFamily id)
{
  switch (id) {
    case IntelGMA500: {
      static const PRUint32 intelGMA500[] = {
        0x8108, 
        0x8109, 
        0
      };
      return (const GfxDeviceFamily) &intelGMA500[0];
    }
    case IntelGMA900: {
      static const PRUint32 intelGMA900[] = {
        0x2582, 
        0x2782, 
        0x2592, 
        0x2792, 
        0
      };
      return (const GfxDeviceFamily) &intelGMA900[0];
    }
    case IntelGMA950: {
      static const PRUint32 intelGMA950[] = {
        0x2772, 
        0x2776, 
        0x27A2, 
        0x27A6, 
        0x27AE, 
        0
      };
      return (const GfxDeviceFamily) &intelGMA950[0];
    }
    case IntelGMA3150: {
      static const PRUint32 intelGMA3150[] = {
        0xA001, 
        0xA002, 
        0xA011, 
        0xA012, 
        0
      };
      return (const GfxDeviceFamily) &intelGMA3150[0];
    }
    case IntelGMAX3000: {
      static const PRUint32 intelGMAX3000[] = {
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
      return (const GfxDeviceFamily) &intelGMAX3000[0];
    }
    case IntelGMAX4500HD: {
      static const PRUint32 intelGMAX4500HD[] = {
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
      return (const GfxDeviceFamily) &intelGMAX4500HD[0];
    }
    case NvidiaBlockD3D9Layers: {
      
      static const PRUint32 nvidiaBlockD3D9Layers[] = {
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
      return (const GfxDeviceFamily) &nvidiaBlockD3D9Layers[0];
    }
    default:
      NS_WARNING("Invalid device family");
  }

  return nsnull;
}
