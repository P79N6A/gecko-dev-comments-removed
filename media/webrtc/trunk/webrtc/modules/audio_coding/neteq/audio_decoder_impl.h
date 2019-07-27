









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_AUDIO_DECODER_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_AUDIO_DECODER_IMPL_H_

#include <assert.h>

#ifndef AUDIO_DECODER_UNITTEST


#include "webrtc/engine_configurations.h"
#endif
#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/audio_coding/codecs/cng/include/webrtc_cng.h"
#ifdef WEBRTC_CODEC_G722
#include "webrtc/modules/audio_coding/codecs/g722/include/g722_interface.h"
#endif
#ifdef WEBRTC_CODEC_ILBC
#include "webrtc/modules/audio_coding/codecs/ilbc/interface/ilbc.h"
#endif
#ifdef WEBRTC_CODEC_ISACFX
#include "webrtc/modules/audio_coding/codecs/isac/fix/interface/isacfix.h"
#endif
#ifdef WEBRTC_CODEC_ISAC
#include "webrtc/modules/audio_coding/codecs/isac/main/interface/isac.h"
#endif
#ifdef WEBRTC_CODEC_OPUS
#include "webrtc/modules/audio_coding/codecs/opus/interface/opus_interface.h"
#endif
#include "webrtc/modules/audio_coding/neteq/interface/audio_decoder.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class AudioDecoderPcmU : public AudioDecoder {
 public:
  AudioDecoderPcmU() {}
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int Init() { return 0; }
  virtual int PacketDuration(const uint8_t* encoded, size_t encoded_len);

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderPcmU);
};

class AudioDecoderPcmA : public AudioDecoder {
 public:
  AudioDecoderPcmA() {}
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int Init() { return 0; }
  virtual int PacketDuration(const uint8_t* encoded, size_t encoded_len);

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderPcmA);
};

class AudioDecoderPcmUMultiCh : public AudioDecoderPcmU {
 public:
  explicit AudioDecoderPcmUMultiCh(size_t channels) : AudioDecoderPcmU() {
    assert(channels > 0);
    channels_ = channels;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderPcmUMultiCh);
};

class AudioDecoderPcmAMultiCh : public AudioDecoderPcmA {
 public:
  explicit AudioDecoderPcmAMultiCh(size_t channels) : AudioDecoderPcmA() {
    assert(channels > 0);
    channels_ = channels;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderPcmAMultiCh);
};

#ifdef WEBRTC_CODEC_PCM16


class AudioDecoderPcm16B : public AudioDecoder {
 public:
  AudioDecoderPcm16B();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int Init() { return 0; }
  virtual int PacketDuration(const uint8_t* encoded, size_t encoded_len);

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderPcm16B);
};




class AudioDecoderPcm16BMultiCh : public AudioDecoderPcm16B {
 public:
  explicit AudioDecoderPcm16BMultiCh(int num_channels);

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderPcm16BMultiCh);
};
#endif

#ifdef WEBRTC_CODEC_ILBC
class AudioDecoderIlbc : public AudioDecoder {
 public:
  AudioDecoderIlbc();
  virtual ~AudioDecoderIlbc();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual bool HasDecodePlc() const { return true; }
  virtual int DecodePlc(int num_frames, int16_t* decoded);
  virtual int Init();

 private:
  iLBC_decinst_t* dec_state_;
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderIlbc);
};
#endif

#ifdef WEBRTC_CODEC_ISAC
class AudioDecoderIsac : public AudioDecoder {
 public:
  explicit AudioDecoderIsac(int decode_sample_rate_hz);
  virtual ~AudioDecoderIsac();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int DecodeRedundant(const uint8_t* encoded, size_t encoded_len,
                              int16_t* decoded, SpeechType* speech_type);
  virtual bool HasDecodePlc() const { return true; }
  virtual int DecodePlc(int num_frames, int16_t* decoded);
  virtual int Init();
  virtual int IncomingPacket(const uint8_t* payload,
                             size_t payload_len,
                             uint16_t rtp_sequence_number,
                             uint32_t rtp_timestamp,
                             uint32_t arrival_timestamp);
  virtual int ErrorCode();

 private:
  ISACStruct* isac_state_;
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderIsac);
};
#endif

#ifdef WEBRTC_CODEC_ISACFX
class AudioDecoderIsacFix : public AudioDecoder {
 public:
  AudioDecoderIsacFix();
  virtual ~AudioDecoderIsacFix();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int Init();
  virtual int IncomingPacket(const uint8_t* payload,
                             size_t payload_len,
                             uint16_t rtp_sequence_number,
                             uint32_t rtp_timestamp,
                             uint32_t arrival_timestamp);
  virtual int ErrorCode();

 private:
  ISACFIX_MainStruct* isac_state_;
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderIsacFix);
};
#endif

#ifdef WEBRTC_CODEC_G722
class AudioDecoderG722 : public AudioDecoder {
 public:
  AudioDecoderG722();
  virtual ~AudioDecoderG722();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual bool HasDecodePlc() const { return false; }
  virtual int Init();
  virtual int PacketDuration(const uint8_t* encoded, size_t encoded_len);

 private:
  G722DecInst* dec_state_;
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderG722);
};

class AudioDecoderG722Stereo : public AudioDecoder {
 public:
  AudioDecoderG722Stereo();
  virtual ~AudioDecoderG722Stereo();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int Init();

 private:
  
  
  
  
  
  void SplitStereoPacket(const uint8_t* encoded, size_t encoded_len,
                         uint8_t* encoded_deinterleaved);

  G722DecInst* dec_state_left_;
  G722DecInst* dec_state_right_;

  DISALLOW_COPY_AND_ASSIGN(AudioDecoderG722Stereo);
};
#endif

#ifdef WEBRTC_CODEC_CELT
class AudioDecoderCelt : public AudioDecoder {
 public:
  explicit AudioDecoderCelt(int num_channels);
  virtual ~AudioDecoderCelt();

  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int Init();
  virtual bool HasDecodePlc() const;
  virtual int DecodePlc(int num_frames, int16_t* decoded);

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderCelt);
};
#endif

#ifdef WEBRTC_CODEC_OPUS
class AudioDecoderOpus : public AudioDecoder {
 public:
  explicit AudioDecoderOpus(int num_channels);
  virtual ~AudioDecoderOpus();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int DecodeRedundant(const uint8_t* encoded, size_t encoded_len,
                              int16_t* decoded, SpeechType* speech_type);
  virtual int Init();
  virtual int PacketDuration(const uint8_t* encoded, size_t encoded_len);
  virtual int PacketDurationRedundant(const uint8_t* encoded,
                                      size_t encoded_len) const;
  virtual bool PacketHasFec(const uint8_t* encoded, size_t encoded_len) const;

 private:
  OpusDecInst* dec_state_;
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderOpus);
};
#endif







class AudioDecoderCng : public AudioDecoder {
 public:
  explicit AudioDecoderCng();
  virtual ~AudioDecoderCng();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type) { return -1; }
  virtual int Init();
  virtual int IncomingPacket(const uint8_t* payload,
                             size_t payload_len,
                             uint16_t rtp_sequence_number,
                             uint32_t rtp_timestamp,
                             uint32_t arrival_timestamp) { return -1; }

  virtual CNG_dec_inst* CngDecoderInstance() OVERRIDE { return dec_state_; }

 private:
  CNG_dec_inst* dec_state_;
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderCng);
};

}  
#endif  
