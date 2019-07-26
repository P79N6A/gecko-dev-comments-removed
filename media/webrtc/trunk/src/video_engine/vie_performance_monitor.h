












#ifndef WEBRTC_VIDEO_ENGINE_VIE_PERFORMANCE_MONITOR_H_
#define WEBRTC_VIDEO_ENGINE_VIE_PERFORMANCE_MONITOR_H_

#include "system_wrappers/interface/scoped_ptr.h"
#include "typedefs.h"  
#include "video_engine/vie_defines.h"

namespace webrtc {

class CpuWrapper;
class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;
class ViEBaseObserver;

class ViEPerformanceMonitor {
 public:
  explicit ViEPerformanceMonitor(int engine_id);
  ~ViEPerformanceMonitor();

  int Init(ViEBaseObserver* vie_base_observer);
  void Terminate();
  bool ViEBaseObserverRegistered();

 protected:
  static bool ViEMonitorThreadFunction(void* obj);
  bool ViEMonitorProcess();

 private:
  const int engine_id_;
  
  CriticalSectionWrapper* pointer_cs_;
  ThreadWrapper* monitor_thread_;
  EventWrapper& monitor_event_;
  CpuWrapper* cpu_;
  ViEBaseObserver* vie_base_observer_;
};

}  

#endif  
