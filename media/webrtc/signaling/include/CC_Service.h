



#pragma once

#include "CC_Common.h"
#include "CC_Observer.h"

#include <vector>

extern "C"
{
#include "ccapi_types.h"
#include "ccapi_service.h"
}

namespace CSF
{
    class ECC_API CC_Service
    {
    public:
        NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CC_Service)
    protected:
    	CC_Service() {}
    public:
        virtual ~CC_Service() {};

    public:
        


        virtual void addCCObserver ( CC_Observer * observer ) = 0;
        virtual void removeCCObserver ( CC_Observer * observer ) = 0;

        




        virtual bool init(const std::string& user, const std::string& password, const std::string& domain, const std::string& deviceName) = 0;
        virtual void destroy() = 0;

        




        



        virtual bool startService() = 0;
        virtual void stop() = 0;


        


        virtual bool isStarted() = 0;

        




        virtual CC_DevicePtr getActiveDevice() = 0;
        virtual std::vector<CC_DevicePtr> getDevices() = 0;

        





        virtual AudioControlPtr getAudioControl() = 0;
        virtual VideoControlPtr getVideoControl() = 0;

        virtual bool setLocalVoipPort(int port) = 0;
        virtual bool setRemoteVoipPort(int port) = 0;
        virtual bool setP2PMode(bool mode) = 0;
        virtual bool setSDPMode(bool mode) = 0;

    private:
        CC_Service(const CC_Service& rhs);
        CC_Service& operator=(const CC_Service& rhs);
    };
}
