





#include "GeckoTaskTracer.h"
#include "GeckoTaskTracerImpl.h"

#include "mozilla/MathAlgorithms.h"
#include "mozilla/StaticMutex.h"
#include "mozilla/ThreadLocal.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/unused.h"

#include "nsString.h"
#include "nsThreadUtils.h"
#include "prtime.h"

#include <stdarg.h>

#if defined(__GLIBC__)

#include <sys/syscall.h>
static pid_t gettid()
{
  return (pid_t) syscall(SYS_gettid);
}
#endif


#define ENSURE_TRUE_VOID(x)   \
  do {                        \
    if (MOZ_UNLIKELY(!(x))) { \
       return;                \
    }                         \
  } while(0)


#define ENSURE_TRUE(x, ret)   \
  do {                        \
    if (MOZ_UNLIKELY(!(x))) { \
       return ret;            \
    }                         \
  } while(0)

namespace mozilla {
namespace tasktracer {

static mozilla::ThreadLocal<TraceInfo*> sTraceInfoTLS;
static mozilla::StaticMutex sMutex;



static mozilla::Atomic<bool> sStarted;
static nsTArray<nsAutoPtr<TraceInfo>>* sTraceInfos = nullptr;
static PRTime sStartTime;

static const char sJSLabelPrefix[] = "#tt#";

namespace {

static PRTime
GetTimestamp()
{
  return PR_Now() / 1000;
}

static TraceInfo*
AllocTraceInfo(int aTid)
{
  StaticMutexAutoLock lock(sMutex);

  nsAutoPtr<TraceInfo>* info = sTraceInfos->AppendElement(
                                 new TraceInfo(aTid));

  return info->get();
}

static void
SaveCurTraceInfo()
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  info->mSavedCurTraceSourceId = info->mCurTraceSourceId;
  info->mSavedCurTraceSourceType = info->mCurTraceSourceType;
  info->mSavedCurTaskId = info->mCurTaskId;
}

static void
RestoreCurTraceInfo()
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  info->mCurTraceSourceId = info->mSavedCurTraceSourceId;
  info->mCurTraceSourceType = info->mSavedCurTraceSourceType;
  info->mCurTaskId = info->mSavedCurTaskId;
}

static void
CreateSourceEvent(SourceEventType aType)
{
  
  SaveCurTraceInfo();

  
  uint64_t newId = GenNewUniqueTaskId();
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  info->mCurTraceSourceId = newId;
  info->mCurTraceSourceType = aType;
  info->mCurTaskId = newId;

  int* namePtr;
#define SOURCE_EVENT_NAME(type)         \
  case SourceEventType::type:           \
  {                                     \
    static int CreateSourceEvent##type; \
    namePtr = &CreateSourceEvent##type; \
    break;                              \
  }

  switch (aType) {
#include "SourceEventTypeMap.h"
    default:
      MOZ_CRASH("Unknown SourceEvent.");
  };
#undef CREATE_SOURCE_EVENT_NAME

  
  LogDispatch(newId, newId, newId, aType);
  LogVirtualTablePtr(newId, newId, namePtr);
  LogBegin(newId, newId);
}

static void
DestroySourceEvent()
{
  
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  LogEnd(info->mCurTraceSourceId, info->mCurTraceSourceId);

  
  RestoreCurTraceInfo();
}

inline static bool
IsStartLogging()
{
  return sStarted;
}

static void
SetLogStarted(bool aIsStartLogging)
{
  MOZ_ASSERT(aIsStartLogging != IsStartLogging());
  sStarted = aIsStartLogging;

  StaticMutexAutoLock lock(sMutex);
  if (!aIsStartLogging) {
    for (uint32_t i = 0; i < sTraceInfos->Length(); ++i) {
      (*sTraceInfos)[i]->mObsolete = true;
    }
  }
}

static void
CleanUp()
{
  SetLogStarted(false);
  StaticMutexAutoLock lock(sMutex);

  if (sTraceInfos) {
    delete sTraceInfos;
    sTraceInfos = nullptr;
  }
}

inline static void
ObsoleteCurrentTraceInfos()
{
  
  
  for (uint32_t i = 0; i < sTraceInfos->Length(); ++i) {
    (*sTraceInfos)[i]->mObsolete = true;
  }
}
} 

nsCString*
TraceInfo::AppendLog()
{
  MutexAutoLock lock(mLogsMutex);
  return mLogs.AppendElement();
}

void
TraceInfo::MoveLogsInto(TraceInfoLogsType& aResult)
{
  MutexAutoLock lock(mLogsMutex);
  aResult.MoveElementsFrom(mLogs);
}

void
InitTaskTracer(uint32_t aFlags)
{
  if (aFlags & FORKED_AFTER_NUWA) {
    ObsoleteCurrentTraceInfos();
    return;
  }

  MOZ_ASSERT(!sTraceInfos);
  sTraceInfos = new nsTArray<nsAutoPtr<TraceInfo>>();

  if (!sTraceInfoTLS.initialized()) {
    unused << sTraceInfoTLS.init();
  }
}

void
ShutdownTaskTracer()
{
  CleanUp();
}

