















































#include "nsGlobalWindow.h"
#include "nsScreen.h"
#include "nsHistory.h"
#include "nsBarProps.h"
#include "nsDOMStorage.h"
#include "nsDOMOfflineResourceList.h"
#include "nsDOMError.h"


#include "nsXPIDLString.h"
#include "nsJSUtils.h"
#include "prmem.h"
#include "jsapi.h"              
#include "jsdbgapi.h"           
#include "nsReadableUtils.h"
#include "nsDOMClassInfo.h"
#include "nsContentUtils.h"


#include "nsIEventListenerManager.h"
#include "nsEscape.h"
#include "nsStyleCoord.h"
#include "nsMimeTypeArray.h"
#include "nsNetUtil.h"
#include "nsICachingChannel.h"
#include "nsPluginArray.h"
#include "nsIPluginHost.h"
#include "nsGeolocation.h"
#include "nsContentCID.h"
#include "nsLayoutStatics.h"
#include "nsCycleCollector.h"
#include "nsCCUncollectableMarker.h"
#include "nsDOMThreadService.h"


#include "nsIFrame.h"
#include "nsICanvasFrame.h"
#include "nsIWidget.h"
#include "nsIBaseWindow.h"
#include "nsIAccelerometer.h"
#include "nsWidgetsCID.h"
#include "nsIContent.h"
#include "nsIContentViewerEdit.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIEditorDocShell.h"
#include "nsIDocCharset.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMCrypto.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMEvent.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMessageEvent.h"
#include "nsIDOMPopupBlockedEvent.h"
#include "nsIDOMOfflineResourceList.h"
#include "nsIDOMGeoGeolocation.h"
#include "nsDOMString.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsThreadUtils.h"
#include "nsIEventStateManager.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIPrefBranch.h"
#include "nsIPresShell.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIProgrammingLanguage.h"
#include "nsIAuthPrompt.h"
#include "nsIServiceManager.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScrollableView.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsISelectionController.h"
#include "nsISelection.h"
#include "nsIPrompt.h"
#include "nsIWebNavigation.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowserFind.h"  
#include "nsIWebContentHandlerRegistrar.h"
#include "nsIWindowMediator.h"  
#include "nsComputedDOMStyle.h"
#include "nsIEntropyCollector.h"
#include "nsDOMCID.h"
#include "nsDOMError.h"
#include "nsDOMWindowUtils.h"
#include "nsIWindowWatcher.h"
#include "nsPIWindowWatcher.h"
#include "nsIContentViewer.h"
#include "nsDOMClassInfo.h"
#include "nsIJSNativeInitializer.h"
#include "nsIScriptError.h"
#include "nsIScriptEventManager.h" 
#include "nsIConsoleService.h"
#include "nsIControllers.h"
#include "nsIControllerContext.h"
#include "nsGlobalWindowCommands.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsCSSProps.h"
#include "nsIURIFixup.h"
#include "nsCDefaultURIFixup.h"
#include "nsEventDispatcher.h"
#include "nsIObserverService.h"
#include "nsIXULAppInfo.h"
#include "nsNetUtil.h"
#include "nsFocusManager.h"
#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#include "nsIDOMXULControlElement.h"
#include "nsIFrame.h"
#endif

#include "plbase64.h"

#ifdef NS_PRINTING
#include "nsIPrintSettings.h"
#include "nsIPrintSettingsService.h"
#include "nsIWebBrowserPrint.h"
#endif

#include "nsWindowRoot.h"
#include "nsNetCID.h"
#include "nsIArray.h"
#include "nsIScriptRuntime.h"


#include "nsIDOMXULDocument.h"
#include "nsIDOMXULCommandDispatcher.h"

#include "nsBindingManager.h"
#include "nsIXBLService.h"



#include "nsIPopupWindowManager.h"

#include "nsIDragService.h"

#ifdef MOZ_LOGGING

#define FORCE_PR_LOG 1
#endif
#include "prlog.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gDOMLeakPRLog;
#endif

nsIDOMStorageList *nsGlobalWindow::sGlobalStorageList  = nsnull;

static nsIEntropyCollector *gEntropyCollector          = nsnull;
static PRInt32              gRefCnt                    = 0;
static PRInt32              gOpenPopupSpamCount        = 0;
static PopupControlState    gPopupControlState         = openAbused;
static PRInt32              gRunningTimeoutDepth       = 0;
static PRBool               gMouseDown                 = PR_FALSE;
static PRBool               gDragServiceDisabled       = PR_FALSE;
static FILE                *gDumpFile                  = nsnull;

#ifdef DEBUG
static PRUint32             gSerialCounter             = 0;
#endif

#ifdef DEBUG_jst
PRInt32 gTimeoutCnt                                    = 0;
#endif

#if !(defined(NS_DEBUG) || defined(MOZ_ENABLE_JS_DUMP))
static PRBool               gDOMWindowDumpEnabled      = PR_FALSE;
#endif

#if defined(DEBUG_bryner) || defined(DEBUG_chb)
#define DEBUG_PAGE_CACHE
#endif


#define DOM_MIN_TIMEOUT_VALUE 10 // 10ms




#define DOM_MAX_TIMEOUT_VALUE    PR_BIT(8 * sizeof(PRIntervalTime) - 1)

#define FORWARD_TO_OUTER(method, args, err_rval)                              \
  PR_BEGIN_MACRO                                                              \
  if (IsInnerWindow()) {                                                      \
    nsGlobalWindow *outer = GetOuterWindowInternal();                         \
    if (!outer) {                                                             \
      NS_WARNING("No outer window available!");                               \
      return err_rval;                                                        \
    }                                                                         \
    return outer->method args;                                                \
  }                                                                           \
  PR_END_MACRO

#define FORWARD_TO_OUTER_VOID(method, args)                                   \
  PR_BEGIN_MACRO                                                              \
  if (IsInnerWindow()) {                                                      \
    nsGlobalWindow *outer = GetOuterWindowInternal();                         \
    if (!outer) {                                                             \
      NS_WARNING("No outer window available!");                               \
      return;                                                                 \
    }                                                                         \
    outer->method args;                                                       \
    return;                                                                   \
  }                                                                           \
  PR_END_MACRO

#define FORWARD_TO_OUTER_CHROME(method, args, err_rval)                       \
  PR_BEGIN_MACRO                                                              \
  if (IsInnerWindow()) {                                                      \
    nsGlobalWindow *outer = GetOuterWindowInternal();                         \
    if (!outer) {                                                             \
      NS_WARNING("No outer window available!");                               \
      return err_rval;                                                        \
    }                                                                         \
    return ((nsGlobalChromeWindow *)outer)->method args;                      \
  }                                                                           \
  PR_END_MACRO

#define FORWARD_TO_OUTER_MODAL_CONTENT_WINDOW(method, args, err_rval)         \
  PR_BEGIN_MACRO                                                              \
  if (IsInnerWindow()) {                                                      \
    nsGlobalWindow *outer = GetOuterWindowInternal();                         \
    if (!outer) {                                                             \
      NS_WARNING("No outer window available!");                               \
      return err_rval;                                                        \
    }                                                                         \
    return ((nsGlobalModalWindow *)outer)->method args;                       \
  }                                                                           \
  PR_END_MACRO

#define FORWARD_TO_INNER(method, args, err_rval)                              \
  PR_BEGIN_MACRO                                                              \
  if (IsOuterWindow()) {                                                      \
    if (!mInnerWindow) {                                                      \
      NS_WARNING("No inner window available!");                               \
      return err_rval;                                                        \
    }                                                                         \
    return GetCurrentInnerWindowInternal()->method args;                      \
  }                                                                           \
  PR_END_MACRO

#define FORWARD_TO_INNER_MODAL_CONTENT_WINDOW(method, args, err_rval)         \
  PR_BEGIN_MACRO                                                              \
  if (IsOuterWindow()) {                                                      \
    if (!mInnerWindow) {                                                      \
      NS_WARNING("No inner window available!");                               \
      return err_rval;                                                        \
    }                                                                         \
    return ((nsGlobalModalWindow*)GetCurrentInnerWindowInternal())->method args; \
  }                                                                           \
  PR_END_MACRO

#define FORWARD_TO_INNER_VOID(method, args)                                   \
  PR_BEGIN_MACRO                                                              \
  if (IsOuterWindow()) {                                                      \
    if (!mInnerWindow) {                                                      \
      NS_WARNING("No inner window available!");                               \
      return;                                                                 \
    }                                                                         \
    GetCurrentInnerWindowInternal()->method args;                             \
    return;                                                                   \
  }                                                                           \
  PR_END_MACRO



#define FORWARD_TO_INNER_CREATE(method, args, err_rval)                       \
  PR_BEGIN_MACRO                                                              \
  if (IsOuterWindow()) {                                                      \
    if (!mInnerWindow) {                                                      \
      if (mIsClosed) {                                                        \
        return err_rval;                                                      \
      }                                                                       \
      nsCOMPtr<nsIDOMDocument> doc;                                           \
      nsresult fwdic_nr = GetDocument(getter_AddRefs(doc));                   \
      NS_ENSURE_SUCCESS(fwdic_nr, err_rval);                                  \
      if (!mInnerWindow) {                                                    \
        return err_rval;                                                      \
      }                                                                       \
    }                                                                         \
    return GetCurrentInnerWindowInternal()->method args;                      \
  }                                                                           \
  PR_END_MACRO


static NS_DEFINE_CID(kXULControllersCID, NS_XULCONTROLLERS_CID);

static const char sJSStackContractID[] = "@mozilla.org/js/xpc/ContextStack;1";

static const char kCryptoContractID[] = NS_CRYPTO_CONTRACTID;
static const char kPkcs11ContractID[] = NS_PKCS11_CONTRACTID;

static PRBool
IsAboutBlank(nsIURI* aURI)
{
  NS_PRECONDITION(aURI, "Must have URI");
    
  
  PRBool isAbout = PR_FALSE;
  if (NS_FAILED(aURI->SchemeIs("about", &isAbout)) || !isAbout) {
    return PR_FALSE;
  }
    
  nsCAutoString str;
  aURI->GetSpec(str);
  return str.EqualsLiteral("about:blank");  
}

static void
StripNullChars(const nsAString& aInStr,
               nsAString& aOutStr)
{
  
  
  PRInt32 firstNullPos = aInStr.FindChar('\0');
  if (firstNullPos == kNotFound) {
    aOutStr.Assign(aInStr);
    return;
  }
  
  nsAString::const_iterator start, end;
  aInStr.BeginReading(start); 
  aInStr.EndReading(end); 

  while (start != end) {
    if (*start != '\0')
      aOutStr.Append(*start);
    ++start;
  }
}

class nsDummyJavaPluginOwner : public nsIPluginInstanceOwner
{
public:
  nsDummyJavaPluginOwner(nsIDocument *aDocument)
    : mDocument(aDocument)
  {
  }

  void Destroy();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIPLUGININSTANCEOWNER

  NS_IMETHOD GetURL(const char *aURL, const char *aTarget, void *aPostData,
                    PRUint32 aPostDataLen, void *aHeadersData,
                    PRUint32 aHeadersDataLen, PRBool aIsFile = PR_FALSE);
  NS_IMETHOD ShowStatus(const PRUnichar *aStatusMsg);
  NPError ShowNativeContextMenu(NPMenu* menu, void* event);
  NPBool ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                      double *destX, double *destY, NPCoordinateSpace destSpace);

  NS_DECL_CYCLE_COLLECTION_CLASS(nsDummyJavaPluginOwner)

private:
  nsCOMPtr<nsIPluginInstance> mInstance;
  nsCOMPtr<nsIDocument> mDocument;
};

NS_IMPL_CYCLE_COLLECTION_2(nsDummyJavaPluginOwner, mDocument, mInstance)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDummyJavaPluginOwner)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIPluginInstanceOwner)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDummyJavaPluginOwner)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDummyJavaPluginOwner)


void
nsDummyJavaPluginOwner::Destroy()
{
  
  if (mInstance) {
    mInstance->Stop();
    mInstance->InvalidateOwner();
    mInstance = nsnull;
  }

  mDocument = nsnull;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::SetInstance(nsIPluginInstance *aInstance)
{
  mInstance = aInstance;

  return NS_OK;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::GetInstance(nsIPluginInstance *&aInstance)
{
  NS_IF_ADDREF(aInstance = mInstance);

  return NS_OK;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::GetWindow(NPWindow *&aWindow)
{
  aWindow = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::GetMode(PRInt32 *aMode)
{
  
  *aMode = NP_EMBED;

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::CreateWidget(void)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::GetURL(const char *aURL, const char *aTarget,
                               void *aPostData, PRUint32 aPostDataLen,
                               void *aHeadersData, PRUint32 aHeadersDataLen,
                               PRBool isFile)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::ShowStatus(const char *aStatusMsg)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::ShowStatus(const PRUnichar *aStatusMsg)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NPError
nsDummyJavaPluginOwner::ShowNativeContextMenu(NPMenu* menu, void* event)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NPBool
nsDummyJavaPluginOwner::ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                                     double *destX, double *destY, NPCoordinateSpace destSpace)
{
  return PR_FALSE;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::GetDocument(nsIDocument **aDocument)
{
  NS_IF_ADDREF(*aDocument = mDocument);

  return NS_OK;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::InvalidateRect(NPRect *invalidRect)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::InvalidateRegion(NPRegion invalidRegion)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::ForceRedraw()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDummyJavaPluginOwner::GetNetscapeWindow(void *value)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}





class nsGlobalWindowObserver : public nsIObserver {
public:
  nsGlobalWindowObserver(nsGlobalWindow* aWindow) : mWindow(aWindow) {}
  NS_DECL_ISUPPORTS
  NS_IMETHOD Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
  {
    if (!mWindow)
      return NS_OK;
    return mWindow->Observe(aSubject, aTopic, aData);
  }
  void Forget() { mWindow = nsnull; }
private:
  nsGlobalWindow* mWindow;
};

NS_IMPL_ISUPPORTS1(nsGlobalWindowObserver, nsIObserver)

nsTimeout::nsTimeout()
{
#ifdef DEBUG_jst
  {
    extern int gTimeoutCnt;

    ++gTimeoutCnt;
  }
#endif

  memset(this, 0, sizeof(*this));

  MOZ_COUNT_CTOR(nsTimeout);
}

nsTimeout::~nsTimeout()
{
#ifdef DEBUG_jst
  {
    extern int gTimeoutCnt;

    --gTimeoutCnt;
  }
#endif

  MOZ_COUNT_DTOR(nsTimeout);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsTimeout)
NS_IMPL_CYCLE_COLLECTION_UNLINK_NATIVE_0(nsTimeout)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(nsTimeout)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mWindow,
                                                       nsIScriptGlobalObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPrincipal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptHandler)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsTimeout, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsTimeout, Release)

  




nsGlobalWindow::nsGlobalWindow(nsGlobalWindow *aOuterWindow)
  : nsPIDOMWindow(aOuterWindow),
    mIsFrozen(PR_FALSE),
    mDidInitJavaProperties(PR_FALSE),
    mFullScreen(PR_FALSE),
    mIsClosed(PR_FALSE), 
    mInClose(PR_FALSE), 
    mHavePendingClose(PR_FALSE),
    mHadOriginalOpener(PR_FALSE),
    mIsPopupSpam(PR_FALSE),
    mBlockScriptedClosingFlag(PR_FALSE),
    mFireOfflineStatusChangeEventOnThaw(PR_FALSE),
    mCreatingInnerWindow(PR_FALSE),
    mIsChrome(PR_FALSE),
    mNeedsFocus(PR_TRUE),
    mHasFocus(PR_FALSE),
    mHasAcceleration(PR_FALSE),
    mTimeoutInsertionPoint(nsnull),
    mTimeoutPublicIdCounter(1),
    mTimeoutFiringDepth(0),
    mJSObject(nsnull),
    mPendingStorageEvents(nsnull),
    mTimeoutsSuspendDepth(0),
    mFocusMethod(0)
#ifdef DEBUG
    , mSetOpenerWindowCalled(PR_FALSE)
#endif
{
  memset(mScriptGlobals, 0, sizeof(mScriptGlobals));
  nsLayoutStatics::AddRef();

  
  PR_INIT_CLIST(this);

  
  PR_INIT_CLIST(&mTimeouts);

  if (aOuterWindow) {
    
    
    PR_INSERT_AFTER(this, aOuterWindow);

    mObserver = new nsGlobalWindowObserver(this);
    if (mObserver) {
      NS_ADDREF(mObserver);
      nsCOMPtr<nsIObserverService> os =
        do_GetService("@mozilla.org/observer-service;1");
      if (os) {
        
        
        os->AddObserver(mObserver, NS_IOSERVICE_OFFLINE_STATUS_TOPIC,
                        PR_FALSE);

        
        
        os->AddObserver(mObserver, "dom-storage-changed", PR_FALSE);
      }
    }
  } else {
    
    
    
    Freeze();

    mObserver = nsnull;
  }

  
  
  

  gRefCnt++;

#if !(defined(NS_DEBUG) || defined(MOZ_ENABLE_JS_DUMP))
  if (gRefCnt == 1) {
    static const char* prefName = "browser.dom.window.dump.enabled";
    nsContentUtils::AddBoolPrefVarCache(prefName, &gDOMWindowDumpEnabled);
    gDOMWindowDumpEnabled = nsContentUtils::GetBoolPref(prefName);
  }
#endif

  if (gDumpFile == nsnull) {
    const nsAdoptingCString& fname = 
      nsContentUtils::GetCharPref("browser.dom.window.dump.file");
    if (!fname.IsEmpty()) {
      
      
      gDumpFile = fopen(fname, "wb+");
    } else {
      gDumpFile = stdout;
    }
  }

  if (!gEntropyCollector) {
    CallGetService(NS_ENTROPYCOLLECTOR_CONTRACTID, &gEntropyCollector);
  }

#ifdef DEBUG
  printf("++DOMWINDOW == %d (%p) [serial = %d] [outer = %p]\n", gRefCnt,
         static_cast<void*>(static_cast<nsIScriptGlobalObject*>(this)),
         ++gSerialCounter, static_cast<void*>(aOuterWindow));
  mSerial = gSerialCounter;
#endif

#ifdef PR_LOGGING
  if (!gDOMLeakPRLog)
    gDOMLeakPRLog = PR_NewLogModule("DOMLeak");

  if (gDOMLeakPRLog)
    PR_LOG(gDOMLeakPRLog, PR_LOG_DEBUG,
           ("DOMWINDOW %p created outer=%p", this, aOuterWindow));
#endif
}

nsGlobalWindow::~nsGlobalWindow()
{
  if (!--gRefCnt) {
    NS_IF_RELEASE(gEntropyCollector);
  }
#ifdef DEBUG
  nsCAutoString url;
  if (mLastOpenedURI) {
    mLastOpenedURI->GetSpec(url);
  }

  printf("--DOMWINDOW == %d (%p) [serial = %d] [outer = %p] [url = %s]\n",
         gRefCnt, static_cast<void*>(static_cast<nsIScriptGlobalObject*>(this)),
         mSerial, static_cast<void*>(mOuterWindow), url.get());
#endif

#ifdef PR_LOGGING
  if (gDOMLeakPRLog)
    PR_LOG(gDOMLeakPRLog, PR_LOG_DEBUG,
           ("DOMWINDOW %p destroyed", this));
#endif

  if (mObserver) {
    nsCOMPtr<nsIObserverService> os =
      do_GetService("@mozilla.org/observer-service;1");
    if (os) {
      os->RemoveObserver(mObserver, NS_IOSERVICE_OFFLINE_STATUS_TOPIC);
      os->RemoveObserver(mObserver, "dom-storage-changed");
    }

    
    
    mObserver->Forget();
    NS_RELEASE(mObserver);
  }

  if (IsOuterWindow()) {
    
    
    
    

    nsGlobalWindow *w;
    while ((w = (nsGlobalWindow *)PR_LIST_HEAD(this)) != this) {
      NS_ASSERTION(w->mOuterWindow == this, "Uh, bad outer window pointer?");

      w->mOuterWindow = nsnull;

      PR_REMOVE_AND_INIT_LINK(w);
    }
  } else {
    if (mListenerManager) {
      mListenerManager->Disconnect();
      mListenerManager = nsnull;
    }

    
    

    PR_REMOVE_LINK(this);

    
    
    nsGlobalWindow *outer = GetOuterWindowInternal();
    if (outer && outer->mInnerWindow == this) {
      outer->mInnerWindow = nsnull;
    }
  }

  mDocument = nsnull;           
  mDoc = nsnull;

  NS_ASSERTION(!mArguments, "mArguments wasn't cleaned up properly!");

  CleanUp();

#ifdef DEBUG
  nsCycleCollector_DEBUG_wasFreed(static_cast<nsIScriptGlobalObject*>(this));
#endif

  delete mPendingStorageEvents;

  nsLayoutStatics::Release();
}


void
nsGlobalWindow::ShutDown()
{
  NS_IF_RELEASE(sGlobalStorageList);

  if (gDumpFile && gDumpFile != stdout) {
    fclose(gDumpFile);
  }
  gDumpFile = nsnull;
}


void
nsGlobalWindow::CleanupCachedXBLHandlers(nsGlobalWindow* aWindow)
{
  if (aWindow->mCachedXBLPrototypeHandlers.IsInitialized() &&
      aWindow->mCachedXBLPrototypeHandlers.Count() > 0) {
    aWindow->mCachedXBLPrototypeHandlers.Clear();

    nsCOMPtr<nsISupports> supports;
    aWindow->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                            getter_AddRefs(supports));
    NS_ASSERTION(supports, "Failed to QI to nsCycleCollectionISupports?!");

    nsContentUtils::DropJSObjects(supports);
  }
}

void
nsGlobalWindow::CleanUp()
{
  mNavigator = nsnull;
  mScreen = nsnull;
  mHistory = nsnull;
  mMenubar = nsnull;
  mToolbar = nsnull;
  mLocationbar = nsnull;
  mPersonalbar = nsnull;
  mStatusbar = nsnull;
  mScrollbars = nsnull;
  mLocation = nsnull;
  mFrames = nsnull;
  mApplicationCache = nsnull;

  ClearControllers();

  mOpener = nsnull;             
  if (mContext) {
#ifdef DEBUG
    nsCycleCollector_DEBUG_shouldBeFreed(mContext);
#endif
    mContext = nsnull;            
  }
  mChromeEventHandler = nsnull; 

  if (IsOuterWindow() && IsPopupSpamWindow()) {
    SetPopupSpamWindow(PR_FALSE);
    --gOpenPopupSpamCount;
  }

  nsGlobalWindow *inner = GetCurrentInnerWindowInternal();

  if (inner) {
    inner->CleanUp();
  }

  if (mHasAcceleration) {
    nsCOMPtr<nsIAccelerometer> ac = do_GetService(NS_ACCELEROMETER_CONTRACTID);
    if (ac)
      ac->RemoveWindowListener(this);
  }

  PRUint32 scriptIndex;
  NS_STID_FOR_INDEX(scriptIndex) {
    mInnerWindowHolders[scriptIndex] = nsnull;
  }
  mArguments = nsnull;
  mArgumentsLast = nsnull;
  mArgumentsOrigin = nsnull;

  CleanupCachedXBLHandlers(this);

#ifdef DEBUG
  nsCycleCollector_DEBUG_shouldBeFreed(static_cast<nsIScriptGlobalObject*>(this));
#endif
}

void
nsGlobalWindow::ClearControllers()
{
  if (mControllers) {
    PRUint32 count;
    mControllers->GetControllerCount(&count);

    while (count--) {
      nsCOMPtr<nsIController> controller;
      mControllers->GetControllerAt(count, getter_AddRefs(controller));

      nsCOMPtr<nsIControllerContext> context = do_QueryInterface(controller);
      if (context)
        context->SetCommandContext(nsnull);
    }

    mControllers = nsnull;
  }
}

void
nsGlobalWindow::FreeInnerObjects(PRBool aClearScope)
{
  NS_ASSERTION(IsInnerWindow(), "Don't free inner objects on an outer window");

  
  nsDOMThreadService* dts = nsDOMThreadService::get();
  if (dts) {
    nsIScriptContext *scx = GetContextInternal();

    JSContext *cx = scx ? (JSContext *)scx->GetNativeContext() : nsnull;

    
    
    JSAutoSuspendRequest asr(cx);

    dts->CancelWorkersForGlobal(static_cast<nsIScriptGlobalObject*>(this));
  }

  ClearAllTimeouts();

  mChromeEventHandler = nsnull;

  if (mListenerManager) {
    mListenerManager->Disconnect();
    mListenerManager = nsnull;
  }

  if (mDocument) {
    NS_ASSERTION(mDoc, "Why is mDoc null?");

    
    mDocumentPrincipal = mDoc->NodePrincipal();
  }

#ifdef DEBUG
  if (mDocument)
    nsCycleCollector_DEBUG_shouldBeFreed(nsCOMPtr<nsISupports>(do_QueryInterface(mDocument)));
#endif

  
  NotifyDOMWindowDestroyed(this);

  
  mDocument = nsnull;
  mDoc = nsnull;

  if (mApplicationCache) {
    static_cast<nsDOMOfflineResourceList*>(mApplicationCache.get())->Disconnect();
    mApplicationCache = nsnull;
  }

  if (aClearScope) {
    PRUint32 lang_id;
    NS_STID_FOR_ID(lang_id) {
      
      
      nsIScriptContext *scx = GetScriptContextInternal(lang_id);
      if (scx)
        scx->ClearScope(mScriptGlobals[NS_STID_INDEX(lang_id)], PR_TRUE);
    }
  }

  if (mDummyJavaPluginOwner) {
    

    
    

    mDummyJavaPluginOwner->Destroy();

    mDummyJavaPluginOwner = nsnull;
  }

  CleanupCachedXBLHandlers(this);

#ifdef DEBUG
  nsCycleCollector_DEBUG_shouldBeFreed(static_cast<nsIScriptGlobalObject*>(this));
#endif
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsGlobalWindow)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsGlobalWindow)
  
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWindowInternal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWindow)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWindow2)
  NS_INTERFACE_MAP_ENTRY(nsIDOMJSWindow)
  NS_INTERFACE_MAP_ENTRY(nsIScriptGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOM3EventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMWindow)
  NS_INTERFACE_MAP_ENTRY(nsIDOMViewCSS)
  NS_INTERFACE_MAP_ENTRY(nsIDOMAbstractView)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageWindow)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Window)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsGlobalWindow, nsIScriptGlobalObject)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsGlobalWindow,
                                           nsIScriptGlobalObject)


NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsGlobalWindow)
  if (tmp->mDoc && nsCCUncollectableMarker::InGeneration(
                     cb, tmp->mDoc->GetMarkedCCGeneration())) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContext)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mControllers)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mArguments)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mArgumentsLast)

  for (PRUint32 i = 0; i < NS_STID_ARRAY_UBOUND; ++i) {      
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptContexts[i])
  }

  for (PRUint32 i = 0; i < NS_STID_ARRAY_UBOUND; ++i) {      
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mInnerWindowHolders[i])
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOpenerScriptPrincipal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mListenerManager)

  for (nsTimeout* timeout = tmp->FirstTimeout();
       tmp->IsTimeout(timeout);
       timeout = timeout->Next()) {
    cb.NoteNativeChild(timeout, &NS_CYCLE_COLLECTION_NAME(nsTimeout));
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSessionStorage)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mApplicationCache)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocumentPrincipal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDoc)

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChromeEventHandler)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFrameElement)

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDummyJavaPluginOwner)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFocusedNode)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsGlobalWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContext)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mControllers)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mArguments)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mArgumentsLast)

  for (PRUint32 i = 0; i < NS_STID_ARRAY_UBOUND; ++i) {      
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mScriptContexts[i])
  }

  for (PRUint32 i = 0; i < NS_STID_ARRAY_UBOUND; ++i) {      
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mInnerWindowHolders[i])
  }

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOpenerScriptPrincipal)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mListenerManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSessionStorage)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mApplicationCache)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocumentPrincipal)

  
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChromeEventHandler)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFrameElement)

  
  if (tmp->mDummyJavaPluginOwner) {
    tmp->mDummyJavaPluginOwner->Destroy();
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDummyJavaPluginOwner)
  }

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFocusedNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_END

struct TraceData
{
  TraceData(TraceCallback& aCallback, void* aClosure) :
    callback(aCallback), closure(aClosure) {}

  TraceCallback& callback;
  void* closure;
};

