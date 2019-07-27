









#include "webrtc/video_engine/overuse_frame_detector.h"

#include <assert.h>
#include <math.h>

#include <algorithm>
#include <list>
#include <map>

#include "webrtc/base/exp_filter.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {



namespace {
const int64_t kProcessIntervalMs = 5000;


const float kWeightFactor = 0.997f;

const float kWeightFactorMean = 0.98f;


const int kQuickRampUpDelayMs = 10 * 1000;

const int kStandardRampUpDelayMs = 40 * 1000;
const int kMaxRampUpDelayMs = 240 * 1000;

const double kRampUpBackoffFactor = 2.0;


const int kMaxOverusesBeforeApplyRampupDelay = 4;


const float kSampleDiffMs = 33.0f;
const float kMaxExp = 7.0f;

}  


Statistics::Statistics() :
    sum_(0.0),
    count_(0),
    filtered_samples_(new rtc::ExpFilter(kWeightFactorMean)),
    filtered_variance_(new rtc::ExpFilter(kWeightFactor)) {
  Reset();
}

void Statistics::SetOptions(const CpuOveruseOptions& options) {
  options_ = options;
}

void Statistics::Reset() {
  sum_ =  0.0;
  count_ = 0;
  filtered_variance_->Reset(kWeightFactor);
  filtered_variance_->Apply(1.0f, InitialVariance());
}

void Statistics::AddSample(float sample_ms) {
  sum_ += sample_ms;
  ++count_;

  if (count_ < static_cast<uint32_t>(options_.min_frame_samples)) {
    
    filtered_samples_->Reset(kWeightFactorMean);
    filtered_samples_->Apply(1.0f, InitialMean());
    return;
  }

  float exp = sample_ms / kSampleDiffMs;
  exp = std::min(exp, kMaxExp);
  filtered_samples_->Apply(exp, sample_ms);
  filtered_variance_->Apply(exp, (sample_ms - filtered_samples_->filtered()) *
                                 (sample_ms - filtered_samples_->filtered()));
}

float Statistics::InitialMean() const {
  if (count_ == 0)
    return 0.0;
  return sum_ / count_;
}

float Statistics::InitialVariance() const {
  
  float average_stddev = (options_.low_capture_jitter_threshold_ms +
                          options_.high_capture_jitter_threshold_ms) / 2.0f;
  return average_stddev * average_stddev;
}

float Statistics::Mean() const { return filtered_samples_->filtered(); }

float Statistics::StdDev() const {
  return sqrt(std::max(filtered_variance_->filtered(), 0.0f));
}

uint64_t Statistics::Count() const { return count_; }



class OveruseFrameDetector::EncodeTimeAvg {
 public:
  EncodeTimeAvg()
      : kWeightFactor(0.5f),
        kInitialAvgEncodeTimeMs(5.0f),
        filtered_encode_time_ms_(new rtc::ExpFilter(kWeightFactor)) {
    filtered_encode_time_ms_->Apply(1.0f, kInitialAvgEncodeTimeMs);
  }
  ~EncodeTimeAvg() {}

  void AddSample(float encode_time_ms, int64_t diff_last_sample_ms) {
    float exp =  diff_last_sample_ms / kSampleDiffMs;
    exp = std::min(exp, kMaxExp);
    filtered_encode_time_ms_->Apply(exp, encode_time_ms);
  }

  int Value() const {
    return static_cast<int>(filtered_encode_time_ms_->filtered() + 0.5);
  }

 private:
  const float kWeightFactor;
  const float kInitialAvgEncodeTimeMs;
  scoped_ptr<rtc::ExpFilter> filtered_encode_time_ms_;
};




class OveruseFrameDetector::SendProcessingUsage {
 public:
  SendProcessingUsage()
      : kWeightFactorFrameDiff(0.998f),
        kWeightFactorProcessing(0.995f),
        kInitialSampleDiffMs(40.0f),
        kMaxSampleDiffMs(45.0f),
        count_(0),
        filtered_processing_ms_(new rtc::ExpFilter(kWeightFactorProcessing)),
        filtered_frame_diff_ms_(new rtc::ExpFilter(kWeightFactorFrameDiff)) {
    Reset();
  }
  ~SendProcessingUsage() {}

  void SetOptions(const CpuOveruseOptions& options) {
    options_ = options;
  }

  void Reset() {
    count_ = 0;
    filtered_frame_diff_ms_->Reset(kWeightFactorFrameDiff);
    filtered_frame_diff_ms_->Apply(1.0f, kInitialSampleDiffMs);
    filtered_processing_ms_->Reset(kWeightFactorProcessing);
    filtered_processing_ms_->Apply(1.0f, InitialProcessingMs());
  }

