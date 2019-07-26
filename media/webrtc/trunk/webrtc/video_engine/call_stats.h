









#ifndef WEBRTC_VIDEO_ENGINE_CALL_STATS_H_
#define WEBRTC_VIDEO_ENGINE_CALL_STATS_H_

#include <list>

#include "webrtc/modules/interface/module.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class CriticalSectionWrapper;
class RtcpRttObserver;



class StatsObserver {
 public:
  virtual void OnRttUpdate(uint32_t rtt) = 0;

  virtual ~StatsObserver() {}
};


class CallStats : public Module {
 public:
  friend class RtcpObserver;

  CallStats();
  ~CallStats();

  
  virtual int32_t TimeUntilNextProcess();
  virtual int32_t Process();

  
  
  RtcpRttObserver* rtcp_rtt_observer() const;

  
  void RegisterStatsObserver(StatsObserver* observer);
  void DeregisterStatsObserver(StatsObserver* observer);

 protected:
  void OnRttUpdate(uint32_t rtt);

 private:
  
  struct RttTime {
    RttTime(uint32_t new_rtt, int64_t rtt_time)
        : rtt(new_rtt), time(rtt_time) {}
    const uint32_t rtt;
    const int64_t time;
  };

  
  scoped_ptr<CriticalSectionWrapper> crit_;
  
  scoped_ptr<RtcpRttObserver> rtcp_rtt_observer_;
  
  int64_t last_process_time_;

  
  std::list<RttTime> reports_;

  
  std::list<StatsObserver*> observers_;

  DISALLOW_COPY_AND_ASSIGN(CallStats);
};

}  

#endif  
