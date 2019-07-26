









#include "webrtc/test/fake_network_pipe.h"

#include <assert.h>
#include <math.h>
#include <string.h>
#include <algorithm>

#include "webrtc/call.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/tick_util.h"

namespace webrtc {

const double kPi = 3.14159265;
const int kDefaultProcessIntervalMs = 30;

static int GaussianRandom(int mean_delay_ms, int standard_deviation_ms) {
  
  
  double uniform1 = (rand() + 1.0) / (RAND_MAX + 1.0);  
  double uniform2 = (rand() + 1.0) / (RAND_MAX + 1.0);  
  return static_cast<int>(mean_delay_ms + standard_deviation_ms *
                          sqrt(-2 * log(uniform1)) * cos(2 * kPi * uniform2));
}

class NetworkPacket {
 public:
  NetworkPacket(const uint8_t* data, size_t length, int64_t send_time,
      int64_t arrival_time)
      : data_(NULL),
        data_length_(length),
        send_time_(send_time),
        arrival_time_(arrival_time) {
    data_ = new uint8_t[length];
    memcpy(data_, data, length);
  }
  ~NetworkPacket() {
    delete [] data_;
  }

  uint8_t* data() const { return data_; }
  size_t data_length() const { return data_length_; }
  int64_t send_time() const { return send_time_; }
  int64_t arrival_time() const { return arrival_time_; }
  void IncrementArrivalTime(int64_t extra_delay) {
    arrival_time_+= extra_delay;
  }

 private:
  
  uint8_t* data_;
  
  size_t data_length_;
  
  const int64_t send_time_;
  
  int64_t arrival_time_;
};

FakeNetworkPipe::FakeNetworkPipe(
    const FakeNetworkPipe::Config& config)
    : lock_(CriticalSectionWrapper::CreateCriticalSection()),
      packet_receiver_(NULL),
      config_(config),
      dropped_packets_(0),
      sent_packets_(0),
      total_packet_delay_(0),
      next_process_time_(TickTime::MillisecondTimestamp()) {
}

FakeNetworkPipe::~FakeNetworkPipe() {
  while (!capacity_link_.empty()) {
    delete capacity_link_.front();
    capacity_link_.pop();
  }
  while (!delay_link_.empty()) {
    delete delay_link_.front();
    delay_link_.pop();
  }
}

void FakeNetworkPipe::SetReceiver(PacketReceiver* receiver) {
  packet_receiver_ = receiver;
}

void FakeNetworkPipe::SendPacket(const uint8_t* data, size_t data_length) {
  
  
  if (packet_receiver_ == NULL)
    return;
  CriticalSectionScoped crit(lock_.get());
  if (config_.queue_length > 0 &&
      capacity_link_.size() >= config_.queue_length) {
    
    ++dropped_packets_;
    return;
  }

  int64_t time_now = TickTime::MillisecondTimestamp();

  
  int64_t capacity_delay_ms = 0;
  if (config_.link_capacity_kbps > 0)
    capacity_delay_ms = data_length / (config_.link_capacity_kbps / 8);
  int64_t network_start_time = time_now;

  
  
  if (capacity_link_.size() > 0)
    network_start_time = capacity_link_.back()->arrival_time();

  int64_t arrival_time = network_start_time + capacity_delay_ms;
  NetworkPacket* packet = new NetworkPacket(data, data_length, time_now,
                                            arrival_time);
  capacity_link_.push(packet);
}

float FakeNetworkPipe::PercentageLoss() {
  CriticalSectionScoped crit(lock_.get());
  if (sent_packets_ == 0)
    return 0;

  return static_cast<float>(dropped_packets_) /
      (sent_packets_ + dropped_packets_);
}

int FakeNetworkPipe::AverageDelay() {
  CriticalSectionScoped crit(lock_.get());
  if (sent_packets_ == 0)
    return 0;

  return total_packet_delay_ / static_cast<int>(sent_packets_);
}

void FakeNetworkPipe::Process() {
  int64_t time_now = TickTime::MillisecondTimestamp();
  std::queue<NetworkPacket*> packets_to_deliver;
  {
    CriticalSectionScoped crit(lock_.get());
    
    while (capacity_link_.size() > 0 &&
           time_now >= capacity_link_.front()->arrival_time()) {
      
      NetworkPacket* packet = capacity_link_.front();
      capacity_link_.pop();

      
      
      int extra_delay = GaussianRandom(config_.queue_delay_ms,
                                       config_.delay_standard_deviation_ms);
      if (delay_link_.size() > 0 &&
          packet->arrival_time() + extra_delay <
          delay_link_.back()->arrival_time()) {
        extra_delay = delay_link_.back()->arrival_time() -
            packet->arrival_time();
      }
      packet->IncrementArrivalTime(extra_delay);
      if (packet->arrival_time() < next_process_time_)
        next_process_time_ = packet->arrival_time();
      delay_link_.push(packet);
    }

    
    while (delay_link_.size() > 0 &&
           time_now >= delay_link_.front()->arrival_time()) {
      
      NetworkPacket* packet = delay_link_.front();
      packets_to_deliver.push(packet);
      delay_link_.pop();
      
      
      
      total_packet_delay_ += packet->arrival_time() - packet->send_time();
    }
    sent_packets_ += packets_to_deliver.size();
  }
  while (!packets_to_deliver.empty()) {
    NetworkPacket* packet = packets_to_deliver.front();
    packets_to_deliver.pop();
    packet_receiver_->DeliverPacket(packet->data(), packet->data_length());
    delete packet;
  }
}

int FakeNetworkPipe::TimeUntilNextProcess() const {
  CriticalSectionScoped crit(lock_.get());
  if (capacity_link_.size() == 0 || delay_link_.size() == 0)
    return kDefaultProcessIntervalMs;
  return std::max(static_cast<int>(next_process_time_ -
      TickTime::MillisecondTimestamp()), 0);
}

}  
