









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TEST_BWE_TEST_FRAMEWORK_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TEST_BWE_TEST_FRAMEWORK_H_

#include <algorithm>
#include <cassert>
#include <cmath>
#include <list>
#include <numeric>
#include <string>
#include <vector>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/remote_bitrate_estimator/test/bwe_test_logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {
namespace testing {
namespace bwe {

class RateCounter;

template<typename T> class Stats {
 public:
  Stats()
      : data_(),
        last_mean_count_(0),
        last_variance_count_(0),
        last_minmax_count_(0),
        mean_(0),
        variance_(0),
        min_(0),
        max_(0) {
  }

  void Push(T data_point) {
    data_.push_back(data_point);
  }

  T GetMean() {
    if (last_mean_count_ != data_.size()) {
      last_mean_count_ = data_.size();
      mean_ = std::accumulate(data_.begin(), data_.end(), static_cast<T>(0));
      assert(last_mean_count_ != 0);
      mean_ /= static_cast<T>(last_mean_count_);
    }
    return mean_;
  }
  T GetVariance() {
    if (last_variance_count_ != data_.size()) {
      last_variance_count_ = data_.size();
      T mean = GetMean();
      variance_ = 0;
      for (typename std::vector<T>::const_iterator it = data_.begin();
          it != data_.end(); ++it) {
        T diff = (*it - mean);
        variance_ += diff * diff;
      }
      assert(last_variance_count_ != 0);
      variance_ /= static_cast<T>(last_variance_count_);
    }
    return variance_;
  }
  T GetStdDev() {
    return std::sqrt(static_cast<double>(GetVariance()));
  }
  T GetMin() {
    RefreshMinMax();
    return min_;
  }
  T GetMax() {
    RefreshMinMax();
    return max_;
  }

  void Log(const std::string& units) {
    BWE_TEST_LOGGING_LOG5("", "%f %s\t+/-%f\t[%f,%f]",
        GetMean(), units.c_str(), GetStdDev(), GetMin(), GetMax());
  }

 private:
  void RefreshMinMax() {
    if (last_minmax_count_ != data_.size()) {
      last_minmax_count_ = data_.size();
      min_ = max_ = 0;
      if (data_.empty()) {
        return;
      }
      typename std::vector<T>::const_iterator it = data_.begin();
      min_ = max_ = *it;
      while (++it != data_.end()) {
        min_ = std::min(min_, *it);
        max_ = std::max(max_, *it);
      }
    }
  }

  std::vector<T> data_;
  typename std::vector<T>::size_type last_mean_count_;
  typename std::vector<T>::size_type last_variance_count_;
  typename std::vector<T>::size_type last_minmax_count_;
  T mean_;
  T variance_;
  T min_;
  T max_;
};

class Random {
 public:
  explicit Random(uint32_t seed);

  
  float Rand();

  
  int Gaussian(int mean, int standard_deviation);

  
  

 private:
  uint32_t a_;
  uint32_t b_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(Random);
};

class Packet {
 public:
  Packet();
  Packet(int64_t send_time_us, uint32_t payload_size,
         const RTPHeader& header);
  Packet(int64_t send_time_us, uint32_t sequence_number);

  bool operator<(const Packet& rhs) const;

  int64_t creation_time_us() const { return creation_time_us_; }
  void set_send_time_us(int64_t send_time_us);
  int64_t send_time_us() const { return send_time_us_; }
  uint32_t payload_size() const { return payload_size_; }
  const RTPHeader& header() const { return header_; }

 private:
  int64_t creation_time_us_;  
  int64_t send_time_us_;   
  uint32_t payload_size_;  
  RTPHeader header_;       
};

typedef std::list<Packet> Packets;
typedef std::list<Packet>::iterator PacketsIt;
typedef std::list<Packet>::const_iterator PacketsConstIt;

bool IsTimeSorted(const Packets& packets);

class PacketProcessor;

class PacketProcessorListener {
 public:
  virtual ~PacketProcessorListener() {}

  virtual void AddPacketProcessor(PacketProcessor* processor) = 0;
  virtual void RemovePacketProcessor(PacketProcessor* processor) = 0;
};

class PacketProcessor {
 public:
  explicit PacketProcessor(PacketProcessorListener* listener);
  virtual ~PacketProcessor();

  
  
  virtual void Plot(int64_t timestamp_ms) {}

  
  
  
  virtual void RunFor(int64_t time_ms, Packets* in_out) = 0;

 private:
  PacketProcessorListener* listener_;

  DISALLOW_COPY_AND_ASSIGN(PacketProcessor);
};

class RateCounterFilter : public PacketProcessor {
 public:
  explicit RateCounterFilter(PacketProcessorListener* listener);
  RateCounterFilter(PacketProcessorListener* listener,
                    const std::string& name);
  virtual ~RateCounterFilter();

  uint32_t packets_per_second() const;
  uint32_t bits_per_second() const;

  void LogStats();
  virtual void Plot(int64_t timestamp_ms);
  virtual void RunFor(int64_t time_ms, Packets* in_out);

 private:
  scoped_ptr<RateCounter> rate_counter_;
  Stats<double> pps_stats_;
  Stats<double> kbps_stats_;
  std::string name_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(RateCounterFilter);
};

class LossFilter : public PacketProcessor {
 public:
  explicit LossFilter(PacketProcessorListener* listener);
  virtual ~LossFilter() {}

  void SetLoss(float loss_percent);
  virtual void RunFor(int64_t time_ms, Packets* in_out);

 private:
  Random random_;
  float loss_fraction_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(LossFilter);
};

class DelayFilter : public PacketProcessor {
 public:
  explicit DelayFilter(PacketProcessorListener* listener);
  virtual ~DelayFilter() {}

