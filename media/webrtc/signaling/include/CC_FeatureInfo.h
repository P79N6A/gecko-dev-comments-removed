



#pragma once

#include "CC_Common.h"

extern "C"
{
#include "ccapi_types.h"
}

namespace CSF
{
    class ECC_API CC_FeatureInfo
    {
    protected:
        CC_FeatureInfo() { }

    public:
        
        virtual ~CC_FeatureInfo() {};

        




        virtual cc_int32_t getButton() = 0;

        




        virtual cc_int32_t getFeatureID() = 0;

        




        virtual std::string getDisplayName() = 0;

        




        virtual std::string getSpeedDialNumber() = 0;

        




        virtual std::string getContact() = 0;

        




        virtual std::string getRetrievalPrefix() = 0;

        




        virtual cc_int32_t getFeatureOptionMask() = 0;
    };
};
