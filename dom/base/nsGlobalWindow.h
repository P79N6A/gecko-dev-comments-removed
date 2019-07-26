





#ifndef nsGlobalWindow_h___
#define nsGlobalWindow_h___

#include "nsPIDOMWindow.h"

#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"



#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"
#include "nsDataHashtable.h"
#include "nsJSThingHashtable.h"
#include "nsCycleCollectionParticipant.h"


#include "nsIBrowserDOMWindow.h"
#include "nsIDOMEventTarget.h"
#include "nsIInterfaceRequestor.h"
#include "nsIDOMJSWindow.h"
#include "nsIDOMChromeWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsITimer.h"
#include "nsIDOMModalContentWindow.h"
#include "nsEventListenerManager.h"
#include "nsIPrincipal.h"
#include "nsSize.h"
#include "mozFlushType.h"
#include "prclist.h"
#include "nsIDOMStorageEvent.h"
#include "nsFrameMessageManager.h"
#include "mozilla/LinkedList.h"
#include "mozilla/TimeStamp.h"
#include "nsIInlineEventHandlers.h"
#include "nsWrapperCacheInlines.h"
#include "nsIIdleObserver.h"
#include "nsIDocument.h"
#include "nsIDOMTouchEvent.h"

#include "mozilla/dom/EventTarget.h"
#include "Units.h"
#include "nsComponentManagerUtils.h"

#ifdef MOZ_B2G
#include "nsIDOMWindowB2G.h"
#endif 

#ifdef MOZ_WEBSPEECH
#include "nsISpeechSynthesisGetter.h"
#endif 

#define DEFAULT_HOME_PAGE "www.mozilla.org"
#define PREF_BROWSER_STARTUP_HOMEPAGE "browser.startup.homepage"



#define DEFAULT_SUCCESSIVE_DIALOG_TIME_LIMIT 3 // 3 sec



#define MAX_SUCCESSIVE_DIALOG_COUNT 5


#define MAX_IDLE_FUZZ_TIME_MS 90000


#define MIN_IDLE_NOTIFICATION_TIME_S 1

class nsIArray;
class nsIBaseWindow;
class nsIContent;
class nsIDocShellTreeOwner;
class nsIDOMCrypto;
class nsIDOMOfflineResourceList;
class nsIDOMMozWakeLock;
class nsIScrollableFrame;
class nsIControllers;
class nsIScriptContext;
class nsIScriptTimeoutHandler;
class nsIWebBrowserChrome;

class nsDOMWindowList;
class nsLocation;
class nsScreen;
class nsHistory;
class nsGlobalWindowObserver;
class nsGlobalWindow;
class nsDOMEventTargetHelper;
class nsDOMWindowUtils;
class nsIIdleService;
struct nsIntSize;
struct nsRect;

class nsWindowSizes;

namespace mozilla {
namespace dom {
class BarProp;
class Gamepad;
class Navigator;
class SpeechSynthesis;
namespace indexedDB {
class IDBFactory;
} 
} 
} 

extern nsresult
NS_CreateJSTimeoutHandler(nsGlobalWindow *aWindow,
                          bool *aIsInterval,
                          int32_t *aInterval,
                          nsIScriptTimeoutHandler **aRet);






struct nsTimeout : mozilla::LinkedListElement<nsTimeout>
{
  nsTimeout();
  ~nsTimeout();

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsTimeout)
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsTimeout)

  nsresult InitTimer(nsTimerCallbackFunc aFunc, uint32_t aDelay)
  {
    return mTimer->InitWithFuncCallback(aFunc, this, aDelay,
                                        nsITimer::TYPE_ONE_SHOT);
  }

  bool HasRefCntOne();

  
  nsRefPtr<nsGlobalWindow> mWindow;

  
  nsCOMPtr<nsITimer> mTimer;

  
  bool mCleared;

  
  bool mRunning;

  
  bool mIsInterval;

  
  uint32_t mPublicId;

  
  uint32_t mInterval;

  
  
  
  
  mozilla::TimeStamp mWhen;
  
  mozilla::TimeDuration mTimeRemaining;

  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  uint32_t mFiringDepth;

  
  uint32_t mNestingLevel;

  
  
  PopupControlState mPopupState;

  
  nsCOMPtr<nsIScriptTimeoutHandler> mScriptHandler;
};

struct IdleObserverHolder
{
  nsCOMPtr<nsIIdleObserver> mIdleObserver;
  uint32_t mTimeInS;
  bool mPrevNotificationIdle;

  IdleObserverHolder()
    : mTimeInS(0), mPrevNotificationIdle(false)
  {
    MOZ_COUNT_CTOR(IdleObserverHolder);
  }

  IdleObserverHolder(const IdleObserverHolder& aOther)
    : mIdleObserver(aOther.mIdleObserver), mTimeInS(aOther.mTimeInS),
      mPrevNotificationIdle(aOther.mPrevNotificationIdle)
  {
    MOZ_COUNT_CTOR(IdleObserverHolder);
  }

