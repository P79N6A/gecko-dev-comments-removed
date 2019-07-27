




#include "GfxDriverInfo.h"

#include "nsIGfxInfo.h"
#include "nsTArray.h"

using namespace mozilla::widget;

int32_t GfxDriverInfo::allFeatures = 0;
uint64_t GfxDriverInfo::allDriverVersions = ~(uint64_t(0));
GfxDeviceFamily* const GfxDriverInfo::allDevices = nullptr;

GfxDeviceFamily* GfxDriverInfo::mDeviceFamilies[DeviceFamilyMax];
nsAString* GfxDriverInfo::mDeviceVendors[DeviceVendorMax];

GfxDriverInfo::GfxDriverInfo()
  : mOperatingSystem(DRIVER_OS_UNKNOWN),
    mOperatingSystemVersion(0),
    mAdapterVendor(GfxDriverInfo::GetDeviceVendor(VendorAll)),
    mDevices(allDevices),
    mDeleteDevices(false),
    mFeature(allFeatures),
    mFeatureStatus(nsIGfxInfo::FEATURE_STATUS_OK),
    mComparisonOp(DRIVER_COMPARISON_IGNORED),
    mDriverVersion(0),
    mDriverVersionMax(0),
    mSuggestedVersion(nullptr),
    mGpu2(false)
{}

GfxDriverInfo::GfxDriverInfo(OperatingSystem os, nsAString& vendor,
                             GfxDeviceFamily* devices,
                             int32_t feature, int32_t featureStatus,
                             VersionComparisonOp op,
                             uint64_t driverVersion,
                             const char *suggestedVersion ,
                             bool ownDevices ,
                             bool gpu2 )
  : mOperatingSystem(os),
    mOperatingSystemVersion(0),
    mAdapterVendor(vendor),
    mDevices(devices),
    mDeleteDevices(ownDevices),
    mFeature(feature),
    mFeatureStatus(featureStatus),
    mComparisonOp(op),
    mDriverVersion(driverVersion),
    mDriverVersionMax(0),
    mSuggestedVersion(suggestedVersion),
    mGpu2(gpu2)
{}

GfxDriverInfo::GfxDriverInfo(const GfxDriverInfo& aOrig)
  : mOperatingSystem(aOrig.mOperatingSystem),
    mOperatingSystemVersion(aOrig.mOperatingSystemVersion),
    mAdapterVendor(aOrig.mAdapterVendor),
    mFeature(aOrig.mFeature),
    mFeatureStatus(aOrig.mFeatureStatus),
    mComparisonOp(aOrig.mComparisonOp),
    mDriverVersion(aOrig.mDriverVersion),
    mDriverVersionMax(aOrig.mDriverVersionMax),
    mSuggestedVersion(aOrig.mSuggestedVersion),
    mGpu2(aOrig.mGpu2)
{
  
  
  if (aOrig.mDeleteDevices && aOrig.mDevices) {
    mDevices = new GfxDeviceFamily;
    *mDevices = *aOrig.mDevices;
  } else {
    mDevices = aOrig.mDevices;
  }

  mDeleteDevices = aOrig.mDeleteDevices;
}

GfxDriverInfo::~GfxDriverInfo()
{
  if (mDeleteDevices)
    delete mDevices;
}


#define APPEND_DEVICE(device) APPEND_DEVICE2(#device)
#define APPEND_DEVICE2(device) deviceFamily->AppendElement(NS_LITERAL_STRING(device))

