









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_INTERFACE_AUDIO_DECODER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_INTERFACE_AUDIO_DECODER_H_

#include <stdlib.h>  

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {

enum NetEqDecoder {
  kDecoderPCMu,
  kDecoderPCMa,
  kDecoderPCMu_2ch,
  kDecoderPCMa_2ch,
  kDecoderILBC,
  kDecoderISAC,
  kDecoderISACswb,
  kDecoderISACfb,
  kDecoderPCM16B,
  kDecoderPCM16Bwb,
  kDecoderPCM16Bswb32kHz,
  kDecoderPCM16Bswb48kHz,
  kDecoderPCM16B_2ch,
  kDecoderPCM16Bwb_2ch,
  kDecoderPCM16Bswb32kHz_2ch,
  kDecoderPCM16Bswb48kHz_2ch,
  kDecoderPCM16B_5ch,
  kDecoderG722,
  kDecoderG722_2ch,
  kDecoderRED,
  kDecoderAVT,
  kDecoderCNGnb,
  kDecoderCNGwb,
  kDecoderCNGswb32kHz,
  kDecoderCNGswb48kHz,
  kDecoderArbitrary,
  kDecoderOpus,
  kDecoderOpus_2ch,
  kDecoderCELT_32,
  kDecoderCELT_32_2ch,
};



class AudioDecoder {
 public:
  enum SpeechType {
    kSpeech = 1,
    kComfortNoise = 2
  };

  
  enum { kNotImplemented = -2 };

  explicit AudioDecoder(enum NetEqDecoder type)
    : codec_type_(type),
      channels_(1),
      state_(NULL) {
  }

  virtual ~AudioDecoder() {}

  
  
  
  
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type) = 0;

  
  
  virtual int DecodeRedundant(const uint8_t* encoded, size_t encoded_len,
                              int16_t* decoded, SpeechType* speech_type);

  
  virtual bool HasDecodePlc() const;

  
  
  virtual int DecodePlc(int num_frames, int16_t* decoded);

  
  virtual int Init() = 0;

  
  virtual int IncomingPacket(const uint8_t* payload,
                             size_t payload_len,
                             uint16_t rtp_sequence_number,
                             uint32_t rtp_timestamp,
                             uint32_t arrival_timestamp);

  
  virtual int ErrorCode();

  
  
  
  virtual int PacketDuration(const uint8_t* encoded, size_t encoded_len);

  virtual NetEqDecoder codec_type() const;

  
  void* state() { return state_; }

  
  static bool CodecSupported(NetEqDecoder codec_type);

  
  static int CodecSampleRateHz(NetEqDecoder codec_type);

  
  
  
  static AudioDecoder* CreateAudioDecoder(NetEqDecoder codec_type);

  size_t channels() { return channels_; }

 protected:
  static SpeechType ConvertSpeechType(int16_t type);

  enum NetEqDecoder codec_type_;
  size_t channels_;
  void* state_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoder);
};

}  
#endif
