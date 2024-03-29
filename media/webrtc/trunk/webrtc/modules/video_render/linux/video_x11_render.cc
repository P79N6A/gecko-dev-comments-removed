









#include "webrtc/modules/video_render/linux/video_x11_channel.h"
#include "webrtc/modules/video_render/linux/video_x11_render.h"

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

VideoX11Render::VideoX11Render(Window window) :
    _window(window),
            _critSect(*CriticalSectionWrapper::CreateCriticalSection())
{
}

VideoX11Render::~VideoX11Render()
{
    delete &_critSect;
}

int32_t VideoX11Render::Init()
{
    CriticalSectionScoped cs(&_critSect);

    _streamIdToX11ChannelMap.clear();

    return 0;
}

int32_t VideoX11Render::ChangeWindow(Window window)
{
    CriticalSectionScoped cs(&_critSect);
    VideoX11Channel* renderChannel = NULL;

    std::map<int, VideoX11Channel*>::iterator iter =
            _streamIdToX11ChannelMap.begin();

    while (iter != _streamIdToX11ChannelMap.end())
    {
        renderChannel = iter->second;
        if (renderChannel)
        {
            renderChannel->ChangeWindow(window);
        }
        iter++;
    }

    _window = window;

    return 0;
}

VideoX11Channel* VideoX11Render::CreateX11RenderChannel(
                                                                int32_t streamId,
                                                                int32_t zOrder,
                                                                const float left,
                                                                const float top,
                                                                const float right,
                                                                const float bottom)
{
    CriticalSectionScoped cs(&_critSect);
    VideoX11Channel* renderChannel = NULL;

    std::map<int, VideoX11Channel*>::iterator iter =
            _streamIdToX11ChannelMap.find(streamId);

    if (iter == _streamIdToX11ChannelMap.end())
    {
        renderChannel = new VideoX11Channel(streamId);
        if (!renderChannel)
        {
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceVideoRenderer,
                         -1,
                         "Failed to create VideoX11Channel for streamId : %d",
                         streamId);
            return NULL;
        }
        renderChannel->Init(_window, left, top, right, bottom);
        _streamIdToX11ChannelMap[streamId] = renderChannel;
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, -1,
                     "Render Channel already exists for streamId: %d", streamId);
        renderChannel = iter->second;
    }

    return renderChannel;
}

int32_t VideoX11Render::DeleteX11RenderChannel(int32_t streamId)
{
    CriticalSectionScoped cs(&_critSect);

    std::map<int, VideoX11Channel*>::iterator iter =
            _streamIdToX11ChannelMap.find(streamId);
    if (iter != _streamIdToX11ChannelMap.end())
    {
        VideoX11Channel *renderChannel = iter->second;
        if (renderChannel)
        {
            renderChannel->ReleaseWindow();
            delete renderChannel;
            renderChannel = NULL;
        }
        _streamIdToX11ChannelMap.erase(iter);
    }

    WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, -1,
                 "No VideoX11Channel object exists for stream id: %d",
                 streamId);
    return -1;
}

int32_t VideoX11Render::GetIncomingStreamProperties(
                                                              int32_t streamId,
                                                              uint32_t& zOrder,
                                                              float& left,
                                                              float& top,
                                                              float& right,
                                                              float& bottom)
{
    CriticalSectionScoped cs(&_critSect);

    std::map<int, VideoX11Channel*>::iterator iter =
            _streamIdToX11ChannelMap.find(streamId);
    if (iter != _streamIdToX11ChannelMap.end())
    {
        VideoX11Channel *renderChannel = iter->second;
        if (renderChannel)
        {
            renderChannel->GetStreamProperties(zOrder, left, top, right, bottom);
        }
    }

    WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, -1,
                 "No VideoX11Channel object exists for stream id: %d",
                 streamId);
    return -1;
}

}  
