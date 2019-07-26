



























































#ifndef CONTENT_BROWSER_SPEECH_ENDPOINTER_ENERGY_ENDPOINTER_H_
#define CONTENT_BROWSER_SPEECH_ENDPOINTER_ENERGY_ENDPOINTER_H_

#include <vector>

#include "nsAutoPtr.h"

#include "energy_endpointer_params.h"

namespace mozilla {


enum EpStatus {
  EP_PRE_SPEECH = 10,
  EP_POSSIBLE_ONSET,
  EP_SPEECH_PRESENT,
  EP_POSSIBLE_OFFSET,
  EP_POST_SPEECH,
};

class EnergyEndpointer {
 public:
  
  
  EnergyEndpointer();
  virtual ~EnergyEndpointer();

  void Init(const EnergyEndpointerParams& params);

  
  void StartSession();

  
  void EndSession();

  
  
  void SetEnvironmentEstimationMode();

  
  
  void SetUserInputMode();

  
  
  void ProcessAudioFrame(int64_t time_us,
                         const int16_t* samples, int num_samples,
                         float* rms_out);

  
  
  EpStatus Status(int64_t* status_time_us) const;

  bool estimating_environment() const {
    return estimating_environment_;
  }

  
  float GetNoiseLevelDb() const;

 private:
  class HistoryRing;

  
  
  
  void Restart(bool reset_threshold);

  
  void UpdateLevels(float rms);

  
  
  int TimeToFrame(float time) const;

  EpStatus status_;  
  float offset_confirm_dur_sec_;  
  int64_t endpointer_time_us_;  
  int64_t fast_update_frames_; 
  int64_t frame_counter_;  
  float max_window_dur_;  
  float sample_rate_;  

  
  nsAutoPtr<HistoryRing> history_;

  
  EnergyEndpointerParams params_;

  
  float decision_threshold_;

  
  
  bool estimating_environment_;

  
  float noise_level_;

  
  float rms_adapt_;

  
  int start_lag_;

  
  int end_lag_;

  
  
  int64_t user_input_start_time_us_;

  
  EnergyEndpointer(const EnergyEndpointer&);
  void operator=(const EnergyEndpointer&);
};

}  

#endif  
