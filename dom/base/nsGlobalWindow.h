





#ifndef nsGlobalWindow_h___
#define nsGlobalWindow_h___

#include "nsPIDOMWindow.h"

#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"
#include "nsInterfaceHashtable.h"



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
#include "mozilla/EventListenerManager.h"
#include "nsIPrincipal.h"
#include "nsSize.h"
#include "mozFlushType.h"
#include "prclist.h"
#include "mozilla/dom/StorageEvent.h"
#include "mozilla/dom/StorageEventBinding.h"
#include "nsFrameMessageManager.h"
#include "mozilla/LinkedList.h"
#include "mozilla/TimeStamp.h"
#include "nsWrapperCacheInlines.h"
#include "nsIIdleObserver.h"
#include "nsIDocument.h"
#include "mozilla/dom/EventTarget.h"
#include "mozilla/dom/WindowBinding.h"
#include "Units.h"
#include "nsComponentManagerUtils.h"

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
class nsICSSDeclaration;
class nsIDocShellTreeOwner;
class nsIDOMCrypto;
class nsIDOMOfflineResourceList;
class nsIScrollableFrame;
class nsIControllers;
class nsIJSID;
class nsIScriptContext;
class nsIScriptTimeoutHandler;
class nsIWebBrowserChrome;

class nsDOMWindowList;
class nsLocation;
class nsScreen;
class nsHistory;
class nsGlobalWindowObserver;
class nsGlobalWindow;
class nsDOMWindowUtils;
class nsIIdleService;
struct nsIntSize;
struct nsRect;

class nsWindowSizes;

