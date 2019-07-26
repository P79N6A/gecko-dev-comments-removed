



























#ifndef CONTENT_BROWSER_SPEECH_ENDPOINTER_ENDPOINTER_H_
#define CONTENT_BROWSER_SPEECH_ENDPOINTER_ENDPOINTER_H_

#include "energy_endpointer.h"

namespace mozilla {

struct AudioChunk;





























class Endpointer {
 public:
  explicit Endpointer(int sample_rate);

  
  void StartSession();

  
  void EndSession();

  
  
  void SetEnvironmentEstimationMode();

  
  
  void SetUserInputMode();

  
  
  EpStatus ProcessAudio(const AudioChunk& raw_audio, float* rms_out);

  
  EpStatus Status(int64_t *time_us);

  
  
  
  int32_t FrameSize() const {
    return frame_size_;
  }

  
  
  bool DidStartReceivingSpeech() const {
    return speech_previously_detected_;
  }

  bool IsEstimatingEnvironment() const {
    return energy_endpointer_.estimating_environment();
  }

  void set_speech_input_complete_silence_length(int64_t time_us) {
    speech_input_complete_silence_length_us_ = time_us;
  }

  void set_long_speech_input_complete_silence_length(int64_t time_us) {
    long_speech_input_complete_silence_length_us_ = time_us;
  }

  void set_speech_input_possibly_complete_silence_length(int64_t time_us) {
    speech_input_possibly_complete_silence_length_us_ = time_us;
  }

  void set_long_speech_length(int64_t time_us) {
    long_speech_length_us_ = time_us;
  }

  bool speech_input_complete() const {
    return speech_input_complete_;
  }

  
  float NoiseLevelDb() const { return energy_endpointer_.GetNoiseLevelDb(); }

 private:
  
  
  void Reset();

  
  int64_t speech_input_minimum_length_us_;

  
  
  
  int64_t speech_input_possibly_complete_silence_length_us_;

  
  
  
  int64_t speech_input_complete_silence_length_us_;

  
  
  
  
  int64_t long_speech_input_complete_silence_length_us_;

  
  
  
  
  int64_t long_speech_length_us_;

  
  int64_t speech_start_time_us_;

  
  int64_t speech_end_time_us_;

  int64_t audio_frame_time_us_;
  EpStatus old_ep_status_;
  bool waiting_for_speech_possibly_complete_timeout_;
  bool waiting_for_speech_complete_timeout_;
  bool speech_previously_detected_;
  bool speech_input_complete_;
  EnergyEndpointer energy_endpointer_;
  int sample_rate_;
  int32_t frame_size_;
};

}  

#endif  
