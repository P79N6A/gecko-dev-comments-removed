















































#include "base/basictypes.h"


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
#include "nsJSEnvironment.h"
#include "nsCharSeparatedTokenizer.h" 
#include "nsUnicharUtils.h"


#include "nsIEventListenerManager.h"
#include "nsEscape.h"
#include "nsStyleCoord.h"
#include "nsMimeTypeArray.h"
#include "nsNetUtil.h"
#include "nsICachingChannel.h"
#include "nsPluginArray.h"
#include "nsIPluginHost.h"
#include "nsGeolocation.h"
#include "nsDesktopNotification.h"
#include "nsContentCID.h"
#include "nsLayoutStatics.h"
#include "nsCycleCollector.h"
#include "nsCCUncollectableMarker.h"
#include "nsDOMThreadService.h"
#include "nsAutoJSValHolder.h"
#include "nsDOMMediaQueryList.h"


#include "nsIFrame.h"
#include "nsCanvasFrame.h"
#include "nsIWidget.h"
#include "nsIBaseWindow.h"
#include "nsAccelerometer.h"
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
#ifndef MOZ_DISABLE_DOMCRYPTO
#include "nsIDOMCrypto.h"
#endif
#include "nsIDOMDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMEvent.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMessageEvent.h"
#include "nsIDOMPopupBlockedEvent.h"
#include "nsIDOMPopStateEvent.h"
#include "nsIDOMHashChangeEvent.h"
#include "nsIDOMOfflineResourceList.h"
#include "nsIDOMGeoGeolocation.h"
#include "nsIDOMDesktopNotification.h"
#include "nsPIDOMStorage.h"
#include "nsDOMString.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsThreadUtils.h"
#include "nsEventStateManager.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIPrefBranch.h"
#include "nsIPresShell.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIProgrammingLanguage.h"
#include "nsIServiceManager.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScrollableFrame.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsISelectionController.h"
#include "nsISelection.h"
#include "nsIPrompt.h"
#include "nsIPromptService.h"
#include "nsIPromptFactory.h"
#include "nsIWritablePropertyBag2.h"
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
#include "nsFileDataProtocolHandler.h"
#include "nsIDOMFile.h"
#include "nsIURIFixup.h"
#include "mozilla/FunctionTimer.h"
#include "nsCDefaultURIFixup.h"
#include "nsEventDispatcher.h"
#include "nsIObserverService.h"
#include "nsIXULAppInfo.h"
#include "nsNetUtil.h"
#include "nsFocusManager.h"
#include "nsIXULWindow.h"
#include "nsEventStateManager.h"
#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#include "nsIDOMXULControlElement.h"
#include "nsIFrame.h"
#endif

#include "xpcprivate.h"

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
#include "mozilla/dom/Element.h"
#include "nsFrameLoader.h"
#include "nsISupportsPrimitives.h"
#include "nsXPCOMCID.h"

#include "mozilla/FunctionTimer.h"
#include "mozIThirdPartyUtil.h"

#ifdef MOZ_LOGGING

#define FORCE_PR_LOG 1
#endif
#include "prlog.h"

#include "mozilla/dom/indexedDB/IDBFactory.h"
#include "mozilla/dom/indexedDB/IndexedDatabaseManager.h"

#include "nsRefreshDriver.h"
#include "mozAutoDocUpdate.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gDOMLeakPRLog;
#endif

static const char kStorageEnabled[] = "dom.storage.enabled";

using namespace mozilla::dom;
using mozilla::TimeStamp;
using mozilla::TimeDuration;

nsIDOMStorageList *nsGlobalWindow::sGlobalStorageList  = nsnull;
nsGlobalWindow::WindowByIdTable *nsGlobalWindow::sOuterWindowsById = nsnull;

static nsIEntropyCollector *gEntropyCollector          = nsnull;
static PRInt32              gRefCnt                    = 0;
static PRInt32              gOpenPopupSpamCount        = 0;
static PopupControlState    gPopupControlState         = openAbused;
static PRInt32              gRunningTimeoutDepth       = 0;
static PRPackedBool         gMouseDown                 = PR_FALSE;
static PRPackedBool         gDragServiceDisabled       = PR_FALSE;
static FILE                *gDumpFile                  = nsnull;
static PRUint64             gNextWindowID              = 0;
static PRUint32             gSerialCounter             = 0;

#ifdef DEBUG_jst
PRInt32 gTimeoutCnt                                    = 0;
#endif

#if !(defined(NS_DEBUG) || defined(MOZ_ENABLE_JS_DUMP))
static PRBool               gDOMWindowDumpEnabled      = PR_FALSE;
#endif

#if defined(DEBUG_bryner) || defined(DEBUG_chb)
#define DEBUG_PAGE_CACHE
#endif


#define DEFAULT_MIN_TIMEOUT_VALUE 4 // 4ms
#define DEFAULT_MIN_BACKGROUND_TIMEOUT_VALUE 1000 // 1000ms
static PRInt32 gMinTimeoutValue;
static PRInt32 gMinBackgroundTimeoutValue;
inline PRInt32
nsGlobalWindow::DOMMinTimeoutValue() const {
  PRBool isBackground = !mOuterWindow || mOuterWindow->IsBackground();
  return
    NS_MAX(isBackground ? gMinBackgroundTimeoutValue : gMinTimeoutValue, 0);
}



#define DOM_CLAMP_TIMEOUT_NESTING_LEVEL 5




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

#define FORWARD_TO_INNER_CHROME(method, args, err_rval)                       \
  PR_BEGIN_MACRO                                                              \
  if (IsOuterWindow()) {                                                      \
    if (!mInnerWindow) {                                                      \
      NS_WARNING("No inner window available!");                               \
      return err_rval;                                                        \
    }                                                                         \
    return ((nsGlobalChromeWindow *)mInnerWindow)->method args;               \
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
#ifndef MOZ_DISABLE_DOMCRYPTO
static const char kCryptoContractID[] = NS_CRYPTO_CONTRACTID;
static const char kPkcs11ContractID[] = NS_PKCS11_CONTRACTID;
#endif
static const char sPopStatePrefStr[] = "browser.history.allowPopState";

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

  NS_IMETHOD GetURL(const char *aURL, const char *aTarget,
                    nsIInputStream *aPostStream,
                    void *aHeadersData, PRUint32 aHeadersDataLen);
  NS_IMETHOD ShowStatus(const PRUnichar *aStatusMsg);
  NPError ShowNativeContextMenu(NPMenu* menu, void* event);
  NPBool ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                      double *destX, double *destY, NPCoordinateSpace destSpace);
  void SendIdleEvent();

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
  
  
  
  if (mInstance && !aInstance)
    mInstance->InvalidateOwner();

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
nsDummyJavaPluginOwner::SetWindow()
{
  return NS_ERROR_NOT_IMPLEMENTED;
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
                               nsIInputStream *aPostStream,
                               void *aHeadersData, PRUint32 aHeadersDataLen)
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
  return NPERR_GENERIC_ERROR;
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

NS_IMETHODIMP
nsDummyJavaPluginOwner::SetEventModel(PRInt32 eventModel)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsDummyJavaPluginOwner::SendIdleEvent()
{
}




class nsDOMMozURLProperty : public nsIDOMMozURLProperty
{
public:
  nsDOMMozURLProperty(nsGlobalWindow* aWindow)
    : mWindow(aWindow)
  {
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZURLPROPERTY

  void ClearWindowReference() {
    mWindow = nsnull;
  }
private:
  nsGlobalWindow* mWindow;
};

DOMCI_DATA(MozURLProperty, nsDOMMozURLProperty)
NS_IMPL_ADDREF(nsDOMMozURLProperty)
NS_IMPL_RELEASE(nsDOMMozURLProperty)
NS_INTERFACE_MAP_BEGIN(nsDOMMozURLProperty)
    NS_INTERFACE_MAP_ENTRY(nsIDOMMozURLProperty)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMozURLProperty)
    NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozURLProperty)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsDOMMozURLProperty::CreateObjectURL(nsIDOMBlob* aBlob, nsAString& aURL)
{
  NS_PRECONDITION(!mWindow || mWindow->IsInnerWindow(),
                  "Should be inner window");

  NS_ENSURE_STATE(mWindow && mWindow->mDoc);
  NS_ENSURE_ARG_POINTER(aBlob);

  nsIDocument* doc = mWindow->mDoc;

  nsresult rv = aBlob->GetInternalUrl(doc->NodePrincipal(), aURL);
  NS_ENSURE_SUCCESS(rv, rv);

  doc->RegisterFileDataUri(NS_LossyConvertUTF16toASCII(aURL));

  return NS_OK;
}

NS_IMETHODIMP
nsDOMMozURLProperty::RevokeObjectURL(const nsAString& aURL)
{
  NS_PRECONDITION(!mWindow || mWindow->IsInnerWindow(),
                  "Should be inner window");

  NS_ENSURE_STATE(mWindow);

  NS_LossyConvertUTF16toASCII asciiurl(aURL);

  nsIPrincipal* winPrincipal = mWindow->GetPrincipal();
  if (!winPrincipal) {
    return NS_OK;
  }

  nsIPrincipal* principal =
    nsFileDataProtocolHandler::GetFileDataEntryPrincipal(asciiurl);
  PRBool subsumes;
  if (principal && winPrincipal &&
      NS_SUCCEEDED(winPrincipal->Subsumes(principal, &subsumes)) &&
      subsumes) {
    if (mWindow->mDoc) {
      mWindow->mDoc->UnregisterFileDataUri(asciiurl);
    }
    nsFileDataProtocolHandler::RemoveFileDataEntry(asciiurl);
  }

  return NS_OK;
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

nsPIDOMWindow::nsPIDOMWindow(nsPIDOMWindow *aOuterWindow)
: mFrameElement(nsnull), mDocShell(nsnull), mModalStateDepth(0),
  mRunningTimeout(nsnull), mMutationBits(0), mIsDocumentLoaded(PR_FALSE),
  mIsHandlingResizeEvent(PR_FALSE), mIsInnerWindow(aOuterWindow != nsnull),
  mMayHavePaintEventListener(PR_FALSE), mMayHaveTouchEventListener(PR_FALSE),
  mMayHaveAudioAvailableEventListener(PR_FALSE), mIsModalContentWindow(PR_FALSE),
  mIsActive(PR_FALSE), mIsBackground(PR_FALSE),
  mInnerWindow(nsnull), mOuterWindow(aOuterWindow),
  
  mWindowID(++gNextWindowID), mHasNotifiedGlobalCreated(PR_FALSE)
 {}

nsPIDOMWindow::~nsPIDOMWindow() {}





JSString *
nsOuterWindowProxy::obj_toString(JSContext *cx, JSObject *proxy)
{
    JS_ASSERT(proxy->isProxy());

    return JS_NewStringCopyZ(cx, "[object Window]");
}

nsOuterWindowProxy
nsOuterWindowProxy::singleton;

JSObject *
NS_NewOuterWindowProxy(JSContext *cx, JSObject *parent)
{
  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, parent)) {
    return nsnull;
  }

  JSObject *obj = JSWrapper::New(cx, parent, parent->getProto(), parent,
                                 &nsOuterWindowProxy::singleton);
  NS_ASSERTION(obj->getClass()->ext.innerObject, "bad class");
  return obj;
}





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
    mCleanMessageManager(PR_FALSE),
    mNeedsFocus(PR_TRUE),
    mHasFocus(PR_FALSE),
#if defined(XP_MAC) || defined(XP_MACOSX)
    mShowAccelerators(PR_FALSE),
    mShowFocusRings(PR_FALSE),
#else
    mShowAccelerators(PR_TRUE),
    mShowFocusRings(PR_TRUE),
#endif
    mShowFocusRingForContent(PR_FALSE),
    mFocusByKeyOccurred(PR_FALSE),
    mHasAcceleration(PR_FALSE),
    mNotifiedIDDestroyed(PR_FALSE),
    mTimeoutInsertionPoint(nsnull),
    mTimeoutPublicIdCounter(1),
    mTimeoutFiringDepth(0),
    mJSObject(nsnull),
    mPendingStorageEventsObsolete(nsnull),
    mTimeoutsSuspendDepth(0),
    mFocusMethod(0),
    mSerial(0),
#ifdef DEBUG
    mSetOpenerWindowCalled(PR_FALSE),
#endif
    mCleanedUp(PR_FALSE),
    mCallCleanUpAfterModalDialogCloses(PR_FALSE),
    mDialogAbuseCount(0),
    mDialogDisabled(PR_FALSE)
{
  nsLayoutStatics::AddRef();

  
  PR_INIT_CLIST(this);

  
  PR_INIT_CLIST(&mTimeouts);

  if (aOuterWindow) {
    
    
    PR_INSERT_AFTER(this, aOuterWindow);

    mObserver = new nsGlobalWindowObserver(this);
    if (mObserver) {
      NS_ADDREF(mObserver);
      nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
      if (os) {
        
        
        os->AddObserver(mObserver, NS_IOSERVICE_OFFLINE_STATUS_TOPIC,
                        PR_FALSE);

        
        
        os->AddObserver(mObserver, "dom-storage2-changed", PR_FALSE);
        os->AddObserver(mObserver, "dom-storage-changed", PR_FALSE);
      }
    }
  } else {
    
    
    
    Freeze();

    mObserver = nsnull;
    SetIsProxy();

    if (!sOuterWindowsById) {
      sOuterWindowsById = new WindowByIdTable();
      if (!sOuterWindowsById->Init()) {
        delete sOuterWindowsById;
        sOuterWindowsById = nsnull;
      }
    }

    if (sOuterWindowsById) {
      sOuterWindowsById->Put(mWindowID, this);
    }
  }

  
  
  

  gRefCnt++;

  if (gRefCnt == 1) {
#if !(defined(NS_DEBUG) || defined(MOZ_ENABLE_JS_DUMP))
    nsContentUtils::AddBoolPrefVarCache("browser.dom.window.dump.enabled",
                                        &gDOMWindowDumpEnabled);
#endif
    nsContentUtils::AddIntPrefVarCache("dom.min_timeout_value",
                                       &gMinTimeoutValue,
                                       DEFAULT_MIN_TIMEOUT_VALUE);
    nsContentUtils::AddIntPrefVarCache("dom.min_background_timeout_value",
                                       &gMinBackgroundTimeoutValue,
                                       DEFAULT_MIN_BACKGROUND_TIMEOUT_VALUE);
  }

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

  mSerial = ++gSerialCounter;

#ifdef DEBUG
  printf("++DOMWINDOW == %d (%p) [serial = %d] [outer = %p]\n", gRefCnt,
         static_cast<void*>(static_cast<nsIScriptGlobalObject*>(this)),
         gSerialCounter, static_cast<void*>(aOuterWindow));
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
  if (sOuterWindowsById) {
    sOuterWindowsById->Remove(mWindowID);
  }
  if (!--gRefCnt) {
    NS_IF_RELEASE(gEntropyCollector);
    delete sOuterWindowsById;
    sOuterWindowsById = nsnull;
  }
#ifdef DEBUG
  nsCAutoString url;
  if (mLastOpenedURI) {
    mLastOpenedURI->GetSpec(url);
  }

  printf("--DOMWINDOW == %d (%p) [serial = %d] [outer = %p] [url = %s]\n",
         gRefCnt, static_cast<void*>(static_cast<nsIScriptGlobalObject*>(this)),
         mSerial, static_cast<void*>(mOuterWindow.get()), url.get());
#endif

#ifdef PR_LOGGING
  if (gDOMLeakPRLog)
    PR_LOG(gDOMLeakPRLog, PR_LOG_DEBUG,
           ("DOMWINDOW %p destroyed", this));
#endif

  if (IsOuterWindow()) {
    
    
    
    

    nsGlobalWindow *w;
    while ((w = (nsGlobalWindow *)PR_LIST_HEAD(this)) != this) {
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

  CleanUp(PR_TRUE);

#ifdef DEBUG
  nsCycleCollector_DEBUG_wasFreed(static_cast<nsIScriptGlobalObject*>(this));
#endif

  if (mURLProperty) {
    mURLProperty->ClearWindowReference();
  }

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

    nsISupports* supports;
    aWindow->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                            reinterpret_cast<void**>(&supports));
    NS_ASSERTION(supports, "Failed to QI to nsCycleCollectionISupports?!");

    nsContentUtils::DropJSObjects(supports);
  }
}

