









#include "modules/remote_bitrate_estimator/remote_bitrate_estimator_single_stream.h"

#include "system_wrappers/interface/tick_util.h"

namespace webrtc {

RemoteBitrateEstimatorSingleStream::RemoteBitrateEstimatorSingleStream(
    RemoteBitrateObserver* observer, const OverUseDetectorOptions& options)
    : options_(options),
      observer_(observer),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()) {
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
  if (prior_state != overuse_detector->State() &&
      overuse_detector->State() == kBwOverusing) {
    
    UpdateEstimate(ssrc, arrival_time);
  }
}

void RemoteBitrateEstimatorSingleStream::UpdateEstimate(unsigned int ssrc,
                                                        int64_t time_now) {
  CriticalSectionScoped cs(crit_sect_.get());
  SsrcOveruseDetectorMap::iterator it = overuse_detectors_.find(ssrc);
  if (it == overuse_detectors_.end()) {
    return;
  }
  OveruseDetector* overuse_detector = &it->second;
  const RateControlInput input(overuse_detector->State(),
                               incoming_bitrate_.BitRate(time_now),
                               overuse_detector->NoiseVar());
  const RateControlRegion region = remote_rate_.Update(&input, time_now);
  unsigned int target_bitrate = remote_rate_.UpdateBandwidthEstimate(time_now);
  if (remote_rate_.ValidEstimate()) {
    std::vector<unsigned int> ssrcs;
    GetSsrcs(&ssrcs);
    if (!ssrcs.empty()) {
      observer_->OnReceiveBitrateChanged(&ssrcs, target_bitrate);
    }
  }
  overuse_detector->SetRateControlRegion(region);
}

void RemoteBitrateEstimatorSingleStream::SetRtt(unsigned int rtt) {
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
