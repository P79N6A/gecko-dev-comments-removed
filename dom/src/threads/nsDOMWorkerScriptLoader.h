





































#ifndef __NSDOMWORKERSCRIPTLOADER_H__
#define __NSDOMWORKERSCRIPTLOADER_H__


#include "nsThreadUtils.h"
#include "nsIStreamLoader.h"


#include "nsIChannel.h"
#include "nsIURI.h"


#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsAutoJSObjectHolder.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "prlock.h"


#include "nsDOMWorkerThread.h"




























class nsDOMWorkerScriptLoader : public nsRunnable,
                                public nsIStreamLoaderObserver
{
  friend class AutoSuspendWorkerEvents;
  friend class nsDOMWorkerFunctions;
  friend class nsDOMWorkerThread;
  friend class ScriptLoaderRunnable;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSISTREAMLOADEROBSERVER

  nsDOMWorkerScriptLoader();

  nsresult LoadScripts(nsDOMWorkerThread* aWorker,
                       JSContext* aCx,
                       const nsTArray<nsString>& aURLs);

  nsresult LoadScript(nsDOMWorkerThread* aWorker,
                       JSContext* aCx,
                       const nsString& aURL);

  void Cancel();

private:
  ~nsDOMWorkerScriptLoader();

  nsresult DoRunLoop();
  nsresult VerifyScripts();
  nsresult ExecuteScripts();

  nsresult RunInternal();

  nsresult OnStreamCompleteInternal(nsIStreamLoader* aLoader,
                                    nsISupports* aContext,
                                    nsresult aStatus,
                                    PRUint32 aStringLen,
                                    const PRUint8* aString);

  void NotifyDone();

  void SuspendWorkerEvents();
  void ResumeWorkerEvents();

  PRLock* Lock() {
    return mWorker->Lock();
  }

  class ScriptLoaderRunnable : public nsRunnable
  {
  protected:
    
    ScriptLoaderRunnable(nsDOMWorkerScriptLoader* aLoader);
    virtual ~ScriptLoaderRunnable();

  public:
    void Revoke();

  protected:
    PRBool mRevoked;

  private:
    nsDOMWorkerScriptLoader* mLoader;
  };

  class ScriptCompiler : public ScriptLoaderRunnable
  {
  public:
    NS_DECL_NSIRUNNABLE

    ScriptCompiler(nsDOMWorkerScriptLoader* aLoader,
                   JSContext* aCx,
                   const nsString& aScriptText,
                   const nsCString& aFilename,
                   nsAutoJSObjectHolder& aScriptObj);

  private:
    JSContext* mCx;
    nsString mScriptText;
    nsCString mFilename;
    nsAutoJSObjectHolder& mScriptObj;
  };

  class ScriptLoaderDone : public ScriptLoaderRunnable
  {
  public:
    NS_DECL_NSIRUNNABLE

    ScriptLoaderDone(nsDOMWorkerScriptLoader* aLoader,
                     volatile PRBool* aDoneFlag);

  private:
    volatile PRBool* mDoneFlag;
  };

  class AutoSuspendWorkerEvents
  {
  public:
    AutoSuspendWorkerEvents(nsDOMWorkerScriptLoader* aLoader);
    ~AutoSuspendWorkerEvents();

  private:
    nsDOMWorkerScriptLoader* mLoader;
  };

  struct ScriptLoadInfo
  {
    ScriptLoadInfo() : done(PR_FALSE), result(NS_ERROR_NOT_INITIALIZED) { }

    nsString url;
    nsString scriptText;
    PRBool done;
    nsresult result;
    nsCOMPtr<nsIURI> finalURI;
    nsCOMPtr<nsIChannel> channel;
    nsAutoJSObjectHolder scriptObj;
  };

  nsDOMWorkerThread* mWorker;
  nsIThread* mTarget;
  JSContext* mCx;

  nsRefPtr<ScriptLoaderDone> mDoneRunnable;

  PRUint32 mScriptCount;
  nsTArray<ScriptLoadInfo> mLoadInfos;

  PRPackedBool mCanceled;
  PRPackedBool mTrackedByWorker;

  
  nsTArray<ScriptLoaderRunnable*> mPendingRunnables;
};

#endif 
