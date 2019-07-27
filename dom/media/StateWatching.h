





#if !defined(StateWatching_h_)
#define StateWatching_h_

#include "AbstractThread.h"

#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"

#include "nsISupportsImpl.h"









































namespace mozilla {

extern PRLogModuleInfo* gStateWatchingLog;

void EnsureStateWatchingLog();

#define WATCH_LOG(x, ...) \
  MOZ_ASSERT(gStateWatchingLog); \
  PR_LOG(gStateWatchingLog, PR_LOG_DEBUG, (x, ##__VA_ARGS__))




class AbstractWatcher
{
public:
  NS_INLINE_DECL_REFCOUNTING(AbstractWatcher)
  AbstractWatcher() : mDestroyed(false) {}
  bool IsDestroyed() { return mDestroyed; }
  virtual void Notify() = 0;

protected:
  virtual ~AbstractWatcher() {}
  virtual void CustomDestroy() {}

private:
  
  friend class WatcherHolder;
  void Destroy()
  {
    mDestroyed = true;
    CustomDestroy();
  }

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
    aWatcher->Notify();
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












class Watcher : public AbstractWatcher, public WatchTarget
{
public:
  explicit Watcher(const char* aName)
    : WatchTarget(aName), mNotifying(false) {}

  void Notify() override
  {
    if (mNotifying) {
      return;
    }
    mNotifying = true;

    
    nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethod(this, &Watcher::DoNotify);
    AbstractThread::GetCurrent()->TailDispatcher().AddDirectTask(r.forget());
  }

  void Watch(WatchTarget& aTarget) { aTarget.AddWatcher(this); }
  void Unwatch(WatchTarget& aTarget) { aTarget.RemoveWatcher(this); }

  template<typename ThisType>
  void AddWeakCallback(ThisType* aThisVal, void(ThisType::*aMethod)())
  {
    mCallbacks.AppendElement(NS_NewNonOwningRunnableMethod(aThisVal, aMethod));
  }

protected:
  void CustomDestroy() override { mCallbacks.Clear(); }

  void DoNotify()
  {
    MOZ_ASSERT(mNotifying);
    mNotifying = false;

    
    NotifyWatchers();

    for (size_t i = 0; i < mCallbacks.Length(); ++i) {
      mCallbacks[i]->Run();
    }
  }

private:
  nsTArray<nsCOMPtr<nsIRunnable>> mCallbacks;

  bool mNotifying;
};





class WatcherHolder
{
public:
  explicit WatcherHolder(const char* aName) { mWatcher = new Watcher(aName); }
  operator Watcher*() { return mWatcher; }
  Watcher* operator->() { return mWatcher; }

  ~WatcherHolder()
  {
    mWatcher->Destroy();
    mWatcher = nullptr;
  }

private:
  nsRefPtr<Watcher> mWatcher;
};


#undef WATCH_LOG

} 

#endif
