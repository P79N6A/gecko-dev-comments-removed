









#include "webrtc/modules/audio_processing/rms_level.h"

#include <assert.h>
#include <math.h>

namespace webrtc {

static const float kMaxSquaredLevel = 32768 * 32768;

RMSLevel::RMSLevel()
    : sum_square_(0),
      sample_count_(0) {}

RMSLevel::~RMSLevel() {}

void RMSLevel::Reset() {
  sum_square_ = 0;
  sample_count_ = 0;
}

void RMSLevel::Process(const int16_t* data, int length) {
  for (int i = 0; i < length; ++i) {
    sum_square_ += data[i] * data[i];
  }
  sample_count_ += length;
}

void RMSLevel::ProcessMuted(int length) {
  sample_count_ += length;
}

int RMSLevel::RMS() {
  if (sample_count_ == 0 || sum_square_ == 0) {
    Reset();
    return kMinLevel;
  }

  
  float rms = sum_square_ / (sample_count_ * kMaxSquaredLevel);
  
  rms = 10 * log10(rms);
  assert(rms <= 0);
  if (rms < -kMinLevel)
    rms = -kMinLevel;

  rms = -rms;
  Reset();
  return static_cast<int>(rms + 0.5);
}

}  
