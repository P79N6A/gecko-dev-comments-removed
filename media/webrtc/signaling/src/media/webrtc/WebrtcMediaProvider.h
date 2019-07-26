






































#ifndef WebrtcMEDIAPROVIDER_H_
#define WebrtcMEDIAPROVIDER_H_

#ifndef _USE_CPVE

#include <CSFMediaProvider.h>
#include "voe_base.h"
#include <string>


namespace CSF
{
	enum	
	{
		eVideoModeChanged,
		eKeyFrameRequest,
		eMediaLost,
		eMediaRestored
	};


#define WEBRTC_MASTER_KEY_LENGTH      16
#define WEBRTC_MASTER_SALT_LENGTH     14
#define WEBRTC_KEY_LENGTH             (WEBRTC_MASTER_KEY_LENGTH + WEBRTC_MASTER_SALT_LENGTH)
#define WEBRTC_CIPHER_LENGTH          WEBRTC_KEY_LENGTH

	class WebrtcAudioProvider;
	class WebrtcVideoProvider;

	class WebrtcMediaProvider : public MediaProvider
	{
		friend class MediaProvider;
        friend class WebrtcVideoProvider;
        friend class WebrtcAudioProvider;

	protected:
		WebrtcMediaProvider( );
		~WebrtcMediaProvider();

		int init();
		virtual void shutdown();

		AudioControl* getAudioControl();
		VideoControl* getVideoControl();
		AudioTermination* getAudioTermination();
		VideoTermination* getVideoTermination();
		void addMediaProviderObserver( MediaProviderObserver* observer );

        bool getKey(
            const unsigned char* masterKey, 
            int masterKeyLen, 
            const unsigned char* masterSalt, 
            int masterSaltLen,
            unsigned char* key,
            unsigned int keyLen
            );

	private:
		WebrtcAudioProvider* pAudio;
		WebrtcVideoProvider* pVideo;

        webrtc::VoiceEngine * getWebrtcVoiceEngine ();
	};

} 

#endif
#endif 
