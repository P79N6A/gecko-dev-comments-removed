






































#ifndef nsPIDOMWindow_h__
#define nsPIDOMWindow_h__

#include "nsIDOMWindow.h"

#include "nsIDOMLocation.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMElement.h"
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
{ 0x8ce567b5, 0xcc8d, 0x410b, \
  { 0xa2, 0x7b, 0x07, 0xaf, 0x31, 0xc0, 0x33, 0xb8 } }

class nsPIDOMWindow : public nsIDOMWindowInternal
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMWINDOW_IID)

  virtual nsPIDOMWindow* GetPrivateRoot() = 0;

  virtual void ActivateOrDeactivate(bool aActivate) = 0;

  
  virtual already_AddRefed<nsPIWindowRoot> GetTopWindowRoot() = 0;

  virtual void SetActive(bool aActive)
  {
    NS_PRECONDITION(IsOuterWindow(),
                    "active state is only maintained on outer windows");
    mIsActive = aActive;
  }

  bool IsActive()
  {
    NS_PRECONDITION(IsOuterWindow(),
                    "active state is only maintained on outer windows");
    return mIsActive;
  }

  virtual void SetIsBackground(bool aIsBackground)
  {
    NS_PRECONDITION(IsOuterWindow(),
                    "background state is only maintained on outer windows");
    mIsBackground = aIsBackground;
  }

  bool IsBackground()
  {
    NS_PRECONDITION(IsOuterWindow(),
                    "background state is only maintained on outer windows");
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

  bool HasMutationListeners(PRUint32 aMutationEventType) const
  {
    const nsPIDOMWindow *win;

    if (IsOuterWindow()) {
      win = GetCurrentInnerWindow();

      if (!win) {
        NS_ERROR("No current inner window available!");

        return false;
      }
    } else {
      if (!mOuterWindow) {
        NS_ERROR("HasMutationListeners() called on orphan inner window!");

        return false;
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

  bool IsLoadingOrRunningTimeout() const
  {
    const nsPIDOMWindow *win = GetCurrentInnerWindow();

    if (!win) {
      win = this;
    }

    return !win->mIsDocumentLoaded || win->mRunningTimeout;
  }

  
  bool IsLoading() const
  {
    const nsPIDOMWindow *win;

    if (IsOuterWindow()) {
      win = GetCurrentInnerWindow();

      if (!win) {
        NS_ERROR("No current inner window available!");

        return false;
      }
    } else {
      if (!mOuterWindow) {
        NS_ERROR("IsLoading() called on orphan inner window!");

        return false;
      }

      win = this;
    }

    return !win->mIsDocumentLoaded;
  }

  bool IsHandlingResizeEvent() const
  {
    const nsPIDOMWindow *win;

    if (IsOuterWindow()) {
      win = GetCurrentInnerWindow();

      if (!win) {
        NS_ERROR("No current inner window available!");

        return false;
      }
    } else {
      if (!mOuterWindow) {
        NS_ERROR("IsHandlingResizeEvent() called on orphan inner window!");

        return false;
      }

      win = this;
    }

    return win->mIsHandlingResizeEvent;
  }

  
  
  
  
  
  virtual void SetOpenerScriptPrincipal(nsIPrincipal* aPrincipal) = 0;
  
  virtual nsIPrincipal* GetOpenerScriptPrincipal() = 0;

  virtual PopupControlState PushPopupControlState(PopupControlState aState,
                                                  bool aForce) const = 0;
  virtual void PopPopupControlState(PopupControlState state) const = 0;
  virtual PopupControlState GetPopupControlState() const = 0;

  
  
  virtual nsresult SaveWindowState(nsISupports **aState) = 0;

  
  virtual nsresult RestoreWindowState(nsISupports *aState) = 0;

  
  virtual void SuspendTimeouts(PRUint32 aIncrease = 1,
                               bool aFreezeChildren = true) = 0;

  
  virtual nsresult ResumeTimeouts(bool aThawChildren = true) = 0;

  virtual PRUint32 TimeoutSuspendCount() = 0;

  
  
  virtual nsresult FireDelayedDOMEvents() = 0;

  virtual bool IsFrozen() const = 0;

  
  virtual nsresult SetTimeoutOrInterval(nsIScriptTimeoutHandler *aHandler,
                                        PRInt32 interval,
                                        bool aIsInterval, PRInt32 *aReturn) = 0;

  
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

  bool IsInnerWindow() const
  {
    return mIsInnerWindow;
  }

  bool IsOuterWindow() const
  {
    return !IsInnerWindow();
  }

  virtual bool WouldReuseInnerWindow(nsIDocument *aNewDocument) = 0;

  


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
                                  bool aForceReuseInnerWindow) = 0;

  






  virtual void SetOpenerWindow(nsIDOMWindow* aOpener,
                               bool aOriginalOpener) = 0;

  virtual void EnsureSizeUpToDate() = 0;

  



  virtual nsIDOMWindow *EnterModalState() = 0;
  virtual void LeaveModalState(nsIDOMWindow *) = 0;

  virtual bool CanClose() = 0;
  virtual nsresult ForceClose() = 0;

  bool IsModalContentWindow() const
  {
    return mIsModalContentWindow;
  }

  



  void SetHasPaintEventListeners()
  {
    mMayHavePaintEventListener = true;
  }

  



  bool HasPaintEventListeners()
  {
    return mMayHavePaintEventListener;
  }
  
  



  void SetHasTouchEventListeners()
  {
    mMayHaveTouchEventListener = true;
    MaybeUpdateTouchState();
  }

  bool HasTouchEventListeners()
  {
    return mMayHaveTouchEventListener;
  }

  



  bool HasAudioAvailableEventListeners()
  {
    return mMayHaveAudioAvailableEventListener;
  }

  



  void SetHasAudioAvailableEventListeners()
  {
    mMayHaveAudioAvailableEventListener = true;
  }

  



  bool HasMouseEnterLeaveEventListeners()
  {
    return mMayHaveMouseEnterLeaveEventListener;
  }

  



  void SetHasMouseEnterLeaveEventListeners()
  {
    mMayHaveMouseEnterLeaveEventListener = true;
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
                              bool aNeedsFocus = false) = 0;

  


  virtual PRUint32 GetFocusMethod() = 0;

  









  virtual bool TakeFocus(bool aFocus, PRUint32 aFocusMethod) = 0;

  



  virtual void SetReadyForFocus() = 0;

  


  virtual bool ShouldShowFocusRing() = 0;

  


  virtual void SetKeyboardIndicators(UIStateChangeType aShowAccelerators,
                                     UIStateChangeType aShowFocusRings) = 0;

  


  virtual void GetKeyboardIndicators(bool* aShowAccelerators,
                                     bool* aShowFocusRings) = 0;

  



  virtual void PageHidden() = 0;

  



  virtual nsresult DispatchAsyncHashchange(nsIURI *aOldURI,
                                           nsIURI *aNewURI) = 0;

  


  virtual nsresult DispatchSyncPopState() = 0;

  


  virtual void SetHasOrientationEventListener() = 0;

  


  virtual void RemoveOrientationEventListener() = 0;

  






  virtual nsresult SetArguments(nsIArray *aArguments, nsIPrincipal *aOrigin) = 0;

  




  virtual PRUint32 GetSerial() = 0;

  


  PRUint64 WindowID() const { return mWindowID; }

  



  virtual bool DispatchCustomEvent(const char *aEventName) = 0;

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

  bool                   mIsDocumentLoaded;
  bool                   mIsHandlingResizeEvent;
  bool                   mIsInnerWindow;
  bool                   mMayHavePaintEventListener;
  bool                   mMayHaveTouchEventListener;
  bool                   mMayHaveAudioAvailableEventListener;
  bool                   mMayHaveMouseEnterLeaveEventListener;

  
  
  bool                   mIsModalContentWindow;

  
  
  bool                   mIsActive;

  
  
  
  bool                   mIsBackground;

  
  nsPIDOMWindow         *mInnerWindow;
  nsCOMPtr<nsPIDOMWindow> mOuterWindow;

  
  
  nsCOMPtr<nsIContent> mFocusedNode;

  
  
  PRUint64 mWindowID;

  
  
  bool mHasNotifiedGlobalCreated;
};


NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMWindow, NS_PIDOMWINDOW_IID)

#ifdef _IMPL_NS_LAYOUT
PopupControlState
PushPopupControlState(PopupControlState aState, bool aForce);

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
  NS_AUTO_POPUP_STATE_PUSHER(PopupControlState aState, bool aForce = false)
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
      mOldState = aWindow->PushPopupControlState(aState, false);
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
