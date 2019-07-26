 









 













#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_H264_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_H264_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class RtpFormatH264 {
 public:
  enum {
    kH264NALHeaderLengthInBytes = 1,
    kH264FUAHeaderLengthInBytes = 2,
    kH264FUANALUType            = 28,
    kH264NALU_SPS               = 7,
    kH264NALU_PPS               = 8,
    kH264NALU_IDR               = 5
  };

  
  
  RtpFormatH264(const uint8_t* payload_data,
                uint32_t payload_size,
                int max_payload_len);

  ~RtpFormatH264();

  
  
  
  
  
  
  
  
  
  
  int NextPacket(uint8_t* buffer,
                 int* bytes_to_send,
                 bool* last_packet);

 private:
  const uint8_t* payload_data_;
  const int payload_size_;
  const int max_payload_len_;
  int   fragments_;
  int   fragment_size_;
  int   next_fragment_;

  DISALLOW_COPY_AND_ASSIGN(RtpFormatH264);
};

}  

#endif
