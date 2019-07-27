





#if !defined(TaskDispatcher_h_)
#define TaskDispatcher_h_

#include "AbstractThread.h"

#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"

#include "nsISupportsImpl.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"

namespace mozilla {
















class TaskDispatcher
{
public:
  TaskDispatcher() {}
  virtual ~TaskDispatcher() {}

  virtual void AddStateChangeTask(AbstractThread* aThread,
                                  already_AddRefed<nsIRunnable> aRunnable) = 0;
  virtual void AddTask(AbstractThread* aThread,
                       already_AddRefed<nsIRunnable> aRunnable,
                       AbstractThread::DispatchFailureHandling aFailureHandling = AbstractThread::AssertDispatchSuccess) = 0;

#ifdef DEBUG
  void AssertIsTailDispatcherIfRequired();
#else
  void AssertIsTailDispatcherIfRequired() {}
#endif
};





class MOZ_STACK_CLASS AutoTaskDispatcher : public TaskDispatcher
{
public:
  AutoTaskDispatcher() {}
  ~AutoTaskDispatcher()
  {
    for (size_t i = 0; i < mTaskGroups.Length(); ++i) {
      UniquePtr<PerThreadTaskGroup> group(Move(mTaskGroups[i]));
      nsRefPtr<AbstractThread> thread = group->mThread;

      AbstractThread::DispatchFailureHandling failureHandling = group->mFailureHandling;
      nsCOMPtr<nsIRunnable> r = new TaskGroupRunnable(Move(group));
      thread->Dispatch(r.forget(), failureHandling);
    }
  }

  void AddStateChangeTask(AbstractThread* aThread,
                          already_AddRefed<nsIRunnable> aRunnable) override
  {
    EnsureTaskGroup(aThread).mStateChangeTasks.AppendElement(aRunnable);
  }

  void AddTask(AbstractThread* aThread,
               already_AddRefed<nsIRunnable> aRunnable,
               AbstractThread::DispatchFailureHandling aFailureHandling) override
  {
    PerThreadTaskGroup& group = EnsureTaskGroup(aThread);
    group.mRegularTasks.AppendElement(aRunnable);

    
    
    if (aFailureHandling == AbstractThread::AssertDispatchSuccess) {
      group.mFailureHandling = AbstractThread::AssertDispatchSuccess;
    }
  }

private:

  struct PerThreadTaskGroup
  {
  public:
    explicit PerThreadTaskGroup(AbstractThread* aThread)
      : mThread(aThread), mFailureHandling(AbstractThread::DontAssertDispatchSuccess)
    {
      MOZ_COUNT_CTOR(PerThreadTaskGroup);
    }

    ~PerThreadTaskGroup() { MOZ_COUNT_DTOR(PerThreadTaskGroup); }

    nsRefPtr<AbstractThread> mThread;
    nsTArray<nsCOMPtr<nsIRunnable>> mStateChangeTasks;
    nsTArray<nsCOMPtr<nsIRunnable>> mRegularTasks;
    AbstractThread::DispatchFailureHandling mFailureHandling;
  };

  class TaskGroupRunnable : public nsRunnable
  {
    public:
      explicit TaskGroupRunnable(UniquePtr<PerThreadTaskGroup>&& aTasks) : mTasks(Move(aTasks)) {}

      NS_IMETHODIMP Run()
      {
        for (size_t i = 0; i < mTasks->mStateChangeTasks.Length(); ++i) {
          mTasks->mStateChangeTasks[i]->Run();
        }

        for (size_t i = 0; i < mTasks->mRegularTasks.Length(); ++i) {
          mTasks->mRegularTasks[i]->Run();
        }

        return NS_OK;
      }

    private:
      UniquePtr<PerThreadTaskGroup> mTasks;
  };

  PerThreadTaskGroup& EnsureTaskGroup(AbstractThread* aThread)
  {
    for (size_t i = 0; i < mTaskGroups.Length(); ++i) {
      if (mTaskGroups[i]->mThread == aThread) {
        return *mTaskGroups[i];
      }
    }

    mTaskGroups.AppendElement(new PerThreadTaskGroup(aThread));
    return *mTaskGroups.LastElement();
  }

  
  nsTArray<UniquePtr<PerThreadTaskGroup>> mTaskGroups;
};



template<typename T>
class PassByRef
{
public:
  PassByRef() {}
  operator T&() { return mVal; }
private:
  T mVal;
};

} 

#endif