  void AddCaptureSample(float sample_ms) {
    float exp = sample_ms / kSampleDiffMs;
    exp = std::min(exp, kMaxExp);
    filtered_frame_diff_ms_->Apply(exp, sample_ms);
  }

  void AddSample(float processing_ms, int64_t diff_last_sample_ms) {
    ++count_;
    float exp = diff_last_sample_ms / kSampleDiffMs;
    exp = std::min(exp, kMaxExp);
    filtered_processing_ms_->Apply(exp, processing_ms);
  }

  int Value() const {
    if (count_ < static_cast<uint32_t>(options_.min_frame_samples)) {
      return static_cast<int>(InitialUsageInPercent() + 0.5f);
    }
    float frame_diff_ms = std::max(filtered_frame_diff_ms_->filtered(), 1.0f);
    frame_diff_ms = std::min(frame_diff_ms, kMaxSampleDiffMs);
    float encode_usage_percent =
        100.0f * filtered_processing_ms_->filtered() / frame_diff_ms;
    return static_cast<int>(encode_usage_percent + 0.5);
  }

 private:
  float InitialUsageInPercent() const {
    
    return (options_.low_encode_usage_threshold_percent +
            options_.high_encode_usage_threshold_percent) / 2.0f;
  }

  float InitialProcessingMs() const {
    return InitialUsageInPercent() * kInitialSampleDiffMs / 100;
  }

  const float kWeightFactorFrameDiff;
  const float kWeightFactorProcessing;
  const float kInitialSampleDiffMs;
  const float kMaxSampleDiffMs;
  uint64_t count_;
  CpuOveruseOptions options_;
  scoped_ptr<rtc::ExpFilter> filtered_processing_ms_;
  scoped_ptr<rtc::ExpFilter> filtered_frame_diff_ms_;
};


class OveruseFrameDetector::FrameQueue {
 public:
  FrameQueue() : last_processing_time_ms_(-1) {}
  ~FrameQueue() {}

  
  
  void Start(int64_t capture_time, int64_t now) {
    const size_t kMaxSize = 90;  
    if (frame_times_.size() > kMaxSize) {
      LOG(LS_WARNING) << "Max size reached, removed oldest frame.";
      frame_times_.erase(frame_times_.begin());
    }
    if (frame_times_.find(capture_time) != frame_times_.end()) {
      
      assert(false);
      return;
    }
    frame_times_[capture_time] = now;
  }

  
  
  int End(int64_t capture_time, int64_t now) {
    std::map<int64_t, int64_t>::iterator it = frame_times_.find(capture_time);
    if (it == frame_times_.end()) {
      return -1;
    }
    
    
    
    last_processing_time_ms_ = now - (*it).second;
    frame_times_.erase(frame_times_.begin(), ++it);
    return last_processing_time_ms_;
  }

  void Reset() { frame_times_.clear(); }
  int NumFrames() const { return frame_times_.size(); }
  int last_processing_time_ms() const { return last_processing_time_ms_; }

 private:
  
  std::map<int64_t, int64_t> frame_times_;
  int last_processing_time_ms_;
};



class OveruseFrameDetector::CaptureQueueDelay {
 public:
  CaptureQueueDelay()
      : kWeightFactor(0.5f),
        delay_ms_(0),
        filtered_delay_ms_per_s_(new rtc::ExpFilter(kWeightFactor)) {
    filtered_delay_ms_per_s_->Apply(1.0f, 0.0f);
  }
  ~CaptureQueueDelay() {}

  void FrameCaptured(int64_t now) {
    const size_t kMaxSize = 200;
    if (frames_.size() > kMaxSize) {
      frames_.pop_front();
    }
    frames_.push_back(now);
  }

  void FrameProcessingStarted(int64_t now) {
    if (frames_.empty()) {
      return;
    }
    delay_ms_ = now - frames_.front();
    frames_.pop_front();
  }

  void CalculateDelayChange(int64_t diff_last_sample_ms) {
    if (diff_last_sample_ms <= 0) {
      return;
    }
    float exp = static_cast<float>(diff_last_sample_ms) / kProcessIntervalMs;
    exp = std::min(exp, kMaxExp);
    filtered_delay_ms_per_s_->Apply(exp,
                                    delay_ms_ * 1000.0f / diff_last_sample_ms);
    ClearFrames();
  }

  void ClearFrames() {
    frames_.clear();
  }

  int delay_ms() const {
    return delay_ms_;
  }

  int Value() const {
    return static_cast<int>(filtered_delay_ms_per_s_->filtered() + 0.5);
  }

