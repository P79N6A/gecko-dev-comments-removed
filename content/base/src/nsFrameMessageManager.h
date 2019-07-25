



































#ifndef nsFrameMessageManager_h__
#define nsFrameMessageManager_h__

#include "nsIFrameMessageManager.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsIAtom.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTArray.h"
#include "nsIPrincipal.h"
#include "nsIXPConnect.h"
#include "nsDataHashtable.h"
#include "mozilla/Services.h"
#include "nsIObserverService.h"

class nsAXPCNativeCallContext;
struct JSContext;
struct JSObject;

struct nsMessageListenerInfo
{
  nsCOMPtr<nsIFrameMessageListener> mListener;
  nsCOMPtr<nsIAtom> mMessage;
};

typedef bool (*nsLoadScriptCallback)(void* aCallbackData, const nsAString& aURL);
typedef bool (*nsSyncMessageCallback)(void* aCallbackData,
                                      const nsAString& aMessage,
                                      const nsAString& aJSON,
                                      InfallibleTArray<nsString>* aJSONRetVal);
typedef bool (*nsAsyncMessageCallback)(void* aCallbackData,
                                       const nsAString& aMessage,
                                       const nsAString& aJSON);

class nsFrameMessageManager : public nsIContentFrameMessageManager,
                              public nsIChromeFrameMessageManager
{
public:
  nsFrameMessageManager(PRBool aChrome,
                        nsSyncMessageCallback aSyncCallback,
                        nsAsyncMessageCallback aAsyncCallback,
                        nsLoadScriptCallback aLoadScriptCallback,
                        void* aCallbackData,
                        nsFrameMessageManager* aParentManager,
                        JSContext* aContext,
                        PRBool aGlobal = PR_FALSE,
                        PRBool aProcessManager = PR_FALSE)
  : mChrome(aChrome), mGlobal(aGlobal), mIsProcessManager(aProcessManager),
    mParentManager(aParentManager),
    mSyncCallback(aSyncCallback), mAsyncCallback(aAsyncCallback),
    mLoadScriptCallback(aLoadScriptCallback), mCallbackData(aCallbackData),
    mContext(aContext)
  {
    NS_ASSERTION(mContext || (aChrome && !aParentManager) || aProcessManager,
                 "Should have mContext in non-global/non-process manager!");
    NS_ASSERTION(aChrome || !aParentManager, "Should not set parent manager!");
    
    
    
    
    if (mParentManager && (mCallbackData || IsWindowLevel())) {
      mParentManager->AddChildManager(this);
    }
  }

  ~nsFrameMessageManager()
  {
    for (PRInt32 i = mChildManagers.Count(); i > 0; --i) {
      static_cast<nsFrameMessageManager*>(mChildManagers[i - 1])->
        Disconnect(PR_FALSE);
    }
    if (mIsProcessManager) {
      if (this == sParentProcessManager) {
        sParentProcessManager = nsnull;
      }
      if (this == sChildProcessManager) {
        sChildProcessManager = nsnull;
      }
    }
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFrameMessageManager,
                                           nsIContentFrameMessageManager)
  NS_DECL_NSIFRAMEMESSAGEMANAGER
  NS_DECL_NSISYNCMESSAGESENDER
  NS_DECL_NSICONTENTFRAMEMESSAGEMANAGER
  NS_DECL_NSICHROMEFRAMEMESSAGEMANAGER

  nsresult ReceiveMessage(nsISupports* aTarget, const nsAString& aMessage,
                          PRBool aSync, const nsAString& aJSON,
                          JSObject* aObjectsArray,
                          InfallibleTArray<nsString>* aJSONRetVal,
                          JSContext* aContext = nsnull);
  void AddChildManager(nsFrameMessageManager* aManager,
                       PRBool aLoadScripts = PR_TRUE);
  void RemoveChildManager(nsFrameMessageManager* aManager)
  {
    mChildManagers.RemoveObject(aManager);
  }

  void Disconnect(PRBool aRemoveFromParent = PR_TRUE);
  void SetCallbackData(void* aData, PRBool aLoadScripts = PR_TRUE);
  nsresult GetParamsForMessage(nsAString& aMessageName, nsAString& aJSON);
  nsresult SendAsyncMessageInternal(const nsAString& aMessage,
                                    const nsAString& aJSON);
  JSContext* GetJSContext() { return mContext; }
  nsFrameMessageManager* GetParentManager() { return mParentManager; }
  void SetParentManager(nsFrameMessageManager* aParent)
  {
    NS_ASSERTION(!mParentManager, "We have parent manager already!");
    NS_ASSERTION(mChrome, "Should not set parent manager!");
    mParentManager = aParent;
  }
  PRBool IsGlobal() { return mGlobal; }
  PRBool IsWindowLevel() { return mParentManager && mParentManager->IsGlobal(); }

  static nsFrameMessageManager* GetParentProcessManager()
  {
    return sParentProcessManager;
  }
  static nsFrameMessageManager* GetChildProcessManager()
  {
    return sChildProcessManager;
  }
protected:
  nsTArray<nsMessageListenerInfo> mListeners;
  nsCOMArray<nsIContentFrameMessageManager> mChildManagers;
  PRPackedBool mChrome;
  PRPackedBool mGlobal;
  PRPackedBool mIsProcessManager;
  nsFrameMessageManager* mParentManager;
  nsSyncMessageCallback mSyncCallback;
  nsAsyncMessageCallback mAsyncCallback;
  nsLoadScriptCallback mLoadScriptCallback;
  void* mCallbackData;
  JSContext* mContext;
  nsTArray<nsString> mPendingScripts;
public:
  static nsFrameMessageManager* sParentProcessManager;
  static nsFrameMessageManager* sChildProcessManager;
};

