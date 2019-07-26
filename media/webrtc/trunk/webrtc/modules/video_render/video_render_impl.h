









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_IMPL_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_IMPL_H_

#include "engine_configurations.h"
#include "video_render.h"
#include "map_wrapper.h"

namespace webrtc {
class CriticalSectionWrapper;
class IncomingVideoStream;
class IVideoRender;
class MapWrapper;


class ModuleVideoRenderImpl: public VideoRender
{
public:
    


    ModuleVideoRenderImpl(const WebRtc_Word32 id,
                          const VideoRenderType videoRenderType,
                          void* window, const bool fullscreen);

    virtual ~ModuleVideoRenderImpl();

    


    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

    virtual WebRtc_Word32 TimeUntilNextProcess();
    virtual WebRtc_Word32 Process();

    


    virtual void* Window();

    


    virtual WebRtc_Word32 ChangeWindow(void* window);

    


    WebRtc_Word32 Id();

    





    


    virtual VideoRenderCallback
            * AddIncomingRenderStream(const WebRtc_UWord32 streamId,
                                      const WebRtc_UWord32 zOrder,
                                      const float left, const float top,
                                      const float right, const float bottom);
    


    virtual WebRtc_Word32
            DeleteIncomingRenderStream(const WebRtc_UWord32 streamId);

    


    virtual WebRtc_Word32
            AddExternalRenderCallback(const WebRtc_UWord32 streamId,
                                      VideoRenderCallback* renderObject);

    


    virtual WebRtc_Word32
            GetIncomingRenderStreamProperties(const WebRtc_UWord32 streamId,
                                              WebRtc_UWord32& zOrder,
                                              float& left, float& top,
                                              float& right, float& bottom) const;
    


    virtual WebRtc_UWord32 GetIncomingFrameRate(const WebRtc_UWord32 streamId);

    


    virtual WebRtc_UWord32 GetNumIncomingRenderStreams() const;

    


    virtual bool HasIncomingRenderStream(const WebRtc_UWord32 streamId) const;

    


    virtual WebRtc_Word32
            RegisterRawFrameCallback(const WebRtc_UWord32 streamId,
                                     VideoRenderCallback* callbackObj);

    virtual WebRtc_Word32 GetLastRenderedFrame(const WebRtc_UWord32 streamId,
                                               I420VideoFrame &frame) const;

    virtual WebRtc_Word32 SetExpectedRenderDelay(WebRtc_UWord32 stream_id,
                                                 WebRtc_Word32 delay_ms);

    





    


    virtual WebRtc_Word32 StartRender(const WebRtc_UWord32 streamId);

    


    virtual WebRtc_Word32 StopRender(const WebRtc_UWord32 streamId);

    


    virtual WebRtc_Word32 ResetRender();

    





    


    virtual RawVideoType PreferredVideoType() const;

    


    virtual bool IsFullScreen();

    


    virtual WebRtc_Word32
            GetScreenResolution(WebRtc_UWord32& screenWidth,
                                WebRtc_UWord32& screenHeight) const;

    



    virtual WebRtc_UWord32 RenderFrameRate(const WebRtc_UWord32 streamId);

    


    virtual WebRtc_Word32 SetStreamCropping(const WebRtc_UWord32 streamId,
                                            const float left, const float top,
                                            const float right,
                                            const float bottom);

    virtual WebRtc_Word32 ConfigureRenderer(const WebRtc_UWord32 streamId,
                                            const unsigned int zOrder,
                                            const float left, const float top,
                                            const float right,
                                            const float bottom);

    virtual WebRtc_Word32 SetTransparentBackground(const bool enable);

    virtual WebRtc_Word32 FullScreenRender(void* window, const bool enable);

    virtual WebRtc_Word32 SetBitmap(const void* bitMap,
                                    const WebRtc_UWord8 pictureId,
                                    const void* colorKey, const float left,
                                    const float top, const float right,
                                    const float bottom);

    virtual WebRtc_Word32 SetText(const WebRtc_UWord8 textId,
                                  const WebRtc_UWord8* text,
                                  const WebRtc_Word32 textLength,
                                  const WebRtc_UWord32 textColorRef,
                                  const WebRtc_UWord32 backgroundColorRef,
                                  const float left, const float top,
                                  const float right, const float bottom);

    virtual WebRtc_Word32 SetStartImage(const WebRtc_UWord32 streamId,
                                        const I420VideoFrame& videoFrame);

    virtual WebRtc_Word32 SetTimeoutImage(const WebRtc_UWord32 streamId,
                                          const I420VideoFrame& videoFrame,
                                          const WebRtc_UWord32 timeout);

    virtual WebRtc_Word32 MirrorRenderStream(const int renderId,
                                             const bool enable,
                                             const bool mirrorXAxis,
                                             const bool mirrorYAxis);

private:
    WebRtc_Word32 _id;
    CriticalSectionWrapper& _moduleCrit;
    void* _ptrWindow;
    bool _fullScreen;

    IVideoRender* _ptrRenderer;
    MapWrapper& _streamRenderMap;
};

} 

#endif  
