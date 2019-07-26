









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_DELAY_PEAK_DETECTOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MOCK_MOCK_DELAY_PEAK_DETECTOR_H_

#include "webrtc/modules/audio_coding/neteq4/delay_peak_detector.h"

#include "gmock/gmock.h"

namespace webrtc {

class MockDelayPeakDetector : public DelayPeakDetector {
 public:
  virtual ~MockDelayPeakDetector() { Die(); }
  MOCK_METHOD0(Die, void());
  MOCK_METHOD0(Reset, void());
  MOCK_METHOD1(SetPacketAudioLength, void(int length_ms));
  MOCK_METHOD0(peak_found, bool());
  MOCK_CONST_METHOD0(MaxPeakHeight, int());
  MOCK_CONST_METHOD0(MaxPeakPeriod, int());
  MOCK_METHOD2(Update, bool(int inter_arrival_time, int target_level));
  MOCK_METHOD1(IncrementCounter, void(int inc_ms));
};

}  
#endif  
