









#ifndef WEBRTC_MODULES_MEDIA_FILE_SOURCE_MEDIA_FILE_IMPL_H_
#define WEBRTC_MODULES_MEDIA_FILE_SOURCE_MEDIA_FILE_IMPL_H_

#include "webrtc/common_types.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/media_file/interface/media_file.h"
#include "webrtc/modules/media_file/interface/media_file_defines.h"
#include "webrtc/modules/media_file/source/media_file_utility.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

namespace webrtc {
class MediaFileImpl : public MediaFile
{

public:
    MediaFileImpl(const int32_t id);
    ~MediaFileImpl();

    int32_t ChangeUniqueId(const int32_t id);
    int32_t Process();
    int32_t TimeUntilNextProcess();

    
    int32_t PlayoutAudioData(int8_t* audioBuffer, uint32_t& dataLengthInBytes);
    int32_t PlayoutAVIVideoData(int8_t* videoBuffer,
                                uint32_t& dataLengthInBytes);
    int32_t PlayoutStereoData(int8_t* audioBufferLeft, int8_t* audioBufferRight,
                              uint32_t& dataLengthInBytes);
    virtual int32_t StartPlayingAudioFile(
        const char*  fileName,
        const uint32_t notificationTimeMs = 0,
        const bool           loop = false,
        const FileFormats    format = kFileFormatPcm16kHzFile,
        const CodecInst*     codecInst = NULL,
        const uint32_t startPointMs = 0,
        const uint32_t stopPointMs = 0);
    int32_t StartPlayingVideoFile(const char* fileName, const bool loop,
                                  bool videoOnly, const FileFormats format);
    int32_t StartPlayingAudioStream(InStream& stream,
        const uint32_t notificationTimeMs = 0,
        const FileFormats format = kFileFormatPcm16kHzFile,
        const CodecInst* codecInst = NULL,
        const uint32_t startPointMs = 0,
        const uint32_t stopPointMs = 0);
    int32_t StopPlaying();
    bool IsPlaying();
    int32_t PlayoutPositionMs(uint32_t& positionMs) const;
    int32_t IncomingAudioData(const int8_t*  audioBuffer,
                              const uint32_t bufferLength);
    int32_t IncomingAVIVideoData(const int8_t*  audioBuffer,
                                 const uint32_t bufferLength);
    int32_t StartRecordingAudioFile(
        const char*  fileName,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const uint32_t notificationTimeMs = 0,
        const uint32_t maxSizeBytes = 0);
    int32_t StartRecordingVideoFile(
        const char* fileName,
        const FileFormats   format,
        const CodecInst&    codecInst,
        const VideoCodec&   videoCodecInst,
        bool                videoOnly = false);
    int32_t StartRecordingAudioStream(
        OutStream&           stream,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const uint32_t notificationTimeMs = 0);
    int32_t StopRecording();
    bool IsRecording();
    int32_t RecordDurationMs(uint32_t& durationMs);
    bool IsStereo();
    int32_t SetModuleFileCallback(FileCallback* callback);
    int32_t FileDurationMs(
        const char*  fileName,
        uint32_t&      durationMs,
        const FileFormats    format,
        const uint32_t freqInHz = 16000);
    int32_t codec_info(CodecInst& codecInst) const;
    int32_t VideoCodecInst(VideoCodec& codecInst) const;

private:
    
    static bool ValidFileFormat(const FileFormats format,
                                const CodecInst*  codecInst);


    
    static bool ValidFileName(const char* fileName);

  
    static bool ValidFilePositions(const uint32_t startPointMs,
                                   const uint32_t stopPointMs);

    
    
    
    
    
    
    
    
    
    
    
    
    int32_t StartPlayingFile(
        const char*  fileName,
        const uint32_t notificationTimeMs = 0,
        const bool           loop               = false,
        bool                 videoOnly          = false,
        const FileFormats    format             = kFileFormatPcm16kHzFile,
        const CodecInst*     codecInst          = NULL,
        const uint32_t startPointMs       = 0,
        const uint32_t stopPointMs        = 0);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t StartPlayingStream(
        InStream&            stream,
        const char*          fileName,
        bool                 loop,
        const uint32_t notificationTimeMs = 0,
        const FileFormats    format             = kFileFormatPcm16kHzFile,
        const CodecInst*     codecInst          = NULL,
        const uint32_t startPointMs       = 0,
        const uint32_t stopPointMs        = 0,
        bool                 videoOnly          = true);

    
    
    
    
    
    int32_t PlayoutData(int8_t* dataBuffer, uint32_t& dataLengthInBytes,
                        bool video);

    
    
    
    int32_t IncomingAudioVideoData(const int8_t*  buffer,
                                   const uint32_t bufferLength,
                                   const bool video);

    
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t StartRecordingFile(
        const char*  fileName,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const VideoCodec&    videoCodecInst,
        const uint32_t notificationTimeMs = 0,
        const uint32_t maxSizeBytes = 0,
        bool                 videoOnly = false);

    
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t StartRecordingStream(
        OutStream&           stream,
        const char*  fileName,
        const FileFormats    format,
        const CodecInst&     codecInst,
        const VideoCodec&    videoCodecInst,
        const uint32_t notificationTimeMs = 0,
        const bool           videoOnly = false);

    
    static bool ValidFrequency(const uint32_t frequencyInHz);

    void HandlePlayCallbacks(int32_t bytesRead);

    int32_t _id;
    CriticalSectionWrapper* _crit;
    CriticalSectionWrapper* _callbackCrit;

    ModuleFileUtility* _ptrFileUtilityObj;
    CodecInst codec_info_;

    InStream*  _ptrInStream;
    OutStream* _ptrOutStream;

    FileFormats _fileFormat;
    uint32_t _recordDurationMs;
    uint32_t _playoutPositionMs;
    uint32_t _notificationMs;

    bool _playingActive;
    bool _recordingActive;
    bool _isStereo;
    bool _openFile;

    char _fileName[512];

    FileCallback* _ptrCallback;
};
}  

#endif 
