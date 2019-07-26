









#ifndef WEBRTC_COMMON_TYPES_H
#define WEBRTC_COMMON_TYPES_H

#include "typedefs.h"

#ifdef WEBRTC_EXPORT
    #define WEBRTC_DLLEXPORT _declspec(dllexport)
#elif WEBRTC_DLL
    #define WEBRTC_DLLEXPORT _declspec(dllimport)
#else
    #define WEBRTC_DLLEXPORT
#endif

#ifndef NULL
    #define NULL 0
#endif

#define RTP_PAYLOAD_NAME_SIZE 32

namespace webrtc {

class InStream
{
public:
    virtual int Read(void *buf,int len) = 0;
    virtual int Rewind() {return -1;}
    virtual ~InStream() {}
protected:
    InStream() {}
};

class OutStream
{
public:
    virtual bool Write(const void *buf,int len) = 0;
    virtual int Rewind() {return -1;}
    virtual ~OutStream() {}
protected:
    OutStream() {}
};

enum TraceModule
{
    
    kTraceVoice              = 0x0001,
    
    kTraceVideo              = 0x0002,
    
    kTraceUtility            = 0x0003,
    kTraceRtpRtcp            = 0x0004,
    kTraceTransport          = 0x0005,
    kTraceSrtp               = 0x0006,
    kTraceAudioCoding        = 0x0007,
    kTraceAudioMixerServer   = 0x0008,
    kTraceAudioMixerClient   = 0x0009,
    kTraceFile               = 0x000a,
    kTraceAudioProcessing    = 0x000b,
    kTraceVideoCoding        = 0x0010,
    kTraceVideoMixer         = 0x0011,
    kTraceAudioDevice        = 0x0012,
    kTraceVideoRenderer      = 0x0014,
    kTraceVideoCapture       = 0x0015,
    kTraceVideoPreocessing   = 0x0016
};

enum TraceLevel
{
    kTraceNone               = 0x0000,    
    kTraceStateInfo          = 0x0001,
    kTraceWarning            = 0x0002,
    kTraceError              = 0x0004,
    kTraceCritical           = 0x0008,
    kTraceApiCall            = 0x0010,
    kTraceDefault            = 0x00ff,

    kTraceModuleCall         = 0x0020,
    kTraceMemory             = 0x0100,   
    kTraceTimer              = 0x0200,   
    kTraceStream             = 0x0400,   

    
    kTraceDebug              = 0x0800,  
    kTraceInfo               = 0x1000,  

    kTraceAll                = 0xffff
};


class TraceCallback
{
public:
    virtual void Print(const TraceLevel level,
                       const char *traceString,
                       const int length) = 0;
protected:
    virtual ~TraceCallback() {}
    TraceCallback() {}
};


enum FileFormats
{
    kFileFormatWavFile        = 1,
    kFileFormatCompressedFile = 2,
    kFileFormatAviFile        = 3,
    kFileFormatPreencodedFile = 4,
    kFileFormatPcm16kHzFile   = 7,
    kFileFormatPcm8kHzFile    = 8,
    kFileFormatPcm32kHzFile   = 9
};


enum ProcessingTypes
{
    kPlaybackPerChannel = 0,
    kPlaybackAllChannelsMixed,
    kRecordingPerChannel,
    kRecordingAllChannelsMixed
};


enum CipherTypes
{
    kCipherNull               = 0,
    kCipherAes128CounterMode  = 1
};

enum AuthenticationTypes
{
    kAuthNull       = 0,
    kAuthHmacSha1   = 3
};

enum SecurityLevels
{
    kNoProtection                    = 0,
    kEncryption                      = 1,
    kAuthentication                  = 2,
    kEncryptionAndAuthentication     = 3
};




class Encryption
{
public:
    
    
    
    
    
    
    
    
    
    
    
