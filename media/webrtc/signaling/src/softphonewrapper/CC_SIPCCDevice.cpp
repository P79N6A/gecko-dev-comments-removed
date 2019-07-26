






































#include "CC_Common.h"

#include "CC_SIPCCDevice.h"
#include "CC_SIPCCDeviceInfo.h"
#include "CC_SIPCCFeatureInfo.h"
#include "CC_SIPCCCall.h"

extern "C"
{
#include "cpr_types.h"
#include "config_api.h"
#include "ccapi_device.h"
#include "ccapi_device_info.h"
#include "ccapi_device_listener.h"
}

using namespace std;
using namespace CSF;

#include "CSFLogStream.h"

CSF_IMPLEMENT_WRAP(CC_SIPCCDevice, cc_device_handle_t);

CC_DevicePtr CC_SIPCCDevice::createDevice ()
{
    cc_device_handle_t deviceHandle = CCAPI_Device_getDeviceID();

    CC_SIPCCDevicePtr pSIPCCDevice = CC_SIPCCDevice::wrap(deviceHandle);

    return pSIPCCDevice;
}

CC_SIPCCDevice::CC_SIPCCDevice (cc_device_handle_t aDeviceHandle)
: deviceHandle(aDeviceHandle)
{
	enableVideo(true);
	enableCamera(true);
}

CC_DeviceInfoPtr CC_SIPCCDevice::getDeviceInfo ()
{
    cc_deviceinfo_ref_t deviceInfoRef = CCAPI_Device_getDeviceInfo(deviceHandle);
    CC_DeviceInfoPtr deviceInfoPtr = CC_SIPCCDeviceInfo::wrap(deviceInfoRef);

    
    
    

    CCAPI_Device_releaseDeviceInfo(deviceInfoRef);

    
    
    
    

    return deviceInfoPtr;
}

std::string CC_SIPCCDevice::toString()
{
    std::string result;
    char tmpString[11];
    csf_sprintf(tmpString, sizeof(tmpString), "%X", deviceHandle);
    result = tmpString;
    return result;
}

CC_CallPtr CC_SIPCCDevice::createCall ()
{
    cc_call_handle_t callHandle = CCAPI_Device_CreateCall(deviceHandle);

    return CC_SIPCCCall::wrap(callHandle);
}

void CC_SIPCCDevice::enableVideo(bool enable)
{
    CCAPI_Device_enableVideo(deviceHandle, enable);
}

void CC_SIPCCDevice::enableCamera(bool enable)    
{
    CCAPI_Device_enableCamera(deviceHandle, enable);
}

void CC_SIPCCDevice::setDigestNamePasswd (char *name, char *pw)
{
    CCAPI_Device_setDigestNamePasswd(deviceHandle, name, pw);
}
