









#ifndef WEBRTC_VIDEO_ENGINE_TEST_LIBVIETEST_INCLUDE_FAKE_NETWORK_PIPE_H_
#define WEBRTC_VIDEO_ENGINE_TEST_LIBVIETEST_INCLUDE_FAKE_NETWORK_PIPE_H_

#include <queue>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;
class NetworkPacket;

class PacketReceiver {
 public:
  
  
  virtual void IncomingPacket(uint8_t* packet, int length) = 0;
  virtual ~PacketReceiver() {}
};






class FakeNetworkPipe {
 public:
  struct Configuration {
    Configuration()
        : packet_receiver(NULL),
          queue_length(0),
          queue_delay_ms(0),
          delay_standard_deviation_ms(0),
          link_capacity_kbps(0),
          loss_percent(0) {
    }
    
    PacketReceiver* packet_receiver;
    
    size_t queue_length;
    
    int queue_delay_ms;
    
    int delay_standard_deviation_ms;
    
    int link_capacity_kbps;
    
    int loss_percent;
  };

  explicit FakeNetworkPipe(const FakeNetworkPipe::Configuration& configuration);
  ~FakeNetworkPipe();

  
  void SendPacket(void* packet, int packet_length);

  
  
  void NetworkProcess();

  
  float PercentageLoss();
  int AverageDelay();
  int dropped_packets() { return dropped_packets_; }
  int sent_packets() { return sent_packets_; }

 private:
  PacketReceiver* packet_receiver_;
  scoped_ptr<CriticalSectionWrapper> link_cs_;
  std::queue<NetworkPacket*> capacity_link_;
  std::queue<NetworkPacket*> delay_link_;

  
  const size_t queue_length_;
  const int queue_delay_ms_;
  const int queue_delay_deviation_ms_;
  const int link_capacity_bytes_ms_;  

  const int loss_percent_;

  
  int dropped_packets_;
  int sent_packets_;
  int total_packet_delay_;

  DISALLOW_COPY_AND_ASSIGN(FakeNetworkPipe);
};

}  

#endif  
