










#ifndef WEBRTC_MODULES_MEDIA_FILE_SOURCE_MEDIA_FILE_UTILITY_H_
#define WEBRTC_MODULES_MEDIA_FILE_SOURCE_MEDIA_FILE_UTILITY_H_

#include <stdio.h>

#include "webrtc/common_types.h"
#include "webrtc/modules/media_file/interface/media_file_defines.h"

namespace webrtc {
class AviFile;
class InStream;
class OutStream;

class ModuleFileUtility
{
public:

    ModuleFileUtility(const int32_t id);
    ~ModuleFileUtility();

#ifdef WEBRTC_MODULE_UTILITY_VIDEO
    
    
    
    
    int32_t InitAviReading(const char* fileName, bool videoOnly, bool loop);

    
    
    
    
    
    
    int32_t ReadAviAudioData(int8_t* outBuffer,
                             const uint32_t bufferLengthInBytes);

    
    
    
    int32_t ReadAviVideoData(int8_t* videoBuffer,
                             const uint32_t bufferLengthInBytes);

    
    
    
    
    int32_t InitAviWriting(const char* filename,
                           const CodecInst& codecInst,
                           const VideoCodec& videoCodecInst,
                           const bool videoOnly);

    
    
    
    
    
    int32_t WriteAviAudioData(const int8_t* audioBuffer,
                              uint32_t bufferLengthInBytes);


    
    
    
    
    
    
    int32_t WriteAviVideoData(const int8_t* videoBuffer,
                              uint32_t bufferLengthInBytes);

    
    int32_t CloseAviFile();

    int32_t VideoCodecInst(VideoCodec& codecInst);
#endif 

    
    
    
    int32_t InitWavReading(InStream& stream,
                           const uint32_t startPointMs = 0,
                           const uint32_t stopPointMs = 0);

    
    
    
    
    
    
    int32_t ReadWavDataAsMono(InStream& stream, int8_t* audioBuffer,
                              const uint32_t dataLengthInBytes);

    
    
    
    
    
    
    
    
    int32_t ReadWavDataAsStereo(InStream& wav,
                                int8_t* audioBufferLeft,
                                int8_t* audioBufferRight,
                                const uint32_t bufferLength);

    
    
    
    
    int32_t InitWavWriting(OutStream& stream, const CodecInst& codecInst);

    
    
    
    
    int32_t WriteWavData(OutStream& stream,
                         const int8_t* audioBuffer,
                         const uint32_t bufferLength);

    
    
    
    
    
    int32_t UpdateWavHeader(OutStream& stream);

    
    
    
    
    
    int32_t InitPCMReading(InStream& stream,
                           const uint32_t startPointMs = 0,
                           const uint32_t stopPointMs = 0,
                           const uint32_t freqInHz = 16000);

    
    
    
    int32_t ReadPCMData(InStream& stream, int8_t* audioBuffer,
                        const uint32_t dataLengthInBytes);

    
    
    
    int32_t InitPCMWriting(OutStream& stream, const uint32_t freqInHz = 16000);

    
    
    
    
    int32_t WritePCMData(OutStream& stream,
                         const int8_t* audioBuffer,
                         uint32_t bufferLength);

    
    
    
    int32_t InitCompressedReading(InStream& stream,
                                  const uint32_t startPointMs = 0,
                                  const uint32_t stopPointMs = 0);

    
    
    
    int32_t ReadCompressedData(InStream& stream,
                               int8_t* audioBuffer,
                               const uint32_t dataLengthInBytes);

    
    
    int32_t InitCompressedWriting(OutStream& stream,
                                  const CodecInst& codecInst);

    
    
    
    
    
    int32_t WriteCompressedData(OutStream& stream,
                                const int8_t* audioBuffer,
                                const uint32_t bufferLength);

    
    
    int32_t InitPreEncodedReading(InStream& stream,
                                  const CodecInst& codecInst);

    
    
    
    int32_t ReadPreEncodedData(InStream& stream,
                               int8_t* audioBuffer,
                               const uint32_t dataLengthInBytes);

    
    
    int32_t InitPreEncodedWriting(OutStream& stream,
                                  const CodecInst& codecInst);

    
    
    
   
    
    int32_t WritePreEncodedData(OutStream& stream,
                                const int8_t* inData,
                                const uint32_t dataLengthInBytes);

    
    
    int32_t FileDurationMs(const char* fileName,
                           const FileFormats fileFormat,
                           const uint32_t freqInHz = 16000);

    
    uint32_t PlayoutPositionMs();

    
    
    int32_t codec_info(CodecInst& codecInst);

private:
    
    enum{WAV_MAX_BUFFER_SIZE = 480*2*2};


    int32_t InitWavCodec(uint32_t samplesPerSec,
                         uint32_t channels,
                         uint32_t bitsPerSample,
                         uint32_t formatTag);

    
    int32_t ReadWavHeader(InStream& stream);

    
    
    
    
    
    
    int32_t WriteWavHeader(OutStream& stream,
                           const uint32_t freqInHz,
                           const uint32_t bytesPerSample,
                           const uint32_t channels,
                           const uint32_t format,
                           const uint32_t lengthInBytes);

    
    
    int32_t ReadWavData(InStream& stream, uint8_t* audioBuffer,
                        const uint32_t dataLengthInBytes);

    
    
    int32_t set_codec_info(const CodecInst& codecInst);

    struct WAVE_FMTINFO_header
    {
        int16_t formatTag;
        int16_t nChannels;
        int32_t nSamplesPerSec;
        int32_t nAvgBytesPerSec;
        int16_t nBlockAlign;
        int16_t nBitsPerSample;
    };
    
    enum MediaFileUtility_CodecType
    {
        kCodecNoCodec  = 0,
        kCodecIsac,
        kCodecIsacSwb,
        kCodecIsacLc,
        kCodecL16_8Khz,
        kCodecL16_16kHz,
        kCodecL16_32Khz,
        kCodecPcmu,
        kCodecPcma,
        kCodecIlbc20Ms,
        kCodecIlbc30Ms,
        kCodecG722,
        kCodecG722_1_32Kbps,
        kCodecG722_1_24Kbps,
        kCodecG722_1_16Kbps,
        kCodecG722_1c_48,
        kCodecG722_1c_32,
        kCodecG722_1c_24,
        kCodecAmr,
        kCodecAmrWb,
        kCodecG729,
        kCodecG729_1,
        kCodecG726_40,
        kCodecG726_32,
        kCodecG726_24,
        kCodecG726_16,
        kCodecSpeex8Khz,
        kCodecSpeex16Khz
    };

    
    
    WAVE_FMTINFO_header _wavFormatObj;
    int32_t _dataSize;      
    
    
    int32_t _readSizeBytes;

    int32_t _id;

    uint32_t _stopPointInMs;
    uint32_t _startPointInMs;
    uint32_t _playoutPositionMs;
    uint32_t _bytesWritten;

    CodecInst codec_info_;
    MediaFileUtility_CodecType _codecId;

    
    int32_t  _bytesPerSample;
    int32_t  _readPos;

    
    bool _reading;
    bool _writing;

    
    uint8_t _tempData[WAV_MAX_BUFFER_SIZE];

#ifdef WEBRTC_MODULE_UTILITY_VIDEO
    AviFile* _aviAudioInFile;
    AviFile* _aviVideoInFile;
    AviFile* _aviOutFile;
    VideoCodec _videoCodec;
#endif
};
}  
#endif
