









#include "webrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_single_stream.h"

#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {

RemoteBitrateEstimatorSingleStream::RemoteBitrateEstimatorSingleStream(
    const OverUseDetectorOptions& options,
    RemoteBitrateObserver* observer,
    Clock* clock)
    : options_(options),
      clock_(clock),
      observer_(observer),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      last_process_time_(-1) {
  assert(observer_);
}

void RemoteBitrateEstimatorSingleStream::IncomingPacket(
    unsigned int ssrc,
    int payload_size,
    int64_t arrival_time,
    uint32_t rtp_timestamp) {
  CriticalSectionScoped cs(crit_sect_.get());
  SsrcOveruseDetectorMap::iterator it = overuse_detectors_.find(ssrc);
  if (it == overuse_detectors_.end()) {
    
    
    
    
    
    
    std::pair<SsrcOveruseDetectorMap::iterator, bool> insert_result =
        overuse_detectors_.insert(std::make_pair(ssrc, OveruseDetector(
            options_)));
    it = insert_result.first;
  }
  OveruseDetector* overuse_detector = &it->second;
  incoming_bitrate_.Update(payload_size, arrival_time);
  const BandwidthUsage prior_state = overuse_detector->State();
  overuse_detector->Update(payload_size, -1, rtp_timestamp, arrival_time);
  if (overuse_detector->State() == kBwOverusing) {
    unsigned int incoming_bitrate = incoming_bitrate_.BitRate(arrival_time);
    if (prior_state != kBwOverusing ||
        remote_rate_.TimeToReduceFurther(arrival_time, incoming_bitrate)) {
      
      
      
      UpdateEstimate(arrival_time);
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
                               incoming_bitrate_.BitRate(time_now),
                               mean_noise_var);
  const RateControlRegion region = remote_rate_.Update(&input, time_now);
  unsigned int target_bitrate = remote_rate_.UpdateBandwidthEstimate(time_now);
  if (remote_rate_.ValidEstimate()) {
    std::vector<unsigned int> ssrcs;
    GetSsrcs(&ssrcs);
    observer_->OnReceiveBitrateChanged(&ssrcs, target_bitrate);
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