 private:
  const float kWeightFactor;
  std::list<int64_t> frames_;
  int delay_ms_;
  scoped_ptr<rtc::ExpFilter> filtered_delay_ms_per_s_;
};

OveruseFrameDetector::OveruseFrameDetector(Clock* clock)
    : crit_(CriticalSectionWrapper::CreateCriticalSection()),
      observer_(NULL),
      clock_(clock),
      next_process_time_(clock_->TimeInMilliseconds()),
      num_process_times_(0),
      last_capture_time_(0),
      last_overuse_time_(0),
      checks_above_threshold_(0),
      num_overuse_detections_(0),
      last_rampup_time_(0),
      in_quick_rampup_(false),
      current_rampup_delay_ms_(kStandardRampUpDelayMs),
      num_pixels_(0),
      last_encode_sample_ms_(0),
      encode_time_(new EncodeTimeAvg()),
      usage_(new SendProcessingUsage()),
      frame_queue_(new FrameQueue()),
      last_sample_time_ms_(0),
      capture_queue_delay_(new CaptureQueueDelay()) {
}

OveruseFrameDetector::~OveruseFrameDetector() {
}

void OveruseFrameDetector::SetObserver(CpuOveruseObserver* observer) {
  CriticalSectionScoped cs(crit_.get());
  observer_ = observer;
}

void OveruseFrameDetector::SetOptions(const CpuOveruseOptions& options) {
  assert(options.min_frame_samples > 0);
  CriticalSectionScoped cs(crit_.get());
  if (options_.Equals(options)) {
    return;
  }
  options_ = options;
  capture_deltas_.SetOptions(options);
  usage_->SetOptions(options);
  ResetAll(num_pixels_);
}

int OveruseFrameDetector::CaptureQueueDelayMsPerS() const {
  CriticalSectionScoped cs(crit_.get());
  return capture_queue_delay_->delay_ms();
}

int OveruseFrameDetector::LastProcessingTimeMs() const {
  CriticalSectionScoped cs(crit_.get());
  return frame_queue_->last_processing_time_ms();
}

int OveruseFrameDetector::FramesInQueue() const {
  CriticalSectionScoped cs(crit_.get());
  return frame_queue_->NumFrames();
}

void OveruseFrameDetector::GetCpuOveruseMetrics(
    CpuOveruseMetrics* metrics) const {
  CriticalSectionScoped cs(crit_.get());
  metrics->capture_jitter_ms = static_cast<int>(capture_deltas_.StdDev() + 0.5);
  metrics->avg_encode_time_ms = encode_time_->Value();
  metrics->encode_rsd = 0;
  metrics->encode_usage_percent = usage_->Value();
  metrics->capture_queue_delay_ms_per_s = capture_queue_delay_->Value();
}

int32_t OveruseFrameDetector::TimeUntilNextProcess() {
  CriticalSectionScoped cs(crit_.get());
  return next_process_time_ - clock_->TimeInMilliseconds();
}

bool OveruseFrameDetector::FrameSizeChanged(int num_pixels) const {
  if (num_pixels != num_pixels_) {
    return true;
  }
  return false;
}

bool OveruseFrameDetector::FrameTimeoutDetected(int64_t now) const {
  if (last_capture_time_ == 0) {
    return false;
  }
  return (now - last_capture_time_) > options_.frame_timeout_interval_ms;
}

void OveruseFrameDetector::ResetAll(int num_pixels) {
  num_pixels_ = num_pixels;
  capture_deltas_.Reset();
  usage_->Reset();
  frame_queue_->Reset();
  capture_queue_delay_->ClearFrames();
  last_capture_time_ = 0;
  num_process_times_ = 0;
}

void OveruseFrameDetector::FrameCaptured(int width,
                                         int height,
                                         int64_t capture_time_ms) {
  CriticalSectionScoped cs(crit_.get());

  int64_t now = clock_->TimeInMilliseconds();
  if (FrameSizeChanged(width * height) || FrameTimeoutDetected(now)) {
    ResetAll(width * height);
  }

  if (last_capture_time_ != 0) {
    capture_deltas_.AddSample(now - last_capture_time_);
    usage_->AddCaptureSample(now - last_capture_time_);
  }
  last_capture_time_ = now;

  capture_queue_delay_->FrameCaptured(now);

  if (options_.enable_extended_processing_usage) {
    frame_queue_->Start(capture_time_ms, now);
  }
}

void OveruseFrameDetector::FrameProcessingStarted() {
  CriticalSectionScoped cs(crit_.get());
  capture_queue_delay_->FrameProcessingStarted(clock_->TimeInMilliseconds());
}

