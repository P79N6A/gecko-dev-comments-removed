



#pragma once

#include <set>

extern "C"
{
#include "ccapi_types.h"
#include "fsmdef_states.h"
}


#include "CC_Common.h"
#include "CC_CallTypes.h"
#include "peer_connection_types.h"

#if defined(__cplusplus) && __cplusplus >= 201103L
typedef struct Timecard Timecard;
#else
#include "timecard.h"
#endif

namespace CSF
{

	class ECC_API CC_CallInfo
    {
    public:
        NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CC_CallInfo)
    protected:
        CC_CallInfo() { }

        
        virtual ~CC_CallInfo() {};

    public:
        




        virtual CC_LinePtr getline () = 0;

        




        virtual cc_call_state_t getCallState () = 0;

        




        virtual fsmdef_states_t getFsmState () const = 0;

        




        virtual std::string callStateToString (cc_call_state_t state) = 0;

        




        virtual std::string fsmStateToString (fsmdef_states_t state) const = 0;

        




        virtual std::string callEventToString (ccapi_call_event_e callEvent) = 0;

        




        virtual bool getRingerState() = 0;

        




        virtual cc_call_attr_t getCallAttr() = 0;

        





        virtual cc_call_type_t getCallType() = 0;

        




        virtual std::string getCalledPartyName() = 0;

        




        virtual std::string getCalledPartyNumber() = 0;

        




        virtual std::string getCallingPartyName() = 0;

        




        virtual std::string getCallingPartyNumber() = 0;

        




        virtual std::string getAlternateNumber() = 0;

        






        virtual bool hasCapability (CC_CallCapabilityEnum::CC_CallCapability capability) = 0;

        




        virtual std::set<CC_CallCapabilityEnum::CC_CallCapability> getCapabilitySet() = 0;

        




        virtual std::string getOriginalCalledPartyName() = 0;

        




        virtual std::string getOriginalCalledPartyNumber() = 0;

        




        virtual std::string getLastRedirectingPartyName() = 0;

        




        virtual std::string getLastRedirectingPartyNumber() = 0;

        




        virtual std::string getPlacedCallPartyName() = 0;

        




        virtual std::string getPlacedCallPartyNumber() = 0;

        




        virtual cc_int32_t getCallInstance() = 0;

        




        virtual std::string getStatus() = 0;

        




        virtual cc_call_security_t getSecurity() = 0;

        




        virtual cc_int32_t getSelectionStatus() = 0;

        




        virtual std::string getGCID() = 0;

        




        virtual bool getIsRingOnce() = 0;

        




        virtual int getRingerMode() = 0;

        




        virtual cc_int32_t getOnhookReason() = 0;

        




        virtual bool getIsConference() = 0;

        






        virtual std::set<cc_int32_t> getStreamStatistics() = 0;

        




        virtual bool isCallSelected() = 0;

        




        virtual std::string getINFOPack() = 0;

        




        virtual std::string getINFOType() = 0;

        




        virtual std::string getINFOBody() = 0;

        







        virtual cc_calllog_ref_t  getCallLogRef() = 0;

        




        virtual cc_sdp_direction_t getVideoDirection() = 0;

         



        virtual bool isMediaStateAvailable() = 0;

        



        virtual bool isAudioMuted(void) = 0;

         



        virtual bool isVideoMuted(void) = 0;

        



        virtual int getVolume() = 0;

    };
};
