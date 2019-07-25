






































#ifndef nsPIDOMWindow_h__
#define nsPIDOMWindow_h__

#include "nsISupports.h"
#include "nsIDOMLocation.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMDocument.h"
#include "nsCOMPtr.h"
#include "nsEvent.h"
#include "nsIURI.h"

#define DOM_WINDOW_DESTROYED_TOPIC "dom-window-destroyed"
#define DOM_WINDOW_FROZEN_TOPIC "dom-window-frozen"
#define DOM_WINDOW_THAWED_TOPIC "dom-window-thawed"

class nsIPrincipal;






enum PopupControlState {
  openAllowed = 0,  
  openControlled,   
  openAbused,       
  openOverridden    
};

class nsIDocShell;
class nsIContent;
class nsIDocument;
class nsIScriptTimeoutHandler;
struct nsTimeout;
class nsScriptObjectHolder;
class nsXBLPrototypeHandler;
class nsIArray;
class nsPIWindowRoot;

#define NS_PIDOMWINDOW_IID \
{ 0x6c05ae9d, 0x4ad1, 0x4e92, \
  { 0x9c, 0x95, 0xd3, 0x54, 0xea, 0x0f, 0xb9, 0x48 } }

class nsPIDOMWindow : public nsIDOMWindowInternal
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMWINDOW_IID)

  virtual nsPIDOMWindow* GetPrivateRoot() = 0;

  virtual void ActivateOrDeactivate(PRBool aActivate) = 0;

  
  virtual already_AddRefed<nsPIWindowRoot> GetTopWindowRoot() = 0;

  virtual void SetActive(PRBool aActive)
  {
    mIsActive = aActive;
  }

  PRBool IsActive()
  {
    return mIsActive;
  }

  virtual void SetIsBackground(PRBool aIsBackground)
  {
    mIsBackground = aIsBackground;
  }

  PRBool IsBackground()
  {
    return mIsBackground;
  }

  nsIDOMEventTarget* GetChromeEventHandler() const
  {
    return mChromeEventHandler;
  }

  virtual void SetChromeEventHandler(nsIDOMEventTarget* aChromeEventHandler) = 0;

  nsIDOMEventTarget* GetParentTarget()
  {
    if (!mParentTarget) {
      UpdateParentTarget();
    }
    return mParentTarget;
  }

  PRBool HasMutationListeners(PRUint32 aMutationEventType) const
  {
    const nsPIDOMWindow *win;

    if (IsOuterWindow()) {
      win = GetCurrentInnerWindow();

      if (!win) {
        NS_ERROR("No current inner window available!");

        return PR_FALSE;
      }
    } else {
      if (!mOuterWindow) {
        NS_ERROR("HasMutationListeners() called on orphan inner window!");

        return PR_FALSE;
      }

      win = this;
    }

    return (win->mMutationBits & aMutationEventType) != 0;
  }

  void SetMutationListeners(PRUint32 aType)
  {
    nsPIDOMWindow *win;

    if (IsOuterWindow()) {
      win = GetCurrentInnerWindow();

      if (!win) {
        NS_ERROR("No inner window available to set mutation bits on!");

        return;
      }
    } else {
      if (!mOuterWindow) {
        NS_ERROR("HasMutationListeners() called on orphan inner window!");

        return;
      }

      win = this;
    }

    win->mMutationBits |= aType;
  }

  virtual void MaybeUpdateTouchState() {}
  virtual void UpdateTouchState() {}

  
  nsIDOMDocument* GetExtantDocument() const
  {
    return mDocument;
  }

  
  
  
  nsIDOMElement* GetFrameElementInternal() const
  {
    if (mOuterWindow) {
      return mOuterWindow->GetFrameElementInternal();
    }

    NS_ASSERTION(!IsInnerWindow(),
                 "GetFrameElementInternal() called on orphan inner window");

    return mFrameElement;
  }

  void SetFrameElementInternal(nsIDOMElement *aFrameElement)
  {
    if (IsOuterWindow()) {
      mFrameElement = aFrameElement;

      return;
    }

    if (!mOuterWindow) {
      NS_ERROR("frameElement set on inner window with no outer!");

      return;
    }

    mOuterWindow->SetFrameElementInternal(aFrameElement);
  }

  PRBool IsLoadingOrRunningTimeout() const
  {
    const nsPIDOMWindow *win = GetCurrentInnerWindow();

    if (!win) {
      win = this;
    }

    return !win->mIsDocumentLoaded || win->mRunningTimeout;
  }

  
  PRBool IsLoading() const
  {
    const nsPIDOMWindow *win;

    if (IsOuterWindow()) {
      win = GetCurrentInnerWindow();

      if (!win) {
        NS_ERROR("No current inner window available!");

        return PR_FALSE;
      }
    } else {
      if (!mOuterWindow) {
        NS_ERROR("IsLoading() called on orphan inner window!");

        return PR_FALSE;
      }

      win = this;
    }

    return !win->mIsDocumentLoaded;
  }

  PRBool IsHandlingResizeEvent() const
  {
    const nsPIDOMWindow *win;

    if (IsOuterWindow()) {
      win = GetCurrentInnerWindow();

      if (!win) {
        NS_ERROR("No current inner window available!");

        return PR_FALSE;
      }
    } else {
      if (!mOuterWindow) {
        NS_ERROR("IsHandlingResizeEvent() called on orphan inner window!");

        return PR_FALSE;
      }

      win = this;
    }

    return win->mIsHandlingResizeEvent;
  }

  
  
  
  
  
  virtual void SetOpenerScriptPrincipal(nsIPrincipal* aPrincipal) = 0;
  
  virtual nsIPrincipal* GetOpenerScriptPrincipal() = 0;

  virtual PopupControlState PushPopupControlState(PopupControlState aState,
                                                  PRBool aForce) const = 0;
  virtual void PopPopupControlState(PopupControlState state) const = 0;
  virtual PopupControlState GetPopupControlState() const = 0;

  
  
  virtual nsresult SaveWindowState(nsISupports **aState) = 0;

  
  virtual nsresult RestoreWindowState(nsISupports *aState) = 0;

  
  virtual void SuspendTimeouts(PRUint32 aIncrease = 1,
                               PRBool aFreezeChildren = PR_TRUE) = 0;

  
  virtual nsresult ResumeTimeouts(PRBool aThawChildren = PR_TRUE) = 0;

  virtual PRUint32 TimeoutSuspendCount() = 0;

  
  
  virtual nsresult FireDelayedDOMEvents() = 0;

  virtual PRBool IsFrozen() const = 0;

  
  virtual nsresult SetTimeoutOrInterval(nsIScriptTimeoutHandler *aHandler,
                                        PRInt32 interval,
                                        PRBool aIsInterval, PRInt32 *aReturn) = 0;

  
  virtual nsresult ClearTimeoutOrInterval(PRInt32 aTimerID) = 0;

  nsPIDOMWindow *GetOuterWindow()
  {
    return mIsInnerWindow ? mOuterWindow.get() : this;
  }

  nsPIDOMWindow *GetCurrentInnerWindow() const
  {
    return mInnerWindow;
  }

  nsPIDOMWindow *EnsureInnerWindow()
  {
    NS_ASSERTION(IsOuterWindow(), "EnsureInnerWindow called on inner window");
    
    nsCOMPtr<nsIDOMDocument> doc;
    GetDocument(getter_AddRefs(doc));
    return GetCurrentInnerWindow();
  }

  PRBool IsInnerWindow() const
  {
    return mIsInnerWindow;
  }

  PRBool IsOuterWindow() const
  {
    return !IsInnerWindow();
  }

  virtual PRBool WouldReuseInnerWindow(nsIDocument *aNewDocument) = 0;

  


  nsIDocShell *GetDocShell()
  {
    if (mOuterWindow) {
      return mOuterWindow->mDocShell;
    }

    return mDocShell;
  }

  


  virtual void SetDocShell(nsIDocShell *aDocShell) = 0;

  









  virtual nsresult SetNewDocument(nsIDocument *aDocument,
                                  nsISupports *aState,
                                  PRBool aForceReuseInnerWindow) = 0;

  






  virtual void SetOpenerWindow(nsIDOMWindowInternal *aOpener,
                               PRBool aOriginalOpener) = 0;

  virtual void EnsureSizeUpToDate() = 0;

  



  virtual nsIDOMWindow *EnterModalState() = 0;
  virtual void LeaveModalState(nsIDOMWindow *) = 0;

  virtual PRBool CanClose() = 0;
  virtual nsresult ForceClose() = 0;

  PRBool IsModalContentWindow() const
  {
    return mIsModalContentWindow;
  }

  



  void SetHasPaintEventListeners()
  {
    mMayHavePaintEventListener = PR_TRUE;
  }

  



  PRBool HasPaintEventListeners()
  {
    return mMayHavePaintEventListener;
  }
  
  



  void SetHasTouchEventListeners()
  {
    mMayHaveTouchEventListener = PR_TRUE;
    MaybeUpdateTouchState();
  }

  PRBool HasTouchEventListeners()
  {
    return mMayHaveTouchEventListener;
  }

  



  PRBool HasAudioAvailableEventListeners()
  {
    return mMayHaveAudioAvailableEventListener;
  }

  



  void SetHasAudioAvailableEventListeners()
  {
    mMayHaveAudioAvailableEventListener = PR_TRUE;
  }

  


  virtual void InitJavaProperties() = 0;

  virtual void* GetCachedXBLPrototypeHandler(nsXBLPrototypeHandler* aKey) = 0;
  virtual void CacheXBLPrototypeHandler(nsXBLPrototypeHandler* aKey,
                                        nsScriptObjectHolder& aHandler) = 0;

  







  nsIContent* GetFocusedNode()
  {
    if (IsOuterWindow()) {
      return mInnerWindow ? mInnerWindow->mFocusedNode.get() : nsnull;
    }
    return mFocusedNode;
  }
  virtual void SetFocusedNode(nsIContent* aNode,
                              PRUint32 aFocusMethod = 0,
                              PRBool aNeedsFocus = PR_FALSE) = 0;

  


  virtual PRUint32 GetFocusMethod() = 0;

  









  virtual PRBool TakeFocus(PRBool aFocus, PRUint32 aFocusMethod) = 0;

  



  virtual void SetReadyForFocus() = 0;

  


  virtual PRBool ShouldShowFocusRing() = 0;

  


  virtual void SetKeyboardIndicators(UIStateChangeType aShowAccelerators,
                                     UIStateChangeType aShowFocusRings) = 0;

  


  virtual void GetKeyboardIndicators(PRBool* aShowAccelerators,
                                     PRBool* aShowFocusRings) = 0;

  



  virtual void PageHidden() = 0;

  



  virtual nsresult DispatchAsyncHashchange(nsIURI *aOldURI,
                                           nsIURI *aNewURI) = 0;

  


  virtual nsresult DispatchSyncPopState() = 0;

  


  virtual void SetHasOrientationEventListener() = 0;

  






  virtual nsresult SetArguments(nsIArray *aArguments, nsIPrincipal *aOrigin) = 0;

  




  virtual PRUint32 GetSerial() = 0;

  


  PRUint64 WindowID() const { return mWindowID; }

