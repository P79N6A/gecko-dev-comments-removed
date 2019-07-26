










#ifndef WEBRTC_MODULES_MEDIA_FILE_SOURCE_MEDIA_FILE_UTILITY_H_
#define WEBRTC_MODULES_MEDIA_FILE_SOURCE_MEDIA_FILE_UTILITY_H_

#include <stdio.h>

#include "common_types.h"
#include "media_file_defines.h"

namespace webrtc {
class AviFile;
class InStream;
class OutStream;

class ModuleFileUtility
{
public:

    ModuleFileUtility(const WebRtc_Word32 id);
    ~ModuleFileUtility();

#ifdef WEBRTC_MODULE_UTILITY_VIDEO
    
    
    
    
    WebRtc_Word32 InitAviReading(const char* fileName, bool videoOnly,
                                 bool loop);

    
    
    
    
    
    
    WebRtc_Word32 ReadAviAudioData(WebRtc_Word8* outBuffer,
                                   const WebRtc_UWord32 bufferLengthInBytes);

    
    
    
    WebRtc_Word32 ReadAviVideoData(WebRtc_Word8* videoBuffer,
                                   const WebRtc_UWord32 bufferLengthInBytes);

    
    
    
    
    WebRtc_Word32 InitAviWriting(const char* filename,
                                 const CodecInst& codecInst,
                                 const VideoCodec& videoCodecInst,
                                 const bool videoOnly);

    
    
    
    
    
    WebRtc_Word32 WriteAviAudioData(const WebRtc_Word8* audioBuffer,
                                    WebRtc_UWord32 bufferLengthInBytes);


    
    
    
    
    
    
    WebRtc_Word32 WriteAviVideoData(const WebRtc_Word8* videoBuffer,
                                    WebRtc_UWord32 bufferLengthInBytes);

    
    WebRtc_Word32 CloseAviFile();

    WebRtc_Word32 VideoCodecInst(VideoCodec& codecInst);
#endif 

    
    
    
    WebRtc_Word32 InitWavReading(InStream& stream,
                                 const WebRtc_UWord32 startPointMs = 0,
                                 const WebRtc_UWord32 stopPointMs = 0);

    
    
    
    
    
    
    WebRtc_Word32 ReadWavDataAsMono(InStream& stream, WebRtc_Word8* audioBuffer,
                                    const WebRtc_UWord32 dataLengthInBytes);

    
    
    
    
    
    
    
    
    WebRtc_Word32 ReadWavDataAsStereo(InStream& wav,
                                      WebRtc_Word8* audioBufferLeft,
                                      WebRtc_Word8* audioBufferRight,
                                      const WebRtc_UWord32 bufferLength);

    
    
    
    
    WebRtc_Word32 InitWavWriting(OutStream& stream, const CodecInst& codecInst);

    
    
    
    
    WebRtc_Word32 WriteWavData(OutStream& stream,
                               const WebRtc_Word8* audioBuffer,
                               const WebRtc_UWord32 bufferLength);

    
    
    
    
    
    WebRtc_Word32 UpdateWavHeader(OutStream& stream);

    
    
    
    
    
    WebRtc_Word32 InitPCMReading(InStream& stream,
                                 const WebRtc_UWord32 startPointMs = 0,
                                 const WebRtc_UWord32 stopPointMs = 0,
                                 const WebRtc_UWord32 freqInHz = 16000);

    
    
    
    WebRtc_Word32 ReadPCMData(InStream& stream, WebRtc_Word8* audioBuffer,
                              const WebRtc_UWord32 dataLengthInBytes);

    
    
    
    WebRtc_Word32 InitPCMWriting(OutStream& stream,
                                 const WebRtc_UWord32 freqInHz = 16000);

    
    
    
    
