









































#ifndef nsGlobalWindow_h___
#define nsGlobalWindow_h___



#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"
#include "nsHashtable.h"
#include "nsDataHashtable.h"
#include "nsCycleCollectionParticipant.h"


#include "nsDOMWindowList.h"
#include "nsIBaseWindow.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIControllers.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMClientInformation.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMNSEventTarget.h"
#include "nsIDOMNavigator.h"
#include "nsIDOMNSLocation.h"
#include "nsIDOMWindowInternal.h"
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
#include "nsIScriptSecurityManager.h"
#include "nsIEventListenerManager.h"
#include "nsIDOMDocument.h"
#include "nsIDOMCrypto.h"
#include "nsIDOMPkcs11.h"
#include "nsIPrincipal.h"
#include "nsPluginArray.h"
#include "nsMimeTypeArray.h"
#include "nsIXPCScriptable.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "mozFlushType.h"
#include "prclist.h"
#include "nsIDOMStorage.h"
#include "nsIDOMStorageList.h"
#include "nsIDOMStorageWindow.h"
#include "nsIDOMOfflineResourceList.h"
#include "nsPIDOMEventTarget.h"

#define DEFAULT_HOME_PAGE "www.mozilla.org"
#define PREF_BROWSER_STARTUP_HOMEPAGE "browser.startup.homepage"

class nsIDOMBarProp;
class nsIDocument;
class nsIContent;
class nsPresContext;
class nsIDOMEvent;
class nsIScrollableView;
class nsIArray;

class nsBarProp;
class nsLocation;
class nsNavigator;
class nsScreen;
class nsHistory;
class nsIDocShellLoadInfo;
class WindowStateHolder;
class nsGlobalWindowObserver;
class nsGlobalWindow;

class nsDOMOfflineResourceList;
class nsDOMOfflineLoadStatusList;


enum OpenAllowValue {
  allowNot = 0,     
  allowNoAbuse,     
  allowWhitelisted  
};

extern nsresult
NS_CreateJSTimeoutHandler(nsIScriptContext *aContext,
                          PRBool *aIsInterval,
                          PRInt32 *aInterval,
                          nsIScriptTimeoutHandler **aRet);






struct nsTimeout : PRCList
{
  nsTimeout();
  ~nsTimeout();

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

  
  PRPackedBool mCleared;

  
  PRPackedBool mRunning;

  
  PRUint32 mPublicId;

  
  PRUint32 mInterval;

  
  
  PRTime mWhen;

  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  PRUint32 mFiringDepth;

  
  
  PopupControlState mPopupState;

  
  nsCOMPtr<nsIScriptTimeoutHandler> mScriptHandler;

private:
  
  PRInt32 mRefCnt;
};




















