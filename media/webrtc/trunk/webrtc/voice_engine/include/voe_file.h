




































#ifndef WEBRTC_VOICE_ENGINE_VOE_FILE_H
#define WEBRTC_VOICE_ENGINE_VOE_FILE_H

#include "common_types.h"

namespace webrtc {

class VoiceEngine;

class WEBRTC_DLLEXPORT VoEFile
{
public:
    
    
    
    static VoEFile* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    
    virtual int StartPlayingFileLocally(
        int channel,
        const char fileNameUTF8[1024],
        bool loop = false,
        FileFormats format = kFileFormatPcm16kHzFile,
        float volumeScaling = 1.0,
        int startPointMs = 0,
        int stopPointMs = 0) = 0;

    
    
    virtual int StartPlayingFileLocally(
        int channel,
        InStream* stream,
        FileFormats format = kFileFormatPcm16kHzFile,
        float volumeScaling = 1.0,
        int startPointMs = 0, int stopPointMs = 0) = 0;

    
    virtual int StopPlayingFileLocally(int channel) = 0;

    
    virtual int IsPlayingFileLocally(int channel) = 0;

    
    virtual int ScaleLocalFilePlayout(int channel, float scale) = 0;

    
    
    virtual int StartPlayingFileAsMicrophone(
        int channel,
        const char fileNameUTF8[1024],
        bool loop = false ,
        bool mixWithMicrophone = false,
        FileFormats format = kFileFormatPcm16kHzFile,
        float volumeScaling = 1.0) = 0;

    
    
    virtual int StartPlayingFileAsMicrophone(
        int channel,
        InStream* stream,
        bool mixWithMicrophone = false,
        FileFormats format = kFileFormatPcm16kHzFile,
        float volumeScaling = 1.0) = 0;

    
    virtual int StopPlayingFileAsMicrophone(int channel) = 0;

    
    virtual int IsPlayingFileAsMicrophone(int channel) = 0;

    
    virtual int ScaleFileAsMicrophonePlayout(int channel, float scale) = 0;

    
    virtual int StartRecordingPlayout(int channel,
                                      const char* fileNameUTF8,
                                      CodecInst* compression = NULL,
                                      int maxSizeBytes = -1) = 0;

    
    virtual int StopRecordingPlayout(int channel) = 0;

    virtual int StartRecordingPlayout(int channel,
                                      OutStream* stream,
                                      CodecInst* compression = NULL) = 0;

    
    virtual int StartRecordingMicrophone(const char* fileNameUTF8,
                                         CodecInst* compression = NULL,
                                         int maxSizeBytes = -1) = 0;

    
    virtual int StartRecordingMicrophone(OutStream* stream,
                                         CodecInst* compression = NULL) = 0;

    
    virtual int StopRecordingMicrophone() = 0;


    
    virtual int GetFileDuration(const char* fileNameUTF8, int& durationMs,
        FileFormats format = kFileFormatPcm16kHzFile) = 0;

    
    virtual int GetPlaybackPosition(int channel, int& positionMs) = 0;

    virtual int ConvertPCMToWAV(const char* fileNameInUTF8,
                                const char* fileNameOutUTF8) = 0;

    virtual int ConvertPCMToWAV(InStream* streamIn,
                                OutStream* streamOut) = 0;

    virtual int ConvertWAVToPCM(const char* fileNameInUTF8,
                                const char* fileNameOutUTF8) = 0;

    virtual int ConvertWAVToPCM(InStream* streamIn,
                                OutStream* streamOut) = 0;

    virtual int ConvertPCMToCompressed(const char* fileNameInUTF8,
                                       const char* fileNameOutUTF8,
                                       CodecInst* compression) = 0;

    virtual int ConvertPCMToCompressed(InStream* streamIn,
                                       OutStream* streamOut,
                                       CodecInst* compression) = 0;

    virtual int ConvertCompressedToPCM(const char* fileNameInUTF8,
                                       const char* fileNameOutUTF8) = 0;

    virtual int ConvertCompressedToPCM(InStream* streamIn,
                                       OutStream* streamOut) = 0;

protected:
    VoEFile() {}
    virtual ~VoEFile() {}
};

}  

#endif  
