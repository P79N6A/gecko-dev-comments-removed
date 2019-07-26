






#ifndef nsPIDOMWindow_h__
#define nsPIDOMWindow_h__

#include "nsIDOMWindow.h"

#include "nsIDOMLocation.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMDocument.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIURI.h"

#define DOM_WINDOW_DESTROYED_TOPIC "dom-window-destroyed"
#define DOM_WINDOW_FROZEN_TOPIC "dom-window-frozen"
#define DOM_WINDOW_THAWED_TOPIC "dom-window-thawed"

class nsIIdleObserver;
class nsIPrincipal;
class nsPerformance;






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
template <class> class nsScriptObjectHolder;
class nsXBLPrototypeHandler;
class nsIArray;
class nsPIWindowRoot;

namespace mozilla {
namespace dom {
class AudioContext;
}
}

#define NS_PIDOMWINDOW_IID \
{0xf30405c2, 0x5da8, 0x4339, \
  {0x87, 0xe2, 0xfa, 0xb2, 0x51, 0x26, 0x8a, 0xe8}}

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

  virtual nsresult RegisterIdleObserver(nsIIdleObserver* aIdleObserver) = 0;
  virtual nsresult UnregisterIdleObserver(nsIIdleObserver* aIdleObserver) = 0;

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

  bool HasMutationListeners(uint32_t aMutationEventType) const
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

  void SetMutationListeners(uint32_t aType)
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
  nsIDocument* GetExtantDoc() const
  {
    return mDoc;
  }
  nsIURI* GetDocumentURI() const;
  nsIURI* GetDocBaseURI() const;

  nsIDocument* GetDoc()
  {
    if (!mDoc) {
      MaybeCreateDoc();
    }
    return mDoc;
  }

protected:
  
  
  void MaybeCreateDoc();

public:
  
  
  
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

  
  
  virtual void SetInitialPrincipalToSubject() = 0;

  virtual PopupControlState PushPopupControlState(PopupControlState aState,
                                                  bool aForce) const = 0;
  virtual void PopPopupControlState(PopupControlState state) const = 0;
  virtual PopupControlState GetPopupControlState() const = 0;

  
  
  virtual already_AddRefed<nsISupports> SaveWindowState() = 0;

  
  virtual nsresult RestoreWindowState(nsISupports *aState) = 0;

  
  virtual void SuspendTimeouts(uint32_t aIncrease = 1,
                               bool aFreezeChildren = true) = 0;

  
  virtual nsresult ResumeTimeouts(bool aThawChildren = true) = 0;

  virtual uint32_t TimeoutSuspendCount() = 0;

  
  
  virtual nsresult FireDelayedDOMEvents() = 0;

  virtual bool IsFrozen() const = 0;

  
  virtual nsresult SetTimeoutOrInterval(nsIScriptTimeoutHandler *aHandler,
                                        int32_t interval,
                                        bool aIsInterval, int32_t *aReturn) = 0;

  
  virtual nsresult ClearTimeoutOrInterval(int32_t aTimerID) = 0;

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

  


  virtual void DetachFromDocShell() = 0;

  











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

  




  virtual nsresult SetFullScreenInternal(bool aIsFullScreen, bool aRequireTrust) = 0;

  



  virtual void SetHasAudioAvailableEventListeners() = 0;

  



  bool HasMouseEnterLeaveEventListeners()
  {
    return mMayHaveMouseEnterLeaveEventListener;
  }

  



  void SetHasMouseEnterLeaveEventListeners()
  {
    mMayHaveMouseEnterLeaveEventListener = true;
  }

  virtual JSObject* GetCachedXBLPrototypeHandler(nsXBLPrototypeHandler* aKey) = 0;
  virtual void CacheXBLPrototypeHandler(nsXBLPrototypeHandler* aKey,
                                        nsScriptObjectHolder<JSObject>& aHandler) = 0;

  







  nsIContent* GetFocusedNode()
  {
    if (IsOuterWindow()) {
      return mInnerWindow ? mInnerWindow->mFocusedNode.get() : nullptr;
    }
    return mFocusedNode;
  }
  virtual void SetFocusedNode(nsIContent* aNode,
                              uint32_t aFocusMethod = 0,
                              bool aNeedsFocus = false) = 0;

  


  virtual uint32_t GetFocusMethod() = 0;

  









  virtual bool TakeFocus(bool aFocus, uint32_t aFocusMethod) = 0;

  



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

  


  virtual void EnableDeviceSensor(uint32_t aType) = 0;

  


  virtual void DisableDeviceSensor(uint32_t aType) = 0;

  virtual void EnableTimeChangeNotifications() = 0;
  virtual void DisableTimeChangeNotifications() = 0;

