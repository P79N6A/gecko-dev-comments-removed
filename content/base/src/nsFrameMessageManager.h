



#ifndef nsFrameMessageManager_h__
#define nsFrameMessageManager_h__

#include "nsIMessageManager.h"
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
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {

class ContentParent;
class ContentChild;
struct StructuredCloneData;
class ClonedMessageData;

namespace ipc {

enum MessageManagerFlags {
  MM_CHILD = 0,
  MM_CHROME = 1,
  MM_GLOBAL = 2,
  MM_PROCESSMANAGER = 4,
  MM_BROADCASTER = 8,
  MM_OWNSCALLBACK = 16
};

class MessageManagerCallback
{
public:
  virtual ~MessageManagerCallback() {}

  virtual bool DoLoadFrameScript(const nsAString& aURL)
  {
    return true;
  }

  virtual bool DoSendSyncMessage(const nsAString& aMessage,
                                 const mozilla::dom::StructuredCloneData& aData,
                                 InfallibleTArray<nsString>* aJSONRetVal)
  {
    return true;
  }

  virtual bool DoSendAsyncMessage(const nsAString& aMessage,
                                  const mozilla::dom::StructuredCloneData& aData)
  {
    return true;
  }

  virtual bool CheckPermission(const nsAString& aPermission)
  {
    return false;
  }

  virtual bool CheckManifestURL(const nsAString& aManifestURL)
  {
    return false;
  }

  virtual bool CheckAppHasPermission(const nsAString& aPermission)
  {
    return false;
  }

  virtual bool CheckAppHasStatus(unsigned short aStatus)
  {
    return false;
  }

protected:
  bool BuildClonedMessageDataForParent(ContentParent* aParent,
				       const StructuredCloneData& aData,
				       ClonedMessageData& aClonedData);
  bool BuildClonedMessageDataForChild(ContentChild* aChild,
				      const StructuredCloneData& aData,
				      ClonedMessageData& aClonedData);
};

StructuredCloneData UnpackClonedMessageDataForParent(const ClonedMessageData& aData);
StructuredCloneData UnpackClonedMessageDataForChild(const ClonedMessageData& aData);

} 
} 
} 

class nsAXPCNativeCallContext;
struct JSContext;
class JSObject;

struct nsMessageListenerInfo
{
  nsCOMPtr<nsIMessageListener> mListener;
  nsCOMPtr<nsIAtom> mMessage;
};


