





































#ifndef __NSDOMWORKERXHRPROXIEDFUNCTIONS_H__
#define __NSDOMWORKERXHRPROXIEDFUNCTIONS_H__

#define MAKE_PROXIED_FUNCTION0(_name) \
  class _name : public SyncEventCapturingRunnable \
  { \
  public: \
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
    _name (_arg1 aArg1) : mArg1(aArg1) { } \
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
    _name (_arg1 aArg1, _arg2 aArg2) : mArg1(aArg1), mArg2(aArg2) { } \
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

namespace nsDOMWorkerProxiedXHRFunctions
{
  typedef nsDOMWorkerXHRProxy::SyncEventQueue SyncEventQueue;

  class SyncEventCapturingRunnable : public nsRunnable
  {
  public:
    SyncEventCapturingRunnable()
    : mXHR(nsnull), mQueue(nsnull) { }

    void Init(nsDOMWorkerXHRProxy* aXHR,
              SyncEventQueue* aQueue) {
      NS_ASSERTION(aXHR, "Null pointer!");
      NS_ASSERTION(aQueue, "Null pointer!");

      mXHR = aXHR;
      mQueue = aQueue;
    }

    virtual nsresult RunInternal() = 0;

    NS_IMETHOD Run() {
      NS_ASSERTION(mXHR && mQueue, "Forgot to call Init!");

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
    virtual nsresult RunInternal() {
      return mXHR->Abort();
    }
  };

  class Open : public SyncEventCapturingRunnable
  {
  public:
    Open(const nsACString& aMethod, const nsACString& aUrl,
         bool aAsync, const nsAString& aUser,
         const nsAString& aPassword)
    : mMethod(aMethod), mUrl(aUrl), mAsync(aAsync), mUser(aUser),
      mPassword(aPassword) { }
  
    virtual nsresult RunInternal() {
      return mXHR->Open(mMethod, mUrl, mAsync, mUser, mPassword);
    }

  private:
    nsCString mMethod;
    nsCString mUrl;
    bool mAsync;
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

  MAKE_PROXIED_FUNCTION1(SetMultipart, bool);

  MAKE_PROXIED_FUNCTION1(GetMultipart, bool*);

  MAKE_PROXIED_FUNCTION1(GetWithCredentials, bool*);

  MAKE_PROXIED_FUNCTION1(SetWithCredentials, bool);
}

#endif 
