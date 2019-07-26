






































#ifndef _CSF_VCM_SIPCC_BINDING_H_
#define _CSF_VCM_SIPCC_BINDING_H_

extern "C"
{
#include "ccapi_types.h"
}

namespace CSF
{
    class AudioTermination;
    class VideoTermination;
    class AudioControl;
    class VideoControl;
    class MediaProvider;
    class MediaProviderObserver;

    class StreamObserver
    {
    public:
    	virtual void registerStream(cc_call_handle_t call, int streamId, bool isVideo) = 0;
    	virtual void deregisterStream(cc_call_handle_t call, int streamId) = 0;
    	virtual void dtmfBurst(int digit, int direction, int duration) = 0;
    	virtual void sendIFrame(cc_call_handle_t call) = 0;
    };
    class VcmSIPCCBinding
    {
    public:
        VcmSIPCCBinding (MediaProvider *mp);
        virtual ~VcmSIPCCBinding();


        
        void setStreamObserver(StreamObserver*);
        static StreamObserver* getStreamObserver();

        static AudioTermination * getAudioTermination();
        static VideoTermination * getVideoTermination();

        static AudioControl * getAudioControl();
        static VideoControl * getVideoControl();

        void setMediaProviderObserver(MediaProviderObserver* obs);
        static MediaProviderObserver * getMediaProviderObserver();
    private:
        static VcmSIPCCBinding * _pSelf;
        MediaProvider * pMediaProvider;
        StreamObserver* streamObserver;
        MediaProviderObserver *mediaProviderObserver;
    };
}

#endif


