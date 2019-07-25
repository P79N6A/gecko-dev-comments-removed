





































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
  NS_FORWARD_SAFE_NSIFRAMEMESSAGEMANAGER(mMessageManager)
  NS_IMETHOD SendSyncMessage()
  {
    return mMessageManager ? mMessageManager->SendSyncMessage()
                           : NS_ERROR_NULL_POINTER;
  }
  NS_IMETHOD GetContent(nsIDOMWindow** aContent);
  NS_IMETHOD GetDocShell(nsIDocShell** aDocShell);
  NS_IMETHOD Dump(const nsAString& aStr)
  {
    return mMessageManager ? mMessageManager->Dump(aStr) : NS_OK;
  }
  NS_IMETHOD PrivateNoteIntentionalCrash();
  NS_DECL_NSIINPROCESSCONTENTFRAMEMESSAGEMANAGER

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              PRBool aUseCapture)
  {
    
    return nsDOMEventTargetHelper::AddEventListener(aType, aListener,
                                                    aUseCapture, PR_FALSE, 2);
  }
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              PRBool aUseCapture, PRBool aWantsUntrusted,
                              PRUint8 optional_argc)
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
  void SendMessageToParent(const nsString& aMessage, PRBool aSync,
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
  PRPackedBool mInitialized;
  PRPackedBool mLoadingScript;
  PRPackedBool mDelayedDisconnect;
public:
  nsIContent* mOwner;
  nsFrameMessageManager* mChromeMessageManager;
  nsTArray<nsCOMPtr<nsIRunnable> > mASyncMessages;
};

#endif
