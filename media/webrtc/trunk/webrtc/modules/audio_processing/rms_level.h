









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_RMS_LEVEL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_RMS_LEVEL_H_

#include "webrtc/typedefs.h"

namespace webrtc {









class RMSLevel {
 public:
  static const int kMinLevel = 127;

  RMSLevel();
  ~RMSLevel();

  
  
  void Reset();

  
  void Process(const int16_t* data, int length);

  
  
  void ProcessMuted(int length);

  
  
  
  int RMS();

 private:
  float sum_square_;
  int sample_count_;
};

}  

#endif  

