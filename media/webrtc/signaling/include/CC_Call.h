



#pragma once

#include "CC_Common.h"
#include "ECC_Types.h"
#include "mozilla/RefPtr.h"

extern "C"
{
#include "ccapi_types.h"
}

#if defined(__cplusplus) && __cplusplus >= 201103L
typedef struct Timecard Timecard;
#else
#include "timecard.h"
#endif

namespace CSF
{
    class ECC_API CC_Call
    {
    public:
        NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CC_Call)

    protected:
        CC_Call () { }

        virtual ~CC_Call () {}

    public:
		virtual void setRemoteWindow (VideoWindowHandle window) = 0;

		virtual int setExternalRenderer(VideoFormat videoFormat, ExternalRendererHandle renderer) = 0;

		virtual void sendIFrame	() = 0;

        virtual CC_CallInfoPtr getCallInfo () = 0;

        virtual std::string toString() = 0;

        







        virtual bool originateCall (cc_sdp_direction_t video_pref, const std::string & digits) = 0;

        






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


        








        virtual void originateP2PCall (cc_sdp_direction_t video_pref, const std::string & digits, const std::string & ip) = 0;

        virtual void createOffer (cc_media_options_t* options, Timecard *) = 0;

        virtual void createAnswer(Timecard *) = 0;

        virtual void setLocalDescription(cc_jsep_action_t action, const std::string & sdp, Timecard *) = 0;

        virtual void setRemoteDescription(cc_jsep_action_t action, const std::string & sdp, Timecard *) = 0;

        virtual void setPeerConnection(const std::string& handle) = 0;

        virtual void addStream(cc_media_stream_id_t stream_id,
                               cc_media_track_id_t track_id,
                               cc_media_type_t media_type) = 0;

        virtual void removeStream(cc_media_stream_id_t stream_id, cc_media_track_id_t track_id, cc_media_type_t media_type) = 0;

        virtual const std::string& getPeerConnection() const = 0;

        virtual void addICECandidate(const std::string & candidate, const std::string & mid, unsigned short level, Timecard *) = 0;

        virtual void foundICECandidate(const std::string & candidate, const std::string & mid, unsigned short level, Timecard *) = 0;

    };
}

