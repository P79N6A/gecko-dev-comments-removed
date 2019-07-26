






































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
static const char* logTag = "CC_SIPCCDevice";

CSF_IMPLEMENT_WRAP(CC_SIPCCDevice, cc_device_handle_t);


CC_DevicePtr CC_SIPCCDevice::createAndValidateXML (bool isXMLString, const string & configFileNameOrXMLString)
{
    cc_device_handle_t deviceHandle = CCAPI_Device_getDeviceID();

    CC_SIPCCDevicePtr pSIPCCDevice = CC_SIPCCDevice::wrap(deviceHandle);

    if (!pSIPCCDevice->checkXMLPhoneConfigValidity(isXMLString, configFileNameOrXMLString))
    {
        if (!isXMLString)
        {
            CSFLogError(logTag, "Phone Config file \"%s\" is not valid.", configFileNameOrXMLString.c_str());
        }
        else
        {
            CSFLogErrorS(logTag, "Phone Config XML is not valid.");
        }

        CSFLogInfoS(logTag, "pSIPCC requires that the phone config contain (at a minimum) a \"port config\", a \"proxy config\" and a \"line config\".");

        wrapper.release(deviceHandle);

        return NULL_PTR(CC_Device);
    }

    return pSIPCCDevice;
}

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

bool CC_SIPCCDevice::checkXMLPhoneConfigValidity (bool isXMLString, const string & configFileNameOrXMLString)
{
    return (CCAPI_Config_checkValidity(deviceHandle, configFileNameOrXMLString.c_str(), (isXMLString) ? TRUE : 0 ) != 0);
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
    std::stringstream sstream;
    sstream << "0x" << std::setw( 5 ) << std::setfill( '0' ) << std::hex << deviceHandle;
    return sstream.str();
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
