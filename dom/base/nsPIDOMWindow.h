






#ifndef nsPIDOMWindow_h__
#define nsPIDOMWindow_h__

#include "nsIDOMWindow.h"

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "mozilla/dom/EventTarget.h"
#include "js/TypeDecls.h"

#define DOM_WINDOW_DESTROYED_TOPIC "dom-window-destroyed"
#define DOM_WINDOW_FROZEN_TOPIC "dom-window-frozen"
#define DOM_WINDOW_THAWED_TOPIC "dom-window-thawed"

class nsIArray;
class nsIContent;
class nsIDocShell;
class nsIDocument;
class nsIIdleObserver;
class nsIPrincipal;
class nsIScriptTimeoutHandler;
class nsIURI;
class nsPerformance;
class nsPIWindowRoot;
class nsXBLPrototypeHandler;
struct nsTimeout;

namespace mozilla {
namespace dom {
class AudioContext;
class Element;
}
namespace gfx {
class VRHMDInfo;
}
}






enum PopupControlState {
  openAllowed = 0,  
  openControlled,   
  openAbused,       
  openOverridden    
};

enum UIStateChangeType
{
  UIStateChangeType_NoChange,
  UIStateChangeType_Set,
  UIStateChangeType_Clear
};

#define NS_PIDOMWINDOW_IID \
{ 0x2485d4d7, 0xf7cb, 0x481e, \
  { 0x9c, 0x89, 0xb2, 0xa8, 0x12, 0x67, 0x7f, 0x97 } }

class nsPIDOMWindow : public nsIDOMWindowInternal
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIDOMWINDOW_IID)

  virtual nsPIDOMWindow* GetPrivateRoot() = 0;

  
  virtual void ActivateOrDeactivate(bool aActivate) = 0;

  
  virtual already_AddRefed<nsPIWindowRoot> GetTopWindowRoot() = 0;

  
  virtual nsresult RegisterIdleObserver(nsIIdleObserver* aIdleObserver) = 0;
  virtual nsresult UnregisterIdleObserver(nsIIdleObserver* aIdleObserver) = 0;

  
  virtual void SetActive(bool aActive)
  {
    MOZ_ASSERT(IsOuterWindow());
    mIsActive = aActive;
  }
  bool IsActive()
  {
    MOZ_ASSERT(IsOuterWindow());
    return mIsActive;
  }

  
  void SetDesktopModeViewport(bool aDesktopModeViewport)
  {
    MOZ_ASSERT(IsOuterWindow());
    mDesktopModeViewport = aDesktopModeViewport;
  }
  bool IsDesktopModeViewport() const
  {
    MOZ_ASSERT(IsOuterWindow());
    return mDesktopModeViewport;
  }

  virtual void SetIsBackground(bool aIsBackground)
  {
    MOZ_ASSERT(IsOuterWindow());
    mIsBackground = aIsBackground;
  }
  bool IsBackground()
  {
    MOZ_ASSERT(IsOuterWindow());
    return mIsBackground;
  }

  mozilla::dom::EventTarget* GetChromeEventHandler() const
  {
    return mChromeEventHandler;
  }

  
  virtual void SetChromeEventHandler(mozilla::dom::EventTarget* aChromeEventHandler) = 0;

  mozilla::dom::EventTarget* GetParentTarget()
  {
    if (!mParentTarget) {
      UpdateParentTarget();
    }
    return mParentTarget;
  }

  bool HasMutationListeners(uint32_t aMutationEventType) const
  {
    MOZ_ASSERT(IsInnerWindow());

    if (!mOuterWindow) {
      NS_ERROR("HasMutationListeners() called on orphan inner window!");

      return false;
    }

    return (mMutationBits & aMutationEventType) != 0;
  }

  void SetMutationListeners(uint32_t aType)
  {
    MOZ_ASSERT(IsInnerWindow());

    if (!mOuterWindow) {
      NS_ERROR("HasMutationListeners() called on orphan inner window!");

      return;
    }

    mMutationBits |= aType;
  }

  virtual void MaybeUpdateTouchState() {}

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

  virtual bool IsRunningTimeout() = 0;

  
  bool GetAudioMuted() const;
  void SetAudioMuted(bool aMuted);

  float GetAudioVolume() const;
  nsresult SetAudioVolume(float aVolume);

  float GetAudioGlobalVolume();

  virtual void SetServiceWorkersTestingEnabled(bool aEnabled)
  {
    MOZ_ASSERT(IsOuterWindow());
    mServiceWorkersTestingEnabled = aEnabled;
  }

  bool GetServiceWorkersTestingEnabled()
  {
    MOZ_ASSERT(IsOuterWindow());
    return mServiceWorkersTestingEnabled;
  }