void
ContentScriptErrorReporter(JSContext* aCx,
                           const char* aMessage,
                           JSErrorReport* aReport);

class nsScriptCacheCleaner;

struct nsFrameScriptExecutorJSObjectHolder
{
  nsFrameScriptExecutorJSObjectHolder(JSObject* aObject) : mObject(aObject)
  { MOZ_COUNT_CTOR(nsFrameScriptExecutorJSObjectHolder); }
  ~nsFrameScriptExecutorJSObjectHolder()
  { MOZ_COUNT_DTOR(nsFrameScriptExecutorJSObjectHolder); }
  JSObject* mObject;
};

class nsFrameScriptExecutor
{
public:
  static void Shutdown();
protected:
  friend class nsFrameScriptCx;
  nsFrameScriptExecutor() : mCx(nsnull), mCxStackRefCnt(0),
                            mDelayedCxDestroy(PR_FALSE)
  { MOZ_COUNT_CTOR(nsFrameScriptExecutor); }
  ~nsFrameScriptExecutor()
  { MOZ_COUNT_DTOR(nsFrameScriptExecutor); }
  void DidCreateCx();
  
  void DestroyCx();
  void LoadFrameScriptInternal(const nsAString& aURL);
  static void Traverse(nsFrameScriptExecutor *tmp,
                       nsCycleCollectionTraversalCallback &cb);
  nsCOMPtr<nsIXPConnectJSObjectHolder> mGlobal;
  JSContext* mCx;
  PRUint32 mCxStackRefCnt;
  PRPackedBool mDelayedCxDestroy;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  static nsDataHashtable<nsStringHashKey, nsFrameScriptExecutorJSObjectHolder*>* sCachedScripts;
  static nsRefPtr<nsScriptCacheCleaner> sScriptCacheCleaner;
};

class nsFrameScriptCx
{
public:
  nsFrameScriptCx(nsISupports* aOwner, nsFrameScriptExecutor* aExec)
  : mOwner(aOwner), mExec(aExec)
  {
    ++(mExec->mCxStackRefCnt);
  }
  ~nsFrameScriptCx()
  {
    if (--(mExec->mCxStackRefCnt) == 0 &&
        mExec->mDelayedCxDestroy) {
      mExec->DestroyCx();
    }
  }
  nsCOMPtr<nsISupports> mOwner;
  nsFrameScriptExecutor* mExec;
};

class nsScriptCacheCleaner : public nsIObserver
{
  NS_DECL_ISUPPORTS

  nsScriptCacheCleaner()
  {
    nsCOMPtr<nsIObserverService> obsSvc = mozilla::services::GetObserverService();
    if (obsSvc)
      obsSvc->AddObserver(this, "xpcom-shutdown", PR_FALSE);
  }

  NS_IMETHODIMP Observe(nsISupports *aSubject,
                        const char *aTopic,
                        const PRUnichar *aData)
  {
    nsFrameScriptExecutor::Shutdown();
    return NS_OK;
  }
};

#endif
