









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_INTERFACE_VIDEO_RENDER_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_INTERFACE_VIDEO_RENDER_H_










#include "webrtc/modules/interface/module.h"
#include "webrtc/modules/video_render/include/video_render_defines.h"

namespace webrtc {

#if defined(WEBRTC_ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
int32_t SetRenderAndroidVM(void* javaVM);
#endif


class VideoRender: public Module
{
public:
    







    static VideoRender
            * CreateVideoRender(
                                          const int32_t id,
                                          void* window,
                                          const bool fullscreen,
                                          const VideoRenderType videoRenderType =
                                                  kRenderDefault);

    




    static void DestroyVideoRender(VideoRender* module);

    




    virtual int32_t ChangeUniqueId(const int32_t id) = 0;

    virtual int32_t TimeUntilNextProcess() = 0;
    virtual int32_t Process() = 0;

    





    


    virtual void* Window() = 0;

    




    virtual int32_t ChangeWindow(void* window) = 0;

    





    











    virtual VideoRenderCallback
            * AddIncomingRenderStream(const uint32_t streamId,
                                      const uint32_t zOrder,
                                      const float left, const float top,
                                      const float right, const float bottom) = 0;
    




    virtual int32_t
            DeleteIncomingRenderStream(const uint32_t streamId) = 0;

    







    virtual int32_t
            AddExternalRenderCallback(const uint32_t streamId,
                                      VideoRenderCallback* renderObject) = 0;

    









    virtual int32_t
            GetIncomingRenderStreamProperties(const uint32_t streamId,
                                              uint32_t& zOrder,
                                              float& left, float& top,
                                              float& right, float& bottom) const = 0;
    


    virtual uint32_t
            GetIncomingFrameRate(const uint32_t streamId) = 0;

    


    virtual uint32_t GetNumIncomingRenderStreams() const = 0;

    


    virtual bool
            HasIncomingRenderStream(const uint32_t streamId) const = 0;

    



    virtual int32_t
            RegisterRawFrameCallback(const uint32_t streamId,
                                     VideoRenderCallback* callbackObj) = 0;

    


    virtual int32_t
            GetLastRenderedFrame(const uint32_t streamId,
                                 I420VideoFrame &frame) const = 0;

    





    


    virtual int32_t StartRender(const uint32_t streamId) = 0;

    


    virtual int32_t StopRender(const uint32_t streamId) = 0;

    



    virtual int32_t ResetRender() = 0;

    





    


    virtual RawVideoType PreferredVideoType() const = 0;

    


    virtual bool IsFullScreen() = 0;

    


    virtual int32_t
            GetScreenResolution(uint32_t& screenWidth,
                                uint32_t& screenHeight) const = 0;

    



    virtual uint32_t RenderFrameRate(const uint32_t streamId) = 0;

    


    virtual int32_t SetStreamCropping(const uint32_t streamId,
                                      const float left,
                                      const float top,
                                      const float right,
                                      const float bottom) = 0;

    



    
    
    
    virtual int32_t SetExpectedRenderDelay(uint32_t stream_id,
                                           int32_t delay_ms) = 0;

    virtual int32_t ConfigureRenderer(const uint32_t streamId,
                                      const unsigned int zOrder,
                                      const float left,
                                      const float top,
                                      const float right,
                                      const float bottom) = 0;

    virtual int32_t SetTransparentBackground(const bool enable) = 0;

    virtual int32_t FullScreenRender(void* window, const bool enable) = 0;

    virtual int32_t SetBitmap(const void* bitMap,
                              const uint8_t pictureId,
                              const void* colorKey,
                              const float left, const float top,
                              const float right, const float bottom) = 0;

    virtual int32_t SetText(const uint8_t textId,
                            const uint8_t* text,
                            const int32_t textLength,
                            const uint32_t textColorRef,
                            const uint32_t backgroundColorRef,
                            const float left, const float top,
                            const float right, const float bottom) = 0;

    


    virtual int32_t
            SetStartImage(const uint32_t streamId,
                          const I420VideoFrame& videoFrame) = 0;

    


    virtual int32_t SetTimeoutImage(const uint32_t streamId,
                                    const I420VideoFrame& videoFrame,
                                    const uint32_t timeout)= 0;

    virtual int32_t MirrorRenderStream(const int renderId,
                                       const bool enable,
                                       const bool mirrorXAxis,
                                       const bool mirrorYAxis) = 0;
};
}  
#endif  
