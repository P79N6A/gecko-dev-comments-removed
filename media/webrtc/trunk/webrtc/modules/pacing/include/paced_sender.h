









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

class PacedSender : public Module {
 public:
  enum Priority {
    kHighPriority = 0,  
    kNormalPriority = 2,  
    kLowPriority = 3,  
  };
  
  

  class Callback {
   public:
    
    
    
    virtual void TimeToSendPacket(uint32_t ssrc, uint16_t sequence_number,
                                  int64_t capture_time_ms) = 0;
    
    virtual void TimeToSendPadding(int bytes) = 0;
   protected:
    virtual ~Callback() {}
  };
  PacedSender(Callback* callback, int target_bitrate_kbps,
              float pace_multiplier);

  virtual ~PacedSender();

  
  void SetStatus(bool enable);

  
  void Pause();

  
  void Resume();

  
  void UpdateBitrate(int target_bitrate_kbps);

  
  
  virtual bool SendPacket(Priority priority,
                          uint32_t ssrc,
                          uint16_t sequence_number,
                          int64_t capture_time_ms,
                          int bytes);

  
  virtual int QueueInMs() const;

  
  
  virtual int32_t TimeUntilNextProcess();

  
  virtual int32_t Process();

 private:
  struct Packet {
    Packet(uint32_t ssrc, uint16_t seq_number, int64_t capture_time_ms,
           int length_in_bytes)
        : ssrc_(ssrc),
          sequence_number_(seq_number),
          capture_time_ms_(capture_time_ms),
          bytes_(length_in_bytes) {
    }
    uint32_t ssrc_;
    uint16_t sequence_number_;
    int64_t capture_time_ms_;
    int bytes_;
  };

  
  class PacketList {
   public:
    PacketList() {};

    bool empty() const;
    Packet front() const;
    void pop_front();
    void push_back(const Packet& packet);

   private:
    std::list<Packet> packet_list_;
    std::set<uint16_t> sequence_number_set_;
  };

  
  bool GetNextPacket(uint32_t* ssrc, uint16_t* sequence_number,
                     int64_t* capture_time_ms, Priority* priority,
                     bool* last_packet);

  
  void GetNextPacketFromList(PacketList* list,
      uint32_t* ssrc, uint16_t* sequence_number, int64_t* capture_time_ms,
      bool* last_packet);

  
  void UpdateBytesPerInterval(uint32_t delta_time_in_ms);

  
  void UpdateState(int num_bytes);

  Callback* callback_;
  const float pace_multiplier_;
  bool enable_;
  bool paused_;
  scoped_ptr<CriticalSectionWrapper> critsect_;
  int target_bitrate_kbytes_per_s_;
  int bytes_remaining_interval_;
  int padding_bytes_remaining_interval_;
  TickTime time_last_update_;
  TickTime time_last_send_;
  int64_t capture_time_ms_last_queued_;
  int64_t capture_time_ms_last_sent_;

  PacketList high_priority_packets_;
  PacketList normal_priority_packets_;
  PacketList low_priority_packets_;
};
}  
#endif