class nsFrameMessageManager MOZ_FINAL : public nsIContentFrameMessageManager,
                                        public nsIMessageBroadcaster,
                                        public nsIFrameScriptLoader,
                                        public nsIProcessChecker
{
  typedef mozilla::dom::StructuredCloneData StructuredCloneData;
public:
  nsFrameMessageManager(mozilla::dom::ipc::MessageManagerCallback* aCallback,
                        nsFrameMessageManager* aParentManager,
                         uint32_t aFlags)
  : mChrome(!!(aFlags & mozilla::dom::ipc::MM_CHROME)),
    mGlobal(!!(aFlags & mozilla::dom::ipc::MM_GLOBAL)),
    mIsProcessManager(!!(aFlags & mozilla::dom::ipc::MM_PROCESSMANAGER)),
    mIsBroadcaster(!!(aFlags & mozilla::dom::ipc::MM_BROADCASTER)),
    mOwnsCallback(!!(aFlags & mozilla::dom::ipc::MM_OWNSCALLBACK)),
    mHandlingMessage(false),
    mDisconnected(false),
    mCallback(aCallback),
    mParentManager(aParentManager)
  {
    NS_ASSERTION(mChrome || !aParentManager, "Should not set parent manager!");
    NS_ASSERTION(!mIsBroadcaster || !mCallback,
                 "Broadcasters cannot have callbacks!");
    
    
    
    
    if (mParentManager && (mCallback || IsWindowLevel())) {
      mParentManager->AddChildManager(this);
    }
    if (mOwnsCallback) {
      mOwnedCallback = aCallback;
    }
  }

  ~nsFrameMessageManager()
  {
    for (int32_t i = mChildManagers.Count(); i > 0; --i) {
      static_cast<nsFrameMessageManager*>(mChildManagers[i - 1])->
        Disconnect(false);
    }
    if (mIsProcessManager) {
      if (this == sParentProcessManager) {
        sParentProcessManager = nullptr;
      }
      if (this == sChildProcessManager) {
        sChildProcessManager = nullptr;
        delete sPendingSameProcessAsyncMessages;
        sPendingSameProcessAsyncMessages = nullptr;
      }
      if (this == sSameProcessParentManager) {
        sSameProcessParentManager = nullptr;
      }
    }
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFrameMessageManager,
                                           nsIContentFrameMessageManager)
  NS_DECL_NSIMESSAGELISTENERMANAGER
  NS_DECL_NSIMESSAGESENDER
  NS_DECL_NSIMESSAGEBROADCASTER
  NS_DECL_NSISYNCMESSAGESENDER
  NS_DECL_NSICONTENTFRAMEMESSAGEMANAGER
  NS_DECL_NSIFRAMESCRIPTLOADER
  NS_DECL_NSIPROCESSCHECKER

  static nsFrameMessageManager*
  NewProcessMessageManager(mozilla::dom::ContentParent* aProcess);

  nsresult ReceiveMessage(nsISupports* aTarget, const nsAString& aMessage,
                          bool aSync, const StructuredCloneData* aCloneData,
                          JS::Handle<JSObject*> aObjectsArray,
                          InfallibleTArray<nsString>* aJSONRetVal);

  void AddChildManager(nsFrameMessageManager* aManager,
                       bool aLoadScripts = true);
  void RemoveChildManager(nsFrameMessageManager* aManager)
  {
    mChildManagers.RemoveObject(aManager);
  }
  void Disconnect(bool aRemoveFromParent = true);

  void SetCallback(mozilla::dom::ipc::MessageManagerCallback* aCallback,
                   bool aLoadScripts = true);
  mozilla::dom::ipc::MessageManagerCallback* GetCallback()
  {
    return mCallback;
  }

  nsresult DispatchAsyncMessage(const nsAString& aMessageName,
                                const JS::Value& aObject,
                                JSContext* aCx,
                                uint8_t aArgc);
  nsresult DispatchAsyncMessageInternal(const nsAString& aMessage,
                                        const StructuredCloneData& aData);
  void RemoveFromParent();
  nsFrameMessageManager* GetParentManager() { return mParentManager; }
  void SetParentManager(nsFrameMessageManager* aParent)
  {
    NS_ASSERTION(!mParentManager, "We have parent manager already!");
    NS_ASSERTION(mChrome, "Should not set parent manager!");
    mParentManager = aParent;
  }
  bool IsGlobal() { return mGlobal; }
  bool IsWindowLevel() { return mParentManager && mParentManager->IsGlobal(); }

  static nsFrameMessageManager* GetParentProcessManager()
  {
    return sParentProcessManager;
  }
  static nsFrameMessageManager* GetChildProcessManager()
  {
    return sChildProcessManager;
  }
protected:
  friend class MMListenerRemover;
  nsTArray<nsMessageListenerInfo> mListeners;
  nsCOMArray<nsIContentFrameMessageManager> mChildManagers;
  bool mChrome;     
  bool mGlobal;     
  bool mIsProcessManager; 
  bool mIsBroadcaster; 
  bool mOwnsCallback;
  bool mHandlingMessage;
  bool mDisconnected;
  mozilla::dom::ipc::MessageManagerCallback* mCallback;
  nsAutoPtr<mozilla::dom::ipc::MessageManagerCallback> mOwnedCallback;
  nsFrameMessageManager* mParentManager;
  nsTArray<nsString> mPendingScripts;
public:
  static nsFrameMessageManager* sParentProcessManager;
  static nsFrameMessageManager* sChildProcessManager;
  static nsFrameMessageManager* sSameProcessParentManager;
  static nsTArray<nsCOMPtr<nsIRunnable> >* sPendingSameProcessAsyncMessages;
private:
  enum ProcessCheckerType {
    PROCESS_CHECKER_PERMISSION,
    PROCESS_CHECKER_MANIFEST_URL,
    ASSERT_APP_HAS_PERMISSION
  };
  nsresult AssertProcessInternal(ProcessCheckerType aType,
                                 const nsAString& aCapability,
                                 bool* aValid);
};

class nsScriptCacheCleaner;

struct nsFrameJSScriptExecutorHolder
{
  nsFrameJSScriptExecutorHolder(JSScript* aScript) : mScript(aScript)
  { MOZ_COUNT_CTOR(nsFrameJSScriptExecutorHolder); }
  ~nsFrameJSScriptExecutorHolder()
  { MOZ_COUNT_DTOR(nsFrameJSScriptExecutorHolder); }
  JSScript* mScript;
};

class nsFrameScriptExecutor
{
public:
  static void Shutdown();
  already_AddRefed<nsIXPConnectJSObjectHolder> GetGlobal()
  {
    nsCOMPtr<nsIXPConnectJSObjectHolder> ref = mGlobal;
    return ref.forget();
  }
protected:
  friend class nsFrameScriptCx;
  nsFrameScriptExecutor()
  { MOZ_COUNT_CTOR(nsFrameScriptExecutor); }
  ~nsFrameScriptExecutor()
  { MOZ_COUNT_DTOR(nsFrameScriptExecutor); }
  void DidCreateGlobal();
  void LoadFrameScriptInternal(const nsAString& aURL);
  enum CacheFailedBehavior { EXECUTE_IF_CANT_CACHE, DONT_EXECUTE };
  void TryCacheLoadAndCompileScript(const nsAString& aURL,
                                    CacheFailedBehavior aBehavior = DONT_EXECUTE);
  bool InitTabChildGlobalInternal(nsISupports* aScope, const nsACString& aID);
  nsCOMPtr<nsIXPConnectJSObjectHolder> mGlobal;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  static nsDataHashtable<nsStringHashKey, nsFrameJSScriptExecutorHolder*>* sCachedScripts;
  static nsScriptCacheCleaner* sScriptCacheCleaner;
};

class nsScriptCacheCleaner MOZ_FINAL : public nsIObserver
{
  NS_DECL_ISUPPORTS

  nsScriptCacheCleaner()
  {
    nsCOMPtr<nsIObserverService> obsSvc = mozilla::services::GetObserverService();
    if (obsSvc)
      obsSvc->AddObserver(this, "xpcom-shutdown", false);
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
