



#ifndef _CC_SIPCC_DEVICE_H
#define _CC_SIPCC_DEVICE_H

#include "CC_Device.h"
#include "common/Wrapper.h"
#include <map>

namespace CSF
{
	DECLARE_NS_PTR(CC_SIPCCDevice);
    class CC_SIPCCDevice : public CC_Device
    {
    private:
    	CSF_DECLARE_WRAP(CC_SIPCCDevice, cc_device_handle_t);
        static CC_DevicePtr create (const std::string & phoneConfigFileNameStr);

        explicit CC_SIPCCDevice (cc_device_handle_t aDeviceHandle);

    public:

        static CC_DevicePtr createDevice ();

        virtual CC_DeviceInfoPtr getDeviceInfo ();
        virtual std::string toString();

        virtual CC_CallPtr createCall();

        virtual void enableVideo(bool enable);
        virtual void enableCamera(bool enable);
		virtual void setDigestNamePasswd (char *name, char *pw);

    private:
        cc_device_handle_t deviceHandle;

    };
};

#endif
