





































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

  class OpenRequest : public SyncEventCapturingRunnable
  {
  public:
    OpenRequest(nsDOMWorkerXHRProxy* aXHR, SyncEventQueue* aQueue,
                const nsACString& aMethod, const nsACString& aUrl,
                PRBool aAsync, const nsAString& aUser,
                const nsAString& aPassword)
    : SyncEventCapturingRunnable(aXHR, aQueue), mMethod(aMethod), mUrl(aUrl),
      mAsync(aAsync), mUser(aUser), mPassword(aPassword) { }
  
    virtual nsresult RunInternal() {
      return mXHR->OpenRequest(mMethod, mUrl, mAsync, mUser, mPassword);
    }

  private:
    nsCString mMethod;
    nsCString mUrl;
    PRBool mAsync;
    nsString mUser;
    nsString mPassword;
  };

  MAKE_PROXIED_FUNCTION1(GetAllResponseHeaders, char**);

  MAKE_PROXIED_FUNCTION2(GetResponseHeader, const nsACString&, nsACString&);

  MAKE_PROXIED_FUNCTION1(Send, nsIVariant*);

  MAKE_PROXIED_FUNCTION1(SendAsBinary, const nsAString&);

  MAKE_PROXIED_FUNCTION2(SetRequestHeader, const nsACString&,
                         const nsACString&);

  MAKE_PROXIED_FUNCTION1(OverrideMimeType, const nsACString&);

  MAKE_PROXIED_FUNCTION1(SetMultipart, PRBool);

  MAKE_PROXIED_FUNCTION1(GetMultipart, PRBool*);
}

#endif 