    WebRtc_Word32 WritePCMData(OutStream& stream,
                               const WebRtc_Word8* audioBuffer,
                               WebRtc_UWord32 bufferLength);

    
    
    
    WebRtc_Word32 InitCompressedReading(InStream& stream,
                                        const WebRtc_UWord32 startPointMs = 0,
                                        const WebRtc_UWord32 stopPointMs = 0);

    
    
    
    WebRtc_Word32 ReadCompressedData(InStream& stream,
                                     WebRtc_Word8* audioBuffer,
                                     const WebRtc_UWord32 dataLengthInBytes);

    
    
    WebRtc_Word32 InitCompressedWriting(OutStream& stream,
                                        const CodecInst& codecInst);

    
    
    
    
    
    WebRtc_Word32 WriteCompressedData(OutStream& stream,
                                      const WebRtc_Word8* audioBuffer,
                                      const WebRtc_UWord32 bufferLength);

    
    
    WebRtc_Word32 InitPreEncodedReading(InStream& stream,
                                        const CodecInst& codecInst);

    
    
    
    WebRtc_Word32 ReadPreEncodedData(InStream& stream,
                                     WebRtc_Word8* audioBuffer,
                                     const WebRtc_UWord32 dataLengthInBytes);

    
    
    WebRtc_Word32 InitPreEncodedWriting(OutStream& stream,
                                        const CodecInst& codecInst);

    
    
    
   
    
    WebRtc_Word32 WritePreEncodedData(OutStream& stream,
                                      const WebRtc_Word8* inData,
                                      const WebRtc_UWord32 dataLengthInBytes);

    
    
    WebRtc_Word32 FileDurationMs(const char* fileName,
                                 const FileFormats fileFormat,
                                 const WebRtc_UWord32 freqInHz = 16000);

    
    WebRtc_UWord32 PlayoutPositionMs();

    
    
    WebRtc_Word32 codec_info(CodecInst& codecInst);

private:
    
    enum{WAV_MAX_BUFFER_SIZE = 480*2*2};


    WebRtc_Word32 InitWavCodec(WebRtc_UWord32 samplesPerSec,
                               WebRtc_UWord32 channels,
                               WebRtc_UWord32 bitsPerSample,
                               WebRtc_UWord32 formatTag);

    
    WebRtc_Word32 ReadWavHeader(InStream& stream);

    
    
    
    
    
    
    WebRtc_Word32 WriteWavHeader(OutStream& stream,
                                 const WebRtc_UWord32 freqInHz,
                                 const WebRtc_UWord32 bytesPerSample,
                                 const WebRtc_UWord32 channels,
                                 const WebRtc_UWord32 format,
                                 const WebRtc_UWord32 lengthInBytes);

    
    
    WebRtc_Word32 ReadWavData(InStream& stream, WebRtc_UWord8* audioBuffer,
                              const WebRtc_UWord32 dataLengthInBytes);

    
    
    WebRtc_Word32 set_codec_info(const CodecInst& codecInst);

    struct WAVE_FMTINFO_header
    {
        WebRtc_Word16 formatTag;
        WebRtc_Word16 nChannels;
        WebRtc_Word32 nSamplesPerSec;
        WebRtc_Word32 nAvgBytesPerSec;
        WebRtc_Word16 nBlockAlign;
        WebRtc_Word16 nBitsPerSample;
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
    WebRtc_Word32 _dataSize;      
    
    
    WebRtc_Word32 _readSizeBytes;

    WebRtc_Word32 _id;

    WebRtc_UWord32 _stopPointInMs;
    WebRtc_UWord32 _startPointInMs;
    WebRtc_UWord32 _playoutPositionMs;
    WebRtc_UWord32 _bytesWritten;

    CodecInst codec_info_;
    MediaFileUtility_CodecType _codecId;

    
    WebRtc_Word32  _bytesPerSample;
    WebRtc_Word32  _readPos;

    
    bool _reading;
    bool _writing;

    
    WebRtc_UWord8 _tempData[WAV_MAX_BUFFER_SIZE];

#ifdef WEBRTC_MODULE_UTILITY_VIDEO
    AviFile* _aviAudioInFile;
    AviFile* _aviVideoInFile;
    AviFile* _aviOutFile;
    VideoCodec _videoCodec;
#endif
};
} 
#endif
