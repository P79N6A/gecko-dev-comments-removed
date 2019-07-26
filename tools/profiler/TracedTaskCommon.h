





#ifndef TRACED_TASK_COMMON_H
#define TRACED_TASK_COMMON_H

#include "base/task.h"
#include "GeckoTaskTracer.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace tasktracer {

class TracedTaskCommon
{
public:
  TracedTaskCommon();
  virtual ~TracedTaskCommon() {}

protected:
  void Init();

  
  
  void SetTraceInfo();
  void ClearTraceInfo();

  
  
  uint64_t mTaskId;

  uint64_t mSourceEventId;
  SourceEventType mSourceEventType;
};

class TracedRunnable : public TracedTaskCommon
                     , public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  TracedRunnable(nsIRunnable* aOriginalObj);

private:
  virtual ~TracedRunnable() {}

  nsCOMPtr<nsIRunnable> mOriginalObj;
};

class TracedTask : public TracedTaskCommon
                 , public Task
{
public:
  TracedTask(Task* aOriginalObj);
  ~TracedTask()
  {
    if (mOriginalObj) {
      delete mOriginalObj;
      mOriginalObj = nullptr;
    }
  }

  virtual void Run();

private:
  Task* mOriginalObj;
};



class FakeTracedTask : public TracedTaskCommon
{
public:
  FakeTracedTask() : TracedTaskCommon() {}
  FakeTracedTask(int* aVptr);
  FakeTracedTask(const FakeTracedTask& aTask);
  void BeginFakeTracedTask();
  void EndFakeTracedTask();
};

class AutoRunFakeTracedTask
{
public:
  AutoRunFakeTracedTask(FakeTracedTask* aFakeTracedTask);
  ~AutoRunFakeTracedTask();
private:
  FakeTracedTask mFakeTracedTask;
  bool mInitialized;
};

} 
} 

#endif
