





#ifndef MOZILLA_LATENCY_H
#define MOZILLA_LATENCY_H

#include "mozilla/TimeStamp.h"
#include "prlog.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"
#include "mozilla/Monitor.h"
#include "nsISupportsImpl.h"

class AsyncLatencyLogger;
class LogEvent;

PRLogModuleInfo* GetLatencyLog();


class AsyncLatencyLogger
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AsyncLatencyLogger);
public:
  enum LatencyLogIndex {
    AudioMediaStreamTrack,
    VideoMediaStreamTrack,
    Cubeb,
    AudioStream,
    NetEQ,
    _MAX_INDEX
  };
  void Log(LatencyLogIndex index, uint64_t aID, int64_t value);
  void WriteLog(LatencyLogIndex index, uint64_t aID, int64_t value);

  static AsyncLatencyLogger* Get(bool aStartTimer = false);
  static void InitializeStatics();
  static void Shutdown();
private:
  AsyncLatencyLogger();
  ~AsyncLatencyLogger();
  int64_t GetTimeStamp();
  void Init();
  
  nsCOMPtr<nsIThread> mThread;
  
  
  
  mozilla::TimeStamp mStart;
  
  
  
  mozilla::Mutex mMutex;
};

void LogLatency(AsyncLatencyLogger::LatencyLogIndex index, uint64_t aID, int64_t value);

#endif
