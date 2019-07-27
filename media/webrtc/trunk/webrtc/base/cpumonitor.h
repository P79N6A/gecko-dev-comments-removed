









#ifndef WEBRTC_BASE_CPUMONITOR_H_
#define WEBRTC_BASE_CPUMONITOR_H_

#include "webrtc/base/basictypes.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/sigslot.h"
#if defined(WEBRTC_LINUX)
#include "webrtc/base/stream.h"
#endif 

namespace rtc {
class Thread;
class SystemInfo;

struct CpuStats {
  CpuStats()
      : prev_total_times_(0),
        prev_cpu_times_(0),
        prev_load_(0.f),
        prev_load_time_(0u) {
  }

  uint64 prev_total_times_;
  uint64 prev_cpu_times_;
  float prev_load_;  
  uint32 prev_load_time_;  
};


class CpuSampler {
 public:
  CpuSampler();
  ~CpuSampler();

  
  bool Init();

  
  
  void set_load_interval(int min_load_interval);

  
  float GetProcessLoad();

  
  float GetSystemLoad();

  
  int GetMaxCpus() const;

  
  int GetCurrentCpus();

  
  void set_force_fallback(bool fallback) {
#if defined(WEBRTC_WIN)
    force_fallback_ = fallback;
#endif
  }

 private:
  float UpdateCpuLoad(uint64 current_total_times,
                      uint64 current_cpu_times,
                      uint64 *prev_total_times,
                      uint64 *prev_cpu_times);
  CpuStats process_;
  CpuStats system_;
  int cpus_;
  int min_load_interval_;  
  scoped_ptr<SystemInfo> sysinfo_;
#if defined(WEBRTC_WIN)
  void* get_system_times_;
  void* nt_query_system_information_;
  bool force_fallback_;
#endif
#if defined(WEBRTC_LINUX)
  
  scoped_ptr<FileStream> sfile_;
#endif 
};


class CpuMonitor
    : public rtc::MessageHandler, public sigslot::has_slots<> {
 public:
  explicit CpuMonitor(Thread* thread);
  virtual ~CpuMonitor();
  void set_thread(Thread* thread);

  bool Start(int period_ms);
  void Stop();
  
  sigslot::signal4<int, int, float, float> SignalUpdate;

 protected:
  
  virtual void OnMessage(rtc::Message* msg);
  
  
  void OnMessageQueueDestroyed() { monitor_thread_ = NULL; }

 private:
  Thread* monitor_thread_;
  CpuSampler sampler_;
  int period_ms_;

  DISALLOW_COPY_AND_ASSIGN(CpuMonitor);
};

}  

#endif  
