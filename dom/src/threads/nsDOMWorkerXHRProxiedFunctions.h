





































#ifndef __NSDOMWORKERXHRPROXIEDFUNCTIONS_H__
#define __NSDOMWORKERXHRPROXIEDFUNCTIONS_H__

#define MAKE_PROXIED_FUNCTION0(_name) \
  class _name : public nsRunnable \
  { \
  public: \
    _name (nsDOMWorkerXHRProxy* aXHR) \
    : mXHR(aXHR) \
    { \
      NS_ASSERTION(aXHR, "Null pointer!"); \
    } \
  \
    NS_IMETHOD Run() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (); \
      } \
      return NS_OK; \
    } \
  private: \
    nsRefPtr<nsDOMWorkerXHRProxy> mXHR; \
  }

#define MAKE_PROXIED_FUNCTION1(_name, _arg1) \
  class _name : public nsRunnable \
  { \
  public: \
     _name (nsDOMWorkerXHRProxy* aXHR, _arg1 aArg1) \
    : mXHR(aXHR), mArg1(aArg1) \
    { \
      NS_ASSERTION(aXHR, "Null pointer!"); \
    } \
  \
    NS_IMETHOD Run() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (mArg1); \
      } \
      return NS_OK; \
    } \
  private: \
    nsRefPtr<nsDOMWorkerXHRProxy> mXHR; \
     _arg1 mArg1; \
  }

#define MAKE_PROXIED_FUNCTION2(_name, _arg1, _arg2) \
  class _name : public nsRunnable \
  { \
  public: \
    _name (nsDOMWorkerXHRProxy* aXHR, _arg1 aArg1, _arg2 aArg2) \
    : mXHR(aXHR), mArg1(aArg1), mArg2(aArg2) \
    { \
      NS_ASSERTION(aXHR, "Null pointer!"); \
    } \
  \
    NS_IMETHOD Run() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (mArg1, mArg2); \
      } \
      return NS_OK; \
    } \
  private: \
    nsRefPtr<nsDOMWorkerXHRProxy> mXHR; \
    _arg1 mArg1; \
    _arg2 mArg2; \
  }

#define MAKE_PROXIED_FUNCTION3(_name, _arg1, _arg2, _arg3) \
  class _name : public nsRunnable \
  { \
  public: \
    _name (nsDOMWorkerXHRProxy* aXHR, _arg1 aArg1, _arg2 aArg2, _arg3 aArg3) \
    : mXHR(aXHR), mArg1(aArg1), mArg2(aArg2), mArg3(aArg3) \
    { \
      NS_ASSERTION(aXHR, "Null pointer!"); \
    } \
  \
    NS_IMETHOD Run() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (mArg1, mArg2, mArg3); \
      } \
      return NS_OK; \
    } \
  private: \
    nsRefPtr<nsDOMWorkerXHRProxy> mXHR; \
    _arg1 mArg1; \
    _arg2 mArg2; \
    _arg3 mArg3; \
  }

#define MAKE_PROXIED_FUNCTION4(_name, _arg1, _arg2, _arg3, _arg4) \
  class _name : public nsRunnable \
  { \
  public: \
    _name (nsDOMWorkerXHRProxy* aXHR, _arg1 aArg1, _arg2 aArg2, _arg3 aArg3, \
           _arg4 aArg4) \
    : mXHR(aXHR), mArg1(aArg1), mArg2(aArg2), mArg3(aArg3), mArg4(aArg4) \
    { \
      NS_ASSERTION(aXHR, "Null pointer!"); \
    } \
  \
    NS_IMETHOD Run() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (mArg1, mArg2, mArg3, mArg4); \
      } \
      return NS_OK; \
    } \
  private: \
    nsRefPtr<nsDOMWorkerXHRProxy> mXHR; \
    _arg1 mArg1; \
    _arg2 mArg2; \
    _arg3 mArg3; \
    _arg4 mArg4; \
  }

#define MAKE_PROXIED_FUNCTION5(_name, _arg1, _arg2, _arg3, _arg4, _arg5) \
  class _name : public nsRunnable \
  { \
  public: \
    _name (nsDOMWorkerXHRProxy* aXHR, _arg1 aArg1, _arg2 aArg2, _arg3 aArg3, \
           _arg4 aArg4, _arg5 aArg5) \
    : mXHR(aXHR), mArg1(aArg1), mArg2(aArg2), mArg3(aArg3), mArg4(aArg4), \
      mArg5(aArg5) \
    { \
      NS_ASSERTION(aXHR, "Null pointer!"); \
    } \
  \
    NS_IMETHOD Run() \
    { \
      nsCOMPtr<nsIXMLHttpRequest> xhr = mXHR->GetXMLHttpRequest(); \
      if (xhr) { \
        return xhr-> _name (mArg1, mArg2, mArg3, mArg4, mArg5); \
      } \
      return NS_OK; \
    } \
  private: \
    nsRefPtr<nsDOMWorkerXHRProxy> mXHR; \
    _arg1 mArg1; \
    _arg2 mArg2; \
    _arg3 mArg3; \
    _arg4 mArg4; \
    _arg5 mArg5; \
  }

#define RUN_PROXIED_FUNCTION(_name, _args) \
  PR_BEGIN_MACRO \
    if (mCanceled) { \
      return NS_ERROR_ABORT; \
    } \
    \
    nsCOMPtr<nsIRunnable> method = new :: _name _args; \
    NS_ENSURE_TRUE(method, NS_ERROR_OUT_OF_MEMORY); \
    \
    nsRefPtr<nsResultReturningRunnable> runnable = \
      new nsResultReturningRunnable(mMainThread, method, mWorkerXHR->mWorker); \
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY); \
    \
    nsresult _rv = runnable->Dispatch(); \
    if (NS_FAILED(_rv)) { \
      return _rv; \
    } \
  PR_END_MACRO

namespace nsDOMWorkerProxiedXHRFunctions
{
  class Abort : public nsRunnable
  {
  public:
    Abort (nsDOMWorkerXHRProxy* aXHR)
    : mXHR(aXHR)
    {
      NS_ASSERTION(aXHR, "Null pointer!");
    }

    NS_IMETHOD Run() {
      return mXHR->Abort();
    }
  private:
    nsRefPtr<nsDOMWorkerXHRProxy> mXHR;
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
};

#endif 
