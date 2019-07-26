









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_DECODER_DATABASE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_DECODER_DATABASE_H_

#include "webrtc/modules/audio_coding/neteq4/decoder_database.h"

#include "gmock/gmock.h"

namespace webrtc {

class MockDecoderDatabase : public DecoderDatabase {
 public:
  virtual ~MockDecoderDatabase() { Die(); }
  MOCK_METHOD0(Die, void());
  MOCK_CONST_METHOD0(Empty,
      bool());
  MOCK_CONST_METHOD0(Size,
      int());
  MOCK_METHOD0(Reset,
      void());
  MOCK_METHOD2(RegisterPayload,
      int(uint8_t rtp_payload_type, NetEqDecoder codec_type));
  MOCK_METHOD4(InsertExternal,
      int(uint8_t rtp_payload_type, NetEqDecoder codec_type, int fs_hz,
          AudioDecoder* decoder));
  MOCK_METHOD1(Remove,
      int(uint8_t rtp_payload_type));
  MOCK_CONST_METHOD1(GetDecoderInfo,
      const DecoderInfo*(uint8_t rtp_payload_type));
  MOCK_CONST_METHOD1(GetRtpPayloadType,
      uint8_t(NetEqDecoder codec_type));
  MOCK_METHOD1(GetDecoder,
      AudioDecoder*(uint8_t rtp_payload_type));
  MOCK_CONST_METHOD2(IsType,
      bool(uint8_t rtp_payload_type, NetEqDecoder codec_type));
  MOCK_CONST_METHOD1(IsComfortNoise,
      bool(uint8_t rtp_payload_type));
  MOCK_CONST_METHOD1(IsDtmf,
      bool(uint8_t rtp_payload_type));
  MOCK_CONST_METHOD1(IsRed,
      bool(uint8_t rtp_payload_type));
  MOCK_METHOD2(SetActiveDecoder,
      int(uint8_t rtp_payload_type, bool* new_decoder));
  MOCK_METHOD0(GetActiveDecoder,
      AudioDecoder*());
  MOCK_METHOD1(SetActiveCngDecoder,
      int(uint8_t rtp_payload_type));
  MOCK_METHOD0(GetActiveCngDecoder,
      AudioDecoder*());
  MOCK_CONST_METHOD1(CheckPayloadTypes,
      int(const PacketList& packet_list));
};

}  
#endif  