void
nsGlobalWindow::MaybeForgiveSpamCount()
{
  if (IsOuterWindow() &&
      IsPopupSpamWindow())
  {
    SetPopupSpamWindow(PR_FALSE);
    --gOpenPopupSpamCount;
    NS_ASSERTION(gOpenPopupSpamCount >= 0,
                 "Unbalanced decrement of gOpenPopupSpamCount");
  }
}

void
nsGlobalWindow::CleanUp(PRBool aIgnoreModalDialog)
{
  if (IsOuterWindow() && !aIgnoreModalDialog) {
    nsGlobalWindow* inner = GetCurrentInnerWindowInternal();
    nsCOMPtr<nsIDOMModalContentWindow>
      dlg(do_QueryInterface(static_cast<nsPIDOMWindow*>(inner)));
    if (dlg) {
      
      
      
      mCallCleanUpAfterModalDialogCloses = PR_TRUE;
      return;
    }
  }

  
  if (mCleanedUp)
    return;
  mCleanedUp = PR_TRUE;

  if (mObserver) {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
      os->RemoveObserver(mObserver, NS_IOSERVICE_OFFLINE_STATUS_TOPIC);
      os->RemoveObserver(mObserver, "dom-storage2-changed");
      os->RemoveObserver(mObserver, "dom-storage-changed");
    }

    
    
    mObserver->Forget();
    NS_RELEASE(mObserver);
  }

  mNavigator = nsnull;
  mScreen = nsnull;
  mMenubar = nsnull;
  mToolbar = nsnull;
  mLocationbar = nsnull;
  mPersonalbar = nsnull;
  mStatusbar = nsnull;
  mScrollbars = nsnull;
  mLocation = nsnull;
  mHistory = nsnull;
  mFrames = nsnull;
  mApplicationCache = nsnull;
  mIndexedDB = nsnull;
  mPendingStorageEventsObsolete = nsnull;


  ClearControllers();

  mOpener = nsnull;             
  if (mContext) {
#ifdef DEBUG
    nsCycleCollector_DEBUG_shouldBeFreed(mContext);
#endif
    mContext = nsnull;            
  }
  mChromeEventHandler = nsnull; 
  mParentTarget = nsnull;

  nsGlobalWindow *inner = GetCurrentInnerWindowInternal();

  if (inner) {
    inner->CleanUp(aIgnoreModalDialog);
  }

  DisableAccelerationUpdates();
  mHasAcceleration = PR_FALSE;

  if (mCleanMessageManager) {
    NS_ABORT_IF_FALSE(mIsChrome, "only chrome should have msg manager cleaned");
    nsGlobalChromeWindow *asChrome = static_cast<nsGlobalChromeWindow*>(this);
    if (asChrome->mMessageManager) {
      static_cast<nsFrameMessageManager*>(
        asChrome->mMessageManager.get())->Disconnect();
    }
  }

  mInnerWindowHolder = nsnull;
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
nsGlobalWindow::TryClearWindowScope(nsISupports *aWindow)
{
  nsGlobalWindow *window =
          static_cast<nsGlobalWindow *>(static_cast<nsIDOMWindow*>(aWindow));

  
  
  
  
  window->ClearScopeWhenAllScriptsStop();
}

void
nsGlobalWindow::ClearScopeWhenAllScriptsStop()
{
  NS_ASSERTION(IsInnerWindow(), "Must be an inner window");

  
  
  
  nsIScriptContext *jsscx = GetContextInternal();
  if (jsscx && jsscx->GetExecutingScript()) {
    
    
    
    
    jsscx->SetTerminationFunction(TryClearWindowScope,
                                  static_cast<nsIDOMWindow *>(this));
    return;
  }

  NotifyWindowIDDestroyed("inner-window-destroyed");
  nsIScriptContext *scx = GetContextInternal();
  if (scx) {
    scx->ClearScope(mJSObject, PR_TRUE);
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

  
  indexedDB::IndexedDatabaseManager* idbManager =
    indexedDB::IndexedDatabaseManager::Get();
  if (idbManager) {
    idbManager->AbortCloseDatabasesForWindow(this);
  }

  ClearAllTimeouts();

  mChromeEventHandler = nsnull;

  if (mListenerManager) {
    mListenerManager->Disconnect();
    mListenerManager = nsnull;
  }

  mLocation = nsnull;
  mHistory = nsnull;

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

  mIndexedDB = nsnull;

  if (aClearScope) {
    ClearScopeWhenAllScriptsStop();
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





#define OUTER_WINDOW_ONLY                                                     \
  if (IsOuterWindow()) {

#define END_OUTER_WINDOW_ONLY                                                 \
    foundInterface = 0;                                                       \
  } else

NS_IMPL_CYCLE_COLLECTION_CLASS(nsGlobalWindow)

DOMCI_DATA(Window, nsGlobalWindow)


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
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageIndexedDB)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWindow_2_0_BRANCH)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Window)
  OUTER_WINDOW_ONLY
    NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  END_OUTER_WINDOW_ONLY
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsGlobalWindow)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsGlobalWindow)


NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsGlobalWindow)
  if (tmp->mDoc && nsCCUncollectableMarker::InGeneration(
                     cb, tmp->mDoc->GetMarkedCCGeneration())) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContext)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mControllers)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mArguments)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mArgumentsLast)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mInnerWindowHolder)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOuterWindow)

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
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParentTarget)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFrameElement)

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDummyJavaPluginOwner)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFocusedNode)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mPendingStorageEvents)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsGlobalWindow)
  nsGlobalWindow::CleanupCachedXBLHandlers(tmp);

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContext)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mControllers)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mArguments)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mArgumentsLast)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mInnerWindowHolder)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOuterWindow)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOpenerScriptPrincipal)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mListenerManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSessionStorage)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mApplicationCache)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocumentPrincipal)

  
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChromeEventHandler)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParentTarget)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFrameElement)

  
  if (tmp->mDummyJavaPluginOwner) {
    tmp->mDummyJavaPluginOwner->Destroy();
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDummyJavaPluginOwner)
  }

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFocusedNode)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mPendingStorageEvents)

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
  data->callback(nsIProgrammingLanguage::JAVASCRIPT, aData,
                 "Cached XBL prototype handler", data->closure);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsGlobalWindow)
  if (tmp->mCachedXBLPrototypeHandlers.IsInitialized()) {
    TraceData data(aCallback, aClosure);
    tmp->mCachedXBLPrototypeHandlers.EnumerateRead(TraceXBLHandlers, &data);
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END





nsresult
nsGlobalWindow::SetScriptContext(PRUint32 lang_id, nsIScriptContext *aScriptContext)
{
  NS_ASSERTION(lang_id == nsIProgrammingLanguage::JAVASCRIPT,
               "We don't support this language ID");
  NS_ASSERTION(IsOuterWindow(), "Uh, SetScriptContext() called on inner window!");

  NS_ASSERTION(!aScriptContext || !mContext, "Bad call to SetContext()!");

  if (aScriptContext) {
    
    aScriptContext->WillInitializeContext();

    nsresult rv = aScriptContext->InitContext();
    NS_ENSURE_SUCCESS(rv, rv);

    if (IsFrame()) {
      
      
      

      aScriptContext->SetGCOnDestruction(PR_FALSE);
    }
  }

  mContext = aScriptContext;
  return NS_OK;
}

nsresult
nsGlobalWindow::EnsureScriptEnvironment(PRUint32 aLangID)
{
  NS_ASSERTION(aLangID == nsIProgrammingLanguage::JAVASCRIPT,
               "We don't support this language ID");
  FORWARD_TO_OUTER(EnsureScriptEnvironment, (aLangID), NS_ERROR_NOT_INITIALIZED);

  if (mJSObject)
      return NS_OK;

  NS_ASSERTION(!GetCurrentInnerWindowInternal(),
               "mJSObject is null, but we have an inner window?");

  nsCOMPtr<nsIScriptRuntime> scriptRuntime;
  nsresult rv = NS_GetScriptRuntimeByID(aLangID, getter_AddRefs(scriptRuntime));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIScriptContext> context;
  rv = scriptRuntime->CreateContext(getter_AddRefs(context));
  NS_ENSURE_SUCCESS(rv, rv);

  return SetScriptContext(aLangID, context);
}

nsIScriptContext *
nsGlobalWindow::GetScriptContext(PRUint32 lang)
{
  NS_ASSERTION(lang == nsIProgrammingLanguage::JAVASCRIPT,
               "We don't support this language ID");

  FORWARD_TO_OUTER(GetScriptContext, (lang), nsnull);
  return mContext;
}

void *
nsGlobalWindow::GetScriptGlobal(PRUint32 lang)
{
  NS_ASSERTION(lang == nsIProgrammingLanguage::JAVASCRIPT,
               "We don't support this language ID");
  return mJSObject;
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
  return FastGetGlobalJSObject();
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

    GetDocShell()->CreateAboutBlankContentViewer(aPrincipal);
    mDoc->SetIsInitialDocument(PR_TRUE);

    nsCOMPtr<nsIPresShell> shell;
    GetDocShell()->GetPresShell(getter_AddRefs(shell));

    if (shell && !shell->DidInitialReflow()) {
      
      
      nsRect r = shell->GetPresContext()->GetVisibleArea();
      shell->InitialReflow(r.width, r.height);
    }
  }
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
                    nsIXPConnectJSObjectHolder *aHolder,
                    nsNavigator *aNavigator,
                    nsIXPConnectJSObjectHolder *aOuterProto,
                    nsIXPConnectJSObjectHolder *aOuterRealProto);

  nsGlobalWindow* GetInnerWindow() { return mInnerWindow; }
  nsIXPConnectJSObjectHolder *GetInnerWindowHolder()
  { return mInnerWindowHolder; }

  nsNavigator* GetNavigator() { return mNavigator; }
  nsIXPConnectJSObjectHolder* GetOuterProto() { return mOuterProto; }
  nsIXPConnectJSObjectHolder* GetOuterRealProto() { return mOuterRealProto; }

  void DidRestoreWindow()
  {
    mInnerWindow = nsnull;

    mInnerWindowHolder = nsnull;
    mNavigator = nsnull;
    mOuterProto = nsnull;
    mOuterRealProto = nsnull;
  }

protected:
  ~WindowStateHolder();

  nsGlobalWindow *mInnerWindow;
  
  
  nsCOMPtr<nsIXPConnectJSObjectHolder> mInnerWindowHolder;
  nsRefPtr<nsNavigator> mNavigator;
  nsCOMPtr<nsIXPConnectJSObjectHolder> mOuterProto;
  nsCOMPtr<nsIXPConnectJSObjectHolder> mOuterRealProto;
};

NS_DEFINE_STATIC_IID_ACCESSOR(WindowStateHolder, WINDOWSTATEHOLDER_IID)

