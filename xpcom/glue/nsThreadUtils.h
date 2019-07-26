





#ifndef nsThreadUtils_h__
#define nsThreadUtils_h__

#include "prthread.h"
#include "prinrval.h"
#include "nsIThreadManager.h"
#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsICancelableRunnable.h"
#include "nsStringGlue.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/threads/nsThreadIDs.h"
#include "mozilla/Likely.h"




#ifdef MOZILLA_INTERNAL_API
# define NS_SetThreadName NS_SetThreadName_P
# define NS_NewThread NS_NewThread_P
# define NS_NewNamedThread NS_NewNamedThread_P
# define NS_GetCurrentThread NS_GetCurrentThread_P
# define NS_GetMainThread NS_GetMainThread_P
# define NS_IsMainThread NS_IsMainThread_P
# define NS_DispatchToCurrentThread NS_DispatchToCurrentThread_P
# define NS_DispatchToMainThread NS_DispatchToMainThread_P
# define NS_ProcessPendingEvents NS_ProcessPendingEvents_P
# define NS_HasPendingEvents NS_HasPendingEvents_P
# define NS_ProcessNextEvent NS_ProcessNextEvent_P
#endif








extern NS_COM_GLUE void
NS_SetThreadName(nsIThread *thread, const nsACString &name);





template <size_t LEN>
inline NS_COM_GLUE void
NS_SetThreadName(nsIThread *thread, const char (&name)[LEN])
{
  MOZ_STATIC_ASSERT(LEN <= 16,
                    "Thread name must be no more than 16 characters");
  NS_SetThreadName(thread, nsDependentCString(name));
}














extern NS_COM_GLUE NS_METHOD
NS_NewThread(nsIThread **result,
             nsIRunnable *initialEvent = nullptr,
             uint32_t stackSize = nsIThreadManager::DEFAULT_STACK_SIZE);




template <size_t LEN>
inline NS_METHOD
NS_NewNamedThread(const char (&name)[LEN],
                  nsIThread **result,
                  nsIRunnable *initialEvent = nullptr,
                  uint32_t stackSize = nsIThreadManager::DEFAULT_STACK_SIZE)
{
    nsresult rv = NS_NewThread(result, initialEvent, stackSize);
    NS_SetThreadName<LEN>(*result, name);
    return rv;
}







extern NS_COM_GLUE NS_METHOD
NS_GetCurrentThread(nsIThread **result);







extern NS_COM_GLUE NS_METHOD
NS_GetMainThread(nsIThread **result);

#if defined(MOZILLA_INTERNAL_API) && defined(XP_WIN)
bool NS_IsMainThread();
#elif defined(MOZILLA_INTERNAL_API) && defined(NS_TLS)


extern NS_TLS mozilla::threads::ID gTLSThreadID;
inline bool NS_IsMainThread()
{
  return gTLSThreadID == mozilla::threads::Main;
}
#else






extern NS_COM_GLUE bool NS_IsMainThread();
#endif










extern NS_COM_GLUE NS_METHOD
NS_DispatchToCurrentThread(nsIRunnable *event);












extern NS_COM_GLUE NS_METHOD
NS_DispatchToMainThread(nsIRunnable *event,
                        uint32_t dispatchFlags = NS_DISPATCH_NORMAL);

#ifndef XPCOM_GLUE_AVOID_NSPR















extern NS_COM_GLUE NS_METHOD
NS_ProcessPendingEvents(nsIThread *thread,
                        PRIntervalTime timeout = PR_INTERVAL_NO_TIMEOUT);
#endif















extern NS_COM_GLUE bool
NS_HasPendingEvents(nsIThread *thread = nullptr);


















extern NS_COM_GLUE bool
NS_ProcessNextEvent(nsIThread *thread = nullptr, bool mayWait = true);




inline already_AddRefed<nsIThread>
do_GetCurrentThread() {
  nsIThread *thread = nullptr;
  NS_GetCurrentThread(&thread);
  return already_AddRefed<nsIThread>(thread);
}

inline already_AddRefed<nsIThread>
do_GetMainThread() {
  nsIThread *thread = nullptr;
  NS_GetMainThread(&thread);
  return already_AddRefed<nsIThread>(thread);
}



#ifdef MOZILLA_INTERNAL_API




extern NS_COM_GLUE nsIThread *NS_GetCurrentThread();
#endif



#ifndef XPCOM_GLUE_AVOID_NSPR

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_COM_GLUE


class NS_COM_GLUE nsRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsRunnable() {
  }

protected:
  virtual ~nsRunnable() {
  }
};


