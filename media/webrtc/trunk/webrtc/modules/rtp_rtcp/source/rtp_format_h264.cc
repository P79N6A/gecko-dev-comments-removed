









#include <string.h>  

#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "webrtc/modules/rtp_rtcp/source/rtp_format_h264.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

RtpFormatH264::RtpFormatH264(const uint8_t* payload_data,
                             uint32_t payload_size,
                             int max_payload_len)
    : payload_data_(payload_data),
      payload_size_(static_cast<int>(payload_size)),
      max_payload_len_(static_cast<int>(max_payload_len)),
      fragments_(0),
      fragment_size_(0),
      next_fragment_(-1) {
  if (payload_size_ <= max_payload_len_) {
    fragments_ = 0;
  } else {
    fragment_size_ = max_payload_len_ - (kFuAHeaderOffset+kFuAHeaderSize);
    fragments_ = ((payload_size_ - kNalHeaderSize) + (fragment_size_-1)) /
                 fragment_size_;
    next_fragment_ = 0;
  }
}

RtpFormatH264::~RtpFormatH264() {
}

int RtpFormatH264::NextPacket(uint8_t* buffer,
                              int* bytes_to_send,
                              bool* last_packet) {
  if (next_fragment_ == fragments_) {
    *bytes_to_send = 0;
    *last_packet   = true;
    return -1;
  }

  

  
  
  
  
  uint8_t header = payload_data_[0];
  uint8_t type   = header & kTypeMask;
  if (payload_size_ <= max_payload_len_) {

#ifdef TEST_STAP_A
    static uint8_t sps_buffer[256];
    static uint32_t sps_size;
    if (type == kSps) {

      sps_buffer[0] = kStapA;
      *(reinterpret_cast<uint16_t*>(&sps_buffer[1])) = htons(payload_size_); 
      memcpy(&sps_buffer[1 + sizeof(uint16_t)], payload_data_, payload_size_);
      sps_size = 1 + sizeof(uint16_t) + payload_size_;
      *bytes_to_send = 0;
      return -1;
    } else if (type == kPps && sps_size != 0) {
      
      *(reinterpret_cast<uint16_t*>(&sps_buffer[sps_size])) = htons(payload_size_);
      memcpy(&sps_buffer[sps_size + sizeof(uint16_t)], payload_data_, payload_size_);
      memcpy(buffer, sps_buffer, sps_size + 2 + payload_size_);
      *bytes_to_send = sps_size + 2 + payload_size_;
      sps_size = 0;
      *last_packet   = false;
      return 0;
    }
#endif
    
    *bytes_to_send = payload_size_;
    
    
    
    
    if (type == kSps || type == kPps ||
        type == kSei || type == kPrefix) {
      *last_packet   = false;
    } else {
      *last_packet   = true;
    }
    memcpy(buffer, payload_data_, payload_size_);
    WEBRTC_TRACE(kTraceStream, kTraceRtpRtcp, -1,
                 "RtpFormatH264(single NALU with type:%d, payload_size:%d",
                 type, payload_size_);
    return 0;
  } else {
    uint8_t fu_indicator = (header & (kFBit | kNriMask)) | kFuA;
    uint8_t fu_header = 0;
    bool first_fragment = (next_fragment_ == 0);
    bool last_fragment = (next_fragment_ == (fragments_ -1));

    
    fu_header |= (first_fragment ? kFragStartBit : 0);
    fu_header |= (last_fragment ? kFragEndBit :0);
    fu_header |= type;
    buffer[0] = fu_indicator;
    buffer[1] = fu_header;

    if (last_fragment) {
      
      *bytes_to_send = payload_size_ -
                       kNalHeaderSize -
                       next_fragment_ * fragment_size_ +
                       kFuAHeaderOffset+kFuAHeaderSize;
      *last_packet   = true;
      memcpy(buffer + kFuAHeaderOffset+kFuAHeaderSize,
             payload_data_ + kNalHeaderSize +
                next_fragment_ * fragment_size_,
             *bytes_to_send - (kFuAHeaderOffset+kFuAHeaderSize));
      
    } else {
      *bytes_to_send = fragment_size_ + kFuAHeaderOffset+kFuAHeaderSize;
      *last_packet   = false;
      memcpy(buffer + kFuAHeaderOffset+kFuAHeaderSize,
             payload_data_ + kNalHeaderSize +
                 next_fragment_ * fragment_size_,
             fragment_size_);  
    }
    next_fragment_++;
    return 1;
  }
}

}  
