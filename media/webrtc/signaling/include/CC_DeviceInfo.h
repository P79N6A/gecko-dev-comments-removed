



#pragma once

#include "CC_Common.h"

#include <vector>

extern "C"
{
#include "ccapi_types.h"
}

namespace CSF
{
    class ECC_API CC_DeviceInfo
    {
    protected:
        CC_DeviceInfo() { }

    public:
        
        virtual ~CC_DeviceInfo() {};

        



        virtual std::string getDeviceName() = 0;

        




        virtual cc_service_state_t getServiceState() = 0;

        




        virtual cc_service_cause_t getServiceCause() = 0;

        




        virtual std::vector<CC_CallPtr> getCalls () = 0;

        










        




        virtual std::vector<CC_LinePtr> getLines () = 0;

        




        virtual std::vector<CC_FeatureInfoPtr> getFeatures () = 0;

        




        virtual std::vector<CC_CallServerInfoPtr> getCallServers () = 0;

        







        






        






        






        






        






    private:
		
		CC_DeviceInfo& operator=(const CC_DeviceInfo& rhs);
		CC_DeviceInfo(const CC_DeviceInfo&);
    };
};
