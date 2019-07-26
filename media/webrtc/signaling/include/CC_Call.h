






































#pragma once

#include "CC_Common.h"
#include "ECC_Types.h"

extern "C"
{
#include "ccapi_types.h"
}

namespace CSF
{
    class ECC_API CC_Call
    {
    protected:
        CC_Call () { }

    public:
        virtual ~CC_Call () {};

		virtual void setRemoteWindow (VideoWindowHandle window) = 0;

		virtual int setExternalRenderer(VideoFormat videoFormat, ExternalRendererHandle renderer) = 0;

		virtual void sendIFrame	() = 0;

        virtual CC_CallInfoPtr getCallInfo () = 0;

        virtual std::string toString() = 0;

        







        virtual bool originateCall (cc_sdp_direction_t video_pref, const std::string & digits, char* sdp, int audioPort, int videoPort) = 0;

        






        virtual bool answerCall (cc_sdp_direction_t video_pref) = 0;

        








        virtual bool hold (cc_hold_reason_t reason) = 0;

        






        virtual bool resume (cc_sdp_direction_t video_pref) = 0;

        




        virtual bool endCall() = 0;
        
        






        virtual bool sendDigit (cc_digit_t digit) = 0;

        




        virtual bool backspace() = 0;

        





        virtual bool redial (cc_sdp_direction_t video_pref) = 0;

        




        virtual bool initiateCallForwardAll() = 0;

        




        virtual bool endConsultativeCall() = 0;

        






        virtual bool conferenceStart (cc_sdp_direction_t video_pref) = 0;

        







        virtual bool conferenceComplete (CC_CallPtr otherLog, cc_sdp_direction_t video_pref) = 0;

        






        virtual bool transferStart (cc_sdp_direction_t video_pref) = 0;

        







        virtual bool transferComplete (CC_CallPtr otherLeg, 
                                       cc_sdp_direction_t video_pref) = 0;

        




        virtual bool cancelTransferOrConferenceFeature() = 0;

        





        virtual bool directTransfer (CC_CallPtr target) = 0;

        





        virtual bool joinAcrossLine (CC_CallPtr target) = 0;

        






        virtual bool blfCallPickup (cc_sdp_direction_t video_pref, const std::string & speed) = 0;

        




        virtual bool select() = 0;

        





        virtual bool updateVideoMediaCap (cc_sdp_direction_t video_pref) = 0;

        







        virtual bool sendInfo (const std::string & infopackage, const std::string & infotype, const std::string & infobody) = 0;

        






        virtual bool muteAudio(void) = 0;
        
        
        






        virtual bool unmuteAudio(void) = 0;
        






        virtual bool muteVideo(void) = 0;
        
        
        






        virtual bool unmuteVideo(void) = 0;


        



        virtual bool setVolume(int volume) = 0;


        








        virtual bool originateP2PCall (cc_sdp_direction_t video_pref, const std::string & digits, const std::string & ip) = 0;

    };
};