static PLDHashOperator
TraceXBLHandlers(const void* aKey, void* aData, void* aClosure)
{
  TraceData* data = static_cast<TraceData*>(aClosure);
  data->callback(nsIProgrammingLanguage::JAVASCRIPT, aData, data->closure);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsGlobalWindow)
  if (tmp->mCachedXBLPrototypeHandlers.IsInitialized()) {
    TraceData data(aCallback, aClosure);
    tmp->mCachedXBLPrototypeHandlers.EnumerateRead(TraceXBLHandlers, &data);
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN(nsGlobalWindow)
  nsGlobalWindow::CleanupCachedXBLHandlers(tmp);
NS_IMPL_CYCLE_COLLECTION_ROOT_END





nsresult
nsGlobalWindow::SetScriptContext(PRUint32 lang_id, nsIScriptContext *aScriptContext)
{
  nsresult rv;
  
  PRBool ok = NS_STID_VALID(lang_id);
  NS_ASSERTION(ok, "Invalid programming language ID requested");
  NS_ENSURE_TRUE(ok, NS_ERROR_INVALID_ARG);
  PRUint32 lang_ndx = NS_STID_INDEX(lang_id);

  NS_ASSERTION(IsOuterWindow(), "Uh, SetScriptContext() called on inner window!");

  if (!aScriptContext)
    NS_WARNING("Possibly early removal of script object, see bug #41608");
  else {
    
    aScriptContext->WillInitializeContext();
    
    rv = aScriptContext->InitContext(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(!aScriptContext || !mScriptContexts[lang_ndx],
               "Bad call to SetContext()!");

  void *script_glob = nsnull;

  if (aScriptContext) {
    if (IsFrame()) {
      
      
      

      aScriptContext->SetGCOnDestruction(PR_FALSE);
    }

    aScriptContext->DidInitializeContext();
    script_glob = aScriptContext->GetNativeGlobal();
    NS_ASSERTION(script_glob, "GetNativeGlobal returned NULL!");
  }
  
  if (lang_id == nsIProgrammingLanguage::JAVASCRIPT) {
    mContext = aScriptContext;
    mJSObject = (JSObject *)script_glob;
  }
  mScriptContexts[lang_ndx] = aScriptContext;
  mScriptGlobals[lang_ndx] = script_glob;
  return NS_OK;
}

nsresult
nsGlobalWindow::EnsureScriptEnvironment(PRUint32 aLangID)
{
  FORWARD_TO_OUTER(EnsureScriptEnvironment, (aLangID), NS_ERROR_NOT_INITIALIZED);
  nsresult rv;
 
  PRBool ok = NS_STID_VALID(aLangID);
  NS_ASSERTION(ok, "Invalid programming language ID requested");
  NS_ENSURE_TRUE(ok, NS_ERROR_INVALID_ARG);
  PRUint32 lang_ndx = NS_STID_INDEX(aLangID);

  if (mScriptGlobals[lang_ndx])
      return NS_OK; 
  nsCOMPtr<nsIScriptRuntime> scriptRuntime;
  rv = NS_GetScriptRuntimeByID(aLangID, getter_AddRefs(scriptRuntime));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIScriptContext> context;
  rv = scriptRuntime->CreateContext(getter_AddRefs(context));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetScriptContext(aLangID, context);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  nsGlobalWindow *currentInner = GetCurrentInnerWindowInternal();
  if (currentInner) {
    
    
    NS_ASSERTION(!mInnerWindowHolders[lang_ndx], "already have a holder?");
    nsCOMPtr<nsISupports> &holder = mInnerWindowHolders[lang_ndx];
    PRBool isChrome = PR_FALSE; 
    void *&innerGlob = currentInner->mScriptGlobals[lang_ndx];
    rv = context->CreateNativeGlobalForInner(this, isChrome,
                                             &innerGlob,
                                             getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ASSERTION(innerGlob && holder, "Failed to get global and holder");
    rv = context->ConnectToInner(currentInner, mScriptGlobals[lang_ndx]);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDOMDocument> doc(do_QueryInterface(mDocument));
    if (doc)
      context->DidSetDocument(doc, innerGlob);

    if (mArgumentsLast != nsnull) {
      context->SetProperty(innerGlob, "arguments", mArgumentsLast);
    }
  }
  return NS_OK;
}

nsIScriptContext *
nsGlobalWindow::GetScriptContext(PRUint32 lang)
{
  FORWARD_TO_OUTER(GetScriptContext, (lang), nsnull);

  PRBool ok = NS_STID_VALID(lang);
  NS_ASSERTION(ok, "Invalid programming language ID requested");
  NS_ENSURE_TRUE(ok, nsnull);

  PRUint32 lang_ndx = NS_STID_INDEX(lang);

  
  
  
  NS_ASSERTION(mScriptContexts[NS_STID_INDEX(nsIProgrammingLanguage::JAVASCRIPT)] == mContext &&
               mScriptGlobals[NS_STID_INDEX(nsIProgrammingLanguage::JAVASCRIPT)] == mJSObject,
               "JS language contexts are confused");
  return mScriptContexts[lang_ndx];
}

void *
nsGlobalWindow::GetScriptGlobal(PRUint32 lang)
{
  PRBool ok = NS_STID_VALID(lang);
  NS_ASSERTION(ok, "Invalid programming language ID requested");
  NS_ENSURE_TRUE(ok, nsnull);

  PRUint32 lang_ndx = NS_STID_INDEX(lang);

  
  
  
  NS_ASSERTION(mScriptContexts[NS_STID_INDEX(nsIProgrammingLanguage::JAVASCRIPT)] == mContext &&
               mScriptGlobals[NS_STID_INDEX(nsIProgrammingLanguage::JAVASCRIPT)] == mJSObject,
               "JS language contexts are confused");
  return mScriptGlobals[lang_ndx];
}

nsIScriptContext *
nsGlobalWindow::GetContext()
{
  FORWARD_TO_OUTER(GetContext, (), nsnull);

  
  NS_ASSERTION(mContext == GetScriptContext(nsIProgrammingLanguage::JAVASCRIPT),
               "GetContext confused?");
  return mContext;
}

JSObject *
nsGlobalWindow::GetGlobalJSObject()
{
  NS_ASSERTION(mJSObject == GetScriptGlobal(nsIProgrammingLanguage::JAVASCRIPT),
               "GetGlobalJSObject confused?");
  return mJSObject;
}


PRBool
nsGlobalWindow::WouldReuseInnerWindow(nsIDocument *aNewDocument)
{
  
  
  
  
  
  
  
  

  if (!mDoc || !aNewDocument) {
    return PR_FALSE;
  }

  if (!mDoc->IsInitialDocument()) {
    return PR_FALSE;
  }
  
  NS_ASSERTION(IsAboutBlank(mDoc->GetDocumentURI()),
               "How'd this happen?");
  
  
  
  if (mDoc == aNewDocument) {
    
    return PR_TRUE;
  }

  PRBool equal;
  if (NS_SUCCEEDED(mDoc->NodePrincipal()->Equals(aNewDocument->NodePrincipal(),
                                                 &equal)) &&
      equal) {
    
    return PR_TRUE;
  }

  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(mDocShell));

  if (treeItem) {
    PRInt32 itemType = nsIDocShellTreeItem::typeContent;
    treeItem->GetItemType(&itemType);

    
    return itemType == nsIDocShellTreeItem::typeChrome;
  }

  
  return PR_FALSE;
}

already_AddRefed<nsComputedDOMStyle>
nsGlobalWindow::LookupComputedStyleFor(nsIContent* aElem)
{
  nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(aElem));
  nsRefPtr<nsComputedDOMStyle> computedDOMStyle;
  GetComputedStyle(domElement, EmptyString(),
                   getter_AddRefs(computedDOMStyle));
  return computedDOMStyle.forget();
}

void
nsGlobalWindow::SetOpenerScriptPrincipal(nsIPrincipal* aPrincipal)
{
  FORWARD_TO_OUTER_VOID(SetOpenerScriptPrincipal, (aPrincipal));

  if (mDoc) {
    if (!mDoc->IsInitialDocument()) {
      
      
      return;
    }
    
#ifdef DEBUG
    
    
    nsCOMPtr<nsIURI> uri;
    mDoc->NodePrincipal()->GetURI(getter_AddRefs(uri));
    NS_ASSERTION(uri && IsAboutBlank(uri) &&
                 IsAboutBlank(mDoc->GetDocumentURI()),
                 "Unexpected original document");
#endif
    
    
    
    mDoc->SetPrincipal(aPrincipal);
  }
    
  mOpenerScriptPrincipal = aPrincipal;
}

nsIPrincipal*
nsGlobalWindow::GetOpenerScriptPrincipal()
{
  FORWARD_TO_OUTER(GetOpenerScriptPrincipal, (), nsnull);

  return mOpenerScriptPrincipal;
}

PopupControlState
PushPopupControlState(PopupControlState aState, PRBool aForce)
{
  PopupControlState oldState = gPopupControlState;

  if (aState < gPopupControlState || aForce) {
    gPopupControlState = aState;
  }

  return oldState;
}

void
PopPopupControlState(PopupControlState aState)
{
  gPopupControlState = aState;
}

PopupControlState
nsGlobalWindow::PushPopupControlState(PopupControlState aState,
                                      PRBool aForce) const
{
  return ::PushPopupControlState(aState, aForce);
}

void
nsGlobalWindow::PopPopupControlState(PopupControlState aState) const
{
  ::PopPopupControlState(aState);
}

PopupControlState
nsGlobalWindow::GetPopupControlState() const
{
  return gPopupControlState;
}

#define WINDOWSTATEHOLDER_IID \
{0x0b917c3e, 0xbd50, 0x4683, {0xaf, 0xc9, 0xc7, 0x81, 0x07, 0xae, 0x33, 0x26}}

class WindowStateHolder : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(WINDOWSTATEHOLDER_IID)
  NS_DECL_ISUPPORTS

  WindowStateHolder(nsGlobalWindow *aWindow,
                    nsCOMPtr<nsISupports> aHolders[],
                    nsNavigator *aNavigator,
                    nsLocation *aLocation,
                    nsIXPConnectJSObjectHolder *aOuterProto);

  nsGlobalWindow* GetInnerWindow() { return mInnerWindow; }
  nsISupports* GetInnerWindowHolder(PRUint32 aScriptTypeID)
  { return mInnerWindowHolders[NS_STID_INDEX(aScriptTypeID)]; }

  nsNavigator* GetNavigator() { return mNavigator; }
  nsLocation* GetLocation() { return mLocation; }
  nsIXPConnectJSObjectHolder* GetOuterProto() { return mOuterProto; }

  void DidRestoreWindow()
  {
    PRUint32 lang_ndx;
    mInnerWindow = nsnull;

    NS_STID_FOR_INDEX(lang_ndx) {
        mInnerWindowHolders[lang_ndx] = nsnull;
    }
    mNavigator = nsnull;
    mLocation = nsnull;
    mOuterProto = nsnull;
  }

protected:
  ~WindowStateHolder();

  nsGlobalWindow *mInnerWindow;
  
  
  nsCOMPtr<nsISupports> mInnerWindowHolders[NS_STID_ARRAY_UBOUND];
  nsRefPtr<nsNavigator> mNavigator;
  nsRefPtr<nsLocation> mLocation;
  nsCOMPtr<nsIXPConnectJSObjectHolder> mOuterProto;
};

NS_DEFINE_STATIC_IID_ACCESSOR(WindowStateHolder, WINDOWSTATEHOLDER_IID)

WindowStateHolder::WindowStateHolder(nsGlobalWindow *aWindow,
                                     nsCOMPtr<nsISupports> aHolders[],
                                     nsNavigator *aNavigator,
                                     nsLocation *aLocation,
                                     nsIXPConnectJSObjectHolder *aOuterProto)
  : mInnerWindow(aWindow),
    mNavigator(aNavigator),
    mLocation(aLocation),
    mOuterProto(aOuterProto)
{
  NS_PRECONDITION(aWindow, "null window");
  NS_PRECONDITION(aWindow->IsInnerWindow(), "Saving an outer window");

  PRUint32 lang_ndx;
  NS_STID_FOR_INDEX(lang_ndx) {
    mInnerWindowHolders[lang_ndx] = aHolders[lang_ndx];
  }

  aWindow->SuspendTimeouts();
}

WindowStateHolder::~WindowStateHolder()
{
  if (mInnerWindow) {
    
    
    
    
    
    
    
    mInnerWindow->FreeInnerObjects(PR_TRUE);
  }
}

NS_IMPL_ISUPPORTS1(WindowStateHolder, WindowStateHolder)

nsresult
nsGlobalWindow::SetNewDocument(nsIDocument* aDocument,
                               nsISupports* aState,
                               PRBool aClearScopeHint)
{
  return SetNewDocument(aDocument, aState, aClearScopeHint, PR_FALSE);
}

nsresult
nsGlobalWindow::SetNewDocument(nsIDocument* aDocument,
                               nsISupports* aState,
                               PRBool aClearScopeHint,
                               PRBool aIsInternalCall)
{
  NS_ASSERTION(mDocumentPrincipal == nsnull,
               "mDocumentPrincipal prematurely set!");
#ifdef PR_LOGGING
  if (IsInnerWindow() && aDocument && gDOMLeakPRLog &&
      PR_LOG_TEST(gDOMLeakPRLog, PR_LOG_DEBUG)) {
    nsIURI *uri = aDocument->GetDocumentURI();
    nsCAutoString spec;
    if (uri)
      uri->GetSpec(spec);
    PR_LogPrint("DOMWINDOW %p SetNewDocument %s", this, spec.get());
  }
#endif

  if (IsOuterWindow() && IsFrozen()) {
    
    

    Thaw();
  }

  if (!aIsInternalCall && IsInnerWindow()) {
    if (!mOuterWindow) {
      return NS_ERROR_NOT_INITIALIZED;
    }

    
    
    if (mOuterWindow->GetCurrentInnerWindow() != this) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    return GetOuterWindowInternal()->SetNewDocument(aDocument,
                                                    aState,
                                                    aClearScopeHint, PR_TRUE);
  }

  if (!aDocument) {
    NS_ERROR("SetNewDocument(null) called!");

    return NS_ERROR_INVALID_ARG;
  }

  NS_ASSERTION(!GetCurrentInnerWindow() ||
               GetCurrentInnerWindow()->GetExtantDocument() == mDocument,
               "Uh, mDocument doesn't match the current inner window "
               "document!");

  nsresult rv = NS_OK;

  nsCOMPtr<nsIDocument> oldDoc(do_QueryInterface(mDocument));

  nsIScriptContext *scx = GetContextInternal();
  NS_ENSURE_TRUE(scx, NS_ERROR_NOT_INITIALIZED);

  JSContext *cx = (JSContext *)scx->GetNativeContext();

  
  if (mCrypto) {
    mCrypto->SetEnableSmartCardEvents(PR_FALSE);
  }

  if (!mDocument) {
    

    
    
    
    nsIDOMWindowInternal *internal = nsGlobalWindow::GetPrivateRoot();

    if (internal == static_cast<nsIDOMWindowInternal *>(this)) {
      nsCOMPtr<nsIXBLService> xblService = do_GetService("@mozilla.org/xbl;1");
      if (xblService) {
        nsCOMPtr<nsPIDOMEventTarget> piTarget =
          do_QueryInterface(mChromeEventHandler);
        xblService->AttachGlobalKeyHandler(piTarget);
      }
    }
  }

  




  SetStatus(EmptyString());
  SetDefaultStatus(EmptyString());

  
  
  
  nsIXPConnect *xpc = nsContentUtils::XPConnect();

  PRBool reUseInnerWindow = WouldReuseInnerWindow(aDocument);

  
  nsIPrincipal *oldPrincipal = nsnull;

  if (oldDoc) {
    oldPrincipal = oldDoc->NodePrincipal();
  }

  
  
  
  if (!reUseInnerWindow && mNavigator && oldPrincipal) {
    PRBool equal;
    rv = oldPrincipal->Equals(aDocument->NodePrincipal(), &equal);

    if (NS_FAILED(rv) || !equal) {
      
      
      
      mNavigator->SetDocShell(nsnull);

      mNavigator = nsnull;
    }
  }

  if (mNavigator && aDocument != oldDoc) {
    
    
    
    

    mNavigator->LoadingNewDocument();
  }

  PRUint32 st_id, st_ndx; 

  
  
  

  mDocument = do_QueryInterface(aDocument);
  mDoc = aDocument;
  mLocalStorage = nsnull;

#ifdef DEBUG
  mLastOpenedURI = aDocument->GetDocumentURI();
#endif

  if (IsOuterWindow()) {
    NS_STID_FOR_ID(st_id) {
      nsIScriptContext *langContext = GetScriptContextInternal(st_id);
      if (langContext)
          langContext->WillInitializeContext();
    }

    nsGlobalWindow *currentInner = GetCurrentInnerWindowInternal();

    nsRefPtr<nsGlobalWindow> newInnerWindow;

    nsCOMPtr<nsIDOMChromeWindow> thisChrome =
      do_QueryInterface(static_cast<nsIDOMWindow *>(this));
    nsCOMPtr<nsIXPConnectJSObjectHolder> navigatorHolder;
    jsval nav;

    PRBool isChrome = PR_FALSE;

    nsCxPusher cxPusher;
    if (!cxPusher.Push(cx)) {
      return NS_ERROR_FAILURE;
    }

    JSAutoRequest ar(cx);

    
    
    
    
    NS_STID_FOR_ID(st_id) {
      nsIScriptContext *langContext = GetScriptContextInternal(st_id);
      if (langContext)
        langContext->ClearScope(mScriptGlobals[NS_STID_INDEX(st_id)],
                                PR_FALSE);
    }

    if (reUseInnerWindow) {
      
      NS_ASSERTION(!currentInner->IsFrozen(),
                   "We should never be reusing a shared inner window");
      newInnerWindow = currentInner;

      if (aDocument != oldDoc) {
        nsWindowSH::InvalidateGlobalScopePolluter(cx, currentInner->mJSObject);
      }
    } else {
      if (aState) {
        nsCOMPtr<WindowStateHolder> wsh = do_QueryInterface(aState);
        NS_ASSERTION(wsh, "What kind of weird state are you giving me here?");

        newInnerWindow = wsh->GetInnerWindow();
        NS_STID_FOR_ID(st_id) {
            mInnerWindowHolders[NS_STID_INDEX(st_id)] = wsh->GetInnerWindowHolder(st_id);
        }

        
        mNavigator = wsh->GetNavigator();
        mLocation = wsh->GetLocation();

        if (mNavigator) {
          
          mNavigator->SetDocShell(mDocShell);
          mNavigator->LoadingNewDocument();
        }
      } else {
        if (thisChrome) {
          newInnerWindow = new nsGlobalChromeWindow(this);

          isChrome = PR_TRUE;
        } else {
          if (mIsModalContentWindow) {
            newInnerWindow = new nsGlobalModalWindow(this);
          } else {
            newInnerWindow = new nsGlobalWindow(this);
          }
        }

        mLocation = nsnull;
      }

      if (!newInnerWindow) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      if (currentInner && currentInner->mJSObject) {
        if (mNavigator && !aState) {
          
          
          
          

          nsIDOMNavigator* navigator =
            static_cast<nsIDOMNavigator*>(mNavigator.get());
          nsContentUtils::WrapNative(cx, currentInner->mJSObject, navigator,
                                     &NS_GET_IID(nsIDOMNavigator), &nav,
                                     getter_AddRefs(navigatorHolder));
        }
      }

      if (!aState) {
        
        nsIScriptGlobalObject *sgo =
          (nsIScriptGlobalObject *)newInnerWindow.get();

        
        
        
        
        
        
        
        
        

        mInnerWindow = nsnull;

        Freeze();
        mCreatingInnerWindow = PR_TRUE;
        
        
        rv = NS_OK;
        NS_STID_FOR_ID(st_id) {
          st_ndx = NS_STID_INDEX(st_id);
          nsIScriptContext *this_ctx = GetScriptContextInternal(st_id);
          if (this_ctx) {
            void *&newGlobal = newInnerWindow->mScriptGlobals[st_ndx];
            nsCOMPtr<nsISupports> &holder = mInnerWindowHolders[st_ndx];
            rv |= this_ctx->CreateNativeGlobalForInner(sgo, isChrome,
                                                       &newGlobal,
                                                       getter_AddRefs(holder));
            NS_ASSERTION(NS_SUCCEEDED(rv) && newGlobal && holder, 
                        "Failed to get script global and holder");
            if (st_id == nsIProgrammingLanguage::JAVASCRIPT) {
                newInnerWindow->mJSObject = (JSObject *)newGlobal;
            }
          }
        }
        mCreatingInnerWindow = PR_FALSE;
        Thaw();

        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentInner && currentInner->mJSObject) {
        PRBool termFuncSet = PR_FALSE;

        if (oldDoc == aDocument) {
          
          
          JSAutoSuspendRequest asr(cx);

          
          
          cxPusher.Pop();

          JSContext *oldCx = nsContentUtils::GetCurrentJSContext();

          nsIScriptContext *callerScx;
          if (oldCx && (callerScx = GetScriptContextFromJSContext(oldCx))) {
            
            
            
            
            NS_ASSERTION(!currentInner->IsFrozen(),
                "How does this opened window get into session history");

            JSAutoRequest ar(oldCx);

            callerScx->SetTerminationFunction(ClearWindowScope,
                                              static_cast<nsIDOMWindow *>
                                                         (currentInner));

            termFuncSet = PR_TRUE;
          }

          
          cxPusher.Push(cx);
        }

        
        
        if (!currentInner->IsFrozen()) {
          
          
          currentInner->FreeInnerObjects(!termFuncSet);
        }
      }

      mInnerWindow = newInnerWindow;
    }

    if (!aState && !reUseInnerWindow) {
      
      

      
      
      
      
      
      
      
      
      NS_STID_FOR_ID(st_id) {
        nsIScriptContext *this_ctx = GetScriptContextInternal(st_id);
        if (st_id == nsIProgrammingLanguage::JAVASCRIPT)
            JS_BeginRequest((JSContext *)this_ctx->GetNativeContext());
        if (this_ctx) {
          this_ctx->InitContext(this);
          
          
          void *glob = mScriptGlobals[NS_STID_INDEX(st_id)];
          this_ctx->ConnectToInner(newInnerWindow, glob);
        }
        if (st_id == nsIProgrammingLanguage::JAVASCRIPT)
            JS_EndRequest((JSContext *)this_ctx->GetNativeContext());
      }

      nsCOMPtr<nsIContent> frame = do_QueryInterface(GetFrameElementInternal());
      if (frame && frame->GetOwnerDoc()) {
        nsPIDOMWindow* parentWindow = frame->GetOwnerDoc()->GetWindow();
        if (parentWindow && parentWindow->TimeoutSuspendCount()) {
          SuspendTimeouts(parentWindow->TimeoutSuspendCount());
        }
      }
    }
    
    NS_STID_FOR_ID(st_id) {
      
      nsCOMPtr<nsIScriptContext> this_ctx =
                                    GetScriptContextInternal(st_id);
      if (this_ctx) {
        nsCOMPtr<nsIDOMDocument> dd(do_QueryInterface(aDocument));
        this_ctx->DidSetDocument(dd, newInnerWindow->GetScriptGlobal(st_id));
      }
    }

    
    
    
    
    
    
    
    
    
    

    if ((!reUseInnerWindow || aDocument != oldDoc) && !aState) {
      nsCOMPtr<nsIHTMLDocument> html_doc(do_QueryInterface(mDocument));
      nsWindowSH::InstallGlobalScopePolluter(cx, newInnerWindow->mJSObject,
                                             html_doc);
    }

    if (aState) {
      

      nsCOMPtr<WindowStateHolder> wsh = do_QueryInterface(aState);
      NS_ASSERTION(wsh, "What kind of weird state are you giving me here?");

      
      
      nsCOMPtr<nsIClassInfo> ci =
        do_QueryInterface((nsIScriptGlobalObject *)this);

      rv = xpc->RestoreWrappedNativePrototype(cx, mJSObject, ci,
                                              wsh->GetOuterProto());
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      
      

      nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
      rv = xpc->GetWrappedNativeOfJSObject(cx, mJSObject,
                                           getter_AddRefs(wrapper));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = wrapper->RefreshPrototype();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (aDocument) {
      aDocument->SetScriptGlobalObject(newInnerWindow);
    }

    if (!aState) {
      if (reUseInnerWindow) {
        if (newInnerWindow->mDoc != aDocument) {
          newInnerWindow->mDocument = do_QueryInterface(aDocument);
          newInnerWindow->mDoc = aDocument;

          
          
          

          
          JSAutoRequest ar(cx);
          ::JS_DeleteProperty(cx, currentInner->mJSObject, "document");
        }
      } else {
        rv = newInnerWindow->SetNewDocument(aDocument, nsnull,
                                            aClearScopeHint, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);

        NS_STID_FOR_ID(st_id) {
          nsIScriptContext *this_ctx = GetScriptContextInternal(st_id);
          if (this_ctx) {
            
            void *glob = newInnerWindow->mScriptGlobals[NS_STID_INDEX(st_id)];
            rv = this_ctx->InitClasses(glob);
            NS_ENSURE_SUCCESS(rv, rv);

            if (navigatorHolder &&
                st_id == nsIProgrammingLanguage::JAVASCRIPT) {
              
              JSAutoRequest ar(cx);

              ::JS_DefineProperty(cx, newInnerWindow->mJSObject, "navigator",
                                  nav, nsnull, nsnull,
                                  JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                  JSPROP_READONLY);

              
              
              
              
              nsIDOMNavigator* navigator =
                static_cast<nsIDOMNavigator*>(mNavigator);

              xpc->
                ReparentWrappedNativeIfFound(cx, JSVAL_TO_OBJECT(nav),
                                             newInnerWindow->mJSObject,
                                             navigator,
                                             getter_AddRefs(navigatorHolder));
            }
          }
        }
      }

      if (mArguments) {
        newInnerWindow->DefineArgumentsProperty(mArguments);
        newInnerWindow->mArguments = mArguments;
        newInnerWindow->mArgumentsOrigin = mArgumentsOrigin;

        mArguments = nsnull;
        mArgumentsOrigin = nsnull;
      }

      
      
      newInnerWindow->mChromeEventHandler = mChromeEventHandler;
    }

    NS_STID_FOR_ID(st_id) {
      
      nsCOMPtr<nsIScriptContext> this_ctx =
                                    GetScriptContextInternal(st_id);
      if (this_ctx) {
        this_ctx->GC();
        this_ctx->DidInitializeContext();
      }
    }
  }

  
  mMutationBits = 0;

  return NS_OK;
}

void
nsGlobalWindow::SetDocShell(nsIDocShell* aDocShell)
{
  NS_ASSERTION(IsOuterWindow(), "Uh, SetDocShell() called on inner window!");

  if (aDocShell == mDocShell)
    return;

  PRUint32 lang_id;
  nsIScriptContext *langCtx;
  
  
  
  
  

  if (!aDocShell) {
    NS_ASSERTION(PR_CLIST_IS_EMPTY(&mTimeouts),
                 "Uh, outer window holds timeouts!");

    
    
    
    for (nsRefPtr<nsGlobalWindow> inner = (nsGlobalWindow *)PR_LIST_HEAD(this);
         inner != this;
         inner = (nsGlobalWindow*)PR_NEXT_LINK(inner)) {
      NS_ASSERTION(inner->mOuterWindow == this, "bad outer window pointer");
      inner->FreeInnerObjects(PR_TRUE);
    }

    
    NotifyDOMWindowDestroyed(this);

    nsGlobalWindow *currentInner = GetCurrentInnerWindowInternal();

    if (currentInner) {
      NS_ASSERTION(mDoc, "Must have doc!");
      
      
      mDocumentPrincipal = mDoc->NodePrincipal();

      
      mDocument = nsnull;
      mDoc = nsnull;
    }

    
    NS_STID_FOR_ID(lang_id) {
      langCtx = mScriptContexts[NS_STID_INDEX(lang_id)];
      if (langCtx)
        langCtx->ClearScope(mScriptGlobals[NS_STID_INDEX(lang_id)], PR_TRUE);
    }

    ClearControllers();

    mChromeEventHandler = nsnull; 

    if (mArguments) { 
      
      
      mArguments = nsnull;
      mArgumentsLast = nsnull;
      mArgumentsOrigin = nsnull;
    }

    PRUint32 st_ndx;

    
    NS_ASSERTION(mContext == mScriptContexts[NS_STID_INDEX(nsIProgrammingLanguage::JAVASCRIPT)],
                 "Contexts confused");
    NS_STID_FOR_INDEX(st_ndx) {
      mInnerWindowHolders[st_ndx] = nsnull;
      langCtx = mScriptContexts[st_ndx];
      if (langCtx) {
        langCtx->GC();
        langCtx->FinalizeContext();
        mScriptContexts[st_ndx] = nsnull;
      }
    }
#ifdef DEBUG
    nsCycleCollector_DEBUG_shouldBeFreed(mContext);
    nsCycleCollector_DEBUG_shouldBeFreed(static_cast<nsIScriptGlobalObject*>(this));
#endif
    mContext = nsnull; 
  }

  mDocShell = aDocShell;        

  if (mNavigator)
    mNavigator->SetDocShell(aDocShell);
  if (mLocation)
    mLocation->SetDocShell(aDocShell);
  if (mHistory)
    mHistory->SetDocShell(aDocShell);
  if (mFrames)
    mFrames->SetDocShell(aDocShell);
  if (mScreen)
    mScreen->SetDocShell(aDocShell);

  if (mDocShell) {
    
    if (mMenubar) {
      nsCOMPtr<nsIWebBrowserChrome> browserChrome;
      GetWebBrowserChrome(getter_AddRefs(browserChrome));
      mMenubar->SetWebBrowserChrome(browserChrome);
    }

    
    
    nsCOMPtr<nsIDOMEventTarget> chromeEventHandler;
    mDocShell->GetChromeEventHandler(getter_AddRefs(chromeEventHandler));
    mChromeEventHandler = do_QueryInterface(chromeEventHandler);
    if (!mChromeEventHandler) {
      
      
      
      
      
      
      nsCOMPtr<nsIDOMWindow> parentWindow;
      GetParent(getter_AddRefs(parentWindow));
      if (parentWindow.get() != static_cast<nsIDOMWindow*>(this)) {
        nsCOMPtr<nsPIDOMWindow> piWindow(do_QueryInterface(parentWindow));
        mChromeEventHandler = piWindow->GetChromeEventHandler();
      }
      else NS_NewWindowRoot(this, getter_AddRefs(mChromeEventHandler));
    }
  }
}

void
nsGlobalWindow::SetOpenerWindow(nsIDOMWindowInternal* aOpener,
                                PRBool aOriginalOpener)
{
  FORWARD_TO_OUTER_VOID(SetOpenerWindow, (aOpener, aOriginalOpener));

  NS_ASSERTION(!aOriginalOpener || !mSetOpenerWindowCalled,
               "aOriginalOpener is true, but not first call to "
               "SetOpenerWindow!");
  NS_ASSERTION(aOpener || !aOriginalOpener,
               "Shouldn't set mHadOriginalOpener if aOpener is null");

  mOpener = do_GetWeakReference(aOpener);
  NS_ASSERTION(mOpener || !aOpener, "Opener must support weak references!");

  if (aOriginalOpener) {
    mHadOriginalOpener = PR_TRUE;
  }

#ifdef DEBUG
  mSetOpenerWindowCalled = PR_TRUE;
#endif
}

nsresult
nsGlobalWindow::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  NS_PRECONDITION(IsInnerWindow(), "PreHandleEvent is used on outer window!?");
  static PRUint32 count = 0;
  PRUint32 msg = aVisitor.mEvent->message;

  aVisitor.mCanHandle = PR_TRUE;
  aVisitor.mForceContentDispatch = PR_TRUE; 
  if ((msg == NS_MOUSE_MOVE) && gEntropyCollector) {
    
    
    
    if (count++ % 100 == 0) {
      
      
      PRInt16 myCoord[2];

      myCoord[0] = aVisitor.mEvent->refPoint.x;
      myCoord[1] = aVisitor.mEvent->refPoint.y;
      gEntropyCollector->RandomUpdate((void*)myCoord, sizeof(myCoord));
      gEntropyCollector->RandomUpdate((void*)&(aVisitor.mEvent->time),
                                      sizeof(PRUint32));
    }
  } else if (msg == NS_RESIZE_EVENT) {
    mIsHandlingResizeEvent = PR_TRUE;
  } else if (msg == NS_MOUSE_BUTTON_DOWN &&
             NS_IS_TRUSTED_EVENT(aVisitor.mEvent)) {
    gMouseDown = PR_TRUE;
  } else if (msg == NS_MOUSE_BUTTON_UP &&
             NS_IS_TRUSTED_EVENT(aVisitor.mEvent)) {
    gMouseDown = PR_FALSE;
    if (gDragServiceDisabled) {
      nsCOMPtr<nsIDragService> ds =
        do_GetService("@mozilla.org/widget/dragservice;1");
      if (ds) {
        gDragServiceDisabled = PR_FALSE;
        ds->Unsuppress();
      }
    }
  }

  aVisitor.mParentTarget = mChromeEventHandler;
  return NS_OK;
}

nsresult
nsGlobalWindow::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  NS_PRECONDITION(IsInnerWindow(), "PostHandleEvent is used on outer window!?");

  
  switch (aVisitor.mEvent->message) {
    case NS_RESIZE_EVENT:
    case NS_PAGE_UNLOAD:
    case NS_LOAD:
      break;
    default:
      return NS_OK;
  }

  


  nsCOMPtr<nsPIDOMEventTarget> kungFuDeathGrip1(mChromeEventHandler);
  nsCOMPtr<nsIScriptContext> kungFuDeathGrip2(GetContextInternal());

  if (aVisitor.mEvent->message == NS_RESIZE_EVENT) {
    mIsHandlingResizeEvent = PR_FALSE;
  } else if (aVisitor.mEvent->message == NS_PAGE_UNLOAD &&
             NS_IS_TRUSTED_EVENT(aVisitor.mEvent)) {
    
    
    if (mDocument) {
      NS_ASSERTION(mDoc, "Must have doc");
      mDoc->BindingManager()->ExecuteDetachedHandlers();
    }
    mIsDocumentLoaded = PR_FALSE;
  } else if (aVisitor.mEvent->message == NS_LOAD &&
             NS_IS_TRUSTED_EVENT(aVisitor.mEvent)) {
    
    
    mIsDocumentLoaded = PR_TRUE;

    nsCOMPtr<nsIContent> content(do_QueryInterface(GetFrameElementInternal()));
    nsCOMPtr<nsIDocShellTreeItem> treeItem =
      do_QueryInterface(GetDocShell());

    PRInt32 itemType = nsIDocShellTreeItem::typeChrome;

    if (treeItem) {
      treeItem->GetItemType(&itemType);
    }

    if (content && GetParentInternal() &&
        itemType != nsIDocShellTreeItem::typeChrome) {
      
      

      nsEventStatus status = nsEventStatus_eIgnore;
      nsEvent event(NS_IS_TRUSTED_EVENT(aVisitor.mEvent), NS_LOAD);
      event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

      
      
      
      
      

      nsEventDispatcher::Dispatch(content, nsnull, &event, nsnull, &status);
    }
  }

  return NS_OK;
}

