









































#ifndef nsGlobalWindow_h___
#define nsGlobalWindow_h___

#include "mozilla/XPCOM.h" 



#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"
#include "nsHashtable.h"
#include "nsDataHashtable.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMScriptObjectHolder.h"


#include "nsDOMWindowList.h"
#include "nsIBaseWindow.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMEventTarget.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMJSWindow.h"
#include "nsIDOMChromeWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptTimeoutHandler.h"
#include "nsITimer.h"
#include "nsIWebBrowserChrome.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMModalContentWindow.h"
#include "nsIScriptSecurityManager.h"
#include "nsEventListenerManager.h"
#include "nsIDOMDocument.h"
#ifndef MOZ_DISABLE_DOMCRYPTO
#include "nsIDOMCrypto.h"
#endif
#include "nsIPrincipal.h"
#include "nsIXPCScriptable.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "nsRect.h"
#include "mozFlushType.h"
#include "prclist.h"
#include "nsIDOMStorageObsolete.h"
#include "nsIDOMStorageList.h"
#include "nsIDOMStorageEvent.h"
#include "nsIDOMStorageIndexedDB.h"
#include "nsIDOMOfflineResourceList.h"
#include "nsIArray.h"
#include "nsIContent.h"
#include "nsIIDBFactory.h"
#include "nsFrameMessageManager.h"
#include "mozilla/TimeStamp.h"
#include "nsIDOMTouchEvent.h"
#include "nsIInlineEventHandlers.h"


#include "jsapi.h"
#include "jswrapper.h"

#define DEFAULT_HOME_PAGE "www.mozilla.org"
#define PREF_BROWSER_STARTUP_HOMEPAGE "browser.startup.homepage"



#define SUCCESSIVE_DIALOG_TIME_LIMIT 3 // 3 sec



#define MAX_DIALOG_COUNT 10

class nsIDOMBarProp;
class nsIDocument;
class nsPresContext;
class nsIDOMEvent;
class nsIScrollableFrame;
class nsIControllers;

class nsBarProp;
class nsLocation;
class nsScreen;
class nsHistory;
class nsPerformance;
class nsIDocShellLoadInfo;
class WindowStateHolder;
class nsGlobalWindowObserver;
class nsGlobalWindow;
class nsDummyJavaPluginOwner;
class PostMessageEvent;
class nsRunnable;

class nsDOMOfflineResourceList;
class nsDOMMozURLProperty;

#ifdef MOZ_DISABLE_DOMCRYPTO
class nsIDOMCrypto;
#endif

namespace mozilla {
namespace dom {
class Navigator;
} 
} 

extern nsresult
NS_CreateJSTimeoutHandler(nsGlobalWindow *aWindow,
                          bool *aIsInterval,
                          PRInt32 *aInterval,
                          nsIScriptTimeoutHandler **aRet);






struct nsTimeout : PRCList
{
  nsTimeout();
  ~nsTimeout();

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsTimeout)

  nsrefcnt Release();
  nsrefcnt AddRef();

  nsTimeout* Next() {
    
    return static_cast<nsTimeout*>(PR_NEXT_LINK(this));
  }

  nsTimeout* Prev() {
    
    return static_cast<nsTimeout*>(PR_PREV_LINK(this));
  }

  
  nsRefPtr<nsGlobalWindow> mWindow;

  
  nsCOMPtr<nsITimer> mTimer;

  
  bool mCleared;

  
  bool mRunning;

  
  bool mIsInterval;

  
  PRUint32 mPublicId;

  
  PRUint32 mInterval;

  
  
  
  
  mozilla::TimeStamp mWhen;
  
  mozilla::TimeDuration mTimeRemaining;

  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  PRUint32 mFiringDepth;

  
  PRUint32 mNestingLevel;

  
  
  PopupControlState mPopupState;

  
  nsCOMPtr<nsIScriptTimeoutHandler> mScriptHandler;

private:
  
  nsAutoRefCnt mRefCnt;
};





class nsOuterWindowProxy : public js::Wrapper
{
public:
  nsOuterWindowProxy() : js::Wrapper((uintN)0) {}

  virtual bool isOuterWindow() {
    return true;
  }
  JSString *obj_toString(JSContext *cx, JSObject *wrapper);
  void finalize(JSContext *cx, JSObject *proxy);

