









#ifndef WEBRTC_VIDEO_ENGINE_CALL_STATS_H_
#define WEBRTC_VIDEO_ENGINE_CALL_STATS_H_

#include <list>

#include "webrtc/modules/interface/module.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class CallStatsObserver;
class CriticalSectionWrapper;
class RtcpRttStats;


class CallStats : public Module {
 public:
  friend class RtcpObserver;

  CallStats();
  ~CallStats();

  
  virtual int32_t TimeUntilNextProcess();
  virtual int32_t Process();

  
  
  RtcpRttStats* rtcp_rtt_stats() const;

  
  void RegisterStatsObserver(CallStatsObserver* observer);
  void DeregisterStatsObserver(CallStatsObserver* observer);

 protected:
  void OnRttUpdate(uint32_t rtt);

  uint32_t last_processed_rtt_ms() const;

 private:
  
  struct RttTime {
    RttTime(uint32_t new_rtt, int64_t rtt_time)
        : rtt(new_rtt), time(rtt_time) {}
    const uint32_t rtt;
    const int64_t time;
  };

  
  scoped_ptr<CriticalSectionWrapper> crit_;
  
  scoped_ptr<RtcpRttStats> rtcp_rtt_stats_;
  
  int64_t last_process_time_;
  
  uint32_t last_processed_rtt_ms_;

  
  std::list<RttTime> reports_;

  
  std::list<CallStatsObserver*> observers_;

  DISALLOW_COPY_AND_ASSIGN(CallStats);
};

}  

#endif  
