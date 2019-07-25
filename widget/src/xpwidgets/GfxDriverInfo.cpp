




































#include "GfxDriverInfo.h"
#include "nsIGfxInfo.h"

using namespace mozilla::widget;

PRUint32 GfxDriverInfo::allAdapterVendors = 0;
PRInt32 GfxDriverInfo::allFeatures = 0;
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
    mDriverVersionMax(0)
{}

GfxDriverInfo::GfxDriverInfo(OperatingSystem os, PRUint32 vendor,
                             GfxDeviceFamily devices,
                             PRInt32 feature, PRInt32 featureStatus,
                             VersionComparisonOp op,
                             PRUint64 driverVersion,
                             bool ownDevices )
  : mOperatingSystem(os),
    mAdapterVendor(vendor),
    mDevices(devices),
    mDeleteDevices(ownDevices),
    mFeature(feature),
    mFeatureStatus(featureStatus),
    mComparisonOp(op),
    mDriverVersion(driverVersion),
    mDriverVersionMax(0)
{}

GfxDriverInfo::GfxDriverInfo(const GfxDriverInfo& aOrig)
  : mOperatingSystem(aOrig.mOperatingSystem),
    mAdapterVendor(aOrig.mAdapterVendor),
    mFeature(aOrig.mFeature),
    mFeatureStatus(aOrig.mFeatureStatus),
    mComparisonOp(aOrig.mComparisonOp),
    mDriverVersion(aOrig.mDriverVersion),
    mDriverVersionMax(aOrig.mDriverVersionMax)
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
