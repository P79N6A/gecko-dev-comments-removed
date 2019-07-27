





#if !defined(StateMirroring_h_)
#define StateMirroring_h_

#include "MediaPromise.h"

#include "StateWatching.h"
#include "TaskDispatcher.h"

#include "mozilla/Maybe.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"

#include "prlog.h"
#include "nsISupportsImpl.h"

























namespace mozilla {




#define MIRROR_LOG(x, ...) \
  MOZ_ASSERT(gStateWatchingLog); \
  PR_LOG(gStateWatchingLog, PR_LOG_DEBUG, (x, ##__VA_ARGS__))

template<typename T> class AbstractMirror;







template<typename T>
class AbstractCanonical
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AbstractCanonical)
  AbstractCanonical(AbstractThread* aThread) : mOwnerThread(aThread) {}
  virtual void AddMirror(AbstractMirror<T>* aMirror) = 0;
  virtual void RemoveMirror(AbstractMirror<T>* aMirror) = 0;

  AbstractThread* OwnerThread() const { return mOwnerThread; }
protected:
  virtual ~AbstractCanonical() {}
  nsRefPtr<AbstractThread> mOwnerThread;
};







template<typename T>
class AbstractMirror
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AbstractMirror)
  AbstractMirror(AbstractThread* aThread) : mOwnerThread(aThread) {}
  virtual void UpdateValue(const T& aNewValue) = 0;
  virtual void NotifyDisconnected() = 0;

  AbstractThread* OwnerThread() const { return mOwnerThread; }
protected:
  virtual ~AbstractMirror() {}
  nsRefPtr<AbstractThread> mOwnerThread;
};













template<typename T>
class Canonical
{
public:
  Canonical(AbstractThread* aThread, const T& aInitialValue, const char* aName)
  {
    mImpl = new Impl(aThread, aInitialValue, aName);
  }


  ~Canonical() {}

private:
  class Impl : public AbstractCanonical<T>, public WatchTarget
  {
  public:
    using AbstractCanonical<T>::OwnerThread;

    Impl(AbstractThread* aThread, const T& aInitialValue, const char* aName)
      : AbstractCanonical<T>(aThread), WatchTarget(aName), mValue(aInitialValue)
    {
      MIRROR_LOG("%s [%p] initialized", mName, this);
      MOZ_ASSERT(aThread->RequiresTailDispatch(), "Can't get coherency without tail dispatch");
    }

    void AddMirror(AbstractMirror<T>* aMirror) override
    {
      MIRROR_LOG("%s [%p] adding mirror %p", mName, this, aMirror);
      MOZ_ASSERT(OwnerThread()->IsCurrentThreadIn());
      MOZ_ASSERT(!mMirrors.Contains(aMirror));
      mMirrors.AppendElement(aMirror);
      aMirror->OwnerThread()->Dispatch(MakeNotifier(aMirror), AbstractThread::DontAssertDispatchSuccess);
    }

    void RemoveMirror(AbstractMirror<T>* aMirror) override
    {
      MIRROR_LOG("%s [%p] removing mirror %p", mName, this, aMirror);
      MOZ_ASSERT(OwnerThread()->IsCurrentThreadIn());
      MOZ_ASSERT(mMirrors.Contains(aMirror));
      mMirrors.RemoveElement(aMirror);
    }

    void DisconnectAll()
    {
      MIRROR_LOG("%s [%p] Disconnecting all mirrors", mName, this);
      for (size_t i = 0; i < mMirrors.Length(); ++i) {
        nsCOMPtr<nsIRunnable> r =
          NS_NewRunnableMethod(mMirrors[i], &AbstractMirror<T>::NotifyDisconnected);
        mMirrors[i]->OwnerThread()->Dispatch(r.forget(), AbstractThread::DontAssertDispatchSuccess);
      }
      mMirrors.Clear();
    }

    operator const T&()
    {
      MOZ_ASSERT(OwnerThread()->IsCurrentThreadIn());
      return mValue;
    }

    void Set(const T& aNewValue)
    {
      MOZ_ASSERT(OwnerThread()->IsCurrentThreadIn());

      if (aNewValue == mValue) {
        return;
      }

      
      
      NotifyWatchers();

      
      
      bool alreadyNotifying = mInitialValue.isSome();

      
      if (mInitialValue.isNothing()) {
        mInitialValue.emplace(mValue);
      }
      mValue = aNewValue;

      
      
      
      if (!alreadyNotifying) {
        nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethod(this, &Impl::DoNotify);
        AbstractThread::GetCurrent()->TailDispatcher().AddDirectTask(r.forget());
      }
    }

    Impl& operator=(const T& aNewValue) { Set(aNewValue); return *this; }
    Impl& operator=(const Impl& aOther) { Set(aOther); return *this; }
    Impl(const Impl& aOther) = delete;

  protected:
    ~Impl() { MOZ_DIAGNOSTIC_ASSERT(mMirrors.IsEmpty()); }

  private:
    void DoNotify()
    {
      MOZ_ASSERT(OwnerThread()->IsCurrentThreadIn());
      MOZ_ASSERT(mInitialValue.isSome());
      bool same = mInitialValue.ref() == mValue;
      mInitialValue.reset();

      if (same) {
        MIRROR_LOG("%s [%p] unchanged - not sending update", mName, this);
        return;
      }

      for (size_t i = 0; i < mMirrors.Length(); ++i) {
        OwnerThread()->TailDispatcher().AddStateChangeTask(mMirrors[i]->OwnerThread(), MakeNotifier(mMirrors[i]));
      }
    }

