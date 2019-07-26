









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_EXTERNAL_DECODER_PCM16B_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_EXTERNAL_DECODER_PCM16B_H_

#include "webrtc/modules/audio_coding/neteq4/interface/audio_decoder.h"

#include "gmock/gmock.h"
#include "webrtc/modules/audio_coding/codecs/pcm16b/include/pcm16b.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {

using ::testing::_;
using ::testing::Invoke;



class ExternalPcm16B : public AudioDecoder {
 public:
  explicit ExternalPcm16B(enum NetEqDecoder type)
      : AudioDecoder(type) {
  }

  virtual int Decode(const uint8_t* encoded, size_t encoded_len,
                     int16_t* decoded, SpeechType* speech_type) {
    int16_t temp_type;
    int16_t ret = WebRtcPcm16b_DecodeW16(
        state_, reinterpret_cast<int16_t*>(const_cast<uint8_t*>(encoded)),
        static_cast<int16_t>(encoded_len), decoded, &temp_type);
    *speech_type = ConvertSpeechType(temp_type);
    return ret;
  }

  virtual int Init() { return 0; }

 private:
  DISALLOW_COPY_AND_ASSIGN(ExternalPcm16B);
};



class MockExternalPcm16B : public ExternalPcm16B {
 public:
  explicit MockExternalPcm16B(enum NetEqDecoder type)
      : ExternalPcm16B(type),
        real_(type) {
    
    ON_CALL(*this, Decode(_, _, _, _))
        .WillByDefault(Invoke(&real_, &ExternalPcm16B::Decode));
    ON_CALL(*this, HasDecodePlc())
        .WillByDefault(Invoke(&real_, &ExternalPcm16B::HasDecodePlc));
    ON_CALL(*this, DecodePlc(_, _))
        .WillByDefault(Invoke(&real_, &ExternalPcm16B::DecodePlc));
    ON_CALL(*this, Init())
        .WillByDefault(Invoke(&real_, &ExternalPcm16B::Init));
    ON_CALL(*this, IncomingPacket(_, _, _, _, _))
        .WillByDefault(Invoke(&real_, &ExternalPcm16B::IncomingPacket));
    ON_CALL(*this, ErrorCode())
        .WillByDefault(Invoke(&real_, &ExternalPcm16B::ErrorCode));
    ON_CALL(*this, codec_type())
        .WillByDefault(Invoke(&real_, &ExternalPcm16B::codec_type));
  }
  virtual ~MockExternalPcm16B() { Die(); }

  MOCK_METHOD0(Die, void());
  MOCK_METHOD4(Decode,
      int(const uint8_t* encoded, size_t encoded_len, int16_t* decoded,
          SpeechType* speech_type));
  MOCK_CONST_METHOD0(HasDecodePlc,
      bool());
  MOCK_METHOD2(DecodePlc,
      int(int num_frames, int16_t* decoded));
  MOCK_METHOD0(Init,
      int());
  MOCK_METHOD5(IncomingPacket,
      int(const uint8_t* payload, size_t payload_len,
          uint16_t rtp_sequence_number, uint32_t rtp_timestamp,
          uint32_t arrival_timestamp));
  MOCK_METHOD0(ErrorCode,
      int());
  MOCK_CONST_METHOD0(codec_type,
      NetEqDecoder());

 private:
  ExternalPcm16B real_;
};

}  
#endif  
