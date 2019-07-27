





#ifndef nsThreadUtils_h__
#define nsThreadUtils_h__

#include "prthread.h"
#include "prinrval.h"
#include "MainThreadUtils.h"
#include "nsIThreadManager.h"
#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsICancelableRunnable.h"
#include "nsStringGlue.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/Likely.h"








extern void NS_SetThreadName(nsIThread* aThread, const nsACString& aName);





template<size_t LEN>
inline void
NS_SetThreadName(nsIThread* aThread, const char (&aName)[LEN])
{
  static_assert(LEN <= 16,
                "Thread name must be no more than 16 characters");
  NS_SetThreadName(aThread, nsDependentCString(aName));
}














extern NS_METHOD
NS_NewThread(nsIThread** aResult,
             nsIRunnable* aInitialEvent = nullptr,
             uint32_t aStackSize = nsIThreadManager::DEFAULT_STACK_SIZE);




template<size_t LEN>
inline NS_METHOD
NS_NewNamedThread(const char (&aName)[LEN],
                  nsIThread** aResult,
                  nsIRunnable* aInitialEvent = nullptr,
                  uint32_t aStackSize = nsIThreadManager::DEFAULT_STACK_SIZE)
{
  
  nsCOMPtr<nsIThread> thread;
  nsresult rv = NS_NewThread(getter_AddRefs(thread), nullptr, aStackSize);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  NS_SetThreadName<LEN>(thread, aName);
  if (aInitialEvent) {
    rv = thread->Dispatch(aInitialEvent, NS_DISPATCH_NORMAL);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Initial event dispatch failed");
  }

  *aResult = nullptr;
  thread.swap(*aResult);
  return rv;
}







extern NS_METHOD NS_GetCurrentThread(nsIThread** aResult);










extern NS_METHOD NS_DispatchToCurrentThread(nsIRunnable* aEvent);












extern NS_METHOD
NS_DispatchToMainThread(nsIRunnable* aEvent,
                        uint32_t aDispatchFlags = NS_DISPATCH_NORMAL);

#ifndef XPCOM_GLUE_AVOID_NSPR















extern NS_METHOD
NS_ProcessPendingEvents(nsIThread* aThread,
                        PRIntervalTime aTimeout = PR_INTERVAL_NO_TIMEOUT);
#endif















extern bool NS_HasPendingEvents(nsIThread* aThread = nullptr);


















extern bool NS_ProcessNextEvent(nsIThread* aThread = nullptr,
                                bool aMayWait = true);




inline already_AddRefed<nsIThread>
do_GetCurrentThread()
{
  nsIThread* thread = nullptr;
  NS_GetCurrentThread(&thread);
  return already_AddRefed<nsIThread>(thread);
}

inline already_AddRefed<nsIThread>
do_GetMainThread()
{
  nsIThread* thread = nullptr;
  NS_GetMainThread(&thread);
  return already_AddRefed<nsIThread>(thread);
}



#ifdef MOZILLA_INTERNAL_API




extern nsIThread* NS_GetCurrentThread();
#endif



#ifndef XPCOM_GLUE_AVOID_NSPR


class nsRunnable : public nsIRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsRunnable() {}

protected:
  virtual ~nsRunnable() {}
};


class nsCancelableRunnable : public nsICancelableRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSICANCELABLERUNNABLE

  nsCancelableRunnable() {}

protected:
  virtual ~nsCancelableRunnable() {}
};




template<class ClassType,
         typename ReturnType = void,
         bool Owning = true>
class nsRunnableMethod : public nsRunnable
{
public:
  virtual void Revoke() = 0;

  
  
  
  template<typename OtherReturnType>
  class ReturnTypeEnforcer
  {
  public:
    typedef int ReturnTypeIsSafe;
  };

  template<class T>
  class ReturnTypeEnforcer<already_AddRefed<T>>
  {
    
  };

  
  typedef typename ReturnTypeEnforcer<ReturnType>::ReturnTypeIsSafe check;
};

template<class ClassType, typename Arg, bool Owning>
struct nsRunnableMethodReceiver
{
  ClassType* mObj;
  Arg mArg;
  nsRunnableMethodReceiver(ClassType* aObj, Arg aArg)
    : mObj(aObj)
    , mArg(aArg)
  {
    NS_IF_ADDREF(mObj);
  }
  ~nsRunnableMethodReceiver() { Revoke(); }
  void Revoke() { NS_IF_RELEASE(mObj); }
};

template<class ClassType, bool Owning>
struct nsRunnableMethodReceiver<ClassType, void, Owning>
{
  ClassType* mObj;
  explicit nsRunnableMethodReceiver(ClassType* aObj)
    : mObj(aObj)
  {
    NS_IF_ADDREF(mObj);
  }
  ~nsRunnableMethodReceiver() { Revoke(); }
  void Revoke() { NS_IF_RELEASE(mObj); }
};

template<class ClassType>
struct nsRunnableMethodReceiver<ClassType, void, false>
{
  ClassType* mObj;
  explicit nsRunnableMethodReceiver(ClassType* aObj) : mObj(aObj) {}
  void Revoke() { mObj = nullptr; }
};

template<typename Method, bool Owning> struct nsRunnableMethodTraits;