  bool operator==(const IdleObserverHolder& aOther) const {
    return
      mIdleObserver == aOther.mIdleObserver &&
      mTimeInS == aOther.mTimeInS;
  }

  ~IdleObserverHolder()
  {
    MOZ_COUNT_DTOR(IdleObserverHolder);
  }
};

static inline already_AddRefed<nsIVariant>
CreateVoidVariant()
{
  nsCOMPtr<nsIWritableVariant> writable =
    do_CreateInstance(NS_VARIANT_CONTRACTID);
  writable->SetAsVoid();
  return writable.forget();
}













class DialogValueHolder : public nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DialogValueHolder)

  DialogValueHolder(nsIPrincipal* aSubject, nsIVariant* aValue)
    : mOrigin(aSubject)
    , mValue(aValue) {}
  nsresult Get(nsIPrincipal* aSubject, nsIVariant** aResult)
  {
    nsCOMPtr<nsIVariant> result;
    if (aSubject->Subsumes(mOrigin)) {
      result = mValue;
    } else {
      result = CreateVoidVariant();
    }
    result.forget(aResult);
    return NS_OK;
  }
  virtual ~DialogValueHolder() {}
private:
  nsCOMPtr<nsIPrincipal> mOrigin;
  nsCOMPtr<nsIVariant> mValue;
};




















class nsGlobalWindow : public mozilla::dom::EventTarget,
                       public nsPIDOMWindow,
                       public nsIScriptGlobalObject,
                       public nsIScriptObjectPrincipal,
                       public nsIDOMJSWindow,
                       public nsSupportsWeakReference,
                       public nsIInterfaceRequestor,
                       public PRCListStr,
                       public nsIDOMWindowPerformance,
                       public nsITouchEventReceiver,
                       public nsIInlineEventHandlers
#ifdef MOZ_B2G
                     , public nsIDOMWindowB2G
#endif 
#ifdef MOZ_WEBSPEECH
                     , public nsISpeechSynthesisGetter
#endif 
{
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;
  typedef mozilla::dom::Navigator Navigator;
  typedef nsDataHashtable<nsUint64HashKey, nsGlobalWindow*> WindowByIdTable;

  
  nsPIDOMWindow* GetPrivateParent();
  
  void ReallyCloseWindow();

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  virtual JSObject *WrapObject(JSContext *cx,
                               JS::Handle<JSObject*> scope) MOZ_OVERRIDE
  {
    NS_ASSERTION(IsOuterWindow(),
                 "Inner window supports nsWrapperCache, fix WrapObject!");
    return EnsureInnerWindow() ? GetWrapper() : nullptr;
  }

  
  virtual JSObject *GetGlobalJSObject();

  
  virtual nsIScriptContext *GetContext();
  JSObject *FastGetGlobalJSObject()
  {
    return mJSObject;
  }
  void TraceGlobalJSObject(JSTracer* aTrc);

  virtual nsresult EnsureScriptEnvironment();

  virtual nsIScriptContext *GetScriptContext();

  void PoisonOuterWindowProxy(JSObject *aObject);
  virtual void OnFinalize(JSObject* aObject);
  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts);

  virtual bool IsBlackForCC();

  
  virtual nsIPrincipal* GetPrincipal();

  
  NS_DECL_NSIDOMWINDOW

#ifdef MOZ_B2G
  
  NS_DECL_NSIDOMWINDOWB2G
#endif 

#ifdef MOZ_WEBSPEECH
  
  NS_DECL_NSISPEECHSYNTHESISGETTER
