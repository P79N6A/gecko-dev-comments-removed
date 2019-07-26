



#pragma once

#include "CC_Common.h"
#include <bitset>
#include <set>
#include <vector>

extern "C"
{
#include "ccapi_types.h"
}

namespace CSF
{
    class ECC_API CC_LineInfo
    {
    protected:
        CC_LineInfo() { }

    public:
        
        virtual ~CC_LineInfo() {};

        




        virtual std::string getName() = 0;

        



        virtual std::string getNumber() = 0;

        




        virtual cc_uint32_t getButton() = 0;

        




        virtual cc_line_feature_t getLineType() = 0;

        



        virtual bool getRegState() = 0;

        




        virtual bool isCFWDActive() = 0;

        




        virtual std::string getCFWDName() = 0;

        





        virtual std::vector<CC_CallPtr> getCalls (CC_LinePtr linePtr) = 0;

        







        virtual std::vector<CC_CallPtr> getCallsByState (CC_LinePtr linePtr, cc_call_state_t state) = 0;

        




        virtual bool getMWIStatus() = 0;

        




        virtual cc_uint32_t getMWIType() = 0;

        




        virtual cc_uint32_t getMWINewMsgCount() = 0;

        




        virtual cc_uint32_t getMWIOldMsgCount() = 0;

        




        virtual cc_uint32_t getMWIPrioNewMsgCount() = 0;

        




        virtual cc_uint32_t getMWIPrioOldMsgCount() = 0;

        





        virtual bool hasCapability(ccapi_call_capability_e capability) = 0;

        




        virtual std::bitset<CCAPI_CALL_CAP_MAX> getCapabilitySet() = 0;
    };
};
