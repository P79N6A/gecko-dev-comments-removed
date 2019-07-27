





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
  virtual ~TracedTaskCommon();

  void DispatchTask(int aDelayTimeMs = 0);

  void SetTLSTraceInfo();
  void GetTLSTraceInfo();
  void ClearTLSTraceInfo();

protected:
  void Init();

  
  
  
  SourceEventType mSourceEventType;
  uint64_t mSourceEventId;
  uint64_t mParentTaskId;
  uint64_t mTaskId;
  bool mIsTraceInfoInit;
};

class TracedRunnable : public TracedTaskCommon
                     , public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  TracedRunnable(nsIRunnable* aOriginalObj);

private:
  virtual ~TracedRunnable();

  nsCOMPtr<nsIRunnable> mOriginalObj;
};

class TracedTask : public TracedTaskCommon
                 , public Task
{
public:
  TracedTask(Task* aOriginalObj);
  ~TracedTask();

  virtual void Run();

private:
  Task* mOriginalObj;
};

} 
} 

#endif