#endif 

  
  NS_DECL_NSIDOMWINDOWPERFORMANCE

  
  NS_DECL_NSIDOMJSWINDOW

  
  NS_DECL_NSIDOMEVENTTARGET

  virtual nsEventListenerManager*
  GetExistingListenerManager() const MOZ_OVERRIDE;

  virtual nsEventListenerManager*
  GetOrCreateListenerManager() MOZ_OVERRIDE;

  using mozilla::dom::EventTarget::RemoveEventListener;
  virtual void AddEventListener(const nsAString& aType,
                                mozilla::dom::EventListener* aListener,
                                bool aUseCapture,
                                const mozilla::dom::Nullable<bool>& aWantsUntrusted,
                                mozilla::ErrorResult& aRv) MOZ_OVERRIDE;
  virtual nsIDOMWindow* GetOwnerGlobal() MOZ_OVERRIDE
  {
    if (IsOuterWindow()) {
      return this;
    }

    return GetOuterFromCurrentInner(this);
  }

  
  NS_DECL_NSITOUCHEVENTRECEIVER

  
  NS_DECL_NSIINLINEEVENTHANDLERS

  
  virtual NS_HIDDEN_(nsPIDOMWindow*) GetPrivateRoot();
  virtual NS_HIDDEN_(void) ActivateOrDeactivate(bool aActivate);
  virtual NS_HIDDEN_(void) SetActive(bool aActive);
  virtual NS_HIDDEN_(void) SetIsBackground(bool aIsBackground);
  virtual NS_HIDDEN_(void) SetChromeEventHandler(mozilla::dom::EventTarget* aChromeEventHandler);

  virtual NS_HIDDEN_(void) SetInitialPrincipalToSubject();

  virtual NS_HIDDEN_(PopupControlState) PushPopupControlState(PopupControlState state, bool aForce) const;
  virtual NS_HIDDEN_(void) PopPopupControlState(PopupControlState state) const;
  virtual NS_HIDDEN_(PopupControlState) GetPopupControlState() const;

  virtual already_AddRefed<nsISupports> SaveWindowState();
  virtual NS_HIDDEN_(nsresult) RestoreWindowState(nsISupports *aState);
  virtual NS_HIDDEN_(void) SuspendTimeouts(uint32_t aIncrease = 1,
                                           bool aFreezeChildren = true);
  virtual NS_HIDDEN_(nsresult) ResumeTimeouts(bool aThawChildren = true);
  virtual NS_HIDDEN_(uint32_t) TimeoutSuspendCount();
  virtual NS_HIDDEN_(nsresult) FireDelayedDOMEvents();
  virtual NS_HIDDEN_(bool) IsFrozen() const
  {
    return mIsFrozen;
  }

  virtual NS_HIDDEN_(bool) WouldReuseInnerWindow(nsIDocument *aNewDocument);

  virtual NS_HIDDEN_(void) SetDocShell(nsIDocShell* aDocShell);
  virtual void DetachFromDocShell();
  virtual NS_HIDDEN_(nsresult) SetNewDocument(nsIDocument *aDocument,
                                              nsISupports *aState,
                                              bool aForceReuseInnerWindow);
  void DispatchDOMWindowCreated();
  virtual NS_HIDDEN_(void) SetOpenerWindow(nsIDOMWindow* aOpener,
                                           bool aOriginalOpener);
  
  virtual NS_HIDDEN_(void) EnsureSizeUpToDate();

  virtual NS_HIDDEN_(void) EnterModalState();
  virtual NS_HIDDEN_(void) LeaveModalState();

  virtual NS_HIDDEN_(bool) CanClose();
  virtual NS_HIDDEN_(nsresult) ForceClose();

  virtual NS_HIDDEN_(void) MaybeUpdateTouchState();
  virtual NS_HIDDEN_(void) UpdateTouchState();
  virtual NS_HIDDEN_(bool) DispatchCustomEvent(const char *aEventName);
  virtual NS_HIDDEN_(bool) DispatchResizeEvent(const nsIntSize& aSize);
  virtual NS_HIDDEN_(void) RefreshCompartmentPrincipal();
  virtual NS_HIDDEN_(nsresult) SetFullScreenInternal(bool aIsFullScreen, bool aRequireTrust);

  virtual NS_HIDDEN_(void) SetHasGamepadEventListener(bool aHasGamepad = true);

  
  NS_DECL_NSIINTERFACEREQUESTOR

  
  uint32_t GetLength();
  already_AddRefed<nsIDOMWindow> IndexedGetter(uint32_t aIndex, bool& aFound);

  void GetSupportedNames(nsTArray<nsString>& aNames);

  
  nsGlobalWindow(nsGlobalWindow *aOuterWindow);

  static nsGlobalWindow *FromSupports(nsISupports *supports)
  {
    
    return (nsGlobalWindow *)(mozilla::dom::EventTarget *)supports;
  }
  static nsISupports *ToSupports(nsGlobalWindow *win)
  {
    
    return (nsISupports *)(mozilla::dom::EventTarget *)win;
  }
  static nsGlobalWindow *FromWrapper(nsIXPConnectWrappedNative *wrapper)
  {
    return FromSupports(wrapper->Native());
  }

  




  nsresult GetTop(nsIDOMWindow **aWindow)
  {
    return nsIDOMWindow::GetTop(aWindow);
  }

  inline nsGlobalWindow *GetTop()
  {
    nsCOMPtr<nsIDOMWindow> top;
    GetTop(getter_AddRefs(top));
    if (top)
      return static_cast<nsGlobalWindow *>(top.get());
    return nullptr;
  }

  inline nsGlobalWindow* GetScriptableTop()
  {
    nsCOMPtr<nsIDOMWindow> top;
    GetScriptableTop(getter_AddRefs(top));
    if (top) {
      return static_cast<nsGlobalWindow *>(top.get());
    }
    return nullptr;
  }

  already_AddRefed<nsIDOMWindow> GetChildWindow(const nsAString& aName);

  
  
  
  
  
  
  
  
  bool ShouldPromptToBlockDialogs();
  bool DialogsAreBeingAbused();

  
  
  
  bool ConfirmDialogIfNeeded();

  
  
  void EnableDialogs();
  void DisableDialogs();
  bool AreDialogsEnabled();

  virtual void SetHasAudioAvailableEventListeners();

  nsIScriptContext *GetContextInternal()
  {
    if (mOuterWindow) {
      return GetOuterWindowInternal()->mContext;
    }

    return mContext;
  }

  nsIScriptContext *GetScriptContextInternal(uint32_t aLangID)
  {
    NS_ASSERTION(aLangID == nsIProgrammingLanguage::JAVASCRIPT,
                 "We don't support this language ID");
    if (mOuterWindow) {
      return GetOuterWindowInternal()->mContext;
    }

    return mContext;
  }

  nsGlobalWindow *GetOuterWindowInternal()
  {
    return static_cast<nsGlobalWindow *>(GetOuterWindow());
  }

  nsGlobalWindow *GetCurrentInnerWindowInternal() const
  {
    return static_cast<nsGlobalWindow *>(mInnerWindow);
  }

  nsGlobalWindow *EnsureInnerWindowInternal()
  {
    return static_cast<nsGlobalWindow *>(EnsureInnerWindow());
  }

  bool IsCreatingInnerWindow() const
  {
    return  mCreatingInnerWindow;
  }

  bool IsChromeWindow() const
  {
    return mIsChrome;
  }

  
  
  nsIScrollableFrame *GetScrollFrame();

  nsresult Observe(nsISupports* aSubject, const char* aTopic,
                   const PRUnichar* aData);

  void UnblockScriptedClosing();

  static void Init();
  static void ShutDown();
  static void CleanupCachedXBLHandlers(nsGlobalWindow* aWindow);
  static bool IsCallerChrome();

  static void RunPendingTimeoutsRecursive(nsGlobalWindow *aTopWindow,
                                          nsGlobalWindow *aWindow);

  friend class WindowStateHolder;

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsGlobalWindow,
                                                                   nsIDOMEventTarget)

  virtual NS_HIDDEN_(JSObject*)
    GetCachedXBLPrototypeHandler(nsXBLPrototypeHandler* aKey);

  virtual NS_HIDDEN_(void)
    CacheXBLPrototypeHandler(nsXBLPrototypeHandler* aKey,
                             JS::Handle<JSObject*> aHandler);

  virtual bool TakeFocus(bool aFocus, uint32_t aFocusMethod);
  virtual void SetReadyForFocus();
  virtual void PageHidden();
  virtual nsresult DispatchAsyncHashchange(nsIURI *aOldURI, nsIURI *aNewURI);
  virtual nsresult DispatchSyncPopState();

  virtual void EnableDeviceSensor(uint32_t aType);
  virtual void DisableDeviceSensor(uint32_t aType);

  virtual void EnableTimeChangeNotifications();
  virtual void DisableTimeChangeNotifications();

