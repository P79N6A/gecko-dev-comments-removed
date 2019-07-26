









#ifndef WEBRTC_MODULES_PACED_SENDER_H_
#define WEBRTC_MODULES_PACED_SENDER_H_

#include <list>
#include <set>

#include "webrtc/modules/interface/module.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class CriticalSectionWrapper;
namespace paced_sender {
class IntervalBudget;
struct Packet;
class PacketList;
}  

class PacedSender : public Module {
 public:
  enum Priority {
    kHighPriority = 0,  
    kNormalPriority = 2,  
    kLowPriority = 3,  
  };
  
  

  class Callback {
   public:
    
    
    
    
    virtual bool TimeToSendPacket(uint32_t ssrc, uint16_t sequence_number,
                                  int64_t capture_time_ms) = 0;
    
    virtual int TimeToSendPadding(int bytes) = 0;
   protected:
    virtual ~Callback() {}
  };
  PacedSender(Callback* callback, int target_bitrate_kbps,
              float pace_multiplier);

  virtual ~PacedSender();

  
  void SetStatus(bool enable);

  bool Enabled() const;

  
  void Pause();

  
  void Resume();

  
  
  
  
  void UpdateBitrate(int target_bitrate_kbps,
                     int max_padding_bitrate_kbps,
                     int pad_up_to_bitrate_kbps);

  
  
  virtual bool SendPacket(Priority priority,
                          uint32_t ssrc,
                          uint16_t sequence_number,
                          int64_t capture_time_ms,
                          int bytes);

  
  virtual int QueueInMs() const;

  
  
  virtual int32_t TimeUntilNextProcess() OVERRIDE;

  
  virtual int32_t Process() OVERRIDE;

 private:
  
  
  bool ShouldSendNextPacket(paced_sender::PacketList** packet_list);

  
  void GetNextPacketFromList(paced_sender::PacketList* packets,
      uint32_t* ssrc, uint16_t* sequence_number, int64_t* capture_time_ms);

  
  void UpdateBytesPerInterval(uint32_t delta_time_in_ms);

  
  void UpdateMediaBytesSent(int num_bytes);

  Callback* callback_;
  const float pace_multiplier_;
  bool enabled_;
  bool paused_;
  scoped_ptr<CriticalSectionWrapper> critsect_;
  
  
  scoped_ptr<paced_sender::IntervalBudget> media_budget_;
  
  
  scoped_ptr<paced_sender::IntervalBudget> padding_budget_;
  
  
  
  scoped_ptr<paced_sender::IntervalBudget> pad_up_to_bitrate_budget_;

  TickTime time_last_update_;
  TickTime time_last_send_;
  int64_t capture_time_ms_last_queued_;
  int64_t capture_time_ms_last_sent_;

  scoped_ptr<paced_sender::PacketList> high_priority_packets_;
  scoped_ptr<paced_sender::PacketList> normal_priority_packets_;
  scoped_ptr<paced_sender::PacketList> low_priority_packets_;
};
}  
#endif
