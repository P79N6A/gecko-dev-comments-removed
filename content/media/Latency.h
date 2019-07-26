





#ifndef MOZILLA_LATENCY_H
#define MOZILLA_LATENCY_H

#include "mozilla/TimeStamp.h"
#include "prlog.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"
#include "mozilla/Monitor.h"
#include "nsISupportsImpl.h"
#include "nsObserverService.h"

class AsyncLatencyLogger;
class LogEvent;

PRLogModuleInfo* GetLatencyLog();


class AsyncLatencyLogger : public nsIObserver
{
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

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
  
  static void ShutdownLogger();
private:
  AsyncLatencyLogger();
  virtual ~AsyncLatencyLogger();
  int64_t GetTimeStamp();
  void Init();
  
  
  void Shutdown();
  
  nsCOMPtr<nsIThread> mThread;
  
  
  
  mozilla::TimeStamp mStart;
  
  
  
  mozilla::Mutex mMutex;
};

void LogLatency(AsyncLatencyLogger::LatencyLogIndex index, uint64_t aID, int64_t value);

#endif