void OveruseFrameDetector::FrameEncoded(int encode_time_ms) {
  CriticalSectionScoped cs(crit_.get());
  int64_t now = clock_->TimeInMilliseconds();
  if (last_encode_sample_ms_ != 0) {
    int64_t diff_ms = now - last_encode_sample_ms_;
    encode_time_->AddSample(encode_time_ms, diff_ms);
  }
  last_encode_sample_ms_ = now;

  if (!options_.enable_extended_processing_usage) {
    AddProcessingTime(encode_time_ms);
  }
}

void OveruseFrameDetector::FrameSent(int64_t capture_time_ms) {
  CriticalSectionScoped cs(crit_.get());
  if (!options_.enable_extended_processing_usage) {
    return;
  }
  int delay_ms = frame_queue_->End(capture_time_ms,
                                   clock_->TimeInMilliseconds());
  if (delay_ms > 0) {
    AddProcessingTime(delay_ms);
  }
}

void OveruseFrameDetector::AddProcessingTime(int elapsed_ms) {
  int64_t now = clock_->TimeInMilliseconds();
  if (last_sample_time_ms_ != 0) {
    int64_t diff_ms = now - last_sample_time_ms_;
    usage_->AddSample(elapsed_ms, diff_ms);
  }
  last_sample_time_ms_ = now;
}

int32_t OveruseFrameDetector::Process() {
  CriticalSectionScoped cs(crit_.get());

  int64_t now = clock_->TimeInMilliseconds();

  
  if (now < next_process_time_)
    return 0;

  int64_t diff_ms = now - next_process_time_ + kProcessIntervalMs;
  next_process_time_ = now + kProcessIntervalMs;
  ++num_process_times_;

  capture_queue_delay_->CalculateDelayChange(diff_ms);

  if (num_process_times_ <= options_.min_process_count) {
    return 0;
  }

  if (IsOverusing()) {
    
    
    
    bool check_for_backoff = last_rampup_time_ > last_overuse_time_;
    if (check_for_backoff) {
      if (now - last_rampup_time_ < kStandardRampUpDelayMs ||
          num_overuse_detections_ > kMaxOverusesBeforeApplyRampupDelay) {
        
        current_rampup_delay_ms_ *= kRampUpBackoffFactor;
        if (current_rampup_delay_ms_ > kMaxRampUpDelayMs)
          current_rampup_delay_ms_ = kMaxRampUpDelayMs;
      } else {
        
        current_rampup_delay_ms_ = kStandardRampUpDelayMs;
      }
    }

    last_overuse_time_ = now;
    in_quick_rampup_ = false;
    checks_above_threshold_ = 0;
    ++num_overuse_detections_;

    if (observer_ != NULL)
      observer_->OveruseDetected();
  } else if (IsUnderusing(now)) {
    last_rampup_time_ = now;
    in_quick_rampup_ = true;

    if (observer_ != NULL)
      observer_->NormalUsage();
  }

  int rampup_delay =
      in_quick_rampup_ ? kQuickRampUpDelayMs : current_rampup_delay_ms_;
  LOG(LS_VERBOSE) << " Frame stats: capture avg: " << capture_deltas_.Mean()
                  << " capture stddev " << capture_deltas_.StdDev()
                  << " encode usage " << usage_->Value()
                  << " overuse detections " << num_overuse_detections_
                  << " rampup delay " << rampup_delay;
  return 0;
}

bool OveruseFrameDetector::IsOverusing() {
  bool overusing = false;
  if (options_.enable_capture_jitter_method) {
    overusing = capture_deltas_.StdDev() >=
        options_.high_capture_jitter_threshold_ms;
  } else if (options_.enable_encode_usage_method) {
    overusing = usage_->Value() >= options_.high_encode_usage_threshold_percent;
  }

  if (overusing) {
    ++checks_above_threshold_;
  } else {
    checks_above_threshold_ = 0;
  }
  return checks_above_threshold_ >= options_.high_threshold_consecutive_count;
}

bool OveruseFrameDetector::IsUnderusing(int64_t time_now) {
  int delay = in_quick_rampup_ ? kQuickRampUpDelayMs : current_rampup_delay_ms_;
  if (time_now < last_rampup_time_ + delay)
    return false;

  bool underusing = false;
  if (options_.enable_capture_jitter_method) {
    underusing = capture_deltas_.StdDev() <
        options_.low_capture_jitter_threshold_ms;
  } else if (options_.enable_encode_usage_method) {
    underusing = usage_->Value() < options_.low_encode_usage_threshold_percent;
  }
  return underusing;
}
}  