#ifdef MOZ_B2G
  virtual void EnableNetworkEvent(uint32_t aType);
  virtual void DisableNetworkEvent(uint32_t aType);
#endif 

  virtual nsresult SetArguments(nsIArray *aArguments);

  void MaybeForgiveSpamCount();
  bool IsClosedOrClosing() {
    return (mIsClosed ||
            mInClose ||
            mHavePendingClose ||
            mCleanedUp);
  }

  static void FirePopupBlockedEvent(nsIDocument* aDoc,
                                    nsIDOMWindow *aRequestingWindow, nsIURI *aPopupURI,
                                    const nsAString &aPopupWindowName,
                                    const nsAString &aPopupWindowFeatures);

  virtual uint32_t GetSerial() {
    return mSerial;
  }

  static nsGlobalWindow* GetOuterWindowWithId(uint64_t aWindowID) {
    if (!sWindowsById) {
      return nullptr;
    }

    nsGlobalWindow* outerWindow = sWindowsById->Get(aWindowID);
    return outerWindow && !outerWindow->IsInnerWindow() ? outerWindow : nullptr;
  }

  static nsGlobalWindow* GetInnerWindowWithId(uint64_t aInnerWindowID) {
    if (!sWindowsById) {
      return nullptr;
    }

    nsGlobalWindow* innerWindow = sWindowsById->Get(aInnerWindowID);
    return innerWindow && innerWindow->IsInnerWindow() ? innerWindow : nullptr;
  }

  static WindowByIdTable* GetWindowsTable() {
    return sWindowsById;
  }

  void AddSizeOfIncludingThis(nsWindowSizes* aWindowSizes) const;

  void UnmarkGrayTimers();

  void AddEventTargetObject(nsDOMEventTargetHelper* aObject);
  void RemoveEventTargetObject(nsDOMEventTargetHelper* aObject);

  void NotifyIdleObserver(IdleObserverHolder* aIdleObserverHolder,
                          bool aCallOnidle);
  nsresult HandleIdleActiveEvent();
  bool ContainsIdleObserver(nsIIdleObserver* aIdleObserver, uint32_t timeInS);
  void HandleIdleObserverCallback();

  void AllowScriptsToClose()
  {
    mAllowScriptsToClose = true;
  }

  enum SlowScriptResponse {
    ContinueSlowScript = 0,
    AlwaysContinueSlowScript,
    KillSlowScript
  };
  SlowScriptResponse ShowSlowScriptDialog();

