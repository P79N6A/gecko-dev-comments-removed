



























#include "energy_endpointer.h"

#include <math.h>

namespace {


float RMS(const int16_t* samples, int num_samples) {
  int64_t ssq_int64_t = 0;
  int64_t sum_int64_t = 0;
  for (int i = 0; i < num_samples; ++i) {
    sum_int64_t += samples[i];
    ssq_int64_t += samples[i] * samples[i];
  }
  
  double sum = static_cast<double>(sum_int64_t);
  sum /= num_samples;
  double ssq = static_cast<double>(ssq_int64_t);
  return static_cast<float>(sqrt((ssq / num_samples) - (sum * sum)));
}

int64_t Secs2Usecs(float seconds) {
  return static_cast<int64_t>(0.5 + (1.0e6 * seconds));
}

float GetDecibel(float value) {
  if (value > 1.0e-100)
    return 20 * log10(value);
  return -2000.0;
}

}  

namespace mozilla {



class EnergyEndpointer::HistoryRing {
 public:
  HistoryRing() : insertion_index_(0) {}

  
  void SetRing(int size, bool initial_state);

  
  void Insert(int64_t time_us, bool decision);

  
  int64_t EndTime() const;

  
  
  
  float RingSum(float duration_sec);

 private:
  struct DecisionPoint {
    int64_t time_us;
    bool decision;
  };

  std::vector<DecisionPoint> decision_points_;
  int insertion_index_;  

