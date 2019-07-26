









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_AUDIO_DECODER_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_AUDIO_DECODER_IMPL_H_

#include <assert.h>

#ifndef AUDIO_DECODER_UNITTEST


#include "webrtc/engine_configurations.h"
#endif
#include "webrtc/modules/audio_coding/neteq4/interface/audio_decoder.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class AudioDecoderPcmU : public AudioDecoder {
 public:
  AudioDecoderPcmU() : AudioDecoder(kDecoderPCMu) {}
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int Init() { return 0; }
  virtual int PacketDuration(const uint8_t* encoded, size_t encoded_len);

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderPcmU);
};

class AudioDecoderPcmA : public AudioDecoder {
 public:
  AudioDecoderPcmA() : AudioDecoder(kDecoderPCMa) {}
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
  explicit AudioDecoderPcm16B(enum NetEqDecoder type);
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int Init() { return 0; }
  virtual int PacketDuration(const uint8_t* encoded, size_t encoded_len);

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderPcm16B);
};




class AudioDecoderPcm16BMultiCh : public AudioDecoderPcm16B {
 public:
  explicit AudioDecoderPcm16BMultiCh(enum NetEqDecoder type);

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
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderIlbc);
};
#endif

#ifdef WEBRTC_CODEC_ISAC
class AudioDecoderIsac : public AudioDecoder {
 public:
  AudioDecoderIsac();
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
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderIsac);
};

class AudioDecoderIsacSwb : public AudioDecoderIsac {
 public:
  AudioDecoderIsacSwb();

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderIsacSwb);
};

class AudioDecoderIsacFb : public AudioDecoderIsacSwb {
 public:
  AudioDecoderIsacFb();

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderIsacFb);
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
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderG722);
};

class AudioDecoderG722Stereo : public AudioDecoderG722 {
 public:
  AudioDecoderG722Stereo();
  virtual ~AudioDecoderG722Stereo();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int Init();

 private:
  
  
  
  
  
  void SplitStereoPacket(const uint8_t* encoded, size_t encoded_len,
                         uint8_t* encoded_deinterleaved);

  void* const state_left_;
  void* state_right_;

  DISALLOW_COPY_AND_ASSIGN(AudioDecoderG722Stereo);
};
#endif

#ifdef WEBRTC_CODEC_CELT
class AudioDecoderCelt : public AudioDecoder {
 public:
  explicit AudioDecoderCelt(enum NetEqDecoder type);
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
  explicit AudioDecoderOpus(enum NetEqDecoder type);
  virtual ~AudioDecoderOpus();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type);
  virtual int Init();
  virtual int PacketDuration(const uint8_t* encoded, size_t encoded_len);

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderOpus);
};
#endif







class AudioDecoderCng : public AudioDecoder {
 public:
  explicit AudioDecoderCng(enum NetEqDecoder type);
  virtual ~AudioDecoderCng();
  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type) { return -1; }
  virtual int Init();
  virtual int IncomingPacket(const uint8_t* payload,
                             size_t payload_len,
                             uint16_t rtp_sequence_number,
                             uint32_t rtp_timestamp,
                             uint32_t arrival_timestamp) { return -1; }

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioDecoderCng);
};

}  
#endif  
