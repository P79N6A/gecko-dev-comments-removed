









#include "video_coding.h"
#include "trace.h"
#include "generic_decoder.h"
#include "internal_defines.h"
#include "tick_time_base.h"

namespace webrtc {

VCMDecodedFrameCallback::VCMDecodedFrameCallback(VCMTiming& timing,
                                                 TickTimeBase* clock)
:
_critSect(CriticalSectionWrapper::CreateCriticalSection()),
_clock(clock),
_receiveCallback(NULL),
_timing(timing),
_timestampMap(kDecoderFrameMemoryLength),
_lastReceivedPictureID(0)
{
}

VCMDecodedFrameCallback::~VCMDecodedFrameCallback()
{
    delete _critSect;
}

void VCMDecodedFrameCallback::SetUserReceiveCallback(
    VCMReceiveCallback* receiveCallback)
{
    CriticalSectionScoped cs(_critSect);
    _receiveCallback = receiveCallback;
}

WebRtc_Word32 VCMDecodedFrameCallback::Decoded(I420VideoFrame& decodedImage)
{
    
    
    CriticalSectionScoped cs(_critSect);
    VCMFrameInformation* frameInfo = static_cast<VCMFrameInformation*>(
        _timestampMap.Pop(decodedImage.timestamp()));
    if (frameInfo == NULL)
    {
        
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    _timing.StopDecodeTimer(
        decodedImage.timestamp(),
        frameInfo->decodeStartTimeMs,
        _clock->MillisecondTimestamp());

    if (_receiveCallback != NULL)
    {
        _frame.SwapFrame(&decodedImage);
        _frame.set_render_time_ms(frameInfo->renderTimeMs);
        WebRtc_Word32 callbackReturn = _receiveCallback->FrameToRender(_frame);
        if (callbackReturn < 0)
        {
            WEBRTC_TRACE(webrtc::kTraceDebug,
                         webrtc::kTraceVideoCoding,
                         -1,
                         "Render callback returned error: %d", callbackReturn);
        }
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32
VCMDecodedFrameCallback::ReceivedDecodedReferenceFrame(
    const WebRtc_UWord64 pictureId)
{
    CriticalSectionScoped cs(_critSect);
    if (_receiveCallback != NULL)
    {
        return _receiveCallback->ReceivedDecodedReferenceFrame(pictureId);
    }
    return -1;
}

WebRtc_Word32
VCMDecodedFrameCallback::ReceivedDecodedFrame(const WebRtc_UWord64 pictureId)
{
    _lastReceivedPictureID = pictureId;
    return 0;
}

WebRtc_UWord64 VCMDecodedFrameCallback::LastReceivedPictureID() const
{
    return _lastReceivedPictureID;
}

WebRtc_Word32 VCMDecodedFrameCallback::Map(WebRtc_UWord32 timestamp, VCMFrameInformation* frameInfo)
{
    CriticalSectionScoped cs(_critSect);
    return _timestampMap.Add(timestamp, frameInfo);
}

WebRtc_Word32 VCMDecodedFrameCallback::Pop(WebRtc_UWord32 timestamp)
{
    CriticalSectionScoped cs(_critSect);
    if (_timestampMap.Pop(timestamp) == NULL)
    {
        return VCM_GENERAL_ERROR;
    }
    return VCM_OK;
}

VCMGenericDecoder::VCMGenericDecoder(VideoDecoder& decoder, WebRtc_Word32 id, bool isExternal)
:
_id(id),
_callback(NULL),
_frameInfos(),
_nextFrameInfoIdx(0),
_decoder(decoder),
_codecType(kVideoCodecUnknown),
_isExternal(isExternal),
_requireKeyFrame(false),
_keyFrameDecoded(false)
{
}

VCMGenericDecoder::~VCMGenericDecoder()
{
}

WebRtc_Word32 VCMGenericDecoder::InitDecode(const VideoCodec* settings,
                                            WebRtc_Word32 numberOfCores,
                                            bool requireKeyFrame)
{
    _requireKeyFrame = requireKeyFrame;
    _keyFrameDecoded = false;
    _codecType = settings->codecType;

    return _decoder.InitDecode(settings, numberOfCores);
}

WebRtc_Word32 VCMGenericDecoder::Decode(const VCMEncodedFrame& frame,
                                        int64_t nowMs)
{
    if (_requireKeyFrame &&
        !_keyFrameDecoded &&
        frame.FrameType() != kVideoFrameKey &&
        frame.FrameType() != kVideoFrameGolden)
    {
        
        
        return VCM_CODEC_ERROR;
    }
    _frameInfos[_nextFrameInfoIdx].decodeStartTimeMs = nowMs;
    _frameInfos[_nextFrameInfoIdx].renderTimeMs = frame.RenderTimeMs();
    _callback->Map(frame.TimeStamp(), &_frameInfos[_nextFrameInfoIdx]);

    WEBRTC_TRACE(webrtc::kTraceDebug,
                 webrtc::kTraceVideoCoding,
                 VCMId(_id),
                 "Decoding timestamp %u", frame.TimeStamp());

    _nextFrameInfoIdx = (_nextFrameInfoIdx + 1) % kDecoderFrameMemoryLength;

    WebRtc_Word32 ret = _decoder.Decode(frame.EncodedImage(),
                                        frame.MissingFrame(),
                                        frame.FragmentationHeader(),
                                        frame.CodecSpecific(),
                                        frame.RenderTimeMs());

    if (ret < WEBRTC_VIDEO_CODEC_OK)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, VCMId(_id), "Decoder error: %d\n", ret);
        _callback->Pop(frame.TimeStamp());
        return ret;
    }
    else if (ret == WEBRTC_VIDEO_CODEC_NO_OUTPUT ||
             ret == WEBRTC_VIDEO_CODEC_REQUEST_SLI)
    {
        
        _callback->Pop(frame.TimeStamp());
    }
    
    _keyFrameDecoded = (_keyFrameDecoded ||
        frame.FrameType() == kVideoFrameKey);
    return ret;
}

WebRtc_Word32
VCMGenericDecoder::Release()
{
    _keyFrameDecoded = false;
    return _decoder.Release();
}

WebRtc_Word32 VCMGenericDecoder::Reset()
{
    _keyFrameDecoded = false;
    return _decoder.Reset();
}

WebRtc_Word32 VCMGenericDecoder::SetCodecConfigParameters(const WebRtc_UWord8* buffer, WebRtc_Word32 size)
{
    return _decoder.SetCodecConfigParameters(buffer, size);
}

WebRtc_Word32 VCMGenericDecoder::RegisterDecodeCompleteCallback(VCMDecodedFrameCallback* callback)
{
    _callback = callback;
    return _decoder.RegisterDecodeCompleteCallback(callback);
}

bool VCMGenericDecoder::External() const
{
    return _isExternal;
}

} 
