









#ifndef WEBRTC_MODULES_MEDIA_FILE_INTERFACE_MEDIA_FILE_H_
#define WEBRTC_MODULES_MEDIA_FILE_INTERFACE_MEDIA_FILE_H_

#include "webrtc/common_types.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/media_file/interface/media_file_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class MediaFile : public Module
{
public:
    
    
    static MediaFile* CreateMediaFile(const int32_t id);
    static void DestroyMediaFile(MediaFile* module);

    
    virtual int32_t ChangeUniqueId(const int32_t id) = 0;

    
    
    
    
    
    
    
    
    virtual int32_t PlayoutAudioData(
        int8_t* audioBuffer,
        uint32_t& dataLengthInBytes) = 0;

    
    
    
    
    virtual int32_t PlayoutAVIVideoData(
        int8_t* videoBuffer,
        uint32_t& dataLengthInBytes) = 0;

    
    
    
    
    
    
    
    
    
    virtual int32_t PlayoutStereoData(
        int8_t* audioBufferLeft,
        int8_t* audioBufferRight,
        uint32_t& dataLengthInBytes) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual int32_t StartPlayingAudioFile(
        const char* fileName,
        const uint32_t notificationTimeMs = 0,
        const bool loop                         = false,
        const FileFormats format                = kFileFormatPcm16kHzFile,
        const CodecInst* codecInst              = NULL,
        const uint32_t startPointMs       = 0,
        const uint32_t stopPointMs        = 0) = 0;

    
    
    
    
    
    virtual int32_t StartPlayingVideoFile(const char* fileName,
                                                const bool loop,
                                                bool videoOnly,
                                                const FileFormats format) = 0;

    
    
    
    
    
    
    
    
    
    
    
    virtual int32_t StartPlayingAudioStream(
        InStream& stream,
        const uint32_t notificationTimeMs = 0,
        const FileFormats    format             = kFileFormatPcm16kHzFile,
        const CodecInst*     codecInst          = NULL,
        const uint32_t startPointMs       = 0,
        const uint32_t stopPointMs        = 0) = 0;

    
    virtual int32_t StopPlaying() = 0;

    
    virtual bool IsPlaying() = 0;


    
    virtual int32_t PlayoutPositionMs(
        uint32_t& durationMs) const = 0;

    
    
    
    
    virtual int32_t IncomingAudioData(
        const int8_t*  audioBuffer,
        const uint32_t bufferLength) = 0;

    
    
    
    
    
    
    virtual int32_t IncomingAVIVideoData(
        const int8_t*  videoBuffer,
        const uint32_t bufferLength) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual int32_t StartRecordingAudioFile(
        const char*  fileName,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const uint32_t notificationTimeMs = 0,
        const uint32_t maxSizeBytes       = 0) = 0;

    
    
    
    
    
    virtual int32_t StartRecordingVideoFile(
        const char* fileName,
        const FileFormats   format,
        const CodecInst&    codecInst,
        const VideoCodec&   videoCodecInst,
        bool videoOnly = false) = 0;

    
    
    
    
    
    
    
    
    virtual int32_t StartRecordingAudioStream(
        OutStream&           stream,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const uint32_t notificationTimeMs = 0) = 0;

    
    virtual int32_t StopRecording() = 0;

    
    virtual bool IsRecording() = 0;

    
    virtual int32_t RecordDurationMs(uint32_t& durationMs) = 0;

    
    virtual bool IsStereo() = 0;

    
    
    virtual int32_t SetModuleFileCallback(FileCallback* callback) = 0;

    
    
    
    virtual int32_t FileDurationMs(
        const char*  fileName,
        uint32_t&      durationMs,
        const FileFormats    format,
        const uint32_t freqInHz = 16000) = 0;

    
    
    virtual int32_t codec_info(CodecInst& codecInst) const = 0;

    
    
    virtual int32_t VideoCodecInst(VideoCodec& videoCodecInst) const = 0;

protected:
    MediaFile() {}
    virtual ~MediaFile() {}
};
}  
#endif 
