









#ifndef WEBRTC_MODULES_UTILITY_SOURCE_FILE_PLAYER_IMPL_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_FILE_PLAYER_IMPL_H_

#include "webrtc/common_audio/resampler/include/resampler.h"
#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/media_file/interface/media_file.h"
#include "webrtc/modules/media_file/interface/media_file_defines.h"
#include "webrtc/modules/utility/interface/file_player.h"
#include "webrtc/modules/utility/source/coder.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class VideoCoder;
class FrameScaler;

class FilePlayerImpl : public FilePlayer
{
public:
    FilePlayerImpl(uint32_t instanceID, FileFormats fileFormat);
    ~FilePlayerImpl();

    virtual int Get10msAudioFromFile(
        int16_t* outBuffer,
        int& lengthInSamples,
        int frequencyInHz);
    virtual int32_t RegisterModuleFileCallback(FileCallback* callback);
    virtual int32_t StartPlayingFile(
        const char* fileName,
        bool loop,
        uint32_t startPosition,
        float volumeScaling,
        uint32_t notification,
        uint32_t stopPosition = 0,
        const CodecInst* codecInst = NULL);
    virtual int32_t StartPlayingFile(
        InStream& sourceStream,
        uint32_t startPosition,
        float volumeScaling,
        uint32_t notification,
        uint32_t stopPosition = 0,
        const CodecInst* codecInst = NULL);
    virtual int32_t StopPlayingFile();
    virtual bool IsPlayingFile() const;
    virtual int32_t GetPlayoutPosition(uint32_t& durationMs);
    virtual int32_t AudioCodec(CodecInst& audioCodec) const;
    virtual int32_t Frequency() const;
    virtual int32_t SetAudioScaling(float scaleFactor);

protected:
    int32_t SetUpAudioDecoder();

    uint32_t _instanceID;
    const FileFormats _fileFormat;
    MediaFile& _fileModule;

    uint32_t _decodedLengthInMS;

private:
    AudioCoder _audioDecoder;

    CodecInst _codec;
    int32_t _numberOf10MsPerFrame;
    int32_t _numberOf10MsInDecoder;

    Resampler _resampler;
    float _scaling;
};

#ifdef WEBRTC_MODULE_UTILITY_VIDEO
class VideoFilePlayerImpl: public FilePlayerImpl
{
public:
    VideoFilePlayerImpl(uint32_t instanceID, FileFormats fileFormat);
    ~VideoFilePlayerImpl();

    
    virtual int32_t TimeUntilNextVideoFrame();
    virtual int32_t StartPlayingVideoFile(const char* fileName,
                                          bool loop,
                                          bool videoOnly);
    virtual int32_t StopPlayingFile();
    virtual int32_t video_codec_info(VideoCodec& videoCodec) const;
    virtual int32_t GetVideoFromFile(I420VideoFrame& videoFrame);
    virtual int32_t GetVideoFromFile(I420VideoFrame& videoFrame,
                                     const uint32_t outWidth,
                                     const uint32_t outHeight);

private:
    int32_t SetUpVideoDecoder();

    scoped_ptr<VideoCoder> video_decoder_;
    VideoCodec video_codec_info_;
    int32_t _decodedVideoFrames;

    EncodedVideoData& _encodedData;

    FrameScaler& _frameScaler;
    CriticalSectionWrapper* _critSec;
    TickTime _startTime;
    int64_t _accumulatedRenderTimeMs;
    uint32_t _frameLengthMS;

    int32_t _numberOfFramesRead;
    bool _videoOnly;
};
#endif 

}  
#endif 
