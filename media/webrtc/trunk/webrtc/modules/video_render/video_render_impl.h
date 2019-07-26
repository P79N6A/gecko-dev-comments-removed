









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_IMPL_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_IMPL_H_

#include <map>

#include "webrtc/engine_configurations.h"
#include "webrtc/modules/video_render/include/video_render.h"

namespace webrtc {
class CriticalSectionWrapper;
class IncomingVideoStream;
class IVideoRender;


class ModuleVideoRenderImpl: public VideoRender
{
public:
    


    ModuleVideoRenderImpl(const int32_t id,
                          const VideoRenderType videoRenderType,
                          void* window, const bool fullscreen);

    virtual ~ModuleVideoRenderImpl();

    


    virtual int32_t ChangeUniqueId(const int32_t id);

    virtual int32_t TimeUntilNextProcess();
    virtual int32_t Process();

    


    virtual void* Window();

    


    virtual int32_t ChangeWindow(void* window);

    


    int32_t Id();

    





    


    virtual VideoRenderCallback
            * AddIncomingRenderStream(const uint32_t streamId,
                                      const uint32_t zOrder,
                                      const float left, const float top,
                                      const float right, const float bottom);
    


    virtual int32_t
            DeleteIncomingRenderStream(const uint32_t streamId);

    


    virtual int32_t
            AddExternalRenderCallback(const uint32_t streamId,
                                      VideoRenderCallback* renderObject);

    


    virtual int32_t
            GetIncomingRenderStreamProperties(const uint32_t streamId,
                                              uint32_t& zOrder,
                                              float& left, float& top,
                                              float& right, float& bottom) const;
    


    virtual uint32_t GetIncomingFrameRate(const uint32_t streamId);

    


    virtual uint32_t GetNumIncomingRenderStreams() const;

    


    virtual bool HasIncomingRenderStream(const uint32_t streamId) const;

    


    virtual int32_t
            RegisterRawFrameCallback(const uint32_t streamId,
                                     VideoRenderCallback* callbackObj);

    virtual int32_t GetLastRenderedFrame(const uint32_t streamId,
                                         I420VideoFrame &frame) const;

    virtual int32_t SetExpectedRenderDelay(uint32_t stream_id,
                                           int32_t delay_ms);

    





    


    virtual int32_t StartRender(const uint32_t streamId);

    


    virtual int32_t StopRender(const uint32_t streamId);

    


    virtual int32_t ResetRender();

    





    


    virtual RawVideoType PreferredVideoType() const;

    


    virtual bool IsFullScreen();

    


    virtual int32_t
            GetScreenResolution(uint32_t& screenWidth,
                                uint32_t& screenHeight) const;

    



    virtual uint32_t RenderFrameRate(const uint32_t streamId);

    


    virtual int32_t SetStreamCropping(const uint32_t streamId,
                                      const float left, const float top,
                                      const float right, const float bottom);

    virtual int32_t ConfigureRenderer(const uint32_t streamId,
                                      const unsigned int zOrder,
                                      const float left, const float top,
                                      const float right, const float bottom);

    virtual int32_t SetTransparentBackground(const bool enable);

    virtual int32_t FullScreenRender(void* window, const bool enable);

    virtual int32_t SetBitmap(const void* bitMap,
                              const uint8_t pictureId,
                              const void* colorKey,
                              const float left, const float top,
                              const float right, const float bottom);

    virtual int32_t SetText(const uint8_t textId,
                            const uint8_t* text,
                            const int32_t textLength,
                            const uint32_t textColorRef,
                            const uint32_t backgroundColorRef,
                            const float left, const float top,
                            const float right, const float bottom);

    virtual int32_t SetStartImage(const uint32_t streamId,
                                  const I420VideoFrame& videoFrame);

    virtual int32_t SetTimeoutImage(const uint32_t streamId,
                                    const I420VideoFrame& videoFrame,
                                    const uint32_t timeout);

    virtual int32_t MirrorRenderStream(const int renderId,
                                       const bool enable,
                                       const bool mirrorXAxis,
                                       const bool mirrorYAxis);

private:
    int32_t _id;
    CriticalSectionWrapper& _moduleCrit;
    void* _ptrWindow;
    bool _fullScreen;

    IVideoRender* _ptrRenderer;
    typedef std::map<uint32_t, IncomingVideoStream*> IncomingVideoStreamMap;
    IncomingVideoStreamMap _streamRenderMap;
};

}  

#endif  
