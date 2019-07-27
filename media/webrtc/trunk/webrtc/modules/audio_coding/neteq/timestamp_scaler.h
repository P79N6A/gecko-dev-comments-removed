









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TIMESTAMP_SCALER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TIMESTAMP_SCALER_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/audio_coding/neteq/packet.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class DecoderDatabase;




class TimestampScaler {
 public:
  explicit TimestampScaler(const DecoderDatabase& decoder_database)
      : first_packet_received_(false),
        numerator_(1),
        denominator_(1),
        external_ref_(0),
        internal_ref_(0),
        decoder_database_(decoder_database) {}

  virtual ~TimestampScaler() {}

  
  virtual void Reset() { first_packet_received_ = false; }

  
  virtual void ToInternal(Packet* packet);

  
  
  virtual void ToInternal(PacketList* packet_list);

  
  
  virtual uint32_t ToInternal(uint32_t external_timestamp,
                              uint8_t rtp_payload_type);

  
  virtual uint32_t ToExternal(uint32_t internal_timestamp) const;

 private:
  bool first_packet_received_;
  int numerator_;
  int denominator_;
  uint32_t external_ref_;
  uint32_t internal_ref_;
  const DecoderDatabase& decoder_database_;

  DISALLOW_COPY_AND_ASSIGN(TimestampScaler);
};

}  
#endif  
