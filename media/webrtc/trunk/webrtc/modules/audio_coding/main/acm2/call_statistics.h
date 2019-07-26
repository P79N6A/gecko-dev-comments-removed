









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_CALL_STATISTICS_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_CALL_STATISTICS_H_

#include "webrtc/common_types.h"
#include "webrtc/modules/interface/module_common_types.h"














namespace webrtc {

namespace acm2 {

class CallStatistics {
 public:
  CallStatistics() {}
  ~CallStatistics() {}

  
  
  void DecodedByNetEq(AudioFrame::SpeechType speech_type);

  
  
  void DecodedBySilenceGenerator();

  
  
  
  const AudioDecodingCallStats& GetDecodingStatistics() const;

 private:
  
  void ResetDecodingStatistics();

  AudioDecodingCallStats decoding_stat_;
};

}  

}  

#endif  