  HistoryRing(const HistoryRing&);
  void operator=(const HistoryRing&);
};

void EnergyEndpointer::HistoryRing::SetRing(int size, bool initial_state) {
  insertion_index_ = 0;
  decision_points_.clear();
  DecisionPoint init = { -1, initial_state };
  decision_points_.resize(size, init);
}

void EnergyEndpointer::HistoryRing::Insert(int64_t time_us, bool decision) {
  decision_points_[insertion_index_].time_us = time_us;
  decision_points_[insertion_index_].decision = decision;
  insertion_index_ = (insertion_index_ + 1) % decision_points_.size();
}

int64_t EnergyEndpointer::HistoryRing::EndTime() const {
  int ind = insertion_index_ - 1;
  if (ind < 0)
    ind = decision_points_.size() - 1;
  return decision_points_[ind].time_us;
}

float EnergyEndpointer::HistoryRing::RingSum(float duration_sec) {
  if (!decision_points_.size())
    return 0.0;

  int64_t sum_us = 0;
  int ind = insertion_index_ - 1;
  if (ind < 0)
    ind = decision_points_.size() - 1;
  int64_t end_us = decision_points_[ind].time_us;
  bool is_on = decision_points_[ind].decision;
  int64_t start_us = end_us - static_cast<int64_t>(0.5 + (1.0e6 * duration_sec));
  if (start_us < 0)
    start_us = 0;
  size_t n_summed = 1;  
  while ((decision_points_[ind].time_us > start_us) &&
         (n_summed < decision_points_.size())) {
    --ind;
    if (ind < 0)
      ind = decision_points_.size() - 1;
    if (is_on)
      sum_us += end_us - decision_points_[ind].time_us;
    is_on = decision_points_[ind].decision;
    end_us = decision_points_[ind].time_us;
    n_summed++;
  }

  return 1.0e-6f * sum_us;  
}

EnergyEndpointer::EnergyEndpointer()
    : status_(EP_PRE_SPEECH),
      offset_confirm_dur_sec_(0),
      endpointer_time_us_(0),
      fast_update_frames_(0),
      frame_counter_(0),
      max_window_dur_(4.0),
      sample_rate_(0),
      history_(new HistoryRing()),
      decision_threshold_(0),
      estimating_environment_(false),
      noise_level_(0),
      rms_adapt_(0),
      start_lag_(0),
      end_lag_(0),
      user_input_start_time_us_(0) {
}

EnergyEndpointer::~EnergyEndpointer() {
}

int EnergyEndpointer::TimeToFrame(float time) const {
  return static_cast<int32_t>(0.5 + (time / params_.frame_period()));
}

void EnergyEndpointer::Restart(bool reset_threshold) {
  status_ = EP_PRE_SPEECH;
  user_input_start_time_us_ = 0;

  if (reset_threshold) {
    decision_threshold_ = params_.decision_threshold();
    rms_adapt_ = decision_threshold_;
    noise_level_ = params_.decision_threshold() / 2.0f;
    frame_counter_ = 0;  
  }

  
  history_->SetRing(TimeToFrame(max_window_dur_), false);

  
  
  
  
  estimating_environment_ = false;
}

void EnergyEndpointer::Init(const EnergyEndpointerParams& params) {
  params_ = params;

  
  
  
  
  max_window_dur_ = params_.onset_window();
  if (params_.speech_on_window() > max_window_dur_)
    max_window_dur_ = params_.speech_on_window();
  if (params_.offset_window() > max_window_dur_)
    max_window_dur_ = params_.offset_window();
  Restart(true);

  offset_confirm_dur_sec_ = params_.offset_window() -
                            params_.offset_confirm_dur();
  if (offset_confirm_dur_sec_ < 0.0)
    offset_confirm_dur_sec_ = 0.0;

  user_input_start_time_us_ = 0;

  
  
  
  
  estimating_environment_ = false;
  
  
  noise_level_ = params_.decision_threshold() / 2.0f;
  fast_update_frames_ =
      static_cast<int64_t>(params_.fast_update_dur() / params_.frame_period());

  frame_counter_ = 0;  

  sample_rate_ = params_.sample_rate();
  start_lag_ = static_cast<int>(sample_rate_ /
                                params_.max_fundamental_frequency());
  end_lag_ = static_cast<int>(sample_rate_ /
                              params_.min_fundamental_frequency());
}

void EnergyEndpointer::StartSession() {
  Restart(true);
}

void EnergyEndpointer::EndSession() {
  status_ = EP_POST_SPEECH;
}

void EnergyEndpointer::SetEnvironmentEstimationMode() {
  Restart(true);
  estimating_environment_ = true;
}

void EnergyEndpointer::SetUserInputMode() {
  estimating_environment_ = false;
  user_input_start_time_us_ = endpointer_time_us_;
}

void EnergyEndpointer::ProcessAudioFrame(int64_t time_us,
                                         const int16_t* samples,
                                         int num_samples,
                                         float* rms_out) {
  endpointer_time_us_ = time_us;
  float rms = RMS(samples, num_samples);

  
  
  
  
  if (!estimating_environment_) {
    bool decision = false;
    if ((endpointer_time_us_ - user_input_start_time_us_) <
        Secs2Usecs(params_.contamination_rejection_period())) {
      decision = false;
      
    } else {
      decision = (rms > decision_threshold_);
    }

    history_->Insert(endpointer_time_us_, decision);

    switch (status_) {
      case EP_PRE_SPEECH:
        if (history_->RingSum(params_.onset_window()) >
            params_.onset_detect_dur()) {
          status_ = EP_POSSIBLE_ONSET;
        }
        break;

      case EP_POSSIBLE_ONSET: {
        float tsum = history_->RingSum(params_.onset_window());
        if (tsum > params_.onset_confirm_dur()) {
          status_ = EP_SPEECH_PRESENT;
        } else {  
          if (tsum <= params_.onset_detect_dur())
            status_ = EP_PRE_SPEECH;
        }
        break;
      }

      case EP_SPEECH_PRESENT: {
        
        
        
        float on_time = history_->RingSum(params_.speech_on_window());
        if (on_time < params_.on_maintain_dur())
          status_ = EP_POSSIBLE_OFFSET;
        break;
      }

      case EP_POSSIBLE_OFFSET:
        if (history_->RingSum(params_.offset_window()) <=
            offset_confirm_dur_sec_) {
          
          
          
          status_ = EP_PRE_SPEECH;  
        } else {  
          if (history_->RingSum(params_.speech_on_window()) >=
              params_.on_maintain_dur())
            status_ = EP_SPEECH_PRESENT;
        }
        break;

      default:
        break;
    }

    
    
    if ((!decision) && (status_ == EP_PRE_SPEECH)) {
      decision_threshold_ = (0.98f * decision_threshold_) + (0.02f * 2 * rms);
      rms_adapt_ = decision_threshold_;
    } else {
      
      
      
      
      
      if ((status_ == EP_SPEECH_PRESENT) && decision) {
        if (rms_adapt_ > rms) {
          rms_adapt_ = (0.99f * rms_adapt_) + (0.01f * rms);
        } else {
          rms_adapt_ = (0.95f * rms_adapt_) + (0.05f * rms);
        }
        float target_threshold = 0.3f * rms_adapt_ +  noise_level_;
        decision_threshold_ = (.90f * decision_threshold_) +
                              (0.10f * target_threshold);
      }
    }

    
    if (decision_threshold_ < params_.min_decision_threshold())
      decision_threshold_ = params_.min_decision_threshold();
  }

  
  UpdateLevels(rms);
  ++frame_counter_;

  if (rms_out)
    *rms_out = GetDecibel(rms);
}

float EnergyEndpointer::GetNoiseLevelDb() const {
  return GetDecibel(noise_level_);
}

void EnergyEndpointer::UpdateLevels(float rms) {
  
  
  if (frame_counter_ < fast_update_frames_) {
    
    
    float alpha = static_cast<float>(frame_counter_) /
        static_cast<float>(fast_update_frames_);
    noise_level_ = (alpha * noise_level_) + ((1 - alpha) * rms);
    
  } else {
    
    
    
    if (noise_level_ < rms)
      noise_level_ = (0.999f * noise_level_) + (0.001f * rms);
    else
      noise_level_ = (0.95f * noise_level_) + (0.05f * rms);
  }
  if (estimating_environment_ || (frame_counter_ < fast_update_frames_)) {
    decision_threshold_ = noise_level_ * 2; 
    
    if (decision_threshold_ < params_.min_decision_threshold())
      decision_threshold_ = params_.min_decision_threshold();
  }
}

EpStatus EnergyEndpointer::Status(int64_t* status_time)  const {
  *status_time = history_->EndTime();
  return status_;
}

}  