class NS_COM_GLUE nsCancelableRunnable : public nsICancelableRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSICANCELABLERUNNABLE

  nsCancelableRunnable() {
  }

protected:
  virtual ~nsCancelableRunnable() {
  }
};

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_VISIBILITY_HIDDEN




template <class ClassType,
          typename ReturnType = void,
          bool Owning = true>
class nsRunnableMethod : public nsRunnable
{
public:
  virtual void Revoke() = 0;

  
  
  
  template <typename OtherReturnType>
  class ReturnTypeEnforcer
  {
  public:
    typedef int ReturnTypeIsSafe;
  };

  template <class T>
  class ReturnTypeEnforcer<already_AddRefed<T> >
  {
    
  };

  
  typedef typename ReturnTypeEnforcer<ReturnType>::ReturnTypeIsSafe check;
};

template <class ClassType, bool Owning>
struct nsRunnableMethodReceiver {
  ClassType *mObj;
  nsRunnableMethodReceiver(ClassType *obj) : mObj(obj) { NS_IF_ADDREF(mObj); }
 ~nsRunnableMethodReceiver() { Revoke(); }
  void Revoke() { NS_IF_RELEASE(mObj); }
};

template <class ClassType>
struct nsRunnableMethodReceiver<ClassType, false> {
  ClassType *mObj;
  nsRunnableMethodReceiver(ClassType *obj) : mObj(obj) {}
  void Revoke() { mObj = nullptr; }
};

template <typename Method, bool Owning> struct nsRunnableMethodTraits;

template <class C, typename R, bool Owning>
struct nsRunnableMethodTraits<R (C::*)(), Owning> {
  typedef C class_type;
  typedef R return_type;
  typedef nsRunnableMethod<C, R, Owning> base_type;
};

#ifdef HAVE_STDCALL
template <class C, typename R, bool Owning>
struct nsRunnableMethodTraits<R (__stdcall C::*)(), Owning> {
  typedef C class_type;
  typedef R return_type;
  typedef nsRunnableMethod<C, R, Owning> base_type;
};
#endif

template <typename Method, bool Owning>
class nsRunnableMethodImpl
  : public nsRunnableMethodTraits<Method, Owning>::base_type
{
  typedef typename nsRunnableMethodTraits<Method, Owning>::class_type ClassType;
  nsRunnableMethodReceiver<ClassType, Owning> mReceiver;
  Method mMethod;

public:
  nsRunnableMethodImpl(ClassType *obj,
                       Method method)
    : mReceiver(obj)
    , mMethod(method)
  {}

  NS_IMETHOD Run() {
    if (MOZ_LIKELY(mReceiver.mObj))
      ((*mReceiver.mObj).*mMethod)();
    return NS_OK;
  }

  void Revoke() {
    mReceiver.Revoke();
  }
};











template<typename PtrType, typename Method>
typename nsRunnableMethodTraits<Method, true>::base_type*
NS_NewRunnableMethod(PtrType ptr, Method method)
{
  return new nsRunnableMethodImpl<Method, true>(ptr, method);
}

template<typename PtrType, typename Method>
typename nsRunnableMethodTraits<Method, false>::base_type*
NS_NewNonOwningRunnableMethod(PtrType ptr, Method method)
{
  return new nsRunnableMethodImpl<Method, false>(ptr, method);
}

#endif  














































template <class T>
class nsRevocableEventPtr {
public:
  nsRevocableEventPtr()
    : mEvent(nullptr) {
  }

  ~nsRevocableEventPtr() {
    Revoke();
  }

  const nsRevocableEventPtr& operator=(T *event) {
    if (mEvent != event) {
      Revoke();
      mEvent = event;
    }
    return *this;
  }

  void Revoke() {
    if (mEvent) {
      mEvent->Revoke();
      mEvent = nullptr;
    }
  }

  void Forget() {
    mEvent = nullptr;
  }

  bool IsPending() {
    return mEvent != nullptr;
  }
  
  T *get() { return mEvent; }

private:
  
  nsRevocableEventPtr(const nsRevocableEventPtr&);
  nsRevocableEventPtr& operator=(const nsRevocableEventPtr&);

  nsRefPtr<T> mEvent;
};





class nsThreadPoolNaming
{
public:
  nsThreadPoolNaming() : mCounter(0) {}

  




  void SetThreadPoolName(const nsACString & aPoolName,
                         nsIThread * aThread = nullptr);

private:
  volatile uint32_t mCounter;

  nsThreadPoolNaming(const nsThreadPoolNaming &) MOZ_DELETE;
  void operator=(const nsThreadPoolNaming &) MOZ_DELETE;
};







class NS_STACK_CLASS nsAutoLowPriorityIO
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



#endif  