template<class C, typename R, typename A, bool Owning>
struct nsRunnableMethodTraits<R(C::*)(A), Owning>
{
  typedef C class_type;
  typedef R return_type;
  typedef A arg_type;
  typedef nsRunnableMethod<C, R, Owning> base_type;
};

template<class C, typename R, bool Owning>
struct nsRunnableMethodTraits<R(C::*)(), Owning>
{
  typedef C class_type;
  typedef R return_type;
  typedef void arg_type;
  typedef nsRunnableMethod<C, R, Owning> base_type;
};

#ifdef NS_HAVE_STDCALL
template<class C, typename R, typename A, bool Owning>
struct nsRunnableMethodTraits<R(__stdcall C::*)(A), Owning>
{
  typedef C class_type;
  typedef R return_type;
  typedef A arg_type;
  typedef nsRunnableMethod<C, R, Owning> base_type;
};

template<class C, typename R, bool Owning>
struct nsRunnableMethodTraits<R(NS_STDCALL C::*)(), Owning>
{
  typedef C class_type;
  typedef R return_type;
  typedef void arg_type;
  typedef nsRunnableMethod<C, R, Owning> base_type;
};
#endif

template<typename Method, typename Arg, bool Owning>
class nsRunnableMethodImpl
  : public nsRunnableMethodTraits<Method, Owning>::base_type
{
  typedef typename nsRunnableMethodTraits<Method, Owning>::class_type ClassType;
  nsRunnableMethodReceiver<ClassType, Arg, Owning> mReceiver;
  Method mMethod;
public:
  nsRunnableMethodImpl(ClassType* aObj, Method aMethod, Arg aArg)
    : mReceiver(aObj, aArg)
    , mMethod(aMethod)
  {
  }
  NS_IMETHOD Run()
  {
    if (MOZ_LIKELY(mReceiver.mObj)) {
      ((*mReceiver.mObj).*mMethod)(mReceiver.mArg);
    }
    return NS_OK;
  }
  void Revoke() { mReceiver.Revoke(); }
};

template<typename Method, bool Owning>
class nsRunnableMethodImpl<Method, void, Owning>
  : public nsRunnableMethodTraits<Method, Owning>::base_type
{
  typedef typename nsRunnableMethodTraits<Method, Owning>::class_type ClassType;
  nsRunnableMethodReceiver<ClassType, void, Owning> mReceiver;
  Method mMethod;

public:
  nsRunnableMethodImpl(ClassType* aObj, Method aMethod)
    : mReceiver(aObj)
    , mMethod(aMethod)
  {
  }

  NS_IMETHOD Run()
  {
    if (MOZ_LIKELY(mReceiver.mObj)) {
      ((*mReceiver.mObj).*mMethod)();
    }
    return NS_OK;
  }

  void Revoke() { mReceiver.Revoke(); }
};











template<typename PtrType, typename Method>
typename nsRunnableMethodTraits<Method, true>::base_type*
NS_NewRunnableMethod(PtrType aPtr, Method aMethod)
{
  return new nsRunnableMethodImpl<Method, void, true>(aPtr, aMethod);
}

template<typename T>
struct dependent_type
{
  typedef T type;
};






template<typename Arg, typename Method, typename PtrType>
typename nsRunnableMethodTraits<Method, true>::base_type*
NS_NewRunnableMethodWithArg(PtrType&& aPtr, Method aMethod,
                            typename dependent_type<Arg>::type aArg)
{
  return new nsRunnableMethodImpl<Method, Arg, true>(aPtr, aMethod, aArg);
}

template<typename PtrType, typename Method>
typename nsRunnableMethodTraits<Method, false>::base_type*
NS_NewNonOwningRunnableMethod(PtrType&& aPtr, Method aMethod)
{
  return new nsRunnableMethodImpl<Method, void, false>(aPtr, aMethod);
}

#endif  














































template<class T>
class nsRevocableEventPtr
{
public:
  nsRevocableEventPtr() : mEvent(nullptr) {}
  ~nsRevocableEventPtr() { Revoke(); }

  const nsRevocableEventPtr& operator=(T* aEvent)
  {
    if (mEvent != aEvent) {
      Revoke();
      mEvent = aEvent;
    }
    return *this;
  }

  void Revoke()
  {
    if (mEvent) {
      mEvent->Revoke();
      mEvent = nullptr;
    }
  }

  void Forget() { mEvent = nullptr; }
  bool IsPending() { return mEvent != nullptr; }
  T* get() { return mEvent; }

private:
  
  nsRevocableEventPtr(const nsRevocableEventPtr&);
  nsRevocableEventPtr& operator=(const nsRevocableEventPtr&);

  nsRefPtr<T> mEvent;
};





class nsThreadPoolNaming
{
public:
  nsThreadPoolNaming() : mCounter(0) {}

  




  void SetThreadPoolName(const nsACString& aPoolName,
                         nsIThread* aThread = nullptr);

private:
  volatile uint32_t mCounter;

  nsThreadPoolNaming(const nsThreadPoolNaming&) MOZ_DELETE;
  void operator=(const nsThreadPoolNaming&) MOZ_DELETE;
};







class MOZ_STACK_CLASS nsAutoLowPriorityIO
{
public:
  nsAutoLowPriorityIO();
  ~nsAutoLowPriorityIO();

private:
  bool lowIOPrioritySet;
#if defined(XP_MACOSX)
  int oldPriority;
#endif
};

void
NS_SetMainThread();

#endif  
