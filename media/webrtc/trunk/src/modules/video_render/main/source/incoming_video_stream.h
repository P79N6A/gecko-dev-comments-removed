









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_INCOMING_VIDEO_STREAM_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_INCOMING_VIDEO_STREAM_H_

#include "video_render.h"
#include "map_wrapper.h"

namespace webrtc {
class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;
class VideoRenderCallback;
class VideoRenderFrames;

struct VideoMirroring
{
    bool mirrorXAxis;
    bool mirrorYAxis;
    VideoMirroring() :
        mirrorXAxis(false), mirrorYAxis(false)
    {
    }
};


class IncomingVideoStream: public VideoRenderCallback
{
public:
    


    IncomingVideoStream(const WebRtc_Word32 moduleId,
                        const WebRtc_UWord32 streamId);
    ~IncomingVideoStream();

    WebRtc_Word32 ChangeModuleId(const WebRtc_Word32 id);

    
    VideoRenderCallback* ModuleCallback();
    virtual WebRtc_Word32 RenderFrame(const WebRtc_UWord32 streamId,
                                      VideoFrame& videoFrame);

    
    WebRtc_Word32 SetRenderCallback(VideoRenderCallback* renderCallback);

    
    WebRtc_Word32 SetExternalCallback(VideoRenderCallback* renderObject);

    


    WebRtc_Word32 Start();
    WebRtc_Word32 Stop();

    
    WebRtc_Word32 Reset();

    


    WebRtc_UWord32 StreamId() const;
    WebRtc_UWord32 IncomingRate() const;

    


    WebRtc_Word32 GetLastRenderedFrame(VideoFrame& videoFrame) const;

    WebRtc_Word32 SetStartImage(const VideoFrame& videoFrame);

    WebRtc_Word32 SetTimeoutImage(const VideoFrame& videoFrame,
                                  const WebRtc_UWord32 timeout);

    WebRtc_Word32 EnableMirroring(const bool enable,
                                  const bool mirrorXAxis,
                                  const bool mirrorYAxis);

protected:
    static bool IncomingVideoStreamThreadFun(void* obj);
    bool IncomingVideoStreamProcess();

private:

    
    enum
    {
        KEventStartupTimeMS = 10
    };
    enum
    {
        KEventMaxWaitTimeMs = 100
    };
    enum
    {
        KFrameRatePeriodMs = 1000
    };

    WebRtc_Word32 _moduleId;
    WebRtc_UWord32 _streamId;
    CriticalSectionWrapper& _streamCritsect; 
    CriticalSectionWrapper& _threadCritsect;
    CriticalSectionWrapper& _bufferCritsect;
    ThreadWrapper* _ptrIncomingRenderThread;
    EventWrapper& _deliverBufferEvent;
    bool _running;

    VideoRenderCallback* _ptrExternalCallback;
    VideoRenderCallback* _ptrRenderCallback;
    VideoRenderFrames& _renderBuffers;

    RawVideoType _callbackVideoType;
    WebRtc_UWord32 _callbackWidth;
    WebRtc_UWord32 _callbackHeight;

    WebRtc_UWord32 _incomingRate;
    WebRtc_Word64 _lastRateCalculationTimeMs;
    WebRtc_UWord16 _numFramesSinceLastCalculation;
    VideoFrame _lastRenderedFrame;
    VideoFrame _tempFrame;
    VideoFrame _startImage;
    VideoFrame _timeoutImage;
    WebRtc_UWord32 _timeoutTime;

    bool _mirrorFramesEnabled;
    VideoMirroring _mirroring;
    VideoFrame _transformedVideoFrame;
};

} 

#endif  
