





#include "GeckoTaskTracerImpl.h"
#include "TracedTaskCommon.h"


#define ENSURE_TRUE_VOID(x)   \
  do {                        \
    if (MOZ_UNLIKELY(!(x))) { \
       return;                \
    }                         \
  } while(0)

namespace mozilla {
namespace tasktracer {

TracedTaskCommon::TracedTaskCommon()
  : mSourceEventType(SourceEventType::Unknown)
  , mSourceEventId(0)
  , mParentTaskId(0)
  , mTaskId(0)
  , mIsTraceInfoInit(false)
{
}

TracedTaskCommon::~TracedTaskCommon()
{
}

void
TracedTaskCommon::Init()
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  mTaskId = GenNewUniqueTaskId();
  mSourceEventId = info->mCurTraceSourceId;
  mSourceEventType = info->mCurTraceSourceType;
  mParentTaskId = info->mCurTaskId;
  mIsTraceInfoInit = true;
}

void
TracedTaskCommon::DispatchTask(int aDelayTimeMs)
{
  LogDispatch(mTaskId, mParentTaskId, mSourceEventId, mSourceEventType,
              aDelayTimeMs);
}

void
TracedTaskCommon::GetTLSTraceInfo()
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  mSourceEventType = info->mCurTraceSourceType;
  mSourceEventId = info->mCurTraceSourceId;
  mTaskId = info->mCurTaskId;
  mIsTraceInfoInit = true;
}

void
TracedTaskCommon::SetTLSTraceInfo()
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  if (mIsTraceInfoInit) {
    info->mCurTraceSourceId = mSourceEventId;
    info->mCurTraceSourceType = mSourceEventType;
    info->mCurTaskId = mTaskId;
  }
}

void
TracedTaskCommon::ClearTLSTraceInfo()
{
  TraceInfo* info = GetOrCreateTraceInfo();
  ENSURE_TRUE_VOID(info);

  info->mCurTraceSourceId = 0;
  info->mCurTraceSourceType = SourceEventType::Unknown;
  info->mCurTaskId = 0;
}




TracedRunnable::TracedRunnable(nsIRunnable* aOriginalObj)
  : TracedTaskCommon()
  , mOriginalObj(aOriginalObj)
{
  Init();
  LogVirtualTablePtr(mTaskId, mSourceEventId, *(int**)(aOriginalObj));
}

TracedRunnable::~TracedRunnable()
{
}

NS_IMETHODIMP
TracedRunnable::Run()
{
  SetTLSTraceInfo();
  LogBegin(mTaskId, mSourceEventId);
  nsresult rv = mOriginalObj->Run();
  LogEnd(mTaskId, mSourceEventId);
  ClearTLSTraceInfo();

  return rv;
}




TracedTask::TracedTask(Task* aOriginalObj)
  : TracedTaskCommon()
  , mOriginalObj(aOriginalObj)
{
  Init();
  LogVirtualTablePtr(mTaskId, mSourceEventId, *(int**)(aOriginalObj));
}

TracedTask::~TracedTask()
{
  if (mOriginalObj) {
    delete mOriginalObj;
    mOriginalObj = nullptr;
  }
}

void
TracedTask::Run()
{
  SetTLSTraceInfo();
  LogBegin(mTaskId, mSourceEventId);
  mOriginalObj->Run();
  LogEnd(mTaskId, mSourceEventId);
  ClearTLSTraceInfo();
}





already_AddRefed<nsIRunnable>
CreateTracedRunnable(nsIRunnable* aRunnable)
{
  nsCOMPtr<nsIRunnable> runnable = new TracedRunnable(aRunnable);
  return runnable.forget();
}





Task*
CreateTracedTask(Task* aTask)
{
  Task* task = new TracedTask(aTask);
  return task;
}

} 
} 