WindowStateHolder::WindowStateHolder(nsGlobalWindow *aWindow,
                                     nsIXPConnectJSObjectHolder *aHolder,
                                     nsNavigator *aNavigator,
                                     nsIXPConnectJSObjectHolder *aOuterProto,
                                     nsIXPConnectJSObjectHolder *aOuterRealProto)
  : mInnerWindow(aWindow),
    mNavigator(aNavigator),
    mOuterProto(aOuterProto),
    mOuterRealProto(aOuterRealProto)
{
  NS_PRECONDITION(aWindow, "null window");
  NS_PRECONDITION(aWindow->IsInnerWindow(), "Saving an outer window");

  mInnerWindowHolder = aHolder;

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
                               PRBool aForceReuseInnerWindow)
{
  NS_TIME_FUNCTION;

  NS_PRECONDITION(mDocumentPrincipal == nsnull,
                  "mDocumentPrincipal prematurely set!");

  if (!aDocument) {
    NS_ERROR("SetNewDocument(null) called!");

    return NS_ERROR_INVALID_ARG;
  }

  if (IsInnerWindow()) {
    if (!mOuterWindow) {
      return NS_ERROR_NOT_INITIALIZED;
    }

    
    
    if (mOuterWindow->GetCurrentInnerWindow() != this) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    return GetOuterWindowInternal()->SetNewDocument(aDocument, aState,
                                                    aForceReuseInnerWindow);
  }

  NS_PRECONDITION(IsOuterWindow(), "Must only be called on outer windows");

  if (IsFrozen()) {
    
    

    Thaw();
  }

  NS_ASSERTION(!GetCurrentInnerWindow() ||
               GetCurrentInnerWindow()->GetExtantDocument() == mDocument,
               "Uh, mDocument doesn't match the current inner window "
               "document!");

  PRBool wouldReuseInnerWindow = WouldReuseInnerWindow(aDocument);
  if (aForceReuseInnerWindow &&
      !wouldReuseInnerWindow &&
      mDoc &&
      mDoc->NodePrincipal() != aDocument->NodePrincipal()) {
    NS_ERROR("Attempted forced inner window reuse while changing principal");
    return NS_ERROR_UNEXPECTED;
  }

  nsresult rv = NS_OK;

  nsCOMPtr<nsIDocument> oldDoc(do_QueryInterface(mDocument));

  nsIScriptContext *scx = GetContextInternal();
  NS_ENSURE_TRUE(scx, NS_ERROR_NOT_INITIALIZED);

  JSContext *cx = (JSContext *)scx->GetNativeContext();
#ifndef MOZ_DISABLE_DOMCRYPTO
  
  if (mCrypto) {
    mCrypto->SetEnableSmartCardEvents(PR_FALSE);
  }
#endif
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

  



  nsContentUtils::AddScriptRunner(
    NS_NewRunnableMethod(this, &nsGlobalWindow::ClearStatus));

  PRBool reUseInnerWindow = aForceReuseInnerWindow || wouldReuseInnerWindow;

  
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

  
  
  
  mDocument = do_QueryInterface(aDocument);
  mDoc = aDocument;

#ifdef DEBUG
  mLastOpenedURI = aDocument->GetDocumentURI();
#endif

  mContext->WillInitializeContext();

  nsGlobalWindow *currentInner = GetCurrentInnerWindowInternal();

  nsRefPtr<nsGlobalWindow> newInnerWindow;

  PRBool thisChrome = IsChromeWindow();
  nsCOMPtr<nsIXPConnectJSObjectHolder> navigatorHolder;
  jsval nav;

  PRBool isChrome = PR_FALSE;

  nsCxPusher cxPusher;
  if (!cxPusher.Push(cx)) {
    return NS_ERROR_FAILURE;
  }

  JSAutoRequest ar(cx);

  nsCOMPtr<WindowStateHolder> wsh = do_QueryInterface(aState);
  NS_ASSERTION(!aState || wsh, "What kind of weird state are you giving me here?");

  
  
  
  
  mContext->ClearScope(mJSObject, PR_FALSE);

  
  
  
  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  if (reUseInnerWindow) {
    
    NS_ASSERTION(!currentInner->IsFrozen(),
                 "We should never be reusing a shared inner window");
    newInnerWindow = currentInner;

    if (aDocument != oldDoc) {
      nsWindowSH::InvalidateGlobalScopePolluter(cx, currentInner->mJSObject);
    }

    
    
    
    
    
    if (!JS_TransplantObject(cx, mJSObject, mJSObject)) {
      return NS_ERROR_FAILURE;
    }
  } else {
    if (aState) {
      newInnerWindow = wsh->GetInnerWindow();
      mInnerWindowHolder = wsh->GetInnerWindowHolder();
      
      NS_ASSERTION(newInnerWindow, "Got a state without inner window");

      
      mNavigator = wsh->GetNavigator();

      if (mNavigator) {
        
        mNavigator->SetDocShell(mDocShell);
        mNavigator->LoadingNewDocument();
      }
    } else if (thisChrome) {
      newInnerWindow = new nsGlobalChromeWindow(this);
      isChrome = PR_TRUE;
    } else if (mIsModalContentWindow) {
      newInnerWindow = new nsGlobalModalWindow(this);
    } else {
      newInnerWindow = new nsGlobalWindow(this);
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
      
      
      void *&newGlobal = (void *&)newInnerWindow->mJSObject;
      nsCOMPtr<nsIXPConnectJSObjectHolder> &holder = mInnerWindowHolder;
      rv = mContext->CreateNativeGlobalForInner(sgo, isChrome,
                                                aDocument->NodePrincipal(),
                                                &newGlobal,
                                                getter_AddRefs(holder));
      NS_ASSERTION(NS_SUCCEEDED(rv) && newGlobal && holder,
                   "Failed to get script global and holder");

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

    if (!mJSObject) {
      mContext->CreateOuterObject(this, newInnerWindow);
      mContext->DidInitializeContext();

      mJSObject = (JSObject *)mContext->GetNativeGlobal();
      SetWrapper(mJSObject);
    } else {
      JSObject *outerObject =
        NS_NewOuterWindowProxy(cx, newInnerWindow->mJSObject);
      if (!outerObject) {
        NS_ERROR("out of memory");
        return NS_ERROR_FAILURE;
      }

      outerObject = JS_TransplantObject(cx, mJSObject, outerObject);
      if (!outerObject) {
        NS_ERROR("unable to transplant wrappers, probably OOM");
        return NS_ERROR_FAILURE;
      }

      mJSObject = outerObject;
      SetWrapper(mJSObject);

      {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, mJSObject)) {
          NS_ERROR("unable to enter a compartment");
          return NS_ERROR_FAILURE;
        }

        JS_SetParent(cx, mJSObject, newInnerWindow->mJSObject);

        mContext->SetOuterObject(mJSObject);
      }
    }

    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, mJSObject)) {
      NS_ERROR("unable to enter a compartment");
      return NS_ERROR_FAILURE;
    }

    
    if (aState) {
      JSObject *proto;
      if (nsIXPConnectJSObjectHolder *holder = wsh->GetOuterRealProto()) {
        holder->GetJSObject(&proto);
      } else {
        proto = nsnull;
      }

      if (!JS_SetPrototype(cx, mJSObject, proto)) {
        NS_ERROR("can't set prototype");
        return NS_ERROR_FAILURE;
      }
    } else {
      if (!JS_DefineProperty(cx, newInnerWindow->mJSObject, "window",
                             OBJECT_TO_JSVAL(mJSObject),
                             JS_PropertyStub, JS_StrictPropertyStub,
                             JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)) {
        NS_ERROR("can't create the 'window' property");
        return NS_ERROR_FAILURE;
      }
    }
  }

  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, mJSObject)) {
    NS_ERROR("unable to enter a compartment");
    return NS_ERROR_FAILURE;
  }

  if (!aState && !reUseInnerWindow) {
    
    

    
    
    mContext->ConnectToInner(newInnerWindow, mJSObject);

    nsCOMPtr<nsIContent> frame = do_QueryInterface(GetFrameElementInternal());
    if (frame && frame->GetOwnerDoc()) {
      nsPIDOMWindow* parentWindow = frame->GetOwnerDoc()->GetWindow();
      if (parentWindow && parentWindow->TimeoutSuspendCount()) {
        SuspendTimeouts(parentWindow->TimeoutSuspendCount());
      }
    }
  }

  
  
  nsCOMPtr<nsIScriptContext> kungFuDeathGrip(mContext);
  nsCOMPtr<nsIDOMDocument> dd(do_QueryInterface(aDocument));
  mContext->DidSetDocument(dd, newInnerWindow->mJSObject);

  
  
  
  
  
  
  
  
  
  

  if ((!reUseInnerWindow || aDocument != oldDoc) && !aState) {
    nsCOMPtr<nsIHTMLDocument> html_doc(do_QueryInterface(mDocument));
    nsWindowSH::InstallGlobalScopePolluter(cx, newInnerWindow->mJSObject,
                                           html_doc);
  }

  if (aDocument) {
    aDocument->SetScriptGlobalObject(newInnerWindow);
  }

  if (!aState) {
    if (reUseInnerWindow) {
      if (newInnerWindow->mDoc != aDocument) {
        newInnerWindow->mDocument = do_QueryInterface(aDocument);
        newInnerWindow->mDoc = aDocument;

        
        
        

        
        ::JS_DeleteProperty(cx, currentInner->mJSObject, "document");
      }
    } else {
      rv = newInnerWindow->InnerSetNewDocument(aDocument);
      NS_ENSURE_SUCCESS(rv, rv);

      
      rv = mContext->InitClasses(newInnerWindow->mJSObject);
      NS_ENSURE_SUCCESS(rv, rv);

      if (navigatorHolder) {
        JS_ASSERT(JSVAL_IS_OBJECT(nav));

        if (JSVAL_TO_OBJECT(nav)->compartment() == newInnerWindow->mJSObject->compartment()) {
          

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

    if (mArguments) {
      newInnerWindow->DefineArgumentsProperty(mArguments);
      newInnerWindow->mArguments = mArguments;
      newInnerWindow->mArgumentsOrigin = mArgumentsOrigin;

      mArguments = nsnull;
      mArgumentsOrigin = nsnull;
    }

    
    
    newInnerWindow->mChromeEventHandler = mChromeEventHandler;
  }

  mContext->GC();
  mContext->DidInitializeContext();

  if (newInnerWindow && !newInnerWindow->mHasNotifiedGlobalCreated && mDoc) {
    
    
    
    
    nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(mDocShell));
    PRInt32 itemType = nsIDocShellTreeItem::typeContent;
    if (treeItem) {
      treeItem->GetItemType(&itemType);
    }

    if (itemType != nsIDocShellTreeItem::typeChrome ||
        nsContentUtils::IsSystemPrincipal(mDoc->NodePrincipal())) {
      newInnerWindow->mHasNotifiedGlobalCreated = PR_TRUE;
      nsContentUtils::AddScriptRunner(
        NS_NewRunnableMethod(this, &nsGlobalWindow::DispatchDOMWindowCreated));
    }
  }

  return NS_OK;
}

void
nsGlobalWindow::DispatchDOMWindowCreated()
{
  if (!mDoc || !mDocument) {
    return;
  }

  
  nsContentUtils::DispatchChromeEvent(mDoc, mDocument, NS_LITERAL_STRING("DOMWindowCreated"),
                                      PR_TRUE ,
                                      PR_FALSE );

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    nsAutoString origin;
    nsIPrincipal* principal = mDoc->NodePrincipal();
    nsContentUtils::GetUTFOrigin(principal, origin);
    observerService->
      NotifyObservers(static_cast<nsIDOMWindow*>(this),
                      nsContentUtils::IsSystemPrincipal(principal) ?
                        "chrome-document-global-created" :
                        "content-document-global-created",
                      origin.get());
  }
}

void
nsGlobalWindow::ClearStatus()
{
  SetStatus(EmptyString());
  SetDefaultStatus(EmptyString());
}

nsresult
nsGlobalWindow::InnerSetNewDocument(nsIDocument* aDocument)
{
  NS_PRECONDITION(IsInnerWindow(), "Must only be called on inner windows");

#ifdef PR_LOGGING
  if (aDocument && gDOMLeakPRLog &&
      PR_LOG_TEST(gDOMLeakPRLog, PR_LOG_DEBUG)) {
    nsIURI *uri = aDocument->GetDocumentURI();
    nsCAutoString spec;
    if (uri)
      uri->GetSpec(spec);
    PR_LogPrint("DOMWINDOW %p SetNewDocument %s", this, spec.get());
  }
#endif

  mDocument = do_QueryInterface(aDocument);
  mDoc = aDocument;
  mLocalStorage = nsnull;
  mSessionStorage = nsnull;

#ifdef DEBUG
  mLastOpenedURI = aDocument->GetDocumentURI();
#endif

  
  mMutationBits = 0;

  return NS_OK;
}

