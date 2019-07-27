









#ifndef WEBRTC_MODULES_PACING_INCLUDE_PACED_SENDER_H_
#define WEBRTC_MODULES_PACING_INCLUDE_PACED_SENDER_H_

#include <list>
#include <set>

#include "webrtc/base/thread_annotations.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class BitrateProber;
class Clock;
class CriticalSectionWrapper;

namespace paced_sender {
class IntervalBudget;
struct Packet;
class PacketQueue;
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
    
    
    
    
    virtual bool TimeToSendPacket(uint32_t ssrc,
                                  uint16_t sequence_number,
                                  int64_t capture_time_ms,
                                  bool retransmission) = 0;
    
    
    virtual int TimeToSendPadding(int bytes) = 0;

   protected:
    virtual ~Callback() {}
  };

  static const int kDefaultMaxQueueLengthMs = 2000;
  
  static const int kDefaultInitialPaceKbps = 2000;
  
  
  
  
  
  static const float kDefaultPaceMultiplier;

  PacedSender(Clock* clock,
              Callback* callback,
              int bitrate_kbps,
              int max_bitrate_kbps,
              int min_bitrate_kbps);

  virtual ~PacedSender();

  
  void SetStatus(bool enable);

  bool Enabled() const;

  
  void Pause();

  
  void Resume();

  
  
  
  
  
  void UpdateBitrate(int bitrate_kbps,
                     int max_bitrate_kbps,
                     int min_bitrate_kbps);

  
  
  virtual bool SendPacket(Priority priority,
                          uint32_t ssrc,
                          uint16_t sequence_number,
                          int64_t capture_time_ms,
                          int bytes,
                          bool retransmission);

  
  virtual int QueueInMs() const;

  virtual size_t QueueSizePackets() const;

  
  
  virtual int ExpectedQueueTimeMs() const;

  
  
  virtual int32_t TimeUntilNextProcess() OVERRIDE;

  
  virtual int32_t Process() OVERRIDE;

 protected:
  virtual bool ProbingExperimentIsEnabled() const;

 private:
  
  void UpdateBytesPerInterval(uint32_t delta_time_in_ms)
      EXCLUSIVE_LOCKS_REQUIRED(critsect_);

  bool SendPacket(const paced_sender::Packet& packet)
      EXCLUSIVE_LOCKS_REQUIRED(critsect_);
  void SendPadding(int padding_needed) EXCLUSIVE_LOCKS_REQUIRED(critsect_);

  Clock* const clock_;
  Callback* const callback_;

  scoped_ptr<CriticalSectionWrapper> critsect_;
  bool enabled_ GUARDED_BY(critsect_);
  bool paused_ GUARDED_BY(critsect_);
  
  
  scoped_ptr<paced_sender::IntervalBudget> media_budget_ GUARDED_BY(critsect_);
  
  
  
  scoped_ptr<paced_sender::IntervalBudget> padding_budget_
      GUARDED_BY(critsect_);

  scoped_ptr<BitrateProber> prober_ GUARDED_BY(critsect_);
  int bitrate_bps_ GUARDED_BY(critsect_);

  int64_t time_last_update_us_ GUARDED_BY(critsect_);

  scoped_ptr<paced_sender::PacketQueue> packets_ GUARDED_BY(critsect_);
  uint64_t packet_counter_ GUARDED_BY(critsect_);
};
}  
#endif