  static nsOuterWindowProxy singleton;
};

JSObject *NS_NewOuterWindowProxy(JSContext *cx, JSObject *parent);




















class nsGlobalWindow : public nsPIDOMWindow,
                       public nsIScriptGlobalObject,
                       public nsIDOMJSWindow,
                       public nsIScriptObjectPrincipal,
                       public nsIDOMEventTarget,
                       public nsIDOMStorageIndexedDB,
                       public nsSupportsWeakReference,
                       public nsIInterfaceRequestor,
                       public nsWrapperCache,
                       public PRCListStr,
                       public nsIDOMWindowPerformance,
                       public nsITouchEventReceiver,
                       public nsIInlineEventHandlers
{
public:
  friend class nsDOMMozURLProperty;

  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;
  typedef mozilla::dom::Navigator Navigator;
  typedef nsDataHashtable<nsUint64HashKey, nsGlobalWindow*> WindowByIdTable;

  
  nsPIDOMWindow* GetPrivateParent();
  
  void ReallyCloseWindow();

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  JSObject *WrapObject(JSContext *cx, XPCWrappedNativeScope *scope,
                       bool *triedToWrap)
  {
    NS_ASSERTION(IsOuterWindow(),
                 "Inner window supports nsWrapperCache, fix WrapObject!");
    *triedToWrap = true;
    return EnsureInnerWindow() ? GetWrapper() : nsnull;
  }

  
  virtual nsIScriptContext *GetContext();
  virtual JSObject *GetGlobalJSObject();
  JSObject *FastGetGlobalJSObject()
  {
    return mJSObject;
  }

  virtual nsresult EnsureScriptEnvironment(PRUint32 aLangID);

  virtual nsIScriptContext *GetScriptContext(PRUint32 lang);

  
  
  virtual nsresult SetScriptContext(PRUint32 lang, nsIScriptContext *aContext);
  
  virtual void OnFinalize(JSObject* aObject);
  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts);

  
  virtual nsIPrincipal* GetPrincipal();

  
  NS_DECL_NSIDOMWINDOW

  
  NS_DECL_NSIDOMWINDOWPERFORMANCE

  
  NS_DECL_NSIDOMJSWINDOW

  
  NS_DECL_NSIDOMEVENTTARGET

  
  NS_DECL_NSITOUCHEVENTRECEIVER

  
  NS_DECL_NSIINLINEEVENTHANDLERS

  
  virtual NS_HIDDEN_(nsPIDOMWindow*) GetPrivateRoot();
  virtual NS_HIDDEN_(void) ActivateOrDeactivate(bool aActivate);
  virtual NS_HIDDEN_(void) SetActive(bool aActive);
  virtual NS_HIDDEN_(void) SetIsBackground(bool aIsBackground);
  virtual NS_HIDDEN_(void) SetChromeEventHandler(nsIDOMEventTarget* aChromeEventHandler);

  virtual NS_HIDDEN_(void) SetOpenerScriptPrincipal(nsIPrincipal* aPrincipal);
  virtual NS_HIDDEN_(nsIPrincipal*) GetOpenerScriptPrincipal();

  virtual NS_HIDDEN_(PopupControlState) PushPopupControlState(PopupControlState state, bool aForce) const;
  virtual NS_HIDDEN_(void) PopPopupControlState(PopupControlState state) const;
  virtual NS_HIDDEN_(PopupControlState) GetPopupControlState() const;

  virtual NS_HIDDEN_(nsresult) SaveWindowState(nsISupports **aState);
  virtual NS_HIDDEN_(nsresult) RestoreWindowState(nsISupports *aState);
  virtual NS_HIDDEN_(void) SuspendTimeouts(PRUint32 aIncrease = 1,
                                           bool aFreezeChildren = true);
  virtual NS_HIDDEN_(nsresult) ResumeTimeouts(bool aThawChildren = true);
  virtual NS_HIDDEN_(PRUint32) TimeoutSuspendCount();
  virtual NS_HIDDEN_(nsresult) FireDelayedDOMEvents();
  virtual NS_HIDDEN_(bool) IsFrozen() const
  {
    return mIsFrozen;
  }

  virtual NS_HIDDEN_(bool) WouldReuseInnerWindow(nsIDocument *aNewDocument);

  virtual NS_HIDDEN_(void) SetDocShell(nsIDocShell* aDocShell);
  virtual NS_HIDDEN_(nsresult) SetNewDocument(nsIDocument *aDocument,
                                              nsISupports *aState,
                                              bool aForceReuseInnerWindow);
  void DispatchDOMWindowCreated();
  virtual NS_HIDDEN_(void) SetOpenerWindow(nsIDOMWindow* aOpener,
                                           bool aOriginalOpener);
  virtual NS_HIDDEN_(void) EnsureSizeUpToDate();

  virtual NS_HIDDEN_(nsIDOMWindow*) EnterModalState();
  virtual NS_HIDDEN_(void) LeaveModalState(nsIDOMWindow* aWindow);

  virtual NS_HIDDEN_(bool) CanClose();
  virtual NS_HIDDEN_(nsresult) ForceClose();

  virtual NS_HIDDEN_(void) SetHasOrientationEventListener();
  virtual NS_HIDDEN_(void) RemoveOrientationEventListener();
  virtual NS_HIDDEN_(void) MaybeUpdateTouchState();
  virtual NS_HIDDEN_(void) UpdateTouchState();
  virtual NS_HIDDEN_(bool) DispatchCustomEvent(const char *aEventName);

  
  NS_DECL_NSIDOMSTORAGEINDEXEDDB

  
  NS_DECL_NSIINTERFACEREQUESTOR

  
  nsGlobalWindow(nsGlobalWindow *aOuterWindow);

  static nsGlobalWindow *FromSupports(nsISupports *supports)
  {
    
    return (nsGlobalWindow *)(nsIScriptGlobalObject *)supports;
  }
  static nsISupports *ToSupports(nsGlobalWindow *win)
  {
    
    return (nsISupports *)(nsIScriptGlobalObject *)win;
  }
  static nsGlobalWindow *FromWrapper(nsIXPConnectWrappedNative *wrapper)
  {
    return FromSupports(wrapper->Native());
  }

  inline nsGlobalWindow *GetTop()
  {
    nsCOMPtr<nsIDOMWindow> top;
    GetTop(getter_AddRefs(top));
    if (top)
      return static_cast<nsGlobalWindow *>(static_cast<nsIDOMWindow *>(top.get()));
    return nsnull;
  }

  
  
  
  bool DialogOpenAttempted();

  
  
  bool AreDialogsBlocked();

  
  
  
  bool ConfirmDialogAllowed();

  
  void PreventFurtherDialogs();

  virtual void SetHasAudioAvailableEventListeners();

  nsIScriptContext *GetContextInternal()
  {
    if (mOuterWindow) {
      return GetOuterWindowInternal()->mContext;
    }

    return mContext;
  }

  nsIScriptContext *GetScriptContextInternal(PRUint32 aLangID)
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

  nsGlobalWindow *GetCurrentInnerWindowInternal()
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

  nsresult Observe(nsISupports* aSubject, const char* aTopic,
                   const PRUnichar* aData);

  static void Init();
  static void ShutDown();
  static void CleanupCachedXBLHandlers(nsGlobalWindow* aWindow);
  static bool IsCallerChrome();
  static void CloseBlockScriptTerminationFunc(nsISupports *aRef);

  static void RunPendingTimeoutsRecursive(nsGlobalWindow *aTopWindow,
                                          nsGlobalWindow *aWindow);

  friend class WindowStateHolder;

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsGlobalWindow,
                                                         nsIScriptGlobalObject)

  void InitJavaProperties();

  virtual NS_HIDDEN_(void*)
    GetCachedXBLPrototypeHandler(nsXBLPrototypeHandler* aKey);

  virtual NS_HIDDEN_(void)
    CacheXBLPrototypeHandler(nsXBLPrototypeHandler* aKey,
                             nsScriptObjectHolder& aHandler);

  virtual bool TakeFocus(bool aFocus, PRUint32 aFocusMethod);
  virtual void SetReadyForFocus();
  virtual void PageHidden();
  virtual nsresult DispatchAsyncHashchange(nsIURI *aOldURI, nsIURI *aNewURI);
  virtual nsresult DispatchSyncPopState();

  virtual nsresult SetArguments(nsIArray *aArguments, nsIPrincipal *aOrigin);

  static bool DOMWindowDumpEnabled();

  void MaybeForgiveSpamCount();
  bool IsClosedOrClosing() {
    return (mIsClosed ||
            mInClose ||
            mHavePendingClose ||
            mCleanedUp);
  }

  static void FirePopupBlockedEvent(nsIDOMDocument* aDoc,
                                    nsIDOMWindow *aRequestingWindow, nsIURI *aPopupURI,
                                    const nsAString &aPopupWindowName,
                                    const nsAString &aPopupWindowFeatures);

  virtual PRUint32 GetSerial() {
    return mSerial;
  }

  static nsGlobalWindow* GetOuterWindowWithId(PRUint64 aWindowID) {
    if (!sWindowsById) {
      return nsnull;
    }

    nsGlobalWindow* outerWindow = sWindowsById->Get(aWindowID);
    return outerWindow && !outerWindow->IsInnerWindow() ? outerWindow : nsnull;
  }

  static nsGlobalWindow* GetInnerWindowWithId(PRUint64 aInnerWindowID) {
    if (!sWindowsById) {
      return nsnull;
    }

    nsGlobalWindow* innerWindow = sWindowsById->Get(aInnerWindowID);
    return innerWindow && innerWindow->IsInnerWindow() ? innerWindow : nsnull;
  }

  static bool HasIndexedDBSupport();

  static bool HasPerformanceSupport();

  static WindowByIdTable* GetWindowsTable() {
    return sWindowsById;
  }

  PRInt64 SizeOf() const;