class nsGlobalWindow : public nsPIDOMWindow,
                       public nsIScriptGlobalObject,
                       public nsIDOMJSWindow,
                       public nsIScriptObjectPrincipal,
                       public nsIDOMEventTarget,
                       public nsPIDOMEventTarget,
                       public nsIDOM3EventTarget,
                       public nsIDOMNSEventTarget,
                       public nsIDOMViewCSS,
                       public nsIDOMStorageWindow,
                       public nsSupportsWeakReference,
                       public nsIInterfaceRequestor,
                       public PRCListStr
{
public:
  
  nsPIDOMWindow* GetPrivateParent();
  
  void ReallyCloseWindow();

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  virtual nsIScriptContext *GetContext();
  virtual JSObject *GetGlobalJSObject();

  virtual nsresult EnsureScriptEnvironment(PRUint32 aLangID);

  virtual nsIScriptContext *GetScriptContext(PRUint32 lang);
  virtual void *GetScriptGlobal(PRUint32 lang);

  
  
  virtual nsresult SetScriptContext(PRUint32 lang, nsIScriptContext *aContext);
  
  virtual void SetGlobalObjectOwner(nsIScriptGlobalObjectOwner* aOwner);
  virtual nsIScriptGlobalObjectOwner *GetGlobalObjectOwner();
  virtual void OnFinalize(PRUint32 aLangID, void *aScriptGlobal);
  virtual void SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts);
  virtual nsresult SetNewArguments(nsIArray *aArguments);

  
  virtual nsIPrincipal* GetPrincipal();

  
  NS_DECL_NSIDOMWINDOW

  
  NS_DECL_NSIDOMWINDOW2

  
  NS_DECL_NSIDOMWINDOWINTERNAL

  
  NS_DECL_NSIDOMJSWINDOW

  
  NS_DECL_NSIDOMEVENTTARGET

  
  NS_DECL_NSIDOM3EVENTTARGET

  
  NS_DECL_NSIDOMNSEVENTTARGET

  
  virtual NS_HIDDEN_(nsPIDOMWindow*) GetPrivateRoot();
  virtual NS_HIDDEN_(nsresult) Activate();
  virtual NS_HIDDEN_(nsresult) Deactivate();
  virtual NS_HIDDEN_(nsIFocusController*) GetRootFocusController();

  virtual NS_HIDDEN_(void) SetOpenerScriptPrincipal(nsIPrincipal* aPrincipal);
  virtual NS_HIDDEN_(nsIPrincipal*) GetOpenerScriptPrincipal();

  virtual NS_HIDDEN_(PopupControlState) PushPopupControlState(PopupControlState state, PRBool aForce) const;
  virtual NS_HIDDEN_(void) PopPopupControlState(PopupControlState state) const;
  virtual NS_HIDDEN_(PopupControlState) GetPopupControlState() const;

  virtual NS_HIDDEN_(nsresult) SaveWindowState(nsISupports **aState);
  virtual NS_HIDDEN_(nsresult) RestoreWindowState(nsISupports *aState);
  virtual NS_HIDDEN_(nsresult) ResumeTimeouts();
  virtual NS_HIDDEN_(nsresult) FireDelayedDOMEvents();

  virtual NS_HIDDEN_(PRBool) WouldReuseInnerWindow(nsIDocument *aNewDocument);

  virtual NS_HIDDEN_(nsPIDOMEventTarget*) GetTargetForDOMEvent()
  {
    return static_cast<nsPIDOMEventTarget*>(GetOuterWindowInternal());
  }
  virtual NS_HIDDEN_(nsPIDOMEventTarget*) GetTargetForEventTargetChain()
  {
    return static_cast<nsPIDOMEventTarget*>(GetCurrentInnerWindowInternal());
  }
  virtual NS_HIDDEN_(nsresult) PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual NS_HIDDEN_(nsresult) PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual NS_HIDDEN_(nsresult) DispatchDOMEvent(nsEvent* aEvent,
                                                nsIDOMEvent* aDOMEvent,
                                                nsPresContext* aPresContext,
                                                nsEventStatus* aEventStatus);
  virtual NS_HIDDEN_(nsresult) GetListenerManager(PRBool aCreateIfNotFound,
                                                  nsIEventListenerManager** aResult);
  virtual NS_HIDDEN_(nsresult) AddEventListenerByIID(nsIDOMEventListener *aListener,
                                                     const nsIID& aIID);
  virtual NS_HIDDEN_(nsresult) RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                                        const nsIID& aIID);
  virtual NS_HIDDEN_(nsresult) GetSystemEventGroup(nsIDOMEventGroup** aGroup);

  virtual NS_HIDDEN_(void) SetDocShell(nsIDocShell* aDocShell);
  virtual NS_HIDDEN_(nsresult) SetNewDocument(nsIDocument *aDocument,
                                  nsISupports *aState,
                                  PRBool aClearScopeHint);
  virtual NS_HIDDEN_(void) SetOpenerWindow(nsIDOMWindowInternal *aOpener,
                                           PRBool aOriginalOpener);
  virtual NS_HIDDEN_(void) EnsureSizeUpToDate();

  virtual NS_HIDDEN_(void) EnterModalState();
  virtual NS_HIDDEN_(void) LeaveModalState();

  
  NS_DECL_NSIDOMVIEWCSS

  
  NS_DECL_NSIDOMABSTRACTVIEW

  
  NS_DECL_NSIDOMSTORAGEWINDOW

  
  NS_DECL_NSIINTERFACEREQUESTOR

  
  nsGlobalWindow(nsGlobalWindow *aOuterWindow);

  static nsGlobalWindow *FromWrapper(nsIXPConnectWrappedNative *wrapper)
  {
    
    return (nsGlobalWindow *)(nsIScriptGlobalObject *)wrapper->Native();
  }

  nsIScriptContext *GetContextInternal()
  {
    if (mOuterWindow) {
      return GetOuterWindowInternal()->mContext;
    }

    return mContext;
  }

  nsIScriptContext *GetScriptContextInternal(PRUint32 aLangID)
  {
    NS_ASSERTION(NS_STID_VALID(aLangID), "Invalid language");
    if (mOuterWindow) {
      return GetOuterWindowInternal()->mScriptContexts[NS_STID_INDEX(aLangID)];
    }

    return mScriptContexts[NS_STID_INDEX(aLangID)];
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

  PRBool IsFrozen() const
  {
    return mIsFrozen;
  }

  PRBool IsCreatingInnerWindow() const
  {
    return  mCreatingInnerWindow;
  }

  nsresult Observe(nsISupports* aSubject, const char* aTopic,
                   const PRUnichar* aData);

  static void ShutDown();
  static PRBool IsCallerChrome();
  static void CloseBlockScriptTerminationFunc(nsISupports *aRef);

  static void RunPendingTimeoutsRecursive(nsGlobalWindow *aTopWindow,
                                          nsGlobalWindow *aWindow);

  friend class WindowStateHolder;

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsGlobalWindow,
                                           nsIScriptGlobalObject)

