









#include "video_render_linux_impl.h"

#include "critical_section_wrapper.h"
#include "trace.h"
#include "video_x11_render.h"

#include <X11/Xlib.h>

namespace webrtc {

VideoRenderLinuxImpl::VideoRenderLinuxImpl(
                                           const int32_t id,
                                           const VideoRenderType videoRenderType,
                                           void* window, const bool fullscreen) :
            _id(id),
            _renderLinuxCritsect(
                                 *CriticalSectionWrapper::CreateCriticalSection()),
            _ptrWindow(window), _ptrX11Render(NULL)
{
}

VideoRenderLinuxImpl::~VideoRenderLinuxImpl()
{
    if (_ptrX11Render)
        delete _ptrX11Render;

    delete &_renderLinuxCritsect;
}

int32_t VideoRenderLinuxImpl::Init()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s",
                 __FUNCTION__);

    CriticalSectionScoped cs(&_renderLinuxCritsect);
    _ptrX11Render = new VideoX11Render((Window) _ptrWindow);
    if (!_ptrX11Render)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                     "%s",
                     "Failed to create instance of VideoX11Render object");
        return -1;
    }
    int retVal = _ptrX11Render->Init();
    if (retVal == -1)
    {
        return -1;
    }

    return 0;

}

int32_t VideoRenderLinuxImpl::ChangeUniqueId(const int32_t id)
{
    CriticalSectionScoped cs(&_renderLinuxCritsect);

    _id = id;
    return 0;
}

int32_t VideoRenderLinuxImpl::ChangeWindow(void* window)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s",
                 __FUNCTION__);

    CriticalSectionScoped cs(&_renderLinuxCritsect);
    _ptrWindow = window;

    if (_ptrX11Render)
    {
        return _ptrX11Render->ChangeWindow((Window) window);
    }

    return -1;
}

VideoRenderCallback* VideoRenderLinuxImpl::AddIncomingRenderStream(
                                                                       const uint32_t streamId,
                                                                       const uint32_t zOrder,
                                                                       const float left,
                                                                       const float top,
                                                                       const float right,
                                                                       const float bottom)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s",
                 __FUNCTION__);
    CriticalSectionScoped cs(&_renderLinuxCritsect);

    VideoRenderCallback* renderCallback = NULL;
    if (_ptrX11Render)
    {
        VideoX11Channel* renderChannel =
                _ptrX11Render->CreateX11RenderChannel(streamId, zOrder, left,
                                                      top, right, bottom);
        if (!renderChannel)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                         "Render channel creation failed for stream id: %d",
                         streamId);
            return NULL;
        }
        renderCallback = (VideoRenderCallback *) renderChannel;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                     "_ptrX11Render is NULL");
        return NULL;
    }
    return renderCallback;
}

int32_t VideoRenderLinuxImpl::DeleteIncomingRenderStream(
                                                               const uint32_t streamId)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s",
                 __FUNCTION__);
    CriticalSectionScoped cs(&_renderLinuxCritsect);

    if (_ptrX11Render)
    {
        return _ptrX11Render->DeleteX11RenderChannel(streamId);
    }
    return -1;
}

int32_t VideoRenderLinuxImpl::GetIncomingRenderStreamProperties(
                                                                      const uint32_t streamId,
                                                                      uint32_t& zOrder,
                                                                      float& left,
                                                                      float& top,
                                                                      float& right,
                                                                      float& bottom) const
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s",
                 __FUNCTION__);
    CriticalSectionScoped cs(&_renderLinuxCritsect);

    if (_ptrX11Render)
    {
        return _ptrX11Render->GetIncomingStreamProperties(streamId, zOrder,
                                                          left, top, right,
                                                          bottom);
    }
    return -1;
}

int32_t VideoRenderLinuxImpl::StartRender()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s",
                 __FUNCTION__);
    return 0;
}

int32_t VideoRenderLinuxImpl::StopRender()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s",
                 __FUNCTION__);
    return 0;
}

VideoRenderType VideoRenderLinuxImpl::RenderType()
{
    return kRenderX11;
}

RawVideoType VideoRenderLinuxImpl::PerferedVideoType()
{
    return kVideoI420;
}

bool VideoRenderLinuxImpl::FullScreen()
{
    return false;
}

int32_t VideoRenderLinuxImpl::GetGraphicsMemory(
                                                      uint64_t& ,
                                                      uint64_t& ) const
{
    WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                 "%s - not supported on Linux", __FUNCTION__);
    return -1;
}

int32_t VideoRenderLinuxImpl::GetScreenResolution(
                                                        uint32_t& ,
                                                        uint32_t& ) const
{
    return -1;
}

uint32_t VideoRenderLinuxImpl::RenderFrameRate(const uint32_t )
{
    return -1;
}

int32_t VideoRenderLinuxImpl::SetStreamCropping(
                                                      const uint32_t ,
                                                      const float ,
                                                      const float ,
                                                      const float ,
                                                      const float )
{
    WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                 "%s - not supported on Linux", __FUNCTION__);
    return -1;
}

int32_t VideoRenderLinuxImpl::SetTransparentBackground(const bool )
{
    WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                 "%s - not supported on Linux", __FUNCTION__);
    return -1;
}

int32_t VideoRenderLinuxImpl::ConfigureRenderer(
                                                      const uint32_t streamId,
                                                      const unsigned int zOrder,
                                                      const float left,
                                                      const float top,
                                                      const float right,
                                                      const float bottom)
{
    WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                 "%s - not supported on Linux", __FUNCTION__);
    return -1;
}

int32_t VideoRenderLinuxImpl::SetText(
                                            const uint8_t textId,
                                            const uint8_t* text,
                                            const int32_t textLength,
                                            const uint32_t textColorRef,
                                            const uint32_t backgroundColorRef,
                                            const float left, const float top,
                                            const float rigth,
                                            const float bottom)
{
    WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                 "%s - not supported on Linux", __FUNCTION__);
    return -1;
}

int32_t VideoRenderLinuxImpl::SetBitmap(const void* bitMap,
                                        const uint8_t pictureId,
                                        const void* colorKey,
                                        const float left,
                                        const float top,
                                        const float right,
                                        const float bottom)
{
    WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
                 "%s - not supported on Linux", __FUNCTION__);
    return -1;
}

} 