namespace mozilla {
class DOMEventTargetHelper;
namespace dom {
class BarProp;
class Console;
class External;
class Function;
class Gamepad;
class MediaQueryList;
class MozSelfSupport;
class Navigator;
class OwningExternalOrWindowProxy;
class Selection;
class SpeechSynthesis;
class WakeLock;
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

extern already_AddRefed<nsIScriptTimeoutHandler>
NS_CreateJSTimeoutHandler(nsGlobalWindow *aWindow,
                          mozilla::dom::Function& aFunction,
                          const mozilla::dom::Sequence<JS::Value>& aArguments,
                          mozilla::ErrorResult& aError);

extern already_AddRefed<nsIScriptTimeoutHandler>
NS_CreateJSTimeoutHandler(JSContext* aCx, nsGlobalWindow *aWindow,
                          const nsAString& aExpression,
                          mozilla::ErrorResult& aError);






struct nsTimeout MOZ_FINAL
  : mozilla::LinkedListElement<nsTimeout>
{
private:
  ~nsTimeout();

public:
  nsTimeout();

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
    if (aSubject->SubsumesConsideringDomain(mOrigin)) {
      result = mValue;
    } else {
      result = CreateVoidVariant();
    }
    result.forget(aResult);
    return NS_OK;
  }
  void Get(JSContext* aCx, JS::Handle<JSObject*> aScope, nsIPrincipal* aSubject,
           JS::MutableHandle<JS::Value> aResult, mozilla::ErrorResult& aError)
  {
    if (aSubject->Subsumes(mOrigin)) {
      aError = nsContentUtils::XPConnect()->VariantToJS(aCx, aScope,
                                                        mValue, aResult);
    } else {
      aResult.setUndefined();
    }
  }
private:
  virtual ~DialogValueHolder() {}

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
                       public PRCListStr
#ifdef MOZ_WEBSPEECH
                     , public nsISpeechSynthesisGetter
#endif 
{
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;
  typedef nsDataHashtable<nsUint64HashKey, nsGlobalWindow*> WindowByIdTable;

  
  nsPIDOMWindow* GetPrivateParent();

  
  void ReallyCloseWindow();

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  virtual JSObject *WrapObject(JSContext *cx) MOZ_OVERRIDE
  {
    return IsInnerWindow() || EnsureInnerWindow() ? GetWrapper() : nullptr;
  }

  
  virtual JSObject *GetGlobalJSObject();

  
  JSObject *FastGetGlobalJSObject() const
  {
    return GetWrapperPreserveColor();
  }

  void TraceGlobalJSObject(JSTracer* aTrc);

  virtual nsresult EnsureScriptEnvironment();

  virtual nsIScriptContext *GetScriptContext();

  void PoisonOuterWindowProxy(JSObject *aObject);

  virtual bool IsBlackForCC(bool aTracingNeeded = true);

  static JSObject* OuterObject(JSContext* aCx, JS::Handle<JSObject*> aObj);

  
  virtual nsIPrincipal* GetPrincipal();

  
  NS_DECL_NSIDOMWINDOW

#ifdef MOZ_WEBSPEECH
  
  NS_DECL_NSISPEECHSYNTHESISGETTER
#endif 

  
  NS_DECL_NSIDOMJSWINDOW

  
  NS_DECL_NSIDOMEVENTTARGET

  virtual mozilla::EventListenerManager*
    GetExistingListenerManager() const MOZ_OVERRIDE;

  virtual mozilla::EventListenerManager*
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

  
  virtual nsPIDOMWindow* GetPrivateRoot();

  
  virtual void ActivateOrDeactivate(bool aActivate);
  virtual void SetActive(bool aActive);
  virtual void SetIsBackground(bool aIsBackground);
  virtual void SetChromeEventHandler(mozilla::dom::EventTarget* aChromeEventHandler);

  
  virtual void SetInitialPrincipalToSubject();

  virtual PopupControlState PushPopupControlState(PopupControlState state, bool aForce) const;
  virtual void PopPopupControlState(PopupControlState state) const;
  virtual PopupControlState GetPopupControlState() const;

  virtual already_AddRefed<nsISupports> SaveWindowState();
  virtual nsresult RestoreWindowState(nsISupports *aState);
  virtual void SuspendTimeouts(uint32_t aIncrease = 1,
                                           bool aFreezeChildren = true);
  virtual nsresult ResumeTimeouts(bool aThawChildren = true);
  virtual uint32_t TimeoutSuspendCount();
  virtual nsresult FireDelayedDOMEvents();
  virtual bool IsFrozen() const
  {
    return mIsFrozen;
  }
  virtual bool IsRunningTimeout() { return mTimeoutFiringDepth > 0; }

  
  virtual bool WouldReuseInnerWindow(nsIDocument* aNewDocument);

  virtual void SetDocShell(nsIDocShell* aDocShell);
  virtual void DetachFromDocShell();
  virtual nsresult SetNewDocument(nsIDocument *aDocument,
                                              nsISupports *aState,
                                              bool aForceReuseInnerWindow);

  
  void DispatchDOMWindowCreated();

  virtual void SetOpenerWindow(nsIDOMWindow* aOpener,
                                           bool aOriginalOpener);

  
  virtual void EnsureSizeUpToDate();

  virtual void EnterModalState();
  virtual void LeaveModalState();

  
  virtual bool CanClose();
  virtual void ForceClose();

  virtual void MaybeUpdateTouchState();
  virtual void UpdateTouchState();

  
  virtual bool DispatchCustomEvent(const nsAString& aEventName);
  bool DispatchResizeEvent(const mozilla::CSSIntSize& aSize);

  
  virtual void RefreshCompartmentPrincipal();

  
  virtual nsresult SetFullScreenInternal(bool aIsFullScreen, bool aRequireTrust);
  bool FullScreen() const;

  
  virtual void SetHasGamepadEventListener(bool aHasGamepad = true);

  
  NS_DECL_NSIINTERFACEREQUESTOR

  
  already_AddRefed<nsIDOMWindow> IndexedGetter(uint32_t aIndex, bool& aFound);

  void GetSupportedNames(nsTArray<nsString>& aNames);

  static bool IsChromeWindow(JSContext* , JSObject* aObj);

  static bool IsShowModalDialogEnabled(JSContext*  = nullptr,
                                       JSObject*  = nullptr);

  bool DoNewResolve(JSContext* aCx, JS::Handle<JSObject*> aObj,
                    JS::Handle<jsid> aId,
                    JS::MutableHandle<JSPropertyDescriptor> aDesc);

  void GetOwnPropertyNames(JSContext* aCx, nsTArray<nsString>& aNames,
                           mozilla::ErrorResult& aRv);

  
  explicit nsGlobalWindow(nsGlobalWindow *aOuterWindow);

  static nsGlobalWindow *FromSupports(nsISupports *supports)
  {
    
    return (nsGlobalWindow *)(mozilla::dom::EventTarget *)supports;
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
    return static_cast<nsGlobalWindow *>(top.get());
  }

  nsPIDOMWindow* GetChildWindow(const nsAString& aName);

  
  
  
  
  
  
  
  
  
  bool ShouldPromptToBlockDialogs();
  
  bool DialogsAreBeingAbused();

  
  
  void EnableDialogs();
  void DisableDialogs();
  
  bool AreDialogsEnabled();

  nsIScriptContext *GetContextInternal()
  {
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
    MOZ_ASSERT(IsOuterWindow());
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

  using nsPIDOMWindow::IsModalContentWindow;
  static bool IsModalContentWindow(JSContext* aCx, JSObject* aGlobal);

  
  
  nsIScrollableFrame *GetScrollFrame();

  nsresult Observe(nsISupports* aSubject, const char* aTopic,
                   const char16_t* aData);

  
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

#ifdef DEBUG
  
  
  void RiskyUnlink();
#endif

  virtual JSObject*
    GetCachedXBLPrototypeHandler(nsXBLPrototypeHandler* aKey);

  virtual void
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

  virtual void
  FirePopupBlockedEvent(nsIDocument* aDoc,
                        nsIURI* aPopupURI,
                        const nsAString& aPopupWindowName,
                        const nsAString& aPopupWindowFeatures) MOZ_OVERRIDE;

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

  
  void AddEventTargetObject(mozilla::DOMEventTargetHelper* aObject);
  void RemoveEventTargetObject(mozilla::DOMEventTargetHelper* aObject);

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
    mozilla::EventListenerManager* elm = GetExistingListenerManager();        \
    return elm ? elm->GetEventHandler(nsGkAtoms::on##name_, EmptyString())    \
               : nullptr;                                                     \
  }                                                                           \
  void SetOn##name_(mozilla::dom::EventHandlerNonNull* handler)               \
  {                                                                           \
    mozilla::EventListenerManager* elm = GetOrCreateListenerManager();        \
    if (elm) {                                                                \
      elm->SetEventHandler(nsGkAtoms::on##name_, EmptyString(), handler);     \
    }                                                                         \
  }
#define ERROR_EVENT(name_, id_, type_, struct_)                               \
  mozilla::dom::OnErrorEventHandlerNonNull* GetOn##name_()                    \
  {                                                                           \
    mozilla::EventListenerManager* elm = GetExistingListenerManager();        \
    return elm ? elm->GetOnErrorEventHandler() : nullptr;                     \
  }                                                                           \
  void SetOn##name_(mozilla::dom::OnErrorEventHandlerNonNull* handler)        \
  {                                                                           \
    mozilla::EventListenerManager* elm = GetOrCreateListenerManager();        \
    if (elm) {                                                                \
      elm->SetEventHandler(handler);                                          \
    }                                                                         \
  }
#define BEFOREUNLOAD_EVENT(name_, id_, type_, struct_)                        \
  mozilla::dom::OnBeforeUnloadEventHandlerNonNull* GetOn##name_()             \
  {                                                                           \
    mozilla::EventListenerManager* elm = GetExistingListenerManager();        \
    return elm ? elm->GetOnBeforeUnloadEventHandler() : nullptr;              \
  }                                                                           \
  void SetOn##name_(mozilla::dom::OnBeforeUnloadEventHandlerNonNull* handler) \
  {                                                                           \
    mozilla::EventListenerManager* elm = GetOrCreateListenerManager();        \
    if (elm) {                                                                \
      elm->SetEventHandler(handler);                                          \
    }                                                                         \
  }
#define WINDOW_ONLY_EVENT EVENT
#define TOUCH_EVENT EVENT
#include "mozilla/EventNameList.h"
#undef TOUCH_EVENT
#undef WINDOW_ONLY_EVENT
#undef BEFOREUNLOAD_EVENT
#undef ERROR_EVENT
#undef EVENT

  nsISupports* GetParentObject()
  {
    return nullptr;
  }

  nsIDOMWindow* GetWindow(mozilla::ErrorResult& aError);
  nsIDOMWindow* GetSelf(mozilla::ErrorResult& aError);
  nsIDocument* GetDocument()
  {
    return GetDoc();
  }
  void GetName(nsAString& aName, mozilla::ErrorResult& aError);
  void SetName(const nsAString& aName, mozilla::ErrorResult& aError);
  nsLocation* GetLocation(mozilla::ErrorResult& aError);
  nsHistory* GetHistory(mozilla::ErrorResult& aError);
  mozilla::dom::BarProp* GetLocationbar(mozilla::ErrorResult& aError);
  mozilla::dom::BarProp* GetMenubar(mozilla::ErrorResult& aError);
  mozilla::dom::BarProp* GetPersonalbar(mozilla::ErrorResult& aError);
  mozilla::dom::BarProp* GetScrollbars(mozilla::ErrorResult& aError);
  mozilla::dom::BarProp* GetStatusbar(mozilla::ErrorResult& aError);
  mozilla::dom::BarProp* GetToolbar(mozilla::ErrorResult& aError);
  void GetStatus(nsAString& aStatus, mozilla::ErrorResult& aError);
  void SetStatus(const nsAString& aStatus, mozilla::ErrorResult& aError);
  void Close(mozilla::ErrorResult& aError);
  bool GetClosed(mozilla::ErrorResult& aError);
  void Stop(mozilla::ErrorResult& aError);
  void Focus(mozilla::ErrorResult& aError);
  void Blur(mozilla::ErrorResult& aError);
  already_AddRefed<nsIDOMWindow> GetFrames(mozilla::ErrorResult& aError);
  uint32_t Length();
  already_AddRefed<nsIDOMWindow> GetTop(mozilla::ErrorResult& aError)
  {
    nsCOMPtr<nsIDOMWindow> top;
    aError = GetScriptableTop(getter_AddRefs(top));
    return top.forget();
  }
protected:
  nsIDOMWindow* GetOpenerWindow(mozilla::ErrorResult& aError);
public:
  void GetOpener(JSContext* aCx, JS::MutableHandle<JS::Value> aRetval,
                 mozilla::ErrorResult& aError);
  void SetOpener(JSContext* aCx, JS::Handle<JS::Value> aOpener,
                 mozilla::ErrorResult& aError);
  using nsIDOMWindow::GetParent;
  already_AddRefed<nsIDOMWindow> GetParent(mozilla::ErrorResult& aError);
  mozilla::dom::Element* GetFrameElement(mozilla::ErrorResult& aError);
  already_AddRefed<nsIDOMWindow> Open(const nsAString& aUrl,
                                      const nsAString& aName,
                                      const nsAString& aOptions,
                                      mozilla::ErrorResult& aError);
  mozilla::dom::Navigator* GetNavigator(mozilla::ErrorResult& aError);
  nsIDOMOfflineResourceList* GetApplicationCache(mozilla::ErrorResult& aError);

  mozilla::dom::Console* GetConsole(mozilla::ErrorResult& aRv);

  void GetSidebar(mozilla::dom::OwningExternalOrWindowProxy& aResult,
                  mozilla::ErrorResult& aRv);
  already_AddRefed<mozilla::dom::External> GetExternal(mozilla::ErrorResult& aRv);

protected:
  bool AlertOrConfirm(bool aAlert, const nsAString& aMessage,
                      mozilla::ErrorResult& aError);

public:
  void Alert(mozilla::ErrorResult& aError);
  void Alert(const nsAString& aMessage, mozilla::ErrorResult& aError);
  bool Confirm(const nsAString& aMessage, mozilla::ErrorResult& aError);
  void Prompt(const nsAString& aMessage, const nsAString& aInitial,
              nsAString& aReturn, mozilla::ErrorResult& aError);
  void Print(mozilla::ErrorResult& aError);
  void ShowModalDialog(JSContext* aCx, const nsAString& aUrl,
                       JS::Handle<JS::Value> aArgument,
                       const nsAString& aOptions,
                       JS::MutableHandle<JS::Value> aRetval,
                       mozilla::ErrorResult& aError);
  void PostMessageMoz(JSContext* aCx, JS::Handle<JS::Value> aMessage,
                      const nsAString& aTargetOrigin,
                      const mozilla::dom::Optional<mozilla::dom::Sequence<JS::Value > >& aTransfer,
                      mozilla::ErrorResult& aError);
  int32_t SetTimeout(JSContext* aCx, mozilla::dom::Function& aFunction,
                     int32_t aTimeout,
                     const mozilla::dom::Sequence<JS::Value>& aArguments,
                     mozilla::ErrorResult& aError);
  int32_t SetTimeout(JSContext* aCx, const nsAString& aHandler,
                     int32_t aTimeout,
                     const mozilla::dom::Sequence<JS::Value>& ,
                     mozilla::ErrorResult& aError);
  void ClearTimeout(int32_t aHandle, mozilla::ErrorResult& aError);
  int32_t SetInterval(JSContext* aCx, mozilla::dom::Function& aFunction,
                      const mozilla::dom::Optional<int32_t>& aTimeout,
                      const mozilla::dom::Sequence<JS::Value>& aArguments,
                      mozilla::ErrorResult& aError);
  int32_t SetInterval(JSContext* aCx, const nsAString& aHandler,
                      const mozilla::dom::Optional<int32_t>& aTimeout,
                      const mozilla::dom::Sequence<JS::Value>& ,
                      mozilla::ErrorResult& aError);
  void ClearInterval(int32_t aHandle, mozilla::ErrorResult& aError);
  void Atob(const nsAString& aAsciiBase64String, nsAString& aBinaryData,
            mozilla::ErrorResult& aError);
  void Btoa(const nsAString& aBinaryData, nsAString& aAsciiBase64String,
            mozilla::ErrorResult& aError);
  mozilla::dom::DOMStorage* GetSessionStorage(mozilla::ErrorResult& aError);
  mozilla::dom::DOMStorage* GetLocalStorage(mozilla::ErrorResult& aError);
  mozilla::dom::Selection* GetSelection(mozilla::ErrorResult& aError);
  mozilla::dom::indexedDB::IDBFactory* GetIndexedDB(mozilla::ErrorResult& aError);
  already_AddRefed<nsICSSDeclaration>
    GetComputedStyle(mozilla::dom::Element& aElt, const nsAString& aPseudoElt,
                     mozilla::ErrorResult& aError);
  already_AddRefed<mozilla::dom::MediaQueryList> MatchMedia(const nsAString& aQuery,
                                                            mozilla::ErrorResult& aError);
  nsScreen* GetScreen(mozilla::ErrorResult& aError);
  void MoveTo(int32_t aXPos, int32_t aYPos, mozilla::ErrorResult& aError);
  void MoveBy(int32_t aXDif, int32_t aYDif, mozilla::ErrorResult& aError);
  void ResizeTo(int32_t aWidth, int32_t aHeight,
                mozilla::ErrorResult& aError);
  void ResizeBy(int32_t aWidthDif, int32_t aHeightDif,
                mozilla::ErrorResult& aError);
  void Scroll(int32_t aXScroll, int32_t aYScroll,
              const mozilla::dom::ScrollOptions& aOptions);
  void ScrollTo(int32_t aXScroll, int32_t aYScroll,
                const mozilla::dom::ScrollOptions& aOptions);
  void ScrollBy(int32_t aXScrollDif, int32_t aYScrollDif,
                const mozilla::dom::ScrollOptions& aOptions);
  void ScrollByLines(int32_t numLines,
                     const mozilla::dom::ScrollOptions& aOptions);
  void ScrollByPages(int32_t numPages,
                     const mozilla::dom::ScrollOptions& aOptions);
  int32_t GetInnerWidth(mozilla::ErrorResult& aError);
  void SetInnerWidth(int32_t aInnerWidth, mozilla::ErrorResult& aError);
  int32_t GetInnerHeight(mozilla::ErrorResult& aError);
  void SetInnerHeight(int32_t aInnerHeight, mozilla::ErrorResult& aError);
  int32_t GetScrollX(mozilla::ErrorResult& aError);
  int32_t GetPageXOffset(mozilla::ErrorResult& aError)
  {
    return GetScrollX(aError);
  }
  int32_t GetScrollY(mozilla::ErrorResult& aError);
  int32_t GetPageYOffset(mozilla::ErrorResult& aError)
  {
    return GetScrollY(aError);
  }
  void MozRequestOverfill(mozilla::dom::OverfillCallback& aCallback, mozilla::ErrorResult& aError);
  int32_t GetScreenX(mozilla::ErrorResult& aError);
  void SetScreenX(int32_t aScreenX, mozilla::ErrorResult& aError);
  int32_t GetScreenY(mozilla::ErrorResult& aError);
  void SetScreenY(int32_t aScreenY, mozilla::ErrorResult& aError);
  int32_t GetOuterWidth(mozilla::ErrorResult& aError);
  void SetOuterWidth(int32_t aOuterWidth, mozilla::ErrorResult& aError);
  int32_t GetOuterHeight(mozilla::ErrorResult& aError);
  void SetOuterHeight(int32_t aOuterHeight, mozilla::ErrorResult& aError);
  int32_t RequestAnimationFrame(mozilla::dom::FrameRequestCallback& aCallback,
                                mozilla::ErrorResult& aError);
  void CancelAnimationFrame(int32_t aHandle, mozilla::ErrorResult& aError);
  nsPerformance* GetPerformance();
#ifdef MOZ_WEBSPEECH
  mozilla::dom::SpeechSynthesis*
    GetSpeechSynthesis(mozilla::ErrorResult& aError);
#endif
  already_AddRefed<nsICSSDeclaration>
    GetDefaultComputedStyle(mozilla::dom::Element& aElt,
                            const nsAString& aPseudoElt,
                            mozilla::ErrorResult& aError);
  mozilla::dom::indexedDB::IDBFactory*
    GetMozIndexedDB(mozilla::ErrorResult& aError)
  {
    return GetIndexedDB(aError);
  }
  int32_t MozRequestAnimationFrame(nsIFrameRequestCallback* aRequestCallback,
                                   mozilla::ErrorResult& aError);
  void MozCancelAnimationFrame(int32_t aHandle, mozilla::ErrorResult& aError)
  {
    return CancelAnimationFrame(aHandle, aError);
  }
  void MozCancelRequestAnimationFrame(int32_t aHandle,
                                      mozilla::ErrorResult& aError)
  {
    return CancelAnimationFrame(aHandle, aError);
  }
  int64_t GetMozAnimationStartTime(mozilla::ErrorResult& aError);
  void SizeToContent(mozilla::ErrorResult& aError);
  nsIDOMCrypto* GetCrypto(mozilla::ErrorResult& aError);
  nsIControllers* GetControllers(mozilla::ErrorResult& aError);
  mozilla::dom::Element* GetRealFrameElement(mozilla::ErrorResult& aError);
  float GetMozInnerScreenX(mozilla::ErrorResult& aError);
  float GetMozInnerScreenY(mozilla::ErrorResult& aError);
  float GetDevicePixelRatio(mozilla::ErrorResult& aError);
  int32_t GetScrollMaxX(mozilla::ErrorResult& aError);
  int32_t GetScrollMaxY(mozilla::ErrorResult& aError);
  bool GetFullScreen(mozilla::ErrorResult& aError);
  void SetFullScreen(bool aFullScreen, mozilla::ErrorResult& aError);
  void Back(mozilla::ErrorResult& aError);
  void Forward(mozilla::ErrorResult& aError);
  void Home(mozilla::ErrorResult& aError);
  bool Find(const nsAString& aString, bool aCaseSensitive, bool aBackwards,
            bool aWrapAround, bool aWholeWord, bool aSearchInFrames,
            bool aShowDialog, mozilla::ErrorResult& aError);
  uint64_t GetMozPaintCount(mozilla::ErrorResult& aError);

  mozilla::dom::MozSelfSupport* GetMozSelfSupport(mozilla::ErrorResult& aError);

  already_AddRefed<nsIDOMWindow> OpenDialog(JSContext* aCx,
                                            const nsAString& aUrl,
                                            const nsAString& aName,
                                            const nsAString& aOptions,
                                            const mozilla::dom::Sequence<JS::Value>& aExtraArgument,
                                            mozilla::ErrorResult& aError);
  void GetContent(JSContext* aCx,
                  JS::MutableHandle<JSObject*> aRetval,
                  mozilla::ErrorResult& aError);
  void Get_content(JSContext* aCx,
                   JS::MutableHandle<JSObject*> aRetval,
                   mozilla::ErrorResult& aError)
  {
    if (mDoc) {
      mDoc->WarnOnceAbout(nsIDocument::eWindow_Content);
    }
    GetContent(aCx, aRetval, aError);
  }

  
  
  uint16_t WindowState();
  nsIBrowserDOMWindow* GetBrowserDOMWindow(mozilla::ErrorResult& aError);
  void SetBrowserDOMWindow(nsIBrowserDOMWindow* aBrowserWindow,
                           mozilla::ErrorResult& aError);
  void GetAttention(mozilla::ErrorResult& aError);
  void GetAttentionWithCycleCount(int32_t aCycleCount,
                                  mozilla::ErrorResult& aError);
  void SetCursor(const nsAString& aCursor, mozilla::ErrorResult& aError);
  void Maximize(mozilla::ErrorResult& aError);
  void Minimize(mozilla::ErrorResult& aError);
  void Restore(mozilla::ErrorResult& aError);
  void NotifyDefaultButtonLoaded(mozilla::dom::Element& aDefaultButton,
                                 mozilla::ErrorResult& aError);
  nsIMessageBroadcaster* GetMessageManager(mozilla::ErrorResult& aError);
  nsIMessageBroadcaster* GetGroupMessageManager(const nsAString& aGroup,
                                                mozilla::ErrorResult& aError);
  void BeginWindowMove(mozilla::dom::Event& aMouseDownEvent,
                       mozilla::dom::Element* aPanel,
                       mozilla::ErrorResult& aError);

  void GetDialogArguments(JSContext* aCx, JS::MutableHandle<JS::Value> aRetval,
                          mozilla::ErrorResult& aError);
  void GetReturnValue(JSContext* aCx, JS::MutableHandle<JS::Value> aReturnValue,
                      mozilla::ErrorResult& aError);
  void SetReturnValue(JSContext* aCx, JS::Handle<JS::Value> aReturnValue,
                      mozilla::ErrorResult& aError);

  void GetInterface(JSContext* aCx, nsIJSID* aIID,
                    JS::MutableHandle<JS::Value> aRetval,
                    mozilla::ErrorResult& aError);

protected:
  
  nsTObserverArray<IdleObserverHolder> mIdleObservers;

  
  nsCOMPtr<nsITimer> mIdleTimer;

  
  uint32_t mIdleFuzzFactor;

  
  
  int32_t mIdleCallbackIndex;

  
  
  bool mCurrentlyIdle;

  
  
  bool mAddActiveEventFuzzTime;

  nsCOMPtr <nsIIdleService> mIdleService;

  nsRefPtr<mozilla::dom::WakeLock> mWakeLock;

  static bool sIdleObserversAPIFuzzTimeDisabled;

  friend class HashchangeCallback;
  friend class mozilla::dom::BarProp;

  
  virtual ~nsGlobalWindow();
  void DropOuterWindowDocs();
  void CleanUp();
  void ClearControllers();
  
  void FinalClose();

  inline void MaybeClearInnerWindow(nsGlobalWindow* aExpectedInner)
  {
    if(mInnerWindow == aExpectedInner) {
      mInnerWindow = nullptr;
    }
  }

  void FreeInnerObjects();
  JSObject *CallerGlobal();
  nsGlobalWindow *CallerInnerWindow();

  
  
  void InnerSetNewDocument(JSContext* aCx, nsIDocument* aDocument);

  
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

private:
  













































  nsresult OpenInternal(const nsAString& aUrl,
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

public:
  
  
  
  nsresult SetTimeoutOrInterval(nsIScriptTimeoutHandler *aHandler,
                                int32_t interval,
                                bool aIsInterval, int32_t *aReturn);
  int32_t SetTimeoutOrInterval(mozilla::dom::Function& aFunction,
                               int32_t aTimeout,
                               const mozilla::dom::Sequence<JS::Value>& aArguments,
                               bool aIsInterval, mozilla::ErrorResult& aError);
  int32_t SetTimeoutOrInterval(JSContext* aCx, const nsAString& aHandler,
                               int32_t aTimeout, bool aIsInterval,
                               mozilla::ErrorResult& aError);
  void ClearTimeoutOrInterval(int32_t aTimerID,
                                  mozilla::ErrorResult& aError);
  nsresult ClearTimeoutOrInterval(int32_t aTimerID)
  {
    mozilla::ErrorResult rv;
    ClearTimeoutOrInterval(aTimerID, rv);
    return rv.ErrorCode();
  }

  
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

  
  
  
  mozilla::CSSIntPoint GetScrollXY(bool aDoFlush);

  void GetScrollMaxXY(int32_t* aScrollMaxX, int32_t* aScrollMaxY,
                      mozilla::ErrorResult& aError);

  
  nsresult GetInnerSize(mozilla::CSSIntSize& aSize);
  nsIntSize GetOuterSize(mozilla::ErrorResult& aError);
  void SetOuterSize(int32_t aLengthCSSPixels, bool aIsWidth,
                    mozilla::ErrorResult& aError);
  nsRect GetInnerScreenRect();

  void ScrollTo(const mozilla::CSSIntPoint& aScroll,
                const mozilla::dom::ScrollOptions& aOptions);

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

  inline int32_t DOMMinTimeoutValue() const;

  
  void ClearDocumentDependentSlots(JSContext* aCx);

  
  already_AddRefed<mozilla::dom::StorageEvent>
  CloneStorageEvent(const nsAString& aType,
                    const nsRefPtr<mozilla::dom::StorageEvent>& aEvent,
                    mozilla::ErrorResult& aRv);

  
  nsDOMWindowList* GetWindowList();

  
  already_AddRefed<nsICSSDeclaration>
    GetComputedStyleHelper(mozilla::dom::Element& aElt,
                           const nsAString& aPseudoElt,
                           bool aDefaultStylesOnly,
                           mozilla::ErrorResult& aError);
  nsresult GetComputedStyleHelper(nsIDOMElement* aElt,
                                  const nsAString& aPseudoElt,
                                  bool aDefaultStylesOnly,
                                  nsIDOMCSSStyleDeclaration** aReturn);

  
  void PreloadLocalStorage();

  
  nsIntPoint GetScreenXY(mozilla::ErrorResult& aError);

  int32_t RequestAnimationFrame(const nsIDocument::FrameRequestCallbackHolder& aCallback,
                                mozilla::ErrorResult& aError);

  nsGlobalWindow* InnerForSetTimeoutOrInterval(mozilla::ErrorResult& aError);

  void PostMessageMoz(JSContext* aCx, JS::Handle<JS::Value> aMessage,
                      const nsAString& aTargetOrigin,
                      JS::Handle<JS::Value> aTransfer,
                      mozilla::ErrorResult& aError);

  already_AddRefed<nsIVariant>
    ShowModalDialog(const nsAString& aUrl, nsIVariant* aArgument,
                    const nsAString& aOptions, mozilla::ErrorResult& aError);

  already_AddRefed<nsIDOMWindow>
    GetContentInternal(mozilla::ErrorResult& aError);

  
  
  
  bool ConfirmDialogIfNeeded();

  
  
  
  
  
  

  
  
  
  
  
  
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

  
  
  bool                   mInnerObjectsFreed : 1;

  
  
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

  
  nsRefPtr<DialogValueHolder> mReturnValue;

  nsRefPtr<mozilla::dom::Navigator> mNavigator;
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
  nsRefPtr<mozilla::dom::Console> mConsole;
  
  
  
  
  nsCOMPtr<nsISupports>         mExternal;

  nsRefPtr<mozilla::dom::MozSelfSupport> mMozSelfSupport;

  nsRefPtr<mozilla::dom::DOMStorage> mLocalStorage;
  nsRefPtr<mozilla::dom::DOMStorage> mSessionStorage;

  
  nsRefPtr<mozilla::EventListenerManager> mListenerManager;
  
  
  
  
  mozilla::LinkedList<nsTimeout> mTimeouts;
  
  
  
  nsTimeout*                    mTimeoutInsertionPoint;
  uint32_t                      mTimeoutPublicIdCounter;
  uint32_t                      mTimeoutFiringDepth;
  nsRefPtr<nsLocation>          mLocation;
  nsRefPtr<nsHistory>           mHistory;

  
  nsCOMPtr<nsIPrincipal> mDocumentPrincipal;

  typedef nsTArray<nsRefPtr<mozilla::dom::StorageEvent>> nsDOMStorageEventArray;
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

  nsTHashtable<nsPtrHashKey<mozilla::DOMEventTargetHelper> > mEventTargetObjects;

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

inline nsISupports*
ToSupports(nsGlobalWindow *p)
{
    return static_cast<nsIDOMEventTarget*>(p);
}

inline nsISupports*
ToCanonicalSupports(nsGlobalWindow *p)
{
    return static_cast<nsIDOMEventTarget*>(p);
}





class nsGlobalChromeWindow : public nsGlobalWindow,
                             public nsIDOMChromeWindow
{
public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMCHROMEWINDOW

  explicit nsGlobalChromeWindow(nsGlobalWindow *aOuterWindow)
    : nsGlobalWindow(aOuterWindow),
      mGroupMessageManagers(1)
  {
    mIsChrome = true;
    mCleanMessageManager = true;
  }

  static PLDHashOperator
  DisconnectGroupMessageManager(const nsAString& aKey,
                                nsIMessageBroadcaster* aMM,
                                void* aUserArg)
  {
    if (aMM) {
      static_cast<nsFrameMessageManager*>(aMM)->Disconnect();
    }
    return PL_DHASH_NEXT;
  }

protected:
  ~nsGlobalChromeWindow()
  {
    NS_ABORT_IF_FALSE(mCleanMessageManager,
                      "chrome windows may always disconnect the msg manager");

    mGroupMessageManagers.EnumerateRead(DisconnectGroupMessageManager, nullptr);
    mGroupMessageManagers.Clear();

    if (mMessageManager) {
      static_cast<nsFrameMessageManager *>(
        mMessageManager.get())->Disconnect();
    }

    mCleanMessageManager = false;
  }

public:
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsGlobalChromeWindow,
                                           nsGlobalWindow)

  using nsGlobalWindow::GetBrowserDOMWindow;
  using nsGlobalWindow::SetBrowserDOMWindow;
  using nsGlobalWindow::GetAttention;
  using nsGlobalWindow::GetAttentionWithCycleCount;
  using nsGlobalWindow::SetCursor;
  using nsGlobalWindow::Maximize;
  using nsGlobalWindow::Minimize;
  using nsGlobalWindow::Restore;
  using nsGlobalWindow::NotifyDefaultButtonLoaded;
  using nsGlobalWindow::GetMessageManager;
  using nsGlobalWindow::GetGroupMessageManager;
  using nsGlobalWindow::BeginWindowMove;

  nsCOMPtr<nsIBrowserDOMWindow> mBrowserDOMWindow;
  nsCOMPtr<nsIMessageBroadcaster> mMessageManager;
  nsInterfaceHashtable<nsStringHashKey, nsIMessageBroadcaster> mGroupMessageManagers;
};






class nsGlobalModalWindow : public nsGlobalWindow,
                            public nsIDOMModalContentWindow
{
public:
  explicit nsGlobalModalWindow(nsGlobalWindow *aOuterWindow)
    : nsGlobalWindow(aOuterWindow)
  {
    mIsModalContentWindow = true;
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMODALCONTENTWINDOW

protected:
  ~nsGlobalModalWindow() {}
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
