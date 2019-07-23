













#ifndef BASE_TRACE_EVENT_H_
#define BASE_TRACE_EVENT_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#include <string>

#include "base/lock.h"
#include "base/scoped_ptr.h"
#include "base/singleton.h"
#include "base/time.h"
#include "base/timer.h"









#define TRACE_EVENT_BEGIN(name, id, extra) \
  Singleton<base::TraceLog>::get()->Trace(name, \
                                          base::TraceLog::EVENT_BEGIN, \
                                          reinterpret_cast<const void*>(id), \
                                          extra, \
                                          __FILE__, \
                                          __LINE__)



#define TRACE_EVENT_END(name, id, extra) \
  Singleton<base::TraceLog>::get()->Trace(name, \
                                          base::TraceLog::EVENT_END, \
                                          reinterpret_cast<const void*>(id), \
                                          extra, \
                                          __FILE__, \
                                          __LINE__)


#define TRACE_EVENT_INSTANT(name, id, extra) \
  Singleton<base::TraceLog>::get()->Trace(name, \
                                          base::TraceLog::EVENT_INSTANT, \
                                          reinterpret_cast<const void*>(id), \
                                          extra, \
                                          __FILE__, \
                                          __LINE__)

namespace base {
class ProcessMetrics;
}

namespace base {

class TraceLog {
 public:
  enum EventType {
    EVENT_BEGIN,
    EVENT_END,
    EVENT_INSTANT
  };

  
  static bool IsTracing();
  
  static bool StartTracing();
  
  static void StopTracing();

  
  void Trace(const std::string& name,
             EventType type,
             const void* id,
             const std::wstring& extra,
             const char* file,
             int line);
  void Trace(const std::string& name,
             EventType type,
             const void* id,
             const std::string& extra,
             const char* file,
             int line);

 private:
  
  
  friend struct DefaultSingletonTraits<TraceLog>;

  TraceLog();
  ~TraceLog();
  bool OpenLogFile();
  void CloseLogFile();
  bool Start();
  void Stop();
  void Heartbeat();
  void Log(const std::string& msg);

  bool enabled_;
  FILE* log_file_;
  Lock file_lock_;
  TimeTicks trace_start_time_;
  scoped_ptr<base::ProcessMetrics> process_metrics_;
  RepeatingTimer<TraceLog> timer_;
};

} 

#endif
