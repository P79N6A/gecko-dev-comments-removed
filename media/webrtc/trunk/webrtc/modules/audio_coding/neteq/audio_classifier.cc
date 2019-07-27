









#include "webrtc/modules/audio_coding/neteq/audio_classifier.h"

#include <assert.h>
#include <string.h>

namespace webrtc {

static const int kDefaultSampleRateHz = 48000;
static const int kDefaultFrameRateHz = 50;
static const int kDefaultFrameSizeSamples =
    kDefaultSampleRateHz / kDefaultFrameRateHz;
static const float kDefaultThreshold = 0.5f;

AudioClassifier::AudioClassifier()
    : analysis_info_(),
      is_music_(false),
      music_probability_(0),
      
      
      
      celt_mode_(opus_custom_mode_create(kDefaultSampleRateHz,
                                         kDefaultFrameSizeSamples,
                                         NULL)),
      analysis_state_() {
  assert(celt_mode_);
}

AudioClassifier::~AudioClassifier() {}

bool AudioClassifier::Analysis(const int16_t* input,
                               int input_length,
                               int channels) {
  
  assert((input_length / channels) == kDefaultFrameSizeSamples);

  
  assert(channels == 1 || channels == 2);

  
  
  
  
  
  run_analysis(&analysis_state_,
               celt_mode_,
               input,
               kDefaultFrameSizeSamples,
               kDefaultFrameSizeSamples,
               0,
               -2,
               channels,
               kDefaultSampleRateHz,
               16,
               downmix_int,
               &analysis_info_);
  music_probability_ = analysis_info_.music_prob;
  is_music_ = music_probability_ > kDefaultThreshold;
  return is_music_;
}

}  
