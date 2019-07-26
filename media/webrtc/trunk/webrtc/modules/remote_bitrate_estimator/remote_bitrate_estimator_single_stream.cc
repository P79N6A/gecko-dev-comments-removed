









#include <map>

#include "webrtc/modules/remote_bitrate_estimator/rate_statistics.h"
#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "webrtc/modules/remote_bitrate_estimator/overuse_detector.h"
#include "webrtc/modules/remote_bitrate_estimator/remote_rate_control.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace {
class RemoteBitrateEstimatorSingleStream : public RemoteBitrateEstimator {
 public:
  RemoteBitrateEstimatorSingleStream(RemoteBitrateObserver* observer,
                                     Clock* clock,
                                     uint32_t min_bitrate_bps);
  virtual ~RemoteBitrateEstimatorSingleStream() {}

  
  
  
  
  
  virtual void IncomingPacket(int64_t arrival_time_ms,
                              int payload_size,
                              const RTPHeader& header) OVERRIDE;

  
  
  virtual int32_t Process() OVERRIDE;
  virtual int32_t TimeUntilNextProcess() OVERRIDE;
  
  
  virtual void OnRttUpdate(uint32_t rtt) OVERRIDE;

  
  virtual void RemoveStream(unsigned int ssrc) OVERRIDE;

  
  
  
  virtual bool LatestEstimate(std::vector<unsigned int>* ssrcs,
                              unsigned int* bitrate_bps) const OVERRIDE;

  virtual bool GetStats(
      ReceiveBandwidthEstimatorStats* output) const OVERRIDE;

 private:
  typedef std::map<unsigned int, OveruseDetector> SsrcOveruseDetectorMap;

  
  void UpdateEstimate(int64_t time_now);

  void GetSsrcs(std::vector<unsigned int>* ssrcs) const;

  Clock* clock_;
  SsrcOveruseDetectorMap overuse_detectors_;
  RateStatistics incoming_bitrate_;
  RemoteRateControl remote_rate_;
  RemoteBitrateObserver* observer_;
  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  int64_t last_process_time_;
};

RemoteBitrateEstimatorSingleStream::RemoteBitrateEstimatorSingleStream(
    RemoteBitrateObserver* observer,
    Clock* clock,
    uint32_t min_bitrate_bps)
    : clock_(clock),
      incoming_bitrate_(500, 8000),
      remote_rate_(min_bitrate_bps),
      observer_(observer),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      last_process_time_(-1) {
  assert(observer_);
}

void RemoteBitrateEstimatorSingleStream::IncomingPacket(
    int64_t arrival_time_ms,
    int payload_size,
    const RTPHeader& header) {
  uint32_t ssrc = header.ssrc;
  uint32_t rtp_timestamp = header.timestamp +
      header.extension.transmissionTimeOffset;
  CriticalSectionScoped cs(crit_sect_.get());
  SsrcOveruseDetectorMap::iterator it = overuse_detectors_.find(ssrc);
  if (it == overuse_detectors_.end()) {
    
    
    
    
    
    
    std::pair<SsrcOveruseDetectorMap::iterator, bool> insert_result =
        overuse_detectors_.insert(std::make_pair(ssrc, OveruseDetector(
            OverUseDetectorOptions())));
    it = insert_result.first;
  }
  OveruseDetector* overuse_detector = &it->second;
  incoming_bitrate_.Update(payload_size, arrival_time_ms);
  const BandwidthUsage prior_state = overuse_detector->State();
  overuse_detector->Update(payload_size, -1, rtp_timestamp, arrival_time_ms);
  if (overuse_detector->State() == kBwOverusing) {
    unsigned int incoming_bitrate = incoming_bitrate_.Rate(arrival_time_ms);
    if (prior_state != kBwOverusing ||
        remote_rate_.TimeToReduceFurther(arrival_time_ms, incoming_bitrate)) {
      
      
      
      UpdateEstimate(arrival_time_ms);
    }
  }
}

int32_t RemoteBitrateEstimatorSingleStream::Process() {
  if (TimeUntilNextProcess() > 0) {
    return 0;
  }
  UpdateEstimate(clock_->TimeInMilliseconds());
  last_process_time_ = clock_->TimeInMilliseconds();
  return 0;
}

int32_t RemoteBitrateEstimatorSingleStream::TimeUntilNextProcess() {
  if (last_process_time_ < 0) {
    return 0;
  }
  return last_process_time_ + kProcessIntervalMs - clock_->TimeInMilliseconds();
}

