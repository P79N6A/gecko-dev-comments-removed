









#include "modules/rtp_rtcp/source/forward_error_correction.h"
#include "typedefs.h"

namespace webrtc {


static const int kMaskSizeLBitSet = 6;

static const int kMaskSizeLBitClear = 2;

namespace internal {

class PacketMaskTable {
 public:
  PacketMaskTable(FecMaskType fec_mask_type, int num_media_packets);
  ~PacketMaskTable() {
  }
  FecMaskType fec_mask_type() const { return fec_mask_type_; }
  const uint8_t*** fec_packet_mask_table() const {
    return fec_packet_mask_table_;
  }
 private:
  FecMaskType InitMaskType(FecMaskType fec_mask_type,
                           int num_media_packets);
  const uint8_t*** InitMaskTable(FecMaskType fec_mask_type_);
  const FecMaskType fec_mask_type_;
  const uint8_t*** fec_packet_mask_table_;
};

 






















void GeneratePacketMasks(int numMediaPackets,
                         int numFecPackets,
                         int numImpPackets,
                         bool useUnequalProtection,
                         const PacketMaskTable& mask_table,
                         uint8_t* packetMask);

} 
} 
