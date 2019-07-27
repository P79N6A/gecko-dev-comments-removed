



#ifndef _CC_SIPCC_CALLSERVERINFO_H
#define _CC_SIPCC_CALLSERVERINFO_H

#include "CC_CallServerInfo.h"

#include "common/Wrapper.h"

namespace CSF
{
	DECLARE_NS_PTR(CC_SIPCCCallServerInfo);
    class CC_SIPCCCallServerInfo : public CC_CallServerInfo
    {
    private:
        cc_callserver_ref_t callserverinfo_ref;

        explicit CC_SIPCCCallServerInfo (cc_callserver_ref_t aCallServerInfo);

        CSF_DECLARE_WRAP(CC_SIPCCCallServerInfo, cc_callserver_ref_t);

    public:
        ~CC_SIPCCCallServerInfo();

    public:
        virtual std::string getCallServerName();
        virtual cc_cucm_mode_t getCallServerMode();
        virtual cc_ccm_status_t getCallServerStatus();
    };
};

#endif
