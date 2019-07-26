



#pragma once

#include "CC_Common.h"

extern "C"
{
#include "ccapi_types.h"
}

namespace CSF
{

    class ECC_API CC_Device
    {
    public:
        NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CC_Device)
    protected:
        CC_Device() {}

    public:
        virtual ~CC_Device() {}

        virtual std::string toString() = 0;

        virtual CC_DeviceInfoPtr getDeviceInfo () = 0;

        







        virtual CC_CallPtr createCall() = 0;

        virtual void enableVideo(bool enable) = 0;
        virtual void enableCamera(bool enable) = 0;
		virtual void setDigestNamePasswd (char *name, char *pw) = 0;

    private:
		
		CC_Device& operator=(const CC_Device& rhs);
		CC_Device(const CC_Device&);
    };
}