const GfxDeviceFamily* GfxDriverInfo::GetDeviceFamily(DeviceFamily id)
{
  
  
  NS_ASSERTION(id >= 0 && id < DeviceFamilyMax, "DeviceFamily id is out of range");

  
  if (mDeviceFamilies[id])
    return mDeviceFamilies[id];

  mDeviceFamilies[id] = new GfxDeviceFamily;
  GfxDeviceFamily* deviceFamily = mDeviceFamilies[id];

  switch (id) {
    case IntelGMA500:
      APPEND_DEVICE(0x8108); 
      APPEND_DEVICE(0x8109); 
      break;
    case IntelGMA900:
      APPEND_DEVICE(0x2582); 
      APPEND_DEVICE(0x2782); 
      APPEND_DEVICE(0x2592); 
      APPEND_DEVICE(0x2792); 
      break;
    case IntelGMA950:
      APPEND_DEVICE(0x2772); 
      APPEND_DEVICE(0x2776); 
      APPEND_DEVICE(0x27a2); 
      APPEND_DEVICE(0x27a6); 
      APPEND_DEVICE(0x27ae); 
      break;
    case IntelGMA3150:
      APPEND_DEVICE(0xa001); 
      APPEND_DEVICE(0xa002); 
      APPEND_DEVICE(0xa011); 
      APPEND_DEVICE(0xa012); 
      break;
    case IntelGMAX3000:
      APPEND_DEVICE(0x2972); 
      APPEND_DEVICE(0x2973); 
      APPEND_DEVICE(0x2982); 
      APPEND_DEVICE(0x2983); 
      APPEND_DEVICE(0x2992); 
      APPEND_DEVICE(0x2993); 
      APPEND_DEVICE(0x29a2); 
      APPEND_DEVICE(0x29a3); 
      APPEND_DEVICE(0x29b2); 
      APPEND_DEVICE(0x29b3); 
      APPEND_DEVICE(0x29c2); 
      APPEND_DEVICE(0x29c3); 
      APPEND_DEVICE(0x29d2); 
      APPEND_DEVICE(0x29d3); 
      APPEND_DEVICE(0x2a02); 
      APPEND_DEVICE(0x2a03); 
      APPEND_DEVICE(0x2a12); 
      APPEND_DEVICE(0x2a13); 
      break;
    case IntelGMAX4500HD:
      APPEND_DEVICE(0x2a42); 
      APPEND_DEVICE(0x2a43); 
      APPEND_DEVICE(0x2e42); 
      APPEND_DEVICE(0x2e43); 
      APPEND_DEVICE(0x2e92); 
      APPEND_DEVICE(0x2e93); 
      APPEND_DEVICE(0x2e32); 
      APPEND_DEVICE(0x2e33); 
      APPEND_DEVICE(0x2e22); 
      APPEND_DEVICE(0x2e23); 
      APPEND_DEVICE(0x2e12); 
      APPEND_DEVICE(0x2e13); 
      APPEND_DEVICE(0x0042); 
      APPEND_DEVICE(0x0046); 
      APPEND_DEVICE(0x0102); 
      APPEND_DEVICE(0x0106); 
      APPEND_DEVICE(0x0112); 
      APPEND_DEVICE(0x0116); 
      APPEND_DEVICE(0x0122); 
      APPEND_DEVICE(0x0126); 
      APPEND_DEVICE(0x010a); 
      APPEND_DEVICE(0x0080); 
      break;
    case IntelHD3000:
      APPEND_DEVICE(0x0126);
      break;
    case IntelMobileHDGraphics:
      APPEND_DEVICE(0x0046); 
      break;
    case NvidiaBlockD3D9Layers:
      
      APPEND_DEVICE(0x00f3); 
      APPEND_DEVICE(0x0146); 
      APPEND_DEVICE(0x014f); 
      APPEND_DEVICE(0x0161); 
      APPEND_DEVICE(0x0162); 
      APPEND_DEVICE(0x0163); 
      APPEND_DEVICE(0x0164); 
      APPEND_DEVICE(0x0167); 
      APPEND_DEVICE(0x0168); 
      APPEND_DEVICE(0x0169); 
      APPEND_DEVICE(0x0222); 
      APPEND_DEVICE(0x0240); 
      APPEND_DEVICE(0x0241); 
      APPEND_DEVICE(0x0244); 
      APPEND_DEVICE(0x0245); 
      APPEND_DEVICE(0x0247); 
      APPEND_DEVICE(0x03d0); 
      APPEND_DEVICE(0x03d1); 
      APPEND_DEVICE(0x03d2); 
      APPEND_DEVICE(0x03d5); 
      break;
    case RadeonX1000:
      
      APPEND_DEVICE(0x7187);
      APPEND_DEVICE(0x7210);
      APPEND_DEVICE(0x71de);
      APPEND_DEVICE(0x7146);
      APPEND_DEVICE(0x7142);
      APPEND_DEVICE(0x7109);
      APPEND_DEVICE(0x71c5);
      APPEND_DEVICE(0x71c0);
      APPEND_DEVICE(0x7240);
      APPEND_DEVICE(0x7249);
      APPEND_DEVICE(0x7291);
      break;
    case Geforce7300GT:
      APPEND_DEVICE(0x0393);
      break;
    case Nvidia310M:
      APPEND_DEVICE(0x0A70);
      break;
    case Nvidia8800GTS:
      APPEND_DEVICE(0x0193);
      break;
    case Bug1137716:
      APPEND_DEVICE(0x0a29);
      APPEND_DEVICE(0x0a2b);
      APPEND_DEVICE(0x0a2d);
      APPEND_DEVICE(0x0a35);
      APPEND_DEVICE(0x0a6c);
      APPEND_DEVICE(0x0a70);
      APPEND_DEVICE(0x0a72);
      APPEND_DEVICE(0x0a7a);
      APPEND_DEVICE(0x0caf);
      APPEND_DEVICE(0x0dd2);
      APPEND_DEVICE(0x0dd3);
      
      APPEND_DEVICE(0x0de3);
      APPEND_DEVICE(0x0de8);
      APPEND_DEVICE(0x0de9);
      APPEND_DEVICE(0x0dea);
      APPEND_DEVICE(0x0deb);
      APPEND_DEVICE(0x0dec);
      APPEND_DEVICE(0x0ded);
      APPEND_DEVICE(0x0dee);
      APPEND_DEVICE(0x0def);
      APPEND_DEVICE(0x0df0);
      APPEND_DEVICE(0x0df1);
      APPEND_DEVICE(0x0df2);
      APPEND_DEVICE(0x0df3);
      APPEND_DEVICE(0x0df4);
      APPEND_DEVICE(0x0df5);
      APPEND_DEVICE(0x0df6);
      APPEND_DEVICE(0x0df7);
      APPEND_DEVICE(0x1050);
      APPEND_DEVICE(0x1051);
      APPEND_DEVICE(0x1052);
      APPEND_DEVICE(0x1054);
      APPEND_DEVICE(0x1055);
      break;
    case Bug1116812:
      APPEND_DEVICE(0x2e32);
      APPEND_DEVICE(0x2a02);
      break;
    case AMDRadeonHD5800:
      APPEND_DEVICE(0x6899);
      break;
    
    case DeviceFamilyMax:
      NS_WARNING("Invalid DeviceFamily id");
      break;
  }

  return deviceFamily;
}


#define DECLARE_VENDOR_ID(name, deviceId) \
  case name: \
    mDeviceVendors[id]->AssignLiteral(deviceId); \
    break;

const nsAString& GfxDriverInfo::GetDeviceVendor(DeviceVendor id)
{
  NS_ASSERTION(id >= 0 && id < DeviceVendorMax, "DeviceVendor id is out of range");

  if (mDeviceVendors[id])
    return *mDeviceVendors[id];

  mDeviceVendors[id] = new nsString();

  switch (id) {
    DECLARE_VENDOR_ID(VendorAll, "");
    DECLARE_VENDOR_ID(VendorIntel, "0x8086");
    DECLARE_VENDOR_ID(VendorNVIDIA, "0x10de");
    DECLARE_VENDOR_ID(VendorAMD, "0x1022");
    DECLARE_VENDOR_ID(VendorATI, "0x1002");
    DECLARE_VENDOR_ID(VendorMicrosoft, "0x1414");
    
    DECLARE_VENDOR_ID(DeviceVendorMax, "");
  }

  return *mDeviceVendors[id];
}
