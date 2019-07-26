














#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_CODEC_DATABASE_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_CODEC_DATABASE_H_

#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/source/acm_generic_codec.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"

namespace webrtc {


class ACMCodecDB {
 public:
  
  
  enum {
    kNone = -1
#if (defined(WEBRTC_CODEC_ISAC) || defined(WEBRTC_CODEC_ISACFX))
    , kISAC
# if (defined(WEBRTC_CODEC_ISAC))
    , kISACSWB
    , kISACFB
# endif
#endif
#ifdef WEBRTC_CODEC_PCM16
    
    , kPCM16B
    , kPCM16Bwb
    , kPCM16Bswb32kHz
    
    , kPCM16B_2ch
    , kPCM16Bwb_2ch
    , kPCM16Bswb32kHz_2ch
#endif
    
    , kPCMU
    , kPCMA
    
    , kPCMU_2ch
    , kPCMA_2ch
#ifdef WEBRTC_CODEC_ILBC
    , kILBC
#endif
#ifdef WEBRTC_CODEC_AMR
    , kGSMAMR
#endif
#ifdef WEBRTC_CODEC_AMRWB
    , kGSMAMRWB
#endif
#ifdef WEBRTC_CODEC_CELT
    
    , kCELT32
    
    , kCELT32_2ch
#endif
#ifdef WEBRTC_CODEC_G722
    
    , kG722
    
    , kG722_2ch
#endif
#ifdef WEBRTC_CODEC_G722_1
    , kG722_1_32
    , kG722_1_24
    , kG722_1_16
#endif
#ifdef WEBRTC_CODEC_G722_1C
    , kG722_1C_48
    , kG722_1C_32
    , kG722_1C_24
#endif
#ifdef WEBRTC_CODEC_G729
    , kG729
#endif
#ifdef WEBRTC_CODEC_G729_1
    , kG729_1
#endif
#ifdef WEBRTC_CODEC_GSMFR
    , kGSMFR
#endif
#ifdef WEBRTC_CODEC_OPUS
    
    , kOpus
#endif
#ifdef WEBRTC_CODEC_SPEEX
    , kSPEEX8
    , kSPEEX16
#endif
    , kCNNB
    , kCNWB
    , kCNSWB
    , kCNFB
#ifdef WEBRTC_CODEC_AVT
    , kAVT
#endif
#ifdef WEBRTC_CODEC_RED
    , kRED
#endif
    , kNumCodecs
  };

  
#ifndef WEBRTC_CODEC_ISAC
  enum {kISACSWB = -1};
  enum {kISACFB = -1};
# ifndef WEBRTC_CODEC_ISACFX
  enum {kISAC = -1};
# endif
#endif
#ifndef WEBRTC_CODEC_PCM16
  
  enum {kPCM16B = -1};
  enum {kPCM16Bwb = -1};
  enum {kPCM16Bswb32kHz = -1};
  
  enum {kPCM16B_2ch = -1};
  enum {kPCM16Bwb_2ch = -1};
  enum {kPCM16Bswb32kHz_2ch = -1};
#endif
  
  enum {kPCM16Bswb48kHz = -1};
#ifndef WEBRTC_CODEC_ILBC
  enum {kILBC = -1};
#endif
#ifndef WEBRTC_CODEC_AMR
  enum {kGSMAMR = -1};
#endif
#ifndef WEBRTC_CODEC_AMRWB
  enum {kGSMAMRWB = -1};
#endif
#ifndef WEBRTC_CODEC_CELT
  
  enum {kCELT32 = -1};
  
  enum {kCELT32_2ch = -1};
#endif
#ifndef WEBRTC_CODEC_G722
  
  enum {kG722 = -1};
  
  enum {kG722_2ch = -1};
#endif
#ifndef WEBRTC_CODEC_G722_1
  enum {kG722_1_32 = -1};
  enum {kG722_1_24 = -1};
  enum {kG722_1_16 = -1};
#endif
#ifndef WEBRTC_CODEC_G722_1C
  enum {kG722_1C_48 = -1};
  enum {kG722_1C_32 = -1};
  enum {kG722_1C_24 = -1};
#endif
#ifndef WEBRTC_CODEC_G729
  enum {kG729 = -1};
#endif
#ifndef WEBRTC_CODEC_G729_1
  enum {kG729_1 = -1};
#endif
#ifndef WEBRTC_CODEC_GSMFR
  enum {kGSMFR = -1};
#endif
#ifndef WEBRTC_CODEC_SPEEX
  enum {kSPEEX8 = -1};
  enum {kSPEEX16 = -1};
#endif
#ifndef WEBRTC_CODEC_OPUS
  
  enum {kOpus = -1};
#endif
#ifndef WEBRTC_CODEC_AVT
  enum {kAVT = -1};
#endif
#ifndef WEBRTC_CODEC_RED
  enum {kRED = -1};
#endif

  
  
  
  
  static const int kMaxNumCodecs =  50;
  static const int kMaxNumPacketSize = 6;

  
  
  
  
  
  
  
  
  
  struct CodecSettings {
    int num_packet_sizes;
    int packet_sizes_samples[kMaxNumPacketSize];
    int basic_block_samples;
    int channel_support;
  };

  
  
  
  
  
  
  
  
  
  static int Codec(int codec_id, CodecInst* codec_inst);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int CodecNumber(const CodecInst* codec_inst, int* mirror_id,
                         char* err_message, int max_message_len_byte);
  static int CodecNumber(const CodecInst* codec_inst, int* mirror_id);
  static int CodecId(const CodecInst* codec_inst);
  static int CodecId(const char* payload_name, int frequency, int channels);
  static int ReceiverCodecNumber(const CodecInst* codec_inst, int* mirror_id);

  
  
  
  
  
  
  
  
  
  static int CodecFreq(int codec_id);

  
  
  
  
  
  
  
  
  static int BasicCodingBlock(int codec_id);

  
  static const WebRtcNetEQDecoder* NetEQDecoders();

  
  
  
  
  
  
  
  
  
  
  
  static int MirrorID(int codec_id);

  
  
  
  
  static ACMGenericCodec* CreateCodecInstance(const CodecInst* codec_inst);

  
  
  
  
  
  
  static bool IsRateValid(int codec_id, int rate);
  static bool IsISACRateValid(int rate);
  static bool IsILBCRateValid(int rate, int frame_size_samples);
  static bool IsAMRRateValid(int rate);
  static bool IsAMRwbRateValid(int rate);
  static bool IsG7291RateValid(int rate);
  static bool IsSpeexRateValid(int rate);
  static bool IsOpusRateValid(int rate);
  static bool IsCeltRateValid(int rate);

  
  
  
  
  static bool ValidPayloadType(int payload_type);

  
  
  
  
  
  
  
  
  static const CodecInst database_[kMaxNumCodecs];
  static const CodecSettings codec_settings_[kMaxNumCodecs];
  static const WebRtcNetEQDecoder neteq_decoders_[kMaxNumCodecs];
};

}  

#endif
