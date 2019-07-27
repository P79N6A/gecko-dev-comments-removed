





#if !defined(StateWatching_h_)
#define StateWatching_h_

#include "AbstractThread.h"

#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"

#include "nsISupportsImpl.h"









































namespace mozilla {

extern PRLogModuleInfo* gStateWatchingLog;

#define WATCH_LOG(x, ...) \
  MOZ_ASSERT(gStateWatchingLog); \
  PR_LOG(gStateWatchingLog, PR_LOG_DEBUG, (x, ##__VA_ARGS__))




class AbstractWatcher
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AbstractWatcher)
  AbstractWatcher() : mDestroyed(false) {}
  bool IsDestroyed() { return mDestroyed; }
  virtual void Notify() = 0;

protected:
  virtual ~AbstractWatcher() { MOZ_ASSERT(mDestroyed); }
  bool mDestroyed;
};









class WatchTarget
{
public:
  explicit WatchTarget(const char* aName) : mName(aName) {}

  void AddWatcher(AbstractWatcher* aWatcher)
  {
    MOZ_ASSERT(!mWatchers.Contains(aWatcher));
    mWatchers.AppendElement(aWatcher);
  }

  void RemoveWatcher(AbstractWatcher* aWatcher)
  {
    MOZ_ASSERT(mWatchers.Contains(aWatcher));
    mWatchers.RemoveElement(aWatcher);
  }

protected:
  void NotifyWatchers()
  {
    WATCH_LOG("%s[%p] notifying watchers\n", mName, this);
    PruneWatchers();
    for (size_t i = 0; i < mWatchers.Length(); ++i) {
      mWatchers[i]->Notify();
    }
  }

private:
  
  
  
  
  void PruneWatchers()
  {
    for (int i = mWatchers.Length() - 1; i >= 0; --i) {
      if (mWatchers[i]->IsDestroyed()) {
        mWatchers.RemoveElementAt(i);
      }
    }
  }

  nsTArray<nsRefPtr<AbstractWatcher>> mWatchers;

protected:
  const char* mName;
};




template<typename T>
class Watchable : public WatchTarget
{
public:
  Watchable(const T& aInitialValue, const char* aName)
    : WatchTarget(aName), mValue(aInitialValue) {}

  operator const T&() const { return mValue; }
  Watchable& operator=(const T& aNewValue)
  {
    if (aNewValue != mValue) {
      mValue = aNewValue;
      NotifyWatchers();
    }

    return *this;
  }

private:
  Watchable(const Watchable& aOther); 
  Watchable& operator=(const Watchable& aOther); 

  T mValue;
};


















template <typename OwnerType>
class WatchManager
{
public:
  typedef void(OwnerType::*CallbackMethod)();
  explicit WatchManager(OwnerType* aOwner, AbstractThread* aOwnerThread)
    : mOwner(aOwner), mOwnerThread(aOwnerThread) {}

  ~WatchManager()
  {
    if (!IsShutdown()) {
      Shutdown();
    }
  }

  bool IsShutdown() const { return !mOwner; }

  
  
  void Shutdown()
  {
    MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());
    for (size_t i = 0; i < mWatchers.Length(); ++i) {
      mWatchers[i]->Destroy();
    }
    mWatchers.Clear();
    mOwner = nullptr;
  }

  void Watch(WatchTarget& aTarget, CallbackMethod aMethod)
  {
    MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());
    aTarget.AddWatcher(&EnsureWatcher(aMethod));
  }

  void Unwatch(WatchTarget& aTarget, CallbackMethod aMethod)
  {
    MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());
    PerCallbackWatcher* watcher = GetWatcher(aMethod);
    MOZ_ASSERT(watcher);
    aTarget.RemoveWatcher(watcher);
  }

  void ManualNotify(CallbackMethod aMethod)
  {
    MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());
    PerCallbackWatcher* watcher = GetWatcher(aMethod);
    MOZ_ASSERT(watcher);
    watcher->Notify();
  }

private:
  class PerCallbackWatcher : public AbstractWatcher
  {
  public:
    PerCallbackWatcher(OwnerType* aOwner, AbstractThread* aOwnerThread, CallbackMethod aMethod)
      : mOwner(aOwner), mOwnerThread(aOwnerThread), mCallbackMethod(aMethod) {}

    void Destroy()
    {
      MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());
      mDestroyed = true;
      mOwner = nullptr;
    }

    void Notify() override
    {
      MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());
      MOZ_DIAGNOSTIC_ASSERT(mOwner, "mOwner is only null after destruction, "
                                    "at which point we shouldn't be notified");
      if (mStrongRef) {
        
        return;
      }
      mStrongRef = mOwner; 

      
      nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethod(this, &PerCallbackWatcher::DoNotify);
      mOwnerThread->TailDispatcher().AddDirectTask(r.forget());
    }

    bool CallbackMethodIs(CallbackMethod aMethod) const
    {
      return mCallbackMethod == aMethod;
    }

  private:
    ~PerCallbackWatcher() {}

    void DoNotify()
    {
      MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());
      MOZ_ASSERT(mStrongRef);
      nsRefPtr<OwnerType> ref = mStrongRef.forget();
      ((*ref).*mCallbackMethod)();
    }

    OwnerType* mOwner; 
    nsRefPtr<OwnerType> mStrongRef; 
    nsRefPtr<AbstractThread> mOwnerThread;
    CallbackMethod mCallbackMethod;
  };

  PerCallbackWatcher* GetWatcher(CallbackMethod aMethod)
  {
    MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());
    for (size_t i = 0; i < mWatchers.Length(); ++i) {
      if (mWatchers[i]->CallbackMethodIs(aMethod)) {
        return mWatchers[i];
      }
    }
    return nullptr;
  }

  PerCallbackWatcher& EnsureWatcher(CallbackMethod aMethod)
  {
    MOZ_ASSERT(mOwnerThread->IsCurrentThreadIn());
    PerCallbackWatcher* watcher = GetWatcher(aMethod);
    if (watcher) {
      return *watcher;
    }
    watcher = mWatchers.AppendElement(new PerCallbackWatcher(mOwner, mOwnerThread, aMethod))->get();
    return *watcher;
  }

  nsTArray<nsRefPtr<PerCallbackWatcher>> mWatchers;
  OwnerType* mOwner;
  nsRefPtr<AbstractThread> mOwnerThread;
};

#undef WATCH_LOG

} 

#endif
