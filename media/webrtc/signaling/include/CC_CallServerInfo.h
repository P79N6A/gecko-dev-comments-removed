



#pragma once

#include "CC_Common.h"

#include <vector>

extern "C"
{
#include "ccapi_types.h"
}

namespace CSF
{
    class ECC_API CC_CallServerInfo
    {
    public:
        NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CC_CallServerInfo)
    protected:
        CC_CallServerInfo() { }

    public:
        
        virtual ~CC_CallServerInfo() {};

        




        virtual std::string getCallServerName() = 0;

        




        virtual cc_cucm_mode_t getCallServerMode() = 0;

        




        virtual cc_ccm_status_t getCallServerStatus() = 0;
    };
};
