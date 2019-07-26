









#ifndef WEBRTC_VOICE_ENGINE_VOE_FILE_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_FILE_IMPL_H

#include "voe_file.h"
#include "shared_data.h"

namespace webrtc {

class VoEFileImpl : public VoEFile
{
public:
    

    virtual int StartPlayingFileLocally(
        int channel,
        const char fileNameUTF8[1024],
        bool loop = false,
        FileFormats format = kFileFormatPcm16kHzFile,
        float volumeScaling = 1.0,
        int startPointMs = 0,
        int stopPointMs = 0);

    virtual int StartPlayingFileLocally(
        int channel,
        InStream* stream,
        FileFormats format = kFileFormatPcm16kHzFile,
        float volumeScaling = 1.0,
        int startPointMs = 0, int stopPointMs = 0);

    virtual int StopPlayingFileLocally(int channel);

    virtual int IsPlayingFileLocally(int channel);

    virtual int ScaleLocalFilePlayout(int channel, float scale);

    

    virtual int StartPlayingFileAsMicrophone(
        int channel,
        const char fileNameUTF8[1024],
        bool loop = false ,
        bool mixWithMicrophone = false,
        FileFormats format = kFileFormatPcm16kHzFile,
        float volumeScaling = 1.0);

    virtual int StartPlayingFileAsMicrophone(
        int channel,
        InStream* stream,
        bool mixWithMicrophone = false,
        FileFormats format = kFileFormatPcm16kHzFile,
        float volumeScaling = 1.0);

    virtual int StopPlayingFileAsMicrophone(int channel);

    virtual int IsPlayingFileAsMicrophone(int channel);

    virtual int ScaleFileAsMicrophonePlayout(int channel, float scale);

    

    virtual int StartRecordingPlayout(int channel,
                                      const char* fileNameUTF8,
                                      CodecInst* compression = NULL,
                                      int maxSizeBytes = -1);

    virtual int StartRecordingPlayout(int channel,
                                      OutStream* stream,
                                      CodecInst* compression = NULL);

    virtual int StopRecordingPlayout(int channel);

    

    virtual int StartRecordingMicrophone(const char* fileNameUTF8,
                                         CodecInst* compression = NULL,
                                         int maxSizeBytes = -1);

    virtual int StartRecordingMicrophone(OutStream* stream,
                                         CodecInst* compression = NULL);

    virtual int StopRecordingMicrophone();

    

    virtual int ConvertPCMToWAV(const char* fileNameInUTF8,
                                const char* fileNameOutUTF8);

    virtual int ConvertPCMToWAV(InStream* streamIn,
                                OutStream* streamOut);

    virtual int ConvertWAVToPCM(const char* fileNameInUTF8,
                                const char* fileNameOutUTF8);

    virtual int ConvertWAVToPCM(InStream* streamIn,
                                OutStream* streamOut);

    virtual int ConvertPCMToCompressed(const char* fileNameInUTF8,
                                       const char* fileNameOutUTF8,
                                       CodecInst* compression);

    virtual int ConvertPCMToCompressed(InStream* streamIn,
                                       OutStream* streamOut,
                                       CodecInst* compression);

    virtual int ConvertCompressedToPCM(const char* fileNameInUTF8,
                                       const char* fileNameOutUTF8);

    virtual int ConvertCompressedToPCM(InStream* streamIn,
                                       OutStream* streamOut);

    

    virtual int GetFileDuration(
        const char* fileNameUTF8,
        int& durationMs,
        FileFormats format = kFileFormatPcm16kHzFile);

    virtual int GetPlaybackPosition(int channel, int& positionMs);

protected:
    VoEFileImpl(voe::SharedData* shared);
    virtual ~VoEFileImpl();

private:
    voe::SharedData* _shared;
};

}  

#endif  

