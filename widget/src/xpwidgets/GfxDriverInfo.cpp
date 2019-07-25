




































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