protected:
  
  virtual ~nsGlobalWindow();
  void CleanUp();
  void ClearControllers();

  void FreeInnerObjects(PRBool aClearScope);

  nsresult SetNewDocument(nsIDocument *aDocument,
                          nsISupports *aState,
                          PRBool aClearScopeHint,
                          PRBool aIsInternalCall);

  
  nsIDOMWindowInternal *GetParentInternal();

  
  PRBool IsPopupSpamWindow()
  {
    if (IsInnerWindow() && !mOuterWindow) {
      return PR_FALSE;
    }

    return GetOuterWindowInternal()->mIsPopupSpam;
  }

  void SetPopupSpamWindow(PRBool aPopup)
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
                                    PRBool aDialog,
                                    PRBool aCalledNoScript,
                                    PRBool aDoJSFixups,
                                    nsIArray *argv,
                                    nsISupports *aExtraArgument,
                                    nsIPrincipal *aCalleePrincipal,
                                    JSContext *aJSCallerContext,
                                    nsIDOMWindow **aReturn);

  static void CloseWindow(nsISupports* aWindow);
  static void ClearWindowScope(nsISupports* aWindow);

  
  
  nsresult SetTimeoutOrInterval(nsIScriptTimeoutHandler *aHandler,
                                PRInt32 interval,
                                PRBool aIsInterval, PRInt32 *aReturn);
  nsresult ClearTimeoutOrInterval(PRInt32 aTimerID);

  
  nsresult SetTimeoutOrInterval(PRBool aIsInterval, PRInt32* aReturn);
  nsresult ClearTimeoutOrInterval();

  
  void RunTimeout(nsTimeout *aTimeout);

  void ClearAllTimeouts();
  
  
  void InsertTimeoutIntoList(nsTimeout *aTimeout);
  static void TimerCallback(nsITimer *aTimer, void *aClosure);

  
  nsresult GetTreeOwner(nsIDocShellTreeOwner** aTreeOwner);
  nsresult GetTreeOwner(nsIBaseWindow** aTreeOwner);
  nsresult GetWebBrowserChrome(nsIWebBrowserChrome** aBrowserChrome);
  
  
  nsresult GetScrollInfo(nsIScrollableView** aScrollableView);
  nsresult SecurityCheckURL(const char *aURL);
  nsresult BuildURIfromBase(const char *aURL,
                            nsIURI **aBuiltURI,
                            PRBool *aFreeSecurityPass, JSContext **aCXused);
  PopupControlState CheckForAbusePoint();
  OpenAllowValue CheckOpenAllow(PopupControlState aAbuseLevel);
  void     FireAbuseEvents(PRBool aBlocked, PRBool aWindow,
                           const nsAString &aPopupURL,
                           const nsAString &aPopupWindowName,
                           const nsAString &aPopupWindowFeatures);
  void FireOfflineStatusEvent();
  
  void FlushPendingNotifications(mozFlushType aType);
  void EnsureReflowFlushAndPaint();
  nsresult CheckSecurityWidthAndHeight(PRInt32* width, PRInt32* height);
  nsresult CheckSecurityLeftAndTop(PRInt32* left, PRInt32* top);
  static PRBool CanSetProperty(const char *aPrefName);

  static void MakeScriptDialogTitle(nsAString &aOutTitle);

  
  nsresult FindInternal(const nsAString& aStr, PRBool caseSensitive,
                       PRBool backwards, PRBool wrapAround, PRBool wholeWord, 
                       PRBool searchInFrames, PRBool showDialog, 
                       PRBool *aReturn);

  nsresult ConvertCharset(const nsAString& aStr, char** aDest);

  PRBool   GetBlurSuppression();

  
  
  nsresult GetScrollXY(PRInt32* aScrollX, PRInt32* aScrollY,
                       PRBool aDoFlush);
  nsresult GetScrollMaxXY(PRInt32* aScrollMaxX, PRInt32* aScrollMaxY);

  PRBool IsFrame()
  {
    return GetParentInternal() != nsnull;
  }

  PRBool DispatchCustomEvent(const char *aEventName);

  
  
  
  PRBool WindowExists(const nsAString& aName, PRBool aLookForCallerOnJSStack);

  already_AddRefed<nsIWidget> GetMainWidget();

  void SuspendTimeouts();

  void Freeze()
  {
    NS_ASSERTION(!IsFrozen(), "Double-freezing?");
    mIsFrozen = PR_TRUE;
  }

  void Thaw()
  {
    mIsFrozen = PR_FALSE;
  }

  PRBool IsInModalState();

  nsTimeout* FirstTimeout() {
    
    return static_cast<nsTimeout*>(PR_LIST_HEAD(&mTimeouts));
  }

  nsTimeout* LastTimeout() {
    
    return static_cast<nsTimeout*>(PR_LIST_TAIL(&mTimeouts));
  }

  PRBool IsTimeout(PRCList* aList) {
    return aList != &mTimeouts;
  }

  
  
  
  
  
  

  
  
  
  
  
  
  PRPackedBool                  mIsFrozen : 1;

  
  
  PRPackedBool                  mFullScreen : 1;
  PRPackedBool                  mIsClosed : 1;
  PRPackedBool                  mInClose : 1;
  
  
  
  PRPackedBool                  mHavePendingClose : 1;
  PRPackedBool                  mHadOriginalOpener : 1;
  PRPackedBool                  mIsPopupSpam : 1;

  
  PRPackedBool                  mBlockScriptedClosingFlag : 1;

  
  PRPackedBool                  mFireOfflineStatusChangeEventOnThaw : 1;

  
  
  PRPackedBool                  mCreatingInnerWindow : 1;
  
  nsCOMPtr<nsIScriptContext>    mContext;
  nsCOMPtr<nsIDOMWindowInternal> mOpener;
  nsCOMPtr<nsIControllers>      mControllers;
  nsCOMPtr<nsIArray>            mArguments;
  nsCOMPtr<nsIArray>            mArgumentsLast;
  nsRefPtr<nsNavigator>         mNavigator;
  nsRefPtr<nsScreen>            mScreen;
  nsRefPtr<nsHistory>           mHistory;
  nsRefPtr<nsDOMWindowList>     mFrames;
  nsRefPtr<nsBarProp>           mMenubar;
  nsRefPtr<nsBarProp>           mToolbar;
  nsRefPtr<nsBarProp>           mLocationbar;
  nsRefPtr<nsBarProp>           mPersonalbar;
  nsRefPtr<nsBarProp>           mStatusbar;
  nsRefPtr<nsBarProp>           mScrollbars;
  nsCOMPtr<nsIWeakReference>    mWindowUtils;
  nsRefPtr<nsLocation>          mLocation;
  nsString                      mStatus;
  nsString                      mDefaultStatus;
  
  nsCOMPtr<nsIScriptContext>    mScriptContexts[NS_STID_ARRAY_UBOUND];
  void *                        mScriptGlobals[NS_STID_ARRAY_UBOUND];
  nsGlobalWindowObserver*       mObserver;

  nsIScriptGlobalObjectOwner*   mGlobalObjectOwner; 
  nsCOMPtr<nsIDOMCrypto>        mCrypto;
  nsCOMPtr<nsIDOMPkcs11>        mPkcs11;


  nsCOMPtr<nsIDOMStorageList>   gGlobalStorageList;

  nsCOMPtr<nsISupports>         mInnerWindowHolders[NS_STID_ARRAY_UBOUND];
  nsCOMPtr<nsIPrincipal> mOpenerScriptPrincipal; 
                                                 

  
  nsCOMPtr<nsIEventListenerManager> mListenerManager;
  PRCList                       mTimeouts;
  
  nsTimeout*                    mTimeoutInsertionPoint;
  PRUint32                      mTimeoutPublicIdCounter;
  PRUint32                      mTimeoutFiringDepth;
  nsCOMPtr<nsIDOMStorage>       mSessionStorage;

  
  nsCOMPtr<nsIPrincipal> mDocumentPrincipal;
  nsCOMPtr<nsIDocument> mDoc;  
  JSObject* mJSObject;

  nsDataHashtable<nsStringHashKey, PRBool> *mPendingStorageEvents;

