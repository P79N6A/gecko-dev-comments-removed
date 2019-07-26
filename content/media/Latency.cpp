






#define FORCE_PR_LOG

#include "Latency.h"
#include "nsThreadUtils.h"
#include <prlog.h>
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
  "NetStat"
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
  LogEvent(AsyncLatencyLogger::LatencyLogIndex aIndex, uint64_t aID, int64_t aValue) :
    mIndex(aIndex),
    mID(aID),
    mValue(aValue)
  {}
  ~LogEvent() {}

  NS_IMETHOD Run() {
    AsyncLatencyLogger::Get(true)->WriteLog(mIndex, mID, mValue);
    return NS_OK;
  }

protected:
  AsyncLatencyLogger::LatencyLogIndex mIndex;
  uint64_t mID;
  int64_t mValue;
};

void LogLatency(AsyncLatencyLogger::LatencyLogIndex aIndex, uint64_t aID, int64_t aValue)
{
  AsyncLatencyLogger::Get()->Log(aIndex, aID, aValue);
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

NS_IMPL_ISUPPORTS1(AsyncLatencyLogger, nsIObserver)

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

nsresult
AsyncLatencyLogger::Observe(nsISupports* aSubject, const char* aTopic,
                            const PRUnichar* aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    Shutdown();
  }
  return NS_OK;
}


void AsyncLatencyLogger::WriteLog(LatencyLogIndex aIndex, uint64_t aID, int64_t aValue)
{
  PR_LOG(GetLatencyLog(), PR_LOG_DEBUG,
         ("%s,%llu,%lld.,%lld.",
          LatencyLogIndex2Strings[aIndex], aID, GetTimeStamp(), aValue));
}

int64_t AsyncLatencyLogger::GetTimeStamp()
{
  TimeDuration t = TimeStamp::Now() - mStart;
  return t.ToMilliseconds();
}

void AsyncLatencyLogger::Log(LatencyLogIndex aIndex, uint64_t aID, int64_t aValue)
{
  if (PR_LOG_TEST(GetLatencyLog(), PR_LOG_DEBUG)) {
    nsCOMPtr<nsIRunnable> event = new LogEvent(aIndex, aID, aValue);
    if (mThread) {
      mThread->Dispatch(event, NS_DISPATCH_NORMAL);
    }
  }
}
