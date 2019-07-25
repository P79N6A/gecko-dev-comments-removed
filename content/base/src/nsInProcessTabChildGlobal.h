





#ifndef nsInProcessTabChildGlobal_h
#define nsInProcessTabChildGlobal_h

#include "nsCOMPtr.h"
#include "nsFrameMessageManager.h"
#include "nsIScriptContext.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptContext.h"
#include "nsIClassInfo.h"
#include "jsapi.h"
#include "nsIDocShell.h"
#include "nsIDOMElement.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"

class nsInProcessTabChildGlobal : public nsDOMEventTargetHelper,
                                  public nsFrameScriptExecutor,
                                  public nsIInProcessContentFrameMessageManager,
                                  public nsIScriptObjectPrincipal,
                                  public nsIScriptContextPrincipal
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
                             const jsval& aObject,
                             JSContext* aCx,
                             uint8_t aArgc,
                             jsval* aRetval)
  {
    return mMessageManager
      ? mMessageManager->SendSyncMessage(aMessageName, aObject, aCx, aArgc, aRetval)
      : NS_ERROR_NULL_POINTER;
  }
  NS_IMETHOD GetContent(nsIDOMWindow** aContent);
  NS_IMETHOD GetDocShell(nsIDocShell** aDocShell);
  NS_IMETHOD Dump(const nsAString& aStr)
  {
    return mMessageManager ? mMessageManager->Dump(aStr) : NS_OK;
  }
  NS_IMETHOD PrivateNoteIntentionalCrash();
  NS_IMETHOD Btoa(const nsAString& aBinaryData,
                  nsAString& aAsciiBase64String);
  NS_IMETHOD Atob(const nsAString& aAsciiString,
                  nsAString& aBinaryData);

  NS_DECL_NSIINPROCESSCONTENTFRAMEMESSAGEMANAGER

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
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
                              uint8_t optional_argc)
  {
    return nsDOMEventTargetHelper::AddEventListener(aType, aListener,
                                                    aUseCapture,
                                                    aWantsUntrusted,
                                                    optional_argc);
  }

  virtual nsIScriptObjectPrincipal* GetObjectPrincipal() { return this; }
  virtual JSContext* GetJSContextForEventHandlers() { return mCx; }
  virtual nsIPrincipal* GetPrincipal() { return mPrincipal; }
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
protected:
  nsresult Init();
  nsresult InitTabChildGlobal();
  nsCOMPtr<nsIContentFrameMessageManager> mMessageManager;
  nsCOMPtr<nsIDocShell> mDocShell;
  bool mInitialized;
  bool mLoadingScript;
  bool mDelayedDisconnect;

  
  
  bool mIsBrowserFrame;
public:
  nsIContent* mOwner;
  nsFrameMessageManager* mChromeMessageManager;
  nsTArray<nsCOMPtr<nsIRunnable> > mASyncMessages;
};

#endif