#ifdef DEBUG
  PRBool mSetOpenerWindowCalled;
#endif

  friend class nsDOMScriptableHelper;
  friend class nsDOMWindowUtils;
  static nsIFactory *sComputedDOMStyleFactory;
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
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsGlobalChromeWindow,
                                                     nsGlobalWindow)

protected:
  nsCOMPtr<nsIBrowserDOMWindow> mBrowserDOMWindow;
};





class nsNavigator : public nsIDOMNavigator,
                    public nsIDOMJSNavigator,
                    public nsIDOMClientInformation
{
public:
  nsNavigator(nsIDocShell *aDocShell);
  virtual ~nsNavigator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNAVIGATOR
  NS_DECL_NSIDOMJSNAVIGATOR
  NS_DECL_NSIDOMCLIENTINFORMATION
  
  void SetDocShell(nsIDocShell *aDocShell);
  nsIDocShell *GetDocShell()
  {
    return mDocShell;
  }

  void LoadingNewDocument();
  nsresult RefreshMIMEArray();

protected:
  nsRefPtr<nsMimeTypeArray> mMimeTypes;
  nsRefPtr<nsPluginArray> mPlugins;
  nsRefPtr<nsDOMOfflineResourceList> mOfflineResources;
  nsRefPtr<nsDOMOfflineLoadStatusList> mPendingOfflineLoads;
  nsIDocShell* mDocShell; 

  static jsval       sPrefInternal_id;
};