protected:
  
  
  
  
  nsPIDOMWindow(nsPIDOMWindow *aOuterWindow);

  ~nsPIDOMWindow();

  void SetChromeEventHandlerInternal(nsIDOMEventTarget* aChromeEventHandler) {
    mChromeEventHandler = aChromeEventHandler;
    
    mParentTarget = nsnull;
  }

  virtual void UpdateParentTarget() = 0;

  
  
  
  nsCOMPtr<nsIDOMEventTarget> mChromeEventHandler; 
  nsCOMPtr<nsIDOMDocument> mDocument; 

  nsCOMPtr<nsIDOMEventTarget> mParentTarget; 

  
  nsCOMPtr<nsIDOMElement> mFrameElement;
  nsIDocShell           *mDocShell;  

  PRUint32               mModalStateDepth;

  
  nsTimeout             *mRunningTimeout;

  PRUint32               mMutationBits;

  PRPackedBool           mIsDocumentLoaded;
  PRPackedBool           mIsHandlingResizeEvent;
  PRPackedBool           mIsInnerWindow;
  PRPackedBool           mMayHavePaintEventListener;
  PRPackedBool           mMayHaveTouchEventListener;
  PRPackedBool           mMayHaveAudioAvailableEventListener;

  
  
  PRPackedBool           mIsModalContentWindow;

  
  
  PRPackedBool           mIsActive;

  
  
  
  PRPackedBool           mIsBackground;

  
  nsPIDOMWindow         *mInnerWindow;
  nsCOMPtr<nsPIDOMWindow> mOuterWindow;

  
  
  nsCOMPtr<nsIContent> mFocusedNode;

  
  
  PRUint64 mWindowID;

  
  
  PRPackedBool mHasNotifiedGlobalCreated;
};


NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMWindow, NS_PIDOMWINDOW_IID)

#ifdef _IMPL_NS_LAYOUT
PopupControlState
PushPopupControlState(PopupControlState aState, PRBool aForce);

void
PopPopupControlState(PopupControlState aState);

#define NS_AUTO_POPUP_STATE_PUSHER nsAutoPopupStatePusherInternal
#else
#define NS_AUTO_POPUP_STATE_PUSHER nsAutoPopupStatePusherExternal
#endif







class NS_AUTO_POPUP_STATE_PUSHER
{
public:
#ifdef _IMPL_NS_LAYOUT
  NS_AUTO_POPUP_STATE_PUSHER(PopupControlState aState, PRBool aForce = PR_FALSE)
    : mOldState(::PushPopupControlState(aState, aForce))
  {
  }

  ~NS_AUTO_POPUP_STATE_PUSHER()
  {
    PopPopupControlState(mOldState);
  }
#else
  NS_AUTO_POPUP_STATE_PUSHER(nsPIDOMWindow *aWindow, PopupControlState aState)
    : mWindow(aWindow), mOldState(openAbused)
  {
    if (aWindow) {
      mOldState = aWindow->PushPopupControlState(aState, PR_FALSE);
    }
  }

  ~NS_AUTO_POPUP_STATE_PUSHER()
  {
    if (mWindow) {
      mWindow->PopPopupControlState(mOldState);
    }
  }
#endif

protected:
#ifndef _IMPL_NS_LAYOUT
  nsCOMPtr<nsPIDOMWindow> mWindow;
#endif
  PopupControlState mOldState;

private:
  
  static void* operator new(size_t ) CPP_THROW_NEW { return nsnull; }
  static void operator delete(void* ) {}
};

#define nsAutoPopupStatePusher NS_AUTO_POPUP_STATE_PUSHER

#endif 
