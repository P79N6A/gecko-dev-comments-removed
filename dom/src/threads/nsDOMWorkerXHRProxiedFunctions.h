





































#ifndef __NSDOMWORKERXHRPROXIEDFUNCTIONS_H__
#define __NSDOMWORKERXHRPROXIEDFUNCTIONS_H__

#define MAKE_PROXIED_FUNCTION0(_name) \
  class _name : public SyncEventCapturingRunnable \
  { \
  public: \
    _name (nsDOMWorkerXHRProxy* aXHR, SyncEventQueue* aQueue) \
    : SyncEventCapturingRunnable(aXHR, aQueue) { } \
  \
    virtual nsresult RunInternal() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (); \
      } \
      return NS_OK; \
    } \
  }

#define MAKE_PROXIED_FUNCTION1(_name, _arg1) \
  class _name : public SyncEventCapturingRunnable \
  { \
  public: \
     _name (nsDOMWorkerXHRProxy* aXHR, SyncEventQueue* aQueue, _arg1 aArg1) \
    : SyncEventCapturingRunnable(aXHR, aQueue), mArg1(aArg1) { } \
  \
    virtual nsresult RunInternal() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (mArg1); \
      } \
      return NS_OK; \
    } \
  private: \
     _arg1 mArg1; \
  }

#define MAKE_PROXIED_FUNCTION2(_name, _arg1, _arg2) \
  class _name : public SyncEventCapturingRunnable \
  { \
  public: \
    _name (nsDOMWorkerXHRProxy* aXHR, SyncEventQueue* aQueue, _arg1 aArg1, \
           _arg2 aArg2) \
    : SyncEventCapturingRunnable(aXHR, aQueue), mArg1(aArg1), mArg2(aArg2) { } \
  \
    virtual nsresult RunInternal() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (mArg1, mArg2); \
      } \
      return NS_OK; \
    } \
  private: \
    _arg1 mArg1; \
    _arg2 mArg2; \
  }

#define MAKE_PROXIED_FUNCTION3(_name, _arg1, _arg2, _arg3) \
  class _name : public SyncEventCapturingRunnable \
  { \
  public: \
    _name (nsDOMWorkerXHRProxy* aXHR, SyncEventQueue* aQueue, _arg1 aArg1, \
           _arg2 aArg2, _arg3 aArg3) \
    : SyncEventCapturingRunnable(aXHR, aQueue), mArg1(aArg1), mArg2(aArg2), \
      mArg3(aArg3) { } \
  \
    virtual nsresult RunInternal() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (mArg1, mArg2, mArg3); \
      } \
      return NS_OK; \
    } \
  private: \
    _arg1 mArg1; \
    _arg2 mArg2; \
    _arg3 mArg3; \
  }

#define MAKE_PROXIED_FUNCTION4(_name, _arg1, _arg2, _arg3, _arg4) \
  class _name : public SyncEventCapturingRunnable \
  { \
  public: \
    _name (nsDOMWorkerXHRProxy* aXHR, SyncEventQueue* aQueue, _arg1 aArg1, \
           _arg2 aArg2, _arg3 aArg3, _arg4 aArg4) \
    : SyncEventCapturingRunnable(aXHR, aQueue), mArg1(aArg1), mArg2(aArg2), \
      mArg3(aArg3), mArg4(aArg4) { } \
  \
    virtual nsresult RunInternal() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (mArg1, mArg2, mArg3, mArg4); \
      } \
      return NS_OK; \
    } \
  private: \
    _arg1 mArg1; \
    _arg2 mArg2; \
    _arg3 mArg3; \
    _arg4 mArg4; \
  }

