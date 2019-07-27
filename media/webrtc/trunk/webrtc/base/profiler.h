





























#ifndef WEBRTC_BASE_PROFILER_H_
#define WEBRTC_BASE_PROFILER_H_

#include <map>
#include <string>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/sharedexclusivelock.h"


#ifndef ENABLE_PROFILING
#define ENABLE_PROFILING
#endif

#ifdef ENABLE_PROFILING

#define UV_HELPER2(x) _uv_ ## x
#define UV_HELPER(x) UV_HELPER2(x)
#define UNIQUE_VAR UV_HELPER(__LINE__)


#define PROFILE(msg) rtc::ProfilerScope UNIQUE_VAR(msg)

#define PROFILE_F() PROFILE(__FUNCTION__)

#define PROFILE_DUMP_ALL(sev) \
  rtc::Profiler::Instance()->ReportAllToLog(__FILE__, __LINE__, sev)



#define PROFILE_DUMP(sev, prefix) \
  rtc::Profiler::Instance()->ReportToLog(__FILE__, __LINE__, sev, prefix)


#define PROFILE_START(msg) rtc::Profiler::Instance()->StartEvent(msg)
#define PROFILE_STOP(msg) rtc::Profiler::Instance()->StopEvent(msg)


#undef UV_HELPER2
#undef UV_HELPER
#undef UNIQUE_VAR

#else  

#define PROFILE(msg) (void)0
#define PROFILE_F() (void)0
#define PROFILE_DUMP_ALL(sev) (void)0
#define PROFILE_DUMP(sev, prefix) (void)0
#define PROFILE_START(msg) (void)0
#define PROFILE_STOP(msg) (void)0

#endif  

namespace rtc {


class ProfilerEvent {
 public:
  ProfilerEvent();
  void Start();
  void Stop();
  void Stop(uint64 stop_time);
  double standard_deviation() const;
  double total_time() const { return total_time_; }
  double mean() const { return mean_; }
  double minimum() const { return minimum_; }
  double maximum() const { return maximum_; }
  int event_count() const { return event_count_; }
  bool is_started() const { return start_count_ > 0; }

 private:
  uint64 current_start_time_;
  double total_time_;
  double mean_;
  double sum_of_squared_differences_;
  double minimum_;
  double maximum_;
  int start_count_;
  int event_count_;
};



class Profiler {
 public:
  void StartEvent(const std::string& event_name);
  void StopEvent(const std::string& event_name);
  void ReportToLog(const char* file, int line, LoggingSeverity severity_to_use,
                   const std::string& event_prefix);
  void ReportAllToLog(const char* file, int line,
                      LoggingSeverity severity_to_use);
  const ProfilerEvent* GetEvent(const std::string& event_name) const;
  
  bool Clear();

  static Profiler* Instance();
 private:
  Profiler() {}

  typedef std::map<std::string, ProfilerEvent> EventMap;
  EventMap events_;
  mutable SharedExclusiveLock lock_;

  DISALLOW_COPY_AND_ASSIGN(Profiler);
};



class ProfilerScope {
 public:
  explicit ProfilerScope(const std::string& event_name)
      : event_name_(event_name) {
    Profiler::Instance()->StartEvent(event_name_);
  }
  ~ProfilerScope() {
    Profiler::Instance()->StopEvent(event_name_);
  }
 private:
  std::string event_name_;

  DISALLOW_COPY_AND_ASSIGN(ProfilerScope);
};

std::ostream& operator<<(std::ostream& stream,
                         const ProfilerEvent& profiler_event);

}  

#endif  
