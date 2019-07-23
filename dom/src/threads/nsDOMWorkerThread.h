






































#ifndef __NSDOMWORKERTHREAD_H__
#define __NSDOMWORKERTHREAD_H__


#include "nsDOMWorkerBase.h"
#include "nsIClassInfo.h"
#include "nsIDOMThreads.h"


#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsThreadUtils.h"
#include "prclist.h"
#include "prlock.h"


#include "nsDOMThreadService.h"


#define NS_IMPL_THREADSAFE_DOM_CI(_class)                                     \
NS_IMETHODIMP                                                                 \
_class::GetInterfaces(PRUint32* _count, nsIID*** _array)                      \
{                                                                             \
  return NS_CI_INTERFACE_GETTER_NAME(_class)(_count, _array);                 \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetHelperForLanguage(PRUint32 _language, nsISupports** _retval)       \
{                                                                             \
  *_retval = nsnull;                                                          \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetContractID(char** _contractID)                                     \
{                                                                             \
  *_contractID = nsnull;                                                      \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassDescription(char** _classDescription)                         \
{                                                                             \
  *_classDescription = nsnull;                                                \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassID(nsCID** _classID)                                          \
{                                                                             \
  *_classID = nsnull;                                                         \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetImplementationLanguage(PRUint32* _language)                        \
{                                                                             \
  *_language = nsIProgrammingLanguage::CPLUSPLUS;                             \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetFlags(PRUint32* _flags)                                            \
{                                                                             \
  *_flags = nsIClassInfo::THREADSAFE | nsIClassInfo::DOM_OBJECT;              \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassIDNoAlloc(nsCID* _classIDNoAlloc)                             \
{                                                                             \
  return NS_ERROR_NOT_AVAILABLE;                                              \
}

class nsDOMWorkerPool;
class nsDOMWorkerTimeout;

class nsDOMWorkerThread : public nsDOMWorkerBase,
                          public nsIDOMWorkerThread,
                          public nsIClassInfo
{
  friend class nsDOMCreateJSContextRunnable;
  friend class nsDOMWorkerFunctions;
  friend class nsDOMWorkerPool;
  friend class nsDOMWorkerRunnable;
  friend class nsDOMWorkerTimeout;

  friend JSBool DOMWorkerOperationCallback(JSContext* aCx);

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMWORKERTHREAD
  NS_DECL_NSICLASSINFO

  nsDOMWorkerThread(nsDOMWorkerPool* aPool,
                    const nsAString& aSource);

  virtual nsDOMWorkerPool* Pool() {
    NS_ASSERTION(!IsCanceled(), "Don't touch Pool after we've been canceled!");
    return mPool;
  }

private:
  virtual ~nsDOMWorkerThread();

  nsresult Init();

  
  virtual nsresult HandleMessage(const nsAString& aMessage,
                                 nsDOMWorkerBase* aSourceThread);

  
  virtual nsresult DispatchMessage(nsIRunnable* aRunnable);

  virtual void Cancel();
  virtual void Suspend();
  virtual void Resume();

  PRBool SetGlobalForContext(JSContext* aCx);
  PRBool CompileGlobalObject(JSContext* aCx);

  inline nsDOMWorkerTimeout* FirstTimeout();
  inline nsDOMWorkerTimeout* NextTimeout(nsDOMWorkerTimeout* aTimeout);

  void AddTimeout(nsDOMWorkerTimeout* aTimeout);
  void RemoveTimeout(nsDOMWorkerTimeout* aTimeout);
  void ClearTimeouts();
  void CancelTimeout(PRUint32 aId);
  void SuspendTimeouts();
  void ResumeTimeouts();

  nsDOMWorkerPool* mPool;
  nsString mSource;

  JSObject* mGlobal;
  PRBool mCompiled;

  PRUint32 mCallbackCount;

  PRUint32 mNextTimeoutId;

  PRLock* mLock;
  PRCList mTimeouts;
};

#endif 