private:
  
  void EnableDeviceMotionUpdates();

  
  void DisableDeviceMotionUpdates();

protected:
  friend class HashchangeCallback;
  friend class nsBarProp;

  
  virtual ~nsGlobalWindow();
  void CleanUp(bool aIgnoreModalDialog);
  void ClearControllers();
  static void TryClearWindowScope(nsISupports* aWindow);
  void ClearScopeWhenAllScriptsStop();
  nsresult FinalClose();

  void FreeInnerObjects(bool aClearScope);
  nsGlobalWindow *CallerInnerWindow();

  nsresult InnerSetNewDocument(nsIDocument* aDocument);

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

  
  































  NS_HIDDEN_(nsresult) OpenInternal(const nsAString& aUrl,
                                    const nsAString& aName,
                                    const nsAString& aOptions,
                                    bool aDialog,
                                    bool aContentModal,
                                    bool aCalledNoScript,
                                    bool aDoJSFixups,
                                    nsIArray *argv,
                                    nsISupports *aExtraArgument,
                                    nsIPrincipal *aCalleePrincipal,
                                    JSContext *aJSCallerContext,
                                    nsIDOMWindow **aReturn);

  static void CloseWindow(nsISupports* aWindow);
  static void ClearWindowScope(nsISupports* aWindow);

  
  
  
  nsresult SetTimeoutOrInterval(nsIScriptTimeoutHandler *aHandler,
                                PRInt32 interval,
                                bool aIsInterval, PRInt32 *aReturn);
  nsresult ClearTimeoutOrInterval(PRInt32 aTimerID);

  
  nsresult SetTimeoutOrInterval(bool aIsInterval, PRInt32* aReturn);
  nsresult ClearTimeoutOrInterval();
  nsresult ResetTimersForNonBackgroundWindow();

  
  void RunTimeout(nsTimeout *aTimeout);
  void RunTimeout() { RunTimeout(nsnull); }

  void ClearAllTimeouts();
  
  
  void InsertTimeoutIntoList(nsTimeout *aTimeout);
  static void TimerCallback(nsITimer *aTimer, void *aClosure);

  
  nsresult GetTreeOwner(nsIDocShellTreeOwner** aTreeOwner);
  nsresult GetTreeOwner(nsIBaseWindow** aTreeOwner);
  nsresult GetWebBrowserChrome(nsIWebBrowserChrome** aBrowserChrome);
  
  
  nsIScrollableFrame *GetScrollFrame();
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
  nsresult FireHashchange(const nsAString &aOldURL, const nsAString &aNewURL);

  void FlushPendingNotifications(mozFlushType aType);
  void EnsureReflowFlushAndPaint();
  nsresult CheckSecurityWidthAndHeight(PRInt32* width, PRInt32* height);
  nsresult CheckSecurityLeftAndTop(PRInt32* left, PRInt32* top);

  
  nsresult SetCSSViewportWidthAndHeight(nscoord width, nscoord height);
  
  nsresult SetDocShellWidthAndHeight(PRInt32 width, PRInt32 height);

  static bool CanSetProperty(const char *aPrefName);

  static void MakeScriptDialogTitle(nsAString &aOutTitle);

  bool CanMoveResizeWindows();

  bool     GetBlurSuppression();

  
  
  nsresult GetScrollXY(PRInt32* aScrollX, PRInt32* aScrollY,
                       bool aDoFlush);
  nsresult GetScrollMaxXY(PRInt32* aScrollMaxX, PRInt32* aScrollMaxY);
  
  nsresult GetOuterSize(nsIntSize* aSizeCSSPixels);
  nsresult SetOuterSize(PRInt32 aLengthCSSPixels, bool aIsWidth);
  nsRect GetInnerScreenRect();

  bool IsFrame()
  {
    return GetParentInternal() != nsnull;
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

  nsTimeout* FirstTimeout() {
    
    return static_cast<nsTimeout*>(PR_LIST_HEAD(&mTimeouts));
  }

  nsTimeout* LastTimeout() {
    
    return static_cast<nsTimeout*>(PR_LIST_TAIL(&mTimeouts));
  }

  bool IsTimeout(PRCList* aList) {
    return aList != &mTimeouts;
  }

  
  
  
  PRInt32 DevToCSSIntPixels(PRInt32 px);
  PRInt32 CSSToDevIntPixels(PRInt32 px);
  nsIntSize DevToCSSIntPixels(nsIntSize px);
  nsIntSize CSSToDevIntPixels(nsIntSize px);

  virtual void SetFocusedNode(nsIContent* aNode,
                              PRUint32 aFocusMethod = 0,
                              bool aNeedsFocus = false);

  virtual PRUint32 GetFocusMethod();

  virtual bool ShouldShowFocusRing();

  virtual void SetKeyboardIndicators(UIStateChangeType aShowAccelerators,
                                     UIStateChangeType aShowFocusRings);
  virtual void GetKeyboardIndicators(bool* aShowAccelerators,
                                     bool* aShowFocusRings);

  void UpdateCanvasFocus(bool aFocusChanged, nsIContent* aNewContent);

  already_AddRefed<nsPIWindowRoot> GetTopWindowRoot();

  static void NotifyDOMWindowDestroyed(nsGlobalWindow* aWindow);
  void NotifyWindowIDDestroyed(const char* aTopic);

  static void NotifyDOMWindowFrozen(nsGlobalWindow* aWindow);
  static void NotifyDOMWindowThawed(nsGlobalWindow* aWindow);
  
  void ClearStatus();

  virtual void UpdateParentTarget();

  bool GetIsTabModalPromptAllowed();

  inline PRInt32 DOMMinTimeoutValue() const;

  
  
  
  
  
  

  
  
  
  
  
  
  bool                          mIsFrozen : 1;

  
  
  bool                          mDidInitJavaProperties : 1;
  
  
  
  bool                          mFullScreen : 1;
  bool                          mIsClosed : 1;
  bool                          mInClose : 1;
  
  
  
  bool                          mHavePendingClose : 1;
  bool                          mHadOriginalOpener : 1;
  bool                          mIsPopupSpam : 1;

  
  bool                          mBlockScriptedClosingFlag : 1;

  
  bool                          mFireOfflineStatusChangeEventOnThaw : 1;

  
  
  bool                          mCreatingInnerWindow : 1;

  
  bool                          mIsChrome : 1;

  
  
  
  bool                          mCleanMessageManager : 1;

  
  
  bool                   mNeedsFocus : 1;
  bool                   mHasFocus : 1;

  
  bool                   mShowAccelerators : 1;

  
  bool                   mShowFocusRings : 1;

  
  
  bool                   mShowFocusRingForContent : 1;

  
  
  bool                   mFocusByKeyOccurred : 1;

  
  bool                   mHasDeviceMotion : 1;

  
  bool                   mNotifiedIDDestroyed : 1;

  nsCOMPtr<nsIScriptContext>    mContext;
  nsWeakPtr                     mOpener;
  nsCOMPtr<nsIControllers>      mControllers;
  nsCOMPtr<nsIArray>            mArguments;
  nsCOMPtr<nsIArray>            mArgumentsLast;
  nsCOMPtr<nsIPrincipal>        mArgumentsOrigin;
  nsRefPtr<Navigator>           mNavigator;
  nsRefPtr<nsScreen>            mScreen;
  nsRefPtr<nsPerformance>       mPerformance;
  nsRefPtr<nsDOMWindowList>     mFrames;
  nsRefPtr<nsBarProp>           mMenubar;
  nsRefPtr<nsBarProp>           mToolbar;
  nsRefPtr<nsBarProp>           mLocationbar;
  nsRefPtr<nsBarProp>           mPersonalbar;
  nsRefPtr<nsBarProp>           mStatusbar;
  nsRefPtr<nsBarProp>           mScrollbars;
  nsCOMPtr<nsIWeakReference>    mWindowUtils;
  nsString                      mStatus;
  nsString                      mDefaultStatus;
  
  nsGlobalWindowObserver*       mObserver;
#ifndef MOZ_DISABLE_DOMCRYPTO
  nsCOMPtr<nsIDOMCrypto>        mCrypto;
#endif
  nsCOMPtr<nsIDOMStorage>      mLocalStorage;
  nsCOMPtr<nsIDOMStorage>      mSessionStorage;

  nsCOMPtr<nsIXPConnectJSObjectHolder> mInnerWindowHolder;
  nsCOMPtr<nsIPrincipal> mOpenerScriptPrincipal; 
                                                 

  
  nsRefPtr<nsEventListenerManager> mListenerManager;
  
  
  
  
  PRCList                       mTimeouts;
  
  
  
  nsTimeout*                    mTimeoutInsertionPoint;
  PRUint32                      mTimeoutPublicIdCounter;
  PRUint32                      mTimeoutFiringDepth;
  nsRefPtr<nsLocation>          mLocation;
  nsRefPtr<nsHistory>           mHistory;

  
  
  nsRefPtr<nsDummyJavaPluginOwner> mDummyJavaPluginOwner;

  
  nsCOMPtr<nsIPrincipal> mDocumentPrincipal;
  nsCOMPtr<nsIDocument> mDoc;  
  JSObject* mJSObject;

  typedef nsCOMArray<nsIDOMStorageEvent> nsDOMStorageEventArray;
  nsDOMStorageEventArray mPendingStorageEvents;
  nsAutoPtr< nsDataHashtable<nsStringHashKey, bool> > mPendingStorageEventsObsolete;

  PRUint32 mTimeoutsSuspendDepth;

  
  PRUint32 mFocusMethod;

  PRUint32 mSerial;

#ifdef DEBUG
  bool mSetOpenerWindowCalled;
  nsCOMPtr<nsIURI> mLastOpenedURI;
#endif

  bool mCleanedUp, mCallCleanUpAfterModalDialogCloses;

  nsCOMPtr<nsIDOMOfflineResourceList> mApplicationCache;

  nsDataHashtable<nsVoidPtrHashKey, void*> mCachedXBLPrototypeHandlers;

  nsCOMPtr<nsIDocument> mSuspendedDoc;

  nsCOMPtr<nsIIDBFactory> mIndexedDB;

  
  
  PRUint32                      mDialogAbuseCount;

  
  
  
  
  
  TimeStamp                     mLastDialogQuitTime;
  bool                          mDialogDisabled;

  nsRefPtr<nsDOMMozURLProperty> mURLProperty;

  friend class nsDOMScriptableHelper;
  friend class nsDOMWindowUtils;
  friend class PostMessageEvent;
  static nsIDOMStorageList* sGlobalStorageList;

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
  nsCOMPtr<nsIChromeFrameMessageManager> mMessageManager;
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

  virtual NS_HIDDEN_(nsresult) SetNewDocument(nsIDocument *aDocument,
                                              nsISupports *aState,
                                              bool aForceReuseInnerWindow);

protected:
  nsCOMPtr<nsIVariant> mReturnValue;
};


nsresult
NS_NewScriptGlobalObject(bool aIsChrome, bool aIsModalContentWindow,
                         nsIScriptGlobalObject **aResult);

#endif 
