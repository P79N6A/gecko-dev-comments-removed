









#ifndef WEBRTC_COMMON_TYPES_H_
#define WEBRTC_COMMON_TYPES_H_

#include <stddef.h> 
#include "webrtc/typedefs.h"

#if defined(_MSC_VER)


#pragma warning(disable:4351)
#endif

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

#if defined(WEBRTC_WIN)

#define STR_CASE_CMP(s1, s2) ::_stricmp(s1, s2)

#define STR_NCASE_CMP(s1, s2, n) ::_strnicmp(s1, s2, n)
#else
#define STR_CASE_CMP(s1, s2) ::strcasecmp(s1, s2)
#define STR_NCASE_CMP(s1, s2, n) ::strncasecmp(s1, s2, n)
#endif

namespace webrtc {

class Config;

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
    kTraceUndefined              = 0,
    
    kTraceVoice                  = 0x0001,
    
    kTraceVideo                  = 0x0002,
    
    kTraceUtility                = 0x0003,
    kTraceRtpRtcp                = 0x0004,
    kTraceTransport              = 0x0005,
    kTraceSrtp                   = 0x0006,
    kTraceAudioCoding            = 0x0007,
    kTraceAudioMixerServer       = 0x0008,
    kTraceAudioMixerClient       = 0x0009,
    kTraceFile                   = 0x000a,
    kTraceAudioProcessing        = 0x000b,
    kTraceVideoCoding            = 0x0010,
    kTraceVideoMixer             = 0x0011,
    kTraceAudioDevice            = 0x0012,
    kTraceVideoRenderer          = 0x0014,
    kTraceVideoCapture           = 0x0015,
    kTraceVideoPreocessing       = 0x0016,
    kTraceRemoteBitrateEstimator = 0x0017,
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

    
    kTraceTerseInfo          = 0x2000,

    kTraceAll                = 0xffff
};


class TraceCallback {
 public:
  virtual void Print(TraceLevel level, const char* message, int length) = 0;

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
    kRecordingAllChannelsMixed,
    kRecordingPreprocessing
};

enum FrameType
{
    kFrameEmpty            = 0,
    kAudioFrameSpeech      = 1,
    kAudioFrameCN          = 2,
    kVideoFrameKey         = 3,    
    kVideoFrameDelta       = 4,    
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


struct RtcpStatistics {
  RtcpStatistics()
    : fraction_lost(0),
      cumulative_lost(0),
      extended_max_sequence_number(0),
      jitter(0) {}

  uint8_t fraction_lost;
  uint32_t cumulative_lost;
  uint32_t extended_max_sequence_number;
  uint32_t jitter;
};


class RtcpStatisticsCallback {
 public:
  virtual ~RtcpStatisticsCallback() {}

  virtual void StatisticsUpdated(const RtcpStatistics& statistics,
                                 uint32_t ssrc) = 0;
};


struct StreamDataCounters {
  StreamDataCounters()
   : bytes(0),
     header_bytes(0),
     padding_bytes(0),
     packets(0),
     retransmitted_packets(0),
     fec_packets(0) {}

  uint32_t bytes;  
  uint32_t header_bytes;  
  uint32_t padding_bytes;  
  uint32_t packets;  
  uint32_t retransmitted_packets;  
  uint32_t fec_packets;  
};


class StreamDataCountersCallback {
 public:
  virtual ~StreamDataCountersCallback() {}

  virtual void DataCountersUpdated(const StreamDataCounters& counters,
                                   uint32_t ssrc) = 0;
};


struct BitrateStatistics {
  BitrateStatistics() : bitrate_bps(0), packet_rate(0), timestamp_ms(0) {}

  uint32_t bitrate_bps;   
  uint32_t packet_rate;   
  uint64_t timestamp_ms;  
};


class BitrateStatisticsObserver {
 public:
  virtual ~BitrateStatisticsObserver() {}

  virtual void Notify(const BitrateStatistics& stats, uint32_t ssrc) = 0;
};


class FrameCountObserver {
 public:
  virtual ~FrameCountObserver() {}
  virtual void FrameCountUpdated(FrameType frame_type,
                                 uint32_t frame_count,
                                 const unsigned int ssrc) = 0;
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
    
    uint16_t currentBufferSize;
    
    uint16_t preferredBufferSize;
    
    bool jitterPeaksFound;
    
    uint16_t currentPacketLossRate;
    
    uint16_t currentDiscardRate;
    
    
    uint16_t currentExpandRate;
    
    
    uint16_t currentPreemptiveRate;
    
    uint16_t currentAccelerateRate;
    
    int32_t clockDriftPPM;
    
