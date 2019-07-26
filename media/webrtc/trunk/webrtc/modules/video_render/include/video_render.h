









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_INTERFACE_VIDEO_RENDER_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_INTERFACE_VIDEO_RENDER_H_










#include "modules/interface/module.h"
#include "webrtc/modules/video_render/include/video_render_defines.h"

namespace webrtc {

#if defined(WEBRTC_ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
WebRtc_Word32 SetRenderAndroidVM(void* javaVM);
#endif


class VideoRender: public Module
{
public:
    







    static VideoRender
            * CreateVideoRender(
                                          const WebRtc_Word32 id,
                                          void* window,
                                          const bool fullscreen,
                                          const VideoRenderType videoRenderType =
                                                  kRenderDefault);

    




    static void DestroyVideoRender(VideoRender* module);

    




    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id) = 0;

    virtual WebRtc_Word32 TimeUntilNextProcess() = 0;
    virtual WebRtc_Word32 Process() = 0;

    





    


    virtual void* Window() = 0;

    




    virtual WebRtc_Word32 ChangeWindow(void* window) = 0;

    





    











    virtual VideoRenderCallback
            * AddIncomingRenderStream(const WebRtc_UWord32 streamId,
                                      const WebRtc_UWord32 zOrder,
                                      const float left, const float top,
                                      const float right, const float bottom) = 0;
    




    virtual WebRtc_Word32
            DeleteIncomingRenderStream(const WebRtc_UWord32 streamId) = 0;

    







    virtual WebRtc_Word32
            AddExternalRenderCallback(const WebRtc_UWord32 streamId,
                                      VideoRenderCallback* renderObject) = 0;

    









    virtual WebRtc_Word32
            GetIncomingRenderStreamProperties(const WebRtc_UWord32 streamId,
                                              WebRtc_UWord32& zOrder,
                                              float& left, float& top,
                                              float& right, float& bottom) const = 0;
    


    virtual WebRtc_UWord32
            GetIncomingFrameRate(const WebRtc_UWord32 streamId) = 0;

    


    virtual WebRtc_UWord32 GetNumIncomingRenderStreams() const = 0;

    


    virtual bool
            HasIncomingRenderStream(const WebRtc_UWord32 streamId) const = 0;

    



    virtual WebRtc_Word32
            RegisterRawFrameCallback(const WebRtc_UWord32 streamId,
                                     VideoRenderCallback* callbackObj) = 0;

    


    virtual WebRtc_Word32
            GetLastRenderedFrame(const WebRtc_UWord32 streamId,
                                 I420VideoFrame &frame) const = 0;

    





    


    virtual WebRtc_Word32 StartRender(const WebRtc_UWord32 streamId) = 0;

    


    virtual WebRtc_Word32 StopRender(const WebRtc_UWord32 streamId) = 0;

    



    virtual WebRtc_Word32 ResetRender() = 0;

    





    


    virtual RawVideoType PreferredVideoType() const = 0;

    


    virtual bool IsFullScreen() = 0;

    


    virtual WebRtc_Word32
            GetScreenResolution(WebRtc_UWord32& screenWidth,
                                WebRtc_UWord32& screenHeight) const = 0;

    



    virtual WebRtc_UWord32 RenderFrameRate(const WebRtc_UWord32 streamId) = 0;

    


    virtual WebRtc_Word32 SetStreamCropping(const WebRtc_UWord32 streamId,
                                            const float left, const float top,
                                            const float right,
                                            const float bottom) = 0;

    



    
    
    
    virtual WebRtc_Word32 SetExpectedRenderDelay(WebRtc_UWord32 stream_id,
                                                 WebRtc_Word32 delay_ms) = 0;

    virtual WebRtc_Word32 ConfigureRenderer(const WebRtc_UWord32 streamId,
                                            const unsigned int zOrder,
                                            const float left, const float top,
                                            const float right,
                                            const float bottom) = 0;

    virtual WebRtc_Word32 SetTransparentBackground(const bool enable) = 0;

    virtual WebRtc_Word32 FullScreenRender(void* window, const bool enable) = 0;

    virtual WebRtc_Word32 SetBitmap(const void* bitMap,
                                    const WebRtc_UWord8 pictureId,
                                    const void* colorKey, const float left,
                                    const float top, const float right,
                                    const float bottom) = 0;

    virtual WebRtc_Word32 SetText(const WebRtc_UWord8 textId,
                                  const WebRtc_UWord8* text,
                                  const WebRtc_Word32 textLength,
                                  const WebRtc_UWord32 textColorRef,
                                  const WebRtc_UWord32 backgroundColorRef,
                                  const float left, const float top,
                                  const float right, const float bottom) = 0;

    


    virtual WebRtc_Word32
            SetStartImage(const WebRtc_UWord32 streamId,
                          const I420VideoFrame& videoFrame) = 0;

    


    virtual WebRtc_Word32 SetTimeoutImage(const WebRtc_UWord32 streamId,
                                          const I420VideoFrame& videoFrame,
                                          const WebRtc_UWord32 timeout)= 0;

    virtual WebRtc_Word32 MirrorRenderStream(const int renderId,
                                             const bool enable,
                                             const bool mirrorXAxis,
                                             const bool mirrorYAxis) = 0;
};
} 
#endif  