#define MAKE_PROXIED_FUNCTION5(_name, _arg1, _arg2, _arg3, _arg4, _arg5) \
  class _name : public SyncEventCapturingRunnable \
  { \
  public: \
    _name (nsDOMWorkerXHRProxy* aXHR, SyncEventQueue* aQueue, _arg1 aArg1, \
           _arg2 aArg2, _arg3 aArg3, _arg4 aArg4, _arg5 aArg5) \
    : SyncEventCapturingRunnable(aXHR, aQueue), mArg1(aArg1), mArg2(aArg2), \
      mArg3(aArg3), mArg4(aArg4), mArg5(aArg5) { } \
  \
    virtual nsresult RunInternal() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (mArg1, mArg2, mArg3, mArg4, mArg5); \
      } \
      return NS_OK; \
    } \
  private: \
    _arg1 mArg1; \
    _arg2 mArg2; \
    _arg3 mArg3; \
    _arg4 mArg4; \
    _arg5 mArg5; \
  }

#define RUN_PROXIED_FUNCTION(_name, _args) \
  PR_BEGIN_MACRO \
    NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!"); \
    \
    if (mCanceled) { \
      return NS_ERROR_ABORT; \
    } \
    SyncEventQueue queue; \
    \
    nsCOMPtr<nsIRunnable> method = new :: _name _args; \
    NS_ENSURE_TRUE(method, NS_ERROR_OUT_OF_MEMORY); \
    \
    nsRefPtr<nsResultReturningRunnable> runnable = \
      new nsResultReturningRunnable(mMainThread, method, mWorkerXHR->mWorker); \
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY); \
    \
    nsresult _rv = runnable->Dispatch(); \
    \
    PRUint32 queueLength = queue.Length(); \
    for (PRUint32 index = 0; index < queueLength; index++) { \
      queue[index]->Run(); \
    } \
    \
    if (NS_FAILED(_rv)) { \
      return _rv; \
    } \
  PR_END_MACRO

namespace nsDOMWorkerProxiedXHRFunctions
{
  typedef nsDOMWorkerXHRProxy::SyncEventQueue SyncEventQueue;

  class SyncEventCapturingRunnable : public nsRunnable
  {
  public:
    SyncEventCapturingRunnable(nsDOMWorkerXHRProxy* aXHR,
                               SyncEventQueue* aQueue)
    : mXHR(aXHR), mQueue(aQueue) {
      NS_ASSERTION(aXHR, "Null pointer!");
      NS_ASSERTION(aQueue, "Null pointer!");
    }

    virtual nsresult RunInternal() = 0;

    NS_IMETHOD Run() {
      SyncEventQueue* oldQueue = mXHR->SetSyncEventQueue(mQueue);

      nsresult rv = RunInternal();

      mXHR->SetSyncEventQueue(oldQueue);

      return rv;
    }

  protected:
    nsRefPtr<nsDOMWorkerXHRProxy> mXHR;
    SyncEventQueue* mQueue;
  };

  class Abort : public SyncEventCapturingRunnable
  {
  public:
    Abort (nsDOMWorkerXHRProxy* aXHR, SyncEventQueue* aQueue)
    : SyncEventCapturingRunnable(aXHR, aQueue) { }

    virtual nsresult RunInternal() {
      return mXHR->Abort();
    }
  };

  MAKE_PROXIED_FUNCTION1(GetAllResponseHeaders, char**);

  MAKE_PROXIED_FUNCTION2(GetResponseHeader, const nsACString&, nsACString&);

  MAKE_PROXIED_FUNCTION5(OpenRequest, const nsACString&, const nsACString&,
                         PRBool, const nsAString&, const nsAString&);

  MAKE_PROXIED_FUNCTION1(Send, nsIVariant*);

  MAKE_PROXIED_FUNCTION1(SendAsBinary, const nsAString&);

  MAKE_PROXIED_FUNCTION2(SetRequestHeader, const nsACString&,
                         const nsACString&);

  MAKE_PROXIED_FUNCTION1(OverrideMimeType, const nsACString&);

  MAKE_PROXIED_FUNCTION1(SetMultipart, PRBool);
}

#endif 