    int meanWaitingTimeMs;
    
    int medianWaitingTimeMs;
    
    int minWaitingTimeMs;
    
    int maxWaitingTimeMs;
    
    int addedSamples;
};


struct AudioDecodingCallStats {
  AudioDecodingCallStats()
      : calls_to_silence_generator(0),
        calls_to_neteq(0),
        decoded_normal(0),
        decoded_plc(0),
        decoded_cng(0),
        decoded_plc_cng(0) {}

  int calls_to_silence_generator;  
                                   
  int calls_to_neteq;  
  int decoded_normal;  
  int decoded_plc;  
  int decoded_cng;  
  int decoded_plc_cng;  
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
    
    
    kNetEqFax = 2,
    
    
    kNetEqOff = 3
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

enum VideoReceiveState
{
  kReceiveStateInitial,            
  kReceiveStateNormal,
  kReceiveStatePreemptiveNACK,     
  kReceiveStateWaitingKey,         
  kReceiveStateDecodingWithErrors, 
  kReceiveStateNoIncoming,         
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
    bool                 errorConcealmentOn;
    bool                 automaticResizeOn;
    bool                 frameDroppingOn;
    int                  keyFrameInterval;
};


struct VideoCodecH264
{
    uint8_t        profile;
    uint8_t        constraints;
    uint8_t        level;
    uint8_t        packetizationMode; 
    bool           frameDroppingOn;
    int            keyFrameInterval;
    
    const uint8_t* spsData;
    size_t         spsLen;
    const uint8_t* ppsData;
    size_t         ppsLen;
};


enum VideoCodecType
{
    kVideoCodecVP8,
    kVideoCodecH264,
    kVideoCodecI420,
    kVideoCodecRED,
    kVideoCodecULPFEC,
    kVideoCodecGeneric,
    kVideoCodecUnknown
};

union VideoCodecUnion
{
    VideoCodecVP8       VP8;
    VideoCodecH264      H264;
};




struct SimulcastStream
{
    unsigned short      width;
    unsigned short      height;
    unsigned char       numberOfTemporalLayers;
    unsigned int        maxBitrate;  
    unsigned int        targetBitrate;  
    unsigned int        minBitrate;  
    unsigned int        qpMax; 
};

enum VideoCodecMode {
  kRealtimeVideo,
  kScreensharing
};


struct VideoCodec
{
    VideoCodecType      codecType;
    char                plName[kPayloadNameSize];
    unsigned char       plType;

    unsigned short      width;
    unsigned short      height;
    
    unsigned char       resolution_divisor;

    unsigned int        startBitrate;  
    unsigned int        maxBitrate;  
    unsigned int        minBitrate;  
    unsigned char       maxFramerate;

    VideoCodecUnion     codecSpecific;

    unsigned int        qpMax;
    unsigned char       numberOfSimulcastStreams;
    SimulcastStream     simulcastStream[kMaxSimulcastStreams];

    VideoCodecMode      mode;

    
    
    Config*  extra_options;
};




struct OverUseDetectorOptions {
  OverUseDetectorOptions()
      : initial_slope(8.0/512.0),
        initial_offset(0),
        initial_e(),
        initial_process_noise(),
        initial_avg_noise(0.0),
        initial_var_noise(50),
        initial_threshold(25.0) {
    initial_e[0][0] = 100;
    initial_e[1][1] = 1e-1;
    initial_e[0][1] = initial_e[1][0] = 0;
    initial_process_noise[0] = 1e-10;
    initial_process_noise[1] = 1e-2;
  }
  double initial_slope;
  double initial_offset;
  double initial_e[2][2];
  double initial_process_noise[2];
  double initial_avg_noise;
  double initial_var_noise;
  double initial_threshold;
};

enum CPULoadState {
  kLoadRelaxed,
  kLoadNormal,
  kLoadStressed
};

class CPULoadStateObserver {
public:
  virtual void onLoadStateChanged(CPULoadState aNewState) = 0;
  virtual ~CPULoadStateObserver() {};
};

class CPULoadStateCallbackInvoker {
public:
    virtual void AddObserver(CPULoadStateObserver* aObserver) = 0;
    virtual void RemoveObserver(CPULoadStateObserver* aObserver) = 0;
    virtual ~CPULoadStateCallbackInvoker() {};
};



struct PacketTime {
  PacketTime() : timestamp(-1), max_error_us(-1) {}
  PacketTime(int64_t timestamp, int64_t max_error_us)
      : timestamp(timestamp), max_error_us(max_error_us) {
  }

  int64_t timestamp;    
  int64_t max_error_us; 
                        
                        
                        
                        
};

}  

#endif  