void
nsGlobalWindow::SetDocShell(nsIDocShell* aDocShell)
{
  NS_ASSERTION(IsOuterWindow(), "Uh, SetDocShell() called on inner window!");

  if (aDocShell == mDocShell)
    return;

  
  
  
  
  

  if (!aDocShell) {
    NS_ASSERTION(PR_CLIST_IS_EMPTY(&mTimeouts),
                 "Uh, outer window holds timeouts!");

    
    
    
    for (nsRefPtr<nsGlobalWindow> inner = (nsGlobalWindow *)PR_LIST_HEAD(this);
         inner != this;
         inner = (nsGlobalWindow*)PR_NEXT_LINK(inner)) {
      NS_ASSERTION(!inner->mOuterWindow || inner->mOuterWindow == this,
                   "bad outer window pointer");
      inner->FreeInnerObjects(PR_TRUE);
    }

    
    NotifyDOMWindowDestroyed(this);

    NotifyWindowIDDestroyed("outer-window-destroyed");

    nsGlobalWindow *currentInner = GetCurrentInnerWindowInternal();

    if (currentInner) {
      NS_ASSERTION(mDoc, "Must have doc!");
      
      
      mDocumentPrincipal = mDoc->NodePrincipal();

      
      mDocument = nsnull;
      mDoc = nsnull;
    }

    if (mContext) {
      mContext->ClearScope(mJSObject, PR_TRUE);
    }

    ClearControllers();

    mChromeEventHandler = nsnull; 

    if (mArguments) { 
      
      
      mArguments = nsnull;
      mArgumentsLast = nsnull;
      mArgumentsOrigin = nsnull;
    }

    if (mContext) {
      mContext->GC();
      mContext->FinalizeContext();
      mContext = nsnull;
    }

#ifdef DEBUG
    nsCycleCollector_DEBUG_shouldBeFreed(mContext);
    nsCycleCollector_DEBUG_shouldBeFreed(static_cast<nsIScriptGlobalObject*>(this));
#endif
  }

  mDocShell = aDocShell;        

  if (mNavigator)
    mNavigator->SetDocShell(aDocShell);
  if (mFrames)
    mFrames->SetDocShell(aDocShell);
  if (mScreen)
    mScreen->SetDocShell(aDocShell);

  
  nsCOMPtr<nsIWebBrowserChrome> browserChrome;
  GetWebBrowserChrome(getter_AddRefs(browserChrome));
  if (mMenubar) {
    mMenubar->SetWebBrowserChrome(browserChrome);
  }
  if (mToolbar) {
    mToolbar->SetWebBrowserChrome(browserChrome);
  }
  if (mLocationbar) {
    mLocationbar->SetWebBrowserChrome(browserChrome);
  }
  if (mPersonalbar) {
    mPersonalbar->SetWebBrowserChrome(browserChrome);
  }
  if (mStatusbar) {
    mStatusbar->SetWebBrowserChrome(browserChrome);
  }
  if (mScrollbars) {
    mScrollbars->SetWebBrowserChrome(browserChrome);
  }

  if (!mDocShell) {
    MaybeForgiveSpamCount();
    CleanUp(PR_FALSE);
  } else {
    
    
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

    PRBool docShellActive;
    mDocShell->GetIsActive(&docShellActive);
    mIsBackground = !docShellActive;
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

void
nsGlobalWindow::UpdateParentTarget()
{
  nsCOMPtr<nsIFrameLoaderOwner> flo = do_QueryInterface(mChromeEventHandler);
  if (flo) {
    nsRefPtr<nsFrameLoader> fl = flo->GetFrameLoader();
    if (fl) {
      mParentTarget = fl->GetTabChildGlobalAsEventTarget();
    }
  }
  if (!mParentTarget) {
    mParentTarget = mChromeEventHandler;
  }
}

PRBool
nsGlobalWindow::GetIsTabModalPromptAllowed()
{
  PRBool allowTabModal = PR_TRUE;
  if (mDocShell) {
    nsCOMPtr<nsIContentViewer> cv;
    mDocShell->GetContentViewer(getter_AddRefs(cv));
    cv->GetIsTabModalPromptAllowed(&allowTabModal);
  }

  return allowTabModal;
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

  aVisitor.mParentTarget = GetParentTarget();
  return NS_OK;
}

bool
nsGlobalWindow::DialogOpenAttempted()
{
  nsGlobalWindow *topWindow = GetTop();
  if (!topWindow) {
    NS_ERROR("DialogOpenAttempted() called without a top window?");

    return false;
  }

  topWindow = topWindow->GetCurrentInnerWindowInternal();
  if (!topWindow ||
      topWindow->mLastDialogQuitTime.IsNull() ||
      nsContentUtils::IsCallerTrustedForCapability("UniversalXPConnect")) {
    return false;
  }

  TimeDuration dialogDuration(TimeStamp::Now() -
                              topWindow->mLastDialogQuitTime);

  if (dialogDuration.ToSeconds() <
      nsContentUtils::GetIntPref("dom.successive_dialog_time_limit",
                                 SUCCESSIVE_DIALOG_TIME_LIMIT)) {
    topWindow->mDialogAbuseCount++;

    return (topWindow->GetPopupControlState() > openAllowed ||
            topWindow->mDialogAbuseCount > MAX_DIALOG_COUNT);
  }

  topWindow->mDialogAbuseCount = 0;

  return false;
}

bool
nsGlobalWindow::AreDialogsBlocked()
{
  nsGlobalWindow *topWindow = GetTop();
  if (!topWindow) {
    NS_ASSERTION(!mDocShell, "AreDialogsBlocked() called without a top window?");

    return true;
  }

  topWindow = topWindow->GetCurrentInnerWindowInternal();

  return !topWindow ||
         (topWindow->mDialogDisabled &&
          (topWindow->GetPopupControlState() > openAllowed ||
           topWindow->mDialogAbuseCount >= MAX_DIALOG_COUNT));
}

bool
nsGlobalWindow::ConfirmDialogAllowed()
{
  FORWARD_TO_OUTER(ConfirmDialogAllowed, (), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_TRUE(mDocShell, false);
  nsCOMPtr<nsIPromptService> promptSvc =
    do_GetService("@mozilla.org/embedcomp/prompt-service;1");

  if (!DialogOpenAttempted() || !promptSvc) {
    return true;
  }

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

  PRBool disableDialog = PR_FALSE;
  nsXPIDLString label, title;
  nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                     "ScriptDialogLabel", label);
  nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                     "ScriptDialogPreventTitle", title);
  promptSvc->Confirm(this, title.get(), label.get(), &disableDialog);
  if (disableDialog) {
    PreventFurtherDialogs();
    return false;
  }

  return true;
}

void
nsGlobalWindow::PreventFurtherDialogs()
{
  nsGlobalWindow *topWindow = GetTop();
  if (!topWindow) {
    NS_ERROR("PreventFurtherDialogs() called without a top window?");

    return;
  }

  topWindow = topWindow->GetCurrentInnerWindowInternal();

  if (topWindow)
    topWindow->mDialogDisabled = PR_TRUE;
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
nsGlobalWindow::OnFinalize(JSObject* aObject)
{
  if (aObject == mJSObject) {
    mJSObject = NULL;
  }
}

void
nsGlobalWindow::SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts)
{
  FORWARD_TO_INNER_VOID(SetScriptsEnabled, (aEnabled, aFireTimeouts));

  if (aEnabled && aFireTimeouts) {
    
    
    void (nsGlobalWindow::*run)() = &nsGlobalWindow::RunTimeout;
    NS_DispatchToCurrentThread(NS_NewRunnableMethod(this, run));
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

  return GetContextInternal()->SetProperty(mJSObject, "arguments", aArguments);
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
  FORWARD_TO_INNER(GetHistory, (aHistory), NS_ERROR_NOT_INITIALIZED);

  *aHistory = nsnull;

  if (!mHistory) {
    mHistory = new nsHistory(this);
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

  *aTop = nsnull;
  if (mDocShell) {
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
    nsCOMPtr<nsIDocShellTreeItem> root;
    docShellAsItem->GetSameTypeRootTreeItem(getter_AddRefs(root));

    if (root) {
      nsCOMPtr<nsIDOMWindow> top(do_GetInterface(root));
      top.swap(*aTop);
    }
  }

  return NS_OK;
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
nsGlobalWindow::CreateBlobURL(nsIDOMBlob* aBlob, nsAString& aURL)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsGlobalWindow::RevokeBlobURL(const nsAString& aURL)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsGlobalWindow::GetCrypto(nsIDOMCrypto** aCrypto)
{
#ifdef MOZ_DISABLE_DOMCRYPTO
  return NS_ERROR_NOT_IMPLEMENTED;
#else
  FORWARD_TO_OUTER(GetCrypto, (aCrypto), NS_ERROR_NOT_INITIALIZED);

  if (!mCrypto) {
    mCrypto = do_CreateInstance(kCryptoContractID);
  }

  NS_IF_ADDREF(*aCrypto = mCrypto);

  return NS_OK;
#endif
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

  nsCOMPtr<nsPIDOMWindow> opener = do_QueryReferent(mOpener);
  if (!opener) {
    return NS_OK;
  }

  
  if (nsContentUtils::IsCallerTrustedForRead()) {
    NS_ADDREF(*aOpener = opener);
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> openerPwin(do_QueryInterface(opener));
  if (!openerPwin) {
    return NS_OK;
  }

  
  nsGlobalWindow *win = static_cast<nsGlobalWindow *>(openerPwin.get());
  if (win->IsChromeWindow()) {
    return NS_OK;
  }

  
  
  
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

  nsRefPtr<nsPresContext> presContext;
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

  nsRefPtr<nsPresContext> presContext;
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

  nsRefPtr<nsPresContext> presContext;
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

  nsRefPtr<nsPresContext> presContext;
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

  nsRefPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));

  if (presContext) {
    nsRect shellArea = presContext->GetVisibleArea();
    *aInnerWidth = nsPresContext::AppUnitsToIntCSSPixels(shellArea.width);
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

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(&aInnerWidth, nsnull),
                    NS_ERROR_FAILURE);


  nsRefPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));

  if (presShell && presShell->GetIsViewportOverridden())
  {
    nscoord height = 0;
    nscoord width  = 0;

    nsRefPtr<nsPresContext> presContext;
    presContext = presShell->GetPresContext();

    nsRect shellArea = presContext->GetVisibleArea();
    height = shellArea.height;
    width  = nsPresContext::CSSPixelsToAppUnits(aInnerWidth);
    return SetCSSViewportWidthAndHeight(width, height);
  }
  else
  {
    PRInt32 height = 0;
    PRInt32 width  = 0;

    nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(mDocShell));
    docShellAsWin->GetSize(&width, &height);
    width  = CSSToDevIntPixels(aInnerWidth);
    return SetDocShellWidthAndHeight(width, height);
  }
}

