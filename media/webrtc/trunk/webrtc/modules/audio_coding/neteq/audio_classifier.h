









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_AUDIO_CLASSIFIER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_AUDIO_CLASSIFIER_H_

#if defined(__cplusplus)
extern "C" {
#endif
#include "celt.h"
#include "analysis.h"
#include "opus_private.h"
#if defined(__cplusplus)
}
#endif

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {





class AudioClassifier {
 public:
  AudioClassifier();
  virtual ~AudioClassifier();

  
  
  
  bool Analysis(const int16_t* input, int input_length, int channels);

  
  virtual bool is_music() const { return is_music_; }

  
  float music_probability() const { return music_probability_; }

 private:
  AnalysisInfo analysis_info_;
  bool is_music_;
  float music_probability_;
  const CELTMode* celt_mode_;
  TonalityAnalysisState analysis_state_;
};

}  

#endif
