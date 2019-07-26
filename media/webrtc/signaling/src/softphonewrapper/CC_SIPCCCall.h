






































#ifndef _CC_SIPCCCALL_H
#define _CC_SIPCCCALL_H

#include "CC_Call.h"

#include <map>
#include <iomanip>
#include <sstream>

#include "common/Wrapper.h"
#include "base/lock.h"

namespace CSF
{
    struct StreamInfo
    {
		
    	bool isVideo;                    
    };
    typedef std::map<int, StreamInfo> StreamMapType;

    DECLARE_PTR(CC_SIPCCCallMediaData);

    class CC_SIPCCCallMediaData
	{
	public:
		CC_SIPCCCallMediaData(): remoteWindow(NULL), audioMuteState(false), videoMuteState(false), volume(-1){}
		CC_SIPCCCallMediaData(VideoWindowHandle remoteWindow,
            bool audioMuteState, bool videoMuteState, int volume): remoteWindow(remoteWindow),
            audioMuteState(audioMuteState), videoMuteState(videoMuteState), volume(volume) {}
        VideoWindowHandle remoteWindow; 
		ExternalRendererHandle extRenderer;
		VideoFormat videoFormat;	
        Lock streamMapMutex;
        StreamMapType streamMap;
        bool audioMuteState;
        bool videoMuteState; 
        int volume;        
    private:
        CC_SIPCCCallMediaData(const CC_SIPCCCallMediaData&);
        CC_SIPCCCallMediaData& operator=(const CC_SIPCCCallMediaData&);
	};

	DECLARE_PTR(CC_SIPCCCall);
    class CC_SIPCCCall : public CC_Call
    {

    CSF_DECLARE_WRAP(CC_SIPCCCall, cc_call_handle_t);
    private:
        cc_call_handle_t callHandle;
        CC_SIPCCCall (cc_call_handle_t aCallHandle);
        CC_SIPCCCallMediaDataPtr  pMediaData;

    public:
        virtual inline std::string toString() {
        	std::stringstream sstream;
            sstream << "0x" << std::setw( 5 ) << std::setfill( '0' ) << std::hex << callHandle;
            return sstream.str();
        };

        virtual void setRemoteWindow (VideoWindowHandle window);
        virtual int setExternalRenderer(VideoFormat vFormat, ExternalRendererHandle renderer);
		virtual void sendIFrame();

        virtual CC_CallInfoPtr getCallInfo ();

        virtual bool originateCall (cc_sdp_direction_t video_pref, const std::string & digits, char* sdp, int audioPort, int videoPort);
        virtual bool answerCall (cc_sdp_direction_t video_pref);
        virtual bool hold (cc_hold_reason_t reason);
        virtual bool resume (cc_sdp_direction_t video_pref);
        virtual bool endCall();
        virtual bool sendDigit (cc_digit_t digit);
        virtual bool backspace();
        virtual bool redial (cc_sdp_direction_t video_pref);
        virtual bool initiateCallForwardAll();
        virtual bool endConsultativeCall();
        virtual bool conferenceStart (cc_sdp_direction_t video_pref);
        virtual bool conferenceComplete (CC_CallPtr otherLog, cc_sdp_direction_t video_pref);
        virtual bool transferStart (cc_sdp_direction_t video_pref);
        virtual bool transferComplete (CC_CallPtr otherLeg, 
                                       cc_sdp_direction_t video_pref);
        virtual bool cancelTransferOrConferenceFeature();
        virtual bool directTransfer (CC_CallPtr target);
        virtual bool joinAcrossLine (CC_CallPtr target);
        virtual bool blfCallPickup (cc_sdp_direction_t video_pref, const std::string & speed);
        virtual bool select();
        virtual bool updateVideoMediaCap (cc_sdp_direction_t video_pref);
        virtual bool sendInfo (const std::string & infopackage, const std::string & infotype, const std::string & infobody);
        virtual bool muteAudio();
        virtual bool unmuteAudio();
        virtual bool muteVideo();
        virtual bool unmuteVideo();
        virtual void addStream(int streamId, bool isVideo);
        virtual void removeStream(int streamId);
        virtual bool setVolume(int volume);
        virtual bool originateP2PCall (cc_sdp_direction_t video_pref, const std::string & digits, const std::string & ip);

        virtual CC_SIPCCCallMediaDataPtr getMediaData();

    private:
        virtual bool setAudioMute(bool mute);
        virtual bool setVideoMute(bool mute);

        Lock m_lock;
    };

};


#endif