#ifdef MOZ_B2G
  



  virtual void EnableNetworkEvent(uint32_t aType) = 0;

  



  virtual void DisableNetworkEvent(uint32_t aType) = 0;
#endif 

  


  virtual void SetHasGamepadEventListener(bool aHasGamepad = true) = 0;

  






  virtual nsresult SetArguments(nsIArray *aArguments, nsIPrincipal *aOrigin) = 0;

  




  virtual uint32_t GetSerial() = 0;

  


  uint64_t WindowID() const { return mWindowID; }

  



  virtual bool DispatchCustomEvent(const char *aEventName) = 0;

  



  virtual void RefreshCompartmentPrincipal() = 0;

  


  virtual nsresult
  OpenNoNavigate(const nsAString& aUrl, const nsAString& aName,
                 const nsAString& aOptions, nsIDOMWindow **_retval) = 0;

  void AddAudioContext(mozilla::dom::AudioContext* aAudioContext);

  
  static bool HasPerformanceSupport();
  nsPerformance* GetPerformance();

protected:
  
  
  
  
  nsPIDOMWindow(nsPIDOMWindow *aOuterWindow);

  ~nsPIDOMWindow();

  void SetChromeEventHandlerInternal(nsIDOMEventTarget* aChromeEventHandler) {
    mChromeEventHandler = aChromeEventHandler;
    
    mParentTarget = nullptr;
  }

  virtual void UpdateParentTarget() = 0;

  
  void CreatePerformanceObjectIfNeeded();

  
  
  
  nsCOMPtr<nsIDOMEventTarget> mChromeEventHandler; 
  nsCOMPtr<nsIDOMDocument> mDocument; 
  nsCOMPtr<nsIDocument> mDoc; 
  
  nsCOMPtr<nsIURI> mDocumentURI; 
  nsCOMPtr<nsIURI> mDocBaseURI; 

  nsCOMPtr<nsIDOMEventTarget> mParentTarget; 

  
  nsCOMPtr<nsIDOMElement> mFrameElement;
  nsIDocShell           *mDocShell;  

  
  nsRefPtr<nsPerformance>       mPerformance;

  uint32_t               mModalStateDepth;

  
  nsTimeout             *mRunningTimeout;

  uint32_t               mMutationBits;

  bool                   mIsDocumentLoaded;
  bool                   mIsHandlingResizeEvent;
  bool                   mIsInnerWindow;
  bool                   mMayHavePaintEventListener;
  bool                   mMayHaveTouchEventListener;
  bool                   mMayHaveMouseEnterLeaveEventListener;

  
  
  bool                   mIsModalContentWindow;

  
  
  bool                   mIsActive;

  
  
  
  bool                   mIsBackground;

  
  nsPIDOMWindow         *mInnerWindow;
  nsCOMPtr<nsPIDOMWindow> mOuterWindow;

  
  
  nsCOMPtr<nsIContent> mFocusedNode;

  
  nsTArray<nsRefPtr<mozilla::dom::AudioContext> > mAudioContexts;

  
  
  uint64_t mWindowID;

  
  
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
  
  static void* operator new(size_t ) CPP_THROW_NEW { return nullptr; }
  static void operator delete(void* ) {}
};

#define nsAutoPopupStatePusher NS_AUTO_POPUP_STATE_PUSHER

#endif 