#ifdef MOZ_GAMEPAD
  void AddGamepad(uint32_t aIndex, mozilla::dom::Gamepad* aGamepad);
  void RemoveGamepad(uint32_t aIndex);
  void GetGamepads(nsTArray<nsRefPtr<mozilla::dom::Gamepad> >& aGamepads);
  already_AddRefed<mozilla::dom::Gamepad> GetGamepad(uint32_t aIndex);
  void SetHasSeenGamepadInput(bool aHasSeen);
  bool HasSeenGamepadInput();
  void SyncGamepadState();
  static PLDHashOperator EnumGamepadsForSync(const uint32_t& aKey,
                                             mozilla::dom::Gamepad* aData,
                                             void* aUserArg);
  static PLDHashOperator EnumGamepadsForGet(const uint32_t& aKey,
                                            mozilla::dom::Gamepad* aData,
                                            void* aUserArg);
#endif

  
  void EnableGamepadUpdates();
  void DisableGamepadUpdates();


#define EVENT(name_, id_, type_, struct_)                                     \
  mozilla::dom::EventHandlerNonNull* GetOn##name_()                           \
  {                                                                           \
    nsEventListenerManager *elm = GetExistingListenerManager();               \
    return elm ? elm->GetEventHandler(nsGkAtoms::on##name_, EmptyString())    \
               : nullptr;                                                     \
  }                                                                           \
  void SetOn##name_(mozilla::dom::EventHandlerNonNull* handler)               \
  {                                                                           \
    nsEventListenerManager *elm = GetOrCreateListenerManager();               \
    if (elm) {                                                                \
      elm->SetEventHandler(nsGkAtoms::on##name_, EmptyString(), handler);     \
    }                                                                         \
  }
#define ERROR_EVENT(name_, id_, type_, struct_)                               \
  mozilla::dom::OnErrorEventHandlerNonNull* GetOn##name_()                    \
  {                                                                           \
    nsEventListenerManager *elm = GetExistingListenerManager();               \
    return elm ? elm->GetOnErrorEventHandler() : nullptr;                     \
  }                                                                           \
  void SetOn##name_(mozilla::dom::OnErrorEventHandlerNonNull* handler)        \
  {                                                                           \
    nsEventListenerManager *elm = GetOrCreateListenerManager();               \
    if (elm) {                                                                \
      elm->SetEventHandler(handler);                                          \
    }                                                                         \
  }
#define BEFOREUNLOAD_EVENT(name_, id_, type_, struct_)                        \
  mozilla::dom::OnBeforeUnloadEventHandlerNonNull* GetOn##name_()             \
  {                                                                           \
    nsEventListenerManager *elm = GetExistingListenerManager();               \
    return elm ? elm->GetOnBeforeUnloadEventHandler() : nullptr;              \
  }                                                                           \
  void SetOn##name_(mozilla::dom::OnBeforeUnloadEventHandlerNonNull* handler) \
  {                                                                           \
    nsEventListenerManager *elm = GetOrCreateListenerManager();               \
    if (elm) {                                                                \
      elm->SetEventHandler(handler);                                          \
    }                                                                         \
  }
#define WINDOW_ONLY_EVENT EVENT
#define TOUCH_EVENT EVENT
#include "nsEventNameList.h"
#undef TOUCH_EVENT
#undef WINDOW_ONLY_EVENT
#undef BEFOREUNLOAD_EVENT
#undef ERROR_EVENT
#undef EVENT

#ifdef MOZ_WEBSPEECH
  mozilla::dom::SpeechSynthesis* GetSpeechSynthesisInternal();
#endif

  mozilla::dom::BarProp* Scrollbars();

protected:
  
  nsTObserverArray<IdleObserverHolder> mIdleObservers;

  
  nsCOMPtr<nsITimer> mIdleTimer;

  
  uint32_t mIdleFuzzFactor;

  
  
  int32_t mIdleCallbackIndex;

  
  
  bool mCurrentlyIdle;

  
  
  bool mAddActiveEventFuzzTime;

  nsCOMPtr <nsIIdleService> mIdleService;

  nsCOMPtr <nsIDOMMozWakeLock> mWakeLock;

  static bool sIdleObserversAPIFuzzTimeDisabled;

  friend class HashchangeCallback;
  friend class mozilla::dom::BarProp;

  
  virtual ~nsGlobalWindow();
  void CleanUp();
  void ClearControllers();
  nsresult FinalClose();

  inline void MaybeClearInnerWindow(nsGlobalWindow* aExpectedInner)
  {
    if(mInnerWindow == aExpectedInner) {
      mInnerWindow = nullptr;
    }
  }

  void FreeInnerObjects();
  JSObject *CallerGlobal();
  nsGlobalWindow *CallerInnerWindow();

  
  
  void InnerSetNewDocument(nsIDocument* aDocument);

  nsresult DefineArgumentsProperty(nsIArray *aArguments);

  
  nsIDOMWindow* GetParentInternal();

  
  bool IsPopupSpamWindow()
  {
    if (IsInnerWindow() && !mOuterWindow) {
      return false;
    }

    return GetOuterWindowInternal()->mIsPopupSpam;
  }

  void SetPopupSpamWindow(bool aPopup)
  {
    if (IsInnerWindow() && !mOuterWindow) {
      NS_ERROR("SetPopupSpamWindow() called on inner window w/o an outer!");

      return;
    }

    GetOuterWindowInternal()->mIsPopupSpam = aPopup;
  }

  

  virtual nsresult
  OpenNoNavigate(const nsAString& aUrl,
                 const nsAString& aName,
                 const nsAString& aOptions,
                 nsIDOMWindow **_retval);

  











































  NS_HIDDEN_(nsresult) OpenInternal(const nsAString& aUrl,
                                    const nsAString& aName,
                                    const nsAString& aOptions,
                                    bool aDialog,
                                    bool aContentModal,
                                    bool aCalledNoScript,
                                    bool aDoJSFixups,
                                    bool aNavigate,
                                    nsIArray *argv,
                                    nsISupports *aExtraArgument,
                                    nsIPrincipal *aCalleePrincipal,
                                    JSContext *aJSCallerContext,
                                    nsIDOMWindow **aReturn);

  
  
  
  nsresult SetTimeoutOrInterval(nsIScriptTimeoutHandler *aHandler,
                                int32_t interval,
                                bool aIsInterval, int32_t *aReturn);
  nsresult ClearTimeoutOrInterval(int32_t aTimerID);

  
  nsresult SetTimeoutOrInterval(bool aIsInterval, int32_t* aReturn);
  nsresult ResetTimersForNonBackgroundWindow();

  
  void RunTimeout(nsTimeout *aTimeout);
  void RunTimeout() { RunTimeout(nullptr); }
  
  bool RunTimeoutHandler(nsTimeout* aTimeout, nsIScriptContext* aScx);
  
  bool RescheduleTimeout(nsTimeout* aTimeout, const TimeStamp& now,
                         bool aRunningPendingTimeouts);

  void ClearAllTimeouts();
  
  
  void InsertTimeoutIntoList(nsTimeout *aTimeout);
  static void TimerCallback(nsITimer *aTimer, void *aClosure);

  
  already_AddRefed<nsIDocShellTreeOwner> GetTreeOwner();
  already_AddRefed<nsIBaseWindow> GetTreeOwnerWindow();
  already_AddRefed<nsIWebBrowserChrome> GetWebBrowserChrome();
  nsresult SecurityCheckURL(const char *aURL);
  nsresult BuildURIfromBase(const char *aURL,
                            nsIURI **aBuiltURI,
                            bool *aFreeSecurityPass, JSContext **aCXused);
  bool PopupWhitelisted();
  PopupControlState RevisePopupAbuseLevel(PopupControlState);
  void     FireAbuseEvents(bool aBlocked, bool aWindow,
                           const nsAString &aPopupURL,
                           const nsAString &aPopupWindowName,
                           const nsAString &aPopupWindowFeatures);
  void FireOfflineStatusEvent();

  nsresult ScheduleNextIdleObserverCallback();
  uint32_t GetFuzzTimeMS();
  nsresult ScheduleActiveTimerCallback();
  uint32_t FindInsertionIndex(IdleObserverHolder* aIdleObserver);
  virtual nsresult RegisterIdleObserver(nsIIdleObserver* aIdleObserverPtr);
  nsresult FindIndexOfElementToRemove(nsIIdleObserver* aIdleObserver,
                                      int32_t* aRemoveElementIndex);
  virtual nsresult UnregisterIdleObserver(nsIIdleObserver* aIdleObserverPtr);

  nsresult FireHashchange(const nsAString &aOldURL, const nsAString &aNewURL);

  void FlushPendingNotifications(mozFlushType aType);
  void EnsureReflowFlushAndPaint();
  void CheckSecurityWidthAndHeight(int32_t* width, int32_t* height);
  void CheckSecurityLeftAndTop(int32_t* left, int32_t* top);

  
  void SetCSSViewportWidthAndHeight(nscoord width, nscoord height);
  
  nsresult SetDocShellWidthAndHeight(int32_t width, int32_t height);

  static bool CanSetProperty(const char *aPrefName);

  static void MakeScriptDialogTitle(nsAString &aOutTitle);

  bool CanMoveResizeWindows();

  bool     GetBlurSuppression();

  
  
  nsresult GetScrollXY(int32_t* aScrollX, int32_t* aScrollY,
                       bool aDoFlush);
  nsresult GetScrollMaxXY(int32_t* aScrollMaxX, int32_t* aScrollMaxY);

  
  nsresult GetInnerSize(mozilla::CSSIntSize& aSize);

  nsresult GetOuterSize(nsIntSize* aSizeCSSPixels);
  nsresult SetOuterSize(int32_t aLengthCSSPixels, bool aIsWidth);
  nsRect GetInnerScreenRect();

  void ScrollTo(const mozilla::CSSIntPoint& aScroll);

  bool IsFrame()
  {
    return GetParentInternal() != nullptr;
  }

  
  
  
  bool WindowExists(const nsAString& aName, bool aLookForCallerOnJSStack);

  already_AddRefed<nsIWidget> GetMainWidget();
  nsIWidget* GetNearestWidget();

  void Freeze()
  {
    NS_ASSERTION(!IsFrozen(), "Double-freezing?");
    mIsFrozen = true;
    NotifyDOMWindowFrozen(this);
  }

  void Thaw()
  {
    mIsFrozen = false;
    NotifyDOMWindowThawed(this);
  }

  bool IsInModalState();

  
  
  
  int32_t DevToCSSIntPixels(int32_t px);
  int32_t CSSToDevIntPixels(int32_t px);
  nsIntSize DevToCSSIntPixels(nsIntSize px);
  nsIntSize CSSToDevIntPixels(nsIntSize px);

  virtual void SetFocusedNode(nsIContent* aNode,
                              uint32_t aFocusMethod = 0,
                              bool aNeedsFocus = false);

  virtual uint32_t GetFocusMethod();

  virtual bool ShouldShowFocusRing();

  virtual void SetKeyboardIndicators(UIStateChangeType aShowAccelerators,
                                     UIStateChangeType aShowFocusRings);
  virtual void GetKeyboardIndicators(bool* aShowAccelerators,
                                     bool* aShowFocusRings);

  void UpdateCanvasFocus(bool aFocusChanged, nsIContent* aNewContent);

public:
  virtual already_AddRefed<nsPIWindowRoot> GetTopWindowRoot() MOZ_OVERRIDE;

protected:
  static void NotifyDOMWindowDestroyed(nsGlobalWindow* aWindow);
  void NotifyWindowIDDestroyed(const char* aTopic);

  static void NotifyDOMWindowFrozen(nsGlobalWindow* aWindow);
  static void NotifyDOMWindowThawed(nsGlobalWindow* aWindow);

  void ClearStatus();

  virtual void UpdateParentTarget();

  bool GetIsTabModalPromptAllowed();

  inline int32_t DOMMinTimeoutValue() const;

  nsresult CreateOuterObject(nsGlobalWindow* aNewInner);
  nsresult SetOuterObject(JSContext* aCx, JS::Handle<JSObject*> aOuterObject);
  nsresult CloneStorageEvent(const nsAString& aType,
                             nsCOMPtr<nsIDOMStorageEvent>& aEvent);

  
  nsresult GetTopImpl(nsIDOMWindow **aWindow, bool aScriptable);

  
  nsDOMWindowList* GetWindowList();

  
  nsresult GetComputedStyleHelper(nsIDOMElement* aElt,
                                  const nsAString& aPseudoElt,
                                  bool aDefaultStylesOnly,
                                  nsIDOMCSSStyleDeclaration** aReturn);

  void PreloadLocalStorage();

  nsresult RequestAnimationFrame(const nsIDocument::FrameRequestCallbackHolder& aCallback,
                                 int32_t* aHandle);

  
  
  
  
  
  

  
  
  
  
  
  
  bool                          mIsFrozen : 1;

  
  
  bool                          mFullScreen : 1;
  bool                          mIsClosed : 1;
  bool                          mInClose : 1;
  
  
  
  bool                          mHavePendingClose : 1;
  bool                          mHadOriginalOpener : 1;
  bool                          mIsPopupSpam : 1;

  
  bool                          mBlockScriptedClosingFlag : 1;

  
  bool                          mFireOfflineStatusChangeEventOnThaw : 1;
  bool                          mNotifyIdleObserversIdleOnThaw : 1;
  bool                          mNotifyIdleObserversActiveOnThaw : 1;

  
  
  bool                          mCreatingInnerWindow : 1;

  
  bool                          mIsChrome : 1;

  
  
  
  bool                          mCleanMessageManager : 1;

  
  
  bool                   mNeedsFocus : 1;
  bool                   mHasFocus : 1;

  
  bool                   mShowAccelerators : 1;

  
  bool                   mShowFocusRings : 1;

  
  
  bool                   mShowFocusRingForContent : 1;

  
  
  bool                   mFocusByKeyOccurred : 1;

  
  bool                   mHasGamepad : 1;
#ifdef MOZ_GAMEPAD
  nsRefPtrHashtable<nsUint32HashKey, mozilla::dom::Gamepad> mGamepads;
  bool mHasSeenGamepadInput;
#endif

  
  bool                   mNotifiedIDDestroyed : 1;
  
  
  bool                   mAllowScriptsToClose : 1;

  nsCOMPtr<nsIScriptContext>    mContext;
  nsWeakPtr                     mOpener;
  nsCOMPtr<nsIControllers>      mControllers;

  
  nsCOMPtr<nsIArray>            mArguments;

  
  nsRefPtr<DialogValueHolder> mDialogArguments;

  nsRefPtr<Navigator>           mNavigator;
  nsRefPtr<nsScreen>            mScreen;
  nsRefPtr<nsDOMWindowList>     mFrames;
  nsRefPtr<mozilla::dom::BarProp> mMenubar;
  nsRefPtr<mozilla::dom::BarProp> mToolbar;
  nsRefPtr<mozilla::dom::BarProp> mLocationbar;
  nsRefPtr<mozilla::dom::BarProp> mPersonalbar;
  nsRefPtr<mozilla::dom::BarProp> mStatusbar;
  nsRefPtr<mozilla::dom::BarProp> mScrollbars;
  nsRefPtr<nsDOMWindowUtils>    mWindowUtils;
  nsString                      mStatus;
  nsString                      mDefaultStatus;
  nsGlobalWindowObserver*       mObserver;
  nsCOMPtr<nsIDOMCrypto>        mCrypto;

  nsCOMPtr<nsIDOMStorage>      mLocalStorage;
  nsCOMPtr<nsIDOMStorage>      mSessionStorage;

  nsCOMPtr<nsIXPConnectJSObjectHolder> mInnerWindowHolder;

  
  nsRefPtr<nsEventListenerManager> mListenerManager;
  
  
  
  
  mozilla::LinkedList<nsTimeout> mTimeouts;
  
  
  
  nsTimeout*                    mTimeoutInsertionPoint;
  uint32_t                      mTimeoutPublicIdCounter;
  uint32_t                      mTimeoutFiringDepth;
  nsRefPtr<nsLocation>          mLocation;
  nsRefPtr<nsHistory>           mHistory;

  
  nsCOMPtr<nsIPrincipal> mDocumentPrincipal;

  
  JS::TenuredHeap<JSObject*> mJSObject;

  typedef nsCOMArray<nsIDOMStorageEvent> nsDOMStorageEventArray;
  nsDOMStorageEventArray mPendingStorageEvents;

  uint32_t mTimeoutsSuspendDepth;

  
  uint32_t mFocusMethod;

  uint32_t mSerial;

#ifdef DEBUG
  bool mSetOpenerWindowCalled;
  nsCOMPtr<nsIURI> mLastOpenedURI;
#endif

#ifdef MOZ_B2G
  bool mNetworkUploadObserverEnabled;
  bool mNetworkDownloadObserverEnabled;
#endif 

  bool mCleanedUp;

  nsCOMPtr<nsIDOMOfflineResourceList> mApplicationCache;

  nsAutoPtr<nsJSThingHashtable<nsPtrHashKey<nsXBLPrototypeHandler>, JSObject*> > mCachedXBLPrototypeHandlers;

  nsCOMPtr<nsIDocument> mSuspendedDoc;

  nsRefPtr<mozilla::dom::indexedDB::IDBFactory> mIndexedDB;

  
  
  
  
  uint32_t                      mDialogAbuseCount;

  
  
  
  
  TimeStamp                     mLastDialogQuitTime;

  
  
  bool                          mAreDialogsEnabled;

  nsTHashtable<nsPtrHashKey<nsDOMEventTargetHelper> > mEventTargetObjects;

  nsTArray<uint32_t> mEnabledSensors;

#ifdef MOZ_WEBSPEECH
  
  nsRefPtr<mozilla::dom::SpeechSynthesis> mSpeechSynthesis;
#endif

  friend class nsDOMScriptableHelper;
  friend class nsDOMWindowUtils;
  friend class PostMessageEvent;
  friend class DesktopNotification;

  static WindowByIdTable* sWindowsById;
  static bool sWarnedAboutWindowInternal;
};





class nsGlobalChromeWindow : public nsGlobalWindow,
                             public nsIDOMChromeWindow
{
public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMCHROMEWINDOW

  nsGlobalChromeWindow(nsGlobalWindow *aOuterWindow)
    : nsGlobalWindow(aOuterWindow)
  {
    mIsChrome = true;
    mCleanMessageManager = true;
  }

  ~nsGlobalChromeWindow()
  {
    NS_ABORT_IF_FALSE(mCleanMessageManager,
                      "chrome windows may always disconnect the msg manager");
    if (mMessageManager) {
      static_cast<nsFrameMessageManager *>(
        mMessageManager.get())->Disconnect();
    }

    mCleanMessageManager = false;
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsGlobalChromeWindow,
                                           nsGlobalWindow)

  nsCOMPtr<nsIBrowserDOMWindow> mBrowserDOMWindow;
  nsCOMPtr<nsIMessageBroadcaster> mMessageManager;
};






class nsGlobalModalWindow : public nsGlobalWindow,
                            public nsIDOMModalContentWindow
{
public:
  nsGlobalModalWindow(nsGlobalWindow *aOuterWindow)
    : nsGlobalWindow(aOuterWindow)
  {
    mIsModalContentWindow = true;
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMODALCONTENTWINDOW

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsGlobalModalWindow, nsGlobalWindow)

protected:
  
  nsRefPtr<DialogValueHolder> mReturnValue;
};


inline already_AddRefed<nsGlobalWindow>
NS_NewScriptGlobalObject(bool aIsChrome, bool aIsModalContentWindow)
{
  nsRefPtr<nsGlobalWindow> global;

  if (aIsChrome) {
    global = new nsGlobalChromeWindow(nullptr);
  } else if (aIsModalContentWindow) {
    global = new nsGlobalModalWindow(nullptr);
  } else {
    global = new nsGlobalWindow(nullptr);
  }

  return global.forget();
}

#endif 
