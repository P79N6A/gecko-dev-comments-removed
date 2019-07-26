









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_TYPING_DETECTION_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_TYPING_DETECTION_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class TypingDetection {
 public:
  TypingDetection();
  virtual ~TypingDetection();

  
  
  
  bool Process(bool key_pressed, bool vad_activity);

  
  int TimeSinceLastDetectionInSeconds();

  
  
  void SetParameters(int time_window,
                     int cost_per_typing,
                     int reporting_threshold,
                     int penalty_decay,
                     int type_event_delay,
                     int report_detection_update_period);

 private:
  int time_active_;
  int time_since_last_typing_;
  int penalty_counter_;

  
  
  int counter_since_last_detection_update_;

  
  
  bool detection_to_report_;

  
  bool new_detection_to_report_;

  

  
  int time_window_;

  
  int cost_per_typing_;

  
  int reporting_threshold_;

  
  int penalty_decay_;

  
  int type_event_delay_;

  

  
  
  
  
  
  
  
  
  
  
  
  
  int report_detection_update_period_;
};

}  

#endif  
