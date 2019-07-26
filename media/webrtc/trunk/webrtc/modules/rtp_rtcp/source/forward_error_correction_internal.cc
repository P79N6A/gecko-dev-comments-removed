









#include "webrtc/modules/rtp_rtcp/source/forward_error_correction_internal.h"

#include <assert.h>
#include <string.h>

#include "webrtc/modules/rtp_rtcp/source/fec_private_tables_bursty.h"
#include "webrtc/modules/rtp_rtcp/source/fec_private_tables_random.h"

namespace {


enum ProtectionMode {
  kModeNoOverlap,
  kModeOverlap,
  kModeBiasFirstPacket,
};













void FitSubMask(int num_mask_bytes, int num_sub_mask_bytes, int num_rows,
                const uint8_t* sub_mask, uint8_t* packet_mask) {
  if (num_mask_bytes == num_sub_mask_bytes) {
    memcpy(packet_mask, sub_mask, num_rows * num_sub_mask_bytes);
  } else {
    for (int i = 0; i < num_rows; ++i) {
      int pkt_mask_idx = i * num_mask_bytes;
      int pkt_mask_idx2 = i * num_sub_mask_bytes;
      for (int j = 0; j < num_sub_mask_bytes; ++j) {
        packet_mask[pkt_mask_idx] = sub_mask[pkt_mask_idx2];
        pkt_mask_idx++;
        pkt_mask_idx2++;
      }
    }
  }
}




















void ShiftFitSubMask(int num_mask_bytes, int res_mask_bytes,
                     int num_column_shift, int end_row, const uint8_t* sub_mask,
                     uint8_t* packet_mask) {

  
  const int num_bit_shifts = (num_column_shift % 8);
  const int num_byte_shifts = num_column_shift >> 3;

  

  
  for (int i = num_column_shift; i < end_row; ++i) {
    
    
    int pkt_mask_idx =
        i * num_mask_bytes + res_mask_bytes - 1 + num_byte_shifts;
    
    int pkt_mask_idx2 =
        (i - num_column_shift) * res_mask_bytes + res_mask_bytes - 1;

    uint8_t shift_right_curr_byte = 0;
    uint8_t shift_left_prev_byte = 0;
    uint8_t comb_new_byte = 0;

    
    
    
    if (num_mask_bytes > res_mask_bytes) {
      shift_left_prev_byte = (sub_mask[pkt_mask_idx2] << (8 - num_bit_shifts));
      packet_mask[pkt_mask_idx + 1] = shift_left_prev_byte;
    }

    
    
    
    for (int j = res_mask_bytes - 1; j > 0; j--) {
      
      shift_right_curr_byte = sub_mask[pkt_mask_idx2] >> num_bit_shifts;

      
      
      shift_left_prev_byte =
          (sub_mask[pkt_mask_idx2 - 1] << (8 - num_bit_shifts));

      
      comb_new_byte = shift_right_curr_byte | shift_left_prev_byte;

      
      packet_mask[pkt_mask_idx] = comb_new_byte;
      pkt_mask_idx--;
      pkt_mask_idx2--;
    }
    
    shift_right_curr_byte = sub_mask[pkt_mask_idx2] >> num_bit_shifts;
    packet_mask[pkt_mask_idx] = shift_right_curr_byte;

  }
}
}  

