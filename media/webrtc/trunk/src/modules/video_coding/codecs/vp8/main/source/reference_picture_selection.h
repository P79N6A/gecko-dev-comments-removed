














#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_REFERENCE_PICTURE_SELECTION_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_REFERENCE_PICTURE_SELECTION_H_

#include "typedefs.h"

namespace webrtc {

class ReferencePictureSelection {
 public:
  ReferencePictureSelection();
  void Init();

  
  
  void ReceivedRPSI(int rpsi_picture_id);

  
  
  
  
  
  
  bool ReceivedSLI(uint32_t now_ts);

  
  
  
  
  
  
  
  
  
  int EncodeFlags(int picture_id, bool send_refresh, uint32_t now_ts);

  
  
  void EncodedKeyFrame(int picture_id);

  
  
  void SetRtt(int rtt);

 private:
  static uint32_t TimestampDiff(uint32_t new_ts, uint32_t old_ts);

  
  enum { kMinUpdateInterval = 90 * 10 };  
  const double kRttConfidence;

  bool update_golden_next_;
  bool established_golden_;
  bool received_ack_;
  int last_sent_ref_picture_id_;
  uint32_t last_sent_ref_update_time_;
  int established_ref_picture_id_;
  uint32_t last_refresh_time_;
  uint32_t rtt_;
};

}  

#endif  
