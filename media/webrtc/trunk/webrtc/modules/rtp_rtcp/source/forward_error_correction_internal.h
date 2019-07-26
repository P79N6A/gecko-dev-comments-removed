









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_FORWARD_ERROR_CORRECTION_INTERNAL_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_FORWARD_ERROR_CORRECTION_INTERNAL_H_

#include "webrtc/modules/rtp_rtcp/source/forward_error_correction.h"
#include "webrtc/typedefs.h"

namespace webrtc {


static const int kMaskSizeLBitSet = 6;

static const int kMaskSizeLBitClear = 2;

namespace internal {

class PacketMaskTable {
 public:
  PacketMaskTable(FecMaskType fec_mask_type, int num_media_packets);
  ~PacketMaskTable() {}
  FecMaskType fec_mask_type() const { return fec_mask_type_; }
  const uint8_t*** fec_packet_mask_table() const {
    return fec_packet_mask_table_;
  }

 private:
  FecMaskType InitMaskType(FecMaskType fec_mask_type, int num_media_packets);
  const uint8_t*** InitMaskTable(FecMaskType fec_mask_type_);
  const FecMaskType fec_mask_type_;
  const uint8_t*** fec_packet_mask_table_;
};






















void GeneratePacketMasks(int num_media_packets, int num_fec_packets,
                         int num_imp_packets, bool use_unequal_protection,
                         const PacketMaskTable& mask_table,
                         uint8_t* packet_mask);

}  
}  
#endif  