  void SetDelay(int64_t delay_ms);
  virtual void RunFor(int64_t time_ms, Packets* in_out);

 private:
  int64_t delay_us_;
  int64_t last_send_time_us_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(DelayFilter);
};

class JitterFilter : public PacketProcessor {
 public:
  explicit JitterFilter(PacketProcessorListener* listener);
  virtual ~JitterFilter() {}

  void SetJitter(int64_t stddev_jitter_ms);
  virtual void RunFor(int64_t time_ms, Packets* in_out);

 private:
  Random random_;
  int64_t stddev_jitter_us_;
  int64_t last_send_time_us_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(JitterFilter);
};

class ReorderFilter : public PacketProcessor {
 public:
  explicit ReorderFilter(PacketProcessorListener* listener);
  virtual ~ReorderFilter() {}

  void SetReorder(float reorder_percent);
  virtual void RunFor(int64_t time_ms, Packets* in_out);

 private:
  Random random_;
  float reorder_fraction_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(ReorderFilter);
};


class ChokeFilter : public PacketProcessor {
 public:
  explicit ChokeFilter(PacketProcessorListener* listener);
  virtual ~ChokeFilter() {}

  void SetCapacity(uint32_t kbps);
  void SetMaxDelay(int64_t max_delay_ms);
  virtual void RunFor(int64_t time_ms, Packets* in_out);

 private:
  uint32_t kbps_;
  int64_t max_delay_us_;
  int64_t last_send_time_us_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(ChokeFilter);
};

class TraceBasedDeliveryFilter : public PacketProcessor {
 public:
  explicit TraceBasedDeliveryFilter(PacketProcessorListener* listener);
  TraceBasedDeliveryFilter(PacketProcessorListener* listener,
                           const std::string& name);
  virtual ~TraceBasedDeliveryFilter();

  
  
  
  bool Init(const std::string& filename);
  virtual void Plot(int64_t timestamp_ms);
  virtual void RunFor(int64_t time_ms, Packets* in_out);

 private:
  void ProceedToNextSlot();

  typedef std::vector<int64_t> TimeList;
  TimeList delivery_times_us_;
  TimeList::const_iterator next_delivery_it_;
  int64_t local_time_us_;
  scoped_ptr<RateCounter> rate_counter_;
  std::string name_;

  DISALLOW_COPY_AND_ASSIGN(TraceBasedDeliveryFilter);
};

class PacketSender : public PacketProcessor {
 public:
  struct Feedback {
    uint32_t estimated_bps;
  };

  explicit PacketSender(PacketProcessorListener* listener);
  virtual ~PacketSender() {}

  virtual uint32_t GetCapacityKbps() const { return 0; }

  
  
  
  
  
  virtual int64_t GetFeedbackIntervalMs() const { return 1000; }
  virtual void GiveFeedback(const Feedback& feedback) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PacketSender);
};

struct PacketSenderFactory {
  PacketSenderFactory() {}
  virtual ~PacketSenderFactory() {}
  virtual PacketSender* Create() const = 0;
};

class VideoSender : public PacketSender {
 public:
  VideoSender(PacketProcessorListener* listener, float fps, uint32_t kbps,
              uint32_t ssrc, float first_frame_offset);
  virtual ~VideoSender() {}

  uint32_t max_payload_size_bytes() const { return kMaxPayloadSizeBytes; }
  uint32_t bytes_per_second() const { return bytes_per_second_; }

  virtual uint32_t GetCapacityKbps() const;

  virtual void RunFor(int64_t time_ms, Packets* in_out);

 protected:
  const uint32_t kMaxPayloadSizeBytes;
  const uint32_t kTimestampBase;
  const double frame_period_ms_;
  uint32_t bytes_per_second_;
  uint32_t frame_size_bytes_;

 private:
  double next_frame_ms_;
  double now_ms_;
  RTPHeader prototype_header_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(VideoSender);
};

class AdaptiveVideoSender : public VideoSender {
 public:
  AdaptiveVideoSender(PacketProcessorListener* listener, float fps,
                      uint32_t kbps, uint32_t ssrc, float first_frame_offset);
  virtual ~AdaptiveVideoSender() {}

  virtual int64_t GetFeedbackIntervalMs() const { return 100; }
  virtual void GiveFeedback(const Feedback& feedback);

private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AdaptiveVideoSender);
};

class VideoPacketSenderFactory : public PacketSenderFactory {
 public:
  VideoPacketSenderFactory(float fps, uint32_t kbps, uint32_t ssrc,
                           float frame_offset)
      : fps_(fps),
        kbps_(kbps),
        ssrc_(ssrc),
        frame_offset_(frame_offset) {
  }
  virtual ~VideoPacketSenderFactory() {}
  virtual PacketSender* Create() const {
    return new VideoSender(NULL, fps_, kbps_, ssrc_, frame_offset_);
  }
 protected:
  float fps_;
  uint32_t kbps_;
  uint32_t ssrc_;
  float frame_offset_;
};

class AdaptiveVideoPacketSenderFactory : public VideoPacketSenderFactory {
 public:
  AdaptiveVideoPacketSenderFactory(float fps, uint32_t kbps, uint32_t ssrc,
                                   float frame_offset)
      : VideoPacketSenderFactory(fps, kbps, ssrc, frame_offset) {}
  virtual ~AdaptiveVideoPacketSenderFactory() {}
  virtual PacketSender* Create() const {
    return new AdaptiveVideoSender(NULL, fps_, kbps_, ssrc_, frame_offset_);
  }
};

}  
}  
}  

#endif  
