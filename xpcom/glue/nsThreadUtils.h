





































#ifndef nsThreadUtils_h__
#define nsThreadUtils_h__

#include "prthread.h"
#include "prinrval.h"
#include "nsIThreadManager.h"
#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsStringGlue.h"
#include "nsCOMPtr.h"




#ifdef MOZILLA_INTERNAL_API
# define NS_NewThread NS_NewThread_P
# define NS_GetCurrentThread NS_GetCurrentThread_P
# define NS_GetMainThread NS_GetMainThread_P
# define NS_IsMainThread NS_IsMainThread_P
# define NS_DispatchToCurrentThread NS_DispatchToCurrentThread_P
# define NS_DispatchToMainThread NS_DispatchToMainThread_P
# define NS_ProcessPendingEvents NS_ProcessPendingEvents_P
# define NS_HasPendingEvents NS_HasPendingEvents_P
# define NS_ProcessNextEvent NS_ProcessNextEvent_P
#endif
















extern NS_COM_GLUE NS_METHOD
NS_NewThread(nsIThread **result, nsIRunnable *initialEvent = nsnull);







extern NS_COM_GLUE NS_METHOD
NS_GetCurrentThread(nsIThread **result);







extern NS_COM_GLUE NS_METHOD
NS_GetMainThread(nsIThread **result);







extern NS_COM_GLUE NS_METHOD_(PRBool)
NS_IsMainThread();










extern NS_COM_GLUE NS_METHOD
NS_DispatchToCurrentThread(nsIRunnable *event);












extern NS_COM_GLUE NS_METHOD
NS_DispatchToMainThread(nsIRunnable *event,
                        PRUint32 dispatchFlags = NS_DISPATCH_NORMAL);

#ifndef XPCOM_GLUE_AVOID_NSPR















extern NS_COM_GLUE NS_METHOD
NS_ProcessPendingEvents(nsIThread *thread,
                        PRIntervalTime timeout = PR_INTERVAL_NO_TIMEOUT);
#endif















extern NS_COM_GLUE PRBool
NS_HasPendingEvents(nsIThread *thread = nsnull);


















extern NS_COM_GLUE PRBool
NS_ProcessNextEvent(nsIThread *thread = nsnull, PRBool mayWait = PR_TRUE);




inline already_AddRefed<nsIThread>
do_GetCurrentThread() {
  nsIThread *thread = nsnull;
  NS_GetCurrentThread(&thread);
  return already_AddRefed<nsIThread>(thread);
}

inline already_AddRefed<nsIThread>
do_GetMainThread() {
  nsIThread *thread = nsnull;
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

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_VISIBILITY_HIDDEN



template <class T>
class nsRunnableMethod : public nsRunnable
{
public:
  typedef void (T::*Method)();

  nsRunnableMethod(T *obj, Method method)
    : mObj(obj), mMethod(method) {
    NS_ADDREF(mObj);
  }

  NS_IMETHOD Run() {
    (mObj->*mMethod)();
    return NS_OK;
  }

private:
  virtual ~nsRunnableMethod() {
    NS_RELEASE(mObj);
  }

  T      *mObj;
  Method  mMethod;
};













#define NS_NEW_RUNNABLE_METHOD(class_, obj_, method_) \
    new nsRunnableMethod<class_>(obj_, &class_::method_)

#endif  














































template <class T>
class nsRevocableEventPtr {
public:
  nsRevocableEventPtr()
    : mEvent(nsnull) {
  }

  ~nsRevocableEventPtr() {
    Revoke();
  }

  const nsRevocableEventPtr& operator=(T *event) {
    Revoke();
    mEvent = event;
    return *this;
  }

  void Revoke() {
    if (mEvent) {
      mEvent->Revoke();
      mEvent = nsnull;
    }
  }

  void Forget() {
    mEvent = nsnull;
  }

  PRBool IsPending() {
    return mEvent != nsnull;
  }

private:
  
  nsRevocableEventPtr(const nsRevocableEventPtr&);
  nsRevocableEventPtr& operator=(const nsRevocableEventPtr&);

  T *mEvent;
};

#endif  