    already_AddRefed<nsIRunnable> MakeNotifier(AbstractMirror<T>* aMirror)
    {
      nsCOMPtr<nsIRunnable> r =
        NS_NewRunnableMethodWithArg<T>(aMirror, &AbstractMirror<T>::UpdateValue, mValue);
      return r.forget();
    }

    T mValue;
    Maybe<T> mInitialValue;
    nsTArray<nsRefPtr<AbstractMirror<T>>> mMirrors;
  };
public:

  
  
  
  void DisconnectAll() { return mImpl->DisconnectAll(); }

  
  operator Impl&() { return *mImpl; }
  Impl* operator&() { return mImpl; }

  
  const T& Ref() const { return *mImpl; }
  operator const T&() const { return Ref(); }
  void Set(const T& aNewValue) { mImpl->Set(aNewValue); }
  Canonical& operator=(const T& aNewValue) { Set(aNewValue); return *this; }
  Canonical& operator=(const Canonical& aOther) { Set(aOther); return *this; }
  Canonical(const Canonical& aOther) = delete;

private:
  nsRefPtr<Impl> mImpl;
};












template<typename T>
class Mirror
{
public:
  Mirror(AbstractThread* aThread, const T& aInitialValue, const char* aName,
         AbstractCanonical<T>* aCanonical = nullptr)
  {
    mImpl = new Impl(aThread, aInitialValue, aName, aCanonical);
  }

  ~Mirror()
  {
    if (mImpl->OwnerThread()->IsCurrentThreadIn()) {
      mImpl->DisconnectIfConnected();
    } else {
      
      
      
      MOZ_DIAGNOSTIC_ASSERT(!mImpl->IsConnected());
    }
  }

private:
  class Impl : public AbstractMirror<T>, public WatchTarget
  {
  public:
    using AbstractMirror<T>::OwnerThread;

    Impl(AbstractThread* aThread, const T& aInitialValue, const char* aName,
         AbstractCanonical<T>* aCanonical)
      : AbstractMirror<T>(aThread), WatchTarget(aName), mValue(aInitialValue)
    {
      MIRROR_LOG("%s [%p] initialized", mName, this);

      if (aCanonical) {
        ConnectInternal(aCanonical);
      }
    }

    operator const T&()
    {
      MOZ_ASSERT(OwnerThread()->IsCurrentThreadIn());
      return mValue;
    }

    virtual void UpdateValue(const T& aNewValue) override
    {
      MOZ_ASSERT(OwnerThread()->IsCurrentThreadIn());
      if (mValue != aNewValue) {
        mValue = aNewValue;
        WatchTarget::NotifyWatchers();
      }
    }

    virtual void NotifyDisconnected() override
    {
      MIRROR_LOG("%s [%p] Notifed of disconnection from %p", mName, this, mCanonical.get());
      MOZ_ASSERT(OwnerThread()->IsCurrentThreadIn());
      mCanonical = nullptr;
    }

    bool IsConnected() const { return !!mCanonical; }

    void Connect(AbstractCanonical<T>* aCanonical)
    {
      MOZ_ASSERT(OwnerThread()->IsCurrentThreadIn());
      ConnectInternal(aCanonical);
    }

  private:
    
    
    void ConnectInternal(AbstractCanonical<T>* aCanonical)
    {
      MIRROR_LOG("%s [%p] Connecting to %p", mName, this, aCanonical);
      MOZ_ASSERT(!IsConnected());

      nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethodWithArg<StorensRefPtrPassByPtr<AbstractMirror<T>>>
                                  (aCanonical, &AbstractCanonical<T>::AddMirror, this);
      aCanonical->OwnerThread()->Dispatch(r.forget(), AbstractThread::DontAssertDispatchSuccess);
      mCanonical = aCanonical;
    }
  public:

    void DisconnectIfConnected()
    {
      MOZ_ASSERT(OwnerThread()->IsCurrentThreadIn());
      if (!IsConnected()) {
        return;
      }

      MIRROR_LOG("%s [%p] Disconnecting from %p", mName, this, mCanonical.get());
      nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethodWithArg<StorensRefPtrPassByPtr<AbstractMirror<T>>>
                                  (mCanonical, &AbstractCanonical<T>::RemoveMirror, this);
      mCanonical->OwnerThread()->Dispatch(r.forget(), AbstractThread::DontAssertDispatchSuccess);
      mCanonical = nullptr;
    }

  protected:
    ~Impl() { MOZ_DIAGNOSTIC_ASSERT(!IsConnected()); }

  private:
    T mValue;
    nsRefPtr<AbstractCanonical<T>> mCanonical;
  };
public:

  
  void Connect(AbstractCanonical<T>* aCanonical) { mImpl->Connect(aCanonical); }
  void DisconnectIfConnected() { mImpl->DisconnectIfConnected(); }

  
  operator Impl&() { return *mImpl; }
  Impl* operator&() { return mImpl; }

  
  const T& Ref() const { return *mImpl; }
  operator const T&() const { return Ref(); }

private:
  nsRefPtr<Impl> mImpl;
};

#undef MIRROR_LOG

} 

#endif