static void
FreeTraceInfo(TraceInfo* aTraceInfo)
{
  StaticMutexAutoLock lock(sMutex);
  if (aTraceInfo) {
    sTraceInfos->RemoveElement(aTraceInfo);
  }
}

void FreeTraceInfo()
{
  FreeTraceInfo(sTraceInfoTLS.get());
}

TraceInfo*
GetOrCreateTraceInfo()
{
  ENSURE_TRUE(sTraceInfoTLS.initialized(), nullptr);
  ENSURE_TRUE(IsStartLogging(), nullptr);

  TraceInfo* info = sTraceInfoTLS.get();
  if (info && info->mObsolete) {
    
    FreeTraceInfo(info);
    info = nullptr;
  }

  if (!info) {
    info = AllocTraceInfo(gettid());
    sTraceInfoTLS.set(info);
  }

  return info;
}

uint64_t
GenNewUniqueTaskId()
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE(info, 0);

  pid_t tid = gettid();
  uint64_t taskid = ((uint64_t)tid << 32) | ++info->mLastUniqueTaskId;
  return taskid;
}

AutoSaveCurTraceInfo::AutoSaveCurTraceInfo()
{
  SaveCurTraceInfo();
}

AutoSaveCurTraceInfo::~AutoSaveCurTraceInfo()
{
  RestoreCurTraceInfo();
}

void
SetCurTraceInfo(uint64_t aSourceEventId, uint64_t aParentTaskId,
                SourceEventType aSourceEventType)
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  info->mCurTraceSourceId = aSourceEventId;
  info->mCurTaskId = aParentTaskId;
  info->mCurTraceSourceType = aSourceEventType;
}

void
GetCurTraceInfo(uint64_t* aOutSourceEventId, uint64_t* aOutParentTaskId,
                SourceEventType* aOutSourceEventType)
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  *aOutSourceEventId = info->mCurTraceSourceId;
  *aOutParentTaskId = info->mCurTaskId;
  *aOutSourceEventType = info->mCurTraceSourceType;
}

void
LogDispatch(uint64_t aTaskId, uint64_t aParentTaskId, uint64_t aSourceEventId,
            SourceEventType aSourceEventType)
{
  LogDispatch(aTaskId, aParentTaskId, aSourceEventId, aSourceEventType, 0);
}

void
LogDispatch(uint64_t aTaskId, uint64_t aParentTaskId, uint64_t aSourceEventId,
            SourceEventType aSourceEventType, int aDelayTimeMs)
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  
  
  uint64_t time = (aDelayTimeMs <= 0) ? GetTimestamp() :
                  GetTimestamp() + aDelayTimeMs;

  
  
  nsCString* log = info->AppendLog();
  if (log) {
    log->AppendPrintf("%d %lld %lld %lld %d %lld",
                      ACTION_DISPATCH, aTaskId, time, aSourceEventId,
                      aSourceEventType, aParentTaskId);
  }
}

void
LogBegin(uint64_t aTaskId, uint64_t aSourceEventId)
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  
  
  nsCString* log = info->AppendLog();
  if (log) {
    log->AppendPrintf("%d %lld %lld %d %d",
                      ACTION_BEGIN, aTaskId, GetTimestamp(), getpid(), gettid());
  }
}

void
LogEnd(uint64_t aTaskId, uint64_t aSourceEventId)
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  
  
  nsCString* log = info->AppendLog();
  if (log) {
    log->AppendPrintf("%d %lld %lld", ACTION_END, aTaskId, GetTimestamp());
  }
}

void
LogVirtualTablePtr(uint64_t aTaskId, uint64_t aSourceEventId, int* aVptr)
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  
  
  nsCString* log = info->AppendLog();
  if (log) {
    log->AppendPrintf("%d %lld %p", ACTION_GET_VTABLE, aTaskId, aVptr);
  }
}

AutoSourceEvent::AutoSourceEvent(SourceEventType aType)
{
  CreateSourceEvent(aType);
}

AutoSourceEvent::~AutoSourceEvent()
{
  DestroySourceEvent();
}

void AddLabel(const char* aFormat, ...)
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  va_list args;
  va_start(args, aFormat);
  nsAutoCString buffer;
  buffer.AppendPrintf(aFormat, args);
  va_end(args);

  
  
  nsCString* log = info->AppendLog();
  if (log) {
    log->AppendPrintf("%d %lld %lld \"%s\"", ACTION_ADD_LABEL, info->mCurTaskId,
                      GetTimestamp(), buffer.get());
  }
}



void
StartLogging()
{
  sStartTime = GetTimestamp();
  SetLogStarted(true);
}

void
StopLogging()
{
  SetLogStarted(false);
}

TraceInfoLogsType*
GetLoggedData(TimeStamp aTimeStamp)
{
  TraceInfoLogsType* result = new TraceInfoLogsType();

  
  StaticMutexAutoLock lock(sMutex);

  for (uint32_t i = 0; i < sTraceInfos->Length(); ++i) {
    (*sTraceInfos)[i]->MoveLogsInto(*result);
  }

  return result;
}

const PRTime
GetStartTime()
{
  return sStartTime;
}

const char*
GetJSLabelPrefix()
{
  return sJSLabelPrefix;
}

#undef ENSURE_TRUE_VOID
#undef ENSURE_TRUE

} 
} 
