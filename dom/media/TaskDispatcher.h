





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
                       bool aAssertDispatchSuccess = true) = 0;
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
      bool assertDispatchSuccess = group->mAssertDispatchSuccess;
      nsCOMPtr<nsIRunnable> r = new TaskGroupRunnable(Move(group));
      nsresult rv = thread->Dispatch(r.forget());
      MOZ_DIAGNOSTIC_ASSERT(!assertDispatchSuccess || NS_SUCCEEDED(rv));
      unused << rv;
    }
  }

  void AddStateChangeTask(AbstractThread* aThread,
                          already_AddRefed<nsIRunnable> aRunnable) override
  {
    EnsureTaskGroup(aThread).mStateChangeTasks.AppendElement(aRunnable);
  }

  void AddTask(AbstractThread* aThread,
               already_AddRefed<nsIRunnable> aRunnable,
               bool aAssertDispatchSuccess) override
  {
    PerThreadTaskGroup& group = EnsureTaskGroup(aThread);
    group.mRegularTasks.AppendElement(aRunnable);

    
    
    group.mAssertDispatchSuccess = group.mAssertDispatchSuccess || aAssertDispatchSuccess;
  }

private:

  struct PerThreadTaskGroup
  {
  public:
    explicit PerThreadTaskGroup(AbstractThread* aThread)
      : mThread(aThread), mAssertDispatchSuccess(false)
    {
      MOZ_COUNT_CTOR(PerThreadTaskGroup);
    }

    ~PerThreadTaskGroup() { MOZ_COUNT_DTOR(PerThreadTaskGroup); }

    nsRefPtr<AbstractThread> mThread;
    nsTArray<nsCOMPtr<nsIRunnable>> mStateChangeTasks;
    nsTArray<nsCOMPtr<nsIRunnable>> mRegularTasks;
    bool mAssertDispatchSuccess;
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

} 

#endif
