









#include "reference_picture_selection.h"

#include "typedefs.h"
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"

namespace webrtc {

ReferencePictureSelection::ReferencePictureSelection()
    : kRttConfidence(1.33),
      update_golden_next_(true),
      established_golden_(false),
      received_ack_(false),
      last_sent_ref_picture_id_(0),
      last_sent_ref_update_time_(0),
      established_ref_picture_id_(0),
      last_refresh_time_(0),
      rtt_(0) {
}

void ReferencePictureSelection::Init() {
  update_golden_next_ = true;
  established_golden_ = false;
  received_ack_ = false;
  last_sent_ref_picture_id_ = 0;
  last_sent_ref_update_time_ = 0;
  established_ref_picture_id_ = 0;
  last_refresh_time_ = 0;
  rtt_ = 0;
}

void ReferencePictureSelection::ReceivedRPSI(int rpsi_picture_id) {
  
  if ((rpsi_picture_id & 0x3fff) == (last_sent_ref_picture_id_ & 0x3fff)) {
    
    received_ack_ = true;
    established_golden_ = update_golden_next_;
    update_golden_next_ = !update_golden_next_;
    established_ref_picture_id_ = last_sent_ref_picture_id_;
  }
}

bool ReferencePictureSelection::ReceivedSLI(uint32_t now_ts) {
  bool send_refresh = false;
  
  
  
  if (TimestampDiff(now_ts, last_refresh_time_) > rtt_) {
    send_refresh = true;
    last_refresh_time_ = now_ts;
  }
  return send_refresh;
}

int ReferencePictureSelection::EncodeFlags(int picture_id, bool send_refresh,
                                           uint32_t now_ts) {
  int flags = 0;
  
  if (send_refresh && received_ack_) {
    flags |= VP8_EFLAG_NO_REF_LAST;  
    if (established_golden_)
      flags |= VP8_EFLAG_NO_REF_ARF;  
    else
      flags |= VP8_EFLAG_NO_REF_GF;  
  }

  
  
  
  
  uint32_t update_interval = kRttConfidence * rtt_;
  if (update_interval < kMinUpdateInterval)
    update_interval = kMinUpdateInterval;
  
  if (TimestampDiff(now_ts, last_sent_ref_update_time_) > update_interval &&
      received_ack_) {
    flags |= VP8_EFLAG_NO_REF_LAST;  
    if (update_golden_next_) {
      flags |= VP8_EFLAG_FORCE_GF;  
      flags |= VP8_EFLAG_NO_UPD_ARF;  
      flags |= VP8_EFLAG_NO_REF_GF;  
    } else {
      flags |= VP8_EFLAG_FORCE_ARF;  
      flags |= VP8_EFLAG_NO_UPD_GF;  
      flags |= VP8_EFLAG_NO_REF_ARF;  
    }
    last_sent_ref_picture_id_ = picture_id;
    last_sent_ref_update_time_ = now_ts;
  } else {
    
    
    if (established_golden_)
      flags |= VP8_EFLAG_NO_REF_ARF;  
    else
      flags |= VP8_EFLAG_NO_REF_GF;   
    flags |= VP8_EFLAG_NO_UPD_GF;  
    flags |= VP8_EFLAG_NO_UPD_ARF;  
  }
  return flags;
}

void ReferencePictureSelection::EncodedKeyFrame(int picture_id) {
  last_sent_ref_picture_id_ = picture_id;
  received_ack_ = false;
}

void ReferencePictureSelection::SetRtt(int rtt) {
  
  rtt_ = 90 * rtt;
}

uint32_t ReferencePictureSelection::TimestampDiff(uint32_t new_ts,
                                                  uint32_t old_ts) {
  if (old_ts > new_ts) {
    
    return (new_ts + (static_cast<int64_t>(1) << 32)) - old_ts;
  }
  return new_ts - old_ts;
}

}  
