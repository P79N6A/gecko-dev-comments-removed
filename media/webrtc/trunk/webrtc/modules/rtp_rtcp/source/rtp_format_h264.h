 









 













#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_H264_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_H264_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class RtpFormatH264 {
 public:

  
  
  
  

  enum NalHeader { 
    kNalHeaderOffset = 0, 
    kNalHeaderSize = 1, 
    kTypeMask = 0x1f, 
    kNriMask = 0x60, 
    kFBit = 0x80, 
  };

  enum NalType { 
    kIpb = 1, 
    kIdr = 5, 
    kSei = 6, 
    kSeiRecPt = 6, 
    kSps = 7, 
    kPps = 8, 
    kPrefix = 14, 
    kStapA = 24, 
    kFuA = 28, 
  };

  enum FuAHeader {
    kFuAHeaderOffset = 1, 
    kFuAHeaderSize = 1, 
    kFragStartBit = 0x80, 
    kFragEndBit = 0x40, 
    kReservedBit = 0x20 
  };
  enum StapAHeader {
    kStapAHeaderOffset = 1, 
    kAggUnitLengthSize = 2 
  };
  enum StartCodePrefix { 
    kStartCodeSize = 4 
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
