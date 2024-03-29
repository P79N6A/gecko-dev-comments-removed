





#include "Latency.h"
#include "nsThreadUtils.h"
#include "mozilla/Logging.h"
#include <cmath>
#include <algorithm>

#include <mozilla/Services.h>
#include <mozilla/StaticPtr.h>
#include "nsContentUtils.h"

using namespace mozilla;

const char* LatencyLogIndex2Strings[] = {
  "Audio MediaStreamTrack",
  "Video MediaStreamTrack",
  "Cubeb",
  "AudioStream",
  "NetEQ",
  "AudioCapture Base",
  "AudioCapture Samples",
  "AudioTrackInsertion",
  "MediaPipeline Audio Insertion",
  "AudioTransmit",
  "AudioReceive",
  "MediaPipelineAudioPlayout",
  "MediaStream Create",
  "AudioStream Create",
  "AudioSendRTP",
  "AudioRecvRTP"
};

static StaticRefPtr<AsyncLatencyLogger> gAsyncLogger;

PRLogModuleInfo*
GetLatencyLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("MediaLatency");
  }
  return sLog;
}


class LogEvent : public nsRunnable
{
public:
  LogEvent(AsyncLatencyLogger::LatencyLogIndex aIndex, uint64_t aID, int64_t aValue,
           TimeStamp aTimeStamp) :
    mIndex(aIndex),
    mID(aID),
    mValue(aValue),
    mTimeStamp(aTimeStamp)
  {}
  LogEvent(AsyncLatencyLogger::LatencyLogIndex aIndex, uint64_t aID, int64_t aValue) :
    mIndex(aIndex),
    mID(aID),
    mValue(aValue),
    mTimeStamp(TimeStamp())
  {}
  ~LogEvent() {}

  NS_IMETHOD Run() {
    AsyncLatencyLogger::Get(true)->WriteLog(mIndex, mID, mValue, mTimeStamp);
    return NS_OK;
  }

protected:
  AsyncLatencyLogger::LatencyLogIndex mIndex;
  uint64_t mID;
  int64_t mValue;
  TimeStamp mTimeStamp;
};

void LogLatency(AsyncLatencyLogger::LatencyLogIndex aIndex, uint64_t aID, int64_t aValue)
{
  AsyncLatencyLogger::Get()->Log(aIndex, aID, aValue);
}

void LogTime(AsyncLatencyLogger::LatencyLogIndex aIndex, uint64_t aID, int64_t aValue)
{
  TimeStamp now = TimeStamp::Now();
  AsyncLatencyLogger::Get()->Log(aIndex, aID, aValue, now);
}

void LogTime(AsyncLatencyLogger::LatencyLogIndex aIndex, uint64_t aID, int64_t aValue, TimeStamp &aTime)
{
  AsyncLatencyLogger::Get()->Log(aIndex, aID, aValue, aTime);
}

void LogTime(uint32_t aIndex, uint64_t aID, int64_t aValue)
{
  LogTime(static_cast<AsyncLatencyLogger::LatencyLogIndex>(aIndex), aID, aValue);
}
void LogTime(uint32_t aIndex, uint64_t aID, int64_t aValue, TimeStamp &aTime)
{
  LogTime(static_cast<AsyncLatencyLogger::LatencyLogIndex>(aIndex), aID, aValue, aTime);
}
void LogLatency(uint32_t aIndex, uint64_t aID, int64_t aValue)
{
  LogLatency(static_cast<AsyncLatencyLogger::LatencyLogIndex>(aIndex), aID, aValue);
}


void AsyncLatencyLogger::InitializeStatics()
{
  NS_ASSERTION(NS_IsMainThread(), "Main thread only");
  GetLatencyLog();
  gAsyncLogger = new AsyncLatencyLogger();
}


void AsyncLatencyLogger::ShutdownLogger()
{
  gAsyncLogger = nullptr;
}


AsyncLatencyLogger* AsyncLatencyLogger::Get(bool aStartTimer)
{
  
  MOZ_ASSERT(gAsyncLogger);

  if (aStartTimer) {
    gAsyncLogger->Init();
  }
  return gAsyncLogger;
}

NS_IMPL_ISUPPORTS(AsyncLatencyLogger, nsIObserver)

AsyncLatencyLogger::AsyncLatencyLogger()
  : mThread(nullptr),
    mMutex("AsyncLatencyLogger")
{
  NS_ASSERTION(NS_IsMainThread(), "Main thread only");
  nsContentUtils::RegisterShutdownObserver(this);
}

AsyncLatencyLogger::~AsyncLatencyLogger()
{
  AsyncLatencyLogger::Shutdown();
}

void AsyncLatencyLogger::Shutdown()
{
  nsContentUtils::UnregisterShutdownObserver(this);

  MutexAutoLock lock(mMutex);
  if (mThread) {
    mThread->Shutdown();
  }
  mStart = TimeStamp(); 
}

void AsyncLatencyLogger::Init()
{
  MutexAutoLock lock(mMutex);
  if (mStart.IsNull()) {
    nsresult rv = NS_NewNamedThread("Latency Logger", getter_AddRefs(mThread));
    NS_ENSURE_SUCCESS_VOID(rv);
    mStart = TimeStamp::Now();
  }
}

void AsyncLatencyLogger::GetStartTime(TimeStamp &aStart)
{
  MutexAutoLock lock(mMutex);
  aStart = mStart;
}

nsresult
AsyncLatencyLogger::Observe(nsISupports* aSubject, const char* aTopic,
                            const char16_t* aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    Shutdown();
  }
  return NS_OK;
}


void AsyncLatencyLogger::WriteLog(LatencyLogIndex aIndex, uint64_t aID, int64_t aValue,
                                  TimeStamp aTimeStamp)
{
  if (aTimeStamp.IsNull()) {
    MOZ_LOG(GetLatencyLog(), LogLevel::Debug,
      ("Latency: %s,%llu,%lld,%lld",
       LatencyLogIndex2Strings[aIndex], aID, GetTimeStamp(), aValue));
  } else {
    MOZ_LOG(GetLatencyLog(), LogLevel::Debug,
      ("Latency: %s,%llu,%lld,%lld,%lld",
       LatencyLogIndex2Strings[aIndex], aID, GetTimeStamp(), aValue,
       static_cast<int64_t>((aTimeStamp - gAsyncLogger->mStart).ToMilliseconds())));
  }
}

int64_t AsyncLatencyLogger::GetTimeStamp()
{
  TimeDuration t = TimeStamp::Now() - mStart;
  return t.ToMilliseconds();
}

void AsyncLatencyLogger::Log(LatencyLogIndex aIndex, uint64_t aID, int64_t aValue)
{
  TimeStamp null;
  Log(aIndex, aID, aValue, null);
}

void AsyncLatencyLogger::Log(LatencyLogIndex aIndex, uint64_t aID, int64_t aValue, TimeStamp &aTime)
{
  if (MOZ_LOG_TEST(GetLatencyLog(), LogLevel::Debug)) {
    nsCOMPtr<nsIRunnable> event = new LogEvent(aIndex, aID, aValue, aTime);
    if (mThread) {
      mThread->Dispatch(event, NS_DISPATCH_NORMAL);
    }
  }
}