protected:
  
  
  void MaybeCreateDoc();

  float GetAudioGlobalVolumeInternal(float aVolume);
  void RefreshMediaElements();

public:
  
  
  
  mozilla::dom::Element* GetFrameElementInternal() const;
  void SetFrameElementInternal(mozilla::dom::Element* aFrameElement);

  bool IsLoadingOrRunningTimeout() const
  {
    const nsPIDOMWindow* win = IsInnerWindow() ? this : GetCurrentInnerWindow();
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
    MOZ_ASSERT(IsOuterWindow());
    return mInnerWindow;
  }

  nsPIDOMWindow *EnsureInnerWindow()
  {
    NS_ASSERTION(IsOuterWindow(), "EnsureInnerWindow called on inner window");
    
    GetDoc();
    return GetCurrentInnerWindow();
  }

  bool IsInnerWindow() const
  {
    return mIsInnerWindow;
  }

  
  
  bool IsCurrentInnerWindow() const
  {
    MOZ_ASSERT(IsInnerWindow(),
               "It doesn't make sense to call this on outer windows.");
    return mOuterWindow && mOuterWindow->GetCurrentInnerWindow() == this;
  }

  
  
  
  bool HasActiveDocument()
  {
    MOZ_ASSERT(IsInnerWindow());
    return IsCurrentInnerWindow() ||
      (mOuterWindow &&
       mOuterWindow->GetCurrentInnerWindow() &&
       mOuterWindow->GetCurrentInnerWindow()->GetDoc() == mDoc);
  }

  bool IsOuterWindow() const
  {
    return !IsInnerWindow();
  }

  
  virtual bool WouldReuseInnerWindow(nsIDocument* aNewDocument) = 0;

  


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

  



  virtual void EnterModalState() = 0;
  virtual void LeaveModalState() = 0;

  
  virtual bool CanClose() = 0;
  virtual void ForceClose() = 0;

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
    if (!mMayHaveTouchEventListener) {
      mMayHaveTouchEventListener = true;
      MaybeUpdateTouchState();
    }
  }

  









  virtual nsresult SetFullScreenInternal(bool aIsFullScreen, bool aRequireTrust,
                                         mozilla::gfx::VRHMDInfo *aHMD = nullptr) = 0;

  



  bool HasMouseEnterLeaveEventListeners()
  {
    return mMayHaveMouseEnterLeaveEventListener;
  }

  



  void SetHasMouseEnterLeaveEventListeners()
  {
    mMayHaveMouseEnterLeaveEventListener = true;
  }

  



  bool HasPointerEnterLeaveEventListeners()
  {
    return mMayHavePointerEnterLeaveEventListener;
  }

  



  void SetHasPointerEnterLeaveEventListeners()
  {
    mMayHavePointerEnterLeaveEventListener = true;
  }


  virtual JSObject* GetCachedXBLPrototypeHandler(nsXBLPrototypeHandler* aKey) = 0;
  virtual void CacheXBLPrototypeHandler(nsXBLPrototypeHandler* aKey,
                                        JS::Handle<JSObject*> aHandler) = 0;

  







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

  











  virtual nsresult SetArguments(nsIArray *aArguments) = 0;

  




  virtual uint32_t GetSerial() = 0;

  


  uint64_t WindowID() const { return mWindowID; }

  





  virtual bool DispatchCustomEvent(const nsAString& aEventName) = 0;

  





  virtual void RefreshCompartmentPrincipal() = 0;

  




  virtual nsresult
  OpenNoNavigate(const nsAString& aUrl, const nsAString& aName,
                 const nsAString& aOptions, nsIDOMWindow **_retval) = 0;

  


  virtual void
  FirePopupBlockedEvent(nsIDocument* aDoc,
                        nsIURI* aPopupURI,
                        const nsAString& aPopupWindowName,
                        const nsAString& aPopupWindowFeatures) = 0;

  
  void AddAudioContext(mozilla::dom::AudioContext* aAudioContext);
  void RemoveAudioContext(mozilla::dom::AudioContext* aAudioContext);
  void MuteAudioContexts();
  void UnmuteAudioContexts();

  
  
  static nsPIDOMWindow* GetOuterFromCurrentInner(nsPIDOMWindow* aInner)
  {
    if (!aInner) {
      return nullptr;
    }

    nsPIDOMWindow* outer = aInner->GetOuterWindow();
    if (!outer || outer->GetCurrentInnerWindow() != aInner) {
      return nullptr;
    }

    return outer;
  }

  
  nsPerformance* GetPerformance();

  void MarkUncollectableForCCGeneration(uint32_t aGeneration)
  {
    mMarkedCCGeneration = aGeneration;
  }

  uint32_t GetMarkedCCGeneration()
  {
    return mMarkedCCGeneration;
  }

