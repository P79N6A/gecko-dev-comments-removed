





































#ifndef __NSDOMWORKERSCRIPTLOADER_H__
#define __NSDOMWORKERSCRIPTLOADER_H__


#include "nsIRunnable.h"
#include "nsIStreamLoader.h"


#include "nsIChannel.h"
#include "nsIURI.h"


#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsAutoJSValHolder.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "prlock.h"

#include "nsDOMWorker.h"

class nsIThread;




























class nsDOMWorkerScriptLoader : public nsDOMWorkerFeature,
                                public nsIRunnable,
                                public nsIStreamLoaderObserver
{
  friend class AutoSuspendWorkerEvents;
  friend class ScriptLoaderRunnable;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSISTREAMLOADEROBSERVER

  nsDOMWorkerScriptLoader(nsDOMWorker* aWorker);

  nsresult LoadScripts(JSContext* aCx,
                       const nsTArray<nsString>& aURLs,
                       PRBool aForWorker);

  nsresult LoadScript(JSContext* aCx,
                      const nsString& aURL,
                      PRBool aForWorker);

  virtual void Cancel();

private:
  ~nsDOMWorkerScriptLoader() { }


  nsresult DoRunLoop(JSContext* aCx);
  nsresult VerifyScripts(JSContext* aCx);
  nsresult ExecuteScripts(JSContext* aCx);

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

  class ScriptLoaderRunnable : public nsIRunnable
  {
  public:
    NS_DECL_ISUPPORTS

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
                   const nsString& aScriptText,
                   const nsCString& aFilename,
                   nsAutoJSValHolder& aScriptObj);

  private:
    nsString mScriptText;
    nsCString mFilename;
    nsAutoJSValHolder& mScriptObj;
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
    nsAutoJSValHolder scriptObj;
  };

  nsIThread* mTarget;

  nsRefPtr<ScriptLoaderDone> mDoneRunnable;

  PRUint32 mScriptCount;
  nsTArray<ScriptLoadInfo> mLoadInfos;

  
  nsTArray<ScriptLoaderRunnable*> mPendingRunnables;

  PRPackedBool mCanceled;
  PRPackedBool mForWorker;
};

#endif 