nsresult
nsGlobalWindow::DispatchDOMEvent(nsEvent* aEvent,
                                 nsIDOMEvent* aDOMEvent,
                                 nsPresContext* aPresContext,
                                 nsEventStatus* aEventStatus)
{
  return
    nsEventDispatcher::DispatchDOMEvent(static_cast<nsPIDOMWindow*>(this),
                                       aEvent, aDOMEvent, aPresContext,
                                       aEventStatus);
}

void
nsGlobalWindow::OnFinalize(PRUint32 aLangID, void *aObject)
{
  if (!NS_STID_VALID(aLangID)) {
    NS_ERROR("Invalid language ID");
    return;
  }
  PRUint32 lang_ndx = NS_STID_INDEX(aLangID);
  if (aObject == mScriptGlobals[lang_ndx]) {
    mScriptGlobals[lang_ndx] = nsnull;
  } else if (mScriptGlobals[lang_ndx]) {
    NS_ERROR("Huh? Script language created more than one wrapper for this global!");
  } else {
    NS_WARNING("Weird, we're finalized with a null language global?");
  }
  if (aLangID==nsIProgrammingLanguage::JAVASCRIPT)
    mJSObject = nsnull; 
}

void
nsGlobalWindow::SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts)
{
  FORWARD_TO_INNER_VOID(SetScriptsEnabled, (aEnabled, aFireTimeouts));

  if (aEnabled && aFireTimeouts) {
    
    

    RunTimeout(nsnull);
  }
}

nsresult
nsGlobalWindow::SetArguments(nsIArray *aArguments, nsIPrincipal *aOrigin)
{
  FORWARD_TO_OUTER(SetArguments, (aArguments, aOrigin),
                   NS_ERROR_NOT_INITIALIZED);

  
  
  mArguments = aArguments;
  mArgumentsOrigin = aOrigin;

  nsGlobalWindow *currentInner = GetCurrentInnerWindowInternal();

  if (!mIsModalContentWindow) {
    mArgumentsLast = aArguments;
  } else if (currentInner) {
    
    
    
    
    
    

    currentInner->mArguments = aArguments;
    currentInner->mArgumentsOrigin = aOrigin;
  }

  return currentInner ?
    currentInner->DefineArgumentsProperty(aArguments) : NS_OK;
}