namespace webrtc {
namespace internal {

PacketMaskTable::PacketMaskTable(FecMaskType fec_mask_type,
                                 int num_media_packets)
    : fec_mask_type_(InitMaskType(fec_mask_type, num_media_packets)),
      fec_packet_mask_table_(InitMaskTable(fec_mask_type_)) {}





FecMaskType PacketMaskTable::InitMaskType(FecMaskType fec_mask_type,
                                          int num_media_packets) {
  
  assert(num_media_packets <= static_cast<int>(sizeof(kPacketMaskRandomTbl) /
                                               sizeof(*kPacketMaskRandomTbl)));
  switch (fec_mask_type) {
    case kFecMaskRandom: { return kFecMaskRandom; }
    case kFecMaskBursty: {
      int max_media_packets = static_cast<int>(sizeof(kPacketMaskBurstyTbl) /
                                               sizeof(*kPacketMaskBurstyTbl));
      if (num_media_packets > max_media_packets) {
        return kFecMaskRandom;
      } else {
        return kFecMaskBursty;
      }
    }
  }
  assert(false);
  return kFecMaskRandom;
}



const uint8_t*** PacketMaskTable::InitMaskTable(FecMaskType fec_mask_type) {
  switch (fec_mask_type) {
    case kFecMaskRandom: { return kPacketMaskRandomTbl; }
    case kFecMaskBursty: { return kPacketMaskBurstyTbl; }
  }
  assert(false);
  return kPacketMaskRandomTbl;
}


void RemainingPacketProtection(int num_media_packets, int num_fec_remaining,
                               int num_fec_for_imp_packets, int num_mask_bytes,
                               ProtectionMode mode, uint8_t* packet_mask,
                               const PacketMaskTable& mask_table) {
  if (mode == kModeNoOverlap) {
    

    const int l_bit =
        (num_media_packets - num_fec_for_imp_packets) > 16 ? 1 : 0;

    const int res_mask_bytes =
        (l_bit == 1) ? kMaskSizeLBitSet : kMaskSizeLBitClear;

    const uint8_t* packet_mask_sub_21 = mask_table.fec_packet_mask_table()[
        num_media_packets - num_fec_for_imp_packets - 1][num_fec_remaining - 1];

    ShiftFitSubMask(num_mask_bytes, res_mask_bytes, num_fec_for_imp_packets,
                    (num_fec_for_imp_packets + num_fec_remaining),
                    packet_mask_sub_21, packet_mask);

  } else if (mode == kModeOverlap || mode == kModeBiasFirstPacket) {
    

    const uint8_t* packet_mask_sub_22 = mask_table
        .fec_packet_mask_table()[num_media_packets - 1][num_fec_remaining - 1];

    FitSubMask(num_mask_bytes, num_mask_bytes, num_fec_remaining,
               packet_mask_sub_22,
               &packet_mask[num_fec_for_imp_packets * num_mask_bytes]);

    if (mode == kModeBiasFirstPacket) {
      for (int i = 0; i < num_fec_remaining; ++i) {
        int pkt_mask_idx = i * num_mask_bytes;
        packet_mask[pkt_mask_idx] = packet_mask[pkt_mask_idx] | (1 << 7);
      }
    }
  } else {
    assert(false);
  }

}


void ImportantPacketProtection(int num_fec_for_imp_packets, int num_imp_packets,
                               int num_mask_bytes, uint8_t* packet_mask,
                               const PacketMaskTable& mask_table) {
  const int l_bit = num_imp_packets > 16 ? 1 : 0;
  const int num_imp_mask_bytes =
      (l_bit == 1) ? kMaskSizeLBitSet : kMaskSizeLBitClear;

  
  const uint8_t* packet_mask_sub_1 = mask_table.fec_packet_mask_table()[
      num_imp_packets - 1][num_fec_for_imp_packets - 1];

  FitSubMask(num_mask_bytes, num_imp_mask_bytes, num_fec_for_imp_packets,
             packet_mask_sub_1, packet_mask);

}




int SetProtectionAllocation(int num_media_packets, int num_fec_packets,
                            int num_imp_packets) {

  

  
  float alloc_par = 0.5;
  int max_num_fec_for_imp = alloc_par * num_fec_packets;

  int num_fec_for_imp_packets =
      (num_imp_packets < max_num_fec_for_imp) ? num_imp_packets
                                              : max_num_fec_for_imp;

  
  if (num_fec_packets == 1 && (num_media_packets > 2 * num_imp_packets)) {
    num_fec_for_imp_packets = 0;
  }

  return num_fec_for_imp_packets;
}














































void UnequalProtectionMask(int num_media_packets, int num_fec_packets,
                           int num_imp_packets, int num_mask_bytes,
                           uint8_t* packet_mask,
                           const PacketMaskTable& mask_table) {

  
  

  ProtectionMode mode = kModeOverlap;
  int num_fec_for_imp_packets = 0;

  if (mode != kModeBiasFirstPacket) {
    num_fec_for_imp_packets = SetProtectionAllocation(
        num_media_packets, num_fec_packets, num_imp_packets);
  }

  int num_fec_remaining = num_fec_packets - num_fec_for_imp_packets;
  

  
  
  
  if (num_fec_for_imp_packets > 0) {
    ImportantPacketProtection(num_fec_for_imp_packets, num_imp_packets,
                              num_mask_bytes, packet_mask, mask_table);
  }

  
  
  
  if (num_fec_remaining > 0) {
    RemainingPacketProtection(num_media_packets, num_fec_remaining,
                              num_fec_for_imp_packets, num_mask_bytes, mode,
                              packet_mask, mask_table);
  }

}

void GeneratePacketMasks(int num_media_packets, int num_fec_packets,
                         int num_imp_packets, bool use_unequal_protection,
                         const PacketMaskTable& mask_table,
                         uint8_t* packet_mask) {
  assert(num_media_packets > 0);
  assert(num_fec_packets <= num_media_packets && num_fec_packets > 0);
  assert(num_imp_packets <= num_media_packets && num_imp_packets >= 0);

  int l_bit = num_media_packets > 16 ? 1 : 0;
  const int num_mask_bytes =
      (l_bit == 1) ? kMaskSizeLBitSet : kMaskSizeLBitClear;

  
  if (!use_unequal_protection || num_imp_packets == 0) {
    
    
    
    memcpy(packet_mask, mask_table.fec_packet_mask_table()[
                            num_media_packets - 1][num_fec_packets - 1],
           num_fec_packets * num_mask_bytes);
  } else  
      {
    UnequalProtectionMask(num_media_packets, num_fec_packets, num_imp_packets,
                          num_mask_bytes, packet_mask, mask_table);

  }  
}  

}  
}  
