









#ifndef WEBRTC_MODULES_MEDIA_FILE_INTERFACE_MEDIA_FILE_H_
#define WEBRTC_MODULES_MEDIA_FILE_INTERFACE_MEDIA_FILE_H_

#include "common_types.h"
#include "typedefs.h"
#include "module.h"
#include "module_common_types.h"
#include "media_file_defines.h"

namespace webrtc {
class MediaFile : public Module
{
public:
    
    
    static MediaFile* CreateMediaFile(const WebRtc_Word32 id);
    static void DestroyMediaFile(MediaFile* module);

    
    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id) = 0;

    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 PlayoutAudioData(
        WebRtc_Word8* audioBuffer,
        WebRtc_UWord32& dataLengthInBytes) = 0;

    
    
    
    
    virtual WebRtc_Word32 PlayoutAVIVideoData(
        WebRtc_Word8* videoBuffer,
        WebRtc_UWord32& dataLengthInBytes) = 0;

    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 PlayoutStereoData(
        WebRtc_Word8* audioBufferLeft,
        WebRtc_Word8* audioBufferRight,
        WebRtc_UWord32& dataLengthInBytes) = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 StartPlayingAudioFile(
        const char* fileName,
        const WebRtc_UWord32 notificationTimeMs = 0,
        const bool loop                         = false,
        const FileFormats format                = kFileFormatPcm16kHzFile,
        const CodecInst* codecInst              = NULL,
        const WebRtc_UWord32 startPointMs       = 0,
        const WebRtc_UWord32 stopPointMs        = 0) = 0;

    
    
    
    
    
    virtual WebRtc_Word32 StartPlayingVideoFile(const char* fileName,
                                                const bool loop,
                                                bool videoOnly,
                                                const FileFormats format) = 0;

    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 StartPlayingAudioStream(
        InStream& stream,
        const WebRtc_UWord32 notificationTimeMs = 0,
        const FileFormats    format             = kFileFormatPcm16kHzFile,
        const CodecInst*     codecInst          = NULL,
        const WebRtc_UWord32 startPointMs       = 0,
        const WebRtc_UWord32 stopPointMs        = 0) = 0;

    
    virtual WebRtc_Word32 StopPlaying() = 0;

    
    virtual bool IsPlaying() = 0;


    
    virtual WebRtc_Word32 PlayoutPositionMs(
        WebRtc_UWord32& durationMs) const = 0;

    
    
    
    
    virtual WebRtc_Word32 IncomingAudioData(
        const WebRtc_Word8*  audioBuffer,
        const WebRtc_UWord32 bufferLength) = 0;

    
    
    
    
    
    
    virtual WebRtc_Word32 IncomingAVIVideoData(
        const WebRtc_Word8*  videoBuffer,
        const WebRtc_UWord32 bufferLength) = 0;

    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 StartRecordingAudioFile(
        const char*  fileName,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const WebRtc_UWord32 notificationTimeMs = 0,
        const WebRtc_UWord32 maxSizeBytes       = 0) = 0;

    
    
    
    
    
    virtual WebRtc_Word32 StartRecordingVideoFile(
        const char* fileName,
        const FileFormats   format,
        const CodecInst&    codecInst,
        const VideoCodec&   videoCodecInst,
        bool videoOnly = false) = 0;

    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 StartRecordingAudioStream(
        OutStream&           stream,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const WebRtc_UWord32 notificationTimeMs = 0) = 0;

    
    virtual WebRtc_Word32 StopRecording() = 0;

    
    virtual bool IsRecording() = 0;

    
    virtual WebRtc_Word32 RecordDurationMs(WebRtc_UWord32& durationMs) = 0;

    
    virtual bool IsStereo() = 0;

    
    
    virtual WebRtc_Word32 SetModuleFileCallback(FileCallback* callback) = 0;

    
    
    
    virtual WebRtc_Word32 FileDurationMs(
        const char*  fileName,
        WebRtc_UWord32&      durationMs,
        const FileFormats    format,
        const WebRtc_UWord32 freqInHz = 16000) = 0;

    
    
    virtual WebRtc_Word32 codec_info(CodecInst& codecInst) const = 0;

    
    
    virtual WebRtc_Word32 VideoCodecInst(VideoCodec& videoCodecInst) const = 0;

protected:
    MediaFile() {}
    virtual ~MediaFile() {}
};
} 
#endif 
