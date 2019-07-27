









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TEST_BWE_TEST_FRAMEWORK_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TEST_BWE_TEST_FRAMEWORK_H_

#include <assert.h>
#include <math.h>

#include <algorithm>
#include <list>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/pacing/include/paced_sender.h"
#include "webrtc/modules/remote_bitrate_estimator/test/bwe_test_logging.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {
namespace testing {
namespace bwe {

class DelayCapHelper;
class RateCounter;


typedef std::vector<int> FlowIds;
const FlowIds CreateFlowIds(const int *flow_ids_array, size_t num_flow_ids);

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
    return sqrt(static_cast<double>(GetVariance()));
  }
  T GetMin() {
    RefreshMinMax();
    return min_;
  }
  T GetMax() {
    RefreshMinMax();
    return max_;
  }

  std::string AsString() {
    std::stringstream ss;
    ss << (GetMean() >= 0 ? GetMean() : -1) << ", " <<
        (GetStdDev() >= 0 ? GetStdDev() : -1);
    return ss.str();
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
  Packet(int flow_id, int64_t send_time_us, uint32_t payload_size,
         const RTPHeader& header);
  Packet(int64_t send_time_us, uint32_t sequence_number);

  bool operator<(const Packet& rhs) const;

  int flow_id() const { return flow_id_; }
  int64_t creation_time_us() const { return creation_time_us_; }
  void set_send_time_us(int64_t send_time_us);
  int64_t send_time_us() const { return send_time_us_; }
  void SetAbsSendTimeMs(int64_t abs_send_time_ms);
  uint32_t payload_size() const { return payload_size_; }
  const RTPHeader& header() const { return header_; }

 private:
  int flow_id_;
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

  virtual void AddPacketProcessor(PacketProcessor* processor,
                                  bool is_sender) = 0;
  virtual void RemovePacketProcessor(PacketProcessor* processor) = 0;
};

class PacketProcessor {
 public:
  PacketProcessor(PacketProcessorListener* listener, bool is_sender);
  PacketProcessor(PacketProcessorListener* listener, const FlowIds& flow_ids,
                  bool is_sender);
  virtual ~PacketProcessor();

  
  
  virtual void Plot(int64_t timestamp_ms) {}

  
  
  
  virtual void RunFor(int64_t time_ms, Packets* in_out) = 0;

  const FlowIds& flow_ids() const { return flow_ids_; }

 private:
  PacketProcessorListener* listener_;
  FlowIds flow_ids_;

  DISALLOW_COPY_AND_ASSIGN(PacketProcessor);
};

class RateCounterFilter : public PacketProcessor {
 public:
  explicit RateCounterFilter(PacketProcessorListener* listener);
  RateCounterFilter(PacketProcessorListener* listener,
                    const std::string& name);
  RateCounterFilter(PacketProcessorListener* listener,
                    const FlowIds& flow_ids,
                    const std::string& name);
  virtual ~RateCounterFilter();

  uint32_t packets_per_second() const;
  uint32_t bits_per_second() const;

  void LogStats();
  Stats<double> GetBitrateStats() const;
  virtual void Plot(int64_t timestamp_ms);
  virtual void RunFor(int64_t time_ms, Packets* in_out);

 private:
  scoped_ptr<RateCounter> rate_counter_;
  Stats<double> packets_per_second_stats_;
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
  ChokeFilter(PacketProcessorListener* listener, const FlowIds& flow_ids);
  virtual ~ChokeFilter();

  void SetCapacity(uint32_t kbps);
  void SetMaxDelay(int max_delay_ms);
  virtual void RunFor(int64_t time_ms, Packets* in_out);

  Stats<double> GetDelayStats() const;

 private:
  uint32_t kbps_;
  int64_t last_send_time_us_;
  scoped_ptr<DelayCapHelper> delay_cap_helper_;

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

  void SetMaxDelay(int max_delay_ms);
  Stats<double> GetDelayStats() const;
  Stats<double> GetBitrateStats() const;

 private:
  void ProceedToNextSlot();

  typedef std::vector<int64_t> TimeList;
  int64_t current_offset_us_;
  TimeList delivery_times_us_;
  TimeList::const_iterator next_delivery_it_;
  int64_t local_time_us_;
  scoped_ptr<RateCounter> rate_counter_;
  std::string name_;
  scoped_ptr<DelayCapHelper> delay_cap_helper_;
  Stats<double> packets_per_second_stats_;
  Stats<double> kbps_stats_;