    virtual void encrypt(
        int channel,
        unsigned char* in_data,
        unsigned char* out_data,
        int bytes_in,
        int* bytes_out) = 0;

    
    
    
    
    
    
    
    
    
    
    
    virtual void decrypt(
        int channel,
        unsigned char* in_data,
        unsigned char* out_data,
        int bytes_in,
        int* bytes_out) = 0;

    
    
    virtual void encrypt_rtcp(
        int channel,
        unsigned char* in_data,
        unsigned char* out_data,
        int bytes_in,
        int* bytes_out) = 0;

    
    
    virtual void decrypt_rtcp(
        int channel,
        unsigned char* in_data,
        unsigned char* out_data,
        int bytes_in,
        int* bytes_out) = 0;

protected:
    virtual ~Encryption() {}
    Encryption() {}
};


class Transport
{
public:
    virtual int SendPacket(int channel, const void *data, int len) = 0;
    virtual int SendRTCPPacket(int channel, const void *data, int len) = 0;

protected:
    virtual ~Transport() {}
    Transport() {}
};






struct CodecInst
{
    int pltype;
    char plname[RTP_PAYLOAD_NAME_SIZE];
    int plfreq;
    int pacsize;
    int channels;
    int rate;
};

enum FrameType
{
    kFrameEmpty            = 0,
    kAudioFrameSpeech      = 1,
    kAudioFrameCN          = 2,
    kVideoFrameKey         = 3,    
    kVideoFrameDelta       = 4,    
    kVideoFrameGolden      = 5,    
    kVideoFrameAltRef      = 6
};


enum {kRtpCsrcSize = 15}; 

enum RTPDirections
{
    kRtpIncoming = 0,
    kRtpOutgoing
};

enum PayloadFrequencies
{
    kFreq8000Hz = 8000,
    kFreq16000Hz = 16000,
    kFreq32000Hz = 32000
};

enum VadModes                 
{
    kVadConventional = 0,      
    kVadAggressiveLow,
    kVadAggressiveMid,
    kVadAggressiveHigh         
};

struct NetworkStatistics           
{
    
    WebRtc_UWord16 currentBufferSize;
    
    WebRtc_UWord16 preferredBufferSize;
    
    bool jitterPeaksFound;
    
    WebRtc_UWord16 currentPacketLossRate;
    
    WebRtc_UWord16 currentDiscardRate;
    
    
    WebRtc_UWord16 currentExpandRate;
    
    
    WebRtc_UWord16 currentPreemptiveRate;
    
    WebRtc_UWord16 currentAccelerateRate;
    
    int32_t clockDriftPPM;
    
    int meanWaitingTimeMs;
    
    int medianWaitingTimeMs;
    
    int minWaitingTimeMs;
    
    int maxWaitingTimeMs;
};

typedef struct
{
    int min;              
    int max;              
    int average;          
} StatVal;

typedef struct           
{
    StatVal speech_rx;   
    StatVal speech_tx;   
    StatVal noise_rx;    
    StatVal noise_tx;    
} LevelStatistics;

typedef struct        
{
    StatVal erl;      
    StatVal erle;     
    StatVal rerl;     
    
    StatVal a_nlp;
} EchoStatistics;

enum TelephoneEventDetectionMethods
{
    kInBand = 0,
    kOutOfBand = 1,
    kInAndOutOfBand = 2
};

enum NsModes    
{
    kNsUnchanged = 0,   
    kNsDefault,         
    kNsConference,      
    kNsLowSuppression,  
    kNsModerateSuppression,
    kNsHighSuppression,
    kNsVeryHighSuppression     
};

enum AgcModes                  
{
    kAgcUnchanged = 0,        
    kAgcDefault,              
    
    
    kAgcAdaptiveAnalog,
    
    
    kAgcAdaptiveDigital,
    
    
    kAgcFixedDigital
};


enum EcModes                   
{
    kEcUnchanged = 0,          
    kEcDefault,                
    kEcConference,             
    kEcAec,                    
    kEcAecm                    
};


enum AecmModes                 
{
    kAecmQuietEarpieceOrHeadset = 0,
                               
