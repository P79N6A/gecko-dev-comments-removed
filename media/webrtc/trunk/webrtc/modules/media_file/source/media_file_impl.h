









#ifndef WEBRTC_MODULES_MEDIA_FILE_SOURCE_MEDIA_FILE_IMPL_H_
#define WEBRTC_MODULES_MEDIA_FILE_SOURCE_MEDIA_FILE_IMPL_H_

#include "common_types.h"
#include "media_file.h"
#include "media_file_defines.h"
#include "media_file_utility.h"
#include "module_common_types.h"

namespace webrtc {
class MediaFileImpl : public MediaFile
{

public:
    MediaFileImpl(const WebRtc_Word32 id);
    ~MediaFileImpl();

    WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);
    WebRtc_Word32 Process();
    WebRtc_Word32 TimeUntilNextProcess();

    
    WebRtc_Word32 PlayoutAudioData(WebRtc_Word8*   audioBuffer,
                                   WebRtc_UWord32& dataLengthInBytes);
    WebRtc_Word32 PlayoutAVIVideoData(WebRtc_Word8* videoBuffer,
                                      WebRtc_UWord32& dataLengthInBytes);
    WebRtc_Word32 PlayoutStereoData(WebRtc_Word8* audioBufferLeft,
                                    WebRtc_Word8* audioBufferRight,
                                    WebRtc_UWord32& dataLengthInBytes);
    virtual WebRtc_Word32 StartPlayingAudioFile(
        const char*  fileName,
        const WebRtc_UWord32 notificationTimeMs = 0,
        const bool           loop = false,
        const FileFormats    format = kFileFormatPcm16kHzFile,
        const CodecInst*     codecInst = NULL,
        const WebRtc_UWord32 startPointMs = 0,
        const WebRtc_UWord32 stopPointMs = 0);
    WebRtc_Word32 StartPlayingVideoFile(const char* fileName,
                                        const bool          loop,
                                        bool                videoOnly,
                                        const FileFormats   format);
    WebRtc_Word32 StartPlayingAudioStream(
        InStream&            stream,
        const WebRtc_UWord32 notificationTimeMs = 0,
        const FileFormats    format = kFileFormatPcm16kHzFile,
        const CodecInst*     codecInst = NULL,
        const WebRtc_UWord32 startPointMs = 0,
        const WebRtc_UWord32 stopPointMs = 0);
    WebRtc_Word32 StopPlaying();
    bool IsPlaying();
    WebRtc_Word32 PlayoutPositionMs(WebRtc_UWord32& positionMs) const;
    WebRtc_Word32 IncomingAudioData(const WebRtc_Word8*  audioBuffer,
                                    const WebRtc_UWord32 bufferLength);
    WebRtc_Word32 IncomingAVIVideoData(const WebRtc_Word8*  audioBuffer,
                                       const WebRtc_UWord32 bufferLength);
    WebRtc_Word32 StartRecordingAudioFile(
        const char*  fileName,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const WebRtc_UWord32 notificationTimeMs = 0,
        const WebRtc_UWord32 maxSizeBytes = 0);
    WebRtc_Word32 StartRecordingVideoFile(
        const char* fileName,
        const FileFormats   format,
        const CodecInst&    codecInst,
        const VideoCodec&   videoCodecInst,
        bool                videoOnly = false);
    WebRtc_Word32 StartRecordingAudioStream(
        OutStream&           stream,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const WebRtc_UWord32 notificationTimeMs = 0);
    WebRtc_Word32 StopRecording();
    bool IsRecording();
    WebRtc_Word32 RecordDurationMs(WebRtc_UWord32& durationMs);
    bool IsStereo();
    WebRtc_Word32 SetModuleFileCallback(FileCallback* callback);
    WebRtc_Word32 FileDurationMs(
        const char*  fileName,
        WebRtc_UWord32&      durationMs,
        const FileFormats    format,
        const WebRtc_UWord32 freqInHz = 16000);
    WebRtc_Word32 codec_info(CodecInst& codecInst) const;
    WebRtc_Word32 VideoCodecInst(VideoCodec& codecInst) const;

private:
    
    static bool ValidFileFormat(const FileFormats format,
                                const CodecInst*  codecInst);


    
    static bool ValidFileName(const char* fileName);

  
    static bool ValidFilePositions(const WebRtc_UWord32 startPointMs,
                                   const WebRtc_UWord32 stopPointMs);

    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 StartPlayingFile(
        const char*  fileName,
        const WebRtc_UWord32 notificationTimeMs = 0,
        const bool           loop               = false,
        bool                 videoOnly          = false,
        const FileFormats    format             = kFileFormatPcm16kHzFile,
        const CodecInst*     codecInst          = NULL,
        const WebRtc_UWord32 startPointMs       = 0,
        const WebRtc_UWord32 stopPointMs        = 0);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 StartPlayingStream(
        InStream&            stream,
        const char*          fileName,
        bool                 loop,
        const WebRtc_UWord32 notificationTimeMs = 0,
        const FileFormats    format             = kFileFormatPcm16kHzFile,
        const CodecInst*     codecInst          = NULL,
        const WebRtc_UWord32 startPointMs       = 0,
        const WebRtc_UWord32 stopPointMs        = 0,
        bool                 videoOnly          = true);

    
    
    
    
    
    WebRtc_Word32 PlayoutData(WebRtc_Word8* dataBuffer,
                              WebRtc_UWord32& dataLengthInBytes, bool video);

    
    
    
    WebRtc_Word32 IncomingAudioVideoData(const WebRtc_Word8*  buffer,
                                         const WebRtc_UWord32 bufferLength,
                                         const bool video);

    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 StartRecordingFile(
        const char*  fileName,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const VideoCodec&    videoCodecInst,
        const WebRtc_UWord32 notificationTimeMs = 0,
        const WebRtc_UWord32 maxSizeBytes = 0,
        bool                 videoOnly = false);

    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 StartRecordingStream(
        OutStream&           stream,
        const char*  fileName,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const VideoCodec&    videoCodecInst,
        const WebRtc_UWord32 notificationTimeMs = 0,
        const bool           videoOnly = false);

    
    static bool ValidFrequency(const WebRtc_UWord32 frequencyInHz);

    void HandlePlayCallbacks(WebRtc_Word32 bytesRead);

    WebRtc_Word32 _id;
    CriticalSectionWrapper* _crit;
    CriticalSectionWrapper* _callbackCrit;

    ModuleFileUtility* _ptrFileUtilityObj;
    CodecInst codec_info_;

    InStream*  _ptrInStream;
    OutStream* _ptrOutStream;

    FileFormats _fileFormat;
    WebRtc_UWord32 _recordDurationMs;
    WebRtc_UWord32 _playoutPositionMs;
    WebRtc_UWord32 _notificationMs;

    bool _playingActive;
    bool _recordingActive;
    bool _isStereo;
    bool _openFile;

    char _fileName[512];

    FileCallback* _ptrCallback;
};
} 

#endif 
