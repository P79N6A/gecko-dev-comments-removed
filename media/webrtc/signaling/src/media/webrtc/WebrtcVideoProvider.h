






































#ifndef WebrtcVideoProvider_H_
#define WebrtcVideoProvider_H_

#ifndef _USE_CPVE

#include <CSFVideoControl.h>
#include <CSFVideoTermination.h>

#include "vie_base.h"
#include "vie_codec.h"
#include "vie_render.h"
#include "vie_capture.h"
#include "vie_rtp_rtcp.h"
#include "vie_encryption.h"
#include "vie_network.h"

#ifdef WIN32
typedef HWND                    WebrtcPlatformWindow;
#endif
#ifdef LINUX
typedef void*                    WebrtcPlatformWindow;    
#endif
#ifdef __APPLE__
typedef void*                    WebrtcPlatformWindow;
#endif

#include <string>
#include <map>

namespace CSF
{

	class WebrtcVideoStream
	{
	public:
		WebrtcVideoStream(int _streamId, int _channelId):
			streamId(_streamId), channelId(_channelId),	isMuted(false), txInitialised(false)
			{}
        int streamId;
        int channelId;
        bool isMuted;
        bool txInitialised;
	};

    class WebrtcMediaProvider;

    DECLARE_PTR(WebrtcVideoStream);

    class WebrtcVideoProvider : public VideoControl, 
								VideoTermination, 
								webrtc::ViEEncoderObserver,
								webrtc::ViEDecoderObserver,
								webrtc::ViERTPObserver	
    {
    public:
        WebrtcVideoProvider( WebrtcMediaProvider* provider );
        ~WebrtcVideoProvider();

        
        
        int init();

        
        VideoControl* getMediaControl() { return this; }

        void setVideoMode( bool enable );

        std::vector<std::string> getCaptureDevices();

        std::string getCaptureDevice() { return captureDevice; }
        bool setCaptureDevice( const std::string& name );

        
        VideoTermination* getMediaTermination() { return this; }

        
        void setRemoteWindow( int streamId, VideoWindowHandle window );
        int setExternalRenderer( int streamId, VideoFormat videoFormat, ExternalRendererHandle renderer);

        
        void setPreviewWindow( VideoWindowHandle window, int top, int left, int bottom, int right, RenderScaling style );
        void showPreviewWindow( bool show ) {}    

        void sendIFrame    ( int streamId );

        
        int  getCodecList( CodecRequestType requestType );

        int  rxAlloc    ( int groupId, int streamId, int requestedPort );
        int  rxOpen     ( int groupId, int streamId, int requestedPort, int listenIp, bool isMulticast );
        int  rxStart    ( int groupId, int streamId, int payloadType, int packPeriod, int localPort, int rfc2833PayloadType,
                          EncryptionAlgorithm algorithm, unsigned char* key, int keyLen, unsigned char* salt, int saltLen, int mode, int party);
        void rxClose    ( int groupId, int streamId );
        void rxRelease  ( int groupId, int streamId, int port );
        int  txStart    ( int groupId, int streamId, int payloadType, int packPeriod, bool vad, short tos,
                          char* remoteIpAddr, int remotePort, int rfc2833PayloadType, EncryptionAlgorithm algorithm,
                          unsigned char* key, int keyLen, unsigned char* salt, int saltLen, int mode, int party );
        void txClose    ( int groupId, int streamId );

        void setLocalIP    ( const char* addr ) { localIP = addr; }
        void setMediaPorts ( int startPort, int endPort ) { this->startPort = startPort; this->endPort = endPort; }
        void setDSCPValue (int value){this->DSCPValue = value;}
        

        bool mute(int streamID, bool muteVideo);
        bool isMuted(int streamID);

        bool setFullScreen(int streamId, bool fullScreen);
        void setRenderWindowForStreamIdFromMap(int streamId);
        void setAudioStreamId( int streamId );

    protected:
        
        void RequestNewKeyFrame		(int channel);
        void OutgoingRate           ( int channel, unsigned int frameRate, unsigned int bitrate );
        void IncomingRate            ( int channel, unsigned int frameRate, unsigned int bitrate );
        void IncomingCodecChanged    ( int channel, const webrtc::VideoCodec& videoCodec);
        void IncomingCSRCChanged  	 ( int channel, unsigned int csrc, bool added );
        void IncomingSSRCChanged  	 ( int channel, unsigned int ssrc);

        WebrtcVideoStreamPtr getStreamByChannel( int channel );

        int getChannelForStreamId( int streamId );
		WebrtcVideoStreamPtr getStream( int streamId );
		void setMuteForStreamId( int streamId, bool muteVideo );
		void setTxInitiatedForStreamId( int streamId, bool txInitiatedValue );

        class RenderWindow
        {
        public:
            RenderWindow( WebrtcPlatformWindow window )
                : window(window) {}

            WebrtcPlatformWindow window;
        };

        void setRenderWindow( int streamId, WebrtcPlatformWindow window);
        const RenderWindow* getRenderWindow( int streamId );

    private:
        WebrtcMediaProvider* provider;
        webrtc::VideoEngine* vieVideo;
	webrtc::ViEBase* vieBase;
	webrtc::ViECapture* vieCapture;
	webrtc::ViERender* vieRender;
	webrtc::ViECodec* vieCodec;
	webrtc::ViENetwork* vieNetwork;
        webrtc::ViEEncryption* vieEncryption;
        webrtc::ViERTP_RTCP* vieRtpRtcp;
        std::map<int, RenderWindow> streamIdToWindow;
        std::map<int, WebrtcVideoStreamPtr> streamMap;
        std::string captureDevice;
	std::string capureDevieUniqueId;
        std::string localIP;
        bool videoMode;
        int startPort;
        int endPort;
	int localRenderId;
	int webCaptureId;
	int vp8Idx;
        RenderWindow* previewWindow;
        int DSCPValue;
        
        
        Lock m_lock;
        
        
        
        Lock streamMapMutex;
        int audioStreamId;
    };

} 

#endif

#endif