nsresult
nsGlobalWindow::DefineArgumentsProperty(nsIArray *aArguments)
{
  JSContext *cx;
  nsIScriptContext *ctx = GetOuterWindowInternal()->mContext;
  NS_ENSURE_TRUE(aArguments && ctx &&
                 (cx = (JSContext *)ctx->GetNativeContext()),
                 NS_ERROR_NOT_INITIALIZED);

  if (mIsModalContentWindow) {
    
    
    

    return NS_OK;
  }

  PRUint32 langID;
  NS_STID_FOR_ID(langID) {
    void *glob = GetScriptGlobal(langID);
    ctx = GetScriptContext(langID);
    if (glob && ctx) {
      nsresult rv = ctx->SetProperty(glob, "arguments", aArguments);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}





nsIPrincipal*
nsGlobalWindow::GetPrincipal()
{
  if (mDoc) {
    
    return mDoc->NodePrincipal();
  }

  if (mDocumentPrincipal) {
    return mDocumentPrincipal;
  }

  
  
  
  
  

  nsCOMPtr<nsIScriptObjectPrincipal> objPrincipal =
    do_QueryInterface(GetParentInternal());

  if (objPrincipal) {
    return objPrincipal->GetPrincipal();
  }

  return nsnull;
}





NS_IMETHODIMP
nsGlobalWindow::GetDocument(nsIDOMDocument** aDocument)
{
  
  
  
  

  
  
  
  
  
  nsIDocShell *docShell;
  if (!mDocument && (docShell = GetDocShell()))
    nsCOMPtr<nsIDOMDocument> domdoc(do_GetInterface(docShell));

  NS_IF_ADDREF(*aDocument = mDocument);

  return NS_OK;
}





NS_IMETHODIMP
nsGlobalWindow::GetWindow(nsIDOMWindowInternal** aWindow)
{
  FORWARD_TO_OUTER(GetWindow, (aWindow), NS_ERROR_NOT_INITIALIZED);

  *aWindow = static_cast<nsIDOMWindowInternal *>(this);
  NS_ADDREF(*aWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetSelf(nsIDOMWindowInternal** aWindow)
{
  FORWARD_TO_OUTER(GetSelf, (aWindow), NS_ERROR_NOT_INITIALIZED);

  *aWindow = static_cast<nsIDOMWindowInternal *>(this);
  NS_ADDREF(*aWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetNavigator(nsIDOMNavigator** aNavigator)
{
  FORWARD_TO_OUTER(GetNavigator, (aNavigator), NS_ERROR_NOT_INITIALIZED);

  *aNavigator = nsnull;

  if (!mNavigator) {
    mNavigator = new nsNavigator(mDocShell);
    if (!mNavigator) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aNavigator = mNavigator);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetScreen(nsIDOMScreen** aScreen)
{
  FORWARD_TO_OUTER(GetScreen, (aScreen), NS_ERROR_NOT_INITIALIZED);

  *aScreen = nsnull;

  if (!mScreen && mDocShell) {
    mScreen = new nsScreen(mDocShell);
    if (!mScreen) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_IF_ADDREF(*aScreen = mScreen);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetHistory(nsIDOMHistory** aHistory)
{
  FORWARD_TO_OUTER(GetHistory, (aHistory), NS_ERROR_NOT_INITIALIZED);

  *aHistory = nsnull;

  if (!mHistory && mDocShell) {
    mHistory = new nsHistory(mDocShell);
    if (!mHistory) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_IF_ADDREF(*aHistory = mHistory);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetParent(nsIDOMWindow** aParent)
{
  FORWARD_TO_OUTER(GetParent, (aParent), NS_ERROR_NOT_INITIALIZED);

  *aParent = nsnull;
  if (!mDocShell)
    return NS_OK;

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeItem> parent;
  docShellAsItem->GetSameTypeParent(getter_AddRefs(parent));

  if (parent) {
    nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(parent));
    NS_ENSURE_SUCCESS(CallQueryInterface(globalObject.get(), aParent),
                      NS_ERROR_FAILURE);
  }
  else {
    *aParent = static_cast<nsIDOMWindowInternal *>(this);
    NS_ADDREF(*aParent);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetTop(nsIDOMWindow** aTop)
{
  FORWARD_TO_OUTER(GetTop, (aTop), NS_ERROR_NOT_INITIALIZED);

  nsresult ret = NS_OK;

  *aTop = nsnull;
  if (mDocShell) {
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
    nsCOMPtr<nsIDocShellTreeItem> root;
    docShellAsItem->GetSameTypeRootTreeItem(getter_AddRefs(root));

    if (root) {
      nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(root));
      CallQueryInterface(globalObject.get(), aTop);
    }
  }

  return ret;
}

NS_IMETHODIMP
nsGlobalWindow::GetContent(nsIDOMWindow** aContent)
{
  FORWARD_TO_OUTER(GetContent, (aContent), NS_ERROR_NOT_INITIALIZED);

  *aContent = nsnull;

  nsCOMPtr<nsIDocShellTreeItem> primaryContent;

  if (!nsContentUtils::IsCallerChrome()) {
    
    
    
    
    nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(mDocShell));

    if (baseWin) {
      PRBool visible = PR_FALSE;
      baseWin->GetVisibility(&visible);

      if (!visible) {
        nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(mDocShell));

        treeItem->GetSameTypeRootTreeItem(getter_AddRefs(primaryContent));
      }
    }
  }

  if (!primaryContent) {
    nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
    GetTreeOwner(getter_AddRefs(treeOwner));
    NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

    treeOwner->GetPrimaryContentShell(getter_AddRefs(primaryContent));
  }

  nsCOMPtr<nsIDOMWindowInternal> domWindow(do_GetInterface(primaryContent));
  NS_IF_ADDREF(*aContent = domWindow);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetPrompter(nsIPrompt** aPrompt)
{
  FORWARD_TO_OUTER(GetPrompter, (aPrompt), NS_ERROR_NOT_INITIALIZED);

  if (!mDocShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPrompt> prompter(do_GetInterface(mDocShell));
  NS_ENSURE_TRUE(prompter, NS_ERROR_NO_INTERFACE);

  NS_ADDREF(*aPrompt = prompter);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetMenubar(nsIDOMBarProp** aMenubar)
{
  FORWARD_TO_OUTER(GetMenubar, (aMenubar), NS_ERROR_NOT_INITIALIZED);

  *aMenubar = nsnull;

  if (!mMenubar) {
    mMenubar = new nsMenubarProp();
    if (!mMenubar) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIWebBrowserChrome> browserChrome;
    GetWebBrowserChrome(getter_AddRefs(browserChrome));

    mMenubar->SetWebBrowserChrome(browserChrome);
  }

  NS_ADDREF(*aMenubar = mMenubar);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetToolbar(nsIDOMBarProp** aToolbar)
{
  FORWARD_TO_OUTER(GetToolbar, (aToolbar), NS_ERROR_NOT_INITIALIZED);

  *aToolbar = nsnull;

  if (!mToolbar) {
    mToolbar = new nsToolbarProp();
    if (!mToolbar) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIWebBrowserChrome> browserChrome;
    GetWebBrowserChrome(getter_AddRefs(browserChrome));

    mToolbar->SetWebBrowserChrome(browserChrome);
  }

  NS_ADDREF(*aToolbar = mToolbar);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetLocationbar(nsIDOMBarProp** aLocationbar)
{
  FORWARD_TO_OUTER(GetLocationbar, (aLocationbar), NS_ERROR_NOT_INITIALIZED);

  *aLocationbar = nsnull;

  if (!mLocationbar) {
    mLocationbar = new nsLocationbarProp();
    if (!mLocationbar) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIWebBrowserChrome> browserChrome;
    GetWebBrowserChrome(getter_AddRefs(browserChrome));

    mLocationbar->SetWebBrowserChrome(browserChrome);
  }

  NS_ADDREF(*aLocationbar = mLocationbar);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetPersonalbar(nsIDOMBarProp** aPersonalbar)
{
  FORWARD_TO_OUTER(GetPersonalbar, (aPersonalbar), NS_ERROR_NOT_INITIALIZED);

  *aPersonalbar = nsnull;

  if (!mPersonalbar) {
    mPersonalbar = new nsPersonalbarProp();
    if (!mPersonalbar) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIWebBrowserChrome> browserChrome;
    GetWebBrowserChrome(getter_AddRefs(browserChrome));

    mPersonalbar->SetWebBrowserChrome(browserChrome);
  }

  NS_ADDREF(*aPersonalbar = mPersonalbar);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetStatusbar(nsIDOMBarProp** aStatusbar)
{
  FORWARD_TO_OUTER(GetStatusbar, (aStatusbar), NS_ERROR_NOT_INITIALIZED);

  *aStatusbar = nsnull;

  if (!mStatusbar) {
    mStatusbar = new nsStatusbarProp();
    if (!mStatusbar) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIWebBrowserChrome> browserChrome;
    GetWebBrowserChrome(getter_AddRefs(browserChrome));

    mStatusbar->SetWebBrowserChrome(browserChrome);
  }

  NS_ADDREF(*aStatusbar = mStatusbar);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetScrollbars(nsIDOMBarProp** aScrollbars)
{
  FORWARD_TO_OUTER(GetScrollbars, (aScrollbars), NS_ERROR_NOT_INITIALIZED);

  *aScrollbars = nsnull;

  if (!mScrollbars) {
    mScrollbars = new nsScrollbarsProp(this);
    if (!mScrollbars) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIWebBrowserChrome> browserChrome;
    GetWebBrowserChrome(getter_AddRefs(browserChrome));

    mScrollbars->SetWebBrowserChrome(browserChrome);
  }

  NS_ADDREF(*aScrollbars = mScrollbars);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetDirectories(nsIDOMBarProp** aDirectories)
{
  return GetPersonalbar(aDirectories);
}

NS_IMETHODIMP
nsGlobalWindow::GetClosed(PRBool* aClosed)
{
  FORWARD_TO_OUTER(GetClosed, (aClosed), NS_ERROR_NOT_INITIALIZED);

  
  
  *aClosed = mIsClosed || !mDocShell;

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetFrames(nsIDOMWindowCollection** aFrames)
{
  FORWARD_TO_OUTER(GetFrames, (aFrames), NS_ERROR_NOT_INITIALIZED);

  *aFrames = nsnull;

  if (!mFrames && mDocShell) {
    mFrames = new nsDOMWindowList(mDocShell);
    if (!mFrames) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  *aFrames = static_cast<nsIDOMWindowCollection *>(mFrames);
  NS_IF_ADDREF(*aFrames);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetApplicationCache(nsIDOMOfflineResourceList **aApplicationCache)
{
  FORWARD_TO_INNER(GetApplicationCache, (aApplicationCache), NS_ERROR_UNEXPECTED);

  NS_ENSURE_ARG_POINTER(aApplicationCache);

  if (!mApplicationCache) {
    nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(GetDocShell()));
    if (!webNav) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIURI> uri;
    nsresult rv = webNav->GetCurrentURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDocument> doc = do_QueryInterface(mDocument);
    nsCOMPtr<nsIURI> manifestURI;
    nsContentUtils::GetOfflineAppManifest(doc, getter_AddRefs(manifestURI));

    nsIScriptContext* scriptContext = GetContext();
    NS_ENSURE_STATE(scriptContext);

    nsRefPtr<nsDOMOfflineResourceList> applicationCache =
      new nsDOMOfflineResourceList(manifestURI, uri, this, scriptContext);
    NS_ENSURE_TRUE(applicationCache, NS_ERROR_OUT_OF_MEMORY);

    applicationCache->Init();

    mApplicationCache = applicationCache;
  }

  NS_IF_ADDREF(*aApplicationCache = mApplicationCache);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetCrypto(nsIDOMCrypto** aCrypto)
{
  FORWARD_TO_OUTER(GetCrypto, (aCrypto), NS_ERROR_NOT_INITIALIZED);

  if (!mCrypto) {
    mCrypto = do_CreateInstance(kCryptoContractID);
  }

  NS_IF_ADDREF(*aCrypto = mCrypto);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetPkcs11(nsIDOMPkcs11** aPkcs11)
{
  *aPkcs11 = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetControllers(nsIControllers** aResult)
{
  FORWARD_TO_OUTER(GetControllers, (aResult), NS_ERROR_NOT_INITIALIZED);

  if (!mControllers) {
    nsresult rv;
    mControllers = do_CreateInstance(kXULControllersCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIController> controller = do_CreateInstance(
                               NS_WINDOWCONTROLLER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mControllers->InsertControllerAt(0, controller);
    nsCOMPtr<nsIControllerContext> controllerContext = do_QueryInterface(controller);
    if (!controllerContext) return NS_ERROR_FAILURE;

    controllerContext->SetCommandContext(static_cast<nsIDOMWindow*>(this));
  }

  *aResult = mControllers;
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetOpener(nsIDOMWindowInternal** aOpener)
{
  FORWARD_TO_OUTER(GetOpener, (aOpener), NS_ERROR_NOT_INITIALIZED);

  *aOpener = nsnull;

  nsCOMPtr<nsIDOMWindowInternal> opener = do_QueryReferent(mOpener);
  if (!opener) {
    return NS_OK;
  }

  
  if (nsContentUtils::IsCallerTrustedForRead()) {
    NS_ADDREF(*aOpener = opener);
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsPIDOMWindow> openerPwin(do_QueryInterface(opener));
  if (openerPwin) {
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
      do_QueryInterface(openerPwin->GetDocShell());

    if (docShellAsItem) {
      nsCOMPtr<nsIDocShellTreeItem> openerRootItem;
      docShellAsItem->GetRootTreeItem(getter_AddRefs(openerRootItem));
      nsCOMPtr<nsIDocShell> openerRootDocShell(do_QueryInterface(openerRootItem));
      if (openerRootDocShell) {
        PRUint32 appType;
        nsresult rv = openerRootDocShell->GetAppType(&appType);
        if (NS_SUCCEEDED(rv) && appType != nsIDocShell::APP_TYPE_MAIL) {
          *aOpener = opener;
        }
      }
    }
  }
  NS_IF_ADDREF(*aOpener);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SetOpener(nsIDOMWindowInternal* aOpener)
{
  
  
  if (aOpener && !nsContentUtils::IsCallerTrustedForWrite()) {
    return NS_OK;
  }

  SetOpenerWindow(aOpener, PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetStatus(nsAString& aStatus)
{
  FORWARD_TO_OUTER(GetStatus, (aStatus), NS_ERROR_NOT_INITIALIZED);

  aStatus = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SetStatus(const nsAString& aStatus)
{
  FORWARD_TO_OUTER(SetStatus, (aStatus), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanSetProperty("dom.disable_window_status_change")) {
    return NS_OK;
  }

  mStatus = aStatus;

  nsCOMPtr<nsIWebBrowserChrome> browserChrome;
  GetWebBrowserChrome(getter_AddRefs(browserChrome));
  if(browserChrome) {
    browserChrome->SetStatus(nsIWebBrowserChrome::STATUS_SCRIPT,
                             PromiseFlatString(aStatus).get());
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetDefaultStatus(nsAString& aDefaultStatus)
{
  FORWARD_TO_OUTER(GetDefaultStatus, (aDefaultStatus),
                   NS_ERROR_NOT_INITIALIZED);

  aDefaultStatus = mDefaultStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SetDefaultStatus(const nsAString& aDefaultStatus)
{
  FORWARD_TO_OUTER(SetDefaultStatus, (aDefaultStatus),
                   NS_ERROR_NOT_INITIALIZED);

  




  if (!CanSetProperty("dom.disable_window_status_change")) {
    return NS_OK;
  }

  mDefaultStatus = aDefaultStatus;

  nsCOMPtr<nsIWebBrowserChrome> browserChrome;
  GetWebBrowserChrome(getter_AddRefs(browserChrome));
  if (browserChrome) {
    browserChrome->SetStatus(nsIWebBrowserChrome::STATUS_SCRIPT_DEFAULT,
                             PromiseFlatString(aDefaultStatus).get());
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetName(nsAString& aName)
{
  FORWARD_TO_OUTER(GetName, (aName), NS_ERROR_NOT_INITIALIZED);

  nsXPIDLString name;
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  if (docShellAsItem)
    docShellAsItem->GetName(getter_Copies(name));

  aName.Assign(name);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SetName(const nsAString& aName)
{
  FORWARD_TO_OUTER(SetName, (aName), NS_ERROR_NOT_INITIALIZED);

  nsresult result = NS_OK;
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  if (docShellAsItem)
    result = docShellAsItem->SetName(PromiseFlatString(aName).get());
  return result;
}


PRInt32
nsGlobalWindow::DevToCSSIntPixels(PRInt32 px)
{
  if (!mDocShell)
    return px; 
    
  nsCOMPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext)
    return px;

  return presContext->DevPixelsToIntCSSPixels(px);
}

PRInt32
nsGlobalWindow::CSSToDevIntPixels(PRInt32 px)
{
  if (!mDocShell)
    return px; 
    
  nsCOMPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext)
    return px;

  return presContext->CSSPixelsToDevPixels(px);
}

nsIntSize
nsGlobalWindow::DevToCSSIntPixels(nsIntSize px)
{
  if (!mDocShell)
    return px; 
    
  nsCOMPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext)
    return px;
  
  return nsIntSize(
      presContext->DevPixelsToIntCSSPixels(px.width),
      presContext->DevPixelsToIntCSSPixels(px.height));
}

nsIntSize
nsGlobalWindow::CSSToDevIntPixels(nsIntSize px)
{
  if (!mDocShell)
    return px; 
    
  nsCOMPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext)
    return px;
  
  return nsIntSize(
    presContext->CSSPixelsToDevPixels(px.width),
    presContext->CSSPixelsToDevPixels(px.height));
}


NS_IMETHODIMP
nsGlobalWindow::GetInnerWidth(PRInt32* aInnerWidth)
{
  FORWARD_TO_OUTER(GetInnerWidth, (aInnerWidth), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_STATE(mDocShell);

  EnsureSizeUpToDate();

  nsCOMPtr<nsIBaseWindow> docShellWin(do_QueryInterface(mDocShell));
  nsCOMPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));

  if (docShellWin && presContext) {
    PRInt32 width, notused;
    docShellWin->GetSize(&width, &notused);
    *aInnerWidth = nsPresContext::
      AppUnitsToIntCSSPixels(presContext->DevPixelsToAppUnits(width));
  } else {
    *aInnerWidth = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SetInnerWidth(PRInt32 aInnerWidth)
{
  FORWARD_TO_OUTER(SetInnerWidth, (aInnerWidth), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_STATE(mDocShell);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(&aInnerWidth, nsnull),
                    NS_ERROR_FAILURE);

  PRInt32 width = CSSToDevIntPixels(aInnerWidth);

  nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(mDocShell));
  PRInt32 notused, height = 0;
  docShellAsWin->GetSize(&notused, &height);

  NS_ENSURE_SUCCESS(treeOwner->SizeShellTo(docShellAsItem, width, height),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetInnerHeight(PRInt32* aInnerHeight)
{
  FORWARD_TO_OUTER(GetInnerHeight, (aInnerHeight), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_STATE(mDocShell);

  EnsureSizeUpToDate();

  nsCOMPtr<nsIBaseWindow> docShellWin(do_QueryInterface(mDocShell));
  nsCOMPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));

  if (docShellWin && presContext) {
    PRInt32 height, notused;
    docShellWin->GetSize(&notused, &height);
    *aInnerHeight = nsPresContext::
      AppUnitsToIntCSSPixels(presContext->DevPixelsToAppUnits(height));
  } else {
    *aInnerHeight = 0;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SetInnerHeight(PRInt32 aInnerHeight)
{
  FORWARD_TO_OUTER(SetInnerHeight, (aInnerHeight), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_STATE(mDocShell);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(nsnull, &aInnerHeight),
                    NS_ERROR_FAILURE);

  PRInt32 height = CSSToDevIntPixels(aInnerHeight);

  nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(mDocShell));
  PRInt32 width = 0, notused;
  docShellAsWin->GetSize(&width, &notused);

  NS_ENSURE_SUCCESS(treeOwner->SizeShellTo(docShellAsItem, width, height),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
nsGlobalWindow::GetOuterSize(nsIntSize* aSizeCSSPixels)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  nsGlobalWindow* rootWindow =
    static_cast<nsGlobalWindow *>(GetPrivateRoot());
  if (rootWindow) {
    rootWindow->FlushPendingNotifications(Flush_Layout);
  }

  nsIntSize sizeDevPixels;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetSize(&sizeDevPixels.width,
                                            &sizeDevPixels.height),
                    NS_ERROR_FAILURE);

  *aSizeCSSPixels = DevToCSSIntPixels(sizeDevPixels);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetOuterWidth(PRInt32* aOuterWidth)
{
  FORWARD_TO_OUTER(GetOuterWidth, (aOuterWidth), NS_ERROR_NOT_INITIALIZED);

  nsIntSize sizeCSSPixels;
  nsresult rv = GetOuterSize(&sizeCSSPixels);
  NS_ENSURE_SUCCESS(rv, rv);

  *aOuterWidth = sizeCSSPixels.width;
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetOuterHeight(PRInt32* aOuterHeight)
{
  FORWARD_TO_OUTER(GetOuterHeight, (aOuterHeight), NS_ERROR_NOT_INITIALIZED);

  nsIntSize sizeCSSPixels;
  nsresult rv = GetOuterSize(&sizeCSSPixels);
  NS_ENSURE_SUCCESS(rv, rv);

  *aOuterHeight = sizeCSSPixels.height;
  return NS_OK;
}

nsresult
nsGlobalWindow::SetOuterSize(PRInt32 aLengthCSSPixels, PRBool aIsWidth)
{
  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(
                        aIsWidth ? &aLengthCSSPixels : nsnull,
                        aIsWidth ? nsnull : &aLengthCSSPixels),
                    NS_ERROR_FAILURE);

  PRInt32 width, height;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetSize(&width, &height), NS_ERROR_FAILURE);

  PRInt32 lengthDevPixels = CSSToDevIntPixels(aLengthCSSPixels);
  if (aIsWidth) {
    width = lengthDevPixels;
  } else {
    height = lengthDevPixels;
  }
  return treeOwnerAsWin->SetSize(width, height, PR_TRUE);    
}

NS_IMETHODIMP
nsGlobalWindow::SetOuterWidth(PRInt32 aOuterWidth)
{
  FORWARD_TO_OUTER(SetOuterWidth, (aOuterWidth), NS_ERROR_NOT_INITIALIZED);

  return SetOuterSize(aOuterWidth, PR_TRUE);
}

NS_IMETHODIMP
nsGlobalWindow::SetOuterHeight(PRInt32 aOuterHeight)
{
  FORWARD_TO_OUTER(SetOuterHeight, (aOuterHeight), NS_ERROR_NOT_INITIALIZED);

  return SetOuterSize(aOuterHeight, PR_FALSE);
}

NS_IMETHODIMP
nsGlobalWindow::GetScreenX(PRInt32* aScreenX)
{
  FORWARD_TO_OUTER(GetScreenX, (aScreenX), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  PRInt32 x, y;

  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, &y),
                    NS_ERROR_FAILURE);

  *aScreenX = DevToCSSIntPixels(x);
  return NS_OK;
}

nsRect
nsGlobalWindow::GetInnerScreenRect()
{
  if (!mDocShell)
    return nsRect();

  nsGlobalWindow* rootWindow =
    static_cast<nsGlobalWindow*>(GetPrivateRoot());
  if (rootWindow) {
    rootWindow->FlushPendingNotifications(Flush_Layout);
  }

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell)
    return nsRect();
  nsIFrame* rootFrame = presShell->GetRootFrame();
  if (!rootFrame)
    return nsRect();

  return rootFrame->GetScreenRectInAppUnits();
}

NS_IMETHODIMP
nsGlobalWindow::GetMozInnerScreenX(float* aScreenX)
{
  FORWARD_TO_OUTER(GetMozInnerScreenX, (aScreenX), NS_ERROR_NOT_INITIALIZED);

  nsRect r = GetInnerScreenRect();
  *aScreenX = nsPresContext::AppUnitsToFloatCSSPixels(r.x);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetMozInnerScreenY(float* aScreenY)
{
  FORWARD_TO_OUTER(GetMozInnerScreenY, (aScreenY), NS_ERROR_NOT_INITIALIZED);

  nsRect r = GetInnerScreenRect();
  *aScreenY = nsPresContext::AppUnitsToFloatCSSPixels(r.y);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SetScreenX(PRInt32 aScreenX)
{
  FORWARD_TO_OUTER(SetScreenX, (aScreenX), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityLeftAndTop(&aScreenX, nsnull),
                    NS_ERROR_FAILURE);

  PRInt32 x, y;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, &y),
                    NS_ERROR_FAILURE);

  x = CSSToDevIntPixels(aScreenX);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetPosition(x, y),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetScreenY(PRInt32* aScreenY)
{
  FORWARD_TO_OUTER(GetScreenY, (aScreenY), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  PRInt32 x, y;

  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, &y),
                    NS_ERROR_FAILURE);

  *aScreenY = DevToCSSIntPixels(y);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SetScreenY(PRInt32 aScreenY)
{
  FORWARD_TO_OUTER(SetScreenY, (aScreenY), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityLeftAndTop(nsnull, &aScreenY),
                    NS_ERROR_FAILURE);

  PRInt32 x, y;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, &y),
                    NS_ERROR_FAILURE);

  y = CSSToDevIntPixels(aScreenY);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetPosition(x, y),
                    NS_ERROR_FAILURE);

  return NS_OK;
}



nsresult
nsGlobalWindow::CheckSecurityWidthAndHeight(PRInt32* aWidth, PRInt32* aHeight)
{
#ifdef MOZ_XUL
  if (!nsContentUtils::IsCallerTrustedForWrite()) {
    
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
    nsContentUtils::HidePopupsInDocument(doc);
  }
#endif

  
  if ((aWidth && *aWidth < 100) || (aHeight && *aHeight < 100)) {
    

    if (!nsContentUtils::IsCallerTrustedForWrite()) {
      
      if (aWidth && *aWidth < 100) {
        *aWidth = 100;
      }
      if (aHeight && *aHeight < 100) {
        *aHeight = 100;
      }
    }
  }

  return NS_OK;
}



nsresult
nsGlobalWindow::CheckSecurityLeftAndTop(PRInt32* aLeft, PRInt32* aTop)
{
  

  

  if (!nsContentUtils::IsCallerTrustedForWrite()) {
#ifdef MOZ_XUL
    
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
    nsContentUtils::HidePopupsInDocument(doc);
#endif

    PRInt32 screenLeft, screenTop, screenWidth, screenHeight;
    PRInt32 winLeft, winTop, winWidth, winHeight;

    nsGlobalWindow* rootWindow =
      static_cast<nsGlobalWindow*>(GetPrivateRoot());
    if (rootWindow) {
      rootWindow->FlushPendingNotifications(Flush_Layout);
    }

    
    nsCOMPtr<nsIBaseWindow> treeOwner;
    GetTreeOwner(getter_AddRefs(treeOwner));
    if (treeOwner)
      treeOwner->GetPositionAndSize(&winLeft, &winTop, &winWidth, &winHeight);

    
    
    winLeft   = DevToCSSIntPixels(winLeft);
    winTop    = DevToCSSIntPixels(winTop);
    winWidth  = DevToCSSIntPixels(winWidth);
    winHeight = DevToCSSIntPixels(winHeight);

    
    
    nsCOMPtr<nsIDOMScreen> screen;
    GetScreen(getter_AddRefs(screen));
    if (screen) {
      screen->GetAvailLeft(&screenLeft);
      screen->GetAvailWidth(&screenWidth);
      screen->GetAvailHeight(&screenHeight);
#if defined(XP_MAC) || defined(XP_MACOSX)
      






      screen->GetTop(&screenTop);
#else
      screen->GetAvailTop(&screenTop);
#endif
    }

    if (screen && treeOwner) {
      if (aLeft) {
        if (screenLeft+screenWidth < *aLeft+winWidth)
          *aLeft = screenLeft+screenWidth - winWidth;
        if (screenLeft > *aLeft)
          *aLeft = screenLeft;
      }
      if (aTop) {
        if (screenTop+screenHeight < *aTop+winHeight)
          *aTop = screenTop+screenHeight - winHeight;
        if (screenTop > *aTop)
          *aTop = screenTop;
      }
    } else {
      if (aLeft)
        *aLeft = 0;
      if (aTop)
        *aTop = 0;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetPageXOffset(PRInt32* aPageXOffset)
{
  return GetScrollX(aPageXOffset);
}

NS_IMETHODIMP
nsGlobalWindow::GetPageYOffset(PRInt32* aPageYOffset)
{
  return GetScrollY(aPageYOffset);
}

nsresult
nsGlobalWindow::GetScrollMaxXY(PRInt32* aScrollMaxX, PRInt32* aScrollMaxY)
{
  FORWARD_TO_OUTER(GetScrollMaxXY, (aScrollMaxX, aScrollMaxY),
                   NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  nsIScrollableView *view = nsnull;      

  FlushPendingNotifications(Flush_Layout);
  GetScrollInfo(&view);
  if (!view)
    return NS_OK;      

  nsSize scrolledSize;
  rv = view->GetContainerSize(&scrolledSize.width, &scrolledSize.height);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRect portRect = view->View()->GetBounds();

  if (aScrollMaxX)
    *aScrollMaxX = PR_MAX(0,
      (PRInt32)floor(nsPresContext::AppUnitsToFloatCSSPixels(scrolledSize.width - portRect.width)));
  if (aScrollMaxY)
    *aScrollMaxY = PR_MAX(0,
      (PRInt32)floor(nsPresContext::AppUnitsToFloatCSSPixels(scrolledSize.height - portRect.height)));

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetScrollMaxX(PRInt32* aScrollMaxX)
{
  NS_ENSURE_ARG_POINTER(aScrollMaxX);
  *aScrollMaxX = 0;
  return GetScrollMaxXY(aScrollMaxX, nsnull);
}

NS_IMETHODIMP
nsGlobalWindow::GetScrollMaxY(PRInt32* aScrollMaxY)
{
  NS_ENSURE_ARG_POINTER(aScrollMaxY);
  *aScrollMaxY = 0;
  return GetScrollMaxXY(nsnull, aScrollMaxY);
}

nsresult
nsGlobalWindow::GetScrollXY(PRInt32* aScrollX, PRInt32* aScrollY,
                            PRBool aDoFlush)
{
  FORWARD_TO_OUTER(GetScrollXY, (aScrollX, aScrollY, aDoFlush),
                   NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  nsIScrollableView *view = nsnull;      

  if (aDoFlush) {
    FlushPendingNotifications(Flush_Layout);
  } else {
    EnsureSizeUpToDate();
  }
  
  GetScrollInfo(&view);
  if (!view)
    return NS_OK;      

  nscoord xPos, yPos;
  rv = view->GetScrollPosition(xPos, yPos);
  NS_ENSURE_SUCCESS(rv, rv);

  if ((xPos != 0 || yPos != 0) && !aDoFlush) {
    
    
    
    return GetScrollXY(aScrollX, aScrollY, PR_TRUE);
  }
  
  if (aScrollX)
    *aScrollX = nsPresContext::AppUnitsToIntCSSPixels(xPos);
  if (aScrollY)
    *aScrollY = nsPresContext::AppUnitsToIntCSSPixels(yPos);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetScrollX(PRInt32* aScrollX)
{
  NS_ENSURE_ARG_POINTER(aScrollX);
  *aScrollX = 0;
  return GetScrollXY(aScrollX, nsnull, PR_FALSE);
}

NS_IMETHODIMP
nsGlobalWindow::GetScrollY(PRInt32* aScrollY)
{
  NS_ENSURE_ARG_POINTER(aScrollY);
  *aScrollY = 0;
  return GetScrollXY(nsnull, aScrollY, PR_FALSE);
}

NS_IMETHODIMP
nsGlobalWindow::GetLength(PRUint32* aLength)
{
  nsCOMPtr<nsIDOMWindowCollection> frames;
  if (NS_SUCCEEDED(GetFrames(getter_AddRefs(frames))) && frames) {
    return frames->GetLength(aLength);
  }
  return NS_ERROR_FAILURE;
}

PRBool
nsGlobalWindow::DispatchCustomEvent(const char *aEventName)
{
  PRBool defaultActionEnabled = PR_TRUE;
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
  nsContentUtils::DispatchTrustedEvent(doc,
                                       static_cast<nsIScriptGlobalObject*>(this),
                                       NS_ConvertASCIItoUTF16(aEventName),
                                       PR_TRUE, PR_TRUE, &defaultActionEnabled);

  return defaultActionEnabled;
}

static already_AddRefed<nsIDocShellTreeItem>
GetCallerDocShellTreeItem()
{
  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  nsIDocShellTreeItem *callerItem = nsnull;

  if (cx) {
    nsCOMPtr<nsIWebNavigation> callerWebNav =
      do_GetInterface(nsJSUtils::GetDynamicScriptGlobal(cx));

    if (callerWebNav) {
      CallQueryInterface(callerWebNav, &callerItem);
    }
  }

  return callerItem;
}

PRBool
nsGlobalWindow::WindowExists(const nsAString& aName,
                             PRBool aLookForCallerOnJSStack)
{
  NS_PRECONDITION(IsOuterWindow(), "Must be outer window");
  NS_PRECONDITION(mDocShell, "Must have docshell");

  nsCOMPtr<nsIDocShellTreeItem> caller;
  if (aLookForCallerOnJSStack) {
    caller = GetCallerDocShellTreeItem();
  }

  nsCOMPtr<nsIDocShellTreeItem> docShell = do_QueryInterface(mDocShell);
  NS_ASSERTION(docShell,
               "Docshell doesn't implement nsIDocShellTreeItem?");

  if (!caller) {
    caller = docShell;
  }

  nsCOMPtr<nsIDocShellTreeItem> namedItem;
  docShell->FindItemWithName(PromiseFlatString(aName).get(), nsnull, caller,
                             getter_AddRefs(namedItem));
  return namedItem != nsnull;
}

already_AddRefed<nsIWidget>
nsGlobalWindow::GetMainWidget()
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));

  nsIWidget *widget = nsnull;

  if (treeOwnerAsWin) {
    treeOwnerAsWin->GetMainWidget(&widget);
  }

  return widget;
}

nsIWidget*
nsGlobalWindow::GetNearestWidget()
{
  nsIDocShell* docShell = GetDocShell();
  NS_ENSURE_TRUE(docShell, nsnull);
  nsCOMPtr<nsIPresShell> presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  NS_ENSURE_TRUE(presShell, nsnull);
  nsIFrame* rootFrame = presShell->GetRootFrame();
  NS_ENSURE_TRUE(rootFrame, nsnull);
  return rootFrame->GetView()->GetNearestWidget(nsnull);
}

NS_IMETHODIMP
nsGlobalWindow::SetFullScreen(PRBool aFullScreen)
{
  FORWARD_TO_OUTER(SetFullScreen, (aFullScreen), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  PRBool rootWinFullScreen;
  GetFullScreen(&rootWinFullScreen);
  
  if (aFullScreen == rootWinFullScreen || 
      !nsContentUtils::IsCallerTrustedForWrite()) {
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(mDocShell);
  nsCOMPtr<nsIDocShellTreeItem> rootItem;
  treeItem->GetRootTreeItem(getter_AddRefs(rootItem));
  nsCOMPtr<nsIDOMWindowInternal> window = do_GetInterface(rootItem);
  if (!window)
    return NS_ERROR_FAILURE;
  if (rootItem != treeItem)
    return window->SetFullScreen(aFullScreen);

  
  
  PRInt32 itemType;
  treeItem->GetItemType(&itemType);
  if (itemType != nsIDocShellTreeItem::typeChrome)
    return NS_ERROR_FAILURE;

  
  
  if (!DispatchCustomEvent("fullscreen")) {
    return NS_OK;
  }

  nsCOMPtr<nsIWidget> widget = GetMainWidget();
  if (widget)
    widget->MakeFullScreen(aFullScreen);

  mFullScreen = aFullScreen;

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetFullScreen(PRBool* aFullScreen)
{
  FORWARD_TO_OUTER(GetFullScreen, (aFullScreen), NS_ERROR_NOT_INITIALIZED);

  
  
  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(mDocShell);
  if (treeItem) {
    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    treeItem->GetRootTreeItem(getter_AddRefs(rootItem));
    if (rootItem != treeItem) {
      nsCOMPtr<nsIDOMWindowInternal> window = do_GetInterface(rootItem);
      if (window)
        return window->GetFullScreen(aFullScreen);
    }
  }

  
  *aFullScreen = mFullScreen;
  return NS_OK;
}

PRBool
nsGlobalWindow::DOMWindowDumpEnabled()
{
#if !(defined(NS_DEBUG) || defined(MOZ_ENABLE_JS_DUMP))
  
  
  
  return gDOMWindowDumpEnabled;
#else
  return PR_TRUE;
#endif
}

NS_IMETHODIMP
nsGlobalWindow::Dump(const nsAString& aStr)
{
  if (!DOMWindowDumpEnabled()) {
    return NS_OK;
  }

  char *cstr = ToNewUTF8String(aStr);

#if defined(XP_MAC) || defined(XP_MACOSX)
  
  char *c = cstr, *cEnd = cstr + aStr.Length();
  while (c < cEnd) {
    if (*c == '\r')
      *c = '\n';
    c++;
  }
#endif

  if (cstr) {
    FILE *fp = gDumpFile ? gDumpFile : stdout;
    fputs(cstr, fp);
    fflush(fp);
    nsMemory::Free(cstr);
  }

  return NS_OK;
}

void
nsGlobalWindow::EnsureReflowFlushAndPaint()
{
  NS_ASSERTION(mDocShell, "EnsureReflowFlushAndPaint() called with no "
               "docshell!");

  if (!mDocShell)
    return;

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));

  if (!presShell)
    return;

  
  if (mDoc) {
    mDoc->FlushPendingNotifications(Flush_Layout);
  }

  
  presShell->UnsuppressPainting();
}

NS_IMETHODIMP
nsGlobalWindow::GetTextZoom(float *aZoom)
{
  FORWARD_TO_OUTER(GetTextZoom, (aZoom), NS_ERROR_NOT_INITIALIZED);

  if (mDocShell) {
    nsCOMPtr<nsIContentViewer> contentViewer;
    mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
    nsCOMPtr<nsIMarkupDocumentViewer> markupViewer(do_QueryInterface(contentViewer));

    if (markupViewer) {
      return markupViewer->GetTextZoom(aZoom);
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGlobalWindow::SetTextZoom(float aZoom)
{
  FORWARD_TO_OUTER(SetTextZoom, (aZoom), NS_ERROR_NOT_INITIALIZED);

  if (mDocShell) {
    nsCOMPtr<nsIContentViewer> contentViewer;
    mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
    nsCOMPtr<nsIMarkupDocumentViewer> markupViewer(do_QueryInterface(contentViewer));

    if (markupViewer)
      return markupViewer->SetTextZoom(aZoom);
  }
  return NS_ERROR_FAILURE;
}


void
nsGlobalWindow::MakeScriptDialogTitle(nsAString &aOutTitle)
{
  aOutTitle.Truncate();

  
  

  nsresult rv = NS_OK;
  NS_ASSERTION(nsContentUtils::GetSecurityManager(),
    "Global Window has no security manager!");
  if (nsContentUtils::GetSecurityManager()) {
    nsCOMPtr<nsIPrincipal> principal;
    rv = nsContentUtils::GetSecurityManager()->
      GetSubjectPrincipal(getter_AddRefs(principal));

    if (NS_SUCCEEDED(rv) && principal) {
      nsCOMPtr<nsIURI> uri;
      rv = principal->GetURI(getter_AddRefs(uri));

      if (NS_SUCCEEDED(rv) && uri) {
        

        nsCOMPtr<nsIURIFixup> fixup(do_GetService(NS_URIFIXUP_CONTRACTID));
        if (fixup) {
          nsCOMPtr<nsIURI> fixedURI;
          rv = fixup->CreateExposableURI(uri, getter_AddRefs(fixedURI));
          if (NS_SUCCEEDED(rv) && fixedURI) {
            nsCAutoString host;
            fixedURI->GetHost(host);

            if (!host.IsEmpty()) {
              
              
              

              nsCAutoString prepath;
              fixedURI->GetPrePath(prepath);

              NS_ConvertUTF8toUTF16 ucsPrePath(prepath);
              const PRUnichar *formatStrings[] = { ucsPrePath.get() };
              nsXPIDLString tempString;
              nsContentUtils::FormatLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                                    "ScriptDlgHeading",
                                                    formatStrings, NS_ARRAY_LENGTH(formatStrings),
                                                    tempString);
              aOutTitle = tempString;
            }
          }
        }
      }
    }
    else { 
      NS_WARNING("No script principal? Who is calling alert/confirm/prompt?!");
    }
  }

  if (aOutTitle.IsEmpty()) {
    
    nsXPIDLString tempString;
    nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                       "ScriptDlgGenericHeading",
                                       tempString);
    aOutTitle = tempString;
  }

  
  if (aOutTitle.IsEmpty()) {
    NS_WARNING("could not get ScriptDlgGenericHeading string from string bundle");
    aOutTitle.AssignLiteral("[Script]");
  }
}


PRBool
nsGlobalWindow::CanMoveResizeWindows()
{
  if (!CanSetProperty("dom.disable_window_move_resize"))
    return PR_FALSE;

  if (gMouseDown && !gDragServiceDisabled) {
    nsCOMPtr<nsIDragService> ds =
      do_GetService("@mozilla.org/widget/dragservice;1");
    if (ds) {
      gDragServiceDisabled = PR_TRUE;
      ds->Suppress();
    }
  }
  return PR_TRUE;
}

NS_IMETHODIMP
nsGlobalWindow::Alert(const nsAString& aString)
{
  FORWARD_TO_OUTER(Alert, (aString), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIPrompt> prompter(do_GetInterface(mDocShell));
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

  
  

  NS_NAMED_LITERAL_STRING(null_str, "null");

  const nsAString *str = DOMStringIsNull(aString) ? &null_str : &aString;

  
  
  EnsureReflowFlushAndPaint();

  nsAutoString title;
  MakeScriptDialogTitle(title);

  
  
  nsAutoString final;
  StripNullChars(*str, final);

  return prompter->Alert(title.get(), final.get());
}

NS_IMETHODIMP
nsGlobalWindow::Confirm(const nsAString& aString, PRBool* aReturn)
{
  FORWARD_TO_OUTER(Confirm, (aString, aReturn), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIPrompt> prompter(do_GetInterface(mDocShell));
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

  *aReturn = PR_FALSE;

  
  
  EnsureReflowFlushAndPaint();

  nsAutoString title;
  MakeScriptDialogTitle(title);

  
  
  nsAutoString final;
  StripNullChars(aString, final);

  return prompter->Confirm(title.get(), final.get(),
                           aReturn);
}

NS_IMETHODIMP
nsGlobalWindow::Prompt(const nsAString& aMessage, const nsAString& aInitial,
                       const nsAString& aTitle, PRUint32 aSavePassword,
                       nsAString& aReturn)
{
  
  
  SetDOMStringToNull(aReturn);

  
  
  

  PR_STATIC_ASSERT(nsIAuthPrompt::SAVE_PASSWORD_NEVER == 0);

  nsresult rv;
  nsCOMPtr<nsIWindowWatcher> wwatch =
    do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAuthPrompt> prompter;
  wwatch->GetNewAuthPrompter(this, getter_AddRefs(prompter));
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

  PRBool b;
  nsXPIDLString uniResult;

  
  
  EnsureReflowFlushAndPaint();

  nsAutoString title;
  MakeScriptDialogTitle(title);
  
  
  
  nsAutoString fixedMessage, fixedInitial;
  StripNullChars(aMessage, fixedMessage);
  StripNullChars(aInitial, fixedInitial);

  rv = prompter->Prompt(title.get(), fixedMessage.get(), nsnull,
                        aSavePassword, fixedInitial.get(),
                        getter_Copies(uniResult), &b);
  NS_ENSURE_SUCCESS(rv, rv);

  if (uniResult && b) {
    aReturn.Assign(uniResult);
  }

  return rv;
}

NS_IMETHODIMP
nsGlobalWindow::Focus()
{
  FORWARD_TO_OUTER(Focus, (), NS_ERROR_NOT_INITIALIZED);

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm)
    return NS_OK;

  nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(mDocShell);

  PRBool isVisible = PR_FALSE;
  if (baseWin) {
    baseWin->GetVisibility(&isVisible);
  }

  if (!isVisible) {
    
    return NS_OK;
  }

  







  PRBool canFocus =
    CanSetProperty("dom.disable_window_flip") ||
    CheckOpenAllow(CheckForAbusePoint()) == allowNoAbuse;

  nsCOMPtr<nsIDOMWindow> activeWindow;
  fm->GetActiveWindow(getter_AddRefs(activeWindow));

  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(mDocShell);
  NS_ASSERTION(treeItem, "What happened?");
  nsCOMPtr<nsIDocShellTreeItem> rootItem;
  treeItem->GetRootTreeItem(getter_AddRefs(rootItem));
  nsCOMPtr<nsIDOMWindow> rootWin = do_GetInterface(rootItem);
  PRBool isActive = (rootWin == activeWindow);

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  if (treeOwnerAsWin && (canFocus || isActive)) {
    PRBool isEnabled = PR_TRUE;
    if (NS_SUCCEEDED(treeOwnerAsWin->GetEnabled(&isEnabled)) && !isEnabled) {
      NS_WARNING( "Should not try to set the focus on a disabled window" );
      return NS_OK;
    }

    
    nsCOMPtr<nsIEmbeddingSiteWindow> embeddingWin(do_GetInterface(treeOwnerAsWin));
    if (embeddingWin)
      embeddingWin->SetFocus();
  }

  if (!mDocShell)
    return NS_OK;

  nsCOMPtr<nsIPresShell> presShell;
  
  
  
  PRBool lookForPresShell = PR_TRUE;
  PRInt32 itemType = nsIDocShellTreeItem::typeContent;
  treeItem->GetItemType(&itemType);
  if (itemType == nsIDocShellTreeItem::typeChrome &&
      GetPrivateRoot() == static_cast<nsIDOMWindowInternal*>(this) &&
      mDocument) {
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
    NS_ASSERTION(doc, "Bogus doc?");
    nsIURI* ourURI = doc->GetDocumentURI();
    if (ourURI) {
      lookForPresShell = !IsAboutBlank(ourURI);
    }
  }

  if (lookForPresShell) {
    mDocShell->GetEldestPresShell(getter_AddRefs(presShell));
  }

  nsCOMPtr<nsIDocShellTreeItem> parentDsti;
  treeItem->GetParent(getter_AddRefs(parentDsti));

  
  nsCOMPtr<nsIDOMWindow> parent(do_GetInterface(parentDsti));
  if (parent) {
    nsCOMPtr<nsIDOMDocument> parentdomdoc;
    parent->GetDocument(getter_AddRefs(parentdomdoc));

    nsCOMPtr<nsIDocument> parentdoc = do_QueryInterface(parentdomdoc);
    if (!parentdoc)
      return NS_OK;

    nsCOMPtr<nsIDocument> doc = do_QueryInterface(mDocument);
    nsIContent* frame = parentdoc->FindContentForSubDocument(doc);
    nsCOMPtr<nsIDOMElement> frameElement = do_QueryInterface(frame);
    if (frameElement) {
      PRUint32 flags = nsIFocusManager::FLAG_NOSCROLL;
      if (canFocus)
        flags |= nsIFocusManager::FLAG_RAISE;
      return fm->SetFocus(frameElement, flags);
    }
  }
  else if (canFocus) {
    
    
    return fm->SetActiveWindow(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::Blur()
{
  FORWARD_TO_OUTER(Blur, (), NS_ERROR_NOT_INITIALIZED);

  
  
  nsresult rv = NS_OK;

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  GetTreeOwner(getter_AddRefs(treeOwner));
  nsCOMPtr<nsIEmbeddingSiteWindow2> siteWindow(do_GetInterface(treeOwner));
  if (siteWindow) {
    
    rv = siteWindow->Blur();

    
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(mDocument);
    if (fm && mDocument) {
      nsCOMPtr<nsIDOMElement> element;
      fm->GetFocusedElementForWindow(this, PR_FALSE, nsnull, getter_AddRefs(element));
      nsCOMPtr<nsIContent> content = do_QueryInterface(element);
      if (content == doc->GetRootContent())
        fm->ClearFocus(this);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsGlobalWindow::Back()
{
  FORWARD_TO_OUTER(Back, (), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);

  return webNav->GoBack();
}

NS_IMETHODIMP
nsGlobalWindow::Forward()
{
  FORWARD_TO_OUTER(Forward, (), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);

  return webNav->GoForward();
}

NS_IMETHODIMP
nsGlobalWindow::Home()
{
  FORWARD_TO_OUTER(Home, (), NS_ERROR_NOT_INITIALIZED);

  if (!mDocShell)
    return NS_OK;

  nsAdoptingString homeURL =
    nsContentUtils::GetLocalizedStringPref(PREF_BROWSER_STARTUP_HOMEPAGE);

  if (homeURL.IsEmpty()) {
    
#ifdef DEBUG_seth
    printf("all else failed.  using %s as the home page\n", DEFAULT_HOME_PAGE);
#endif
    CopyASCIItoUTF16(DEFAULT_HOME_PAGE, homeURL);
  }

#ifdef MOZ_PHOENIX
  {
    
    
    
    
    
    
    
    
    
    PRInt32 firstPipe = homeURL.FindChar('|');

    if (firstPipe > 0) {
      homeURL.Truncate(firstPipe);
    }
  }
#endif

  nsresult rv;
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);
  rv = webNav->LoadURI(homeURL.get(),
                       nsIWebNavigation::LOAD_FLAGS_NONE,
                       nsnull,
                       nsnull,
                       nsnull);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::Stop()
{
  FORWARD_TO_OUTER(Stop, (), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
  if (!webNav)
    return NS_OK;

  return webNav->Stop(nsIWebNavigation::STOP_ALL);
}

NS_IMETHODIMP
nsGlobalWindow::Print()
{
#ifdef NS_PRINTING
  FORWARD_TO_OUTER(Print, (), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIWebBrowserPrint> webBrowserPrint;
  if (NS_SUCCEEDED(GetInterface(NS_GET_IID(nsIWebBrowserPrint),
                                getter_AddRefs(webBrowserPrint)))) {

    nsCOMPtr<nsIPrintSettingsService> printSettingsService = 
      do_GetService("@mozilla.org/gfx/printsettings-service;1");

    nsCOMPtr<nsIPrintSettings> printSettings;
    if (printSettingsService) {
      PRBool printSettingsAreGlobal =
        nsContentUtils::GetBoolPref("print.use_global_printsettings", PR_FALSE);

      if (printSettingsAreGlobal) {
        printSettingsService->GetGlobalPrintSettings(getter_AddRefs(printSettings));

        nsXPIDLString printerName;
        printSettingsService->GetDefaultPrinterName(getter_Copies(printerName));
        if (printerName)
          printSettingsService->InitPrintSettingsFromPrinter(printerName, printSettings);
        printSettingsService->InitPrintSettingsFromPrefs(printSettings, 
                                                         PR_TRUE, 
                                                         nsIPrintSettings::kInitSaveAll);
      } else {
        printSettingsService->GetNewPrintSettings(getter_AddRefs(printSettings));
      }

      EnterModalState();
      webBrowserPrint->Print(printSettings, nsnull);
      LeaveModalState();

      PRBool savePrintSettings =
        nsContentUtils::GetBoolPref("print.save_print_settings", PR_FALSE);
      if (printSettingsAreGlobal && savePrintSettings) {
        printSettingsService->
          SavePrintSettingsToPrefs(printSettings,
                                   PR_TRUE,
                                   nsIPrintSettings::kInitSaveAll);
        printSettingsService->
          SavePrintSettingsToPrefs(printSettings,
                                   PR_FALSE,
                                   nsIPrintSettings::kInitSavePrinterName);
      }
    } else {
      webBrowserPrint->GetGlobalPrintSettings(getter_AddRefs(printSettings));
      webBrowserPrint->Print(printSettings, nsnull);
    }
  } 

  return NS_OK;
#else
  return NS_ERROR_NOT_AVAILABLE;
#endif
}

NS_IMETHODIMP
nsGlobalWindow::MoveTo(PRInt32 aXPos, PRInt32 aYPos)
{
  FORWARD_TO_OUTER(MoveTo, (aXPos, aYPos), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityLeftAndTop(&aXPos, &aYPos),
                    NS_ERROR_FAILURE);

  
  nsIntSize devPos(CSSToDevIntPixels(nsIntSize(aXPos, aYPos)));

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetPosition(devPos.width, devPos.height),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::MoveBy(PRInt32 aXDif, PRInt32 aYDif)
{
  FORWARD_TO_OUTER(MoveBy, (aXDif, aYDif), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  
  
  

  PRInt32 x, y;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, &y), NS_ERROR_FAILURE);

  
  nsIntSize cssPos(DevToCSSIntPixels(nsIntSize(x, y)));

  cssPos.width += aXDif;
  cssPos.height += aYDif;
  
  NS_ENSURE_SUCCESS(CheckSecurityLeftAndTop(&cssPos.width,
                                            &cssPos.height),
                    NS_ERROR_FAILURE);

  nsIntSize newDevPos(CSSToDevIntPixels(cssPos));

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetPosition(newDevPos.width,
                                                newDevPos.height),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::ResizeTo(PRInt32 aWidth, PRInt32 aHeight)
{
  FORWARD_TO_OUTER(ResizeTo, (aWidth, aHeight), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);
  
  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(&aWidth, &aHeight),
                    NS_ERROR_FAILURE);

  nsIntSize devSz(CSSToDevIntPixels(nsIntSize(aWidth, aHeight)));

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetSize(devSz.width, devSz.height, PR_TRUE),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::ResizeBy(PRInt32 aWidthDif, PRInt32 aHeightDif)
{
  FORWARD_TO_OUTER(ResizeBy, (aWidthDif, aHeightDif), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  PRInt32 width, height;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetSize(&width, &height), NS_ERROR_FAILURE);

  
  
  

  nsIntSize cssSize(DevToCSSIntPixels(nsIntSize(width, height)));

  cssSize.width += aWidthDif;
  cssSize.height += aHeightDif;

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(&cssSize.width,
                                                &cssSize.height),
                    NS_ERROR_FAILURE);

  nsIntSize newDevSize(CSSToDevIntPixels(cssSize));

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetSize(newDevSize.width,
                                            newDevSize.height,
                                            PR_TRUE),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SizeToContent()
{
  FORWARD_TO_OUTER(SizeToContent, (), NS_ERROR_NOT_INITIALIZED);

  if (!mDocShell) {
    return NS_OK;
  }

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIContentViewer> cv;
  mDocShell->GetContentViewer(getter_AddRefs(cv));
  nsCOMPtr<nsIMarkupDocumentViewer> markupViewer(do_QueryInterface(cv));
  NS_ENSURE_TRUE(markupViewer, NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(markupViewer->SizeToContent(), NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetWindowRoot(nsIDOMEventTarget **aWindowRoot)
{
  *aWindowRoot = nsnull;

  nsIDOMWindowInternal *rootWindow = nsGlobalWindow::GetPrivateRoot();
  nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(rootWindow));
  if (!piWin) {
    return NS_OK;
  }

  nsPIDOMEventTarget *chromeHandler = piWin->GetChromeEventHandler();
  if (!chromeHandler) {
    return NS_OK;
  }

  return CallQueryInterface(chromeHandler, aWindowRoot);
}

NS_IMETHODIMP
nsGlobalWindow::Scroll(PRInt32 aXScroll, PRInt32 aYScroll)
{
  return ScrollTo(aXScroll, aYScroll);
}

NS_IMETHODIMP
nsGlobalWindow::ScrollTo(PRInt32 aXScroll, PRInt32 aYScroll)
{
  nsresult result;
  nsIScrollableView *view = nsnull;      

  FlushPendingNotifications(Flush_Layout);
  result = GetScrollInfo(&view);

  if (view) {
    
    
    
    
    
    const PRInt32 maxpx = nsPresContext::AppUnitsToIntCSSPixels(0x7fffffff) - 4;

    if (aXScroll > maxpx) {
      aXScroll = maxpx;
    }

    if (aYScroll > maxpx) {
      aYScroll = maxpx;
    }

    result = view->ScrollTo(nsPresContext::CSSPixelsToAppUnits(aXScroll),
                            nsPresContext::CSSPixelsToAppUnits(aYScroll),
                            0);
  }

  return result;
}

NS_IMETHODIMP
nsGlobalWindow::ScrollBy(PRInt32 aXScrollDif, PRInt32 aYScrollDif)
{
  nsresult result;
  nsIScrollableView *view = nsnull;      

  FlushPendingNotifications(Flush_Layout);
  result = GetScrollInfo(&view);

  if (view) {
    nscoord xPos, yPos;
    result = view->GetScrollPosition(xPos, yPos);
    if (NS_SUCCEEDED(result)) {
      result = ScrollTo(nsPresContext::AppUnitsToIntCSSPixels(xPos) + aXScrollDif,
                        nsPresContext::AppUnitsToIntCSSPixels(yPos) + aYScrollDif);
    }
  }

  return result;
}

NS_IMETHODIMP
nsGlobalWindow::ScrollByLines(PRInt32 numLines)
{
  nsresult result;
  nsIScrollableView *view = nsnull;   

  FlushPendingNotifications(Flush_Layout);
  result = GetScrollInfo(&view);
  if (view) {
    result = view->ScrollByLines(0, numLines);
  }

  return result;
}

NS_IMETHODIMP
nsGlobalWindow::ScrollByPages(PRInt32 numPages)
{
  nsresult result;
  nsIScrollableView *view = nsnull;   

  FlushPendingNotifications(Flush_Layout);
  result = GetScrollInfo(&view);
  if (view) {
    result = view->ScrollByPages(0, numPages);
  }

  return result;
}

NS_IMETHODIMP
nsGlobalWindow::ClearTimeout()
{
  return ClearTimeoutOrInterval();
}

NS_IMETHODIMP
nsGlobalWindow::ClearInterval()
{
  return ClearTimeoutOrInterval();
}

NS_IMETHODIMP
nsGlobalWindow::SetTimeout(PRInt32 *_retval)
{
  return SetTimeoutOrInterval(PR_FALSE, _retval);
}

NS_IMETHODIMP
nsGlobalWindow::SetInterval(PRInt32 *_retval)
{
  return SetTimeoutOrInterval(PR_TRUE, _retval);
}

NS_IMETHODIMP
nsGlobalWindow::SetResizable(PRBool aResizable)
{
  

  return NS_OK;
}

static void
ReportUseOfDeprecatedMethod(nsGlobalWindow* aWindow, const char* aWarning)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aWindow->GetExtantDocument());
  nsContentUtils::ReportToConsole(nsContentUtils::eDOM_PROPERTIES,
                                  aWarning,
                                  nsnull, 0,
                                  doc ? doc->GetDocumentURI() : nsnull,
                                  EmptyString(), 0, 0,
                                  nsIScriptError::warningFlag,
                                  "DOM Events");
}

NS_IMETHODIMP
nsGlobalWindow::CaptureEvents(PRInt32 aEventFlags)
{
  ReportUseOfDeprecatedMethod(this, "UseOfCaptureEventsWarning");
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::ReleaseEvents(PRInt32 aEventFlags)
{
  ReportUseOfDeprecatedMethod(this, "UseOfReleaseEventsWarning");
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::RouteEvent(nsIDOMEvent* aEvt)
{
  ReportUseOfDeprecatedMethod(this, "UseOfRouteEventWarning");
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::EnableExternalCapture()
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGlobalWindow::DisableExternalCapture()
{
  return NS_ERROR_FAILURE;
}

static
PRBool IsPopupBlocked(nsIDOMDocument* aDoc)
{
  nsCOMPtr<nsIPopupWindowManager> pm =
    do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);

  if (!pm) {
    return PR_FALSE;
  }

  PRBool blocked = PR_TRUE;
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(aDoc));

  if (doc) {
    PRUint32 permission = nsIPopupWindowManager::ALLOW_POPUP;
    pm->TestPermission(doc->GetDocumentURI(), &permission);
    blocked = (permission == nsIPopupWindowManager::DENY_POPUP);
  }
  return blocked;
}

static
void FirePopupBlockedEvent(nsIDOMDocument* aDoc,
                           nsIDOMWindow *aRequestingWindow, nsIURI *aPopupURI,
                           const nsAString &aPopupWindowName,
                           const nsAString &aPopupWindowFeatures)
{
  if (aDoc) {
    
    
    nsCOMPtr<nsIDOMDocumentEvent> docEvent(do_QueryInterface(aDoc));
    nsCOMPtr<nsIDOMEvent> event;
    docEvent->CreateEvent(NS_LITERAL_STRING("PopupBlockedEvents"),
                          getter_AddRefs(event));
    if (event) {
      nsCOMPtr<nsIDOMPopupBlockedEvent> pbev(do_QueryInterface(event));
      pbev->InitPopupBlockedEvent(NS_LITERAL_STRING("DOMPopupBlocked"),
                                  PR_TRUE, PR_TRUE, aRequestingWindow,
                                  aPopupURI, aPopupWindowName,
                                  aPopupWindowFeatures);
      nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
      privateEvent->SetTrusted(PR_TRUE);

      nsCOMPtr<nsIDOMEventTarget> targ(do_QueryInterface(aDoc));
      PRBool defaultActionEnabled;
      targ->DispatchEvent(event, &defaultActionEnabled);
    }
  }
}

void FirePopupWindowEvent(nsIDOMDocument* aDoc)
{
  
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(aDoc));
  nsContentUtils::DispatchTrustedEvent(doc, aDoc,
                                       NS_LITERAL_STRING("PopupWindow"),
                                       PR_TRUE, PR_TRUE);
}


PRBool
nsGlobalWindow::CanSetProperty(const char *aPrefName)
{
  
  if (nsContentUtils::IsCallerTrustedForWrite()) {
    return PR_TRUE;
  }

  
  
  return !nsContentUtils::GetBoolPref(aPrefName, PR_TRUE);
}








PopupControlState
nsGlobalWindow::CheckForAbusePoint()
{
  FORWARD_TO_OUTER(CheckForAbusePoint, (), openAbused);

  NS_ASSERTION(mDocShell, "Must have docshell");
  
  nsCOMPtr<nsIDocShellTreeItem> item(do_QueryInterface(mDocShell));

  NS_ASSERTION(item, "Docshell doesn't implement nsIDocShellTreeItem?");

  PRInt32 type = nsIDocShellTreeItem::typeChrome;
  item->GetItemType(&type);
  if (type != nsIDocShellTreeItem::typeContent)
    return openAllowed;

  
  
  PopupControlState abuse = gPopupControlState;

  
  if (abuse == openAbused || abuse == openControlled) {
    PRInt32 popupMax = nsContentUtils::GetIntPref("dom.popup_maximum", -1);
    if (popupMax >= 0 && gOpenPopupSpamCount >= popupMax)
      abuse = openOverridden;
  }

  return abuse;
}




OpenAllowValue
nsGlobalWindow::CheckOpenAllow(PopupControlState aAbuseLevel)
{
  NS_PRECONDITION(GetDocShell(), "Must have docshell");

  OpenAllowValue allowWindow = allowNoAbuse; 
  
  if (aAbuseLevel >= openAbused) {
    allowWindow = allowNot;

    
    
    
    
    
    
    if (aAbuseLevel == openAbused) {
      nsCOMPtr<nsIDOMWindow> topWindow;
      GetTop(getter_AddRefs(topWindow));

      nsCOMPtr<nsPIDOMWindow> topPIWin(do_QueryInterface(topWindow));

      if (topPIWin && (!IsPopupBlocked(topPIWin->GetExtantDocument()) ||
                       !IsPopupBlocked(mDocument))) {
        allowWindow = allowWhitelisted;
      }
    }
  }

  return allowWindow;
}





void
nsGlobalWindow::FireAbuseEvents(PRBool aBlocked, PRBool aWindow,
                                const nsAString &aPopupURL,
                                const nsAString &aPopupWindowName,
                                const nsAString &aPopupWindowFeatures)
{
  

  nsCOMPtr<nsIDOMWindow> topWindow;
  GetTop(getter_AddRefs(topWindow));
  if (!topWindow)
    return;

  nsCOMPtr<nsIDOMDocument> topDoc;
  topWindow->GetDocument(getter_AddRefs(topDoc));

  nsCOMPtr<nsIURI> popupURI;

  
  

  

  nsIURI *baseURL = 0;

  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  nsCOMPtr<nsIDOMWindow> contextWindow;

  if (cx) {
    nsIScriptContext *currentCX = nsJSUtils::GetDynamicScriptContext(cx);
    if (currentCX) {
      contextWindow = do_QueryInterface(currentCX->GetGlobalObject());
    }
  }
  if (!contextWindow)
    contextWindow = static_cast<nsIDOMWindow*>(this);

  nsCOMPtr<nsIDOMDocument> domdoc;
  contextWindow->GetDocument(getter_AddRefs(domdoc));
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domdoc));
  if (doc)
    baseURL = doc->GetBaseURI();

  
  nsCOMPtr<nsIIOService> ios(do_GetService(NS_IOSERVICE_CONTRACTID));
  if (ios)
    ios->NewURI(NS_ConvertUTF16toUTF8(aPopupURL), 0, baseURL,
                getter_AddRefs(popupURI));

  
  if (aBlocked)
    FirePopupBlockedEvent(topDoc, this, popupURI, aPopupWindowName,
                          aPopupWindowFeatures);
  if (aWindow)
    FirePopupWindowEvent(topDoc);
}

NS_IMETHODIMP
nsGlobalWindow::Open(const nsAString& aUrl, const nsAString& aName,
                     const nsAString& aOptions, nsIDOMWindow **_retval)
{
  return OpenInternal(aUrl, aName, aOptions,
                      PR_FALSE,          
                      PR_FALSE,          
                      PR_TRUE,           
                      PR_FALSE,          
                      nsnull, nsnull,    
                      GetPrincipal(),    
                      nsnull,            
                      _retval);
}

NS_IMETHODIMP
nsGlobalWindow::OpenJS(const nsAString& aUrl, const nsAString& aName,
                       const nsAString& aOptions, nsIDOMWindow **_retval)
{
  return OpenInternal(aUrl, aName, aOptions,
                      PR_FALSE,          
                      PR_FALSE,          
                      PR_FALSE,          
                      PR_TRUE,           
                      nsnull, nsnull,    
                      GetPrincipal(),    
                      nsContentUtils::GetCurrentJSContext(), 
                      _retval);
}



NS_IMETHODIMP
nsGlobalWindow::OpenDialog(const nsAString& aUrl, const nsAString& aName,
                           const nsAString& aOptions,
                           nsISupports* aExtraArgument, nsIDOMWindow** _retval)
{
  return OpenInternal(aUrl, aName, aOptions,
                      PR_TRUE,                    
                      PR_FALSE,                   
                      PR_TRUE,                    
                      PR_FALSE,                   
                      nsnull, aExtraArgument,     
                      GetPrincipal(),             
                      nsnull,                     
                      _retval);
}

NS_IMETHODIMP
nsGlobalWindow::OpenDialog(const nsAString& aUrl, const nsAString& aName,
                           const nsAString& aOptions, nsIDOMWindow** _retval)
{
  if (!nsContentUtils::IsCallerTrustedForWrite()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsAXPCNativeCallContext *ncc = nsnull;
  nsresult rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(&ncc);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  JSContext *cx = nsnull;

  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 argc;
  jsval *argv = nsnull;

  
  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  
  PRUint32 argOffset = argc < 3 ? argc : 3;
  nsCOMPtr<nsIArray> argvArray;
  rv = NS_CreateJSArgv(cx, argc - argOffset, argv + argOffset,
                       getter_AddRefs(argvArray));
  NS_ENSURE_SUCCESS(rv, rv);

  return OpenInternal(aUrl, aName, aOptions,
                      PR_TRUE,             
                      PR_FALSE,            
                      PR_FALSE,            
                      PR_FALSE,            
                      argvArray, nsnull,   
                      GetPrincipal(),      
                      cx,                  
                      _retval);
}

NS_IMETHODIMP
nsGlobalWindow::GetFrames(nsIDOMWindow** aFrames)
{
  FORWARD_TO_OUTER(GetFrames, (aFrames), NS_ERROR_NOT_INITIALIZED);

  *aFrames = this;
  NS_ADDREF(*aFrames);

  FlushPendingNotifications(Flush_ContentAndNotify);

  return NS_OK;
}

nsGlobalWindow*
nsGlobalWindow::CallerInnerWindow()
{
  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  if (!cx) {
    NS_ERROR("Please don't call this method from C++!");

    return nsnull;
  }

  JSObject *scope = ::JS_GetScopeChain(cx);
  if (!scope)
    return nsnull;

  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  nsContentUtils::XPConnect()->
    GetWrappedNativeOfJSObject(cx, ::JS_GetGlobalForObject(cx, scope),
                               getter_AddRefs(wrapper));
  if (!wrapper)
    return nsnull;

  
  
  
  nsCOMPtr<nsPIDOMWindow> win = do_QueryWrappedNative(wrapper);
  if (!win)
    return GetCurrentInnerWindowInternal();
  return static_cast<nsGlobalWindow*>(win.get());
}






class PostMessageEvent : public nsRunnable
{
  public:
    NS_DECL_NSIRUNNABLE

    PostMessageEvent(nsGlobalWindow* aSource,
                     const nsAString& aCallerOrigin,
                     const nsAString& aMessage,
                     nsGlobalWindow* aTargetWindow,
                     nsIURI* aProvidedOrigin,
                     PRBool aTrustedCaller)
    : mSource(aSource),
      mCallerOrigin(aCallerOrigin),
      mMessage(aMessage),
      mTargetWindow(aTargetWindow),
      mProvidedOrigin(aProvidedOrigin),
      mTrustedCaller(aTrustedCaller)
    {
      MOZ_COUNT_CTOR(PostMessageEvent);
    }
    
    ~PostMessageEvent()
    {
      MOZ_COUNT_DTOR(PostMessageEvent);
    }

  private:
    nsRefPtr<nsGlobalWindow> mSource;
    nsString mCallerOrigin;
    nsString mMessage;
    nsRefPtr<nsGlobalWindow> mTargetWindow;
    nsCOMPtr<nsIURI> mProvidedOrigin;
    PRBool mTrustedCaller;
};

NS_IMETHODIMP
PostMessageEvent::Run()
{
  NS_ABORT_IF_FALSE(mTargetWindow->IsOuterWindow(),
                    "should have been passed an outer window!");
  NS_ABORT_IF_FALSE(!mSource || mSource->IsOuterWindow(),
                    "should have been passed an outer window!");

  nsRefPtr<nsGlobalWindow> targetWindow =
    mTargetWindow->GetCurrentInnerWindowInternal();
  if (!targetWindow)
    return NS_OK;

  NS_ABORT_IF_FALSE(targetWindow->IsInnerWindow(),
                    "we ordered an inner window!");

  
  
  
  
  
  
  
  
  if (mProvidedOrigin) {
    
    
    
    nsIPrincipal* targetPrin = targetWindow->GetPrincipal();
    if (!targetPrin)
      return NS_OK;
    nsCOMPtr<nsIURI> targetURI;
    if (NS_FAILED(targetPrin->GetURI(getter_AddRefs(targetURI))))
      return NS_OK;
    if (!targetURI) {
      targetURI = targetWindow->mDoc->GetDocumentURI();
      if (!targetURI)
        return NS_OK;
    }

    
    
    
    
    
    nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
    nsresult rv =
      ssm->CheckSameOriginURI(mProvidedOrigin, targetURI, PR_TRUE);
    if (NS_FAILED(rv))
      return NS_OK;
  }


  
  nsCOMPtr<nsIDOMDocumentEvent> docEvent =
    do_QueryInterface(targetWindow->mDocument);
  if (!docEvent)
    return NS_OK;
  nsCOMPtr<nsIDOMEvent> event;
  docEvent->CreateEvent(NS_LITERAL_STRING("MessageEvent"),
                        getter_AddRefs(event));
  if (!event)
    return NS_OK;

  nsCOMPtr<nsIDOMMessageEvent> message = do_QueryInterface(event);
  nsresult rv = message->InitMessageEvent(NS_LITERAL_STRING("message"),
                                          PR_FALSE ,
                                          PR_TRUE ,
                                          mMessage,
                                          mCallerOrigin,
                                          EmptyString(),
                                          mSource);
  if (NS_FAILED(rv))
    return NS_OK;


  
  
  
  

  nsIPresShell *shell = targetWindow->mDoc->GetPrimaryShell();
  nsRefPtr<nsPresContext> presContext;
  if (shell)
    presContext = shell->GetPresContext();

  nsCOMPtr<nsIPrivateDOMEvent> privEvent = do_QueryInterface(message);
  privEvent->SetTrusted(mTrustedCaller);
  nsEvent *internalEvent = privEvent->GetInternalNSEvent();

  nsEventStatus status = nsEventStatus_eIgnore;
  nsEventDispatcher::Dispatch(static_cast<nsPIDOMWindow*>(mTargetWindow),
                              presContext,
                              internalEvent,
                              message,
                              &status);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::PostMessageMoz(const nsAString& aMessage, const nsAString& aOrigin)
{
  
  
  
  
  
  
  
  
  NS_ABORT_IF_FALSE(IsOuterWindow(), "only call this method on outer windows");

  
  
  
  
  
  
  

  
  nsRefPtr<nsGlobalWindow> callerInnerWin = CallerInnerWindow();
  if (!callerInnerWin)
    return NS_OK;
  NS_ABORT_IF_FALSE(callerInnerWin->IsInnerWindow(),
                    "should have gotten an inner window here");

  
  
  
  
  
  
  nsIPrincipal* callerPrin = callerInnerWin->GetPrincipal();
  if (!callerPrin)
    return NS_OK;
  
  nsCOMPtr<nsIURI> callerOuterURI;
  if (NS_FAILED(callerPrin->GetURI(getter_AddRefs(callerOuterURI))))
    return NS_OK;

  nsAutoString origin;
  if (callerOuterURI) {
    
    nsContentUtils::GetUTFOrigin(callerPrin, origin);
  }
  else {
    
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(callerInnerWin->mDocument);
    if (!doc)
      return NS_OK;
    callerOuterURI = doc->GetDocumentURI();
    
    nsContentUtils::GetUTFOrigin(callerOuterURI, origin);
  }

  
  
  nsCOMPtr<nsIURI> providedOrigin;
  if (!aOrigin.EqualsASCII("*")) {
    if (NS_FAILED(NS_NewURI(getter_AddRefs(providedOrigin), aOrigin)))
      return NS_ERROR_DOM_SYNTAX_ERR;
    if (NS_FAILED(providedOrigin->SetUserPass(EmptyCString())) ||
        NS_FAILED(providedOrigin->SetPath(EmptyCString())))
      return NS_OK;
  }

  
  
  nsRefPtr<PostMessageEvent> event =
    new PostMessageEvent(nsContentUtils::IsCallerChrome()
                         ? nsnull
                         : callerInnerWin->GetOuterWindowInternal(),
                         origin,
                         aMessage,
                         this,
                         providedOrigin,
                         nsContentUtils::IsCallerTrustedForWrite());
  return NS_DispatchToCurrentThread(event);
}

class nsCloseEvent : public nsRunnable {
public:
  nsCloseEvent (nsGlobalWindow *aWindow)
    : mWindow(aWindow)
  {
  }
 
  NS_IMETHOD Run() {
    if (mWindow)
      mWindow->ReallyCloseWindow();
    return NS_OK;
  }

  nsRefPtr<nsGlobalWindow> mWindow;
};

NS_IMETHODIMP
nsGlobalWindow::Close()
{
  FORWARD_TO_OUTER(Close, (), NS_ERROR_NOT_INITIALIZED);

  if (IsFrame() || !mDocShell || IsInModalState()) {
    
    
    

    return NS_OK;
  }

  if (mHavePendingClose) {
    
    
    return NS_OK;
  }

  if (mBlockScriptedClosingFlag)
  {
    
    
    
    return NS_OK;
  }

  
  
  nsresult rv = NS_OK;
  if (!mHadOriginalOpener && !nsContentUtils::IsCallerTrustedForWrite()) {
    PRBool allowClose =
      nsContentUtils::GetBoolPref("dom.allow_scripts_to_close_windows",
                                  PR_TRUE);
    if (!allowClose) {
      
      
      nsContentUtils::ReportToConsole(
          nsContentUtils::eDOM_PROPERTIES,
          "WindowCloseBlockedWarning",
          nsnull, 0, 
          nsnull, 
                  
          EmptyString(), 0, 0, 
          nsIScriptError::warningFlag,
          "DOM Window");  

      return NS_OK;
    }
  }

  
  
  
  

  nsCOMPtr<nsIContentViewer> cv;
  mDocShell->GetContentViewer(getter_AddRefs(cv));
  if (!mInClose && !mIsClosed && cv) {
    PRBool canClose;

    rv = cv->PermitUnload(&canClose);
    if (NS_SUCCEEDED(rv) && !canClose)
      return NS_OK;

    rv = cv->RequestWindowClose(&canClose);
    if (NS_SUCCEEDED(rv) && !canClose)
      return NS_OK;
  }

  
  
  
  
  
  

  PRBool wasInClose = mInClose;
  mInClose = PR_TRUE;

  if (!DispatchCustomEvent("DOMWindowClose")) {
    
    

    mInClose = wasInClose;
    return NS_OK;
  }

  
  mIsClosed = PR_TRUE;

  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService(sJSStackContractID);

  JSContext *cx = nsnull;

  if (stack) {
    stack->Peek(&cx);
  }

  if (cx) {
    nsIScriptContext *currentCX = nsJSUtils::GetDynamicScriptContext(cx);

    if (currentCX && currentCX == mContext) {
      
      
      
      
      rv = currentCX->SetTerminationFunction(CloseWindow,
                                             static_cast<nsIDOMWindow *>
                                                        (this));
      if (NS_SUCCEEDED(rv)) {
        mHavePendingClose = PR_TRUE;
      }
      return NS_OK;
    }
  }

  
  
  
  
  
  rv = NS_ERROR_FAILURE;
  if (!nsContentUtils::IsCallerChrome()) {
    nsCOMPtr<nsIRunnable> ev = new nsCloseEvent(this);
    rv = NS_DispatchToCurrentThread(ev);
  }
  
  if (NS_FAILED(rv)) {
    ReallyCloseWindow();
    rv = NS_OK;
  } else {
    mHavePendingClose = PR_TRUE;
  }
  
  return rv;
}


void
nsGlobalWindow::ReallyCloseWindow()
{
  FORWARD_TO_OUTER_VOID(ReallyCloseWindow, ());

  
  mHavePendingClose = PR_TRUE;

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));

  

  if (treeOwnerAsWin) {

    
    

    nsCOMPtr<nsIDocShellTreeItem> docItem(do_QueryInterface(mDocShell));
    if (docItem) {
      nsCOMPtr<nsIBrowserDOMWindow> bwin;
      nsCOMPtr<nsIDocShellTreeItem> rootItem;
      docItem->GetRootTreeItem(getter_AddRefs(rootItem));
      nsCOMPtr<nsIDOMWindow> rootWin(do_GetInterface(rootItem));
      nsCOMPtr<nsIDOMChromeWindow> chromeWin(do_QueryInterface(rootWin));
      if (chromeWin)
        chromeWin->GetBrowserDOMWindow(getter_AddRefs(bwin));

      if (rootWin) {
        









        
        PRBool isTab = PR_FALSE;
        if (rootWin == this ||
            !bwin || (bwin->IsTabContentWindow(GetOuterWindowInternal(),
                                               &isTab), isTab))
          treeOwnerAsWin->Destroy();
      }
    }

    CleanUp();
  }
}

void
nsGlobalWindow::EnterModalState()
{
  nsCOMPtr<nsIDOMWindow> top;
  GetTop(getter_AddRefs(top));

  if (!top) {
    NS_ERROR("Uh, EnterModalState() called w/o a reachable top window?");

    return;
  }

  nsGlobalWindow* topWin =
    static_cast<nsGlobalWindow*>(static_cast<nsIDOMWindow *>(top.get()));
  if (topWin->mModalStateDepth == 0) {
    NS_ASSERTION(!mSuspendedDoc, "Shouldn't have mSuspendedDoc here!");

    mSuspendedDoc = do_QueryInterface(topWin->GetExtantDocument());
    if (mSuspendedDoc && mSuspendedDoc->EventHandlingSuppressed()) {
      mSuspendedDoc->SuppressEventHandling();
    } else {
      mSuspendedDoc = nsnull;
    }
  }
  topWin->mModalStateDepth++;

  JSContext *cx = nsContentUtils::GetCurrentJSContext();

  nsIScriptContext *scx;
  if (cx && (scx = GetScriptContextFromJSContext(cx))) {
    scx->EnterModalState();
  }
}


void
nsGlobalWindow::RunPendingTimeoutsRecursive(nsGlobalWindow *aTopWindow,
                                            nsGlobalWindow *aWindow)
{
  nsGlobalWindow *inner;

  
  if (!(inner = aWindow->GetCurrentInnerWindowInternal()) ||
      inner->IsFrozen()) {
    return;
  }

  inner->RunTimeout(nsnull);

  
  
  if (inner->IsFrozen()) {
    return;
  }

  nsCOMPtr<nsIDOMWindowCollection> frames;
  aWindow->GetFrames(getter_AddRefs(frames));

  if (!frames) {
    return;
  }

  PRUint32 i, length;
  if (NS_FAILED(frames->GetLength(&length)) || !length) {
    return;
  }

  for (i = 0; i < length && aTopWindow->mModalStateDepth == 0; i++) {
    nsCOMPtr<nsIDOMWindow> child;
    frames->Item(i, getter_AddRefs(child));

    if (!child) {
      return;
    }

    nsGlobalWindow *childWin =
      static_cast<nsGlobalWindow *>
                 (static_cast<nsIDOMWindow *>
                             (child.get()));

    RunPendingTimeoutsRecursive(aTopWindow, childWin);
  }
}

class nsPendingTimeoutRunner : public nsRunnable
{
public:
  nsPendingTimeoutRunner(nsGlobalWindow *aWindow)
    : mWindow(aWindow)
  {
    NS_ASSERTION(mWindow, "mWindow is null.");
  }

  NS_IMETHOD Run()
  {
    nsGlobalWindow::RunPendingTimeoutsRecursive(mWindow, mWindow);

    return NS_OK;
  }

private:
  nsRefPtr<nsGlobalWindow> mWindow;
};

void
nsGlobalWindow::LeaveModalState()
{
  nsCOMPtr<nsIDOMWindow> top;
  GetTop(getter_AddRefs(top));

  if (!top) {
    NS_ERROR("Uh, LeaveModalState() called w/o a reachable top window?");

    return;
  }

  nsGlobalWindow *topWin =
    static_cast<nsGlobalWindow *>
               (static_cast<nsIDOMWindow *>
                           (top.get()));

  topWin->mModalStateDepth--;

  if (topWin->mModalStateDepth == 0) {
    nsCOMPtr<nsIRunnable> runner = new nsPendingTimeoutRunner(topWin);
    if (NS_FAILED(NS_DispatchToCurrentThread(runner)))
      NS_WARNING("failed to dispatch pending timeout runnable");

    if (mSuspendedDoc) {
      nsCOMPtr<nsIDocument> currentDoc =
        do_QueryInterface(topWin->GetExtantDocument());
      mSuspendedDoc->UnsuppressEventHandlingAndFireEvents(currentDoc == mSuspendedDoc);
      mSuspendedDoc = nsnull;
    }
  }

  JSContext *cx = nsContentUtils::GetCurrentJSContext();

  nsIScriptContext *scx;
  if (cx && (scx = GetScriptContextFromJSContext(cx))) {
    scx->LeaveModalState();
  }
}

PRBool
nsGlobalWindow::IsInModalState()
{
  nsCOMPtr<nsIDOMWindow> top;
  GetTop(getter_AddRefs(top));

  if (!top) {
    NS_ERROR("Uh, IsInModalState() called w/o a reachable top window?");

    return PR_FALSE;
  }

  return static_cast<nsGlobalWindow *>
                    (static_cast<nsIDOMWindow *>
                                (top.get()))->mModalStateDepth != 0;
}


void
nsGlobalWindow::NotifyDOMWindowDestroyed(nsGlobalWindow* aWindow) {
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
  if (observerService) {
    observerService->
      NotifyObservers(static_cast<nsIScriptGlobalObject*>(aWindow),
                      DOM_WINDOW_DESTROYED_TOPIC, nsnull);
  }
}

void
nsGlobalWindow::InitJavaProperties()
{
  nsIScriptContext *scx = GetContextInternal();

  if (mDidInitJavaProperties || IsOuterWindow() || !scx || !mJSObject) {
    return;
  }

  
  
  mDidInitJavaProperties = PR_TRUE;

  
  

  nsCOMPtr<nsIPluginHost> host(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID));
  if (!host) {
    return;
  }

  mDummyJavaPluginOwner = new nsDummyJavaPluginOwner(mDoc);
  if (!mDummyJavaPluginOwner) {
    return;
  }

  host->InstantiateDummyJavaPlugin(mDummyJavaPluginOwner);

  
  
  
  
  if (!mDummyJavaPluginOwner) {
    return;
  }

  nsCOMPtr<nsIPluginInstance> dummyPlugin;
  mDummyJavaPluginOwner->GetInstance(*getter_AddRefs(dummyPlugin));

  if (dummyPlugin) {
    
    
    
    

    return;
  }

  
  
  mDummyJavaPluginOwner = nsnull;
}

void*
nsGlobalWindow::GetCachedXBLPrototypeHandler(nsXBLPrototypeHandler* aKey)
{
  void* handler = nsnull;
  if (mCachedXBLPrototypeHandlers.IsInitialized()) {
    mCachedXBLPrototypeHandlers.Get(aKey, &handler);
  }
  return handler;
}

void
nsGlobalWindow::CacheXBLPrototypeHandler(nsXBLPrototypeHandler* aKey,
                                         nsScriptObjectHolder& aHandler)
{
  if (!mCachedXBLPrototypeHandlers.IsInitialized() &&
      !mCachedXBLPrototypeHandlers.Init()) {
    NS_ERROR("Failed to initiailize hashtable!");
    return;
  }

  if (!mCachedXBLPrototypeHandlers.Count()) {
    
    
    nsXPCOMCycleCollectionParticipant* participant;
    CallQueryInterface(this, &participant);
    NS_ASSERTION(participant,
                 "Failed to QI to nsXPCOMCycleCollectionParticipant!");

    nsCOMPtr<nsISupports> thisSupports;
    QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                   getter_AddRefs(thisSupports));
    NS_ASSERTION(thisSupports, "Failed to QI to nsCycleCollectionISupports!");

    nsresult rv = nsContentUtils::HoldJSObjects(thisSupports, participant);
    if (NS_FAILED(rv)) {
      NS_ERROR("nsContentUtils::HoldJSObjects failed!");
      return;
    }
  }

  mCachedXBLPrototypeHandlers.Put(aKey, aHandler);
}

NS_IMETHODIMP
nsGlobalWindow::GetFrameElement(nsIDOMElement** aFrameElement)
{
  FORWARD_TO_OUTER(GetFrameElement, (aFrameElement), NS_ERROR_NOT_INITIALIZED);

  *aFrameElement = nsnull;

  nsCOMPtr<nsIDocShellTreeItem> docShellTI(do_QueryInterface(mDocShell));

  if (!docShellTI) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocShellTreeItem> parent;
  docShellTI->GetSameTypeParent(getter_AddRefs(parent));

  if (!parent || parent == docShellTI) {
    
    

    return NS_OK;
  }

  *aFrameElement = mFrameElement;
  NS_IF_ADDREF(*aFrameElement);

  return NS_OK;
}





void
ConvertDialogOptions(const nsAString& aOptions, nsAString& aResult)
{
  nsAString::const_iterator end;
  aOptions.EndReading(end);

  nsAString::const_iterator iter;
  aOptions.BeginReading(iter);

  while (iter != end) {
    
    while (nsCRT::IsAsciiSpace(*iter) && iter != end) {
      ++iter;
    }

    nsAString::const_iterator name_start = iter;

    
    while (iter != end && !nsCRT::IsAsciiSpace(*iter) &&
           *iter != ';' &&
           *iter != ':' &&
           *iter != '=') {
      ++iter;
    }

    nsAString::const_iterator name_end = iter;

    
    while (nsCRT::IsAsciiSpace(*iter) && iter != end) {
      ++iter;
    }

    if (*iter == ';') {
      
      ++iter;

      continue;
    }

    nsAString::const_iterator value_start = iter;
    nsAString::const_iterator value_end = iter;

    if (*iter == ':' || *iter == '=') {
      

      iter++; 

      
      while (nsCRT::IsAsciiSpace(*iter) && iter != end) {
        ++iter;
      }

      value_start = iter;

      
      while (iter != end && !nsCRT::IsAsciiSpace(*iter) &&
             *iter != ';') {
        ++iter;
      }

      value_end = iter;

      
      while (nsCRT::IsAsciiSpace(*iter) && iter != end) {
        ++iter;
      }
    }

    const nsDependentSubstring& name = Substring(name_start, name_end);
    const nsDependentSubstring& value = Substring(value_start, value_end);

    if (name.LowerCaseEqualsLiteral("center")) {
      if (value.LowerCaseEqualsLiteral("on")  ||
          value.LowerCaseEqualsLiteral("yes") ||
          value.LowerCaseEqualsLiteral("1")) {
        aResult.AppendLiteral(",centerscreen=1");
      }
    } else if (name.LowerCaseEqualsLiteral("dialogwidth")) {
      if (!value.IsEmpty()) {
        aResult.AppendLiteral(",width=");
        aResult.Append(value);
      }
    } else if (name.LowerCaseEqualsLiteral("dialogheight")) {
      if (!value.IsEmpty()) {
        aResult.AppendLiteral(",height=");
        aResult.Append(value);
      }
    } else if (name.LowerCaseEqualsLiteral("dialogtop")) {
      if (!value.IsEmpty()) {
        aResult.AppendLiteral(",top=");
        aResult.Append(value);
      }
    } else if (name.LowerCaseEqualsLiteral("dialogleft")) {
      if (!value.IsEmpty()) {
        aResult.AppendLiteral(",left=");
        aResult.Append(value);
      }
    } else if (name.LowerCaseEqualsLiteral("resizable")) {
      if (value.LowerCaseEqualsLiteral("on")  ||
          value.LowerCaseEqualsLiteral("yes") ||
          value.LowerCaseEqualsLiteral("1")) {
        aResult.AppendLiteral(",resizable=1");
      }
    } else if (name.LowerCaseEqualsLiteral("scroll")) {
      if (value.LowerCaseEqualsLiteral("off")  ||
          value.LowerCaseEqualsLiteral("no") ||
          value.LowerCaseEqualsLiteral("0")) {
        aResult.AppendLiteral(",scrollbars=0");
      }
    }

    if (iter == end) {
      break;
    }

    iter++;
  }
}

NS_IMETHODIMP
nsGlobalWindow::ShowModalDialog(const nsAString& aURI, nsIVariant *aArgs,
                                const nsAString& aOptions,
                                nsIVariant **aRetVal)
{
  *aRetVal = nsnull;

  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMWindow> dlgWin;
  nsAutoString options(NS_LITERAL_STRING("-moz-internal-modal=1,status=1"));

  ConvertDialogOptions(aOptions, options);

  options.AppendLiteral(",scrollbars=1,centerscreen=1,resizable=0");

  
  
  EnsureReflowFlushAndPaint();

  nsresult rv = OpenInternal(aURI, EmptyString(), options,
                             PR_FALSE,          
                             PR_TRUE,           
                             PR_TRUE,           
                             PR_TRUE,           
                             nsnull, aArgs,     
                             GetPrincipal(),    
                             nsnull,            
                             getter_AddRefs(dlgWin));

  NS_ENSURE_SUCCESS(rv, rv);
  
  if (dlgWin) {
    nsCOMPtr<nsIPrincipal> subjectPrincipal;
    rv = nsContentUtils::GetSecurityManager()->
      GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
    if (NS_FAILED(rv)) {
      return rv;
    }

    PRBool canAccess = PR_TRUE;

    if (subjectPrincipal) {
      nsCOMPtr<nsIScriptObjectPrincipal> objPrincipal =
        do_QueryInterface(dlgWin);
      nsCOMPtr<nsIPrincipal> dialogPrincipal;

      if (objPrincipal) {
        dialogPrincipal = objPrincipal->GetPrincipal();

        rv = subjectPrincipal->Subsumes(dialogPrincipal, &canAccess);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        
        

        canAccess = PR_FALSE;
      }
    }

    if (canAccess) {
      nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(dlgWin));
      nsPIDOMWindow *inner = win->GetCurrentInnerWindow();

      nsCOMPtr<nsIDOMModalContentWindow> dlgInner(do_QueryInterface(inner));

      if (dlgInner) {
        dlgInner->GetReturnValue(aRetVal);
      }
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::UpdateCommands(const nsAString& anAction)
{
  nsPIDOMWindow *rootWindow = nsGlobalWindow::GetPrivateRoot();
  if (!rootWindow)
    return NS_OK;

  nsCOMPtr<nsIDOMXULDocument> xulDoc =
    do_QueryInterface(rootWindow->GetExtantDocument());
  
  if (xulDoc) {
    
    nsCOMPtr<nsIDOMXULCommandDispatcher> xulCommandDispatcher;
    xulDoc->GetCommandDispatcher(getter_AddRefs(xulCommandDispatcher));
    xulCommandDispatcher->UpdateCommands(anAction);
  }

  return NS_OK;
}

PRBool
nsGlobalWindow::GetBlurSuppression()
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  PRBool suppress = PR_FALSE;
  if (treeOwnerAsWin)
    treeOwnerAsWin->GetBlurSuppression(&suppress);
  return suppress;
}

NS_IMETHODIMP
nsGlobalWindow::GetSelection(nsISelection** aSelection)
{
  FORWARD_TO_OUTER(GetSelection, (aSelection), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aSelection);
  *aSelection = nsnull;

  if (!mDocShell)
    return NS_OK;

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));

  if (!presShell)
    return NS_OK;
    
  *aSelection = presShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);
  
  NS_IF_ADDREF(*aSelection);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::Find(const nsAString& aStr, PRBool aCaseSensitive,
                     PRBool aBackwards, PRBool aWrapAround, PRBool aWholeWord,
                     PRBool aSearchInFrames, PRBool aShowDialog,
                     PRBool *aDidFind)
{
  FORWARD_TO_OUTER(Find, (aStr, aCaseSensitive, aBackwards, aWrapAround,
                          aWholeWord, aSearchInFrames, aShowDialog, aDidFind),
                   NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_OK;
  *aDidFind = PR_FALSE;

  nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mDocShell));
  NS_ENSURE_TRUE(finder, NS_ERROR_FAILURE);

  
  rv = finder->SetSearchString(PromiseFlatString(aStr).get());
  NS_ENSURE_SUCCESS(rv, rv);
  finder->SetMatchCase(aCaseSensitive);
  finder->SetFindBackwards(aBackwards);
  finder->SetWrapFind(aWrapAround);
  finder->SetEntireWord(aWholeWord);
  finder->SetSearchFrames(aSearchInFrames);

  
  
  
  
  nsCOMPtr<nsIWebBrowserFindInFrames> framesFinder(do_QueryInterface(finder));
  if (framesFinder) {
    framesFinder->SetRootSearchFrame(this);   
    framesFinder->SetCurrentSearchFrame(this);
  }
  
  
  if (aStr.IsEmpty() || aShowDialog) {
    
    nsCOMPtr<nsIWindowMediator> windowMediator =
      do_GetService(NS_WINDOWMEDIATOR_CONTRACTID);

    nsCOMPtr<nsIDOMWindowInternal> findDialog;

    if (windowMediator) {
      windowMediator->GetMostRecentWindow(NS_LITERAL_STRING("findInPage").get(),
                                          getter_AddRefs(findDialog));
    }

    if (findDialog) {
      
      rv = findDialog->Focus();
    } else { 
      if (finder) {
        nsCOMPtr<nsIDOMWindow> dialog;
        rv = OpenDialog(NS_LITERAL_STRING("chrome://global/content/finddialog.xul"),
                        NS_LITERAL_STRING("_blank"),
                        NS_LITERAL_STRING("chrome, resizable=no, dependent=yes"),
                        finder, getter_AddRefs(dialog));
      }
    }
  } else {
    
    rv = finder->FindNext(aDidFind);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

static PRBool
Is8bit(const nsAString& aString)
{
  static const PRUnichar EIGHT_BIT = PRUnichar(~0x00FF);

  nsAString::const_iterator done_reading;
  aString.EndReading(done_reading);

  
  PRUint32 fragmentLength = 0;
  nsAString::const_iterator iter;
  for (aString.BeginReading(iter); iter != done_reading;
       iter.advance(PRInt32(fragmentLength))) {
    fragmentLength = PRUint32(iter.size_forward());
    const PRUnichar* c = iter.get();
    const PRUnichar* fragmentEnd = c + fragmentLength;

    
    while (c < fragmentEnd)
      if (*c++ & EIGHT_BIT)
        return PR_FALSE;
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsGlobalWindow::Atob(const nsAString& aAsciiBase64String,
                     nsAString& aBinaryData)
{
  aBinaryData.Truncate();

  if (!Is8bit(aAsciiBase64String)) {
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  PRUint32 dataLen = aAsciiBase64String.Length();

  NS_LossyConvertUTF16toASCII base64(aAsciiBase64String);
  if (base64.Length() != dataLen) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRInt32 resultLen = dataLen;
  if (!base64.IsEmpty() && base64[dataLen - 1] == '=') {
    if (base64.Length() > 1 && base64[dataLen - 2] == '=') {
      resultLen = dataLen - 2;
    } else {
      resultLen = dataLen - 1;
    }
  }

  resultLen = ((resultLen * 3) / 4);
  
  
  
  
  
  char *dest = static_cast<char *>(nsMemory::Alloc(resultLen + 4));
  if (!dest) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  char *bin_data = PL_Base64Decode(base64.get(), dataLen, dest);

  nsresult rv = NS_OK;

  if (bin_data) {
    CopyASCIItoUTF16(Substring(bin_data, bin_data + resultLen), aBinaryData);
  } else {
    rv = NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  nsMemory::Free(dest);

  return rv;
}

NS_IMETHODIMP
nsGlobalWindow::Btoa(const nsAString& aBinaryData,
                     nsAString& aAsciiBase64String)
{
  aAsciiBase64String.Truncate();

  if (!Is8bit(aBinaryData)) {
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  char *bin_data = ToNewCString(aBinaryData);
  if (!bin_data) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRInt32 resultLen = ((aBinaryData.Length() + 2) / 3) * 4;

  char *base64 = PL_Base64Encode(bin_data, aBinaryData.Length(), nsnull);
  if (!base64) {
    nsMemory::Free(bin_data);

    return NS_ERROR_OUT_OF_MEMORY;
  }

  CopyASCIItoUTF16(nsDependentCString(base64, resultLen), aAsciiBase64String);

  PR_Free(base64);
  nsMemory::Free(bin_data);

  return NS_OK;
}





NS_IMETHODIMP
nsGlobalWindow::AddEventListener(const nsAString& aType,
                                 nsIDOMEventListener* aListener,
                                 PRBool aUseCapture)
{
  FORWARD_TO_INNER_CREATE(AddEventListener, (aType, aListener, aUseCapture),
                          NS_ERROR_NOT_AVAILABLE);

  return AddEventListener(aType, aListener, aUseCapture,
                          !nsContentUtils::IsChromeDoc(mDoc));
}

NS_IMETHODIMP
nsGlobalWindow::RemoveEventListener(const nsAString& aType,
                                    nsIDOMEventListener* aListener,
                                    PRBool aUseCapture)
{
  return RemoveGroupedEventListener(aType, aListener, aUseCapture, nsnull);
}

NS_IMETHODIMP
nsGlobalWindow::DispatchEvent(nsIDOMEvent* aEvent, PRBool* _retval)
{
  FORWARD_TO_INNER(DispatchEvent, (aEvent, _retval), NS_OK);

  if (!mDoc) {
    return NS_ERROR_FAILURE;
  }

  
  nsIPresShell *shell = mDoc->GetPrimaryShell();
  nsCOMPtr<nsPresContext> presContext;
  if (shell) {
    
    presContext = shell->GetPresContext();
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  nsresult rv =
    nsEventDispatcher::DispatchDOMEvent(GetOuterWindow(), nsnull, aEvent,
                                        presContext, &status);

  *_retval = (status != nsEventStatus_eConsumeNoDefault);
  return rv;
}





NS_IMETHODIMP
nsGlobalWindow::AddGroupedEventListener(const nsAString & aType,
                                        nsIDOMEventListener *aListener,
                                        PRBool aUseCapture,
                                        nsIDOMEventGroup *aEvtGrp)
{
  FORWARD_TO_INNER_CREATE(AddGroupedEventListener,
                          (aType, aListener, aUseCapture, aEvtGrp),
                          NS_ERROR_NOT_AVAILABLE);

  nsIEventListenerManager* manager = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(manager);
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
  return manager->AddEventListenerByType(aListener, aType, flags, aEvtGrp);
}

NS_IMETHODIMP
nsGlobalWindow::RemoveGroupedEventListener(const nsAString & aType,
                                           nsIDOMEventListener *aListener,
                                           PRBool aUseCapture,
                                           nsIDOMEventGroup *aEvtGrp)
{
  FORWARD_TO_INNER(RemoveGroupedEventListener,
                   (aType, aListener, aUseCapture, aEvtGrp),
                   NS_ERROR_NOT_INITIALIZED);

  if (mListenerManager) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    mListenerManager->RemoveEventListenerByType(aListener, aType, flags,
                                                aEvtGrp);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGlobalWindow::CanTrigger(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsGlobalWindow::IsRegisteredHere(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsGlobalWindow::AddEventListener(const nsAString& aType,
                                 nsIDOMEventListener *aListener,
                                 PRBool aUseCapture, PRBool aWantsUntrusted)
{
  nsIEventListenerManager* manager = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(manager);

  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

  if (aWantsUntrusted) {
    flags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;
  }

  return manager->AddEventListenerByType(aListener, aType, flags, nsnull);
}

nsresult
nsGlobalWindow::AddEventListenerByIID(nsIDOMEventListener* aListener,
                                      const nsIID& aIID)
{
  nsIEventListenerManager* manager = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(manager);
  return manager->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
}

nsresult
nsGlobalWindow::RemoveEventListenerByIID(nsIDOMEventListener* aListener,
                                         const nsIID& aIID)
{
  FORWARD_TO_INNER(RemoveEventListenerByIID, (aListener, aIID),
                   NS_ERROR_NOT_INITIALIZED);

  if (mListenerManager) {
    mListenerManager->RemoveEventListenerByIID(aListener, aIID,
                                               NS_EVENT_FLAG_BUBBLE);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsIEventListenerManager*
nsGlobalWindow::GetListenerManager(PRBool aCreateIfNotFound)
{
  FORWARD_TO_INNER_CREATE(GetListenerManager, (aCreateIfNotFound), nsnull);

  if (!mListenerManager) {
    if (!aCreateIfNotFound) {
      return nsnull;
    }

    static NS_DEFINE_CID(kEventListenerManagerCID,
                         NS_EVENTLISTENERMANAGER_CID);

    mListenerManager = do_CreateInstance(kEventListenerManagerCID);
    if (mListenerManager) {
      mListenerManager->SetListenerTarget(
        static_cast<nsPIDOMEventTarget*>(this));
    }
  }

  return mListenerManager;
}

nsresult
nsGlobalWindow::GetSystemEventGroup(nsIDOMEventGroup **aGroup)
{
  nsIEventListenerManager* manager = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(manager);
  return manager->GetSystemEventGroupLM(aGroup);
}

nsIScriptContext*
nsGlobalWindow::GetContextForEventHandlers(nsresult* aRv)
{
  nsIScriptContext* scx = GetContext();
  *aRv = scx ? NS_OK : NS_ERROR_UNEXPECTED;
  return scx;
}





nsPIDOMWindow*
nsGlobalWindow::GetPrivateParent()
{
  FORWARD_TO_OUTER(GetPrivateParent, (), nsnull);

  nsCOMPtr<nsIDOMWindow> parent;
  GetParent(getter_AddRefs(parent));

  if (static_cast<nsIDOMWindow *>(this) == parent.get()) {
    nsCOMPtr<nsIContent> chromeElement(do_QueryInterface(mChromeEventHandler));
    if (!chromeElement)
      return nsnull;             

    nsIDocument* doc = chromeElement->GetDocument();
    if (!doc)
      return nsnull;             

    nsIScriptGlobalObject *globalObject = doc->GetScriptGlobalObject();
    if (!globalObject)
      return nsnull;             

    parent = do_QueryInterface(globalObject);
  }

  if (parent) {
    return static_cast<nsGlobalWindow *>
                      (static_cast<nsIDOMWindow*>(parent.get()));
  }

  return nsnull;
}

nsPIDOMWindow*
nsGlobalWindow::GetPrivateRoot()
{
  FORWARD_TO_OUTER(GetPrivateRoot, (), nsnull);

  nsCOMPtr<nsIDOMWindow> top;
  GetTop(getter_AddRefs(top));

  nsCOMPtr<nsPIDOMWindow> ptop = do_QueryInterface(top);
  NS_ASSERTION(ptop, "cannot get ptop");
  if (!ptop)
    return nsnull;

  nsIDocShell *docShell = ptop->GetDocShell();

  
  
  
  nsCOMPtr<nsIDOMEventTarget> chromeEventHandler;
  docShell->GetChromeEventHandler(getter_AddRefs(chromeEventHandler));

  nsCOMPtr<nsIContent> chromeElement(do_QueryInterface(mChromeEventHandler));
  if (chromeElement) {
    nsIDocument* doc = chromeElement->GetDocument();
    if (doc) {
      nsIDOMWindow *parent = doc->GetWindow();
      if (parent) {
        parent->GetTop(getter_AddRefs(top));
      }
    }
  }

  return static_cast<nsGlobalWindow *>
                    (static_cast<nsIDOMWindow *>(top));
}


NS_IMETHODIMP
nsGlobalWindow::GetLocation(nsIDOMLocation ** aLocation)
{
  FORWARD_TO_OUTER(GetLocation, (aLocation), NS_ERROR_NOT_INITIALIZED);

  *aLocation = nsnull;

  if (!mLocation && mDocShell) {
    mLocation = new nsLocation(mDocShell);
    if (!mLocation) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_IF_ADDREF(*aLocation = mLocation);

  return NS_OK;
}

void
nsGlobalWindow::ActivateOrDeactivate(PRBool aActivate)
{
  
  
  nsCOMPtr<nsIWidget> mainWidget = GetMainWidget();
  if (mainWidget) {
    
    
    nsCOMPtr<nsIWidget> topLevelWidget = mainWidget->GetSheetWindowParent();
    if (!topLevelWidget)
      topLevelWidget = mainWidget;

    
    nsCOMPtr<nsIDOMWindowInternal> topLevelWindow;
    if (topLevelWidget == mainWidget) {
      topLevelWindow = static_cast<nsIDOMWindowInternal *>(this);
    } else {
      
      
      
      
      void* clientData;
      topLevelWidget->GetClientData(clientData); 
      nsISupports* data = static_cast<nsISupports*>(clientData);
      nsCOMPtr<nsIInterfaceRequestor> req(do_QueryInterface(data));
      topLevelWindow = do_GetInterface(req);
    }

    if (topLevelWindow) {
      
      
      nsCOMPtr<nsIDOMDocument> domDoc;
      topLevelWindow->GetDocument(getter_AddRefs(domDoc));
      nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
      nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(doc));
      nsCOMPtr<nsIDOMChromeWindow> chromeWin = do_QueryInterface(topLevelWindow);
      if (xulDoc && chromeWin) {
        nsCOMPtr<nsIContent> rootElem = doc->GetRootContent();
        if (rootElem) {
          if (aActivate)
            rootElem->SetAttr(kNameSpaceID_None, nsGkAtoms::active,
                              NS_LITERAL_STRING("true"), PR_TRUE);
          else
            rootElem->UnsetAttr(kNameSpaceID_None, nsGkAtoms::active, PR_TRUE);
        }
      }
    }
  }
}

void
nsGlobalWindow::SetChromeEventHandler(nsPIDOMEventTarget* aChromeEventHandler)
{
  SetChromeEventHandlerInternal(aChromeEventHandler);
  if (IsOuterWindow()) {
    
    for (nsGlobalWindow *inner = (nsGlobalWindow *)PR_LIST_HEAD(this);
         inner != this;
         inner = (nsGlobalWindow*)PR_NEXT_LINK(inner)) {
      NS_ASSERTION(inner->mOuterWindow == this, "bad outer window pointer");
      inner->SetChromeEventHandlerInternal(aChromeEventHandler);
    }
  } else if (mOuterWindow) {
    
    
    
    static_cast<nsGlobalWindow*>(mOuterWindow)->
      SetChromeEventHandlerInternal(aChromeEventHandler);
  }
}

nsIFocusController*
nsGlobalWindow::GetRootFocusController()
{
  nsIDOMWindowInternal* rootWindow = nsGlobalWindow::GetPrivateRoot();
  nsCOMPtr<nsIFocusController> fc;

  nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(rootWindow));
  if (piWin) {
    
    nsPIDOMEventTarget* chromeHandler = piWin->GetChromeEventHandler();
    nsCOMPtr<nsPIWindowRoot> windowRoot(do_QueryInterface(chromeHandler));
    if (windowRoot) {
      windowRoot->GetFocusController(getter_AddRefs(fc));
    }
  }

  
  
  return fc;
}

nsIContent*
nsGlobalWindow::GetFocusedNode()
{
  FORWARD_TO_INNER(GetFocusedNode, (), NS_OK);

  return mFocusedNode;
}

void
nsGlobalWindow::SetFocusedNode(nsIContent* aNode,
                               PRUint32 aFocusMethod,
                               PRBool aNeedsFocus)
{
  FORWARD_TO_INNER_VOID(SetFocusedNode, (aNode, aFocusMethod, aNeedsFocus));

  NS_ASSERTION(!aNode || aNode->GetCurrentDoc() == mDoc,
               "setting focus to a node from the wrong document");

  if (mFocusedNode != aNode) {
    UpdateCanvasFocus(PR_FALSE, aNode);
    mFocusedNode = aNode;
    mFocusMethod = aFocusMethod;
  }

  if (aNeedsFocus)
    mNeedsFocus = aNeedsFocus;
}

PRUint32
nsGlobalWindow::GetFocusMethod()
{
  FORWARD_TO_INNER(GetFocusMethod, (), 0);

  return mFocusMethod;
}

PRBool
nsGlobalWindow::TakeFocus(PRBool aFocus, PRUint32 aFocusMethod)
{
  FORWARD_TO_INNER(TakeFocus, (aFocus, aFocusMethod), PR_FALSE);

  if (aFocus)
    mFocusMethod = aFocusMethod;

  if (mHasFocus != aFocus) {
    mHasFocus = aFocus;
    UpdateCanvasFocus(PR_TRUE, mFocusedNode);
  }

  
  
  
  
  
  if (aFocus && mNeedsFocus && mDoc && mDoc->GetRootContent() != nsnull) {
    mNeedsFocus = PR_FALSE;
    return PR_TRUE;
  }

  mNeedsFocus = PR_FALSE;
  return PR_FALSE;
}

void
nsGlobalWindow::SetReadyForFocus()
{
  FORWARD_TO_INNER_VOID(SetReadyForFocus, ());

  
  if (!mNeedsFocus)
    return;

  mNeedsFocus = PR_FALSE;

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm)
    fm->WindowShown(this);
}

void
nsGlobalWindow::PageHidden()
{
  FORWARD_TO_INNER_VOID(PageHidden, ());

  
  
  

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm)
    fm->WindowHidden(this);

  mNeedsFocus = PR_TRUE;
}

nsresult
nsGlobalWindow::DispatchAsyncHashchange()
{
  FORWARD_TO_INNER(DispatchAsyncHashchange, (), NS_OK);

  nsCOMPtr<nsIRunnable> event =
    NS_NEW_RUNNABLE_METHOD(nsGlobalWindow, this, FireHashchange);
   
  return NS_DispatchToCurrentThread(event);
}

nsresult
nsGlobalWindow::FireHashchange()
{
  NS_ENSURE_TRUE(IsInnerWindow(), NS_ERROR_FAILURE);

  
  if (IsFrozen())
      return NS_OK;

  
  
  return nsContentUtils::DispatchTrustedEvent(mDoc, GetOuterWindow(),
                                              NS_LITERAL_STRING("hashchange"),
                                              PR_FALSE, PR_FALSE);
}



static nsICanvasFrame* FindCanvasFrame(nsIFrame* aFrame)
{
    nsICanvasFrame* canvasFrame = do_QueryFrame(aFrame);
    if (canvasFrame) {
        return canvasFrame;
    }

    nsIFrame* kid = aFrame->GetFirstChild(nsnull);
    while (kid) {
        canvasFrame = FindCanvasFrame(kid);
        if (canvasFrame) {
            return canvasFrame;
        }
        kid = kid->GetNextSibling();
    }

    return nsnull;
}



void
nsGlobalWindow::UpdateCanvasFocus(PRBool aFocusChanged, nsIContent* aNewContent)
{
  
  nsIDocShell* docShell = GetDocShell();
  if (!docShell)
    return;

  nsCOMPtr<nsIEditorDocShell> editorDocShell = do_QueryInterface(docShell);
  if (editorDocShell) {
    PRBool editable;
    editorDocShell->GetEditable(&editable);
    if (editable)
      return;
  }

  nsCOMPtr<nsIPresShell> presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell || !mDocument)
    return;

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
  nsIContent *rootContent = doc->GetRootContent();
  if (rootContent) {
      if ((mHasFocus || aFocusChanged) &&
          (mFocusedNode == rootContent || aNewContent == rootContent)) {
          nsIFrame* frame = presShell->GetPrimaryFrameFor(rootContent);
          if (frame) {
              frame = frame->GetParent();
              nsICanvasFrame* canvasFrame = do_QueryFrame(frame);
              if (canvasFrame) {
                  canvasFrame->SetHasFocus(mHasFocus && rootContent == aNewContent);
              }
          }
      }
  } else {
      
      nsIFrame* frame = presShell->GetRootFrame();
      if (frame) {
          nsICanvasFrame* canvasFrame = FindCanvasFrame(frame);
          if (canvasFrame) {
              canvasFrame->SetHasFocus(PR_FALSE);
          }
      }      
  }
}






nsresult
nsGlobalWindow::GetComputedStyle(nsIDOMElement* aElt,
                                 const nsAString& aPseudoElt,
                                 nsComputedDOMStyle** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  if (!aElt) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  if (!mDocShell) {
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));

  if (!presShell) {
    return NS_OK;
  }

  return NS_NewComputedDOMStyle(aElt, aPseudoElt, presShell,
                                aReturn);
}
NS_IMETHODIMP
nsGlobalWindow::GetComputedStyle(nsIDOMElement* aElt,
                                 const nsAString& aPseudoElt,
                                 nsIDOMCSSStyleDeclaration** aReturn)
{
  FORWARD_TO_OUTER(GetComputedStyle, (aElt, aPseudoElt, aReturn),
                   NS_ERROR_NOT_INITIALIZED);
  nsRefPtr<nsComputedDOMStyle> compStyle;
  nsresult rv = GetComputedStyle(aElt, aPseudoElt, getter_AddRefs(compStyle));
  NS_ENSURE_SUCCESS(rv, rv);

  *aReturn = compStyle.forget().get();

  return NS_OK;
}





NS_IMETHODIMP
nsGlobalWindow::GetDocument(nsIDOMDocumentView ** aDocumentView)
{
  NS_ENSURE_ARG_POINTER(aDocumentView);

  nsresult rv = NS_OK;

  if (mDocument) {
    rv = CallQueryInterface(mDocument, aDocumentView);
  }
  else {
    *aDocumentView = nsnull;
  }

  return rv;
}





NS_IMETHODIMP
nsGlobalWindow::GetSessionStorage(nsIDOMStorage ** aSessionStorage)
{
  FORWARD_TO_INNER(GetSessionStorage, (aSessionStorage), NS_ERROR_UNEXPECTED);

  *aSessionStorage = nsnull;

  nsIPrincipal *principal = GetPrincipal();
  nsIDocShell* docShell = GetDocShell();

  if (!principal || !docShell) {
    return NS_OK;
  }

  nsresult rv = docShell->GetSessionStorageForPrincipal(principal,
                                                        PR_TRUE,
                                                        aSessionStorage);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!*aSessionStorage) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetGlobalStorage(nsIDOMStorageList ** aGlobalStorage)
{
  NS_ENSURE_ARG_POINTER(aGlobalStorage);

#ifdef MOZ_STORAGE
  if (!sGlobalStorageList) {
    nsresult rv = NS_NewDOMStorageList(&sGlobalStorageList);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aGlobalStorage = sGlobalStorageList;
  NS_IF_ADDREF(*aGlobalStorage);

  return NS_OK;
#else
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
#endif
}

NS_IMETHODIMP
nsGlobalWindow::GetLocalStorage(nsIDOMStorage ** aLocalStorage)
{
  FORWARD_TO_INNER(GetLocalStorage, (aLocalStorage), NS_ERROR_UNEXPECTED);

  NS_ENSURE_ARG(aLocalStorage);

  if (!mLocalStorage) {
    *aLocalStorage = nsnull;

    nsresult rv;

    PRPackedBool unused;
    if (!nsDOMStorage::CanUseStorage(&unused))
      return NS_ERROR_DOM_SECURITY_ERR;

    nsIPrincipal *principal = GetPrincipal();
    if (!principal)
      return NS_OK;

    nsCOMPtr<nsIDOMStorageManager> storageManager =
      do_GetService("@mozilla.org/dom/storagemanager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = storageManager->GetLocalStorageForPrincipal(principal, getter_AddRefs(mLocalStorage));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aLocalStorage = mLocalStorage);
  return NS_OK;
}





NS_IMETHODIMP
nsGlobalWindow::GetInterface(const nsIID & aIID, void **aSink)
{
  NS_ENSURE_ARG_POINTER(aSink);
  *aSink = nsnull;

  if (aIID.Equals(NS_GET_IID(nsIDocCharset))) {
    FORWARD_TO_OUTER(GetInterface, (aIID, aSink), NS_ERROR_NOT_INITIALIZED);

    if (mDocShell) {
      nsCOMPtr<nsIDocCharset> docCharset(do_QueryInterface(mDocShell));
      if (docCharset) {
        *aSink = docCharset;
        NS_ADDREF(((nsISupports *) *aSink));
      }
    }
  }
  else if (aIID.Equals(NS_GET_IID(nsIWebNavigation))) {
    FORWARD_TO_OUTER(GetInterface, (aIID, aSink), NS_ERROR_NOT_INITIALIZED);

    if (mDocShell) {
      nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
      if (webNav) {
        *aSink = webNav;
        NS_ADDREF(((nsISupports *) *aSink));
      }
    }
  }
#ifdef NS_PRINTING
  else if (aIID.Equals(NS_GET_IID(nsIWebBrowserPrint))) {
    FORWARD_TO_OUTER(GetInterface, (aIID, aSink), NS_ERROR_NOT_INITIALIZED);

    if (mDocShell) {
      nsCOMPtr<nsIContentViewer> viewer;
      mDocShell->GetContentViewer(getter_AddRefs(viewer));
      if (viewer) {
        nsCOMPtr<nsIWebBrowserPrint> webBrowserPrint(do_QueryInterface(viewer));
        if (webBrowserPrint) {
          *aSink = webBrowserPrint;
          NS_ADDREF(((nsISupports *) *aSink));
        }
      }
    }
  }
#endif
  else if (aIID.Equals(NS_GET_IID(nsIScriptEventManager))) {
    if (mDoc) {
      nsIScriptEventManager* mgr = mDoc->GetScriptEventManager();
      if (mgr) {
        *aSink = mgr;
        NS_ADDREF(((nsISupports *) *aSink));
      }
    }
  }
  else if (aIID.Equals(NS_GET_IID(nsIDOMWindowUtils))) {
    FORWARD_TO_OUTER(GetInterface, (aIID, aSink), NS_ERROR_NOT_INITIALIZED);

    nsCOMPtr<nsISupports> utils(do_QueryReferent(mWindowUtils));
    if (utils) {
      *aSink = utils;
      NS_ADDREF(((nsISupports *) *aSink));
    } else {
      nsDOMWindowUtils *utilObj = new nsDOMWindowUtils(this);
      nsCOMPtr<nsISupports> utilsIfc =
                              NS_ISUPPORTS_CAST(nsIDOMWindowUtils *, utilObj);
      if (utilsIfc) {
        mWindowUtils = do_GetWeakReference(utilsIfc);
        *aSink = utilsIfc;
        NS_ADDREF(((nsISupports *) *aSink));
      }
    }
  }
  else {
    return QueryInterface(aIID, aSink);
  }

  return *aSink ? NS_OK : NS_ERROR_NO_INTERFACE;
}

void
nsGlobalWindow::FireOfflineStatusEvent()
{
  if (!mDoc)
    return;
  nsAutoString name;
  if (NS_IsOffline()) {
    name.AssignLiteral("offline");
  } else {
    name.AssignLiteral("online");
  }
  
  
  nsCOMPtr<nsISupports> eventTarget = mDoc.get();
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(mDoc);
  if (htmlDoc) {
    nsCOMPtr<nsIDOMHTMLElement> body;
    htmlDoc->GetBody(getter_AddRefs(body));
    if (body) {
      eventTarget = body;
    }
  }
  else {
    nsCOMPtr<nsIDOMElement> documentElement;
    mDocument->GetDocumentElement(getter_AddRefs(documentElement));
    if(documentElement) {        
      eventTarget = documentElement;
    }
  }
  nsContentUtils::DispatchTrustedEvent(mDoc, eventTarget, name, PR_TRUE, PR_FALSE);
}

nsresult
nsGlobalWindow::Observe(nsISupports* aSubject, const char* aTopic,
                        const PRUnichar* aData)
{
  if (!nsCRT::strcmp(aTopic, NS_IOSERVICE_OFFLINE_STATUS_TOPIC)) {
    if (IsFrozen()) {
      
      
      mFireOfflineStatusChangeEventOnThaw = !mFireOfflineStatusChangeEventOnThaw;
    } else {
      FireOfflineStatusEvent();
    }
    return NS_OK;
  }

  if (IsInnerWindow() && !nsCRT::strcmp(aTopic, "dom-storage-changed")) {
    nsIPrincipal *principal;
    nsresult rv;

    principal = GetPrincipal();
    if (!aData) {
      nsIDocShell* docShell = GetDocShell();
      if (principal && docShell) {
        nsCOMPtr<nsIDOMStorage> storage;
        docShell->GetSessionStorageForPrincipal(principal,
                                                PR_FALSE,
                                                getter_AddRefs(storage));

        if (!SameCOMIdentity(storage, aSubject)) {
          
          
          return NS_OK;
        }
      }
    } else if (principal) {
      
      

      nsCOMPtr<nsIURI> codebase;
      principal->GetURI(getter_AddRefs(codebase));

      if (!codebase) {
        return NS_OK;
      }

      nsCAutoString currentDomain;
      rv = codebase->GetAsciiHost(currentDomain);
      if (NS_FAILED(rv)) {
        return NS_OK;
      }

      if (!nsDOMStorageList::CanAccessDomain(NS_ConvertUTF16toUTF8(aData),
                                             currentDomain)) {
        
        
        

        return NS_OK;
      }
    }

    nsAutoString domain(aData);

    if (IsFrozen()) {
      
      
      

      if (!mPendingStorageEvents) {
        mPendingStorageEvents = new nsDataHashtable<nsStringHashKey, PRBool>;
        NS_ENSURE_TRUE(mPendingStorageEvents, NS_ERROR_OUT_OF_MEMORY);

        rv = mPendingStorageEvents->Init();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      mPendingStorageEvents->Put(domain, PR_TRUE);

      return NS_OK;
    }

    nsRefPtr<nsDOMStorageEvent> event = new nsDOMStorageEvent(domain);
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

    rv = event->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(mDocument));

    nsCOMPtr<nsIDOMEventTarget> target;

    if (htmlDoc) {
      nsCOMPtr<nsIDOMHTMLElement> body;
      htmlDoc->GetBody(getter_AddRefs(body));

      target = do_QueryInterface(body);
    }

    if (!target) {
      target = this;
    }

    PRBool defaultActionEnabled;
    target->DispatchEvent((nsIDOMStorageEvent *)event, &defaultActionEnabled);

    return NS_OK;
  }

  NS_WARNING("unrecognized topic in nsGlobalWindow::Observe");
  return NS_ERROR_FAILURE;
}

static PLDHashOperator
FirePendingStorageEvents(const nsAString& aKey, PRBool aData, void *userArg)
{
  nsGlobalWindow *win = static_cast<nsGlobalWindow *>(userArg);

  nsCOMPtr<nsIDOMStorage> storage;
  win->GetSessionStorage(getter_AddRefs(storage));

  if (storage) {
    win->Observe(storage, "dom-storage-changed",
                 aKey.IsEmpty() ? nsnull : PromiseFlatString(aKey).get());
  }

  return PL_DHASH_NEXT;
}

nsresult
nsGlobalWindow::FireDelayedDOMEvents()
{
  FORWARD_TO_INNER(FireDelayedDOMEvents, (), NS_ERROR_UNEXPECTED);

  if (mPendingStorageEvents) {
    
    mPendingStorageEvents->EnumerateRead(FirePendingStorageEvents, this);

    delete mPendingStorageEvents;
    mPendingStorageEvents = nsnull;
  }

  if (mApplicationCache) {
    static_cast<nsDOMOfflineResourceList*>(mApplicationCache.get())->FirePendingEvents();
  }

  if (mFireOfflineStatusChangeEventOnThaw) {
    mFireOfflineStatusChangeEventOnThaw = PR_FALSE;
    FireOfflineStatusEvent();
  }

  nsCOMPtr<nsIDocShellTreeNode> node =
    do_QueryInterface(GetDocShell());
  if (node) {
    PRInt32 childCount = 0;
    node->GetChildCount(&childCount);

    for (PRInt32 i = 0; i < childCount; ++i) {
      nsCOMPtr<nsIDocShellTreeItem> childShell;
      node->GetChildAt(i, getter_AddRefs(childShell));
      NS_ASSERTION(childShell, "null child shell");

      nsCOMPtr<nsPIDOMWindow> pWin = do_GetInterface(childShell);
      if (pWin) {
        nsGlobalWindow *win =
          static_cast<nsGlobalWindow*>
                     (static_cast<nsPIDOMWindow*>(pWin));
        win->FireDelayedDOMEvents();
      }
    }
  }

  return NS_OK;
}





nsIDOMWindowInternal *
nsGlobalWindow::GetParentInternal()
{
  FORWARD_TO_OUTER(GetParentInternal, (), nsnull);

  nsIDOMWindowInternal *parentInternal = nsnull;

  nsCOMPtr<nsIDOMWindow> parent;
  GetParent(getter_AddRefs(parent));

  if (parent && parent != static_cast<nsIDOMWindow *>(this)) {
    nsCOMPtr<nsIDOMWindowInternal> tmp(do_QueryInterface(parent));
    NS_ASSERTION(parent, "Huh, parent not an nsIDOMWindowInternal?");

    parentInternal = tmp;
  }

  return parentInternal;
}


void
nsGlobalWindow::CloseBlockScriptTerminationFunc(nsISupports *aRef)
{
  nsGlobalWindow* pwin = static_cast<nsGlobalWindow*>
                                    (static_cast<nsPIDOMWindow*>(aRef));
  pwin->mBlockScriptedClosingFlag = PR_FALSE;
}

nsresult
nsGlobalWindow::OpenInternal(const nsAString& aUrl, const nsAString& aName,
                             const nsAString& aOptions, PRBool aDialog,
                             PRBool aContentModal, PRBool aCalledNoScript,
                             PRBool aDoJSFixups, nsIArray *argv,
                             nsISupports *aExtraArgument,
                             nsIPrincipal *aCalleePrincipal,
                             JSContext *aJSCallerContext,
                             nsIDOMWindow **aReturn)
{
  FORWARD_TO_OUTER(OpenInternal, (aUrl, aName, aOptions, aDialog,
                                  aContentModal, aCalledNoScript, aDoJSFixups,
                                  argv, aExtraArgument, aCalleePrincipal,
                                  aJSCallerContext, aReturn),
                   NS_ERROR_NOT_INITIALIZED);

#ifdef NS_DEBUG
  PRUint32 argc = 0;
  if (argv)
      argv->GetLength(&argc);
#endif
  NS_PRECONDITION(!aExtraArgument || (!argv && argc == 0),
                  "Can't pass in arguments both ways");
  NS_PRECONDITION(!aCalledNoScript || (!argv && argc == 0),
                  "Can't pass JS args when called via the noscript methods");
  NS_PRECONDITION(!aJSCallerContext || !aCalledNoScript,
                  "Shouldn't have caller context when called noscript");

  *aReturn = nsnull;

  nsCOMPtr<nsIWebBrowserChrome> chrome;
  GetWebBrowserChrome(getter_AddRefs(chrome));
  if (!chrome) {
    
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ASSERTION(mDocShell, "Must have docshell here");

  const PRBool checkForPopup =
    !aDialog && !WindowExists(aName, !aCalledNoScript);

  
  
  
  nsXPIDLCString url;
  nsresult rv = NS_OK;

  
  
  
  if (!aUrl.IsEmpty()) {
    AppendUTF16toUTF8(aUrl, url);

    


    if (url.get() && !aDialog)
      rv = SecurityCheckURL(url.get());
  }

  if (NS_FAILED(rv))
    return rv;

  
  PopupControlState abuseLevel;
  OpenAllowValue allowReason;
  if (checkForPopup) {
    abuseLevel = CheckForAbusePoint();
    allowReason = CheckOpenAllow(abuseLevel);
    if (allowReason == allowNot) {
      if (aJSCallerContext) {
        
        
        
        
        
        if (mContext == GetScriptContextFromJSContext(aJSCallerContext)) {
          mBlockScriptedClosingFlag = PR_TRUE;
          mContext->SetTerminationFunction(CloseBlockScriptTerminationFunc,
                                           static_cast<nsPIDOMWindow*>
                                                      (this));
        }
      }

      FireAbuseEvents(PR_TRUE, PR_FALSE, aUrl, aName, aOptions);
      return aDoJSFixups ? NS_OK : NS_ERROR_FAILURE;
    }
  }    

  nsCOMPtr<nsIDOMWindow> domReturn;

  nsCOMPtr<nsIWindowWatcher> wwatch =
    do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  NS_ENSURE_TRUE(wwatch, rv);

  NS_ConvertUTF16toUTF8 options(aOptions);
  NS_ConvertUTF16toUTF8 name(aName);

  const char *options_ptr = aOptions.IsEmpty() ? nsnull : options.get();
  const char *name_ptr = aName.IsEmpty() ? nsnull : name.get();

  {
    
    
    
    nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

    if (!aCalledNoScript) {
      nsCOMPtr<nsPIWindowWatcher> pwwatch(do_QueryInterface(wwatch));
      NS_ASSERTION(pwwatch,
                   "Unable to open windows from JS because window watcher "
                   "is broken");
      NS_ENSURE_TRUE(pwwatch, NS_ERROR_UNEXPECTED);
        
      rv = pwwatch->OpenWindowJS(this, url.get(), name_ptr, options_ptr,
                                 aDialog, argv,
                                 getter_AddRefs(domReturn));
    } else {
      
      
      
      
      nsCOMPtr<nsIJSContextStack> stack;

      if (!aContentModal) {
        stack = do_GetService(sJSStackContractID);
      }

      if (stack) {
        rv = stack->Push(nsnull);
        NS_ENSURE_SUCCESS(rv, rv);
      }
        
      rv = wwatch->OpenWindow(this, url.get(), name_ptr, options_ptr,
                              aExtraArgument, getter_AddRefs(domReturn));

      if (stack) {
        JSContext* cx;
        stack->Pop(&cx);
        NS_ASSERTION(!cx, "Unexpected JSContext popped!");
      }
    }
  }

  NS_ENSURE_SUCCESS(rv, rv);

  

  domReturn.swap(*aReturn);

  if (aDoJSFixups) {      
    nsCOMPtr<nsIDOMChromeWindow> chrome_win(do_QueryInterface(*aReturn));
    if (!chrome_win) {
      
      
      
      
      
      
#ifdef DEBUG_jst
      {
        nsCOMPtr<nsPIDOMWindow> pidomwin(do_QueryInterface(*aReturn));

        nsIDOMDocument *temp = pidomwin->GetExtantDocument();

        NS_ASSERTION(temp, "No document in new window!!!");
      }
#endif

      nsCOMPtr<nsIDOMDocument> doc;
      (*aReturn)->GetDocument(getter_AddRefs(doc));
    }
  }
    
  if (checkForPopup) {
    if (abuseLevel >= openControlled) {
      nsGlobalWindow *opened = static_cast<nsGlobalWindow *>(*aReturn);
      if (!opened->IsPopupSpamWindow()) {
        opened->SetPopupSpamWindow(PR_TRUE);
        ++gOpenPopupSpamCount;
      }
    }
    if (abuseLevel >= openAbused)
      FireAbuseEvents(PR_FALSE, PR_TRUE, aUrl, aName, aOptions);
  }

  return rv;
}


void
nsGlobalWindow::CloseWindow(nsISupports *aWindow)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(aWindow));

  nsGlobalWindow* globalWin =
    static_cast<nsGlobalWindow *>
               (static_cast<nsPIDOMWindow*>(win));

  
  
  nsCOMPtr<nsIRunnable> ev = new nsCloseEvent(globalWin);
  if (ev) {
    NS_DispatchToCurrentThread(ev);
  }
  
}


void
nsGlobalWindow::ClearWindowScope(nsISupports *aWindow)
{
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(aWindow));
  PRUint32 lang_id;
  NS_STID_FOR_ID(lang_id) {
    nsIScriptContext *scx = sgo->GetScriptContext(lang_id);
    if (scx) {
      void *global = sgo->GetScriptGlobal(lang_id);
      scx->ClearScope(global, PR_TRUE);
    }
  }
}





nsresult
nsGlobalWindow::SetTimeoutOrInterval(nsIScriptTimeoutHandler *aHandler,
                                     PRInt32 interval,
                                     PRBool aIsInterval, PRInt32 *aReturn)
{
  FORWARD_TO_INNER(SetTimeoutOrInterval, (aHandler, interval, aIsInterval, aReturn),
                   NS_ERROR_NOT_INITIALIZED);

  
  
  if (!mDocument) {
    return NS_OK;
  }

  if (interval < DOM_MIN_TIMEOUT_VALUE) {
    
    

    interval = DOM_MIN_TIMEOUT_VALUE;
  }

  NS_ASSERTION(interval >= 0, "DOM_MIN_TIMEOUT_VALUE lies");
  PRUint32 realInterval = interval;

  
  
  if (realInterval > PR_IntervalToMilliseconds(DOM_MAX_TIMEOUT_VALUE)) {
    realInterval = PR_IntervalToMilliseconds(DOM_MAX_TIMEOUT_VALUE);
  }

  nsTimeout *timeout = new nsTimeout();
  if (!timeout)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  timeout->AddRef();

  if (aIsInterval) {
    timeout->mInterval = realInterval;
  }
  timeout->mScriptHandler = aHandler;

  
  
  
  

  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsresult rv;
  rv = nsContentUtils::GetSecurityManager()->
    GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  if (NS_FAILED(rv)) {
    timeout->Release();

    return NS_ERROR_FAILURE;
  }

  PRBool subsumes = PR_FALSE;
  nsCOMPtr<nsIPrincipal> ourPrincipal = GetPrincipal();

  
  
  
  
  rv = ourPrincipal->Subsumes(subjectPrincipal, &subsumes);
  if (NS_FAILED(rv)) {
    timeout->Release();

    return NS_ERROR_FAILURE;
  }

  if (subsumes) {
    timeout->mPrincipal = subjectPrincipal;
  } else {
    timeout->mPrincipal = ourPrincipal;
  }

  PRTime delta = (PRTime)realInterval * PR_USEC_PER_MSEC;

  if (!IsFrozen() && !mTimeoutsSuspendDepth) {
    
    
    

    timeout->mWhen = PR_Now() + delta;

    timeout->mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_FAILED(rv)) {
      timeout->Release();

      return rv;
    }

    rv = timeout->mTimer->InitWithFuncCallback(TimerCallback, timeout,
                                               realInterval,
                                               nsITimer::TYPE_ONE_SHOT);
    if (NS_FAILED(rv)) {
      timeout->Release();

      return rv;
    }

    
    timeout->AddRef();
  } else {
    
    
    
    

    timeout->mWhen = delta;
  }

  timeout->mWindow = this;

  
  timeout->mPopupState = openAbused;

  if (gRunningTimeoutDepth == 0 && gPopupControlState < openAbused) {
    
    
    
    

    PRInt32 delay =
      nsContentUtils::GetIntPref("dom.disable_open_click_delay");

    if (interval <= delay) {
      timeout->mPopupState = gPopupControlState;
    }
  }

  InsertTimeoutIntoList(timeout);

  timeout->mPublicId = ++mTimeoutPublicIdCounter;
  *aReturn = timeout->mPublicId;

  
  
  timeout->Release();

  return NS_OK;

}

nsresult
nsGlobalWindow::SetTimeoutOrInterval(PRBool aIsInterval, PRInt32 *aReturn)
{
  
  
  
  
  
  

  if (IsOuterWindow()) {
    nsGlobalWindow* callerInner = CallerInnerWindow();
    NS_ENSURE_TRUE(callerInner, NS_ERROR_NOT_AVAILABLE);

    
    
    
    

    if (callerInner->GetOuterWindow() == this &&
        callerInner->IsInnerWindow()) {
      return callerInner->SetTimeoutOrInterval(aIsInterval, aReturn);
    }

    FORWARD_TO_INNER(SetTimeoutOrInterval, (aIsInterval, aReturn),
                     NS_ERROR_NOT_INITIALIZED);
  }

  PRInt32 interval = 0;
  PRBool isInterval = aIsInterval;
  nsCOMPtr<nsIScriptTimeoutHandler> handler;
  nsresult rv = NS_CreateJSTimeoutHandler(this,
                                          &isInterval,
                                          &interval,
                                          getter_AddRefs(handler));
  if (NS_FAILED(rv))
    return (rv == NS_ERROR_DOM_TYPE_ERR) ? NS_OK : rv;

  return SetTimeoutOrInterval(handler, interval, isInterval, aReturn);
}


void
nsGlobalWindow::RunTimeout(nsTimeout *aTimeout)
{
  
  
  if (IsInModalState() || mTimeoutsSuspendDepth) {
    return;
  }

  NS_ASSERTION(IsInnerWindow(), "Timeout running on outer window!");
  NS_ASSERTION(!IsFrozen(), "Timeout running on a window in the bfcache!");

  nsTimeout *nextTimeout, *timeout;
  nsTimeout *last_expired_timeout, *last_insertion_point;
  nsTimeout dummy_timeout;
  PRUint32 firingDepth = mTimeoutFiringDepth + 1;

  
  
  nsCOMPtr<nsIScriptGlobalObject> windowKungFuDeathGrip(this);

  
  
  PRTime now = PR_Now();
  PRTime deadline;

  if (aTimeout && aTimeout->mWhen > now) {
    
    
    
    
    

    deadline = aTimeout->mWhen;
  } else {
    deadline = now;
  }

  
  
  
  
  last_expired_timeout = nsnull;
  for (timeout = FirstTimeout(); IsTimeout(timeout); timeout = timeout->Next()) {
    if (((timeout == aTimeout) || (timeout->mWhen <= deadline)) &&
        (timeout->mFiringDepth == 0)) {
      
      
      timeout->mFiringDepth = firingDepth;
      last_expired_timeout = timeout;
    }
  }

  
  
  
  if (!last_expired_timeout) {
    return;
  }

  
  
  
  
  
  dummy_timeout.mFiringDepth = firingDepth;
  PR_INSERT_AFTER(&dummy_timeout, last_expired_timeout);

  
  
  dummy_timeout.AddRef();
  dummy_timeout.AddRef();

  last_insertion_point = mTimeoutInsertionPoint;
  mTimeoutInsertionPoint = &dummy_timeout;

  for (timeout = FirstTimeout();
       timeout != &dummy_timeout && !IsFrozen();
       timeout = nextTimeout) {
    nextTimeout = timeout->Next();

    if (timeout->mFiringDepth != firingDepth) {
      
      

      continue;
    }

    if (mTimeoutsSuspendDepth) {
      
      
      timeout->mFiringDepth = 0;
      continue;
    }

    
    

    
    
    nsCOMPtr<nsIScriptContext> scx = GetScriptContextInternal(
                                timeout->mScriptHandler->GetScriptTypeID());

    if (!scx) {
      
      
      continue;
    }

    
    
    
    if (!scx->GetScriptsEnabled()) {
      
      
      
      
      
      
      
      continue;
    }

    
    nsTimeout *last_running_timeout = mRunningTimeout;
    mRunningTimeout = timeout;
    timeout->mRunning = PR_TRUE;

    
    
    
    
    nsAutoPopupStatePusher popupStatePusher(timeout->mPopupState);

    
    
    timeout->mPopupState = openAbused;

    
    
    timeout->AddRef();

    ++gRunningTimeoutDepth;
    ++mTimeoutFiringDepth;

    nsCOMPtr<nsIScriptTimeoutHandler> handler(timeout->mScriptHandler);
    void *scriptObject = handler->GetScriptObject();
    if (!scriptObject) {
      
      const PRUnichar *script = handler->GetHandlerText();
      NS_ASSERTION(script, "timeout has no script nor handler text!");

      const char *filename = nsnull;
      PRUint32 lineNo = 0;
      handler->GetLocation(&filename, &lineNo);

      PRBool is_undefined;
      scx->EvaluateString(nsDependentString(script), 
                          GetScriptGlobal(handler->GetScriptTypeID()),
                          timeout->mPrincipal, filename, lineNo,
                          handler->GetScriptVersion(), nsnull,
                          &is_undefined);
    } else {
      
      
      PRTime lateness = now - timeout->mWhen;

      
      
      
      handler->SetLateness((PRIntervalTime)(lateness /
                                            (PRTime)PR_USEC_PER_MSEC));

      nsCOMPtr<nsIVariant> dummy;
      nsCOMPtr<nsISupports> me(static_cast<nsIDOMWindow *>(this));
      scx->CallEventHandler(me,
                            GetScriptGlobal(handler->GetScriptTypeID()),
                            scriptObject, handler->GetArgv(),
                            
                            
                            getter_AddRefs(dummy));

    }
    handler = nsnull; 

    --mTimeoutFiringDepth;
    --gRunningTimeoutDepth;

    mRunningTimeout = last_running_timeout;
    timeout->mRunning = PR_FALSE;

    
    
    
    
    
    
    
    

    
    
    
    PRBool timeout_was_cleared = timeout->mCleared;

    timeout->Release();

    if (timeout_was_cleared) {
      
      
      
      

      mTimeoutInsertionPoint = last_insertion_point;

      return;
    }

    PRBool isInterval = PR_FALSE;

    
    
    if (timeout->mInterval) {
      
      
      
      
      
      
      
      PRTime nextInterval = (PRTime)timeout->mInterval * PR_USEC_PER_MSEC;
      if (!aTimeout || nextInterval + timeout->mWhen <= now)
        nextInterval += now;
      else
        nextInterval += timeout->mWhen;

      PRTime delay = nextInterval - PR_Now();

      
      
      
      if (delay < (PRTime)(DOM_MIN_TIMEOUT_VALUE * PR_USEC_PER_MSEC)) {
        delay = DOM_MIN_TIMEOUT_VALUE * PR_USEC_PER_MSEC;
      }

      if (timeout->mTimer) {
        timeout->mWhen = nextInterval;

        
        
        
        

        
        
        
        
        
        nsresult rv = timeout->mTimer->
          InitWithFuncCallback(TimerCallback, timeout,
                               (PRInt32)(delay / (PRTime)PR_USEC_PER_MSEC),
                               nsITimer::TYPE_ONE_SHOT);

        if (NS_FAILED(rv)) {
          NS_ERROR("Error initializing timer for DOM timeout!");

          
          
          
          
          
          
          timeout->mTimer->Cancel();
          timeout->mTimer = nsnull;

          
          
          timeout->Release();
        }
      } else {
        NS_ASSERTION(IsFrozen() || mTimeoutsSuspendDepth,
                     "How'd our timer end up null if we're not frozen or "
                     "suspended?");

        timeout->mWhen = delay;
        isInterval = PR_TRUE;
      }
    }

    if (timeout->mTimer) {
      if (timeout->mInterval) {
        isInterval = PR_TRUE;
      } else {
        
        
        
        
        timeout->mTimer->Cancel();
        timeout->mTimer = nsnull;

        timeout->Release();
      }
    }

    
    
    nextTimeout = timeout->Next();

    PR_REMOVE_LINK(timeout);

    if (isInterval) {
      
      
      
      InsertTimeoutIntoList(timeout);
    }

    
    timeout->Release();
  }

  
  PR_REMOVE_LINK(&dummy_timeout);

  mTimeoutInsertionPoint = last_insertion_point;
}

nsrefcnt
nsTimeout::Release()
{
  if (--mRefCnt > 0)
    return mRefCnt;

  

  
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }

  delete this;
  return 0;
}

nsrefcnt
nsTimeout::AddRef()
{
  return ++mRefCnt;
}


nsresult
nsGlobalWindow::ClearTimeoutOrInterval(PRInt32 aTimerID)
{
  FORWARD_TO_INNER(ClearTimeoutOrInterval, (aTimerID), NS_ERROR_NOT_INITIALIZED);

  PRUint32 public_id = (PRUint32)aTimerID;
  nsTimeout *timeout;

  for (timeout = FirstTimeout();
       IsTimeout(timeout);
       timeout = timeout->Next()) {
    if (timeout->mPublicId == public_id) {
      if (timeout->mRunning) {
        


        timeout->mInterval = 0;
      }
      else {
        
        PR_REMOVE_LINK(timeout);

        if (timeout->mTimer) {
          timeout->mTimer->Cancel();
          timeout->mTimer = nsnull;
          timeout->Release();
        }
        timeout->Release();
      }
      break;
    }
  }

  return NS_OK;
}


nsresult
nsGlobalWindow::ClearTimeoutOrInterval()
{
  FORWARD_TO_INNER(ClearTimeoutOrInterval, (), NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_OK;
  nsAXPCNativeCallContext *ncc = nsnull;

  rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(&ncc);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  JSContext *cx = nsnull;

  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 argc;

  ncc->GetArgc(&argc);

  if (argc < 1) {
    

    return NS_OK;
  }

  jsval *argv = nsnull;

  ncc->GetArgvPtr(&argv);

  int32 timer_id;

  JSAutoRequest ar(cx);

  
  if (argv[0] == JSVAL_VOID || !::JS_ValueToInt32(cx, argv[0], &timer_id) ||
      timer_id <= 0) {
    
    

    ::JS_ClearPendingException(cx);
    return NS_OK;
  }

  return ClearTimeoutOrInterval(timer_id);
}

void
nsGlobalWindow::ClearAllTimeouts()
{
  nsTimeout *timeout, *nextTimeout;

  for (timeout = FirstTimeout(); IsTimeout(timeout); timeout = nextTimeout) {
    




    if (mRunningTimeout == timeout)
      mTimeoutInsertionPoint = nsnull;

    nextTimeout = timeout->Next();

    if (timeout->mTimer) {
      timeout->mTimer->Cancel();
      timeout->mTimer = nsnull;

      
      
      timeout->Release();
    }

    
    
    timeout->mCleared = PR_TRUE;

    
    timeout->Release();
  }

  
  PR_INIT_CLIST(&mTimeouts);
}

void
nsGlobalWindow::InsertTimeoutIntoList(nsTimeout *aTimeout)
{
  NS_ASSERTION(IsInnerWindow(),
               "InsertTimeoutIntoList() called on outer window!");

  
  
  
  nsTimeout* prevSibling;
  for (prevSibling = LastTimeout();
       IsTimeout(prevSibling) && prevSibling != mTimeoutInsertionPoint &&
         prevSibling->mWhen > aTimeout->mWhen;
       prevSibling = prevSibling->Prev()) {
    
  }

  
  PR_INSERT_AFTER(aTimeout, prevSibling);

  aTimeout->mFiringDepth = 0;

  
  
  aTimeout->AddRef();
}


void
nsGlobalWindow::TimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsTimeout *timeout = (nsTimeout *)aClosure;

  
  
  timeout->AddRef();

  timeout->mWindow->RunTimeout(timeout);

  
  timeout->Release();
}




nsresult
nsGlobalWindow::GetTreeOwner(nsIDocShellTreeOwner **aTreeOwner)
{
  FORWARD_TO_OUTER(GetTreeOwner, (aTreeOwner), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));

  
  

  if (!docShellAsItem) {
    *aTreeOwner = nsnull;

    return NS_OK;
  }

  return docShellAsItem->GetTreeOwner(aTreeOwner);
}

nsresult
nsGlobalWindow::GetTreeOwner(nsIBaseWindow **aTreeOwner)
{
  FORWARD_TO_OUTER(GetTreeOwner, (aTreeOwner), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;

  
  

  if (docShellAsItem) {
    docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
  }

  if (!treeOwner) {
    *aTreeOwner = nsnull;
    return NS_OK;
  }

  return CallQueryInterface(treeOwner, aTreeOwner);
}

nsresult
nsGlobalWindow::GetWebBrowserChrome(nsIWebBrowserChrome **aBrowserChrome)
{
  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  GetTreeOwner(getter_AddRefs(treeOwner));

  nsCOMPtr<nsIWebBrowserChrome> browserChrome(do_GetInterface(treeOwner));
  NS_IF_ADDREF(*aBrowserChrome = browserChrome);

  return NS_OK;
}

nsresult
nsGlobalWindow::GetScrollInfo(nsIScrollableView **aScrollableView)
{
  FORWARD_TO_OUTER(GetScrollInfo, (aScrollableView),
                   NS_ERROR_NOT_INITIALIZED);

  *aScrollableView = nsnull;

  if (!mDocShell) {
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));
  if (presShell) {
    nsIViewManager* vm = presShell->GetViewManager();
    if (vm)
      return vm->GetRootScrollableView(aScrollableView);
  }
  return NS_OK;
}

nsresult
nsGlobalWindow::BuildURIfromBase(const char *aURL, nsIURI **aBuiltURI,
                                 PRBool *aFreeSecurityPass,
                                 JSContext **aCXused)
{
  nsIScriptContext *scx = GetContextInternal();
  JSContext *cx = nsnull;

  *aBuiltURI = nsnull;
  *aFreeSecurityPass = PR_FALSE;
  if (aCXused)
    *aCXused = nsnull;

  
  NS_ASSERTION(scx, "opening window missing its context");
  NS_ASSERTION(mDocument, "opening window missing its document");
  if (!scx || !mDocument)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMChromeWindow> chrome_win =
    do_QueryInterface(static_cast<nsIDOMWindow *>(this));

  if (nsContentUtils::IsCallerChrome() && !chrome_win) {
    
    
    
    
    

    cx = (JSContext *)scx->GetNativeContext();
  } else {
    
    nsCOMPtr<nsIThreadJSContextStack> stack(do_GetService(sJSStackContractID));
    if (stack)
      stack->Peek(&cx);
  }

  


  nsCAutoString charset(NS_LITERAL_CSTRING("UTF-8")); 
  nsIURI* baseURI = nsnull;
  nsCOMPtr<nsIURI> uriToLoad;
  nsCOMPtr<nsIDOMWindow> sourceWindow;

  if (cx) {
    nsIScriptContext *scriptcx = nsJSUtils::GetDynamicScriptContext(cx);
    if (scriptcx)
      sourceWindow = do_QueryInterface(scriptcx->GetGlobalObject());
  }

  if (!sourceWindow) {
    sourceWindow = do_QueryInterface(NS_ISUPPORTS_CAST(nsIDOMWindow *, this));
    *aFreeSecurityPass = PR_TRUE;
  }

  if (sourceWindow) {
    nsCOMPtr<nsIDOMDocument> domDoc;
    sourceWindow->GetDocument(getter_AddRefs(domDoc));
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
    if (doc) {
      baseURI = doc->GetBaseURI();
      charset = doc->GetDocumentCharacterSet();
    }
  }

  if (aCXused)
    *aCXused = cx;
  return NS_NewURI(aBuiltURI, nsDependentCString(aURL), charset.get(), baseURI);
}

nsresult
nsGlobalWindow::SecurityCheckURL(const char *aURL)
{
  JSContext       *cx;
  PRBool           freePass;
  nsCOMPtr<nsIURI> uri;

  if (NS_FAILED(BuildURIfromBase(aURL, getter_AddRefs(uri), &freePass, &cx)))
    return NS_ERROR_FAILURE;

  if (!freePass && NS_FAILED(nsContentUtils::GetSecurityManager()->
        CheckLoadURIFromScript(cx, uri)))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

void
nsGlobalWindow::FlushPendingNotifications(mozFlushType aType)
{
  if (mDoc) {
    mDoc->FlushPendingNotifications(aType);
  }
}

void
nsGlobalWindow::EnsureSizeUpToDate()
{
  
  
  
  nsGlobalWindow *parent =
    static_cast<nsGlobalWindow *>(GetPrivateParent());
  if (parent) {
    parent->FlushPendingNotifications(Flush_Layout);
  }
}

nsresult
nsGlobalWindow::SaveWindowState(nsISupports **aState)
{
  NS_PRECONDITION(IsOuterWindow(), "Can't save the inner window's state");

  *aState = nsnull;

  if (!mContext || !mJSObject) {
    
    return NS_OK;
  }

  nsGlobalWindow *inner = GetCurrentInnerWindowInternal();
  NS_ASSERTION(inner, "No inner window to save");

  
  
  
  
  
  inner->Freeze();

  
  nsCOMPtr<nsIClassInfo> ci =
    do_QueryInterface((nsIScriptGlobalObject *)this);
  nsCOMPtr<nsIXPConnectJSObjectHolder> proto;
  nsresult rv = nsContentUtils::XPConnect()->
    GetWrappedNativePrototype((JSContext *)mContext->GetNativeContext(),
                              mJSObject, ci, getter_AddRefs(proto));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> state = new WindowStateHolder(inner,
                                                      mInnerWindowHolders,
                                                      mNavigator,
                                                      mLocation,
                                                      proto);
  NS_ENSURE_TRUE(state, NS_ERROR_OUT_OF_MEMORY);

#ifdef DEBUG_PAGE_CACHE
  printf("saving window state, state = %p\n", (void*)state);
#endif

  state.swap(*aState);
  return NS_OK;
}

nsresult
nsGlobalWindow::RestoreWindowState(nsISupports *aState)
{
  NS_ASSERTION(IsOuterWindow(), "Cannot restore an inner window");

  if (!mContext || !mJSObject) {
    
    return NS_OK;
  }

  nsCOMPtr<WindowStateHolder> holder = do_QueryInterface(aState);
  NS_ENSURE_TRUE(holder, NS_ERROR_FAILURE);

#ifdef DEBUG_PAGE_CACHE
  printf("restoring window state, state = %p\n", (void*)holder);
#endif

  
  nsGlobalWindow *inner = GetCurrentInnerWindowInternal();
  inner->Thaw();

  holder->DidRestoreWindow();

  return NS_OK;
}

void
nsGlobalWindow::SuspendTimeouts(PRUint32 aIncrease,
                                PRBool aFreezeChildren)
{
  FORWARD_TO_INNER_VOID(SuspendTimeouts, (aIncrease, aFreezeChildren));

  PRBool suspended = (mTimeoutsSuspendDepth != 0);
  mTimeoutsSuspendDepth += aIncrease;

  if (!suspended) {
    nsDOMThreadService* dts = nsDOMThreadService::get();
    if (dts) {
      dts->SuspendWorkersForGlobal(static_cast<nsIScriptGlobalObject*>(this));
    }
  
    PRTime now = PR_Now();
    for (nsTimeout *t = FirstTimeout(); IsTimeout(t); t = t->Next()) {
      
      if (t->mWhen > now)
        t->mWhen -= now;
      else
        t->mWhen = 0;
  
      
      if (t->mTimer) {
        t->mTimer->Cancel();
        t->mTimer = nsnull;
  
        
        
        
        
        t->Release();
      }
    }
  }

  
  nsCOMPtr<nsIDocShellTreeNode> node(do_QueryInterface(GetDocShell()));
  if (node) {
    PRInt32 childCount = 0;
    node->GetChildCount(&childCount);

    for (PRInt32 i = 0; i < childCount; ++i) {
      nsCOMPtr<nsIDocShellTreeItem> childShell;
      node->GetChildAt(i, getter_AddRefs(childShell));
      NS_ASSERTION(childShell, "null child shell");

      nsCOMPtr<nsPIDOMWindow> pWin = do_GetInterface(childShell);
      if (pWin) {
        nsGlobalWindow *win =
          static_cast<nsGlobalWindow*>
                     (static_cast<nsPIDOMWindow*>(pWin));
        win->SuspendTimeouts(aIncrease, aFreezeChildren);

        NS_ASSERTION(win->IsOuterWindow(), "Expected outer window");
        nsGlobalWindow* inner = win->GetCurrentInnerWindowInternal();
        if (inner && aFreezeChildren) {
          inner->Freeze();
        }
      }
    }
  }
}

nsresult
nsGlobalWindow::ResumeTimeouts(PRBool aThawChildren)
{
  FORWARD_TO_INNER(ResumeTimeouts, (), NS_ERROR_NOT_INITIALIZED);

  NS_ASSERTION(mTimeoutsSuspendDepth, "Mismatched calls to ResumeTimeouts!");
  --mTimeoutsSuspendDepth;
  PRBool shouldResume = (mTimeoutsSuspendDepth == 0);
  nsresult rv;

  if (shouldResume) {
    nsDOMThreadService* dts = nsDOMThreadService::get();
    if (dts) {
      dts->ResumeWorkersForGlobal(static_cast<nsIScriptGlobalObject*>(this));
    }

    
    

    PRTime now = PR_Now();

#ifdef DEBUG
    PRBool _seenDummyTimeout = PR_FALSE;
#endif

    for (nsTimeout *t = FirstTimeout(); IsTimeout(t); t = t->Next()) {
      
      
      
      if (!t->mWindow) {
#ifdef DEBUG
        NS_ASSERTION(!_seenDummyTimeout, "More than one dummy timeout?!");
        _seenDummyTimeout = PR_TRUE;
#endif
        continue;
      }

      
      
      
      
      
      PRUint32 delay =
        PR_MAX(((PRUint32)(t->mWhen / (PRTime)PR_USEC_PER_MSEC)),
                DOM_MIN_TIMEOUT_VALUE);

      
      
      t->mWhen += now;

      t->mTimer = do_CreateInstance("@mozilla.org/timer;1");
      NS_ENSURE_TRUE(t->mTimer, NS_ERROR_OUT_OF_MEMORY);

      rv = t->mTimer->InitWithFuncCallback(TimerCallback, t, delay,
                                           nsITimer::TYPE_ONE_SHOT);
      if (NS_FAILED(rv)) {
        t->mTimer = nsnull;
        return rv;
      }

      
      t->AddRef();
    }
  }

  
  nsCOMPtr<nsIDocShellTreeNode> node =
    do_QueryInterface(GetDocShell());
  if (node) {
    PRInt32 childCount = 0;
    node->GetChildCount(&childCount);

    for (PRInt32 i = 0; i < childCount; ++i) {
      nsCOMPtr<nsIDocShellTreeItem> childShell;
      node->GetChildAt(i, getter_AddRefs(childShell));
      NS_ASSERTION(childShell, "null child shell");

      nsCOMPtr<nsPIDOMWindow> pWin = do_GetInterface(childShell);
      if (pWin) {
        nsGlobalWindow *win =
          static_cast<nsGlobalWindow*>
                     (static_cast<nsPIDOMWindow*>(pWin));

        NS_ASSERTION(win->IsOuterWindow(), "Expected outer window");
        nsGlobalWindow* inner = win->GetCurrentInnerWindowInternal();
        if (inner && aThawChildren) {
          inner->Thaw();
        }

        rv = win->ResumeTimeouts(aThawChildren);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  return NS_OK;
}

PRUint32
nsGlobalWindow::TimeoutSuspendCount()
{
  FORWARD_TO_INNER(TimeoutSuspendCount, (), 0);
  return mTimeoutsSuspendDepth;
}

NS_IMETHODIMP
nsGlobalWindow::GetScriptTypeID(PRUint32 *aScriptType)
{
  NS_ERROR("No default script type here - ask some element");
  return nsIProgrammingLanguage::UNKNOWN;
}

NS_IMETHODIMP
nsGlobalWindow::SetScriptTypeID(PRUint32 aScriptType)
{
  NS_ERROR("Can't change default script type for a document");
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsGlobalWindow::SetHasOrientationEventListener()
{
  nsCOMPtr<nsIAccelerometer> ac = 
    do_GetService(NS_ACCELEROMETER_CONTRACTID);

  if (ac) {
    mHasAcceleration = PR_TRUE;
    ac->AddWindowListener(this);
  }
}



NS_IMPL_CYCLE_COLLECTION_CLASS(nsGlobalChromeWindow)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsGlobalChromeWindow,
                                                  nsGlobalWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mBrowserDOMWindow)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsGlobalChromeWindow)
  NS_INTERFACE_MAP_ENTRY(nsIDOMChromeWindow)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ChromeWindow)
NS_INTERFACE_MAP_END_INHERITING(nsGlobalWindow)

NS_IMPL_ADDREF_INHERITED(nsGlobalChromeWindow, nsGlobalWindow)
NS_IMPL_RELEASE_INHERITED(nsGlobalChromeWindow, nsGlobalWindow)

NS_IMETHODIMP
nsGlobalChromeWindow::GetWindowState(PRUint16* aWindowState)
{
  *aWindowState = nsIDOMChromeWindow::STATE_NORMAL;

  nsCOMPtr<nsIWidget> widget = GetMainWidget();

  PRInt32 aMode = 0;

  if (widget) {
    nsresult rv = widget->GetSizeMode(&aMode);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  switch (aMode) {
    case nsSizeMode_Minimized:
      *aWindowState = nsIDOMChromeWindow::STATE_MINIMIZED;
      break;
    case nsSizeMode_Maximized:
      *aWindowState = nsIDOMChromeWindow::STATE_MAXIMIZED;
      break;
    case nsSizeMode_Fullscreen:
      *aWindowState = nsIDOMChromeWindow::STATE_FULLSCREEN;
      break;
    case nsSizeMode_Normal:
      *aWindowState = nsIDOMChromeWindow::STATE_NORMAL;
      break;
    default:
      NS_WARNING("Illegal window state for this chrome window");
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalChromeWindow::Maximize()
{
  nsCOMPtr<nsIWidget> widget = GetMainWidget();
  nsresult rv = NS_OK;

  if (widget) {
    rv = widget->SetSizeMode(nsSizeMode_Maximized);
  }

  return rv;
}

NS_IMETHODIMP
nsGlobalChromeWindow::Minimize()
{
  nsCOMPtr<nsIWidget> widget = GetMainWidget();
  nsresult rv = NS_OK;

  if (widget)
    rv = widget->SetSizeMode(nsSizeMode_Minimized);

  return rv;
}

NS_IMETHODIMP
nsGlobalChromeWindow::Restore()
{
  nsCOMPtr<nsIWidget> widget = GetMainWidget();
  nsresult rv = NS_OK;

  if (widget) {
    rv = widget->SetSizeMode(nsSizeMode_Normal);
  }

  return rv;
}

NS_IMETHODIMP
nsGlobalChromeWindow::GetAttention()
{
  return GetAttentionWithCycleCount(-1);
}

NS_IMETHODIMP
nsGlobalChromeWindow::GetAttentionWithCycleCount(PRInt32 aCycleCount)
{
  nsCOMPtr<nsIWidget> widget = GetMainWidget();
  nsresult rv = NS_OK;

  if (widget) {
    rv = widget->GetAttention(aCycleCount);
  }

  return rv;
}



NS_IMETHODIMP
nsGlobalChromeWindow::SetCursor(const nsAString& aCursor)
{
  FORWARD_TO_OUTER_CHROME(SetCursor, (aCursor), NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_OK;
  PRInt32 cursor;

  
  NS_ConvertUTF16toUTF8 cursorString(aCursor);

  if (cursorString.Equals("auto"))
    cursor = NS_STYLE_CURSOR_AUTO;
  else {
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(aCursor);
    if (eCSSKeyword_UNKNOWN == keyword ||
        !nsCSSProps::FindKeyword(keyword, nsCSSProps::kCursorKTable, cursor)) {
      
      
      
      if (cursorString.Equals("grab"))
        cursor = NS_STYLE_CURSOR_GRAB;
      else if (cursorString.Equals("grabbing"))
        cursor = NS_STYLE_CURSOR_GRABBING;
      else if (cursorString.Equals("spinning"))
        cursor = NS_STYLE_CURSOR_SPINNING;
      else
        return NS_OK;
    }
  }

  nsCOMPtr<nsPresContext> presContext;
  if (mDocShell) {
    mDocShell->GetPresContext(getter_AddRefs(presContext));
  }

  if (presContext) {
    
    nsCOMPtr<nsIPresShell> presShell;
    mDocShell->GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    nsIViewManager* vm = presShell->GetViewManager();
    NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);

    nsIView *rootView;
    vm->GetRootView(rootView);
    NS_ENSURE_TRUE(rootView, NS_ERROR_FAILURE);

    nsIWidget* widget = rootView->GetNearestWidget(nsnull);
    NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

    
    rv = presContext->EventStateManager()->SetCursor(cursor, nsnull,
                                                     PR_FALSE, 0.0f, 0.0f,
                                                     widget, PR_TRUE);
  }

  return rv;
}

NS_IMETHODIMP
nsGlobalChromeWindow::GetBrowserDOMWindow(nsIBrowserDOMWindow **aBrowserWindow)
{
  FORWARD_TO_OUTER_CHROME(GetBrowserDOMWindow, (aBrowserWindow),
                          NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aBrowserWindow);

  *aBrowserWindow = mBrowserDOMWindow;
  NS_IF_ADDREF(*aBrowserWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalChromeWindow::SetBrowserDOMWindow(nsIBrowserDOMWindow *aBrowserWindow)
{
  FORWARD_TO_OUTER_CHROME(SetBrowserDOMWindow, (aBrowserWindow),
                          NS_ERROR_NOT_INITIALIZED);

  mBrowserDOMWindow = aBrowserWindow;
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalChromeWindow::NotifyDefaultButtonLoaded(nsIDOMElement* aDefaultButton)
{
#ifdef MOZ_XUL
  NS_ENSURE_ARG(aDefaultButton);

  
  nsCOMPtr<nsIDOMXULControlElement> xulControl =
                                      do_QueryInterface(aDefaultButton);
  NS_ENSURE_TRUE(xulControl, NS_ERROR_FAILURE);
  PRBool disabled;
  nsresult rv = xulControl->GetDisabled(&disabled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (disabled)
    return NS_OK;

  
  nsCOMPtr<nsIContent> content(do_QueryInterface(aDefaultButton));
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);
  nsIDocument *doc = content->GetCurrentDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
  nsIPresShell *shell = doc->GetPrimaryShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);
  nsIFrame *frame = shell->GetPrimaryFrameFor(content);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);
  nsIntRect buttonRect = frame->GetScreenRect();

  
  nsIWidget *widget = GetNearestWidget();
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);
  nsIntRect widgetRect;
  rv = widget->GetScreenBounds(widgetRect);
  NS_ENSURE_SUCCESS(rv, rv);

  
  buttonRect -= widgetRect.TopLeft();
  rv = widget->OnDefaultButtonLoaded(buttonRect);
  if (rv == NS_ERROR_NOT_IMPLEMENTED)
    return NS_OK;
  return rv;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsGlobalModalWindow)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsGlobalModalWindow,
                                                  nsGlobalWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mReturnValue)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsGlobalModalWindow)
  NS_INTERFACE_MAP_ENTRY(nsIDOMModalContentWindow)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ModalContentWindow)
NS_INTERFACE_MAP_END_INHERITING(nsGlobalWindow)

NS_IMPL_ADDREF_INHERITED(nsGlobalModalWindow, nsGlobalWindow)
NS_IMPL_RELEASE_INHERITED(nsGlobalModalWindow, nsGlobalWindow)


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsGlobalModalWindow,
                                                nsGlobalWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mReturnValue)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


NS_IMETHODIMP
nsGlobalModalWindow::GetDialogArguments(nsIArray **aArguments)
{
  FORWARD_TO_INNER_MODAL_CONTENT_WINDOW(GetDialogArguments, (aArguments),
                                        NS_ERROR_NOT_INITIALIZED);

  PRBool subsumes = PR_FALSE;
  nsIPrincipal *self = GetPrincipal();
  if (self && NS_SUCCEEDED(self->Subsumes(mArgumentsOrigin, &subsumes)) &&
      subsumes) {
    NS_IF_ADDREF(*aArguments = mArguments);
  } else {
    *aArguments = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalModalWindow::GetReturnValue(nsIVariant **aRetVal)
{
  FORWARD_TO_OUTER_MODAL_CONTENT_WINDOW(GetReturnValue, (aRetVal), NS_OK);

  NS_IF_ADDREF(*aRetVal = mReturnValue);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalModalWindow::SetReturnValue(nsIVariant *aRetVal)
{
  FORWARD_TO_OUTER_MODAL_CONTENT_WINDOW(SetReturnValue, (aRetVal), NS_OK);

  mReturnValue = aRetVal;

  return NS_OK;
}

nsresult
nsGlobalModalWindow::SetNewDocument(nsIDocument *aDocument,
                                    nsISupports *aState,
                                    PRBool aClearScopeHint)
{
  
  
  if (aDocument) {
    mReturnValue = nsnull;
  }

  return nsGlobalWindow::SetNewDocument(aDocument, aState, aClearScopeHint);
}





nsresult
NS_NewScriptGlobalObject(PRBool aIsChrome, PRBool aIsModalContentWindow,
                         nsIScriptGlobalObject **aResult)
{
  *aResult = nsnull;

  nsGlobalWindow *global;

  if (aIsChrome) {
    global = new nsGlobalChromeWindow(nsnull);
  } else if (aIsModalContentWindow) {
    global = new nsGlobalModalWindow(nsnull);
  } else {
    global = new nsGlobalWindow(nsnull);
  }

  NS_ENSURE_TRUE(global, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aResult = global);

  return NS_OK;
}





nsNavigator::nsNavigator(nsIDocShell *aDocShell)
  : mDocShell(aDocShell)
{
}

nsNavigator::~nsNavigator()
{
  sPrefInternal_id = JSVAL_VOID;
}







NS_INTERFACE_MAP_BEGIN(nsNavigator)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMJSNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMClientInformation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorGeolocation)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Navigator)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsNavigator)
NS_IMPL_RELEASE(nsNavigator)


void
nsNavigator::SetDocShell(nsIDocShell *aDocShell)
{
  mDocShell = aDocShell;
  if (mPlugins)
    mPlugins->SetDocShell(aDocShell);

  
  if (mGeolocation)
  {
    mGeolocation->Shutdown();
    mGeolocation = nsnull;
  }
}





NS_IMETHODIMP
nsNavigator::GetUserAgent(nsAString& aUserAgent)
{
  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString ua;
    rv = service->GetUserAgent(ua);
    CopyASCIItoUTF16(ua, aUserAgent);
  }

  return rv;
}

NS_IMETHODIMP
nsNavigator::GetAppCodeName(nsAString& aAppCodeName)
{
  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString appName;
    rv = service->GetAppName(appName);
    CopyASCIItoUTF16(appName, aAppCodeName);
  }

  return rv;
}

NS_IMETHODIMP
nsNavigator::GetAppVersion(nsAString& aAppVersion)
{
  if (!nsContentUtils::IsCallerTrustedForRead()) {
    const nsAdoptingCString& override = 
      nsContentUtils::GetCharPref("general.appversion.override");

    if (override) {
      CopyUTF8toUTF16(override, aAppVersion);
      return NS_OK;
    }
  }

  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString str;
    rv = service->GetAppVersion(str);
    CopyASCIItoUTF16(str, aAppVersion);
    if (NS_FAILED(rv))
      return rv;

    aAppVersion.AppendLiteral(" (");

    rv = service->GetPlatform(str);
    if (NS_FAILED(rv))
      return rv;

    AppendASCIItoUTF16(str, aAppVersion);

    aAppVersion.AppendLiteral("; ");

    rv = service->GetLanguage(str);
    if (NS_FAILED(rv))
      return rv;
    AppendASCIItoUTF16(str, aAppVersion);

    aAppVersion.Append(PRUnichar(')'));
  }

  return rv;
}

NS_IMETHODIMP
nsNavigator::GetAppName(nsAString& aAppName)
{
  if (!nsContentUtils::IsCallerTrustedForRead()) {
    const nsAdoptingCString& override =
      nsContentUtils::GetCharPref("general.appname.override");

    if (override) {
      CopyUTF8toUTF16(override, aAppName);
      return NS_OK;
    }
  }

  aAppName.AssignLiteral("Netscape");
  return NS_OK;
}

NS_IMETHODIMP
nsNavigator::GetLanguage(nsAString& aLanguage)
{
  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString lang;
    rv = service->GetLanguage(lang);
    CopyASCIItoUTF16(lang, aLanguage);
  }

  return rv;
}

NS_IMETHODIMP
nsNavigator::GetPlatform(nsAString& aPlatform)
{
  if (!nsContentUtils::IsCallerTrustedForRead()) {
    const nsAdoptingCString& override =
      nsContentUtils::GetCharPref("general.platform.override");

    if (override) {
      CopyUTF8toUTF16(override, aPlatform);
      return NS_OK;
    }
  }

  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_SUCCEEDED(rv)) {
    
    
    
#if defined(_WIN64)
    aPlatform.AssignLiteral("Win64");
#elif defined(WIN32)
    aPlatform.AssignLiteral("Win32");
#elif defined(XP_MACOSX) && defined(__ppc__)
    aPlatform.AssignLiteral("MacPPC");
#elif defined(XP_MACOSX) && defined(__i386__)
    aPlatform.AssignLiteral("MacIntel");
#elif defined(XP_OS2)
    aPlatform.AssignLiteral("OS/2");
#else
    
    
    
    nsCAutoString plat;
    rv = service->GetOscpu(plat);
    CopyASCIItoUTF16(plat, aPlatform);
#endif
  }

  return rv;
}

NS_IMETHODIMP
nsNavigator::GetOscpu(nsAString& aOSCPU)
{
  if (!nsContentUtils::IsCallerTrustedForRead()) {
    const nsAdoptingCString& override =
      nsContentUtils::GetCharPref("general.oscpu.override");

    if (override) {
      CopyUTF8toUTF16(override, aOSCPU);
      return NS_OK;
    }
  }

  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString oscpu;
    rv = service->GetOscpu(oscpu);
    CopyASCIItoUTF16(oscpu, aOSCPU);
  }

  return rv;
}

NS_IMETHODIMP
nsNavigator::GetVendor(nsAString& aVendor)
{
  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString vendor;
    rv = service->GetVendor(vendor);
    CopyASCIItoUTF16(vendor, aVendor);
  }

  return rv;
}


NS_IMETHODIMP
nsNavigator::GetVendorSub(nsAString& aVendorSub)
{
  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString vendor;
    rv = service->GetVendorSub(vendor);
    CopyASCIItoUTF16(vendor, aVendorSub);
  }

  return rv;
}

NS_IMETHODIMP
nsNavigator::GetProduct(nsAString& aProduct)
{
  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString product;
    rv = service->GetProduct(product);
    CopyASCIItoUTF16(product, aProduct);
  }

  return rv;
}

NS_IMETHODIMP
nsNavigator::GetProductSub(nsAString& aProductSub)
{
  if (!nsContentUtils::IsCallerTrustedForRead()) {
    const nsAdoptingCString& override =
      nsContentUtils::GetCharPref("general.productSub.override");

    if (override) {
      CopyUTF8toUTF16(override, aProductSub);
      return NS_OK;
    } else {
      
      const nsAdoptingCString& override2 =
        nsContentUtils::GetCharPref("general.useragent.productSub");

      if (override2) {
        CopyUTF8toUTF16(override2, aProductSub);
        return NS_OK;
      }
    }
  }

  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString productSub;
    rv = service->GetProductSub(productSub);
    CopyASCIItoUTF16(productSub, aProductSub);
  }

  return rv;
}

NS_IMETHODIMP
nsNavigator::GetSecurityPolicy(nsAString& aSecurityPolicy)
{
  return NS_OK;
}

NS_IMETHODIMP
nsNavigator::GetMimeTypes(nsIDOMMimeTypeArray **aMimeTypes)
{
  if (!mMimeTypes) {
    mMimeTypes = new nsMimeTypeArray(this);
    if (!mMimeTypes) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aMimeTypes = mMimeTypes);

  return NS_OK;
}

NS_IMETHODIMP
nsNavigator::GetPlugins(nsIDOMPluginArray **aPlugins)
{
  if (!mPlugins) {
    mPlugins = new nsPluginArray(this, mDocShell);
    if (!mPlugins) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aPlugins = mPlugins);

  return NS_OK;
}



#define COOKIE_BEHAVIOR_REJECT 2

NS_IMETHODIMP
nsNavigator::GetCookieEnabled(PRBool *aCookieEnabled)
{
  *aCookieEnabled =
    (nsContentUtils::GetIntPref("network.cookie.cookieBehavior",
                                COOKIE_BEHAVIOR_REJECT) !=
     COOKIE_BEHAVIOR_REJECT);

  return NS_OK;
}

NS_IMETHODIMP
nsNavigator::GetOnLine(PRBool* aOnline)
{
  NS_PRECONDITION(aOnline, "Null out param");
  
  *aOnline = !NS_IsOffline();
  return NS_OK;
}

NS_IMETHODIMP
nsNavigator::GetBuildID(nsAString& aBuildID)
{
  if (!nsContentUtils::IsCallerTrustedForRead()) {
    const nsAdoptingCString& override =
      nsContentUtils::GetCharPref("general.buildID.override");

    if (override) {
      CopyUTF8toUTF16(override, aBuildID);
      return NS_OK;
    }
  }

  nsCOMPtr<nsIXULAppInfo> appInfo =
    do_GetService("@mozilla.org/xre/app-info;1");
  if (!appInfo)
    return NS_ERROR_NOT_IMPLEMENTED;

  nsCAutoString buildID;
  nsresult rv = appInfo->GetAppBuildID(buildID);
  if (NS_FAILED(rv))
    return rv;

  aBuildID.Truncate();
  AppendASCIItoUTF16(buildID, aBuildID);
  return NS_OK;
}

NS_IMETHODIMP
nsNavigator::JavaEnabled(PRBool *aReturn)
{
  
  
  *aReturn = PR_FALSE;

  if (!mMimeTypes) {
    mMimeTypes = new nsMimeTypeArray(this);
    if (!mMimeTypes)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  RefreshMIMEArray();

  PRUint32 count;
  mMimeTypes->GetLength(&count);
  for (PRUint32 i = 0; i < count; i++) {
    nsresult rv;
    nsIDOMMimeType* type = mMimeTypes->GetItemAt(i, &rv);
    nsAutoString mimeString;
    if (type && NS_SUCCEEDED(type->GetType(mimeString))) {
      if (mimeString.EqualsLiteral("application/x-java-vm")) {
        *aReturn = PR_TRUE;
        break;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNavigator::TaintEnabled(PRBool *aReturn)
{
  *aReturn = PR_FALSE;
  return NS_OK;
}

jsval
nsNavigator::sPrefInternal_id = JSVAL_VOID;

NS_IMETHODIMP
nsNavigator::Preference()
{
  
  
  nsAXPCNativeCallContext *ncc = nsnull;
  nsresult rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(&ncc);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  PRUint32 argc;

  ncc->GetArgc(&argc);

  if (argc == 0) {
    

    return NS_OK;
  }

  jsval *argv = nsnull;

  ncc->GetArgvPtr(&argv);
  NS_ENSURE_TRUE(argv, NS_ERROR_UNEXPECTED);

  JSContext *cx = nsnull;

  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(cx);

  
  if (sPrefInternal_id == JSVAL_VOID) {
    sPrefInternal_id =
      STRING_TO_JSVAL(::JS_InternString(cx, "preferenceinternal"));
  }

  PRUint32 action;
  if (argc == 1) {
    action = nsIXPCSecurityManager::ACCESS_GET_PROPERTY;
  } else {
    action = nsIXPCSecurityManager::ACCESS_SET_PROPERTY;
  }

  rv = nsContentUtils::GetSecurityManager()->
    CheckPropertyAccess(cx, nsnull, "Navigator", sPrefInternal_id, action);
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  nsIPrefBranch *prefBranch = nsContentUtils::GetPrefBranch();
  NS_ENSURE_STATE(prefBranch);

  JSString *str = ::JS_ValueToString(cx, argv[0]);
  NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

  jsval *retval = nsnull;

  rv = ncc->GetRetValPtr(&retval);
  NS_ENSURE_SUCCESS(rv, rv);

  char *prefStr = ::JS_GetStringBytes(str);
  if (argc == 1) {
    PRInt32 prefType;

    prefBranch->GetPrefType(prefStr, &prefType);

    switch (prefType) {
    case nsIPrefBranch::PREF_STRING:
      {
        nsXPIDLCString prefCharVal;
        rv = prefBranch->GetCharPref(prefStr, getter_Copies(prefCharVal));
        NS_ENSURE_SUCCESS(rv, rv);

        JSString *retStr = ::JS_NewStringCopyZ(cx, prefCharVal);
        NS_ENSURE_TRUE(retStr, NS_ERROR_OUT_OF_MEMORY);

        *retval = STRING_TO_JSVAL(retStr);

        break;
      }

    case nsIPrefBranch::PREF_INT:
      {
        PRInt32 prefIntVal;
        rv = prefBranch->GetIntPref(prefStr, &prefIntVal);
        NS_ENSURE_SUCCESS(rv, rv);

        *retval = INT_TO_JSVAL(prefIntVal);

        break;
      }

    case nsIPrefBranch::PREF_BOOL:
      {
        PRBool prefBoolVal;

        rv = prefBranch->GetBoolPref(prefStr, &prefBoolVal);
        NS_ENSURE_SUCCESS(rv, rv);

        *retval = BOOLEAN_TO_JSVAL(prefBoolVal);

        break;
      }
    default:
      {
        

        return ncc->SetReturnValueWasSet(PR_FALSE);
      }
    }

    ncc->SetReturnValueWasSet(PR_TRUE);
  } else {
    if (JSVAL_IS_STRING(argv[1])) {
      JSString *valueJSStr = ::JS_ValueToString(cx, argv[1]);
      NS_ENSURE_TRUE(valueJSStr, NS_ERROR_OUT_OF_MEMORY);

      rv = prefBranch->SetCharPref(prefStr, ::JS_GetStringBytes(valueJSStr));
    } else if (JSVAL_IS_INT(argv[1])) {
      jsint valueInt = JSVAL_TO_INT(argv[1]);

      rv = prefBranch->SetIntPref(prefStr, (PRInt32)valueInt);
    } else if (JSVAL_IS_BOOLEAN(argv[1])) {
      JSBool valueBool = JSVAL_TO_BOOLEAN(argv[1]);

      rv = prefBranch->SetBoolPref(prefStr, (PRBool)valueBool);
    } else if (JSVAL_IS_NULL(argv[1])) {
      rv = prefBranch->DeleteBranch(prefStr);
    }
  }

  return rv;
}

void
nsNavigator::LoadingNewDocument()
{
  
  
  
  mMimeTypes = nsnull;
  mPlugins = nsnull;

  if (mGeolocation)
  {
    mGeolocation->Shutdown();
    mGeolocation = nsnull;
  }
}

nsresult
nsNavigator::RefreshMIMEArray()
{
  nsresult rv = NS_OK;
  if (mMimeTypes)
    rv = mMimeTypes->Refresh();
  return rv;
}





NS_IMETHODIMP
nsNavigator::RegisterContentHandler(const nsAString& aMIMEType, 
                                    const nsAString& aURI, 
                                    const nsAString& aTitle)
{
  nsCOMPtr<nsIWebContentHandlerRegistrar> registrar = 
    do_GetService(NS_WEBCONTENTHANDLERREGISTRAR_CONTRACTID);
  if (registrar && mDocShell) {
    nsCOMPtr<nsIDOMWindow> contentDOMWindow(do_GetInterface(mDocShell));
    if (contentDOMWindow)
      return registrar->RegisterContentHandler(aMIMEType, aURI, aTitle,
                                               contentDOMWindow);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNavigator::RegisterProtocolHandler(const nsAString& aProtocol, 
                                     const nsAString& aURI, 
                                     const nsAString& aTitle)
{
  nsCOMPtr<nsIWebContentHandlerRegistrar> registrar = 
    do_GetService(NS_WEBCONTENTHANDLERREGISTRAR_CONTRACTID);
  if (registrar && mDocShell) {
    nsCOMPtr<nsIDOMWindow> contentDOMWindow(do_GetInterface(mDocShell));
    if (contentDOMWindow)
      return registrar->RegisterProtocolHandler(aProtocol, aURI, aTitle,
                                                contentDOMWindow);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavigator::MozIsLocallyAvailable(const nsAString &aURI,
                                   PRBool aWhenOffline,
                                   PRBool *aIsAvailable)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool match;
  rv = uri->SchemeIs("http", &match);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!match) {
    rv = uri->SchemeIs("https", &match);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!match) return NS_ERROR_DOM_BAD_URI;
  }

  
  nsCOMPtr<nsIJSContextStack> stack = do_GetService(sJSStackContractID);
  NS_ENSURE_TRUE(stack, NS_ERROR_FAILURE);

  JSContext *cx = nsnull;
  rv = stack->Peek(&cx);
  NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);

  rv = nsContentUtils::GetSecurityManager()->CheckSameOrigin(cx, uri);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  PRUint32 loadFlags = nsIChannel::INHIBIT_CACHING |
                       nsICachingChannel::LOAD_NO_NETWORK_IO |
                       nsICachingChannel::LOAD_ONLY_IF_MODIFIED |
                       nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY;

  if (aWhenOffline) {
    loadFlags |= nsICachingChannel::LOAD_CHECK_OFFLINE_CACHE |
                 nsICachingChannel::LOAD_ONLY_FROM_CACHE |
                 nsIRequest::LOAD_FROM_CACHE;
  }

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), uri,
                     nsnull, nsnull, nsnull, loadFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> stream;
  rv = channel->Open(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  stream->Close();

  nsresult status;
  rv = channel->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  if (NS_SUCCEEDED(status)) {
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel);
    rv = httpChannel->GetRequestSucceeded(aIsAvailable);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    *aIsAvailable = PR_FALSE;
  }

  return NS_OK;
}





NS_IMETHODIMP nsNavigator::GetGeolocation(nsIDOMGeoGeolocation **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mGeolocation && mDocShell) {
    nsCOMPtr<nsIDOMWindow> contentDOMWindow(do_GetInterface(mDocShell));
    mGeolocation = new nsGeolocation(contentDOMWindow);
  }

  NS_IF_ADDREF(*_retval = mGeolocation);
  return NS_OK;
}