  DISALLOW_COPY_AND_ASSIGN(TraceBasedDeliveryFilter);
};

class PacketSender : public PacketProcessor {
 public:
  struct Feedback {
    uint32_t estimated_bps;
  };

  explicit PacketSender(PacketProcessorListener* listener);
  PacketSender(PacketProcessorListener* listener, const FlowIds& flow_ids);
  virtual ~PacketSender() {}

  virtual uint32_t GetCapacityKbps() const { return 0; }

  
  
  
  
  
  virtual int GetFeedbackIntervalMs() const { return 1000; }
  virtual void GiveFeedback(const Feedback& feedback) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PacketSender);
};

class VideoSender : public PacketSender {
 public:
  VideoSender(int flow_id,
              PacketProcessorListener* listener,
              float fps,
              uint32_t kbps,
              uint32_t ssrc,
              float first_frame_offset);
  virtual ~VideoSender() {}

  uint32_t max_payload_size_bytes() const { return kMaxPayloadSizeBytes; }
  uint32_t bytes_per_second() const { return bytes_per_second_; }

  virtual uint32_t GetCapacityKbps() const OVERRIDE;

  virtual void RunFor(int64_t time_ms, Packets* in_out) OVERRIDE;

 protected:
  virtual uint32_t NextFrameSize();
  virtual uint32_t NextPacketSize(uint32_t frame_size,
                                  uint32_t remaining_payload);

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
  AdaptiveVideoSender(int flow_id, PacketProcessorListener* listener,
                      float fps, uint32_t kbps, uint32_t ssrc,
                      float first_frame_offset);
  virtual ~AdaptiveVideoSender() {}

  virtual int GetFeedbackIntervalMs() const OVERRIDE { return 100; }
  virtual void GiveFeedback(const Feedback& feedback) OVERRIDE;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AdaptiveVideoSender);
};

class PeriodicKeyFrameSender : public AdaptiveVideoSender {
 public:
  PeriodicKeyFrameSender(int flow_id,
                         PacketProcessorListener* listener,
                         float fps,
                         uint32_t kbps,
                         uint32_t ssrc,
                         float first_frame_offset,
                         int key_frame_interval);
  virtual ~PeriodicKeyFrameSender() {}

 protected:
  virtual uint32_t NextFrameSize() OVERRIDE;
  virtual uint32_t NextPacketSize(uint32_t frame_size,
                                  uint32_t remaining_payload) OVERRIDE;

 private:
  int key_frame_interval_;
  uint32_t frame_counter_;
  int compensation_bytes_;
  int compensation_per_frame_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(PeriodicKeyFrameSender);
};

class PacedVideoSender : public PacketSender, public PacedSender::Callback {
 public:
  PacedVideoSender(PacketProcessorListener* listener,
                   uint32_t kbps, AdaptiveVideoSender* source);
  virtual ~PacedVideoSender() {}

  virtual int GetFeedbackIntervalMs() const OVERRIDE { return 100; }
  virtual void GiveFeedback(const Feedback& feedback) OVERRIDE;
  virtual void RunFor(int64_t time_ms, Packets* in_out) OVERRIDE;

  
  virtual bool TimeToSendPacket(uint32_t ssrc,
                                uint16_t sequence_number,
                                int64_t capture_time_ms,
                                bool retransmission) OVERRIDE;
  virtual int TimeToSendPadding(int bytes) OVERRIDE;

 private:
  class ProbingPacedSender : public PacedSender {
   public:
    ProbingPacedSender(Clock* clock,
                       Callback* callback,
                       int bitrate_kbps,
                       int max_bitrate_kbps,
                       int min_bitrate_kbps)
        : PacedSender(clock,
                      callback,
                      bitrate_kbps,
                      max_bitrate_kbps,
                      min_bitrate_kbps) {}

    virtual bool ProbingExperimentIsEnabled() const OVERRIDE { return true; }
  };

  void QueuePackets(Packets* batch, int64_t end_of_batch_time_us);

  static const int64_t kInitialTimeMs = 0;
  SimulatedClock clock_;
  int64_t start_of_run_ms_;
  ProbingPacedSender pacer_;
  Packets pacer_queue_;
  Packets queue_;
  AdaptiveVideoSender* source_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(PacedVideoSender);
};
}  
}  
}  

#endif  