    kAecmEarpiece,             
    kAecmLoudEarpiece,         
    kAecmSpeakerphone,         
    kAecmLoudSpeakerphone      
};


typedef struct
{
    unsigned short targetLeveldBOv;
    unsigned short digitalCompressionGaindB;
    bool           limiterEnable;
} AgcConfig;                  

enum StereoChannel
{
    kStereoLeft = 0,
    kStereoRight,
    kStereoBoth
};


enum AudioLayers
{
    kAudioPlatformDefault = 0,
    kAudioWindowsWave = 1,
    kAudioWindowsCore = 2,
    kAudioLinuxAlsa = 3,
    kAudioLinuxPulse = 4
};

enum NetEqModes             
{
    
    
    kNetEqDefault = 0,
    
    
    kNetEqStreaming = 1,
    
    
    kNetEqFax = 2
};

enum NetEqBgnModes          
{
    
    
    kBgnOn = 0,
    
    kBgnFade = 1,
    
    
    kBgnOff = 2
};

enum OnHoldModes            
{
    kHoldSendAndPlay = 0,    
    kHoldSendOnly,           
    kHoldPlayOnly            
};

enum AmrMode
{
    kRfc3267BwEfficient = 0,
    kRfc3267OctetAligned = 1,
    kRfc3267FileStorage = 2
};






enum RawVideoType
{
    kVideoI420     = 0,
    kVideoYV12     = 1,
    kVideoYUY2     = 2,
    kVideoUYVY     = 3,
    kVideoIYUV     = 4,
    kVideoARGB     = 5,
    kVideoRGB24    = 6,
    kVideoRGB565   = 7,
    kVideoARGB4444 = 8,
    kVideoARGB1555 = 9,
    kVideoMJPEG    = 10,
    kVideoNV12     = 11,
    kVideoNV21     = 12,
    kVideoBGRA     = 13,
    kVideoUnknown  = 99
};


enum { kConfigParameterSize = 128};
enum { kPayloadNameSize = 32};
enum { kMaxSimulcastStreams = 4};
enum { kMaxTemporalStreams = 4};

enum VideoCodecComplexity
{
    kComplexityNormal = 0,
    kComplexityHigh    = 1,
    kComplexityHigher  = 2,
    kComplexityMax     = 3
};

enum VideoCodecProfile
{
    kProfileBase = 0x00,
    kProfileMain = 0x01
};

enum VP8ResilienceMode {
  kResilienceOff,    
                     
                     
  kResilientStream,  
                     
                     
  kResilientFrames   
                     
};


struct VideoCodecVP8
{
    bool                 pictureLossIndicationOn;
    bool                 feedbackModeOn;
    VideoCodecComplexity complexity;
    VP8ResilienceMode    resilience;
    unsigned char        numberOfTemporalLayers;
    bool                 denoisingOn;
};


struct VideoCodecGeneric
{
};


enum VideoCodecType
{
    kVideoCodecVP8,
    kVideoCodecI420,
    kVideoCodecRED,
    kVideoCodecULPFEC,
    kVideoCodecUnknown
};

union VideoCodecUnion
{
    VideoCodecVP8       VP8;
    VideoCodecGeneric   Generic;
};




struct SimulcastStream
{
    unsigned short      width;
    unsigned short      height;
    unsigned char       numberOfTemporalLayers;
    unsigned int        maxBitrate;
    unsigned int        qpMax; 
};


struct VideoCodec
{
    VideoCodecType      codecType;
    char                plName[kPayloadNameSize];
    unsigned char       plType;

    unsigned short      width;
    unsigned short      height;

    unsigned int        startBitrate;
    unsigned int        maxBitrate;
    unsigned int        minBitrate;
    unsigned char       maxFramerate;

    VideoCodecUnion     codecSpecific;

    unsigned int        qpMax;
    unsigned char       numberOfSimulcastStreams;
    SimulcastStream     simulcastStream[kMaxSimulcastStreams];
};
}  
#endif  
