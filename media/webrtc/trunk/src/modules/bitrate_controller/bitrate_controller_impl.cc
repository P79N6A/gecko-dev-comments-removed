










#include "modules/bitrate_controller/bitrate_controller_impl.h"

#include <utility>

#include "modules/rtp_rtcp/interface/rtp_rtcp_defines.h"

namespace webrtc {

class RtcpBandwidthObserverImpl : public RtcpBandwidthObserver {
 public:
  explicit RtcpBandwidthObserverImpl(BitrateControllerImpl* owner)
      : owner_(owner) {
  }
  virtual ~RtcpBandwidthObserverImpl() {
  }
  
  virtual void OnReceivedEstimatedBitrate(const uint32_t bitrate) {
    owner_->OnReceivedEstimatedBitrate(bitrate);
  }
  
  virtual void OnReceivedRtcpReceiverReport(
      const uint32_t ssrc,
      const uint8_t fraction_loss,
      const uint32_t rtt,
      const uint32_t last_received_extended_high_seq_num,
      const uint32_t now_ms) {
    uint32_t number_of_packets = 0;
    std::map<uint32_t, uint32_t>::iterator it =
        ssrc_to_last_received_extended_high_seq_num_.find(ssrc);

    if (it != ssrc_to_last_received_extended_high_seq_num_.end()) {
      number_of_packets = last_received_extended_high_seq_num - it->second;
    }
    
    ssrc_to_last_received_extended_high_seq_num_[ssrc] =
        last_received_extended_high_seq_num;
    owner_->OnReceivedRtcpReceiverReport(fraction_loss, rtt, number_of_packets,
                                         now_ms);
  }
 private:
  std::map<uint32_t, uint32_t> ssrc_to_last_received_extended_high_seq_num_;
  BitrateControllerImpl* owner_;
};

BitrateController* BitrateController::CreateBitrateController() {
  return new BitrateControllerImpl();
}

BitrateControllerImpl::BitrateControllerImpl()
    : critsect_(CriticalSectionWrapper::CreateCriticalSection()) {
}

BitrateControllerImpl::~BitrateControllerImpl() {
  BitrateObserverConfList::iterator it =
      bitrate_observers_.begin();
  while (it != bitrate_observers_.end()) {
    delete it->second;
    bitrate_observers_.erase(it);
    it = bitrate_observers_.begin();
  }
  delete critsect_;
}

RtcpBandwidthObserver* BitrateControllerImpl::CreateRtcpBandwidthObserver() {
  return new RtcpBandwidthObserverImpl(this);
}

BitrateControllerImpl::BitrateObserverConfList::iterator
BitrateControllerImpl::FindObserverConfigurationPair(const BitrateObserver*
                                                     observer) {
  BitrateObserverConfList::iterator it = bitrate_observers_.begin();
  for (; it != bitrate_observers_.end(); ++it) {
    if (it->first == observer) {
      return it;
    }
  }
  return bitrate_observers_.end();
}

void BitrateControllerImpl::SetBitrateObserver(
    BitrateObserver* observer,
    const uint32_t start_bitrate,
    const uint32_t min_bitrate,
    const uint32_t max_bitrate) {
  CriticalSectionScoped cs(critsect_);

  BitrateObserverConfList::iterator it = FindObserverConfigurationPair(
      observer);

  if (it != bitrate_observers_.end()) {
    
    it->second->start_bitrate_ = start_bitrate;
    it->second->min_bitrate_ = min_bitrate;
    it->second->max_bitrate_ = max_bitrate;
  } else {
    
    bitrate_observers_.push_back(BitrateObserverConfiguration(observer,
        new BitrateConfiguration(start_bitrate, min_bitrate, max_bitrate)));
  }
  uint32_t sum_start_bitrate = 0;
  uint32_t sum_min_bitrate = 0;
  uint32_t sum_max_bitrate = 0;

  
  for (it = bitrate_observers_.begin(); it != bitrate_observers_.end(); ++it) {
    sum_start_bitrate += it->second->start_bitrate_;
    sum_min_bitrate += it->second->min_bitrate_;
    sum_max_bitrate += it->second->max_bitrate_;
  }
  
  
  
  if (bitrate_observers_.size() == 1) {
    bandwidth_estimation_.SetSendBitrate(sum_start_bitrate);
  }
  bandwidth_estimation_.SetMinMaxBitrate(sum_min_bitrate,
                                         sum_max_bitrate);
}

void BitrateControllerImpl::RemoveBitrateObserver(BitrateObserver* observer) {
  CriticalSectionScoped cs(critsect_);
  BitrateObserverConfList::iterator it = FindObserverConfigurationPair(
      observer);
  if (it != bitrate_observers_.end()) {
    delete it->second;
    bitrate_observers_.erase(it);
  }
}

void BitrateControllerImpl::OnReceivedEstimatedBitrate(const uint32_t bitrate) {
  uint32_t new_bitrate = 0;
  uint8_t fraction_lost = 0;
  uint16_t rtt = 0;
  CriticalSectionScoped cs(critsect_);
  if (bandwidth_estimation_.UpdateBandwidthEstimate(bitrate,
                                                    &new_bitrate,
                                                    &fraction_lost,
                                                    &rtt)) {
    OnNetworkChanged(new_bitrate, fraction_lost, rtt);
  }
}

void BitrateControllerImpl::OnReceivedRtcpReceiverReport(
    const uint8_t fraction_loss,
    const uint32_t rtt,
    const int number_of_packets,
    const uint32_t now_ms) {
  uint32_t new_bitrate = 0;
  uint8_t loss = fraction_loss;
  CriticalSectionScoped cs(critsect_);
  if (bandwidth_estimation_.UpdatePacketLoss(number_of_packets, rtt, now_ms,
                                             &loss, &new_bitrate)) {
    OnNetworkChanged(new_bitrate, loss, rtt);
  }
}


void BitrateControllerImpl::OnNetworkChanged(const uint32_t bitrate,
                                             const uint8_t fraction_loss,
                                             const uint32_t rtt) {
  
  uint32_t number_of_observers = bitrate_observers_.size();
  if (number_of_observers == 0) {
    return;
  }
  uint32_t sum_min_bitrates = 0;
  BitrateObserverConfList::iterator it;
  for (it = bitrate_observers_.begin(); it != bitrate_observers_.end(); ++it) {
    sum_min_bitrates += it->second->min_bitrate_;
  }
  if (bitrate <= sum_min_bitrates) {
    
    for (it = bitrate_observers_.begin(); it != bitrate_observers_.end();
        ++it) {
      it->first->OnNetworkChanged(it->second->min_bitrate_, fraction_loss,
                                  rtt);
    }
    
    bandwidth_estimation_.SetSendBitrate(sum_min_bitrates);
    return;
  }
  uint32_t bitrate_per_observer = (bitrate - sum_min_bitrates) /
      number_of_observers;
  
  ObserverSortingMap list_max_bitrates;
  for (it = bitrate_observers_.begin(); it != bitrate_observers_.end(); ++it) {
    list_max_bitrates.insert(std::pair<uint32_t, ObserverConfiguration*>(
        it->second->max_bitrate_,
        new ObserverConfiguration(it->first, it->second->min_bitrate_)));
  }
  ObserverSortingMap::iterator max_it = list_max_bitrates.begin();
  while (max_it != list_max_bitrates.end()) {
    number_of_observers--;
    uint32_t observer_allowance = max_it->second->min_bitrate_ +
        bitrate_per_observer;
    if (max_it->first < observer_allowance) {
      
      
      uint32_t remainder = observer_allowance - max_it->first;
      if (number_of_observers != 0) {
        bitrate_per_observer += remainder / number_of_observers;
      }
      max_it->second->observer_->OnNetworkChanged(max_it->first, fraction_loss,
                                                  rtt);
    } else {
      max_it->second->observer_->OnNetworkChanged(observer_allowance,
                                                  fraction_loss, rtt);
    }
    delete max_it->second;
    list_max_bitrates.erase(max_it);
    
    max_it = list_max_bitrates.begin();
  }
}

bool BitrateControllerImpl::AvailableBandwidth(uint32_t* bandwidth) const {
  return bandwidth_estimation_.AvailableBandwidth(bandwidth);
}
}  

