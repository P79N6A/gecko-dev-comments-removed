









#ifndef WEBRTC_MODULES_UTILITY_SOURCE_FILE_PLAYER_IMPL_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_FILE_PLAYER_IMPL_H_

#include "coder.h"
#include "common_types.h"
#include "critical_section_wrapper.h"
#include "engine_configurations.h"
#include "file_player.h"
#include "media_file_defines.h"
#include "media_file.h"
#include "resampler.h"
#include "tick_util.h"
#include "typedefs.h"

namespace webrtc {
class VideoCoder;
class FrameScaler;

class FilePlayerImpl : public FilePlayer
{
public:
    FilePlayerImpl(WebRtc_UWord32 instanceID, FileFormats fileFormat);
    ~FilePlayerImpl();

    virtual int Get10msAudioFromFile(
        int16_t* outBuffer,
        int& lengthInSamples,
        int frequencyInHz);
    virtual WebRtc_Word32 RegisterModuleFileCallback(FileCallback* callback);
    virtual WebRtc_Word32 StartPlayingFile(
        const char* fileName,
        bool loop,
        WebRtc_UWord32 startPosition,
        float volumeScaling,
        WebRtc_UWord32 notification,
        WebRtc_UWord32 stopPosition = 0,
        const CodecInst* codecInst = NULL);
    virtual WebRtc_Word32 StartPlayingFile(
        InStream& sourceStream,
        WebRtc_UWord32 startPosition,
        float volumeScaling,
        WebRtc_UWord32 notification,
        WebRtc_UWord32 stopPosition = 0,
        const CodecInst* codecInst = NULL);
    virtual WebRtc_Word32 StopPlayingFile();
    virtual bool IsPlayingFile() const;
    virtual WebRtc_Word32 GetPlayoutPosition(WebRtc_UWord32& durationMs);
    virtual WebRtc_Word32 AudioCodec(CodecInst& audioCodec) const;
    virtual WebRtc_Word32 Frequency() const;
    virtual WebRtc_Word32 SetAudioScaling(float scaleFactor);

protected:
    WebRtc_Word32 SetUpAudioDecoder();

    WebRtc_UWord32 _instanceID;
    const FileFormats _fileFormat;
    MediaFile& _fileModule;

    WebRtc_UWord32 _decodedLengthInMS;

private:
    AudioCoder _audioDecoder;

    CodecInst _codec;
    WebRtc_Word32 _numberOf10MsPerFrame;
    WebRtc_Word32 _numberOf10MsInDecoder;

    Resampler _resampler;
    float _scaling;
};

#ifdef WEBRTC_MODULE_UTILITY_VIDEO
class VideoFilePlayerImpl: public FilePlayerImpl
{
public:
    VideoFilePlayerImpl(WebRtc_UWord32 instanceID, FileFormats fileFormat);
    ~VideoFilePlayerImpl();

    
    virtual WebRtc_Word32 TimeUntilNextVideoFrame();
    virtual WebRtc_Word32 StartPlayingVideoFile(const char* fileName,
                                                bool loop,
                                                bool videoOnly);
    virtual WebRtc_Word32 StopPlayingFile();
    virtual WebRtc_Word32 video_codec_info(VideoCodec& videoCodec) const;
    virtual WebRtc_Word32 GetVideoFromFile(I420VideoFrame& videoFrame);
    virtual WebRtc_Word32 GetVideoFromFile(I420VideoFrame& videoFrame,
                                           const WebRtc_UWord32 outWidth,
                                           const WebRtc_UWord32 outHeight);

private:
    WebRtc_Word32 SetUpVideoDecoder();

    VideoCoder& _videoDecoder;
    VideoCodec video_codec_info_;
    WebRtc_Word32 _decodedVideoFrames;

    EncodedVideoData& _encodedData;

    FrameScaler& _frameScaler;
    CriticalSectionWrapper* _critSec;
    TickTime _startTime;
    WebRtc_Word64 _accumulatedRenderTimeMs;
    WebRtc_UWord32 _frameLengthMS;

    WebRtc_Word32 _numberOfFramesRead;
    bool _videoOnly;
};
#endif 

} 
#endif 