NS_IMETHODIMP
nsGlobalWindow::GetInnerHeight(PRInt32* aInnerHeight)
{
  FORWARD_TO_OUTER(GetInnerHeight, (aInnerHeight), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_STATE(mDocShell);

  EnsureSizeUpToDate();

  nsRefPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));

  if (presContext) {
    nsRect shellArea = presContext->GetVisibleArea();
    *aInnerHeight = nsPresContext::AppUnitsToIntCSSPixels(shellArea.height);
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

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(nsnull, &aInnerHeight),
                    NS_ERROR_FAILURE);

  nsRefPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));

  if (presShell && presShell->GetIsViewportOverridden())
  {
    nscoord height = 0;
    nscoord width  = 0;

    nsRefPtr<nsPresContext> presContext;
    presContext = presShell->GetPresContext();

    nsRect shellArea = presContext->GetVisibleArea();
    width = shellArea.width;
    height  = nsPresContext::CSSPixelsToAppUnits(aInnerHeight);
    return SetCSSViewportWidthAndHeight(width, height);
  }
  else
  {
    PRInt32 height = 0;
    PRInt32 width  = 0;

    nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(mDocShell));
    docShellAsWin->GetSize(&width, &height);
    height  = CSSToDevIntPixels(aInnerHeight);
    return SetDocShellWidthAndHeight(width, height);
  }
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
nsGlobalWindow::GetMozPaintCount(PRUint64* aResult)
{
  FORWARD_TO_OUTER(GetMozPaintCount, (aResult), NS_ERROR_NOT_INITIALIZED);

  *aResult = 0;

  if (!mDocShell)
    return NS_OK;

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell)
    return NS_OK;

  *aResult = presShell->GetPaintCount();
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::MozRequestAnimationFrame(nsIAnimationFrameListener* aListener)
{
  FORWARD_TO_INNER(MozRequestAnimationFrame, (aListener),
                   NS_ERROR_NOT_INITIALIZED);

  if (!mDoc) {
    return NS_OK;
  }

  mDoc->ScheduleBeforePaintEvent(aListener);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetMozAnimationStartTime(PRInt64 *aTime)
{
  FORWARD_TO_INNER(GetMozAnimationStartTime, (aTime), NS_ERROR_NOT_INITIALIZED);

  if (mDoc) {
    nsIPresShell* presShell = mDoc->GetShell();
    if (presShell) {
      *aTime = presShell->GetPresContext()->RefreshDriver()->
        MostRecentRefreshEpochTime() / PR_USEC_PER_MSEC;
      return NS_OK;
    }
  }

  
  *aTime = JS_Now() / PR_USEC_PER_MSEC;
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::MatchMedia(const nsAString& aMediaQueryList,
                           nsIDOMMediaQueryList** aResult)
{
  
  
  
  
  FORWARD_TO_OUTER(MatchMedia, (aMediaQueryList, aResult),
                   NS_ERROR_NOT_INITIALIZED);

  *aResult = nsnull;

  if (!mDocShell)
    return NS_OK;

  nsRefPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));

  if (!presContext)
    return NS_OK;

  presContext->MatchMedia(aMediaQueryList, aResult);
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
nsGlobalWindow::SetDocShellWidthAndHeight(PRInt32 aInnerWidth, PRInt32 aInnerHeight)
{
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(treeOwner->SizeShellTo(docShellAsItem, aInnerWidth, aInnerHeight),
                    NS_ERROR_FAILURE);

  return NS_OK;
}


nsresult
nsGlobalWindow::SetCSSViewportWidthAndHeight(nscoord aInnerWidth, nscoord aInnerHeight)
{
  nsRefPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));

  nsRect shellArea = presContext->GetVisibleArea();
  shellArea.height = aInnerHeight;
  shellArea.width = aInnerWidth;

  presContext->SetVisibleArea(shellArea);
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

    nsGlobalWindow* rootWindow =
      static_cast<nsGlobalWindow*>(GetPrivateRoot());
    if (rootWindow) {
      rootWindow->FlushPendingNotifications(Flush_Layout);
    }

    nsCOMPtr<nsIBaseWindow> treeOwner;
    GetTreeOwner(getter_AddRefs(treeOwner));

    nsCOMPtr<nsIDOMScreen> screen;
    GetScreen(getter_AddRefs(screen));

    if (treeOwner && screen) {
      PRInt32 screenLeft, screenTop, screenWidth, screenHeight;
      PRInt32 winLeft, winTop, winWidth, winHeight;

      
      treeOwner->GetPositionAndSize(&winLeft, &winTop, &winWidth, &winHeight);

      
      
      winLeft   = DevToCSSIntPixels(winLeft);
      winTop    = DevToCSSIntPixels(winTop);
      winWidth  = DevToCSSIntPixels(winWidth);
      winHeight = DevToCSSIntPixels(winHeight);

      
      
      screen->GetAvailLeft(&screenLeft);
      screen->GetAvailWidth(&screenWidth);
      screen->GetAvailHeight(&screenHeight);
#if defined(XP_MAC) || defined(XP_MACOSX)
      






      screen->GetTop(&screenTop);
#else
      screen->GetAvailTop(&screenTop);
#endif

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

  FlushPendingNotifications(Flush_Layout);
  nsIScrollableFrame *sf = GetScrollFrame();
  if (!sf)
    return NS_OK;

  nsRect scrollRange = sf->GetScrollRange();

  if (aScrollMaxX)
    *aScrollMaxX = NS_MAX(0,
      (PRInt32)floor(nsPresContext::AppUnitsToFloatCSSPixels(scrollRange.XMost())));
  if (aScrollMaxY)
    *aScrollMaxY = NS_MAX(0,
      (PRInt32)floor(nsPresContext::AppUnitsToFloatCSSPixels(scrollRange.YMost())));

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

  if (aDoFlush) {
    FlushPendingNotifications(Flush_Layout);
  } else {
    EnsureSizeUpToDate();
  }

  nsIScrollableFrame *sf = GetScrollFrame();
  if (!sf)
    return NS_OK;

  nsPoint scrollPos = sf->GetScrollPosition();
  if (scrollPos != nsPoint(0,0) && !aDoFlush) {
    
    
    
    return GetScrollXY(aScrollX, aScrollY, PR_TRUE);
  }

  if (aScrollX)
    *aScrollX = nsPresContext::AppUnitsToIntCSSPixels(scrollPos.x);
  if (aScrollY)
    *aScrollY = nsPresContext::AppUnitsToIntCSSPixels(scrollPos.y);

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

  
  if (mFullScreen == aFullScreen)
    return NS_OK;

  
  
  if (!DispatchCustomEvent("fullscreen")) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  nsCOMPtr<nsIXULWindow> xulWin(do_GetInterface(treeOwnerAsWin));
  if (aFullScreen && xulWin) {
    xulWin->SetIntrinsicallySized(PR_FALSE);
  }

  
  
  mFullScreen = aFullScreen;

  nsCOMPtr<nsIWidget> widget = GetMainWidget();
  if (widget)
    widget->MakeFullScreen(aFullScreen);

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
  
  char *c = cstr, *cEnd = cstr + strlen(cstr);
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
  NS_ASSERTION(IsOuterWindow(), "EnsureReflowFlushAndPaint() must be called on"
               "the outer window");
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

  if (AreDialogsBlocked())
    return NS_ERROR_NOT_AVAILABLE;

  
  
  PRBool shouldEnableDisableDialog = DialogOpenAttempted();

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

  
  

  NS_NAMED_LITERAL_STRING(null_str, "null");

  const nsAString *str = DOMStringIsNull(aString) ? &null_str : &aString;

  
  
  EnsureReflowFlushAndPaint();

  nsAutoString title;
  MakeScriptDialogTitle(title);

  
  
  nsAutoString final;
  nsContentUtils::StripNullChars(*str, final);

  
  
  PRBool allowTabModal = GetIsTabModalPromptAllowed();

  nsresult rv;
  nsCOMPtr<nsIPromptFactory> promptFac =
    do_GetService("@mozilla.org/prompter;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrompt> prompt;
  rv = promptFac->GetPrompt(this, NS_GET_IID(nsIPrompt),
                            reinterpret_cast<void**>(&prompt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritablePropertyBag2> promptBag = do_QueryInterface(prompt);
  if (promptBag)
    promptBag->SetPropertyAsBool(NS_LITERAL_STRING("allowTabModal"), allowTabModal);

  if (shouldEnableDisableDialog) {
    PRBool disallowDialog = PR_FALSE;
    nsXPIDLString label;
    nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                       "ScriptDialogLabel", label);

    rv = prompt->AlertCheck(title.get(), final.get(), label.get(),
                            &disallowDialog);
    if (disallowDialog)
      PreventFurtherDialogs();
  } else {
    rv = prompt->Alert(title.get(), final.get());
  }

  return rv;
}

NS_IMETHODIMP
nsGlobalWindow::Confirm(const nsAString& aString, PRBool* aReturn)
{
  FORWARD_TO_OUTER(Confirm, (aString, aReturn), NS_ERROR_NOT_INITIALIZED);

  if (AreDialogsBlocked())
    return NS_ERROR_NOT_AVAILABLE;

  
  
  PRBool shouldEnableDisableDialog = DialogOpenAttempted();

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

  *aReturn = PR_FALSE;

  
  
  EnsureReflowFlushAndPaint();

  nsAutoString title;
  MakeScriptDialogTitle(title);

  
  
  nsAutoString final;
  nsContentUtils::StripNullChars(aString, final);

  
  
  PRBool allowTabModal = GetIsTabModalPromptAllowed();

  nsresult rv;
  nsCOMPtr<nsIPromptFactory> promptFac =
    do_GetService("@mozilla.org/prompter;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrompt> prompt;
  rv = promptFac->GetPrompt(this, NS_GET_IID(nsIPrompt),
                            reinterpret_cast<void**>(&prompt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritablePropertyBag2> promptBag = do_QueryInterface(prompt);
  if (promptBag)
    promptBag->SetPropertyAsBool(NS_LITERAL_STRING("allowTabModal"), allowTabModal);

  if (shouldEnableDisableDialog) {
    PRBool disallowDialog = PR_FALSE;
    nsXPIDLString label;
    nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                       "ScriptDialogLabel", label);

    rv = prompt->ConfirmCheck(title.get(), final.get(), label.get(),
                              &disallowDialog, aReturn);
    if (disallowDialog)
      PreventFurtherDialogs();
  } else {
    rv = prompt->Confirm(title.get(), final.get(), aReturn);
  }

  return rv;
}

NS_IMETHODIMP
nsGlobalWindow::Prompt(const nsAString& aMessage, const nsAString& aInitial,
                       nsAString& aReturn)
{
  FORWARD_TO_OUTER(Prompt, (aMessage, aInitial, aReturn),
                   NS_ERROR_NOT_INITIALIZED);

  SetDOMStringToNull(aReturn);

  if (AreDialogsBlocked())
    return NS_ERROR_NOT_AVAILABLE;

  
  
  PRBool shouldEnableDisableDialog = DialogOpenAttempted();

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, PR_TRUE);

  
  
  EnsureReflowFlushAndPaint();

  nsAutoString title;
  MakeScriptDialogTitle(title);
  
  
  
  nsAutoString fixedMessage, fixedInitial;
  nsContentUtils::StripNullChars(aMessage, fixedMessage);
  nsContentUtils::StripNullChars(aInitial, fixedInitial);

  
  
  PRBool allowTabModal = GetIsTabModalPromptAllowed();

  nsresult rv;
  nsCOMPtr<nsIPromptFactory> promptFac =
    do_GetService("@mozilla.org/prompter;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrompt> prompt;
  rv = promptFac->GetPrompt(this, NS_GET_IID(nsIPrompt),
                            reinterpret_cast<void**>(&prompt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritablePropertyBag2> promptBag = do_QueryInterface(prompt);
  if (promptBag)
    promptBag->SetPropertyAsBool(NS_LITERAL_STRING("allowTabModal"), allowTabModal);

  
  PRUnichar *inoutValue = ToNewUnicode(fixedInitial);
  PRBool disallowDialog = PR_FALSE;

  nsXPIDLString label;
  if (shouldEnableDisableDialog) {
    nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                       "ScriptDialogLabel", label);
  }

  PRBool ok;
  rv = prompt->Prompt(title.get(), fixedMessage.get(),
                      &inoutValue, label.get(), &disallowDialog, &ok);

  if (disallowDialog) {
    PreventFurtherDialogs();
  }

  NS_ENSURE_SUCCESS(rv, rv);

  nsAdoptingString outValue(inoutValue);

  if (ok && outValue) {
    aReturn.Assign(outValue);
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

  nsIDOMWindowInternal *caller =
    static_cast<nsIDOMWindowInternal*>(nsContentUtils::GetWindowFromCaller());
  nsCOMPtr<nsIDOMWindowInternal> opener;
  GetOpener(getter_AddRefs(opener));

  
  
  
  PRBool canFocus = CanSetProperty("dom.disable_window_flip") ||
                    (opener == caller &&
                     RevisePopupAbuseLevel(gPopupControlState) < openAbused);

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

  
  
  if (!CanSetProperty("dom.disable_window_flip")) {
    return NS_OK;
  }

  
  
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
      if (content == doc->GetRootElement())
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

  if (AreDialogsBlocked() || !ConfirmDialogAllowed())
    return NS_ERROR_NOT_AVAILABLE;

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
        printSettings->GetPrinterName(getter_Copies(printerName));
        if (printerName.IsEmpty()) {
          printSettingsService->GetDefaultPrinterName(getter_Copies(printerName));
          printSettings->SetPrinterName(printerName);
        }
        printSettingsService->InitPrintSettingsFromPrinter(printerName, printSettings);
        printSettingsService->InitPrintSettingsFromPrefs(printSettings, 
                                                         PR_TRUE, 
                                                         nsIPrintSettings::kInitSaveAll);
      } else {
        printSettingsService->GetNewPrintSettings(getter_AddRefs(printSettings));
      }

      nsCOMPtr<nsIDOMWindow> callerWin = EnterModalState();
      webBrowserPrint->Print(printSettings, nsnull);
      LeaveModalState(callerWin);

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
#endif 

  return NS_OK;
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
  nsCOMPtr<nsPIWindowRoot> root = GetTopWindowRoot();
  return CallQueryInterface(root, aWindowRoot);
}

already_AddRefed<nsPIWindowRoot>
nsGlobalWindow::GetTopWindowRoot()
{
  nsIDOMWindowInternal *rootWindow = GetPrivateRoot();
  nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(rootWindow));
  if (!piWin)
    return nsnull;

  nsCOMPtr<nsPIWindowRoot> window = do_QueryInterface(piWin->GetChromeEventHandler());
  return window.forget();
}

NS_IMETHODIMP
nsGlobalWindow::Scroll(PRInt32 aXScroll, PRInt32 aYScroll)
{
  return ScrollTo(aXScroll, aYScroll);
}

NS_IMETHODIMP
nsGlobalWindow::ScrollTo(PRInt32 aXScroll, PRInt32 aYScroll)
{
  FlushPendingNotifications(Flush_Layout);
  nsIScrollableFrame *sf = GetScrollFrame();

  if (sf) {
    
    
    
    
    
    const PRInt32 maxpx = nsPresContext::AppUnitsToIntCSSPixels(0x7fffffff) - 4;

    if (aXScroll > maxpx) {
      aXScroll = maxpx;
    }

    if (aYScroll > maxpx) {
      aYScroll = maxpx;
    }
    sf->ScrollTo(nsPoint(nsPresContext::CSSPixelsToAppUnits(aXScroll),
                         nsPresContext::CSSPixelsToAppUnits(aYScroll)),
                 nsIScrollableFrame::INSTANT);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::ScrollBy(PRInt32 aXScrollDif, PRInt32 aYScrollDif)
{
  FlushPendingNotifications(Flush_Layout);
  nsIScrollableFrame *sf = GetScrollFrame();

  if (sf) {
    nsPoint scrollPos = sf->GetScrollPosition();
    
    
    
    return ScrollTo(nsPresContext::AppUnitsToIntCSSPixels(scrollPos.x) + aXScrollDif,
                    nsPresContext::AppUnitsToIntCSSPixels(scrollPos.y) + aYScrollDif);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::ScrollByLines(PRInt32 numLines)
{
  FlushPendingNotifications(Flush_Layout);
  nsIScrollableFrame *sf = GetScrollFrame();
  if (sf) {
    
    
    
    sf->ScrollBy(nsIntPoint(0, numLines), nsIScrollableFrame::LINES,
                 nsIScrollableFrame::INSTANT);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::ScrollByPages(PRInt32 numPages)
{
  FlushPendingNotifications(Flush_Layout);
  nsIScrollableFrame *sf = GetScrollFrame();
  if (sf) {
    
    
    
    sf->ScrollBy(nsIntPoint(0, numPages), nsIScrollableFrame::PAGES,
                 nsIScrollableFrame::INSTANT);
  }

  return NS_OK;
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
                                  nsnull,
                                  EmptyString(), 0, 0,
                                  nsIScriptError::warningFlag,
                                  "DOM Events", doc);
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


void 
nsGlobalWindow::FirePopupBlockedEvent(nsIDOMDocument* aDoc,
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

PRBool
nsGlobalWindow::PopupWhitelisted()
{
  if (!IsPopupBlocked(mDocument))
    return PR_TRUE;

  nsCOMPtr<nsIDOMWindow> parent;

  if (NS_FAILED(GetParent(getter_AddRefs(parent))) ||
      parent == static_cast<nsIDOMWindow*>(this))
  {
    return PR_FALSE;
  }

  return static_cast<nsGlobalWindow*>
                    (static_cast<nsIDOMWindow*>
                                (parent.get()))->PopupWhitelisted();
}







PopupControlState
nsGlobalWindow::RevisePopupAbuseLevel(PopupControlState aControl)
{
  FORWARD_TO_OUTER(RevisePopupAbuseLevel, (aControl), aControl);

  NS_ASSERTION(mDocShell, "Must have docshell");
  
  nsCOMPtr<nsIDocShellTreeItem> item(do_QueryInterface(mDocShell));

  NS_ASSERTION(item, "Docshell doesn't implement nsIDocShellTreeItem?");

  PRInt32 type = nsIDocShellTreeItem::typeChrome;
  item->GetItemType(&type);
  if (type != nsIDocShellTreeItem::typeContent)
    return openAllowed;

  PopupControlState abuse = aControl;
  switch (abuse) {
  case openControlled:
  case openAbused:
  case openOverridden:
    if (PopupWhitelisted())
      abuse = PopupControlState(abuse - 1);
  case openAllowed: break;
  default:
    NS_WARNING("Strange PopupControlState!");
  }

  
  if (abuse == openAbused || abuse == openControlled) {
    PRInt32 popupMax = nsContentUtils::GetIntPref("dom.popup_maximum", -1);
    if (popupMax >= 0 && gOpenPopupSpamCount >= popupMax)
      abuse = openOverridden;
  }

  return abuse;
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
    baseURL = doc->GetDocBaseURI();

  
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

  JSObject *scope = nsnull;
  JSStackFrame *fp = nsnull;
  JS_FrameIterator(cx, &fp);
  if (fp) {
    while (fp->isDummyFrame()) {
      if (!JS_FrameIterator(cx, &fp))
        break;
    }

    if (fp)
      scope = &fp->scopeChain();
  }

  if (!scope)
    scope = JS_GetScopeChain(cx);

  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, scope))
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

  nsRefPtr<nsGlobalWindow> targetWindow;
  if (mTargetWindow->IsClosedOrClosing() ||
      !(targetWindow = mTargetWindow->GetCurrentInnerWindowInternal()) ||
      targetWindow->IsClosedOrClosing())
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


  
  
  
  

  nsIPresShell *shell = targetWindow->mDoc->GetShell();
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
  FORWARD_TO_OUTER(PostMessageMoz, (aMessage, aOrigin), NS_ERROR_NOT_INITIALIZED);

  
  
  
  
  
  
  

  
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

  nsRefPtr<nsGlobalWindow> mWindow;

  nsCloseEvent(nsGlobalWindow *aWindow)
    : mWindow(aWindow)
  {}

public:

  static nsresult
  PostCloseEvent(nsGlobalWindow* aWindow) {
    nsCOMPtr<nsIRunnable> ev = new nsCloseEvent(aWindow);
    nsresult rv = NS_DispatchToCurrentThread(ev);
    if (NS_SUCCEEDED(rv))
      aWindow->MaybeForgiveSpamCount();
    return rv;
  }

  NS_IMETHOD Run() {
    if (mWindow)
      mWindow->ReallyCloseWindow();
    return NS_OK;
  }

};

PRBool
nsGlobalWindow::CanClose()
{
  if (!mDocShell)
    return PR_TRUE;

  
  
  
  

  nsCOMPtr<nsIContentViewer> cv;
  mDocShell->GetContentViewer(getter_AddRefs(cv));
  if (cv) {
    PRBool canClose;
    nsresult rv = cv->PermitUnload(PR_FALSE, &canClose);
    if (NS_SUCCEEDED(rv) && !canClose)
      return PR_FALSE;

    rv = cv->RequestWindowClose(&canClose);
    if (NS_SUCCEEDED(rv) && !canClose)
      return PR_FALSE;
  }

  return PR_TRUE;
}

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
          "DOM Window", mDoc);  

      return NS_OK;
    }
  }

  if (!mInClose && !mIsClosed && !CanClose())
    return NS_OK;

  
  
  
  
  
  

  PRBool wasInClose = mInClose;
  mInClose = PR_TRUE;

  if (!DispatchCustomEvent("DOMWindowClose")) {
    
    

    mInClose = wasInClose;
    return NS_OK;
  }

  return FinalClose();
}

nsresult
nsGlobalWindow::ForceClose()
{
  if (IsFrame() || !mDocShell) {
    
    

    return NS_OK;
  }

  if (mHavePendingClose) {
    
    
    return NS_OK;
  }

  mInClose = PR_TRUE;

  DispatchCustomEvent("DOMWindowClose");

  return FinalClose();
}

nsresult
nsGlobalWindow::FinalClose()
{
  nsresult rv;
  
  mIsClosed = PR_TRUE;

  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService(sJSStackContractID);

  JSContext *cx = nsnull;

  if (stack) {
    stack->Peek(&cx);
  }

  if (cx) {
    nsIScriptContext *currentCX = nsJSUtils::GetDynamicScriptContext(cx);

    if (currentCX && currentCX == GetContextInternal()) {
      
      
      
      
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
    rv = nsCloseEvent::PostCloseEvent(this);
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

    CleanUp(PR_FALSE);
  }
}

nsIDOMWindow *
nsGlobalWindow::EnterModalState()
{
  nsGlobalWindow* topWin = GetTop();

  if (!topWin) {
    NS_ERROR("Uh, EnterModalState() called w/o a reachable top window?");

    return nsnull;
  }

  
  
  nsEventStateManager* activeESM =
    static_cast<nsEventStateManager*>(nsEventStateManager::GetActiveEventStateManager());
  if (activeESM && activeESM->GetPresContext()) {
    nsIPresShell* activeShell = activeESM->GetPresContext()->GetPresShell();
    if (activeShell && (
        nsContentUtils::ContentIsCrossDocDescendantOf(activeShell->GetDocument(), mDoc) ||
        nsContentUtils::ContentIsCrossDocDescendantOf(mDoc, activeShell->GetDocument()))) {
      nsEventStateManager::ClearGlobalActiveContent(activeESM);

      activeShell->SetCapturingContent(nsnull, 0);

      if (activeShell) {
        nsCOMPtr<nsFrameSelection> frameSelection = activeShell->FrameSelection();
        frameSelection->SetMouseDownState(PR_FALSE);
      }
    }
  }

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

  nsCOMPtr<nsIDOMWindow> callerWin;
  nsIScriptContext *scx;
  if (cx && (scx = GetScriptContextFromJSContext(cx))) {
    scx->EnterModalState();
    callerWin = do_QueryInterface(nsJSUtils::GetDynamicScriptGlobal(cx));
  }

  if (mContext) {
    mContext->EnterModalState();
  }

  return callerWin;
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
nsGlobalWindow::LeaveModalState(nsIDOMWindow *aCallerWin)
{
  nsGlobalWindow *topWin = GetTop();

  if (!topWin) {
    NS_ERROR("Uh, LeaveModalState() called w/o a reachable top window?");
    return;
  }

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

  if (aCallerWin) {
    nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(aCallerWin));
    nsIScriptContext *scx = sgo->GetContext();
    if (scx)
      scx->LeaveModalState();
  }

  if (mContext) {
    mContext->LeaveModalState();
  }

  
  nsGlobalWindow *inner = topWin->GetCurrentInnerWindowInternal();
  if (inner)
    inner->mLastDialogQuitTime = TimeStamp::Now();
}

PRBool
nsGlobalWindow::IsInModalState()
{
  nsGlobalWindow *topWin = GetTop();

  if (!topWin) {
    NS_ERROR("Uh, IsInModalState() called w/o a reachable top window?");

    return PR_FALSE;
  }

  return topWin->mModalStateDepth != 0;
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

class WindowDestroyedEvent : public nsRunnable
{
public:
  WindowDestroyedEvent(PRUint64 aID, const char* aTopic) :
    mID(aID), mTopic(aTopic) {}

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1");
    if (observerService) {
      nsCOMPtr<nsISupportsPRUint64> wrapper =
        do_CreateInstance(NS_SUPPORTS_PRUINT64_CONTRACTID);
      if (wrapper) {
        wrapper->SetData(mID);
        observerService->NotifyObservers(wrapper, mTopic.get(), nsnull);
      }
    }
    return NS_OK;
  }

private:
  PRUint64 mID;
  nsCString mTopic;
};

void
nsGlobalWindow::NotifyWindowIDDestroyed(const char* aTopic)
{
  nsRefPtr<nsIRunnable> runnable = new WindowDestroyedEvent(mWindowID, aTopic);
  nsresult rv = NS_DispatchToCurrentThread(runnable);
  if (NS_SUCCEEDED(rv)) {
    mNotifiedIDDestroyed = PR_TRUE;
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

    nsISupports* thisSupports;
    QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                   reinterpret_cast<void**>(&thisSupports));
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
  FORWARD_TO_OUTER(ShowModalDialog, (aURI, aArgs, aOptions, aRetVal),
                   NS_ERROR_NOT_INITIALIZED);

  *aRetVal = nsnull;

  
  
  EnsureReflowFlushAndPaint();

  if (AreDialogsBlocked() || !ConfirmDialogAllowed())
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsIDOMWindow> dlgWin;
  nsAutoString options(NS_LITERAL_STRING("-moz-internal-modal=1,status=1"));

  ConvertDialogOptions(aOptions, options);

  options.AppendLiteral(",scrollbars=1,centerscreen=1,resizable=0");

  nsCOMPtr<nsIDOMWindow> callerWin = EnterModalState();
  nsresult rv = OpenInternal(aURI, EmptyString(), options,
                             PR_FALSE,          
                             PR_TRUE,           
                             PR_TRUE,           
                             PR_TRUE,           
                             nsnull, aArgs,     
                             GetPrincipal(),    
                             nsnull,            
                             getter_AddRefs(dlgWin));
  LeaveModalState(callerWin);

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

    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(dlgWin));

    if (canAccess) {
      nsPIDOMWindow *inner = win->GetCurrentInnerWindow();

      nsCOMPtr<nsIDOMModalContentWindow> dlgInner(do_QueryInterface(inner));

      if (dlgInner) {
        dlgInner->GetReturnValue(aRetVal);
      }
    }

    nsRefPtr<nsGlobalWindow> winInternal =
      static_cast<nsGlobalWindow*>(win.get());
    if (winInternal->mCallCleanUpAfterModalDialogCloses) {
      winInternal->mCallCleanUpAfterModalDialogCloses = PR_FALSE;
      winInternal->CleanUp(PR_TRUE);
    }
  }
  
  return NS_OK;
}

class CommandDispatcher : public nsRunnable
{
public:
  CommandDispatcher(nsIDOMXULCommandDispatcher* aDispatcher,
                    const nsAString& aAction)
  : mDispatcher(aDispatcher), mAction(aAction) {}

  NS_IMETHOD Run()
  {
    return mDispatcher->UpdateCommands(mAction);
  }

  nsCOMPtr<nsIDOMXULCommandDispatcher> mDispatcher;
  nsString                             mAction;
};

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
    if (xulCommandDispatcher) {
      nsContentUtils::AddScriptRunner(new CommandDispatcher(xulCommandDispatcher,
                                                            anAction));
    }
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
  if (!Is8bit(aAsciiBase64String)) {
    aBinaryData.Truncate();
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  nsresult rv = nsXPConnect::Base64Decode(aAsciiBase64String, aBinaryData);
  if (NS_FAILED(rv) && rv == NS_ERROR_INVALID_ARG) {
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }
  return rv;
}

NS_IMETHODIMP
nsGlobalWindow::Btoa(const nsAString& aBinaryData,
                     nsAString& aAsciiBase64String)
{
  if (!Is8bit(aBinaryData)) {
    aAsciiBase64String.Truncate();
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  return nsXPConnect::Base64Encode(aBinaryData, aAsciiBase64String);
}





NS_IMETHODIMP
nsGlobalWindow::AddEventListener(const nsAString& aType,
                                 nsIDOMEventListener* aListener,
                                 PRBool aUseCapture)
{
  FORWARD_TO_INNER_CREATE(AddEventListener, (aType, aListener, aUseCapture),
                          NS_ERROR_NOT_AVAILABLE);

  return AddEventListener(aType, aListener, aUseCapture, PR_FALSE, 0);
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

  
  nsIPresShell *shell = mDoc->GetShell();
  nsRefPtr<nsPresContext> presContext;
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
  }
  return NS_OK;
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
                                 PRBool aUseCapture, PRBool aWantsUntrusted,
                                 PRUint8 optional_argc)
{
  NS_ASSERTION(!aWantsUntrusted || optional_argc > 0,
               "Won't check if this is chrome, you want to set "
               "aWantsUntrusted to PR_FALSE or make the aWantsUntrusted "
               "explicit by making optional_argc non-zero.");

  if (IsOuterWindow() && mInnerWindow &&
      !nsContentUtils::CanCallerAccess(mInnerWindow)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsIEventListenerManager* manager = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(manager);

  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

  if (aWantsUntrusted ||
      (optional_argc == 0 && !nsContentUtils::IsChromeDoc(mDoc))) {
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
  FORWARD_TO_INNER(GetLocation, (aLocation), NS_ERROR_NOT_INITIALIZED);

  *aLocation = nsnull;

  nsIDocShell *docShell = GetDocShell();
  if (!mLocation && docShell) {
    mLocation = new nsLocation(docShell);
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
  if (!mainWidget)
    return;

  
  
  nsCOMPtr<nsIWidget> topLevelWidget = mainWidget->GetSheetWindowParent();
  if (!topLevelWidget) {
    topLevelWidget = mainWidget;
  }

  
  nsCOMPtr<nsIDOMWindowInternal> topLevelWindow;
  if (topLevelWidget == mainWidget) {
    topLevelWindow = static_cast<nsIDOMWindowInternal*>(this);
  } else {
    
    
    
    
    void* clientData;
    topLevelWidget->GetClientData(clientData); 
    nsISupports* data = static_cast<nsISupports*>(clientData);
    nsCOMPtr<nsIInterfaceRequestor> req(do_QueryInterface(data));
    topLevelWindow = do_GetInterface(req);
  }
  if (topLevelWindow) {
    nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(topLevelWindow));
    piWin->SetActive(aActivate);
  }
}

static PRBool
NotifyDocumentTree(nsIDocument* aDocument, void* aData)
{
  aDocument->EnumerateSubDocuments(NotifyDocumentTree, nsnull);
  aDocument->DocumentStatesChanged(NS_DOCUMENT_STATE_WINDOW_INACTIVE);
  return PR_TRUE;
}

void
nsGlobalWindow::SetActive(PRBool aActive)
{
  nsPIDOMWindow::SetActive(aActive);
  NotifyDocumentTree(mDoc, nsnull);
}

void nsGlobalWindow::MaybeUpdateTouchState()
{
  FORWARD_TO_INNER_VOID(MaybeUpdateTouchState, ());

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();

  nsCOMPtr<nsIDOMWindow> focusedWindow;
  fm->GetFocusedWindow(getter_AddRefs(focusedWindow));

  if(this == focusedWindow) {
    UpdateTouchState();
  }
}

void nsGlobalWindow::UpdateTouchState()
{
  FORWARD_TO_INNER_VOID(UpdateTouchState, ());

  nsCOMPtr<nsIWidget> mainWidget = GetMainWidget();
  if (!mainWidget)
    return;

  if (mMayHaveTouchEventListener) {
    mainWidget->RegisterTouchWindow();
  } else {
    mainWidget->UnregisterTouchWindow();
  }
}


void
nsGlobalWindow::EnableAccelerationUpdates()
{
  if (mHasAcceleration) {
    nsCOMPtr<nsIAccelerometer> ac =
      do_GetService(NS_ACCELEROMETER_CONTRACTID);
    if (ac) {
      ac->AddWindowListener(this);
    }
  }
}

void
nsGlobalWindow::DisableAccelerationUpdates()
{
  if (mHasAcceleration) {
    nsCOMPtr<nsIAccelerometer> ac =
      do_GetService(NS_ACCELEROMETER_CONTRACTID);
    if (ac) {
      ac->RemoveWindowListener(this);
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
      NS_ASSERTION(!inner->mOuterWindow || inner->mOuterWindow == this,
                   "bad outer window pointer");
      inner->SetChromeEventHandlerInternal(aChromeEventHandler);
    }
  } else if (mOuterWindow) {
    
    
    
    static_cast<nsGlobalWindow*>(mOuterWindow.get())->
      SetChromeEventHandlerInternal(aChromeEventHandler);
  }
}

static PRBool IsLink(nsIContent* aContent)
{
  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor = do_QueryInterface(aContent);
  return (anchor || (aContent &&
                     aContent->AttrValueIs(kNameSpaceID_XLink, nsGkAtoms::type,
                                           nsGkAtoms::simple, eCaseMatters)));
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
    mFocusMethod = aFocusMethod & FOCUSMETHOD_MASK;
    mShowFocusRingForContent = PR_FALSE;
  }

  if (mFocusedNode) {
    
    
    if (mFocusMethod & nsIFocusManager::FLAG_BYKEY) {
      mFocusByKeyOccurred = PR_TRUE;
    } else if (
      
      
      
      
#ifndef XP_WIN
      !(mFocusMethod & nsIFocusManager::FLAG_BYMOUSE) || !IsLink(aNode) ||
#endif
      aFocusMethod & nsIFocusManager::FLAG_SHOWRING) {
        mShowFocusRingForContent = PR_TRUE;
    }
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
nsGlobalWindow::ShouldShowFocusRing()
{
  FORWARD_TO_INNER(ShouldShowFocusRing, (), PR_FALSE);

  return mShowFocusRings || mShowFocusRingForContent || mFocusByKeyOccurred;
}

void
nsGlobalWindow::SetKeyboardIndicators(UIStateChangeType aShowAccelerators,
                                      UIStateChangeType aShowFocusRings)
{
  FORWARD_TO_INNER_VOID(SetKeyboardIndicators, (aShowAccelerators, aShowFocusRings));

  
  if (aShowAccelerators != UIStateChangeType_NoChange)
    mShowAccelerators = aShowAccelerators == UIStateChangeType_Set;
  if (aShowFocusRings != UIStateChangeType_NoChange)
    mShowFocusRings = aShowFocusRings == UIStateChangeType_Set;

  
  nsCOMPtr<nsIDocShellTreeNode> node = do_QueryInterface(GetDocShell());
  if (node) {
    PRInt32 childCount = 0;
    node->GetChildCount(&childCount);

    for (PRInt32 i = 0; i < childCount; ++i) {
      nsCOMPtr<nsIDocShellTreeItem> childShell;
      node->GetChildAt(i, getter_AddRefs(childShell));
      nsCOMPtr<nsPIDOMWindow> childWindow = do_GetInterface(childShell);
      if (childWindow) {
        childWindow->SetKeyboardIndicators(aShowAccelerators, aShowFocusRings);
      }
    }
  }

  if (mHasFocus && mFocusedNode) { 
    nsIDocument *doc = mFocusedNode->GetCurrentDoc();
    if (doc) {
      MOZ_AUTO_DOC_UPDATE(doc, UPDATE_CONTENT_STATE, PR_TRUE);
      doc->ContentStateChanged(mFocusedNode, NS_EVENT_STATE_FOCUSRING);
    }
  }
}

void
nsGlobalWindow::GetKeyboardIndicators(PRBool* aShowAccelerators,
                                      PRBool* aShowFocusRings)
{
  FORWARD_TO_INNER_VOID(GetKeyboardIndicators, (aShowAccelerators, aShowFocusRings));

  *aShowAccelerators = mShowAccelerators;
  *aShowFocusRings = mShowFocusRings;
}

PRBool
nsGlobalWindow::TakeFocus(PRBool aFocus, PRUint32 aFocusMethod)
{
  FORWARD_TO_INNER(TakeFocus, (aFocus, aFocusMethod), PR_FALSE);

  if (aFocus)
    mFocusMethod = aFocusMethod & FOCUSMETHOD_MASK;

  if (mHasFocus != aFocus) {
    mHasFocus = aFocus;
    UpdateCanvasFocus(PR_TRUE, mFocusedNode);
  }

  
  
  
  
  
  if (aFocus && mNeedsFocus && mDoc && mDoc->GetRootElement() != nsnull) {
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

  PRBool oldNeedsFocus = mNeedsFocus;
  mNeedsFocus = PR_FALSE;

  
  
  nsPIDOMWindow* root = GetPrivateRoot();
  if (root) {
    PRBool showAccelerators, showFocusRings;
    root->GetKeyboardIndicators(&showAccelerators, &showFocusRings);
    mShowFocusRings = showFocusRings;
  }

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm)
    fm->WindowShown(this, oldNeedsFocus);
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

class HashchangeCallback : public nsRunnable
{
public:
  HashchangeCallback(const nsAString &aOldURL,
                     const nsAString &aNewURL,
                     nsGlobalWindow* aWindow)
    : mWindow(aWindow)
  {
    mOldURL.Assign(aOldURL);
    mNewURL.Assign(aNewURL);
  }

  NS_IMETHOD Run()
  {
    NS_PRECONDITION(NS_IsMainThread(), "Should be called on the main thread.");
    return mWindow->FireHashchange(mOldURL, mNewURL);
  }

private:
  nsString mOldURL;
  nsString mNewURL;
  nsRefPtr<nsGlobalWindow> mWindow;
};

nsresult
nsGlobalWindow::DispatchAsyncHashchange(nsIURI *aOldURI, nsIURI *aNewURI)
{
  FORWARD_TO_INNER(DispatchAsyncHashchange, (aOldURI, aNewURI), NS_OK);

  
  
  nsCAutoString oldBeforeHash, oldHash, newBeforeHash, newHash;
  nsContentUtils::SplitURIAtHash(aOldURI, oldBeforeHash, oldHash);
  nsContentUtils::SplitURIAtHash(aNewURI, newBeforeHash, newHash);

  NS_ENSURE_STATE(oldBeforeHash.Equals(newBeforeHash));
  NS_ENSURE_STATE(!oldHash.Equals(newHash));

  nsCAutoString oldSpec, newSpec;
  aOldURI->GetSpec(oldSpec);
  aNewURI->GetSpec(newSpec);

  NS_ConvertUTF8toUTF16 oldWideSpec(oldSpec);
  NS_ConvertUTF8toUTF16 newWideSpec(newSpec);

  nsCOMPtr<nsIRunnable> callback =
    new HashchangeCallback(oldWideSpec, newWideSpec, this);
  return NS_DispatchToMainThread(callback);
}

nsresult
nsGlobalWindow::FireHashchange(const nsAString &aOldURL,
                               const nsAString &aNewURL)
{
  NS_ENSURE_TRUE(IsInnerWindow(), NS_ERROR_FAILURE);

  
  if (IsFrozen())
    return NS_OK;

  
  NS_ENSURE_STATE(mDoc);

  nsIPresShell *shell = mDoc->GetShell();
  nsRefPtr<nsPresContext> presContext;
  if (shell) {
    presContext = shell->GetPresContext();
  }

  
  nsCOMPtr<nsIDOMEvent> domEvent;
  nsresult rv =
    nsEventDispatcher::CreateEvent(presContext, nsnull,
                                   NS_LITERAL_STRING("hashchangeevent"),
                                   getter_AddRefs(domEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(domEvent);
  NS_ENSURE_TRUE(privateEvent, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMHashChangeEvent> hashchangeEvent = do_QueryInterface(domEvent);
  NS_ENSURE_TRUE(hashchangeEvent, NS_ERROR_UNEXPECTED);

  
  rv = hashchangeEvent->InitHashChangeEvent(NS_LITERAL_STRING("hashchange"),
                                            PR_TRUE, PR_FALSE,
                                            aOldURL, aNewURL);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = privateEvent->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dummy;
  return DispatchEvent(hashchangeEvent, &dummy);
}

nsresult
nsGlobalWindow::DispatchSyncPopState()
{
  FORWARD_TO_INNER(DispatchSyncPopState, (), NS_OK);

  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(),
               "Must be safe to run script here.");

  
  if (!nsContentUtils::GetBoolPref(sPopStatePrefStr, PR_FALSE))
    return NS_OK;

  nsresult rv = NS_OK;

  
  if (IsFrozen()) {
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIVariant> stateObj;
  rv = mDoc->GetMozCurrentStateObject(getter_AddRefs(stateObj));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsIPresShell *shell = mDoc->GetShell();
  nsRefPtr<nsPresContext> presContext;
  if (shell) {
    presContext = shell->GetPresContext();
  }

  
  nsCOMPtr<nsIDOMEvent> domEvent;
  rv = nsEventDispatcher::CreateEvent(presContext, nsnull,
                                      NS_LITERAL_STRING("popstateevent"),
                                      getter_AddRefs(domEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(domEvent);
  NS_ENSURE_TRUE(privateEvent, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDOMPopStateEvent> popstateEvent = do_QueryInterface(domEvent);
  rv = popstateEvent->InitPopStateEvent(NS_LITERAL_STRING("popstate"),
                                        PR_TRUE, PR_FALSE,
                                        stateObj);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = privateEvent->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMEventTarget> outerWindow =
    do_QueryInterface(GetOuterWindow());
  NS_ENSURE_TRUE(outerWindow, NS_ERROR_UNEXPECTED);

  rv = privateEvent->SetTarget(outerWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dummy; 
  return DispatchEvent(popstateEvent, &dummy);
}



static nsCanvasFrame* FindCanvasFrame(nsIFrame* aFrame)
{
    nsCanvasFrame* canvasFrame = do_QueryFrame(aFrame);
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
  Element *rootElement = doc->GetRootElement();
  if (rootElement) {
      if ((mHasFocus || aFocusChanged) &&
          (mFocusedNode == rootElement || aNewContent == rootElement)) {
          nsIFrame* frame = rootElement->GetPrimaryFrame();
          if (frame) {
              frame = frame->GetParent();
              nsCanvasFrame* canvasFrame = do_QueryFrame(frame);
              if (canvasFrame) {
                  canvasFrame->SetHasFocus(mHasFocus && rootElement == aNewContent);
              }
          }
      }
  } else {
      
      nsIFrame* frame = presShell->GetRootFrame();
      if (frame) {
          nsCanvasFrame* canvasFrame = FindCanvasFrame(frame);
          if (canvasFrame) {
              canvasFrame->SetHasFocus(PR_FALSE);
          }
      }      
  }
}





NS_IMETHODIMP
nsGlobalWindow::GetComputedStyle(nsIDOMElement* aElt,
                                 const nsAString& aPseudoElt,
                                 nsIDOMCSSStyleDeclaration** aReturn)
{
  FORWARD_TO_OUTER(GetComputedStyle, (aElt, aPseudoElt, aReturn),
                   NS_ERROR_NOT_INITIALIZED);

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

  nsRefPtr<nsComputedDOMStyle> compStyle;
  nsresult rv = NS_NewComputedDOMStyle(aElt, aPseudoElt, presShell,
                                       getter_AddRefs(compStyle));
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

  nsIPrincipal *principal = GetPrincipal();
  nsIDocShell* docShell = GetDocShell();

  if (!principal || !docShell) {
    *aSessionStorage = nsnull;
    return NS_OK;
  }

  if (!nsContentUtils::GetBoolPref(kStorageEnabled)) {
    *aSessionStorage = nsnull;
    return NS_OK;
  }

  if (mSessionStorage) {
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gDOMLeakPRLog, PR_LOG_DEBUG)) {
      PR_LogPrint("nsGlobalWindow %p has %p sessionStorage", this, mSessionStorage.get());
    }
#endif
    nsCOMPtr<nsPIDOMStorage> piStorage = do_QueryInterface(mSessionStorage);
    if (piStorage) {
      PRBool canAccess = piStorage->CanAccess(principal);
      NS_ASSERTION(canAccess,
                   "window %x owned sessionStorage "
                   "that could not be accessed!");
      if (!canAccess) {
          mSessionStorage = nsnull;
      }
    }
  }

  if (!mSessionStorage) {
    *aSessionStorage = nsnull;

    nsString documentURI;
    if (mDocument) {
      mDocument->GetDocumentURI(documentURI);
    }

    nsresult rv = docShell->GetSessionStorageForPrincipal(principal,
                                                          documentURI,
                                                          PR_TRUE,
                                                          getter_AddRefs(mSessionStorage));
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gDOMLeakPRLog, PR_LOG_DEBUG)) {
      PR_LogPrint("nsGlobalWindow %p tried to get a new sessionStorage %p", this, mSessionStorage.get());
    }
#endif

    if (!mSessionStorage) {
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
  }

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gDOMLeakPRLog, PR_LOG_DEBUG)) {
      PR_LogPrint("nsGlobalWindow %p returns %p sessionStorage", this, mSessionStorage.get());
    }
#endif

  NS_ADDREF(*aSessionStorage = mSessionStorage);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetGlobalStorage(nsIDOMStorageList ** aGlobalStorage)
{
  NS_ENSURE_ARG_POINTER(aGlobalStorage);

#ifdef MOZ_STORAGE
  if (!nsContentUtils::GetBoolPref(kStorageEnabled)) {
    *aGlobalStorage = nsnull;
    return NS_OK;
  }

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

  if (!nsContentUtils::GetBoolPref(kStorageEnabled)) {
    *aLocalStorage = nsnull;
    return NS_OK;
  }

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

    nsString documentURI;
    if (mDocument) {
      mDocument->GetDocumentURI(documentURI);
    }

    rv = storageManager->GetLocalStorageForPrincipal(principal,
                                                     documentURI,
                                                     getter_AddRefs(mLocalStorage));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aLocalStorage = mLocalStorage);
  return NS_OK;
}





NS_IMETHODIMP
nsGlobalWindow::GetMozIndexedDB(nsIIDBFactory** _retval)
{
  if (!mIndexedDB) {
    nsCOMPtr<mozIThirdPartyUtil> thirdPartyUtil =
      do_GetService(THIRDPARTYUTIL_CONTRACTID);
    NS_ENSURE_TRUE(thirdPartyUtil, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

    PRBool isThirdParty;
    nsresult rv = thirdPartyUtil->IsThirdPartyWindow(this, nsnull,
                                                     &isThirdParty);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

    if (isThirdParty) {
      NS_WARNING("IndexedDB is not permitted in a third-party window.");
      *_retval = nsnull;
      return NS_OK;
    }

    mIndexedDB = indexedDB::IDBFactory::Create(this);
    NS_ENSURE_TRUE(mIndexedDB, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);
  }

  nsCOMPtr<nsIIDBFactory> request(mIndexedDB);
  request.forget(_retval);
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
    if (principal) {
      
      

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
      
      
      

      if (!mPendingStorageEventsObsolete) {
        mPendingStorageEventsObsolete = new nsDataHashtable<nsStringHashKey, PRBool>;
        NS_ENSURE_TRUE(mPendingStorageEventsObsolete, NS_ERROR_OUT_OF_MEMORY);

        rv = mPendingStorageEventsObsolete->Init();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      mPendingStorageEventsObsolete->Put(domain, PR_TRUE);

      return NS_OK;
    }

    nsRefPtr<nsDOMStorageEventObsolete> event = new nsDOMStorageEventObsolete();
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

    rv = event->InitStorageEvent(NS_LITERAL_STRING("storage"), PR_FALSE, PR_FALSE, domain);
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
    target->DispatchEvent((nsIDOMStorageEventObsolete *)event, &defaultActionEnabled);

    return NS_OK;
  }

  if (IsInnerWindow() && !nsCRT::strcmp(aTopic, "dom-storage2-changed")) {
    nsIPrincipal *principal;
    nsresult rv;

    nsCOMPtr<nsIDOMStorageEvent> event = do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMStorage> changingStorage;
    rv = event->GetStorageArea(getter_AddRefs(changingStorage));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsPIDOMStorage> pistorage = do_QueryInterface(changingStorage);
    nsPIDOMStorage::nsDOMStorageType storageType = pistorage->StorageType();

    principal = GetPrincipal();
    switch (storageType)
    {
    case nsPIDOMStorage::SessionStorage:
    {
      if (SameCOMIdentity(mSessionStorage, changingStorage)) {
        
        
        return NS_OK;
      }

      nsCOMPtr<nsIDOMStorage> storage = mSessionStorage;
      if (!storage) {
        nsIDocShell* docShell = GetDocShell();
        if (principal && docShell) {
          
          
          docShell->GetSessionStorageForPrincipal(principal,
                                                  EmptyString(),
                                                  PR_FALSE,
                                                  getter_AddRefs(storage));
        }
      }

      if (!pistorage->IsForkOf(storage)) {
        
        
        return NS_OK;
      }

#ifdef PR_LOGGING
      if (PR_LOG_TEST(gDOMLeakPRLog, PR_LOG_DEBUG)) {
        PR_LogPrint("nsGlobalWindow %p with sessionStorage %p passing event from %p", this, mSessionStorage.get(), pistorage.get());
      }
#endif

      break;
    }
    case nsPIDOMStorage::LocalStorage:
    {
      if (SameCOMIdentity(mLocalStorage, changingStorage)) {
        
        
        return NS_OK;
      }

      
      
      nsIPrincipal *storagePrincipal = pistorage->Principal();
      PRBool equals;

      rv = storagePrincipal->Equals(principal, &equals);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!equals)
        return NS_OK;

      break;
    }
    default:
      return NS_OK;
    }

    if (IsFrozen()) {
      
      
      

      mPendingStorageEvents.AppendObject(event);
      return NS_OK;
    }

    PRBool defaultActionEnabled;
    DispatchEvent((nsIDOMStorageEvent *)event, &defaultActionEnabled);

    return NS_OK;
  }

  if (!nsCRT::strcmp(aTopic, "offline-cache-update-added")) {
    if (mApplicationCache)
      return NS_OK;

    
    
    
    nsCOMPtr<nsIDOMOfflineResourceList> applicationCache;
    GetApplicationCache(getter_AddRefs(applicationCache));
    nsCOMPtr<nsIObserver> observer = do_QueryInterface(applicationCache);
    if (observer)
      observer->Observe(aSubject, aTopic, aData);

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

  for (PRInt32 i = 0; i < mPendingStorageEvents.Count(); ++i) {
    Observe(mPendingStorageEvents[i], "dom-storage2-changed", nsnull);
  }

  if (mPendingStorageEventsObsolete) {
    
    mPendingStorageEventsObsolete->EnumerateRead(FirePendingStorageEvents, this);

    delete mPendingStorageEventsObsolete;
    mPendingStorageEventsObsolete = nsnull;
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

  PopupControlState abuseLevel = gPopupControlState;
  if (checkForPopup) {
    abuseLevel = RevisePopupAbuseLevel(abuseLevel);
    if (abuseLevel >= openAbused) {
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

  
  
  nsCloseEvent::PostCloseEvent(globalWin);
  
}


void
nsGlobalWindow::ClearWindowScope(nsISupports *aWindow)
{
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(aWindow));
  nsIScriptContext *scx = sgo->GetContext();
  if (scx) {
    scx->ClearScope(sgo->GetGlobalJSObject(), PR_TRUE);
  }
}





PRUint32 sNestingLevel;

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

  
  
  interval = NS_MAX(aIsInterval ? 1 : 0, interval);

  
  
  
  PRUint32 maxTimeoutMs = PR_IntervalToMilliseconds(DOM_MAX_TIMEOUT_VALUE);
  if (static_cast<PRUint32>(interval) > maxTimeoutMs) {
    interval = maxTimeoutMs;
  }

  nsTimeout *timeout = new nsTimeout();

  
  
  timeout->AddRef();

  if (aIsInterval) {
    timeout->mInterval = interval;
  }
  timeout->mScriptHandler = aHandler;

  
  PRUint32 nestingLevel = sNestingLevel + 1;
  PRInt32 realInterval = interval;
  if (aIsInterval || nestingLevel >= DOM_CLAMP_TIMEOUT_NESTING_LEVEL) {
    
    
    realInterval = NS_MAX(realInterval, DOMMinTimeoutValue());
  }

  
  
  
  

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

  TimeDuration delta = TimeDuration::FromMilliseconds(realInterval);

  if (!IsFrozen() && !mTimeoutsSuspendDepth) {
    
    
    

    timeout->mWhen = TimeStamp::Now() + delta;

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
    
    
    
    
    

    timeout->mTimeRemaining = delta;
  }

  timeout->mWindow = this;

  if (!aIsInterval) {
    timeout->mNestingLevel = nestingLevel;
  }

  
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

  NS_TIME_FUNCTION;

  NS_ASSERTION(IsInnerWindow(), "Timeout running on outer window!");
  NS_ASSERTION(!IsFrozen(), "Timeout running on a window in the bfcache!");

  nsTimeout *nextTimeout, *timeout;
  nsTimeout *last_expired_timeout, *last_insertion_point;
  nsTimeout dummy_timeout;
  PRUint32 firingDepth = mTimeoutFiringDepth + 1;

  
  
  nsCOMPtr<nsIScriptGlobalObject> windowKungFuDeathGrip(this);

  
  
  TimeStamp now = TimeStamp::Now();
  TimeStamp deadline;

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
  dummy_timeout.mWhen = now;
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

    PRBool trackNestingLevel = !timeout->mInterval;
    PRUint32 nestingLevel;
    if (trackNestingLevel) {
      nestingLevel = sNestingLevel;
      sNestingLevel = timeout->mNestingLevel;
    }

    nsCOMPtr<nsIScriptTimeoutHandler> handler(timeout->mScriptHandler);
    void *scriptObject = handler->GetScriptObject();
    if (!scriptObject) {
      
      const PRUnichar *script = handler->GetHandlerText();
      NS_ASSERTION(script, "timeout has no script nor handler text!");

      const char *filename = nsnull;
      PRUint32 lineNo = 0;
      handler->GetLocation(&filename, &lineNo);

      NS_TIME_FUNCTION_MARK("(file: %s, line: %d)", filename, lineNo);

      PRBool is_undefined;
      scx->EvaluateString(nsDependentString(script), 
                          GetScriptGlobal(handler->GetScriptTypeID()),
                          timeout->mPrincipal, filename, lineNo,
                          handler->GetScriptVersion(), nsnull,
                          &is_undefined);
    } else {
      
      
      TimeDuration lateness = now - timeout->mWhen;

      handler->SetLateness(lateness.ToMilliseconds());

      nsCOMPtr<nsIVariant> dummy;
      nsCOMPtr<nsISupports> me(static_cast<nsIDOMWindow *>(this));
      scx->CallEventHandler(me,
                            GetScriptGlobal(handler->GetScriptTypeID()),
                            scriptObject, handler->GetArgv(),
                            
                            
                            getter_AddRefs(dummy));

    }
    handler = nsnull; 

    if (trackNestingLevel) {
      sNestingLevel = nestingLevel;
    }

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
      
      
      TimeDuration nextInterval =
        TimeDuration::FromMilliseconds(NS_MAX(timeout->mInterval,
                                              PRUint32(DOMMinTimeoutValue())));

      
      
      
      
      
      TimeStamp firingTime;
      if (!aTimeout || timeout->mWhen + nextInterval <= now)
        firingTime = now + nextInterval;
      else
        firingTime = timeout->mWhen + nextInterval;

      TimeDuration delay = firingTime - TimeStamp::Now();

      
      
      if (delay < TimeDuration(0)) {
        delay = TimeDuration(0);
      }

      if (timeout->mTimer) {
        timeout->mWhen = firingTime;

        
        
        
        

        
        
        
        
        
        nsresult rv = timeout->mTimer->
          InitWithFuncCallback(TimerCallback, timeout,
                               delay.ToMilliseconds(),
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

        timeout->mTimeRemaining = delay;
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
         
         
         ((IsFrozen() || mTimeoutsSuspendDepth) ?
          prevSibling->mTimeRemaining > aTimeout->mTimeRemaining :
          prevSibling->mWhen > aTimeout->mWhen);
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

nsIScrollableFrame *
nsGlobalWindow::GetScrollFrame()
{
  FORWARD_TO_OUTER(GetScrollFrame, (), nsnull);

  if (!mDocShell) {
    return nsnull;
  }

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));
  if (presShell) {
    return presShell->GetRootScrollFrameAsScrollable();
  }
  return nsnull;
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
      baseURI = doc->GetDocBaseURI();
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

  
  JSContext *cx = (JSContext *)mContext->GetNativeContext();
  JSAutoRequest req(cx);

  nsIXPConnect *xpc = nsContentUtils::XPConnect();

  nsCOMPtr<nsIClassInfo> ci =
    do_QueryInterface((nsIScriptGlobalObject *)this);
  nsCOMPtr<nsIXPConnectJSObjectHolder> proto;
  nsresult rv = xpc->GetWrappedNativePrototype(cx, mJSObject, ci,
                                               getter_AddRefs(proto));
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject *realProto = JS_GetPrototype(cx, mJSObject);
  nsCOMPtr<nsIXPConnectJSObjectHolder> realProtoHolder;
  if (realProto) {
    rv = xpc->HoldObject(cx, realProto, getter_AddRefs(realProtoHolder));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsISupports> state = new WindowStateHolder(inner,
                                                      mInnerWindowHolder,
                                                      mNavigator,
                                                      proto,
                                                      realProtoHolder);
  NS_ENSURE_TRUE(state, NS_ERROR_OUT_OF_MEMORY);

  JSObject *wnProto;
  proto->GetJSObject(&wnProto);
  if (!JS_SetPrototype(cx, mJSObject, wnProto)) {
    return NS_ERROR_FAILURE;
  }

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

  
  
  nsIContent* focusedNode = inner->GetFocusedNode();
  if (IsLink(focusedNode)) {
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (fm) {
      nsCOMPtr<nsIDOMElement> focusedElement(do_QueryInterface(focusedNode));
      fm->SetFocus(focusedElement, nsIFocusManager::FLAG_NOSCROLL |
                                   nsIFocusManager::FLAG_SHOWRING);
    }
  }

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
    DisableAccelerationUpdates();

    nsDOMThreadService* dts = nsDOMThreadService::get();
    if (dts) {
      dts->SuspendWorkersForGlobal(static_cast<nsIScriptGlobalObject*>(this));
    }
  
    TimeStamp now = TimeStamp::Now();
    for (nsTimeout *t = FirstTimeout(); IsTimeout(t); t = t->Next()) {
      
      if (t->mWhen > now)
        t->mTimeRemaining = t->mWhen - now;
      else
        t->mTimeRemaining = TimeDuration(0);
  
      
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
        NS_ASSERTION(win->IsOuterWindow(), "Expected outer window");
        nsGlobalWindow* inner = win->GetCurrentInnerWindowInternal();

        
        
        nsCOMPtr<nsIContent> frame = do_QueryInterface(pWin->GetFrameElementInternal());
        if (!mDoc || !frame || mDoc != frame->GetOwnerDoc() || !inner) {
          continue;
        }

        win->SuspendTimeouts(aIncrease, aFreezeChildren);

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
    EnableAccelerationUpdates();

    nsDOMThreadService* dts = nsDOMThreadService::get();
    if (dts) {
      dts->ResumeWorkersForGlobal(static_cast<nsIScriptGlobalObject*>(this));
    }

    
    

    TimeStamp now = TimeStamp::Now();

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
        NS_MAX(PRInt32(t->mTimeRemaining.ToMilliseconds()),
               DOMMinTimeoutValue());

      
      
      t->mWhen = now + t->mTimeRemaining;

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

        
        
        nsCOMPtr<nsIContent> frame = do_QueryInterface(pWin->GetFrameElementInternal());
        if (!mDoc || !frame || mDoc != frame->GetOwnerDoc() || !inner) {
          continue;
        }

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
  mHasAcceleration = PR_TRUE;
  EnableAccelerationUpdates();
}

NS_IMETHODIMP
nsGlobalWindow::GetURL(nsIDOMMozURLProperty** aURL)
{
  FORWARD_TO_INNER(GetURL, (aURL), NS_ERROR_UNEXPECTED);

  if (!mURLProperty) {
    mURLProperty = new nsDOMMozURLProperty(this);
  }

  NS_ADDREF(*aURL = mURLProperty);

  return NS_OK;
}



NS_IMPL_CYCLE_COLLECTION_CLASS(nsGlobalChromeWindow)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsGlobalChromeWindow,
                                                  nsGlobalWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mBrowserDOMWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mMessageManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(ChromeWindow, nsGlobalChromeWindow)


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
nsGlobalChromeWindow::BeginWindowMove(nsIDOMEvent *aMouseDownEvent)
{
  nsCOMPtr<nsIWidget> widget = GetMainWidget();
  if (!widget) {
    return NS_OK;
  }

  nsCOMPtr<nsIPrivateDOMEvent> privEvent = do_QueryInterface(aMouseDownEvent);
  NS_ENSURE_TRUE(privEvent, NS_ERROR_FAILURE);
  nsEvent *internalEvent = privEvent->GetInternalNSEvent();
  NS_ENSURE_TRUE(internalEvent &&
                 internalEvent->eventStructType == NS_MOUSE_EVENT,
                 NS_ERROR_FAILURE);
  nsMouseEvent *mouseEvent = static_cast<nsMouseEvent*>(internalEvent);

  return widget->BeginMoveDrag(mouseEvent);
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

  nsRefPtr<nsPresContext> presContext;
  if (mDocShell) {
    mDocShell->GetPresContext(getter_AddRefs(presContext));
  }

  if (presContext) {
    
    nsCOMPtr<nsIPresShell> presShell;
    mDocShell->GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    nsIViewManager* vm = presShell->GetViewManager();
    NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);

    nsIView* rootView = vm->GetRootView();
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
  nsIFrame *frame = content->GetPrimaryFrame();
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

NS_IMETHODIMP
nsGlobalChromeWindow::GetMessageManager(nsIChromeFrameMessageManager** aManager)
{
  FORWARD_TO_INNER_CHROME(GetMessageManager, (aManager), NS_ERROR_FAILURE);
  if (!mMessageManager) {
    nsIScriptContext* scx = GetContextInternal();
    NS_ENSURE_STATE(scx);
    JSContext* cx = (JSContext *)scx->GetNativeContext();
    NS_ENSURE_STATE(cx);
    nsCOMPtr<nsIChromeFrameMessageManager> globalMM =
      do_GetService("@mozilla.org/globalmessagemanager;1");
    mMessageManager =
      new nsFrameMessageManager(PR_TRUE,
                                nsnull,
                                nsnull,
                                nsnull,
                                nsnull,
                                static_cast<nsFrameMessageManager*>(globalMM.get()),
                                cx);
    NS_ENSURE_TRUE(mMessageManager, NS_ERROR_OUT_OF_MEMORY);
  }
  CallQueryInterface(mMessageManager, aManager);
  return NS_OK;
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsGlobalModalWindow)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsGlobalModalWindow,
                                                  nsGlobalWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mReturnValue)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(ModalContentWindow, nsGlobalModalWindow)

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
                                    PRBool aForceReuseInnerWindow)
{
  
  
  if (aDocument) {
    mReturnValue = nsnull;
  }

  return nsGlobalWindow::SetNewDocument(aDocument, aState,
                                        aForceReuseInnerWindow);
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
  if (mMimeTypes)
    mMimeTypes->Invalidate();
  if (mPlugins)
    mPlugins->Invalidate();
}






DOMCI_DATA(Navigator, nsNavigator)


NS_INTERFACE_MAP_BEGIN(nsNavigator)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMClientInformation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorGeolocation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorDesktopNotification)
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

  if (mNotification)
  {
    mNotification->Shutdown();
    mNotification = nsnull;
  }
}





nsresult
NS_GetNavigatorUserAgent(nsAString& aUserAgent)
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

nsresult
NS_GetNavigatorPlatform(nsAString& aPlatform)
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
#elif defined(XP_MACOSX) && defined(__x86_64__)
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
nsresult
NS_GetNavigatorAppVersion(nsAString& aAppVersion)
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

    aAppVersion.Append(PRUnichar(')'));
  }

  return rv;
}

nsresult
NS_GetNavigatorAppName(nsAString& aAppName)
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
nsNavigator::GetUserAgent(nsAString& aUserAgent)
{
  return NS_GetNavigatorUserAgent(aUserAgent);
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
  return NS_GetNavigatorAppVersion(aAppVersion);
}

NS_IMETHODIMP
nsNavigator::GetAppName(nsAString& aAppName)
{
  return NS_GetNavigatorAppName(aAppName);
}













NS_IMETHODIMP
nsNavigator::GetLanguage(nsAString& aLanguage)
{
  
  const nsAdoptingString& acceptLang =
      nsContentUtils::GetLocalizedStringPref("intl.accept_languages");
  
  nsCharSeparatedTokenizer langTokenizer(acceptLang, ',');
  const nsSubstring &firstLangPart = langTokenizer.nextToken();
  nsCharSeparatedTokenizer qTokenizer(firstLangPart, ';');
  aLanguage.Assign(qTokenizer.nextToken());

  
  
  if (aLanguage.Length() > 2 && aLanguage[2] == PRUnichar('_'))
    aLanguage.Replace(2, 1, PRUnichar('-')); 
  
  
  if (aLanguage.Length() > 2)
  {
    nsCharSeparatedTokenizer localeTokenizer(aLanguage, '-');
    PRInt32 pos = 0;
    bool first = true;
    while (localeTokenizer.hasMoreTokens())
    {
      const nsSubstring &code = localeTokenizer.nextToken();
      if (code.Length() == 2 && !first)
      {
        nsAutoString upper(code);
        ::ToUpperCase(upper);
        aLanguage.Replace(pos, code.Length(), upper);
      }
      pos += code.Length() + 1; 
      if (first)
        first = false;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavigator::GetPlatform(nsAString& aPlatform)
{
  return NS_GetNavigatorPlatform(aPlatform);
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
  aVendor.Truncate();
  return NS_OK;
}


NS_IMETHODIMP
nsNavigator::GetVendorSub(nsAString& aVendorSub)
{
  aVendorSub.Truncate();
  return NS_OK;
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

void
nsNavigator::LoadingNewDocument()
{
  
  
  
  if (mMimeTypes) {
    mMimeTypes->Invalidate();
    mMimeTypes = nsnull;
  }

  if (mPlugins) {
    mPlugins->Invalidate();
    mPlugins = nsnull;
  }

  if (mGeolocation)
  {
    mGeolocation->Shutdown();
    mGeolocation = nsnull;
  }

  if (mNotification)
  {
    mNotification->Shutdown();
    mNotification = nsnull;
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

bool
nsNavigator::HasDesktopNotificationSupport()
{
  return nsContentUtils::GetBoolPref("notification.feature.enabled", PR_FALSE);
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
  *_retval = nsnull;

  if (mGeolocation) {
    NS_ADDREF(*_retval = mGeolocation);
    return NS_OK;
  }

  if (!mDocShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMWindow> contentDOMWindow(do_GetInterface(mDocShell));
  if (!contentDOMWindow)
    return NS_ERROR_FAILURE;
    
  mGeolocation = new nsGeolocation();
  if (!mGeolocation)
    return NS_ERROR_FAILURE;
  
  if (NS_FAILED(mGeolocation->Init(contentDOMWindow))) {
    mGeolocation = nsnull;
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*_retval = mGeolocation);    
  return NS_OK; 
}






NS_IMETHODIMP nsNavigator::GetMozNotification(nsIDOMDesktopNotificationCenter **aRetVal)
{
  NS_ENSURE_ARG_POINTER(aRetVal);
  *aRetVal = nsnull;

  if (mNotification) {
    NS_ADDREF(*aRetVal = mNotification);
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> window(do_GetInterface(mDocShell));
  NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);
    
  nsCOMPtr<nsIDocument> document = do_GetInterface(mDocShell);
  NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);

  nsIScriptGlobalObject *sgo = document->GetScopeObject();
  NS_ENSURE_TRUE(sgo, NS_ERROR_FAILURE);

  nsIScriptContext *scx = sgo->GetContext();
  NS_ENSURE_TRUE(scx, NS_ERROR_FAILURE);

  mNotification = new nsDesktopNotificationCenter(window->GetCurrentInnerWindow(),
                                                  scx);
  if (!mNotification) {
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*aRetVal = mNotification);    
  return NS_OK; 
}