protected:
  
  
  
  
  explicit nsPIDOMWindow(nsPIDOMWindow *aOuterWindow);

  ~nsPIDOMWindow();

  void SetChromeEventHandlerInternal(mozilla::dom::EventTarget* aChromeEventHandler) {
    mChromeEventHandler = aChromeEventHandler;
    
    mParentTarget = nullptr;
  }

  virtual void UpdateParentTarget() = 0;

  
  
  void CreatePerformanceObjectIfNeeded();

  
  
  
  nsCOMPtr<mozilla::dom::EventTarget> mChromeEventHandler; 
  nsCOMPtr<nsIDocument> mDoc; 
  
  nsCOMPtr<nsIURI> mDocumentURI; 
  nsCOMPtr<nsIURI> mDocBaseURI; 

  nsCOMPtr<mozilla::dom::EventTarget> mParentTarget; 

  
  nsCOMPtr<mozilla::dom::Element> mFrameElement;
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
  bool                   mMayHavePointerEnterLeaveEventListener;

  
  
  bool                   mIsModalContentWindow;

  
  
  bool                   mIsActive;

  
  
  
  bool                   mIsBackground;

  bool                   mAudioMuted;
  float                  mAudioVolume;

  
  bool                   mDesktopModeViewport;

  
  nsPIDOMWindow* MOZ_NON_OWNING_REF mInnerWindow;
  nsCOMPtr<nsPIDOMWindow> mOuterWindow;

  
  
  nsCOMPtr<nsIContent> mFocusedNode;

  
  nsTArray<mozilla::dom::AudioContext*> mAudioContexts; 

  
  
  uint64_t mWindowID;

  
  
  bool mHasNotifiedGlobalCreated;

  uint32_t mMarkedCCGeneration;

  
  
  bool mServiceWorkersTestingEnabled;
};


NS_DEFINE_STATIC_IID_ACCESSOR(nsPIDOMWindow, NS_PIDOMWINDOW_IID)

#ifdef MOZILLA_INTERNAL_API
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
#ifdef MOZILLA_INTERNAL_API
  explicit NS_AUTO_POPUP_STATE_PUSHER(PopupControlState aState, bool aForce = false)
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
#ifndef MOZILLA_INTERNAL_API
  nsCOMPtr<nsPIDOMWindow> mWindow;
#endif
  PopupControlState mOldState;

private:
  
  static void* operator new(size_t ) CPP_THROW_NEW { return nullptr; }
  static void operator delete(void* ) {}
};

#define nsAutoPopupStatePusher NS_AUTO_POPUP_STATE_PUSHER

#endif 
