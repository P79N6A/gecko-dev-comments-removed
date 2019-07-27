





#include "GeckoTaskTracerImpl.h"
#include "TracedTaskCommon.h"

namespace mozilla {
namespace tasktracer {

TracedTaskCommon::TracedTaskCommon()
  : mSourceEventId(0)
  , mSourceEventType(SourceEventType::UNKNOWN)
{
  Init();
}

void
TracedTaskCommon::Init()
{
  TraceInfo* info = GetOrCreateTraceInfo();
  NS_ENSURE_TRUE_VOID(info);

  mTaskId = GenNewUniqueTaskId();
  mSourceEventId = info->mCurTraceSourceId;
  mSourceEventType = info->mCurTraceSourceType;

  LogDispatch(mTaskId, info->mCurTaskId, mSourceEventId, mSourceEventType);
}

void
TracedTaskCommon::SetTraceInfo()
{
  TraceInfo* info = GetOrCreateTraceInfo();
  if (!info) {
    return;
  }

  info->mCurTraceSourceId = mSourceEventId;
  info->mCurTraceSourceType = mSourceEventType;
  info->mCurTaskId = mTaskId;
}

void
TracedTaskCommon::ClearTraceInfo()
{
  TraceInfo* info = GetOrCreateTraceInfo();
  if (!info) {
    return;
  }

  info->mCurTraceSourceId = 0;
  info->mCurTraceSourceType = SourceEventType::UNKNOWN;
  info->mCurTaskId = 0;
}




TracedRunnable::TracedRunnable(nsIRunnable* aOriginalObj)
  : TracedTaskCommon()
  , mOriginalObj(aOriginalObj)
{
  LogVirtualTablePtr(mTaskId, mSourceEventId, *(int**)(aOriginalObj));
}

NS_IMETHODIMP
TracedRunnable::Run()
{
  LogBegin(mTaskId, mSourceEventId);

  SetTraceInfo();
  nsresult rv = mOriginalObj->Run();
  ClearTraceInfo();

  LogEnd(mTaskId, mSourceEventId);
  return rv;
}




TracedTask::TracedTask(Task* aOriginalObj)
  : TracedTaskCommon()
  , mOriginalObj(aOriginalObj)
{
  LogVirtualTablePtr(mTaskId, mSourceEventId, *(int**)(aOriginalObj));
}

void
TracedTask::Run()
{
  LogBegin(mTaskId, mSourceEventId);

  SetTraceInfo();
  mOriginalObj->Run();
  ClearTraceInfo();

  LogEnd(mTaskId, mSourceEventId);
}

FakeTracedTask::FakeTracedTask(int* aVptr)
  : TracedTaskCommon()
{
  LogVirtualTablePtr(mTaskId, mSourceEventId, aVptr);
}

void
FakeTracedTask::BeginFakeTracedTask()
{
  LogBegin(mTaskId, mSourceEventId);
  SetTraceInfo();
}

void
FakeTracedTask::EndFakeTracedTask()
{
  ClearTraceInfo();
  LogEnd(mTaskId, mSourceEventId);
}

AutoRunFakeTracedTask::AutoRunFakeTracedTask(FakeTracedTask* aFakeTracedTask)
  : mFakeTracedTask(aFakeTracedTask)
{
  if (mFakeTracedTask) {
    mFakeTracedTask->BeginFakeTracedTask();
  }
}

AutoRunFakeTracedTask::~AutoRunFakeTracedTask()
{
  if (mFakeTracedTask) {
    mFakeTracedTask->EndFakeTracedTask();
  }
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





FakeTracedTask*
CreateFakeTracedTask(int* aVptr)
{
  nsAutoPtr<FakeTracedTask> task(new FakeTracedTask(aVptr));
  return task.forget();
}

} 
} 
