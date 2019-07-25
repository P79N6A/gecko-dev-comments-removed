



































#ifndef nsFrameMessageManager_h__
#define nsFrameMessageManager_h__

#include "nsIFrameMessageManager.h"
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
                                      nsTArray<nsString>* aJSONRetVal);
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
                        PRBool aGlobal = PR_FALSE)
  : mChrome(aChrome), mGlobal(aGlobal), mParentManager(aParentManager),
    mSyncCallback(aSyncCallback), mAsyncCallback(aAsyncCallback),
    mLoadScriptCallback(aLoadScriptCallback), mCallbackData(aCallbackData),
    mContext(aContext)
  {
    NS_ASSERTION(mContext || (aChrome && !aParentManager),
                 "Should have mContext in non-global manager!");
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
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFrameMessageManager,
                                           nsIContentFrameMessageManager)
  NS_DECL_NSIFRAMEMESSAGEMANAGER
  NS_DECL_NSICONTENTFRAMEMESSAGEMANAGER
  NS_DECL_NSICHROMEFRAMEMESSAGEMANAGER

  nsresult ReceiveMessage(nsISupports* aTarget, const nsAString& aMessage,
                          PRBool aSync, const nsAString& aJSON,
                          JSObject* aObjectsArray,
                          nsTArray<nsString>* aJSONRetVal,
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
protected:
  nsTArray<nsMessageListenerInfo> mListeners;
  nsCOMArray<nsIContentFrameMessageManager> mChildManagers;
  PRPackedBool mChrome;
  PRPackedBool mGlobal;
  nsFrameMessageManager* mParentManager;
  nsSyncMessageCallback mSyncCallback;
  nsAsyncMessageCallback mAsyncCallback;
  nsLoadScriptCallback mLoadScriptCallback;
  void* mCallbackData;
  JSContext* mContext;
  nsTArray<nsString> mPendingScripts;
};

struct nsFrameScriptExecutorJSObjectHolder
{
  nsFrameScriptExecutorJSObjectHolder(JSObject* aObject) : mObject(aObject) {}
  JSObject* mObject;
};

class nsFrameScriptExecutor
{
public:
  static void Shutdown();
protected:
  nsFrameScriptExecutor() : mCx(nsnull) {}
  void DidCreateCx();
  
  void DestroyCx();
  void LoadFrameScriptInternal(const nsAString& aURL);
  nsCOMPtr<nsIXPConnectJSObjectHolder> mGlobal;
  JSContext* mCx;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  static nsDataHashtable<nsStringHashKey, nsFrameScriptExecutorJSObjectHolder*>* sCachedScripts;
};

#endif