void RemoteBitrateEstimatorSingleStream::UpdateEstimate(int64_t time_now) {
  CriticalSectionScoped cs(crit_sect_.get());
  BandwidthUsage bw_state = kBwNormal;
  double sum_noise_var = 0.0;
  SsrcOveruseDetectorMap::iterator it = overuse_detectors_.begin();
  while (it != overuse_detectors_.end()) {
    const int64_t time_of_last_received_packet =
         it->second.time_of_last_received_packet();
    if (time_of_last_received_packet >= 0 &&
        time_now - time_of_last_received_packet > kStreamTimeOutMs) {
      
      
      overuse_detectors_.erase(it++);
    } else {
      sum_noise_var += it->second.NoiseVar();
      
      
      if (it->second.State() > bw_state) {
        bw_state = it->second.State();
      }
      ++it;
    }
  }
  
  if (overuse_detectors_.empty()) {
    remote_rate_.Reset();
    return;
  }
  double mean_noise_var = sum_noise_var /
      static_cast<double>(overuse_detectors_.size());
  const RateControlInput input(bw_state,
                               incoming_bitrate_.Rate(time_now),
                               mean_noise_var);
  const RateControlRegion region = remote_rate_.Update(&input, time_now);
  unsigned int target_bitrate = remote_rate_.UpdateBandwidthEstimate(time_now);
  if (remote_rate_.ValidEstimate()) {
    std::vector<unsigned int> ssrcs;
    GetSsrcs(&ssrcs);
    observer_->OnReceiveBitrateChanged(ssrcs, target_bitrate);
  }
  for (it = overuse_detectors_.begin(); it != overuse_detectors_.end(); ++it) {
    it->second.SetRateControlRegion(region);
  }
}

void RemoteBitrateEstimatorSingleStream::OnRttUpdate(uint32_t rtt) {
  CriticalSectionScoped cs(crit_sect_.get());
  remote_rate_.SetRtt(rtt);
}

void RemoteBitrateEstimatorSingleStream::RemoveStream(unsigned int ssrc) {
  CriticalSectionScoped cs(crit_sect_.get());
  
  overuse_detectors_.erase(ssrc);
}

bool RemoteBitrateEstimatorSingleStream::LatestEstimate(
    std::vector<unsigned int>* ssrcs,
    unsigned int* bitrate_bps) const {
  CriticalSectionScoped cs(crit_sect_.get());
  assert(bitrate_bps);
  if (!remote_rate_.ValidEstimate()) {
    return false;
  }
  GetSsrcs(ssrcs);
  if (ssrcs->empty())
    *bitrate_bps = 0;
  else
    *bitrate_bps = remote_rate_.LatestEstimate();
  return true;
}

bool RemoteBitrateEstimatorSingleStream::GetStats(
    ReceiveBandwidthEstimatorStats* output) const {
  
  return false;
}

void RemoteBitrateEstimatorSingleStream::GetSsrcs(
    std::vector<unsigned int>* ssrcs) const {
  assert(ssrcs);
  ssrcs->resize(overuse_detectors_.size());
  int i = 0;
  for (SsrcOveruseDetectorMap::const_iterator it = overuse_detectors_.begin();
      it != overuse_detectors_.end(); ++it, ++i) {
    (*ssrcs)[i] = it->first;
  }
}
}  

RemoteBitrateEstimator* RemoteBitrateEstimatorFactory::Create(
    RemoteBitrateObserver* observer,
    Clock* clock,
    uint32_t min_bitrate_bps) const {
  WEBRTC_TRACE(kTraceStateInfo, kTraceRemoteBitrateEstimator, -1,
      "RemoteBitrateEstimatorFactory: Instantiating.");
  return new RemoteBitrateEstimatorSingleStream(observer, clock,
                                                min_bitrate_bps);
}

RemoteBitrateEstimator* AbsoluteSendTimeRemoteBitrateEstimatorFactory::Create(
    RemoteBitrateObserver* observer,
    Clock* clock,
    uint32_t min_bitrate_bps) const {
  WEBRTC_TRACE(kTraceStateInfo, kTraceRemoteBitrateEstimator, -1,
      "AbsoluteSendTimeRemoteBitrateEstimatorFactory: Instantiating.");
  return new RemoteBitrateEstimatorSingleStream(observer, clock,
                                                min_bitrate_bps);
}
}  
