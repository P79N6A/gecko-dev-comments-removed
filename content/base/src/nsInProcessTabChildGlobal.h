





#ifndef nsInProcessTabChildGlobal_h
#define nsInProcessTabChildGlobal_h

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsFrameMessageManager.h"
#include "nsIScriptContext.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptContext.h"
#include "nsIClassInfo.h"
#include "nsIDocShell.h"
#include "nsIDOMElement.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"
#include "nsIGlobalObject.h"
#include "nsWeakReference.h"

class nsInProcessTabChildGlobal : public nsDOMEventTargetHelper,
                                  public nsFrameScriptExecutor,
                                  public nsIInProcessContentFrameMessageManager,
                                  public nsIGlobalObject,
                                  public nsSupportsWeakReference,
                                  public mozilla::dom::ipc::MessageManagerCallback
{
public:
  nsInProcessTabChildGlobal(nsIDocShell* aShell, nsIContent* aOwner,
                            nsFrameMessageManager* aChrome);
  virtual ~nsInProcessTabChildGlobal();
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsInProcessTabChildGlobal,
                                           nsDOMEventTargetHelper)
  NS_FORWARD_SAFE_NSIMESSAGELISTENERMANAGER(mMessageManager)
  NS_FORWARD_SAFE_NSIMESSAGESENDER(mMessageManager)
  NS_IMETHOD SendSyncMessage(const nsAString& aMessageName,
                             const JS::Value& aObject,
                             const JS::Value& aRemote,
                             JSContext* aCx,
                             uint8_t aArgc,
                             JS::Value* aRetval)
  {
    return mMessageManager
      ? mMessageManager->SendSyncMessage(aMessageName, aObject, aRemote, aCx, aArgc, aRetval)
      : NS_ERROR_NULL_POINTER;
  }
  NS_IMETHOD GetContent(nsIDOMWindow** aContent) MOZ_OVERRIDE;
  NS_IMETHOD GetDocShell(nsIDocShell** aDocShell) MOZ_OVERRIDE;
  NS_IMETHOD Dump(const nsAString& aStr) MOZ_OVERRIDE
  {
    return mMessageManager ? mMessageManager->Dump(aStr) : NS_OK;
  }
  NS_IMETHOD PrivateNoteIntentionalCrash() MOZ_OVERRIDE;
  NS_IMETHOD Btoa(const nsAString& aBinaryData,
                  nsAString& aAsciiBase64String) MOZ_OVERRIDE;
  NS_IMETHOD Atob(const nsAString& aAsciiString,
                  nsAString& aBinaryData) MOZ_OVERRIDE;

  NS_DECL_NSIINPROCESSCONTENTFRAMEMESSAGEMANAGER

  


  virtual bool DoSendSyncMessage(JSContext* aCx,
                                 const nsAString& aMessage,
                                 const mozilla::dom::StructuredCloneData& aData,
                                 JS::Handle<JSObject *> aCpows,
                                 InfallibleTArray<nsString>* aJSONRetVal) MOZ_OVERRIDE;
  virtual bool DoSendAsyncMessage(JSContext* aCx,
                                  const nsAString& aMessage,
                                  const mozilla::dom::StructuredCloneData& aData,
                                  JS::Handle<JSObject *> aCpows);

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor) MOZ_OVERRIDE;
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              bool aUseCapture)
  {
    
    return nsDOMEventTargetHelper::AddEventListener(aType, aListener,
                                                    aUseCapture, false, 2);
  }
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              bool aUseCapture, bool aWantsUntrusted,
                              uint8_t optional_argc) MOZ_OVERRIDE
  {
    return nsDOMEventTargetHelper::AddEventListener(aType, aListener,
                                                    aUseCapture,
                                                    aWantsUntrusted,
                                                    optional_argc);
  }
  using nsDOMEventTargetHelper::AddEventListener;

  virtual JSContext* GetJSContextForEventHandlers() MOZ_OVERRIDE { return nsContentUtils::GetSafeJSContext(); }
  virtual nsIPrincipal* GetPrincipal() MOZ_OVERRIDE { return mPrincipal; }
  void LoadFrameScript(const nsAString& aURL);
  void Disconnect();
  void SendMessageToParent(const nsString& aMessage, bool aSync,
                           const nsString& aJSON,
                           nsTArray<nsString>* aJSONRetVal);
  nsFrameMessageManager* GetInnerManager()
  {
    return static_cast<nsFrameMessageManager*>(mMessageManager.get());
  }

  void SetOwner(nsIContent* aOwner) { mOwner = aOwner; }
  nsFrameMessageManager* GetChromeMessageManager()
  {
    return mChromeMessageManager;
  }
  void SetChromeMessageManager(nsFrameMessageManager* aParent)
  {
    mChromeMessageManager = aParent;
  }

  void DelayedDisconnect();

  virtual JSObject* GetGlobalJSObject() MOZ_OVERRIDE {
    if (!mGlobal) {
      return nullptr;
    }

    return mGlobal->GetJSObject();
  }
protected:
  nsresult Init();
  nsresult InitTabChildGlobal();
  nsCOMPtr<nsIContentFrameMessageManager> mMessageManager;
  nsCOMPtr<nsIDocShell> mDocShell;
  bool mInitialized;
  bool mLoadingScript;
  bool mDelayedDisconnect;

  
  
  
  bool mIsBrowserOrAppFrame;
public:
  nsIContent* mOwner;
  nsFrameMessageManager* mChromeMessageManager;
  nsTArray<nsCOMPtr<nsIRunnable> > mASyncMessages;
};

#endif
