





#include "nsMessageLoop.h"
#include "mozilla/WeakPtr.h"
#include "base/message_loop.h"
#include "base/task.h"
#include "nsIRunnable.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsThreadUtils.h"

using namespace mozilla;

namespace {









class MessageLoopIdleTask
  : public Task
  , public SupportsWeakPtr<MessageLoopIdleTask>
{
public:
  MOZ_DECLARE_WEAKREFERENCE_TYPENAME(MessageLoopIdleTask)
  MessageLoopIdleTask(nsIRunnable* aTask, uint32_t aEnsureRunsAfterMS);
  virtual void Run();

private:
  nsresult Init(uint32_t aEnsureRunsAfterMS);

  nsCOMPtr<nsIRunnable> mTask;
  nsCOMPtr<nsITimer> mTimer;

  virtual ~MessageLoopIdleTask() {}
};











class MessageLoopTimerCallback
  : public nsITimerCallback
{
public:
  explicit MessageLoopTimerCallback(MessageLoopIdleTask* aTask);

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

private:
  WeakPtr<MessageLoopIdleTask> mTask;

  virtual ~MessageLoopTimerCallback() {}
};

MessageLoopIdleTask::MessageLoopIdleTask(nsIRunnable* aTask,
                                         uint32_t aEnsureRunsAfterMS)
  : mTask(aTask)
{
  
  
  
  nsresult rv = Init(aEnsureRunsAfterMS);
  if (NS_FAILED(rv)) {
    NS_WARNING("Running idle task early because we couldn't initialize our timer.");
    NS_DispatchToCurrentThread(mTask);

    mTask = nullptr;
    mTimer = nullptr;
  }
}

nsresult
MessageLoopIdleTask::Init(uint32_t aEnsureRunsAfterMS)
{
  mTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (NS_WARN_IF(!mTimer)) {
    return NS_ERROR_UNEXPECTED;
  }

  nsRefPtr<MessageLoopTimerCallback> callback =
    new MessageLoopTimerCallback(this);

  return mTimer->InitWithCallback(callback, aEnsureRunsAfterMS,
                                  nsITimer::TYPE_ONE_SHOT);
}

 void
MessageLoopIdleTask::Run()
{
  
  
  

  if (mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
  }

  if (mTask) {
    mTask->Run();
    mTask = nullptr;
  }
}

MessageLoopTimerCallback::MessageLoopTimerCallback(MessageLoopIdleTask* aTask)
  : mTask(aTask)
{
}

NS_IMETHODIMP
MessageLoopTimerCallback::Notify(nsITimer* aTimer)
{
  
  
  
  NS_WARN_IF_FALSE(mTask, "This timer shouldn't have fired.");

  if (mTask) {
    mTask->Run();
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS(MessageLoopTimerCallback, nsITimerCallback)

} 

NS_IMPL_ISUPPORTS(nsMessageLoop, nsIMessageLoop)

NS_IMETHODIMP
nsMessageLoop::PostIdleTask(nsIRunnable* aTask, uint32_t aEnsureRunsAfterMS)
{
  
  
  MessageLoop::current()->PostIdleTask(FROM_HERE,
    new MessageLoopIdleTask(aTask, aEnsureRunsAfterMS));
  return NS_OK;
}

nsresult
nsMessageLoopConstructor(nsISupports* aOuter,
                         const nsIID& aIID,
                         void** aInstancePtr)
{
  if (NS_WARN_IF(aOuter)) {
    return NS_ERROR_NO_AGGREGATION;
  }
  nsISupports* messageLoop = new nsMessageLoop();
  return messageLoop->QueryInterface(aIID, aInstancePtr);
}