class nsIURI;





class nsLocation : public nsIDOMLocation,
                   public nsIDOMNSLocation
{
public:
  nsLocation(nsIDocShell *aDocShell);
  virtual ~nsLocation();

  NS_DECL_ISUPPORTS

  void SetDocShell(nsIDocShell *aDocShell);
  nsIDocShell *GetDocShell();

  
  NS_DECL_NSIDOMLOCATION

  
  NS_DECL_NSIDOMNSLOCATION

protected:
  
  
  
  nsresult GetURI(nsIURI** aURL, PRBool aGetInnermostURI = PR_FALSE);
  nsresult GetWritableURI(nsIURI** aURL);
  nsresult SetURI(nsIURI* aURL, PRBool aReplace = PR_FALSE);
  nsresult SetHrefWithBase(const nsAString& aHref, nsIURI* aBase,
                           PRBool aReplace);
  nsresult SetHrefWithContext(JSContext* cx, const nsAString& aHref,
                              PRBool aReplace);

  nsresult GetSourceURL(JSContext* cx, nsIURI** sourceURL);
  nsresult GetSourceBaseURL(JSContext* cx, nsIURI** sourceURL);
  nsresult GetSourceDocument(JSContext* cx, nsIDocument** aDocument);

  nsresult CheckURL(nsIURI *url, nsIDocShellLoadInfo** aLoadInfo);
  nsresult FindUsableBaseURI(nsIURI * aBaseURI, nsIDocShell * aParent, nsIURI ** aUsableURI);

  nsWeakPtr mDocShell;
};


nsresult NS_NewScriptGlobalObject(PRBool aIsChrome,
                                  nsIScriptGlobalObject **aResult);

#endif 
