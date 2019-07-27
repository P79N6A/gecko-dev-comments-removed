





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

  
  
  
  virtual void AddDirectTask(already_AddRefed<nsIRunnable> aRunnable) = 0;

  
  
  
  virtual void AddStateChangeTask(AbstractThread* aThread,
                                  already_AddRefed<nsIRunnable> aRunnable) = 0;

  
  
  virtual void AddTask(AbstractThread* aThread,
                       already_AddRefed<nsIRunnable> aRunnable,
                       AbstractThread::DispatchFailureHandling aFailureHandling = AbstractThread::AssertDispatchSuccess) = 0;

  virtual bool HasTasksFor(AbstractThread* aThread) = 0;
  virtual void DrainDirectTasks() = 0;
};





class AutoTaskDispatcher : public TaskDispatcher
{
public:
  explicit AutoTaskDispatcher(bool aIsTailDispatcher = false) : mIsTailDispatcher(aIsTailDispatcher) {}
  ~AutoTaskDispatcher()
  {
    
    
    
    
    
    
    
    
    
    MOZ_ASSERT(mDirectTasks.empty());

    for (size_t i = 0; i < mTaskGroups.Length(); ++i) {
      UniquePtr<PerThreadTaskGroup> group(Move(mTaskGroups[i]));
      nsRefPtr<AbstractThread> thread = group->mThread;

      AbstractThread::DispatchFailureHandling failureHandling = group->mFailureHandling;
      AbstractThread::DispatchReason reason = mIsTailDispatcher ? AbstractThread::TailDispatch
                                                                : AbstractThread::NormalDispatch;
      nsCOMPtr<nsIRunnable> r = new TaskGroupRunnable(Move(group));
      thread->Dispatch(r.forget(), failureHandling, reason);
    }
  }

  void DrainDirectTasks() override
  {
    while (!mDirectTasks.empty()) {
      nsCOMPtr<nsIRunnable> r = mDirectTasks.front();
      mDirectTasks.pop();
      r->Run();
    }
  }

  void AddDirectTask(already_AddRefed<nsIRunnable> aRunnable) override
  {
    mDirectTasks.push(Move(aRunnable));
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

  bool HasTasksFor(AbstractThread* aThread) override
  {
    return !!GetTaskGroup(aThread) || (aThread == AbstractThread::GetCurrent() && !mDirectTasks.empty());
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

        
        
        
        
        MaybeDrainDirectTasks();

        for (size_t i = 0; i < mTasks->mRegularTasks.Length(); ++i) {
          mTasks->mRegularTasks[i]->Run();

          
          MaybeDrainDirectTasks();
        }

        return NS_OK;
      }

    private:
      void MaybeDrainDirectTasks()
      {
        AbstractThread* currentThread = AbstractThread::GetCurrent();
        if (currentThread) {
          currentThread->TailDispatcher().DrainDirectTasks();
        }
      }

      UniquePtr<PerThreadTaskGroup> mTasks;
  };

  PerThreadTaskGroup& EnsureTaskGroup(AbstractThread* aThread)
  {
    PerThreadTaskGroup* existing = GetTaskGroup(aThread);
    if (existing) {
      return *existing;
    }

    mTaskGroups.AppendElement(new PerThreadTaskGroup(aThread));
    return *mTaskGroups.LastElement();
  }

  PerThreadTaskGroup* GetTaskGroup(AbstractThread* aThread)
  {
    for (size_t i = 0; i < mTaskGroups.Length(); ++i) {
      if (mTaskGroups[i]->mThread == aThread) {
        return mTaskGroups[i].get();
      }
    }

    
    return nullptr;
  }

  
  std::queue<nsCOMPtr<nsIRunnable>> mDirectTasks;

  
  nsTArray<UniquePtr<PerThreadTaskGroup>> mTaskGroups;

  
  
  const bool mIsTailDispatcher;
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
