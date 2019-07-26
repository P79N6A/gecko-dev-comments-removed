





#include "base/basictypes.h"
#include <algorithm>


#include "mozilla/Util.h"


#include "nsGlobalWindow.h"
#include "Navigator.h"
#include "nsScreen.h"
#include "nsHistory.h"
#include "nsPerformance.h"
#include "nsDOMNavigationTiming.h"
#include "nsBarProps.h"
#include "nsDOMStorage.h"
#include "nsDOMOfflineResourceList.h"
#include "nsError.h"
#include "nsIIdleService.h"
#include "nsIPowerManagerService.h"
#include "nsISizeOfEventTarget.h"

#ifdef XP_WIN
#ifdef GetClassName
#undef GetClassName
#endif 
#endif 


#include "nsXPIDLString.h"
#include "nsJSUtils.h"
#include "jsapi.h"              
#include "jsdbgapi.h"           
#include "jsfriendapi.h"        
#include "jswrapper.h"
#include "nsReadableUtils.h"
#include "nsDOMClassInfo.h"
#include "nsJSEnvironment.h"
#include "nsCharSeparatedTokenizer.h" 
#include "nsUnicharUtils.h"
#include "mozilla/Preferences.h"
#include "mozilla/Likely.h"


#include "nsEventListenerManager.h"
#include "nsEscape.h"
#include "nsStyleCoord.h"
#include "nsMimeTypeArray.h"
#include "nsNetUtil.h"
#include "nsICachingChannel.h"
#include "nsPluginArray.h"
#include "nsIPluginHost.h"
#include "nsPluginHost.h"
#include "nsIPluginInstanceOwner.h"
#include "nsGeolocation.h"
#include "nsDesktopNotification.h"
#include "nsContentCID.h"
#include "nsLayoutStatics.h"
#include "nsCycleCollector.h"
#include "nsCCUncollectableMarker.h"
#include "nsAutoJSValHolder.h"
#include "nsDOMMediaQueryList.h"
#include "mozilla/dom/workers/Workers.h"
#include "nsJSPrincipals.h"
#include "mozilla/Attributes.h"


#include "nsIFrame.h"
#include "nsCanvasFrame.h"
#include "nsIWidget.h"
#include "nsIWidgetListener.h"
#include "nsIBaseWindow.h"
#include "nsDeviceSensors.h"
#include "nsIContent.h"
#include "nsIContentViewerEdit.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIDocCharset.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "Crypto.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
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
#include "nsIEmbeddingSiteWindow.h"
#include "nsThreadUtils.h"
#include "nsEventStateManager.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsILoadContext.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIPresShell.h"
#include "nsIProgrammingLanguage.h"
#include "nsIServiceManager.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScrollableFrame.h"
#include "nsView.h"
#include "nsViewManager.h"
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
#include "nsDOMWindowUtils.h"
#include "nsIWindowWatcher.h"
#include "nsPIWindowWatcher.h"
#include "nsIContentViewer.h"
#include "nsIJSNativeInitializer.h"
#include "nsIScriptError.h"
#include "nsIConsoleService.h"
#include "nsIControllers.h"
#include "nsIControllerContext.h"
#include "nsGlobalWindowCommands.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsCSSProps.h"
#include "nsIDOMFile.h"
#include "nsIDOMFileList.h"
#include "nsIURIFixup.h"
#include "nsIAppStartup.h"
#include "nsToolkitCompsCID.h"
#include "nsCDefaultURIFixup.h"
#include "nsEventDispatcher.h"
#include "nsIObserverService.h"
#include "nsIXULAppInfo.h"
#include "nsNetUtil.h"
#include "nsFocusManager.h"
#include "nsIXULWindow.h"
#include "nsEventStateManager.h"
#include "nsITimedChannel.h"
#include "nsICookiePermission.h"
#include "nsServiceManagerUtils.h"
#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#include "nsIDOMXULControlElement.h"
#include "nsMenuPopupFrame.h"
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
#include "nsXBLService.h"



#include "nsIPopupWindowManager.h"

#include "nsIDragService.h"
#include "mozilla/dom/Element.h"
#include "mozilla/Selection.h"
#include "nsFrameLoader.h"
#include "nsISupportsPrimitives.h"
#include "nsXPCOMCID.h"
#include "GeneratedEvents.h"
#include "mozIThirdPartyUtil.h"

#ifdef MOZ_LOGGING

#define FORCE_PR_LOG 1
#endif
#include "prlog.h"
#include "prenv.h"

#include "mozilla/dom/indexedDB/IDBFactory.h"
#include "mozilla/dom/indexedDB/IndexedDatabaseManager.h"

#include "mozilla/dom/StructuredCloneTags.h"

#ifdef MOZ_GAMEPAD
#include "mozilla/dom/GamepadService.h"
#endif

#include "nsRefreshDriver.h"
#include "mozAutoDocUpdate.h"

#include "mozilla/Telemetry.h"
#include "nsLocation.h"
#include "nsWrapperCacheInlines.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIAppsService.h"
#include "prrng.h"
#include "nsSandboxFlags.h"
#include "TimeChangeObserver.h"
#include "nsPISocketTransportService.h"
#include "mozilla/dom/AudioContext.h"


#ifdef check
#undef check
#endif 
#include "AccessCheck.h"

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef PR_LOGGING
static PRLogModuleInfo* gDOMLeakPRLog;
#endif

static const char kStorageEnabled[] = "dom.storage.enabled";

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::dom::ipc;
using mozilla::TimeStamp;
using mozilla::TimeDuration;

nsGlobalWindow::WindowByIdTable *nsGlobalWindow::sWindowsById = nullptr;
bool nsGlobalWindow::sWarnedAboutWindowInternal = false;
bool nsGlobalWindow::sIdleObserversAPIFuzzTimeDisabled = false;

static nsIEntropyCollector *gEntropyCollector          = nullptr;
static int32_t              gRefCnt                    = 0;
static int32_t              gOpenPopupSpamCount        = 0;
static PopupControlState    gPopupControlState         = openAbused;
static int32_t              gRunningTimeoutDepth       = 0;
static bool                 gMouseDown                 = false;
static bool                 gDragServiceDisabled       = false;
static FILE                *gDumpFile                  = nullptr;
static uint64_t             gNextWindowID              = 0;
static uint32_t             gSerialCounter             = 0;
static uint32_t             gTimeoutsRecentlySet       = 0;
static TimeStamp            gLastRecordedRecentTimeouts;
#define STATISTICS_INTERVAL (30 * PR_MSEC_PER_SEC)

#ifdef DEBUG_jst
int32_t gTimeoutCnt                                    = 0;
#endif

#if !(defined(DEBUG) || defined(MOZ_ENABLE_JS_DUMP))
static bool                 gDOMWindowDumpEnabled      = false;
#endif

#if defined(DEBUG_bryner) || defined(DEBUG_chb)
#define DEBUG_PAGE_CACHE
#endif

#define DOM_TOUCH_LISTENER_ADDED "dom-touch-listener-added"


#define DEFAULT_MIN_TIMEOUT_VALUE 4 // 4ms
#define DEFAULT_MIN_BACKGROUND_TIMEOUT_VALUE 1000 // 1000ms
static int32_t gMinTimeoutValue;
static int32_t gMinBackgroundTimeoutValue;
inline int32_t
nsGlobalWindow::DOMMinTimeoutValue() const {
  bool isBackground = !mOuterWindow || mOuterWindow->IsBackground();
  return
    std::max(isBackground ? gMinBackgroundTimeoutValue : gMinTimeoutValue, 0);
}



#define DOM_CLAMP_TIMEOUT_NESTING_LEVEL 5




#define DOM_MAX_TIMEOUT_VALUE    DELAY_INTERVAL_LIMIT

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
static const char sPopStatePrefStr[] = "browser.history.allowPopState";

#define NETWORK_UPLOAD_EVENT_NAME     NS_LITERAL_STRING("moznetworkupload")
#define NETWORK_DOWNLOAD_EVENT_NAME   NS_LITERAL_STRING("moznetworkdownload")





class nsGlobalWindowObserver MOZ_FINAL : public nsIObserver,
                                         public nsIInterfaceRequestor
{
public:
  nsGlobalWindowObserver(nsGlobalWindow* aWindow) : mWindow(aWindow) {}
  NS_DECL_ISUPPORTS
  NS_IMETHOD Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
  {
    if (!mWindow)
      return NS_OK;
    return mWindow->Observe(aSubject, aTopic, aData);
  }
  void Forget() { mWindow = nullptr; }
  NS_IMETHODIMP GetInterface(const nsIID& aIID, void** aResult)
  {
    if (mWindow && aIID.Equals(NS_GET_IID(nsIDOMWindow)) && mWindow) {
      return mWindow->QueryInterface(aIID, aResult);
    }
    return NS_NOINTERFACE;
  }

private:
  nsGlobalWindow* mWindow;
};

NS_IMPL_ISUPPORTS2(nsGlobalWindowObserver, nsIObserver, nsIInterfaceRequestor)

nsTimeout::nsTimeout()
  : mCleared(false),
    mRunning(false),
    mIsInterval(false),
    mPublicId(0),
    mInterval(0),
    mFiringDepth(0),
    mNestingLevel(0),
    mPopupState(openAllowed)
{
#ifdef DEBUG_jst
  {
    extern int gTimeoutCnt;

    ++gTimeoutCnt;
  }
#endif

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

NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsTimeout)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsTimeout)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPrincipal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mScriptHandler)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsTimeout, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsTimeout, Release)

nsPIDOMWindow::nsPIDOMWindow(nsPIDOMWindow *aOuterWindow)
: mFrameElement(nullptr), mDocShell(nullptr), mModalStateDepth(0),
  mRunningTimeout(nullptr), mMutationBits(0), mIsDocumentLoaded(false),
  mIsHandlingResizeEvent(false), mIsInnerWindow(aOuterWindow != nullptr),
  mMayHavePaintEventListener(false), mMayHaveTouchEventListener(false),
  mMayHaveMouseEnterLeaveEventListener(false),
  mIsModalContentWindow(false),
  mIsActive(false), mIsBackground(false),
  mInnerWindow(nullptr), mOuterWindow(aOuterWindow),
  
  mWindowID(++gNextWindowID), mHasNotifiedGlobalCreated(false)
 {}

nsPIDOMWindow::~nsPIDOMWindow() {}





class nsOuterWindowProxy : public js::Wrapper
{
public:
  nsOuterWindowProxy() : js::Wrapper(0) { }

  virtual bool isOuterWindow() {
    return true;
  }

  virtual bool finalizeInBackground(JS::Value priv) {
    return false;
  }

  virtual JSString *obj_toString(JSContext *cx,
                                 JS::Handle<JSObject*> wrapper) MOZ_OVERRIDE;
  virtual void finalize(JSFreeOp *fop, JSObject *proxy) MOZ_OVERRIDE;

  
  virtual bool getPropertyDescriptor(JSContext* cx,
                                     JS::Handle<JSObject*> proxy,
                                     JS::Handle<jsid> id,
                                     JSPropertyDescriptor* desc,
                                     unsigned flags) MOZ_OVERRIDE;
  virtual bool getOwnPropertyDescriptor(JSContext* cx,
                                        JS::Handle<JSObject*> proxy,
                                        JS::Handle<jsid> id,
                                        JSPropertyDescriptor* desc,
                                        unsigned flags) MOZ_OVERRIDE;
  virtual bool defineProperty(JSContext* cx,
                              JS::Handle<JSObject*> proxy,
                              JS::Handle<jsid> id,
                              JSPropertyDescriptor* desc) MOZ_OVERRIDE;
  virtual bool getOwnPropertyNames(JSContext *cx,
                                   JS::Handle<JSObject*> proxy,
                                   JS::AutoIdVector &props) MOZ_OVERRIDE;
  virtual bool delete_(JSContext *cx, JS::Handle<JSObject*> proxy,
                       JS::Handle<jsid> id,
                       bool *bp) MOZ_OVERRIDE;
  virtual bool enumerate(JSContext *cx, JS::Handle<JSObject*> proxy,
                         JS::AutoIdVector &props) MOZ_OVERRIDE;

  
  virtual bool has(JSContext *cx, JS::Handle<JSObject*> proxy,
                   JS::Handle<jsid> id, bool *bp) MOZ_OVERRIDE;
  virtual bool hasOwn(JSContext *cx, JS::Handle<JSObject*> proxy,
                      JS::Handle<jsid> id, bool *bp) MOZ_OVERRIDE;
  virtual bool get(JSContext *cx, JS::Handle<JSObject*> proxy,
                   JS::Handle<JSObject*> receiver,
                   JS::Handle<jsid> id,
                   JS::MutableHandle<JS::Value> vp) MOZ_OVERRIDE;
  virtual bool set(JSContext *cx, JS::Handle<JSObject*> proxy,
                   JS::Handle<JSObject*> receiver,
                   JS::Handle<jsid> id,
                   bool strict,
                   JS::MutableHandle<JS::Value> vp) MOZ_OVERRIDE;
  virtual bool keys(JSContext *cx, JS::Handle<JSObject*> proxy,
                    JS::AutoIdVector &props) MOZ_OVERRIDE;
  virtual bool iterate(JSContext *cx, JS::Handle<JSObject*> proxy,
                       unsigned flags,
                       JS::MutableHandle<JS::Value> vp) MOZ_OVERRIDE;

  static nsOuterWindowProxy singleton;

protected:
  nsGlobalWindow* GetWindow(JSObject *proxy)
  {
    return nsGlobalWindow::FromSupports(
      static_cast<nsISupports*>(js::GetProxyExtra(proxy, 0).toPrivate()));
  }

  
  
  bool GetSubframeWindow(JSContext *cx, JSObject *proxy, jsid id,
                         JS::Value *vp, bool &found);

  
  
  already_AddRefed<nsIDOMWindow> GetSubframeWindow(JSContext *cx,
                                                   JSObject *proxy,
                                                   jsid id);

  bool AppendIndexedPropertyNames(JSContext *cx, JSObject *proxy,
                                  JS::AutoIdVector &props);
};


JSString *
nsOuterWindowProxy::obj_toString(JSContext *cx, JS::Handle<JSObject*> proxy)
{
    MOZ_ASSERT(js::IsProxy(proxy));

    return JS_NewStringCopyZ(cx, "[object Window]");
}

void
nsOuterWindowProxy::finalize(JSFreeOp *fop, JSObject *proxy)
{
  nsGlobalWindow* global = GetWindow(proxy);
  if (global) {
    global->ClearWrapper();
  }
}

bool
nsOuterWindowProxy::getPropertyDescriptor(JSContext* cx,
                                          JS::Handle<JSObject*> proxy,
                                          JS::Handle<jsid> id,
                                          JSPropertyDescriptor* desc,
                                          unsigned flags)
{
  
  
  
  desc->obj = nullptr;
  if (!getOwnPropertyDescriptor(cx, proxy, id, desc, flags)) {
    return false;
  }

  if (desc->obj) {
    return true;
  }

  return js::Wrapper::getPropertyDescriptor(cx, proxy, id, desc, flags);
}

bool
nsOuterWindowProxy::getOwnPropertyDescriptor(JSContext* cx,
                                             JS::Handle<JSObject*> proxy,
                                             JS::Handle<jsid> id,
                                             JSPropertyDescriptor* desc,
                                             unsigned flags)
{
  bool found;
  if (!GetSubframeWindow(cx, proxy, id, &desc->value, found)) {
    return false;
  }
  if (found) {
    FillPropertyDescriptor(desc, proxy, true);
    return true;
  }
  

  return js::Wrapper::getOwnPropertyDescriptor(cx, proxy, id, desc, flags);
}

bool
nsOuterWindowProxy::defineProperty(JSContext* cx,
                                   JS::Handle<JSObject*> proxy,
                                   JS::Handle<jsid> id,
                                   JSPropertyDescriptor* desc)
{
  int32_t index = GetArrayIndexFromId(cx, id);
  if (IsArrayIndex(index)) {
    
    
    
    
    return true;
  }

  return js::Wrapper::defineProperty(cx, proxy, id, desc);
}

bool
nsOuterWindowProxy::getOwnPropertyNames(JSContext *cx,
                                        JS::Handle<JSObject*> proxy,
                                        JS::AutoIdVector &props)
{
  
  if (!AppendIndexedPropertyNames(cx, proxy, props)) {
    return false;
  }

  JS::AutoIdVector innerProps(cx);
  if (!js::Wrapper::getOwnPropertyNames(cx, proxy, innerProps)) {
    return false;
  }
  return js::AppendUnique(cx, props, innerProps);
}

bool
nsOuterWindowProxy::delete_(JSContext *cx, JS::Handle<JSObject*> proxy,
                            JS::Handle<jsid> id, bool *bp)
{
  if (nsCOMPtr<nsIDOMWindow> frame = GetSubframeWindow(cx, proxy, id)) {
    
    
    *bp = false;
    return true;
  }

  int32_t index = GetArrayIndexFromId(cx, id);
  if (IsArrayIndex(index)) {
    
    *bp = true;
    return true;
  }

  return js::Wrapper::delete_(cx, proxy, id, bp);
}

bool
nsOuterWindowProxy::enumerate(JSContext *cx, JS::Handle<JSObject*> proxy,
                              JS::AutoIdVector &props)
{
  
  if (!AppendIndexedPropertyNames(cx, proxy, props)) {
    return false;
  }

  JS::AutoIdVector innerProps(cx);
  if (!js::Wrapper::enumerate(cx, proxy, innerProps)) {
    return false;
  }
  return js::AppendUnique(cx, props, innerProps);
}

bool
nsOuterWindowProxy::has(JSContext *cx, JS::Handle<JSObject*> proxy,
                        JS::Handle<jsid> id, bool *bp)
{
  if (nsCOMPtr<nsIDOMWindow> frame = GetSubframeWindow(cx, proxy, id)) {
    *bp = true;
    return true;
  }

  return js::Wrapper::has(cx, proxy, id, bp);
}

bool
nsOuterWindowProxy::hasOwn(JSContext *cx, JS::Handle<JSObject*> proxy,
                           JS::Handle<jsid> id, bool *bp)
{
  if (nsCOMPtr<nsIDOMWindow> frame = GetSubframeWindow(cx, proxy, id)) {
    *bp = true;
    return true;
  }

  return js::Wrapper::hasOwn(cx, proxy, id, bp);
}

bool
nsOuterWindowProxy::get(JSContext *cx, JS::Handle<JSObject*> proxy,
                        JS::Handle<JSObject*> receiver,
                        JS::Handle<jsid> id,
                        JS::MutableHandle<JS::Value> vp)
{
  if (id == nsDOMClassInfo::sWrappedJSObject_id &&
      xpc::AccessCheck::isChrome(js::GetContextCompartment(cx))) {
    vp.set(JS::ObjectValue(*proxy));
    return true;
  }

  bool found;
  if (!GetSubframeWindow(cx, proxy, id, vp.address(), found)) {
    return false;
  }
  if (found) {
    return true;
  }
  

  return js::Wrapper::get(cx, proxy, receiver, id, vp);
}

bool
nsOuterWindowProxy::set(JSContext *cx, JS::Handle<JSObject*> proxy,
                        JS::Handle<JSObject*> receiver,
                        JS::Handle<jsid> id,
                        bool strict,
                        JS::MutableHandle<JS::Value> vp)
{
  int32_t index = GetArrayIndexFromId(cx, id);
  if (IsArrayIndex(index)) {
    
    if (strict) {
      
    }
    return true;
  }

  return js::Wrapper::set(cx, proxy, receiver, id, strict, vp);
}

bool
nsOuterWindowProxy::keys(JSContext *cx, JS::Handle<JSObject*> proxy,
                         JS::AutoIdVector &props)
{
  
  
  return js::BaseProxyHandler::keys(cx, proxy, props);
}

bool
nsOuterWindowProxy::iterate(JSContext *cx, JS::Handle<JSObject*> proxy,
                            unsigned flags, JS::MutableHandle<JS::Value> vp)
{
  
  
  return js::BaseProxyHandler::iterate(cx, proxy, flags, vp);
}

bool
nsOuterWindowProxy::GetSubframeWindow(JSContext *cx, JSObject *proxy,
                                      jsid id, JS::Value* vp,
                                      bool& found)
{
  nsCOMPtr<nsIDOMWindow> frame = GetSubframeWindow(cx, proxy, id);
  if (!frame) {
    found = false;
    return true;
  }

  found = true;
  
  nsGlobalWindow* global = static_cast<nsGlobalWindow*>(frame.get());
  global->EnsureInnerWindow();
  JSObject* obj = global->FastGetGlobalJSObject();
  
  
  
  if (MOZ_UNLIKELY(!obj)) {
    return xpc::Throw(cx, NS_ERROR_FAILURE);
  }
  *vp = JS::ObjectValue(*obj);
  return JS_WrapValue(cx, vp);
}

already_AddRefed<nsIDOMWindow>
nsOuterWindowProxy::GetSubframeWindow(JSContext *cx, JSObject *proxy, jsid id)
{
  int32_t index = GetArrayIndexFromId(cx, id);
  if (!IsArrayIndex(index)) {
    return nullptr;
  }

  nsGlobalWindow* win = GetWindow(proxy);
  bool unused;
  return win->IndexedGetter(index, unused);
}

bool
nsOuterWindowProxy::AppendIndexedPropertyNames(JSContext *cx, JSObject *proxy,
                                               JS::AutoIdVector &props)
{
  uint32_t length = GetWindow(proxy)->GetLength();
  MOZ_ASSERT(int32_t(length) >= 0);
  if (!props.reserve(props.length() + length)) {
    return false;
  }
  for (int32_t i = 0; i < int32_t(length); ++i) {
    props.append(INT_TO_JSID(i));
  }

  return true;
}

nsOuterWindowProxy
nsOuterWindowProxy::singleton;

class nsChromeOuterWindowProxy : public nsOuterWindowProxy
{
public:
  nsChromeOuterWindowProxy() : nsOuterWindowProxy() {}

  virtual JSString *obj_toString(JSContext *cx, JS::Handle<JSObject*> wrapper);

  static nsChromeOuterWindowProxy singleton;
};

JSString *
nsChromeOuterWindowProxy::obj_toString(JSContext *cx,
                                       JS::Handle<JSObject*> proxy)
{
    MOZ_ASSERT(js::IsProxy(proxy));

    return JS_NewStringCopyZ(cx, "[object ChromeWindow]");
}

nsChromeOuterWindowProxy
nsChromeOuterWindowProxy::singleton;

static JSObject*
NewOuterWindowProxy(JSContext *cx, JSObject *parent, bool isChrome)
{
  JSAutoCompartment ac(cx, parent);
  JSObject *proto;
  if (!js::GetObjectProto(cx, parent, &proto))
    return nullptr;

  JSObject *obj = js::Wrapper::New(cx, parent, proto, parent,
                                   isChrome ? &nsChromeOuterWindowProxy::singleton
                                            : &nsOuterWindowProxy::singleton);

  NS_ASSERTION(js::GetObjectClass(obj)->ext.innerObject, "bad class");
  return obj;
}





nsGlobalWindow::nsGlobalWindow(nsGlobalWindow *aOuterWindow)
  : nsPIDOMWindow(aOuterWindow),
    mIdleFuzzFactor(0),
    mIdleCallbackIndex(-1),
    mCurrentlyIdle(false),
    mAddActiveEventFuzzTime(true),
    mIsFrozen(false),
    mFullScreen(false),
    mIsClosed(false),
    mInClose(false),
    mHavePendingClose(false),
    mHadOriginalOpener(false),
    mIsPopupSpam(false),
    mBlockScriptedClosingFlag(false),
    mFireOfflineStatusChangeEventOnThaw(false),
    mNotifyIdleObserversIdleOnThaw(false),
    mNotifyIdleObserversActiveOnThaw(false),
    mCreatingInnerWindow(false),
    mIsChrome(false),
    mCleanMessageManager(false),
    mNeedsFocus(true),
    mHasFocus(false),
#if defined(XP_MACOSX)
    mShowAccelerators(false),
    mShowFocusRings(false),
#else
    mShowAccelerators(true),
    mShowFocusRings(true),
#endif
    mShowFocusRingForContent(false),
    mFocusByKeyOccurred(false),
    mHasGamepad(false),
#ifdef MOZ_GAMEPAD
    mHasSeenGamepadInput(false),
#endif
    mNotifiedIDDestroyed(false),
    mAllowScriptsToClose(false),
    mTimeoutInsertionPoint(nullptr),
    mTimeoutPublicIdCounter(1),
    mTimeoutFiringDepth(0),
    mJSObject(nullptr),
    mTimeoutsSuspendDepth(0),
    mFocusMethod(0),
    mSerial(0),
#ifdef DEBUG
    mSetOpenerWindowCalled(false),
#endif
    mCleanedUp(false),
    mCallCleanUpAfterModalDialogCloses(false),
    mDialogAbuseCount(0),
    mStopAbuseDialogs(false),
    mDialogsPermanentlyDisabled(false)
{
  nsLayoutStatics::AddRef();

#ifdef MOZ_GAMEPAD
  mGamepads.Init();
#endif

  
  PR_INIT_CLIST(this);

  if (aOuterWindow) {
    
    
    PR_INSERT_AFTER(this, aOuterWindow);

    mObserver = new nsGlobalWindowObserver(this);
    if (mObserver) {
      NS_ADDREF(mObserver);
      nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
      if (os) {
        
        
        os->AddObserver(mObserver, NS_IOSERVICE_OFFLINE_STATUS_TOPIC,
                        false);

        
        
        os->AddObserver(mObserver, "dom-storage2-changed", false);
      }
    }
  } else {
    
    
    
    Freeze();

    mObserver = nullptr;
    SetIsDOMBinding();
  }

  
  
  

  gRefCnt++;

  if (gRefCnt == 1) {
#if !(defined(DEBUG) || defined(MOZ_ENABLE_JS_DUMP))
    Preferences::AddBoolVarCache(&gDOMWindowDumpEnabled,
                                 "browser.dom.window.dump.enabled");
#endif
    Preferences::AddIntVarCache(&gMinTimeoutValue,
                                "dom.min_timeout_value",
                                DEFAULT_MIN_TIMEOUT_VALUE);
    Preferences::AddIntVarCache(&gMinBackgroundTimeoutValue,
                                "dom.min_background_timeout_value",
                                DEFAULT_MIN_BACKGROUND_TIMEOUT_VALUE);
    Preferences::AddBoolVarCache(&sIdleObserversAPIFuzzTimeDisabled, 
                                 "dom.idle-observers-api.fuzz_time.disabled",
                                 false);
  }

  if (gDumpFile == nullptr) {
    const nsAdoptingCString& fname =
      Preferences::GetCString("browser.dom.window.dump.file");
    if (!fname.IsEmpty()) {
      
      
      gDumpFile = fopen(fname, "wb+");
    } else {
      gDumpFile = stdout;
    }
  }

  mSerial = ++gSerialCounter;

#ifdef DEBUG
  if (!PR_GetEnv("MOZ_QUIET")) {
    printf("++DOMWINDOW == %d (%p) [serial = %d] [outer = %p]\n", gRefCnt,
           static_cast<void*>(static_cast<nsIScriptGlobalObject*>(this)),
           gSerialCounter,
           static_cast<void*>(static_cast<nsIScriptGlobalObject*>(aOuterWindow)));
  }
#endif

#ifdef PR_LOGGING
  if (gDOMLeakPRLog)
    PR_LOG(gDOMLeakPRLog, PR_LOG_DEBUG,
           ("DOMWINDOW %p created outer=%p", this, aOuterWindow));
#endif

  NS_ASSERTION(sWindowsById, "Windows hash table must be created!");
  NS_ASSERTION(!sWindowsById->Get(mWindowID),
               "This window shouldn't be in the hash table yet!");
  
  if (sWindowsById) {
    sWindowsById->Put(mWindowID, this);
  }

  mEventTargetObjects.Init();
}


void
nsGlobalWindow::Init()
{
  CallGetService(NS_ENTROPYCOLLECTOR_CONTRACTID, &gEntropyCollector);
  NS_ASSERTION(gEntropyCollector,
               "gEntropyCollector should have been initialized!");

#ifdef PR_LOGGING
  gDOMLeakPRLog = PR_NewLogModule("DOMLeak");
  NS_ASSERTION(gDOMLeakPRLog, "gDOMLeakPRLog should have been initialized!");
#endif

  sWindowsById = new WindowByIdTable();
  sWindowsById->Init();
}

static PLDHashOperator
DisconnectEventTargetObjects(nsPtrHashKey<nsDOMEventTargetHelper>* aKey,
                             void* aClosure)
{
  nsRefPtr<nsDOMEventTargetHelper> target = aKey->GetKey();
  target->DisconnectFromOwner();
  return PL_DHASH_NEXT;
}

nsGlobalWindow::~nsGlobalWindow()
{
  mEventTargetObjects.EnumerateEntries(DisconnectEventTargetObjects, nullptr);
  mEventTargetObjects.Clear();

  
  
  if (sWindowsById) {
    NS_ASSERTION(sWindowsById->Get(mWindowID),
                 "This window should be in the hash table");
    sWindowsById->Remove(mWindowID);
  }

  --gRefCnt;

#ifdef DEBUG
  if (!PR_GetEnv("MOZ_QUIET")) {
    nsAutoCString url;
    if (mLastOpenedURI) {
      mLastOpenedURI->GetSpec(url);

      
      const uint32_t maxURLLength = 1000;
      if (url.Length() > maxURLLength) {
        url.Truncate(maxURLLength);
      }
    }

    nsGlobalWindow* outer = static_cast<nsGlobalWindow*>(mOuterWindow.get());
    printf("--DOMWINDOW == %d (%p) [serial = %d] [outer = %p] [url = %s]\n",
           gRefCnt, static_cast<void*>(static_cast<nsIScriptGlobalObject*>(this)),
           mSerial, static_cast<void*>(static_cast<nsIScriptGlobalObject*>(outer)), url.get());
  }
#endif

#ifdef PR_LOGGING
  if (gDOMLeakPRLog)
    PR_LOG(gDOMLeakPRLog, PR_LOG_DEBUG,
           ("DOMWINDOW %p destroyed", this));
#endif

  if (IsOuterWindow()) {
    JSObject *proxy = GetWrapperPreserveColor();
    if (proxy) {
      js::SetProxyExtra(proxy, 0, js::PrivateValue(NULL));
    }

    
    
    
    

    nsGlobalWindow *w;
    while ((w = (nsGlobalWindow *)PR_LIST_HEAD(this)) != this) {
      PR_REMOVE_AND_INIT_LINK(w);
    }
  } else {
    Telemetry::Accumulate(Telemetry::INNERWINDOWS_WITH_MUTATION_LISTENERS,
                          mMutationBits ? 1 : 0);

    if (mListenerManager) {
      mListenerManager->Disconnect();
      mListenerManager = nullptr;
    }

    
    

    PR_REMOVE_LINK(this);

    
    
    nsGlobalWindow *outer = GetOuterWindowInternal();
    if (outer) {
      outer->MaybeClearInnerWindow(this);
    }
  }

  mDocument = nullptr;           
  mDoc = nullptr;

  NS_ASSERTION(!mArguments, "mArguments wasn't cleaned up properly!");

  CleanUp(true);

  nsCOMPtr<nsIDeviceSensors> ac = do_GetService(NS_DEVICE_SENSORS_CONTRACTID);
  if (ac)
    ac->RemoveWindowAsListener(this);

  nsLayoutStatics::Release();
}

void
nsGlobalWindow::AddEventTargetObject(nsDOMEventTargetHelper* aObject)
{
  mEventTargetObjects.PutEntry(aObject);
}

void
nsGlobalWindow::RemoveEventTargetObject(nsDOMEventTargetHelper* aObject)
{
  mEventTargetObjects.RemoveEntry(aObject);
}


void
nsGlobalWindow::ShutDown()
{
  if (gDumpFile && gDumpFile != stdout) {
    fclose(gDumpFile);
  }
  gDumpFile = nullptr;

  NS_IF_RELEASE(gEntropyCollector);

  delete sWindowsById;
  sWindowsById = nullptr;
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
    SetPopupSpamWindow(false);
    --gOpenPopupSpamCount;
    NS_ASSERTION(gOpenPopupSpamCount >= 0,
                 "Unbalanced decrement of gOpenPopupSpamCount");
  }
}

void
nsGlobalWindow::CleanUp(bool aIgnoreModalDialog)
{
  if (IsOuterWindow() && !aIgnoreModalDialog) {
    nsGlobalWindow* inner = GetCurrentInnerWindowInternal();
    nsCOMPtr<nsIDOMModalContentWindow> dlg(do_QueryObject(inner));
    if (dlg) {
      
      
      
      mCallCleanUpAfterModalDialogCloses = true;
      return;
    }
  }

  
  if (mCleanedUp)
    return;
  mCleanedUp = true;

  mEventTargetObjects.EnumerateEntries(DisconnectEventTargetObjects, nullptr);
  mEventTargetObjects.Clear();

  if (mObserver) {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
      os->RemoveObserver(mObserver, NS_IOSERVICE_OFFLINE_STATUS_TOPIC);
      os->RemoveObserver(mObserver, "dom-storage2-changed");
    }

#ifdef MOZ_B2G
    DisableNetworkEvent(NS_NETWORK_UPLOAD_EVENT);
    DisableNetworkEvent(NS_NETWORK_DOWNLOAD_EVENT);
#endif 

    if (mIdleService) {
      mIdleService->RemoveIdleObserver(mObserver, MIN_IDLE_NOTIFICATION_TIME_S);
    }

    
    
    mObserver->Forget();
    NS_RELEASE(mObserver);
  }

  mNavigator = nullptr;
  mScreen = nullptr;
  mMenubar = nullptr;
  mToolbar = nullptr;
  mLocationbar = nullptr;
  mPersonalbar = nullptr;
  mStatusbar = nullptr;
  mScrollbars = nullptr;
  mLocation = nullptr;
  mHistory = nullptr;
  mFrames = nullptr;
  mWindowUtils = nullptr;
  mApplicationCache = nullptr;
  mIndexedDB = nullptr;

  mPerformance = nullptr;

  ClearControllers();

  mOpener = nullptr;             
  if (mContext) {
    mContext = nullptr;            
  }
  mChromeEventHandler = nullptr; 
  mParentTarget = nullptr;

  nsGlobalWindow *inner = GetCurrentInnerWindowInternal();

  if (inner) {
    inner->CleanUp(aIgnoreModalDialog);
  }

  DisableGamepadUpdates();
  mHasGamepad = false;

  if (mCleanMessageManager) {
    NS_ABORT_IF_FALSE(mIsChrome, "only chrome should have msg manager cleaned");
    nsGlobalChromeWindow *asChrome = static_cast<nsGlobalChromeWindow*>(this);
    if (asChrome->mMessageManager) {
      static_cast<nsFrameMessageManager*>(
        asChrome->mMessageManager.get())->Disconnect();
    }
  }

  mInnerWindowHolder = nullptr;
  mArguments = nullptr;
  mArgumentsLast = nullptr;
  mArgumentsOrigin = nullptr;

  CleanupCachedXBLHandlers(this);

  if (mIdleTimer) {
    mIdleTimer->Cancel();
    mIdleTimer = nullptr;
  }

  DisableTimeChangeNotifications();
}

void
nsGlobalWindow::ClearControllers()
{
  if (mControllers) {
    uint32_t count;
    mControllers->GetControllerCount(&count);

    while (count--) {
      nsCOMPtr<nsIController> controller;
      mControllers->GetControllerAt(count, getter_AddRefs(controller));

      nsCOMPtr<nsIControllerContext> context = do_QueryInterface(controller);
      if (context)
        context->SetCommandContext(nullptr);
    }

    mControllers = nullptr;
  }
}

void
nsGlobalWindow::FreeInnerObjects()
{
  NS_ASSERTION(IsInnerWindow(), "Don't free inner objects on an outer window");

  
  
  
  NotifyDOMWindowDestroyed(this);

  
  nsIScriptContext *scx = GetContextInternal();
  AutoPushJSContext cx(scx ? scx->GetNativeContext() : nullptr);
  mozilla::dom::workers::CancelWorkersForWindow(cx, this);

  
  indexedDB::IndexedDatabaseManager* idbManager =
    indexedDB::IndexedDatabaseManager::Get();
  if (idbManager) {
    idbManager->AbortCloseDatabasesForWindow(this);
  }

  ClearAllTimeouts();

  if (mIdleTimer) {
    mIdleTimer->Cancel();
    mIdleTimer = nullptr;
  }

  mIdleObservers.Clear();

  mChromeEventHandler = nullptr;

  if (mListenerManager) {
    mListenerManager->Disconnect();
    mListenerManager = nullptr;
  }

  mLocation = nullptr;
  mHistory = nullptr;

  if (mNavigator) {
    mNavigator->OnNavigation();
    mNavigator->Invalidate();
    mNavigator = nullptr;
  }

  if (mScreen) {
    mScreen->Reset();
    mScreen = nullptr;
  }

  if (mDocument) {
    NS_ASSERTION(mDoc, "Why is mDoc null?");

    
    mDocumentPrincipal = mDoc->NodePrincipal();
    mDocumentURI = mDoc->GetDocumentURI();
    mDocBaseURI = mDoc->GetDocBaseURI();
  }

  
  mDocument = nullptr;
  mDoc = nullptr;
  mFocusedNode = nullptr;

  if (mApplicationCache) {
    static_cast<nsDOMOfflineResourceList*>(mApplicationCache.get())->Disconnect();
    mApplicationCache = nullptr;
  }

  mIndexedDB = nullptr;

  NotifyWindowIDDestroyed("inner-window-destroyed");

  CleanupCachedXBLHandlers(this);

  for (uint32_t i = 0; i < mAudioContexts.Length(); ++i) {
    mAudioContexts[i]->Shutdown();
  }
  mAudioContexts.Clear();
}





#define OUTER_WINDOW_ONLY                                                     \
  if (IsOuterWindow()) {

#define END_OUTER_WINDOW_ONLY                                                 \
    foundInterface = 0;                                                       \
  } else

DOMCI_DATA(Window, nsGlobalWindow)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsGlobalWindow)
  
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWindow)
#ifdef MOZ_B2G
  NS_INTERFACE_MAP_ENTRY(nsIDOMWindowB2G)
#endif 
  NS_INTERFACE_MAP_ENTRY(nsIDOMJSWindow)
  if (aIID.Equals(NS_GET_IID(nsIDOMWindowInternal))) {
    foundInterface = static_cast<nsIDOMWindowInternal*>(this);
    if (!sWarnedAboutWindowInternal) {
      sWarnedAboutWindowInternal = true;
      nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                      "Extensions", mDoc,
                                      nsContentUtils::eDOM_PROPERTIES,
                                      "nsIDOMWindowInternalWarning");
    }
  } else
  NS_INTERFACE_MAP_ENTRY(nsIScriptGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(mozilla::dom::EventTarget)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMWindow)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageIndexedDB)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWindowPerformance)
  NS_INTERFACE_MAP_ENTRY(nsITouchEventReceiver)
  NS_INTERFACE_MAP_ENTRY(nsIInlineEventHandlers)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Window)
  OUTER_WINDOW_ONLY
    NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  END_OUTER_WINDOW_ONLY
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsGlobalWindow)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsGlobalWindow)

static PLDHashOperator
MarkXBLHandlers(nsXBLPrototypeHandler* aKey, JSObject* aData, void* aClosure)
{
  xpc_UnmarkGrayObject(aData);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(nsGlobalWindow)
  if (tmp->IsBlackForCC()) {
    if (tmp->mCachedXBLPrototypeHandlers.IsInitialized()) {
      tmp->mCachedXBLPrototypeHandlers.EnumerateRead(MarkXBLHandlers, nullptr);
    }
    nsEventListenerManager* elm = tmp->GetListenerManager(false);
    if (elm) {
      elm->MarkForCC();
    }
    tmp->UnmarkGrayTimers();
    return true;
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(nsGlobalWindow)
  return tmp->IsBlackForCC();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(nsGlobalWindow)
  return tmp->IsBlackForCC();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            IdleObserverHolder& aField,
                            const char* aName,
                            unsigned aFlags)
{
  CycleCollectionNoteChild(aCallback, aField.mIdleObserver.get(), aName, aFlags);
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(nsGlobalWindow)
  if (MOZ_UNLIKELY(cb.WantDebugInfo())) {
    char name[512];
    PR_snprintf(name, sizeof(name), "nsGlobalWindow #%ld", tmp->mWindowID);
    cb.DescribeRefCountedNode(tmp->mRefCnt.get(), name);
  } else {
    NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsGlobalWindow, tmp->mRefCnt.get())
  }

  if (!cb.WantAllTraces() && tmp->IsBlackForCC()) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mContext)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mControllers)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mArguments)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mArgumentsLast)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPerformance)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mInnerWindowHolder)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOuterWindow)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mListenerManager)

  for (nsTimeout* timeout = tmp->mTimeouts.getFirst();
       timeout;
       timeout = timeout->getNext()) {
    cb.NoteNativeChild(timeout, NS_CYCLE_COLLECTION_PARTICIPANT(nsTimeout));
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mLocalStorage)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSessionStorage)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mApplicationCache)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocumentPrincipal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDoc)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIdleService)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPendingStorageEvents)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIdleObservers)

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mChromeEventHandler)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mParentTarget)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFrameElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFocusedNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAudioContexts)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsGlobalWindow)
  nsGlobalWindow::CleanupCachedXBLHandlers(tmp);

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mContext)

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mControllers)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mArguments)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mArgumentsLast)

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPerformance)

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mInnerWindowHolder)
  if (tmp->mOuterWindow) {
    static_cast<nsGlobalWindow*>(tmp->mOuterWindow.get())->MaybeClearInnerWindow(tmp);
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mOuterWindow)
  }

  if (tmp->mListenerManager) {
    tmp->mListenerManager->Disconnect();
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mListenerManager)
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mLocalStorage)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mSessionStorage)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mApplicationCache)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDocumentPrincipal)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDoc)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mIdleService)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPendingStorageEvents)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mIdleObservers)

  
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mChromeEventHandler)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mParentTarget)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mFrameElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mFocusedNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mAudioContexts)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

struct TraceData
{
  TraceData(TraceCallback& aCallback, void* aClosure) :
    callback(aCallback), closure(aClosure) {}

  TraceCallback& callback;
  void* closure;
};

static PLDHashOperator
TraceXBLHandlers(nsXBLPrototypeHandler* aKey, JSObject* aData, void* aClosure)
{
  TraceData* data = static_cast<TraceData*>(aClosure);
  data->callback(aData, "Cached XBL prototype handler", data->closure);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsGlobalWindow)
  if (tmp->mCachedXBLPrototypeHandlers.IsInitialized()) {
    TraceData data(aCallback, aClosure);
    tmp->mCachedXBLPrototypeHandlers.EnumerateRead(TraceXBLHandlers, &data);
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

bool
nsGlobalWindow::IsBlackForCC()
{
  return
    (mDoc &&
     nsCCUncollectableMarker::InGeneration(mDoc->GetMarkedCCGeneration())) ||
    (nsCCUncollectableMarker::sGeneration && IsBlack());
}

void
nsGlobalWindow::UnmarkGrayTimers()
{
  for (nsTimeout* timeout = mTimeouts.getFirst();
       timeout;
       timeout = timeout->getNext()) {
    if (timeout->mScriptHandler) {
      JSObject* o = timeout->mScriptHandler->GetScriptObject();
      xpc_UnmarkGrayObject(o);
    }
  }
}





nsresult
nsGlobalWindow::EnsureScriptEnvironment()
{
  FORWARD_TO_OUTER(EnsureScriptEnvironment, (), NS_ERROR_NOT_INITIALIZED);

  if (mJSObject) {
    return NS_OK;
  }

  NS_ASSERTION(!GetCurrentInnerWindowInternal(),
               "mJSObject is null, but we have an inner window?");

  nsCOMPtr<nsIScriptRuntime> scriptRuntime;
  nsresult rv = NS_GetJSRuntime(getter_AddRefs(scriptRuntime));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsCOMPtr<nsIScriptContext> context =
    scriptRuntime->CreateContext(!IsFrame(), this);

  NS_ASSERTION(!mContext, "Will overwrite mContext!");

  
  context->WillInitializeContext();

  rv = context->InitContext();
  NS_ENSURE_SUCCESS(rv, rv);

  mContext = context;
  return NS_OK;
}

nsIScriptContext *
nsGlobalWindow::GetScriptContext()
{
  FORWARD_TO_OUTER(GetScriptContext, (), nullptr);
  return mContext;
}

nsIScriptContext *
nsGlobalWindow::GetContext()
{
  FORWARD_TO_OUTER(GetContext, (), nullptr);

  
  NS_ASSERTION(mContext == GetScriptContext(),
               "GetContext confused?");
  return mContext;
}

JSObject *
nsGlobalWindow::GetGlobalJSObject()
{
  return FastGetGlobalJSObject();
}

bool
nsGlobalWindow::WouldReuseInnerWindow(nsIDocument *aNewDocument)
{
  
  
  
  
  
  

  if (!mDoc || !aNewDocument) {
    return false;
  }

  if (!mDoc->IsInitialDocument()) {
    return false;
  }
  
  NS_ASSERTION(NS_IsAboutBlank(mDoc->GetDocumentURI()),
               "How'd this happen?");

  
  

  if (mDoc == aNewDocument) {
    return true;
  }

  bool equal;
  if (NS_SUCCEEDED(mDoc->NodePrincipal()->Equals(aNewDocument->NodePrincipal(),
                                                 &equal)) &&
      equal) {
    
    return true;
  }

  return false;
}

void
nsGlobalWindow::SetInitialPrincipalToSubject()
{
  FORWARD_TO_OUTER_VOID(SetInitialPrincipalToSubject, ());

  
  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIPrincipal> newWindowPrincipal, systemPrincipal;
  ssm->GetSubjectPrincipal(getter_AddRefs(newWindowPrincipal));
  ssm->GetSystemPrincipal(getter_AddRefs(systemPrincipal));
  if (!newWindowPrincipal) {
    newWindowPrincipal = systemPrincipal;
  }

  
  
  if (newWindowPrincipal == systemPrincipal) {
    int32_t itemType;
    nsresult rv = GetDocShell()->GetItemType(&itemType);
    if (NS_FAILED(rv) || itemType != nsIDocShellTreeItem::typeChrome) {
      newWindowPrincipal = nullptr;
    }
  }

  
  if (mDoc) {
    
    if (!mDoc->IsInitialDocument())
      return;
    
    if (mDoc->NodePrincipal() == newWindowPrincipal)
      return;

#ifdef DEBUG
    
    
    nsCOMPtr<nsIURI> uri;
    mDoc->NodePrincipal()->GetURI(getter_AddRefs(uri));
    NS_ASSERTION(uri && NS_IsAboutBlank(uri) &&
                 NS_IsAboutBlank(mDoc->GetDocumentURI()),
                 "Unexpected original document");
#endif
  }

  GetDocShell()->CreateAboutBlankContentViewer(newWindowPrincipal);
  mDoc->SetIsInitialDocument(true);

  nsCOMPtr<nsIPresShell> shell = GetDocShell()->GetPresShell();

  if (shell && !shell->DidInitialize()) {
    
    
    nsRect r = shell->GetPresContext()->GetVisibleArea();
    shell->Initialize(r.width, r.height);
  }
}

PopupControlState
PushPopupControlState(PopupControlState aState, bool aForce)
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
                                      bool aForce) const
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

class WindowStateHolder MOZ_FINAL : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(WINDOWSTATEHOLDER_IID)
  NS_DECL_ISUPPORTS

  WindowStateHolder(nsGlobalWindow *aWindow,
                    nsIXPConnectJSObjectHolder *aHolder);

  nsGlobalWindow* GetInnerWindow() { return mInnerWindow; }
  nsIXPConnectJSObjectHolder *GetInnerWindowHolder()
  { return mInnerWindowHolder; }

  void DidRestoreWindow()
  {
    mInnerWindow = nullptr;
    mInnerWindowHolder = nullptr;
  }

protected:
  ~WindowStateHolder();

  nsGlobalWindow *mInnerWindow;
  
  
  nsCOMPtr<nsIXPConnectJSObjectHolder> mInnerWindowHolder;
};

NS_DEFINE_STATIC_IID_ACCESSOR(WindowStateHolder, WINDOWSTATEHOLDER_IID)

WindowStateHolder::WindowStateHolder(nsGlobalWindow *aWindow,
                                     nsIXPConnectJSObjectHolder *aHolder)
  : mInnerWindow(aWindow)
{
  NS_PRECONDITION(aWindow, "null window");
  NS_PRECONDITION(aWindow->IsInnerWindow(), "Saving an outer window");

  mInnerWindowHolder = aHolder;

  aWindow->SuspendTimeouts();
}

WindowStateHolder::~WindowStateHolder()
{
  if (mInnerWindow) {
    
    
    
    
    
    mInnerWindow->FreeInnerObjects();
  }
}

NS_IMPL_ISUPPORTS1(WindowStateHolder, WindowStateHolder)

nsresult
nsGlobalWindow::CreateOuterObject(nsGlobalWindow* aNewInner)
{
  AutoPushJSContext cx(mContext->GetNativeContext());

  JSObject* outer = NewOuterWindowProxy(cx, aNewInner->FastGetGlobalJSObject(),
                                        IsChromeWindow());
  if (!outer) {
    return NS_ERROR_FAILURE;
  }

  js::SetProxyExtra(outer, 0, js::PrivateValue(ToSupports(this)));

  return SetOuterObject(cx, outer);
}

nsresult
nsGlobalWindow::SetOuterObject(JSContext* aCx, JSObject* aOuterObject)
{
  
  
  JS_SetGlobalObject(aCx, aOuterObject);

  
  JSObject* inner = JS_GetParent(aOuterObject);
  JSObject* proto;
  if (!JS_GetPrototype(aCx, inner, &proto)) {
    return NS_ERROR_FAILURE;
  }
  JS_SetPrototype(aCx, aOuterObject, proto);

  return NS_OK;
}






static nsresult
CreateNativeGlobalForInner(JSContext* aCx,
                           nsGlobalWindow* aNewInner,
                           nsIURI* aURI,
                           nsIPrincipal* aPrincipal,
                           JSObject** aNativeGlobal,
                           nsIXPConnectJSObjectHolder** aHolder)
{
  MOZ_ASSERT(aCx);
  MOZ_ASSERT(aNewInner);
  MOZ_ASSERT(aNewInner->IsInnerWindow());
  MOZ_ASSERT(aPrincipal);
  MOZ_ASSERT(aNativeGlobal);
  MOZ_ASSERT(aHolder);

  nsGlobalWindow *top = NULL;
  if (aNewInner->GetOuterWindow()) {
    top = aNewInner->GetTop();
  }
  JS::ZoneSpecifier zoneSpec = JS::FreshZone;
  if (top) {
    if (top->GetGlobalJSObject()) {
      zoneSpec = JS::SameZoneAs(top->GetGlobalJSObject());
    }
  }

  nsIXPConnect* xpc = nsContentUtils::XPConnect();

  nsRefPtr<nsIXPConnectJSObjectHolder> jsholder;
  nsresult rv = xpc->InitClassesWithNewWrappedGlobal(
    aCx, ToSupports(aNewInner),
    aPrincipal, 0, zoneSpec, getter_AddRefs(jsholder));
  NS_ENSURE_SUCCESS(rv, rv);

  MOZ_ASSERT(jsholder);
  jsholder->GetJSObject(aNativeGlobal);
  jsholder.forget(aHolder);

  
  
  MOZ_ASSERT(*aNativeGlobal);
  xpc::SetLocationForGlobal(*aNativeGlobal, aURI);

  return NS_OK;
}

nsresult
nsGlobalWindow::SetNewDocument(nsIDocument* aDocument,
                               nsISupports* aState,
                               bool aForceReuseInnerWindow)
{
  NS_PRECONDITION(mDocumentPrincipal == nullptr,
                  "mDocumentPrincipal prematurely set!");
  MOZ_ASSERT(aDocument);

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

  bool wouldReuseInnerWindow = WouldReuseInnerWindow(aDocument);
  if (aForceReuseInnerWindow &&
      !wouldReuseInnerWindow &&
      mDoc &&
      mDoc->NodePrincipal() != aDocument->NodePrincipal()) {
    NS_ERROR("Attempted forced inner window reuse while changing principal");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIDocument> oldDoc(do_QueryInterface(mDocument));

  nsIScriptContext *scx = GetContextInternal();
  NS_ENSURE_TRUE(scx, NS_ERROR_NOT_INITIALIZED);

  JSContext *cx = scx->GetNativeContext();

#ifndef MOZ_DISABLE_CRYPTOLEGACY
  
  if (mCrypto) {
    nsresult rv = mCrypto->SetEnableSmartCardEvents(false);
    NS_ENSURE_SUCCESS(rv, rv);
  }
#endif

  if (!mDocument) {
    

    
    
    
    nsIDOMWindow* privateRoot = nsGlobalWindow::GetPrivateRoot();

    if (privateRoot == static_cast<nsIDOMWindow*>(this)) {
      nsXBLService::AttachGlobalKeyHandler(mChromeEventHandler);
    }
  }

  



  nsContentUtils::AddScriptRunner(
    NS_NewRunnableMethod(this, &nsGlobalWindow::ClearStatus));

  
  
  bool reUseInnerWindow = (aForceReuseInnerWindow || wouldReuseInnerWindow) &&
                          GetCurrentInnerWindowInternal();

  nsresult rv = NS_OK;

  
  
  
  mDocument = do_QueryInterface(aDocument);
  mDoc = aDocument;

#ifdef DEBUG
  mLastOpenedURI = aDocument->GetDocumentURI();
#endif

  mContext->WillInitializeContext();

  nsGlobalWindow *currentInner = GetCurrentInnerWindowInternal();

  if (currentInner && currentInner->mNavigator) {
    currentInner->mNavigator->OnNavigation();
  }

  nsRefPtr<nsGlobalWindow> newInnerWindow;
  bool createdInnerWindow = false;

  bool thisChrome = IsChromeWindow();

  nsCxPusher cxPusher;
  cxPusher.Push(cx);

  XPCAutoRequest ar(cx);

  nsCOMPtr<WindowStateHolder> wsh = do_QueryInterface(aState);
  NS_ASSERTION(!aState || wsh, "What kind of weird state are you giving me here?");

  if (reUseInnerWindow) {
    
    NS_ASSERTION(!currentInner->IsFrozen(),
                 "We should never be reusing a shared inner window");
    newInnerWindow = currentInner;

    if (aDocument != oldDoc) {
      xpc_UnmarkGrayObject(currentInner->mJSObject);
      if (!nsWindowSH::InvalidateGlobalScopePolluter(cx, currentInner->mJSObject)) {
        return NS_ERROR_FAILURE;
      }
    }

    
    
    
    xpc_UnmarkGrayObject(mJSObject);
    if (!JS_RefreshCrossCompartmentWrappers(cx, mJSObject)) {
      return NS_ERROR_FAILURE;
    }

    
    
    
    
    
    JSCompartment *compartment = js::GetObjectCompartment(currentInner->mJSObject);
#ifdef DEBUG
    bool sameOrigin = false;
    nsIPrincipal *existing =
      nsJSPrincipals::get(JS_GetCompartmentPrincipals(compartment));
    aDocument->NodePrincipal()->Equals(existing, &sameOrigin);
    MOZ_ASSERT(sameOrigin);
#endif
    JS_SetCompartmentPrincipals(compartment,
                                nsJSPrincipals::get(aDocument->NodePrincipal()));
  } else {
    if (aState) {
      newInnerWindow = wsh->GetInnerWindow();
      mInnerWindowHolder = wsh->GetInnerWindowHolder();

      NS_ASSERTION(newInnerWindow, "Got a state without inner window");
    } else if (thisChrome) {
      newInnerWindow = new nsGlobalChromeWindow(this);
    } else if (mIsModalContentWindow) {
      newInnerWindow = new nsGlobalModalWindow(this);
    } else {
      newInnerWindow = new nsGlobalWindow(this);
    }

    if (!aState) {
      
      
      
      
      
      
      
      
      

      mInnerWindow = nullptr;

      Freeze();
      mCreatingInnerWindow = true;
      
      
      rv = CreateNativeGlobalForInner(cx, newInnerWindow,
                                      aDocument->GetDocumentURI(),
                                      aDocument->NodePrincipal(),
                                      &newInnerWindow->mJSObject,
                                      getter_AddRefs(mInnerWindowHolder));
      NS_ASSERTION(NS_SUCCEEDED(rv) && newInnerWindow->mJSObject && mInnerWindowHolder,
                   "Failed to get script global and holder");

      mCreatingInnerWindow = false;
      createdInnerWindow = true;
      Thaw();

      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (currentInner && currentInner->mJSObject) {
      if (oldDoc == aDocument) {
        
        
        
        newInnerWindow->mNavigator = currentInner->mNavigator;
        currentInner->mNavigator = nullptr;
        if (newInnerWindow->mNavigator) {
          newInnerWindow->mNavigator->SetWindow(newInnerWindow);
        }

        
        
        
        currentInner->CreatePerformanceObjectIfNeeded();
        if (currentInner->mPerformance) {
          newInnerWindow->mPerformance =
            new nsPerformance(newInnerWindow,
                              currentInner->mPerformance->GetDOMTiming(),
                              currentInner->mPerformance->GetChannel());
        }
      }

      
      
      if (!currentInner->IsFrozen()) {
        currentInner->FreeInnerObjects();
      }
    }

    mInnerWindow = newInnerWindow;

    if (!mJSObject) {
      CreateOuterObject(newInnerWindow);
      mContext->DidInitializeContext();

      mJSObject = mContext->GetNativeGlobal();
      SetWrapper(mJSObject);
    } else {
      JSObject *outerObject = NewOuterWindowProxy(cx, xpc_UnmarkGrayObject(newInnerWindow->mJSObject),
                                                  thisChrome);
      if (!outerObject) {
        NS_ERROR("out of memory");
        return NS_ERROR_FAILURE;
      }

      js::SetProxyExtra(mJSObject, 0, js::PrivateValue(NULL));

      outerObject = xpc::TransplantObject(cx, mJSObject, outerObject);
      if (!outerObject) {
        NS_ERROR("unable to transplant wrappers, probably OOM");
        return NS_ERROR_FAILURE;
      }

      js::SetProxyExtra(outerObject, 0, js::PrivateValue(ToSupports(this)));

      mJSObject = outerObject;
      SetWrapper(mJSObject);

      {
        JSAutoCompartment ac(cx, mJSObject);

        JS_SetParent(cx, mJSObject, newInnerWindow->mJSObject);

        rv = SetOuterObject(cx, mJSObject);
        NS_ENSURE_SUCCESS(rv, rv);

        NS_ASSERTION(!JS_IsExceptionPending(cx),
                     "We might overwrite a pending exception!");
        XPCWrappedNativeScope* scope = xpc::GetObjectScope(mJSObject);
        if (scope->mWaiverWrapperMap) {
          scope->mWaiverWrapperMap->Reparent(cx, newInnerWindow->mJSObject);
        }
      }
    }

    
    JSAutoCompartment ac(cx, mJSObject);

    
    
    if (createdInnerWindow) {
      nsIXPConnect *xpc = nsContentUtils::XPConnect();
      nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
      nsresult rv = xpc->GetWrappedNativeOfJSObject(cx, newInnerWindow->mJSObject,
                                                    getter_AddRefs(wrapper));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ABORT_IF_FALSE(wrapper, "bad wrapper");
      rv = wrapper->FinishInitForWrappedGlobal();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (!aState) {
      if (!JS_DefineProperty(cx, newInnerWindow->mJSObject, "window",
                             OBJECT_TO_JSVAL(mJSObject),
                             JS_PropertyStub, JS_StrictPropertyStub,
                             JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)) {
        NS_ERROR("can't create the 'window' property");
        return NS_ERROR_FAILURE;
      }
    }
  }

  JSAutoCompartment ac(cx, mJSObject);

  if (!aState && !reUseInnerWindow) {
    
    

    
    
#ifdef DEBUG
    JSObject* newInnerJSObject = newInnerWindow->FastGetGlobalJSObject();
#endif

    
    
    
    
    JS_SetGlobalObject(cx, mJSObject);
#ifdef DEBUG
    JSObject *proto1, *proto2;
    JS_GetPrototype(cx, mJSObject, &proto1);
    JS_GetPrototype(cx, newInnerJSObject, &proto2);
    NS_ASSERTION(proto1 == proto2,
                 "outer and inner globals should have the same prototype");
#endif

    nsCOMPtr<nsIContent> frame = do_QueryInterface(GetFrameElementInternal());
    if (frame) {
      nsPIDOMWindow* parentWindow = frame->OwnerDoc()->GetWindow();
      if (parentWindow && parentWindow->TimeoutSuspendCount()) {
        SuspendTimeouts(parentWindow->TimeoutSuspendCount());
      }
    }
  }

  
  nsCOMPtr<nsIScriptContext> kungFuDeathGrip(mContext);

  
  
  
  
  
  
  
  
  
  

  if ((!reUseInnerWindow || aDocument != oldDoc) && !aState) {
    nsWindowSH::InstallGlobalScopePolluter(cx, newInnerWindow->mJSObject);
  }

  aDocument->SetScriptGlobalObject(newInnerWindow);

  if (!aState) {
    if (reUseInnerWindow) {
      if (newInnerWindow->mDoc != aDocument) {
        newInnerWindow->mDocument = do_QueryInterface(aDocument);
        newInnerWindow->mDoc = aDocument;

        
        
        

        
        ::JS_DeleteProperty(cx, currentInner->mJSObject, "document");
      }
    } else {
      newInnerWindow->InnerSetNewDocument(aDocument);

      
      rv = mContext->InitClasses(newInnerWindow->mJSObject);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (mArguments) {
      newInnerWindow->DefineArgumentsProperty(mArguments);
      newInnerWindow->mArguments = mArguments;
      newInnerWindow->mArgumentsOrigin = mArgumentsOrigin;

      mArguments = nullptr;
      mArgumentsOrigin = nullptr;
    }

    
    
    newInnerWindow->mChromeEventHandler = mChromeEventHandler;
  }

  mContext->GC(JS::gcreason::SET_NEW_DOCUMENT);
  mContext->DidInitializeContext();

  if (newInnerWindow && !newInnerWindow->mHasNotifiedGlobalCreated && mDoc) {
    
    
    
    
    int32_t itemType = nsIDocShellTreeItem::typeContent;
    if (mDocShell) {
      mDocShell->GetItemType(&itemType);
    }

    if (itemType != nsIDocShellTreeItem::typeChrome ||
        nsContentUtils::IsSystemPrincipal(mDoc->NodePrincipal())) {
      newInnerWindow->mHasNotifiedGlobalCreated = true;
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
                                      true ,
                                      false );

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

void
nsGlobalWindow::InnerSetNewDocument(nsIDocument* aDocument)
{
  NS_PRECONDITION(IsInnerWindow(), "Must only be called on inner windows");
  MOZ_ASSERT(aDocument);

#ifdef PR_LOGGING
  if (gDOMLeakPRLog && PR_LOG_TEST(gDOMLeakPRLog, PR_LOG_DEBUG)) {
    nsIURI *uri = aDocument->GetDocumentURI();
    nsAutoCString spec;
    if (uri)
      uri->GetSpec(spec);
    PR_LogPrint("DOMWINDOW %p SetNewDocument %s", this, spec.get());
  }
#endif

  mDocument = do_QueryInterface(aDocument);
  mDoc = aDocument;
  mFocusedNode = nullptr;
  mLocalStorage = nullptr;
  mSessionStorage = nullptr;

#ifdef DEBUG
  mLastOpenedURI = aDocument->GetDocumentURI();
#endif

  Telemetry::Accumulate(Telemetry::INNERWINDOWS_WITH_MUTATION_LISTENERS,
                        mMutationBits ? 1 : 0);

  
  mMutationBits = 0;
}

void
nsGlobalWindow::SetDocShell(nsIDocShell* aDocShell)
{
  NS_ASSERTION(IsOuterWindow(), "Uh, SetDocShell() called on inner window!");
  MOZ_ASSERT(aDocShell);

  if (aDocShell == mDocShell) {
    return;
  }

  mDocShell = aDocShell; 

  NS_ASSERTION(!mNavigator, "Non-null mNavigator in outer window!");

  if (mFrames) {
    mFrames->SetDocShell(aDocShell);
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
    else {
      NS_NewWindowRoot(this, getter_AddRefs(mChromeEventHandler));
    }
  }

  bool docShellActive;
  mDocShell->GetIsActive(&docShellActive);
  mIsBackground = !docShellActive;
}

void
nsGlobalWindow::DetachFromDocShell()
{
  NS_ASSERTION(IsOuterWindow(), "Uh, DetachFromDocShell() called on inner window!");

  
  
  
  
  

  NS_ASSERTION(mTimeouts.isEmpty(), "Uh, outer window holds timeouts!");

  
  
  
  for (nsRefPtr<nsGlobalWindow> inner = (nsGlobalWindow *)PR_LIST_HEAD(this);
       inner != this;
       inner = (nsGlobalWindow*)PR_NEXT_LINK(inner)) {
    NS_ASSERTION(!inner->mOuterWindow || inner->mOuterWindow == this,
                 "bad outer window pointer");
    inner->FreeInnerObjects();
  }

  
  NotifyDOMWindowDestroyed(this);

  NotifyWindowIDDestroyed("outer-window-destroyed");

  nsGlobalWindow *currentInner = GetCurrentInnerWindowInternal();

  if (currentInner) {
    NS_ASSERTION(mDoc, "Must have doc!");
    
    
    mDocumentPrincipal = mDoc->NodePrincipal();
    mDocumentURI = mDoc->GetDocumentURI();
    mDocBaseURI = mDoc->GetDocBaseURI();

    
    mDocument = nullptr;
    mDoc = nullptr;
    mFocusedNode = nullptr;
  }

  ClearControllers();

  mChromeEventHandler = nullptr; 

  if (mArguments) { 
    
    
    mArguments = nullptr;
    mArgumentsLast = nullptr;
    mArgumentsOrigin = nullptr;
  }

  if (mContext) {
    mContext->GC(JS::gcreason::SET_DOC_SHELL);
    mContext = nullptr;
  }

  mDocShell = nullptr; 

  NS_ASSERTION(!mNavigator, "Non-null mNavigator in outer window!");

  if (mFrames) {
    mFrames->SetDocShell(nullptr);
  }

  MaybeForgiveSpamCount();
  CleanUp(false);

    if (mLocalStorage) {
      nsCOMPtr<nsIPrivacyTransitionObserver> obs = do_GetInterface(mLocalStorage);
      if (obs) {
        mDocShell->AddWeakPrivacyTransitionObserver(obs);
      }
    }
    if (mSessionStorage) {
      nsCOMPtr<nsIPrivacyTransitionObserver> obs = do_GetInterface(mSessionStorage);
      if (obs) {
        mDocShell->AddWeakPrivacyTransitionObserver(obs);
      }
    }
}

void
nsGlobalWindow::SetOpenerWindow(nsIDOMWindow* aOpener,
                                bool aOriginalOpener)
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
    mHadOriginalOpener = true;
  }

#ifdef DEBUG
  mSetOpenerWindowCalled = true;
#endif
}

static
already_AddRefed<nsIDOMEventTarget>
TryGetTabChildGlobalAsEventTarget(nsISupports *aFrom)
{
  nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner = do_QueryInterface(aFrom);
  if (!frameLoaderOwner) {
    return NULL;
  }

  nsRefPtr<nsFrameLoader> frameLoader = frameLoaderOwner->GetFrameLoader();
  if (!frameLoader) {
    return NULL;
  }

  nsCOMPtr<nsIDOMEventTarget> eventTarget =
    frameLoader->GetTabChildGlobalAsEventTarget();
  return eventTarget.forget();
}

void
nsGlobalWindow::UpdateParentTarget()
{
  
  
  
  

  nsCOMPtr<nsIDOMElement> frameElement = GetFrameElementInternal();
  nsCOMPtr<nsIDOMEventTarget> eventTarget =
    TryGetTabChildGlobalAsEventTarget(frameElement);

  if (!eventTarget) {
    eventTarget = TryGetTabChildGlobalAsEventTarget(mChromeEventHandler);
  }

  if (!eventTarget) {
    eventTarget = mChromeEventHandler;
  }

  mParentTarget = eventTarget;
}

bool
nsGlobalWindow::GetIsTabModalPromptAllowed()
{
  bool allowTabModal = true;
  if (mDocShell) {
    nsCOMPtr<nsIContentViewer> cv;
    mDocShell->GetContentViewer(getter_AddRefs(cv));
    cv->GetIsTabModalPromptAllowed(&allowTabModal);
  }

  return allowTabModal;
}

EventTarget*
nsGlobalWindow::GetTargetForDOMEvent()
{
  return GetOuterWindowInternal();
}

EventTarget*
nsGlobalWindow::GetTargetForEventTargetChain()
{
  return IsInnerWindow() ? this : GetCurrentInnerWindowInternal();
}

nsresult
nsGlobalWindow::WillHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return NS_OK;
}

JSContext*
nsGlobalWindow::GetJSContextForEventHandlers()
{
  return nullptr;
}

nsresult
nsGlobalWindow::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  NS_PRECONDITION(IsInnerWindow(), "PreHandleEvent is used on outer window!?");
  static uint32_t count = 0;
  uint32_t msg = aVisitor.mEvent->message;

  aVisitor.mCanHandle = true;
  aVisitor.mForceContentDispatch = true; 
  if ((msg == NS_MOUSE_MOVE) && gEntropyCollector) {
    
    
    
    if (count++ % 100 == 0) {
      
      
      int16_t myCoord[2];

      myCoord[0] = aVisitor.mEvent->refPoint.x;
      myCoord[1] = aVisitor.mEvent->refPoint.y;
      gEntropyCollector->RandomUpdate((void*)myCoord, sizeof(myCoord));
      gEntropyCollector->RandomUpdate((void*)&(aVisitor.mEvent->time),
                                      sizeof(uint32_t));
    }
  } else if (msg == NS_RESIZE_EVENT) {
    mIsHandlingResizeEvent = true;
  } else if (msg == NS_MOUSE_BUTTON_DOWN &&
             aVisitor.mEvent->mFlags.mIsTrusted) {
    gMouseDown = true;
  } else if ((msg == NS_MOUSE_BUTTON_UP ||
              msg == NS_DRAGDROP_END) &&
             aVisitor.mEvent->mFlags.mIsTrusted) {
    gMouseDown = false;
    if (gDragServiceDisabled) {
      nsCOMPtr<nsIDragService> ds =
        do_GetService("@mozilla.org/widget/dragservice;1");
      if (ds) {
        gDragServiceDisabled = false;
        ds->Unsuppress();
      }
    }
  }

  aVisitor.mParentTarget = GetParentTarget();

  
  if (!mIdleObservers.IsEmpty() &&
      aVisitor.mEvent->mFlags.mIsTrusted &&
      (NS_IS_MOUSE_EVENT(aVisitor.mEvent) ||
       NS_IS_DRAG_EVENT(aVisitor.mEvent))) {
    mAddActiveEventFuzzTime = false;
  }

  return NS_OK;
}

bool
nsGlobalWindow::DialogsAreBlocked(bool *aBeingAbused)
{
  *aBeingAbused = false;

  nsGlobalWindow *topWindow = GetScriptableTop();
  if (!topWindow) {
    NS_ASSERTION(!mDocShell, "DialogsAreBlocked() called without a top window?");
    return true;
  }

  topWindow = topWindow->GetCurrentInnerWindowInternal();
  if (!topWindow) {
    return true;
  }

  if (topWindow->mDialogsPermanentlyDisabled) {
    return true;
  }

  
  if (mDocShell) {
    nsCOMPtr<nsIContentViewer> cv;
    mDocShell->GetContentViewer(getter_AddRefs(cv));

    bool isHidden;
    cv->GetIsHidden(&isHidden);
    if (isHidden) {
      return true;
    }
  }

  *aBeingAbused = topWindow->DialogsAreBeingAbused();

  return topWindow->mStopAbuseDialogs && *aBeingAbused;
}

bool
nsGlobalWindow::DialogsAreBeingAbused()
{
  NS_ASSERTION(GetScriptableTop() &&
               GetScriptableTop()->GetCurrentInnerWindowInternal() == this,
               "DialogsAreBeingAbused called with invalid window");

  if (mLastDialogQuitTime.IsNull() ||
      nsContentUtils::IsCallerChrome()) {
    return false;
  }

  TimeDuration dialogInterval(TimeStamp::Now() - mLastDialogQuitTime);
  if (dialogInterval.ToSeconds() <
      Preferences::GetInt("dom.successive_dialog_time_limit",
                          DEFAULT_SUCCESSIVE_DIALOG_TIME_LIMIT)) {
    mDialogAbuseCount++;

    return GetPopupControlState() > openAllowed ||
           mDialogAbuseCount > MAX_SUCCESSIVE_DIALOG_COUNT;
  }

  
  mDialogAbuseCount = 0;

  return false;
}

bool
nsGlobalWindow::ConfirmDialogIfNeeded()
{
  FORWARD_TO_OUTER(ConfirmDialogIfNeeded, (), false);

  NS_ENSURE_TRUE(mDocShell, false);
  nsCOMPtr<nsIPromptService> promptSvc =
    do_GetService("@mozilla.org/embedcomp/prompt-service;1");

  if (!promptSvc) {
    return true;
  }

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, true);

  bool disableDialog = false;
  nsXPIDLString label, title;
  nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                     "ScriptDialogLabel", label);
  nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                     "ScriptDialogPreventTitle", title);
  promptSvc->Confirm(this, title.get(), label.get(), &disableDialog);
  if (disableDialog) {
    PreventFurtherDialogs(false);
    return false;
  }

  return true;
}

void
nsGlobalWindow::PreventFurtherDialogs(bool aPermanent)
{
  nsGlobalWindow *topWindow = GetScriptableTop();
  if (!topWindow) {
    NS_ERROR("PreventFurtherDialogs() called without a top window?");
    return;
  }

  topWindow = topWindow->GetCurrentInnerWindowInternal();
  if (topWindow) {
    topWindow->mStopAbuseDialogs = true;
    if (aPermanent) {
      topWindow->mDialogsPermanentlyDisabled = true;
    }
  }
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

  


  nsCOMPtr<nsIDOMEventTarget> kungFuDeathGrip1(mChromeEventHandler);
  nsCOMPtr<nsIScriptContext> kungFuDeathGrip2(GetContextInternal());

  if (aVisitor.mEvent->message == NS_RESIZE_EVENT) {
    mIsHandlingResizeEvent = false;
  } else if (aVisitor.mEvent->message == NS_PAGE_UNLOAD &&
             aVisitor.mEvent->mFlags.mIsTrusted) {
    
    
    if (mDocument) {
      NS_ASSERTION(mDoc, "Must have doc");
      mDoc->BindingManager()->ExecuteDetachedHandlers();
    }
    mIsDocumentLoaded = false;
  } else if (aVisitor.mEvent->message == NS_LOAD &&
             aVisitor.mEvent->mFlags.mIsTrusted) {
    
    
    mIsDocumentLoaded = true;

    nsCOMPtr<nsIContent> content(do_QueryInterface(GetFrameElementInternal()));
    nsIDocShell* docShell = GetDocShell();

    int32_t itemType = nsIDocShellTreeItem::typeChrome;

    if (docShell) {
      docShell->GetItemType(&itemType);
    }

    if (content && GetParentInternal() &&
        itemType != nsIDocShellTreeItem::typeChrome) {
      
      

      nsEventStatus status = nsEventStatus_eIgnore;
      nsEvent event(aVisitor.mEvent->mFlags.mIsTrusted, NS_LOAD);
      event.mFlags.mBubbles = false;

      
      
      
      
      
      nsEventDispatcher::Dispatch(content, nullptr, &event, nullptr, &status);
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
nsGlobalWindow::SetScriptsEnabled(bool aEnabled, bool aFireTimeouts)
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
  nsIScriptContext *ctx = GetOuterWindowInternal()->mContext;
  NS_ENSURE_TRUE(aArguments && ctx, NS_ERROR_NOT_INITIALIZED);
  AutoPushJSContext cx(ctx->GetNativeContext());
  NS_ENSURE_TRUE(cx, NS_ERROR_NOT_INITIALIZED);

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

  return nullptr;
}





nsIURI*
nsPIDOMWindow::GetDocumentURI() const
{
  return mDoc ? mDoc->GetDocumentURI() : mDocumentURI.get();
}

nsIURI*
nsPIDOMWindow::GetDocBaseURI() const
{
  return mDoc ? mDoc->GetDocBaseURI() : mDocBaseURI.get();
}

void
nsPIDOMWindow::MaybeCreateDoc()
{
  MOZ_ASSERT(!mDoc);
  if (nsIDocShell* docShell = GetDocShell()) {
    
    
    
    nsCOMPtr<nsIDocument> document = do_GetInterface(docShell);
  }
}

void
nsPIDOMWindow::AddAudioContext(AudioContext* aAudioContext)
{
  mAudioContexts.AppendElement(aAudioContext);
}

NS_IMETHODIMP
nsGlobalWindow::GetDocument(nsIDOMDocument** aDocument)
{
  nsCOMPtr<nsIDOMDocument> document = do_QueryInterface(GetDoc());
  document.forget(aDocument);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetWindow(nsIDOMWindow** aWindow)
{
  FORWARD_TO_OUTER(GetWindow, (aWindow), NS_ERROR_NOT_INITIALIZED);

  *aWindow = static_cast<nsIDOMWindow*>(this);
  NS_ADDREF(*aWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetSelf(nsIDOMWindow** aWindow)
{
  FORWARD_TO_OUTER(GetSelf, (aWindow), NS_ERROR_NOT_INITIALIZED);

  *aWindow = static_cast<nsIDOMWindow*>(this);
  NS_ADDREF(*aWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetNavigator(nsIDOMNavigator** aNavigator)
{
  FORWARD_TO_INNER(GetNavigator, (aNavigator), NS_ERROR_NOT_INITIALIZED);

  *aNavigator = nullptr;

  if (!mNavigator) {
    mNavigator = new Navigator(this);
  }

  NS_ADDREF(*aNavigator = mNavigator);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetScreen(nsIDOMScreen** aScreen)
{
  FORWARD_TO_INNER(GetScreen, (aScreen), NS_ERROR_NOT_INITIALIZED);

  *aScreen = nullptr;

  if (!mScreen) {
    mScreen = nsScreen::Create(this);
    if (!mScreen) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  NS_IF_ADDREF(*aScreen = mScreen);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetHistory(nsIDOMHistory** aHistory)
{
  FORWARD_TO_INNER(GetHistory, (aHistory), NS_ERROR_NOT_INITIALIZED);

  *aHistory = nullptr;

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
nsGlobalWindow::GetPerformance(nsISupports** aPerformance)
{
  FORWARD_TO_INNER(GetPerformance, (aPerformance), NS_ERROR_NOT_INITIALIZED);

  NS_IF_ADDREF(*aPerformance = nsPIDOMWindow::GetPerformance());
  return NS_OK;
}

nsPerformance*
nsPIDOMWindow::GetPerformance()
{
  MOZ_ASSERT(IsInnerWindow());
  if (HasPerformanceSupport()) {
    CreatePerformanceObjectIfNeeded();
    return mPerformance;
  }
  return nullptr;
}

void
nsPIDOMWindow::CreatePerformanceObjectIfNeeded()
{
  if (mPerformance || !mDoc) {
    return;
  }
  nsRefPtr<nsDOMNavigationTiming> timing = mDoc->GetNavigationTiming();
  nsCOMPtr<nsITimedChannel> timedChannel(do_QueryInterface(mDoc->GetChannel()));
  bool timingEnabled = false;
  if (!timedChannel ||
      !NS_SUCCEEDED(timedChannel->GetTimingEnabled(&timingEnabled)) ||
      !timingEnabled) {
    timedChannel = nullptr;
  }
  if (timing) {
    mPerformance = new nsPerformance(this, timing, timedChannel);
  }
}








NS_IMETHODIMP
nsGlobalWindow::GetScriptableParent(nsIDOMWindow** aParent)
{
  FORWARD_TO_OUTER(GetScriptableParent, (aParent), NS_ERROR_NOT_INITIALIZED);

  *aParent = NULL;
  if (!mDocShell) {
    return NS_OK;
  }

  if (mDocShell->GetIsBrowserOrApp()) {
    nsCOMPtr<nsIDOMWindow> parent = static_cast<nsIDOMWindow*>(this);
    parent.swap(*aParent);
    return NS_OK;
  }

  return GetRealParent(aParent);
}





NS_IMETHODIMP
nsGlobalWindow::GetRealParent(nsIDOMWindow** aParent)
{
  FORWARD_TO_OUTER(GetRealParent, (aParent), NS_ERROR_NOT_INITIALIZED);

  *aParent = nullptr;
  if (!mDocShell) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocShell> parent;
  mDocShell->GetSameTypeParentIgnoreBrowserAndAppBoundaries(getter_AddRefs(parent));

  if (parent) {
    nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(parent));
    NS_ENSURE_SUCCESS(CallQueryInterface(globalObject.get(), aParent),
                      NS_ERROR_FAILURE);
  }
  else {
    *aParent = static_cast<nsIDOMWindow*>(this);
    NS_ADDREF(*aParent);
  }
  return NS_OK;
}








NS_IMETHODIMP
nsGlobalWindow::GetScriptableTop(nsIDOMWindow **aTop)
{
  return GetTopImpl(aTop,  true);
}





NS_IMETHODIMP
nsGlobalWindow::GetRealTop(nsIDOMWindow** aTop)
{
  return GetTopImpl(aTop,  false);
}

nsresult
nsGlobalWindow::GetTopImpl(nsIDOMWindow** aTop, bool aScriptable)
{
  FORWARD_TO_OUTER(GetTopImpl, (aTop, aScriptable), NS_ERROR_NOT_INITIALIZED);
  *aTop = nullptr;

  

  nsCOMPtr<nsIDOMWindow> prevParent = this;
  nsCOMPtr<nsIDOMWindow> parent = this;
  do {
    if (!parent) {
      break;
    }

    prevParent = parent;

    nsCOMPtr<nsIDOMWindow> newParent;
    nsresult rv;
    if (aScriptable) {
      rv = parent->GetScriptableParent(getter_AddRefs(newParent));
    }
    else {
      rv = parent->GetParent(getter_AddRefs(newParent));
    }
    NS_ENSURE_SUCCESS(rv, rv);

    parent = newParent;

  } while (parent != prevParent);

  if (parent) {
    parent.swap(*aTop);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetContent(nsIDOMWindow** aContent)
{
  FORWARD_TO_OUTER(GetContent, (aContent), NS_ERROR_NOT_INITIALIZED);
  *aContent = nullptr;

  
  
  if (mDocShell && mDocShell->GetIsInBrowserOrApp()) {
    return GetScriptableTop(aContent);
  }

  nsCOMPtr<nsIDocShellTreeItem> primaryContent;
  if (!nsContentUtils::IsCallerChrome()) {
    
    
    
    
    nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(mDocShell));

    if (baseWin) {
      bool visible = false;
      baseWin->GetVisibility(&visible);

      if (!visible) {
        mDocShell->GetSameTypeRootTreeItem(getter_AddRefs(primaryContent));
      }
    }
  }

  if (!primaryContent) {
    nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
    GetTreeOwner(getter_AddRefs(treeOwner));
    NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

    treeOwner->GetPrimaryContentShell(getter_AddRefs(primaryContent));
  }

  nsCOMPtr<nsIDOMWindow> domWindow(do_GetInterface(primaryContent));
  domWindow.forget(aContent);

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

  *aMenubar = nullptr;

  if (!mMenubar) {
    mMenubar = new nsMenubarProp(this);
    if (!mMenubar) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aMenubar = mMenubar);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetToolbar(nsIDOMBarProp** aToolbar)
{
  FORWARD_TO_OUTER(GetToolbar, (aToolbar), NS_ERROR_NOT_INITIALIZED);

  *aToolbar = nullptr;

  if (!mToolbar) {
    mToolbar = new nsToolbarProp(this);
    if (!mToolbar) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aToolbar = mToolbar);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetLocationbar(nsIDOMBarProp** aLocationbar)
{
  FORWARD_TO_OUTER(GetLocationbar, (aLocationbar), NS_ERROR_NOT_INITIALIZED);

  *aLocationbar = nullptr;

  if (!mLocationbar) {
    mLocationbar = new nsLocationbarProp(this);
    if (!mLocationbar) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aLocationbar = mLocationbar);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetPersonalbar(nsIDOMBarProp** aPersonalbar)
{
  FORWARD_TO_OUTER(GetPersonalbar, (aPersonalbar), NS_ERROR_NOT_INITIALIZED);

  *aPersonalbar = nullptr;

  if (!mPersonalbar) {
    mPersonalbar = new nsPersonalbarProp(this);
    if (!mPersonalbar) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aPersonalbar = mPersonalbar);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetStatusbar(nsIDOMBarProp** aStatusbar)
{
  FORWARD_TO_OUTER(GetStatusbar, (aStatusbar), NS_ERROR_NOT_INITIALIZED);

  *aStatusbar = nullptr;

  if (!mStatusbar) {
    mStatusbar = new nsStatusbarProp(this);
    if (!mStatusbar) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aStatusbar = mStatusbar);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetScrollbars(nsIDOMBarProp** aScrollbars)
{
  FORWARD_TO_OUTER(GetScrollbars, (aScrollbars), NS_ERROR_NOT_INITIALIZED);

  *aScrollbars = nullptr;

  if (!mScrollbars) {
    mScrollbars = new nsScrollbarsProp(this);
    if (!mScrollbars) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aScrollbars = mScrollbars);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetClosed(bool* aClosed)
{
  FORWARD_TO_OUTER(GetClosed, (aClosed), NS_ERROR_NOT_INITIALIZED);

  
  
  *aClosed = mIsClosed || !mDocShell;

  return NS_OK;
}

nsDOMWindowList*
nsGlobalWindow::GetWindowList()
{
  MOZ_ASSERT(IsOuterWindow());

  if (!mFrames && mDocShell) {
    mFrames = new nsDOMWindowList(mDocShell);
  }

  return mFrames;
}

NS_IMETHODIMP
nsGlobalWindow::GetFrames(nsIDOMWindowCollection** aFrames)
{
  FORWARD_TO_OUTER(GetFrames, (aFrames), NS_ERROR_NOT_INITIALIZED);

  *aFrames = GetWindowList();
  NS_IF_ADDREF(*aFrames);
  return NS_OK;
}

already_AddRefed<nsIDOMWindow>
nsGlobalWindow::IndexedGetter(uint32_t aIndex, bool& aFound)
{
  aFound = false;

  FORWARD_TO_OUTER(IndexedGetter, (aIndex, aFound), nullptr);

  nsDOMWindowList* windows = GetWindowList();
  NS_ENSURE_TRUE(windows, nullptr);

  return windows->IndexedGetter(aIndex, aFound);
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

    nsRefPtr<nsDOMOfflineResourceList> applicationCache =
      new nsDOMOfflineResourceList(manifestURI, uri, this);
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
#ifndef MOZ_DISABLE_CRYPTOLEGACY
    mCrypto = do_CreateInstance(NS_CRYPTO_CONTRACTID);
#else
    mCrypto = new Crypto();
#endif
  }
  NS_IF_ADDREF(*aCrypto = mCrypto);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetPkcs11(nsIDOMPkcs11** aPkcs11)
{
  *aPkcs11 = nullptr;
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
nsGlobalWindow::GetOpener(nsIDOMWindow** aOpener)
{
  FORWARD_TO_OUTER(GetOpener, (aOpener), NS_ERROR_NOT_INITIALIZED);

  *aOpener = nullptr;

  nsCOMPtr<nsPIDOMWindow> opener = do_QueryReferent(mOpener);
  if (!opener) {
    return NS_OK;
  }

  
  if (nsContentUtils::IsCallerChrome()) {
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

  
  
  
  nsCOMPtr<nsIDocShell> openerDocShell = openerPwin->GetDocShell();

  if (openerDocShell) {
    nsCOMPtr<nsIDocShellTreeItem> openerRootItem;
    openerDocShell->GetRootTreeItem(getter_AddRefs(openerRootItem));
    nsCOMPtr<nsIDocShell> openerRootDocShell(do_QueryInterface(openerRootItem));
    if (openerRootDocShell) {
      uint32_t appType;
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
nsGlobalWindow::SetOpener(nsIDOMWindow* aOpener)
{
  
  
  if (aOpener && !nsContentUtils::IsCallerChrome()) {
    return NS_OK;
  }

  SetOpenerWindow(aOpener, false);

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

  if (mDocShell)
    mDocShell->GetName(aName);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SetName(const nsAString& aName)
{
  FORWARD_TO_OUTER(SetName, (aName), NS_ERROR_NOT_INITIALIZED);

  nsresult result = NS_OK;
  if (mDocShell)
    result = mDocShell->SetName(aName);
  return result;
}


int32_t
nsGlobalWindow::DevToCSSIntPixels(int32_t px)
{
  if (!mDocShell)
    return px; 

  nsRefPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext)
    return px;

  return presContext->DevPixelsToIntCSSPixels(px);
}

int32_t
nsGlobalWindow::CSSToDevIntPixels(int32_t px)
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
nsGlobalWindow::GetInnerWidth(int32_t* aInnerWidth)
{
  FORWARD_TO_OUTER(GetInnerWidth, (aInnerWidth), NS_ERROR_NOT_INITIALIZED);

  EnsureSizeUpToDate();

  NS_ENSURE_STATE(mDocShell);

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
nsGlobalWindow::SetInnerWidth(int32_t aInnerWidth)
{
  FORWARD_TO_OUTER(SetInnerWidth, (aInnerWidth), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_STATE(mDocShell);

  



  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(&aInnerWidth, nullptr),
                    NS_ERROR_FAILURE);


  nsRefPtr<nsIPresShell> presShell = mDocShell->GetPresShell();

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
    int32_t height = 0;
    int32_t width  = 0;

    nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(mDocShell));
    docShellAsWin->GetSize(&width, &height);
    width  = CSSToDevIntPixels(aInnerWidth);
    return SetDocShellWidthAndHeight(width, height);
  }
}

NS_IMETHODIMP
nsGlobalWindow::GetInnerHeight(int32_t* aInnerHeight)
{
  FORWARD_TO_OUTER(GetInnerHeight, (aInnerHeight), NS_ERROR_NOT_INITIALIZED);

  EnsureSizeUpToDate();

  NS_ENSURE_STATE(mDocShell);

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
nsGlobalWindow::SetInnerHeight(int32_t aInnerHeight)
{
  FORWARD_TO_OUTER(SetInnerHeight, (aInnerHeight), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_STATE(mDocShell);

  



  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(nullptr, &aInnerHeight),
                    NS_ERROR_FAILURE);

  nsRefPtr<nsIPresShell> presShell = mDocShell->GetPresShell();

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
    int32_t height = 0;
    int32_t width  = 0;

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
nsGlobalWindow::GetOuterWidth(int32_t* aOuterWidth)
{
  FORWARD_TO_OUTER(GetOuterWidth, (aOuterWidth), NS_ERROR_NOT_INITIALIZED);

  nsIntSize sizeCSSPixels;
  nsresult rv = GetOuterSize(&sizeCSSPixels);
  NS_ENSURE_SUCCESS(rv, rv);

  *aOuterWidth = sizeCSSPixels.width;
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetOuterHeight(int32_t* aOuterHeight)
{
  FORWARD_TO_OUTER(GetOuterHeight, (aOuterHeight), NS_ERROR_NOT_INITIALIZED);

  nsIntSize sizeCSSPixels;
  nsresult rv = GetOuterSize(&sizeCSSPixels);
  NS_ENSURE_SUCCESS(rv, rv);

  *aOuterHeight = sizeCSSPixels.height;
  return NS_OK;
}

nsresult
nsGlobalWindow::SetOuterSize(int32_t aLengthCSSPixels, bool aIsWidth)
{
  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(
                        aIsWidth ? &aLengthCSSPixels : nullptr,
                        aIsWidth ? nullptr : &aLengthCSSPixels),
                    NS_ERROR_FAILURE);

  int32_t width, height;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetSize(&width, &height), NS_ERROR_FAILURE);

  int32_t lengthDevPixels = CSSToDevIntPixels(aLengthCSSPixels);
  if (aIsWidth) {
    width = lengthDevPixels;
  } else {
    height = lengthDevPixels;
  }
  return treeOwnerAsWin->SetSize(width, height, true);    
}

NS_IMETHODIMP
nsGlobalWindow::SetOuterWidth(int32_t aOuterWidth)
{
  FORWARD_TO_OUTER(SetOuterWidth, (aOuterWidth), NS_ERROR_NOT_INITIALIZED);

  return SetOuterSize(aOuterWidth, true);
}

NS_IMETHODIMP
nsGlobalWindow::SetOuterHeight(int32_t aOuterHeight)
{
  FORWARD_TO_OUTER(SetOuterHeight, (aOuterHeight), NS_ERROR_NOT_INITIALIZED);

  return SetOuterSize(aOuterHeight, false);
}

NS_IMETHODIMP
nsGlobalWindow::GetScreenX(int32_t* aScreenX)
{
  FORWARD_TO_OUTER(GetScreenX, (aScreenX), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  int32_t x, y;

  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, &y),
                    NS_ERROR_FAILURE);

  *aScreenX = DevToCSSIntPixels(x);
  return NS_OK;
}

nsRect
nsGlobalWindow::GetInnerScreenRect()
{
  if (!mDocShell) {
    return nsRect();
  }

  nsGlobalWindow* rootWindow =
    static_cast<nsGlobalWindow*>(GetPrivateRoot());
  if (rootWindow) {
    rootWindow->FlushPendingNotifications(Flush_Layout);
  }

  if (!mDocShell) {
    return nsRect();
  }

  nsCOMPtr<nsIPresShell> presShell = mDocShell->GetPresShell();
  if (!presShell) {
    return nsRect();
  }
  nsIFrame* rootFrame = presShell->GetRootFrame();
  if (!rootFrame) {
    return nsRect();
  }

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
nsGlobalWindow::GetDevicePixelRatio(float* aRatio)
{
  FORWARD_TO_OUTER(GetDevicePixelRatio, (aRatio), NS_ERROR_NOT_INITIALIZED);

  *aRatio = 1.0;

  if (!mDocShell)
    return NS_OK;

  nsCOMPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext)
    return NS_OK;

  *aRatio = float(nsPresContext::AppUnitsPerCSSPixel())/
      presContext->AppUnitsPerDevPixel();
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetMozPaintCount(uint64_t* aResult)
{
  FORWARD_TO_OUTER(GetMozPaintCount, (aResult), NS_ERROR_NOT_INITIALIZED);

  *aResult = 0;

  if (!mDocShell)
    return NS_OK;

  nsCOMPtr<nsIPresShell> presShell = mDocShell->GetPresShell();
  if (!presShell)
    return NS_OK;

  *aResult = presShell->GetPaintCount();
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::MozRequestAnimationFrame(nsIFrameRequestCallback* aCallback,
                                         int32_t *aHandle)
{
  FORWARD_TO_INNER(MozRequestAnimationFrame, (aCallback, aHandle),
                   NS_ERROR_NOT_INITIALIZED);

  if (!mDoc) {
    return NS_OK;
  }

  if (!aCallback) {
    mDoc->WarnOnceAbout(nsIDocument::eMozBeforePaint);
    return NS_ERROR_XPC_BAD_CONVERT_JS;
  }

  if (mJSObject)
    js::NotifyAnimationActivity(mJSObject);

  return mDoc->ScheduleFrameRequestCallback(aCallback, aHandle);
}

NS_IMETHODIMP
nsGlobalWindow::MozCancelRequestAnimationFrame(int32_t aHandle)
{
  return MozCancelAnimationFrame(aHandle);
}

NS_IMETHODIMP
nsGlobalWindow::MozCancelAnimationFrame(int32_t aHandle)
{
  FORWARD_TO_INNER(MozCancelAnimationFrame, (aHandle),
                   NS_ERROR_NOT_INITIALIZED);

  if (!mDoc) {
    return NS_OK;
  }

  mDoc->CancelFrameRequestCallback(aHandle);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetMozAnimationStartTime(int64_t *aTime)
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

  *aResult = nullptr;

  
  
  
  nsGlobalWindow *parent = static_cast<nsGlobalWindow*>(GetPrivateParent());
  if (parent) {
    parent->FlushPendingNotifications(Flush_Frames);
  }

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
nsGlobalWindow::SetScreenX(int32_t aScreenX)
{
  FORWARD_TO_OUTER(SetScreenX, (aScreenX), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityLeftAndTop(&aScreenX, nullptr),
                    NS_ERROR_FAILURE);

  int32_t x, y;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, &y),
                    NS_ERROR_FAILURE);

  x = CSSToDevIntPixels(aScreenX);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetPosition(x, y),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetScreenY(int32_t* aScreenY)
{
  FORWARD_TO_OUTER(GetScreenY, (aScreenY), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  int32_t x, y;

  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, &y),
                    NS_ERROR_FAILURE);

  *aScreenY = DevToCSSIntPixels(y);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::SetScreenY(int32_t aScreenY)
{
  FORWARD_TO_OUTER(SetScreenY, (aScreenY), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityLeftAndTop(nullptr, &aScreenY),
                    NS_ERROR_FAILURE);

  int32_t x, y;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, &y),
                    NS_ERROR_FAILURE);

  y = CSSToDevIntPixels(aScreenY);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetPosition(x, y),
                    NS_ERROR_FAILURE);

  return NS_OK;
}



nsresult
nsGlobalWindow::CheckSecurityWidthAndHeight(int32_t* aWidth, int32_t* aHeight)
{
#ifdef MOZ_XUL
  if (!nsContentUtils::IsCallerChrome()) {
    
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
    nsContentUtils::HidePopupsInDocument(doc);
  }
#endif

  
  if ((aWidth && *aWidth < 100) || (aHeight && *aHeight < 100)) {
    

    if (!nsContentUtils::IsCallerChrome()) {
      
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
nsGlobalWindow::SetDocShellWidthAndHeight(int32_t aInnerWidth, int32_t aInnerHeight)
{
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  mDocShell->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(treeOwner->SizeShellTo(mDocShell, aInnerWidth, aInnerHeight),
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
nsGlobalWindow::CheckSecurityLeftAndTop(int32_t* aLeft, int32_t* aTop)
{
  

  

  if (!nsContentUtils::IsCallerChrome()) {
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
      int32_t screenLeft, screenTop, screenWidth, screenHeight;
      int32_t winLeft, winTop, winWidth, winHeight;

      
      treeOwner->GetPositionAndSize(&winLeft, &winTop, &winWidth, &winHeight);

      
      
      winLeft   = DevToCSSIntPixels(winLeft);
      winTop    = DevToCSSIntPixels(winTop);
      winWidth  = DevToCSSIntPixels(winWidth);
      winHeight = DevToCSSIntPixels(winHeight);

      
      
      screen->GetAvailLeft(&screenLeft);
      screen->GetAvailWidth(&screenWidth);
      screen->GetAvailHeight(&screenHeight);
#if defined(XP_MACOSX)
      






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
nsGlobalWindow::GetPageXOffset(int32_t* aPageXOffset)
{
  return GetScrollX(aPageXOffset);
}

NS_IMETHODIMP
nsGlobalWindow::GetPageYOffset(int32_t* aPageYOffset)
{
  return GetScrollY(aPageYOffset);
}

nsresult
nsGlobalWindow::GetScrollMaxXY(int32_t* aScrollMaxX, int32_t* aScrollMaxY)
{
  FORWARD_TO_OUTER(GetScrollMaxXY, (aScrollMaxX, aScrollMaxY),
                   NS_ERROR_NOT_INITIALIZED);

  FlushPendingNotifications(Flush_Layout);
  nsIScrollableFrame *sf = GetScrollFrame();
  if (!sf)
    return NS_OK;

  nsRect scrollRange = sf->GetScrollRange();

  if (aScrollMaxX)
    *aScrollMaxX = std::max(0,
      (int32_t)floor(nsPresContext::AppUnitsToFloatCSSPixels(scrollRange.XMost())));
  if (aScrollMaxY)
    *aScrollMaxY = std::max(0,
      (int32_t)floor(nsPresContext::AppUnitsToFloatCSSPixels(scrollRange.YMost())));

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetScrollMaxX(int32_t* aScrollMaxX)
{
  NS_ENSURE_ARG_POINTER(aScrollMaxX);
  *aScrollMaxX = 0;
  return GetScrollMaxXY(aScrollMaxX, nullptr);
}

NS_IMETHODIMP
nsGlobalWindow::GetScrollMaxY(int32_t* aScrollMaxY)
{
  NS_ENSURE_ARG_POINTER(aScrollMaxY);
  *aScrollMaxY = 0;
  return GetScrollMaxXY(nullptr, aScrollMaxY);
}

nsresult
nsGlobalWindow::GetScrollXY(int32_t* aScrollX, int32_t* aScrollY,
                            bool aDoFlush)
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
    
    
    
    return GetScrollXY(aScrollX, aScrollY, true);
  }

  nsIntPoint scrollPosCSSPixels = sf->GetScrollPositionCSSPixels();
  if (aScrollX) {
    *aScrollX = scrollPosCSSPixels.x;
  }
  if (aScrollY) {
    *aScrollY = scrollPosCSSPixels.y;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetScrollX(int32_t* aScrollX)
{
  NS_ENSURE_ARG_POINTER(aScrollX);
  *aScrollX = 0;
  return GetScrollXY(aScrollX, nullptr, false);
}

NS_IMETHODIMP
nsGlobalWindow::GetScrollY(int32_t* aScrollY)
{
  NS_ENSURE_ARG_POINTER(aScrollY);
  *aScrollY = 0;
  return GetScrollXY(nullptr, aScrollY, false);
}

uint32_t
nsGlobalWindow::GetLength()
{
  FORWARD_TO_OUTER(GetLength, (), 0);

  nsDOMWindowList* windows = GetWindowList();
  NS_ENSURE_TRUE(windows, 0);

  return windows->GetLength();
}

NS_IMETHODIMP
nsGlobalWindow::GetLength(uint32_t* aLength)
{
  *aLength = GetLength();
  return NS_OK;
}

already_AddRefed<nsIDOMWindow>
nsGlobalWindow::GetChildWindow(jsid aName)
{
  const jschar *chars = JS_GetInternedStringChars(JSID_TO_STRING(aName));

  nsCOMPtr<nsIDocShellTreeNode> dsn(do_QueryInterface(GetDocShell()));
  NS_ENSURE_TRUE(dsn, nullptr);

  nsCOMPtr<nsIDocShellTreeItem> child;
  dsn->FindChildWithName(reinterpret_cast<const PRUnichar*>(chars),
                         false, true, nullptr, nullptr,
                         getter_AddRefs(child));

  nsCOMPtr<nsIDOMWindow> child_win(do_GetInterface(child));
  return child_win.forget();
}

bool
nsGlobalWindow::DispatchCustomEvent(const char *aEventName)
{
  bool defaultActionEnabled = true;
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
  nsContentUtils::DispatchTrustedEvent(doc,
                                       GetOuterWindow(),
                                       NS_ConvertASCIItoUTF16(aEventName),
                                       true, true, &defaultActionEnabled);

  return defaultActionEnabled;
}

void
nsGlobalWindow::RefreshCompartmentPrincipal()
{
  FORWARD_TO_INNER(RefreshCompartmentPrincipal, (),  );

  JS_SetCompartmentPrincipals(js::GetObjectCompartment(mJSObject),
                              nsJSPrincipals::get(mDoc->NodePrincipal()));
}

static already_AddRefed<nsIDocShellTreeItem>
GetCallerDocShellTreeItem()
{
  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  nsIDocShellTreeItem *callerItem = nullptr;

  if (cx) {
    nsCOMPtr<nsIWebNavigation> callerWebNav =
      do_GetInterface(nsJSUtils::GetDynamicScriptGlobal(cx));

    if (callerWebNav) {
      CallQueryInterface(callerWebNav, &callerItem);
    }
  }

  return callerItem;
}

bool
nsGlobalWindow::WindowExists(const nsAString& aName,
                             bool aLookForCallerOnJSStack)
{
  NS_PRECONDITION(IsOuterWindow(), "Must be outer window");
  NS_PRECONDITION(mDocShell, "Must have docshell");

  nsCOMPtr<nsIDocShellTreeItem> caller;
  if (aLookForCallerOnJSStack) {
    caller = GetCallerDocShellTreeItem();
  }

  if (!caller) {
    caller = mDocShell;
  }

  nsCOMPtr<nsIDocShellTreeItem> namedItem;
  mDocShell->FindItemWithName(PromiseFlatString(aName).get(), nullptr, caller,
                              getter_AddRefs(namedItem));
  return namedItem != nullptr;
}

already_AddRefed<nsIWidget>
nsGlobalWindow::GetMainWidget()
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));

  nsIWidget *widget = nullptr;

  if (treeOwnerAsWin) {
    treeOwnerAsWin->GetMainWidget(&widget);
  }

  return widget;
}

nsIWidget*
nsGlobalWindow::GetNearestWidget()
{
  nsIDocShell* docShell = GetDocShell();
  NS_ENSURE_TRUE(docShell, nullptr);
  nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();
  NS_ENSURE_TRUE(presShell, nullptr);
  nsIFrame* rootFrame = presShell->GetRootFrame();
  NS_ENSURE_TRUE(rootFrame, nullptr);
  return rootFrame->GetView()->GetNearestWidget(nullptr);
}

NS_IMETHODIMP
nsGlobalWindow::SetFullScreen(bool aFullScreen)
{
  return SetFullScreenInternal(aFullScreen, true);
}

nsresult
nsGlobalWindow::SetFullScreenInternal(bool aFullScreen, bool aRequireTrust)
{
  FORWARD_TO_OUTER(SetFullScreen, (aFullScreen), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  bool rootWinFullScreen;
  GetFullScreen(&rootWinFullScreen);
  
  
  if (aFullScreen == rootWinFullScreen ||
      (aRequireTrust && !nsContentUtils::IsCallerChrome())) {
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIDocShellTreeItem> rootItem;
  mDocShell->GetRootTreeItem(getter_AddRefs(rootItem));
  nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(rootItem);
  if (!window)
    return NS_ERROR_FAILURE;
  if (rootItem != mDocShell)
    return window->SetFullScreenInternal(aFullScreen, aRequireTrust);

  
  
  int32_t itemType;
  mDocShell->GetItemType(&itemType);
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
    xulWin->SetIntrinsicallySized(false);
  }

  
  
  mFullScreen = aFullScreen;

  
  
  
  
  if (!Preferences::GetBool("full-screen-api.ignore-widgets", false)) {
    nsCOMPtr<nsIWidget> widget = GetMainWidget();
    if (widget)
      widget->MakeFullScreen(aFullScreen);
  }

  if (!mFullScreen) {
    
    
    
    
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
    nsIDocument::ExitFullscreen(doc,  false);
  }

  if (!mWakeLock && mFullScreen) {
    nsCOMPtr<nsIPowerManagerService> pmService =
      do_GetService(POWERMANAGERSERVICE_CONTRACTID);
    NS_ENSURE_TRUE(pmService, NS_OK);

    pmService->NewWakeLock(NS_LITERAL_STRING("DOM_Fullscreen"), this, getter_AddRefs(mWakeLock));
  } else if (mWakeLock && !mFullScreen) {
    mWakeLock->Unlock();
    mWakeLock = NULL;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetFullScreen(bool* aFullScreen)
{
  FORWARD_TO_OUTER(GetFullScreen, (aFullScreen), NS_ERROR_NOT_INITIALIZED);

  
  
  if (mDocShell) {
    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    mDocShell->GetRootTreeItem(getter_AddRefs(rootItem));
    if (rootItem != mDocShell) {
      nsCOMPtr<nsIDOMWindow> window = do_GetInterface(rootItem);
      if (window)
        return window->GetFullScreen(aFullScreen);
    }
  }

  
  *aFullScreen = mFullScreen;
  return NS_OK;
}

bool
nsGlobalWindow::DOMWindowDumpEnabled()
{
#if !(defined(DEBUG) || defined(MOZ_ENABLE_JS_DUMP))
  
  
  
  return gDOMWindowDumpEnabled;
#else
  return true;
#endif
}

NS_IMETHODIMP
nsGlobalWindow::Dump(const nsAString& aStr)
{
  if (!DOMWindowDumpEnabled()) {
    return NS_OK;
  }

  char *cstr = ToNewUTF8String(aStr);

#if defined(XP_MACOSX)
  
  char *c = cstr, *cEnd = cstr + strlen(cstr);
  while (c < cEnd) {
    if (*c == '\r')
      *c = '\n';
    c++;
  }
#endif

  if (cstr) {
#ifdef XP_WIN
    if (IsDebuggerPresent()) {
      OutputDebugStringA(cstr);
    }
#endif
#ifdef ANDROID
    __android_log_write(ANDROID_LOG_INFO, "GeckoDump", cstr);
#endif
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

  nsCOMPtr<nsIPresShell> presShell = mDocShell->GetPresShell();

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
            nsAutoCString host;
            fixedURI->GetHost(host);

            if (!host.IsEmpty()) {
              
              
              

              nsAutoCString prepath;
              fixedURI->GetPrePath(prepath);

              NS_ConvertUTF8toUTF16 ucsPrePath(prepath);
              const PRUnichar *formatStrings[] = { ucsPrePath.get() };
              nsXPIDLString tempString;
              nsContentUtils::FormatLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                                    "ScriptDlgHeading",
                                                    formatStrings,
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

bool
nsGlobalWindow::CanMoveResizeWindows()
{
  
  if (!nsContentUtils::IsCallerChrome()) {
    
    
    if (!mHadOriginalOpener) {
      return false;
    }

    if (!CanSetProperty("dom.disable_window_move_resize")) {
      return false;
    }

    
    nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
    GetTreeOwner(getter_AddRefs(treeOwner));
    if (treeOwner) {
      uint32_t itemCount;
      if (NS_SUCCEEDED(treeOwner->GetTargetableShellCount(&itemCount)) &&
          itemCount > 1) {
        return false;
      }
    }
  }

  if (mDocShell) {
    bool allow;
    nsresult rv = mDocShell->GetAllowWindowControl(&allow);
    if (NS_SUCCEEDED(rv) && !allow)
      return false;
  }

  if (gMouseDown && !gDragServiceDisabled) {
    nsCOMPtr<nsIDragService> ds =
      do_GetService("@mozilla.org/widget/dragservice;1");
    if (ds) {
      gDragServiceDisabled = true;
      ds->Suppress();
    }
  }
  return true;
}

NS_IMETHODIMP
nsGlobalWindow::Alert(const nsAString& aString)
{
  FORWARD_TO_OUTER(Alert, (aString), NS_ERROR_NOT_INITIALIZED);

  bool needToPromptForAbuse;
  if (DialogsAreBlocked(&needToPromptForAbuse)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, true);

  
  

  NS_NAMED_LITERAL_STRING(null_str, "null");

  const nsAString *str = DOMStringIsNull(aString) ? &null_str : &aString;

  
  
  EnsureReflowFlushAndPaint();

  nsAutoString title;
  MakeScriptDialogTitle(title);

  
  
  nsAutoString final;
  nsContentUtils::StripNullChars(*str, final);

  
  
  bool allowTabModal = GetIsTabModalPromptAllowed();

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

  nsAutoSyncOperation sync(GetCurrentInnerWindowInternal() ? 
                             GetCurrentInnerWindowInternal()->mDoc :
                             nullptr);
  if (needToPromptForAbuse) {
    bool disallowDialog = false;
    nsXPIDLString label;
    nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                       "ScriptDialogLabel", label);

    rv = prompt->AlertCheck(title.get(), final.get(), label.get(),
                            &disallowDialog);
    if (disallowDialog)
      PreventFurtherDialogs(false);
  } else {
    rv = prompt->Alert(title.get(), final.get());
  }

  return rv;
}

NS_IMETHODIMP
nsGlobalWindow::Confirm(const nsAString& aString, bool* aReturn)
{
  FORWARD_TO_OUTER(Confirm, (aString, aReturn), NS_ERROR_NOT_INITIALIZED);

  bool needToPromptForAbuse;
  if (DialogsAreBlocked(&needToPromptForAbuse)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, true);

  *aReturn = false;

  
  
  EnsureReflowFlushAndPaint();

  nsAutoString title;
  MakeScriptDialogTitle(title);

  
  
  nsAutoString final;
  nsContentUtils::StripNullChars(aString, final);

  
  
  bool allowTabModal = GetIsTabModalPromptAllowed();

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

  nsAutoSyncOperation sync(GetCurrentInnerWindowInternal() ? 
                             GetCurrentInnerWindowInternal()->mDoc :
                             nullptr);
  if (needToPromptForAbuse) {
    bool disallowDialog = false;
    nsXPIDLString label;
    nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                       "ScriptDialogLabel", label);

    rv = prompt->ConfirmCheck(title.get(), final.get(), label.get(),
                              &disallowDialog, aReturn);
    if (disallowDialog)
      PreventFurtherDialogs(false);
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

  bool needToPromptForAbuse;
  if (DialogsAreBlocked(&needToPromptForAbuse)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  nsAutoPopupStatePusher popupStatePusher(openAbused, true);

  
  
  EnsureReflowFlushAndPaint();

  nsAutoString title;
  MakeScriptDialogTitle(title);
  
  
  
  nsAutoString fixedMessage, fixedInitial;
  nsContentUtils::StripNullChars(aMessage, fixedMessage);
  nsContentUtils::StripNullChars(aInitial, fixedInitial);

  
  
  bool allowTabModal = GetIsTabModalPromptAllowed();

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
  bool disallowDialog = false;

  nsXPIDLString label;
  if (needToPromptForAbuse) {
    nsContentUtils::GetLocalizedString(nsContentUtils::eCOMMON_DIALOG_PROPERTIES,
                                       "ScriptDialogLabel", label);
  }

  nsAutoSyncOperation sync(GetCurrentInnerWindowInternal() ? 
                             GetCurrentInnerWindowInternal()->mDoc :
                             nullptr);
  bool ok;
  rv = prompt->Prompt(title.get(), fixedMessage.get(),
                      &inoutValue, label.get(), &disallowDialog, &ok);

  if (disallowDialog) {
    PreventFurtherDialogs(false);
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

  bool isVisible = false;
  if (baseWin) {
    baseWin->GetVisibility(&isVisible);
  }

  if (!isVisible) {
    
    return NS_OK;
  }

  nsIDOMWindow *caller = nsContentUtils::GetWindowFromCaller();
  nsCOMPtr<nsIDOMWindow> opener;
  GetOpener(getter_AddRefs(opener));

  
  
  
  bool canFocus = CanSetProperty("dom.disable_window_flip") ||
                    (opener == caller &&
                     RevisePopupAbuseLevel(gPopupControlState) < openAbused);

  nsCOMPtr<nsIDOMWindow> activeWindow;
  fm->GetActiveWindow(getter_AddRefs(activeWindow));

  nsCOMPtr<nsIDocShellTreeItem> rootItem;
  mDocShell->GetRootTreeItem(getter_AddRefs(rootItem));
  nsCOMPtr<nsIDOMWindow> rootWin = do_GetInterface(rootItem);
  bool isActive = (rootWin == activeWindow);

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  if (treeOwnerAsWin && (canFocus || isActive)) {
    bool isEnabled = true;
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
  
  
  
  bool lookForPresShell = true;
  int32_t itemType = nsIDocShellTreeItem::typeContent;
  mDocShell->GetItemType(&itemType);
  if (itemType == nsIDocShellTreeItem::typeChrome &&
      GetPrivateRoot() == static_cast<nsIDOMWindow*>(this) &&
      mDocument) {
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
    NS_ASSERTION(doc, "Bogus doc?");
    nsIURI* ourURI = doc->GetDocumentURI();
    if (ourURI) {
      lookForPresShell = !NS_IsAboutBlank(ourURI);
    }
  }

  if (lookForPresShell) {
    mDocShell->GetEldestPresShell(getter_AddRefs(presShell));
  }

  nsCOMPtr<nsIDocShellTreeItem> parentDsti;
  mDocShell->GetParent(getter_AddRefs(parentDsti));

  
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
      uint32_t flags = nsIFocusManager::FLAG_NOSCROLL;
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
  nsCOMPtr<nsIEmbeddingSiteWindow> siteWindow(do_GetInterface(treeOwner));
  if (siteWindow) {
    
    rv = siteWindow->Blur();

    
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(mDocument);
    if (fm && mDocument) {
      nsCOMPtr<nsIDOMElement> element;
      fm->GetFocusedElementForWindow(this, false, nullptr, getter_AddRefs(element));
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
    Preferences::GetLocalizedString(PREF_BROWSER_STARTUP_HOMEPAGE);

  if (homeURL.IsEmpty()) {
    
#ifdef DEBUG_seth
    printf("all else failed.  using %s as the home page\n", DEFAULT_HOME_PAGE);
#endif
    CopyASCIItoUTF16(DEFAULT_HOME_PAGE, homeURL);
  }

#ifdef MOZ_PHOENIX
  {
    
    
    
    
    
    
    
    
    
    int32_t firstPipe = homeURL.FindChar('|');

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
                       nullptr,
                       nullptr,
                       nullptr);
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

  if (Preferences::GetBool("dom.disable_window_print", false))
    return NS_ERROR_NOT_AVAILABLE;

  bool needToPromptForAbuse;
  if (DialogsAreBlocked(&needToPromptForAbuse)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (needToPromptForAbuse && !ConfirmDialogIfNeeded()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIWebBrowserPrint> webBrowserPrint;
  if (NS_SUCCEEDED(GetInterface(NS_GET_IID(nsIWebBrowserPrint),
                                getter_AddRefs(webBrowserPrint)))) {
    nsAutoSyncOperation sync(GetCurrentInnerWindowInternal() ? 
                               GetCurrentInnerWindowInternal()->mDoc :
                               nullptr);

    nsCOMPtr<nsIPrintSettingsService> printSettingsService = 
      do_GetService("@mozilla.org/gfx/printsettings-service;1");

    nsCOMPtr<nsIPrintSettings> printSettings;
    if (printSettingsService) {
      bool printSettingsAreGlobal =
        Preferences::GetBool("print.use_global_printsettings", false);

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
                                                         true, 
                                                         nsIPrintSettings::kInitSaveAll);
      } else {
        printSettingsService->GetNewPrintSettings(getter_AddRefs(printSettings));
      }

      nsCOMPtr<nsIDOMWindow> callerWin = EnterModalState();
      webBrowserPrint->Print(printSettings, nullptr);
      LeaveModalState(callerWin);

      bool savePrintSettings =
        Preferences::GetBool("print.save_print_settings", false);
      if (printSettingsAreGlobal && savePrintSettings) {
        printSettingsService->
          SavePrintSettingsToPrefs(printSettings,
                                   true,
                                   nsIPrintSettings::kInitSaveAll);
        printSettingsService->
          SavePrintSettingsToPrefs(printSettings,
                                   false,
                                   nsIPrintSettings::kInitSavePrinterName);
      }
    } else {
      webBrowserPrint->GetGlobalPrintSettings(getter_AddRefs(printSettings));
      webBrowserPrint->Print(printSettings, nullptr);
    }
  }
#endif 

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::MoveTo(int32_t aXPos, int32_t aYPos)
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
nsGlobalWindow::MoveBy(int32_t aXDif, int32_t aYDif)
{
  FORWARD_TO_OUTER(MoveBy, (aXDif, aYDif), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  
  
  

  int32_t x, y;
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
nsGlobalWindow::ResizeTo(int32_t aWidth, int32_t aHeight)
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

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetSize(devSz.width, devSz.height, true),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::ResizeBy(int32_t aWidthDif, int32_t aHeightDif)
{
  FORWARD_TO_OUTER(ResizeBy, (aWidthDif, aHeightDif), NS_ERROR_NOT_INITIALIZED);

  




  if (!CanMoveResizeWindows() || IsFrame()) {
    return NS_OK;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  int32_t width, height;
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
                                            true),
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

  int32_t width, height;
  NS_ENSURE_SUCCESS(markupViewer->GetContentSize(&width, &height),
                    NS_ERROR_FAILURE);

  
  
  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

  nsIntSize cssSize(DevToCSSIntPixels(nsIntSize(width, height)));
  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(&cssSize.width,
                                                &cssSize.height),
                    NS_ERROR_FAILURE);

  nsIntSize newDevSize(CSSToDevIntPixels(cssSize));

  NS_ENSURE_SUCCESS(treeOwner->SizeShellTo(mDocShell,
                                           newDevSize.width, newDevSize.height),
                    NS_ERROR_FAILURE);

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
  nsIDOMWindow *rootWindow = GetPrivateRoot();
  nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(rootWindow));
  if (!piWin)
    return nullptr;

  nsCOMPtr<nsPIWindowRoot> window = do_QueryInterface(piWin->GetChromeEventHandler());
  return window.forget();
}

NS_IMETHODIMP
nsGlobalWindow::Scroll(int32_t aXScroll, int32_t aYScroll)
{
  return ScrollTo(aXScroll, aYScroll);
}

NS_IMETHODIMP
nsGlobalWindow::ScrollTo(int32_t aXScroll, int32_t aYScroll)
{
  FlushPendingNotifications(Flush_Layout);
  nsIScrollableFrame *sf = GetScrollFrame();

  if (sf) {
    
    
    
    
    
    const int32_t maxpx = nsPresContext::AppUnitsToIntCSSPixels(0x7fffffff) - 4;

    if (aXScroll > maxpx) {
      aXScroll = maxpx;
    }

    if (aYScroll > maxpx) {
      aYScroll = maxpx;
    }
    sf->ScrollToCSSPixels(nsIntPoint(aXScroll, aYScroll));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::ScrollBy(int32_t aXScrollDif, int32_t aYScrollDif)
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
nsGlobalWindow::ScrollByLines(int32_t numLines)
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
nsGlobalWindow::ScrollByPages(int32_t numPages)
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
nsGlobalWindow::ClearTimeout(int32_t aHandle)
{
  if (aHandle <= 0) {
    return NS_OK;
  }

  return ClearTimeoutOrInterval(aHandle);
}

NS_IMETHODIMP
nsGlobalWindow::ClearInterval(int32_t aHandle)
{
  if (aHandle <= 0) {
    return NS_OK;
  }

  return ClearTimeoutOrInterval(aHandle);
}

NS_IMETHODIMP
nsGlobalWindow::SetTimeout(int32_t *_retval)
{
  return SetTimeoutOrInterval(false, _retval);
}

NS_IMETHODIMP
nsGlobalWindow::SetInterval(int32_t *_retval)
{
  return SetTimeoutOrInterval(true, _retval);
}

NS_IMETHODIMP
nsGlobalWindow::SetResizable(bool aResizable)
{
  

  return NS_OK;
}

static void
ReportUseOfDeprecatedMethod(nsGlobalWindow* aWindow, const char* aWarning)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aWindow->GetExtantDocument());
  nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                  "DOM Events", doc,
                                  nsContentUtils::eDOM_PROPERTIES,
                                  aWarning);
}

NS_IMETHODIMP
nsGlobalWindow::CaptureEvents(int32_t aEventFlags)
{
  ReportUseOfDeprecatedMethod(this, "UseOfCaptureEventsWarning");
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::ReleaseEvents(int32_t aEventFlags)
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
bool IsPopupBlocked(nsIDOMDocument* aDoc)
{
  nsCOMPtr<nsIPopupWindowManager> pm =
    do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);

  if (!pm) {
    return false;
  }

  bool blocked = true;
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(aDoc));

  if (doc) {
    uint32_t permission = nsIPopupWindowManager::ALLOW_POPUP;
    pm->TestPermission(doc->NodePrincipal(), &permission);
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
    
    
    nsCOMPtr<nsIDOMEvent> event;
    aDoc->CreateEvent(NS_LITERAL_STRING("PopupBlockedEvents"),
                      getter_AddRefs(event));
    if (event) {
      nsCOMPtr<nsIDOMPopupBlockedEvent> pbev(do_QueryInterface(event));
      pbev->InitPopupBlockedEvent(NS_LITERAL_STRING("DOMPopupBlocked"),
                                  true, true, aRequestingWindow,
                                  aPopupURI, aPopupWindowName,
                                  aPopupWindowFeatures);
      event->SetTrusted(true);

      nsCOMPtr<nsIDOMEventTarget> targ(do_QueryInterface(aDoc));
      bool defaultActionEnabled;
      targ->DispatchEvent(event, &defaultActionEnabled);
    }
  }
}

void FirePopupWindowEvent(nsIDOMDocument* aDoc)
{
  
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(aDoc));
  nsContentUtils::DispatchTrustedEvent(doc, aDoc,
                                       NS_LITERAL_STRING("PopupWindow"),
                                       true, true);
}


bool
nsGlobalWindow::CanSetProperty(const char *aPrefName)
{
  
  if (nsContentUtils::IsCallerChrome()) {
    return true;
  }

  
  
  return !Preferences::GetBool(aPrefName, true);
}

bool
nsGlobalWindow::PopupWhitelisted()
{
  if (!IsPopupBlocked(mDocument))
    return true;

  nsCOMPtr<nsIDOMWindow> parent;

  if (NS_FAILED(GetParent(getter_AddRefs(parent))) ||
      parent == static_cast<nsIDOMWindow*>(this))
  {
    return false;
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
  
  int32_t type = nsIDocShellTreeItem::typeChrome;
  mDocShell->GetItemType(&type);
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
    int32_t popupMax = Preferences::GetInt("dom.popup_maximum", -1);
    if (popupMax >= 0 && gOpenPopupSpamCount >= popupMax)
      abuse = openOverridden;
  }

  return abuse;
}





void
nsGlobalWindow::FireAbuseEvents(bool aBlocked, bool aWindow,
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
                      false,          
                      false,          
                      true,           
                      false,          
                      true,           
                      nullptr, nullptr,  
                      GetPrincipal(),    
                      nullptr,           
                      _retval);
}

NS_IMETHODIMP
nsGlobalWindow::OpenJS(const nsAString& aUrl, const nsAString& aName,
                       const nsAString& aOptions, nsIDOMWindow **_retval)
{
  return OpenInternal(aUrl, aName, aOptions,
                      false,          
                      false,          
                      false,          
                      true,           
                      true,           
                      nullptr, nullptr,  
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
                      true,                    
                      false,                   
                      true,                    
                      false,                   
                      true,                    
                      nullptr, aExtraArgument,    
                      GetPrincipal(),             
                      nullptr,                    
                      _retval);
}


 nsresult
nsGlobalWindow::OpenNoNavigate(const nsAString& aUrl,
                               const nsAString& aName,
                               const nsAString& aOptions,
                               nsIDOMWindow **_retval)
{
  return OpenInternal(aUrl, aName, aOptions,
                      false,          
                      false,          
                      true,           
                      false,          
                      false,          
                      nullptr, nullptr,  
                      GetPrincipal(),    
                      nullptr,           
                      _retval);

}

NS_IMETHODIMP
nsGlobalWindow::OpenDialog(const nsAString& aUrl, const nsAString& aName,
                           const nsAString& aOptions, nsIDOMWindow** _retval)
{
  if (!nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsAXPCNativeCallContext *ncc = nullptr;
  nsresult rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(&ncc);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  JSContext *cx = nullptr;

  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t argc;
  jsval *argv = nullptr;

  
  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  
  uint32_t argOffset = argc < 3 ? argc : 3;
  nsCOMPtr<nsIJSArgArray> argvArray;
  rv = NS_CreateJSArgv(cx, argc - argOffset, argv + argOffset,
                       getter_AddRefs(argvArray));
  NS_ENSURE_SUCCESS(rv, rv);

  return OpenInternal(aUrl, aName, aOptions,
                      true,             
                      false,            
                      false,            
                      false,            
                      true,                
                      argvArray, nullptr,  
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

JSObject* nsGlobalWindow::CallerGlobal()
{
  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  if (!cx) {
    NS_ERROR("Please don't call this method from C++!");

    return nullptr;
  }

  
  
  
  
  
  
  
  
  JSObject *scriptedGlobal = JS_GetScriptedGlobal(cx);
  JSObject *cxGlobal = JS_GetGlobalForScopeChain(cx);
  if (!xpc::AccessCheck::subsumes(cxGlobal, scriptedGlobal)) {
    NS_WARNING("Something nasty is happening! Applying countermeasures...");
    return cxGlobal;
  }
  return scriptedGlobal;
}


nsGlobalWindow*
nsGlobalWindow::CallerInnerWindow()
{
  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  NS_ENSURE_TRUE(cx, nullptr);
  JSObject *scope = CallerGlobal();

  
  
  
  
  
  
  {
    JSAutoCompartment ac(cx, scope);
    JSObject *scopeProto;
    bool ok = JS_GetPrototype(cx, scope, &scopeProto);
    NS_ENSURE_TRUE(ok, nullptr);
    if (scopeProto && xpc::IsSandboxPrototypeProxy(scopeProto) &&
        (scopeProto = js::UnwrapObjectChecked(scopeProto,  false)))
    {
      scope = scopeProto;
    }
  }
  JSAutoCompartment ac(cx, scope);

  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  nsContentUtils::XPConnect()->
    GetWrappedNativeOfJSObject(cx, scope, getter_AddRefs(wrapper));
  if (!wrapper)
    return nullptr;

  
  
  
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
                     nsGlobalWindow* aTargetWindow,
                     nsIURI* aProvidedOrigin,
                     bool aTrustedCaller)
    : mSource(aSource),
      mCallerOrigin(aCallerOrigin),
      mMessage(nullptr),
      mMessageLen(0),
      mTargetWindow(aTargetWindow),
      mProvidedOrigin(aProvidedOrigin),
      mTrustedCaller(aTrustedCaller)
    {
      MOZ_COUNT_CTOR(PostMessageEvent);
    }
    
    ~PostMessageEvent()
    {
      NS_ASSERTION(!mMessage, "Message should have been deserialized!");
      MOZ_COUNT_DTOR(PostMessageEvent);
    }

    void SetJSData(JSAutoStructuredCloneBuffer& aBuffer)
    {
      NS_ASSERTION(!mMessage && mMessageLen == 0, "Don't call twice!");
      aBuffer.steal(&mMessage, &mMessageLen);
    }

    bool StoreISupports(nsISupports* aSupports)
    {
      mSupportsArray.AppendElement(aSupports);
      return true;
    }

  private:
    nsRefPtr<nsGlobalWindow> mSource;
    nsString mCallerOrigin;
    uint64_t* mMessage;
    size_t mMessageLen;
    nsRefPtr<nsGlobalWindow> mTargetWindow;
    nsCOMPtr<nsIURI> mProvidedOrigin;
    bool mTrustedCaller;
    nsTArray<nsCOMPtr<nsISupports> > mSupportsArray;
};

namespace {

struct StructuredCloneInfo {
  PostMessageEvent* event;
  bool subsumes;
};

static JSObject*
PostMessageReadStructuredClone(JSContext* cx,
                               JSStructuredCloneReader* reader,
                               uint32_t tag,
                               uint32_t data,
                               void* closure)
{
  NS_ASSERTION(closure, "Must have closure!");

  if (tag == SCTAG_DOM_BLOB || tag == SCTAG_DOM_FILELIST) {
    NS_ASSERTION(!data, "Data should be empty");

    nsISupports* supports;
    if (JS_ReadBytes(reader, &supports, sizeof(supports))) {
      JSObject* global = JS_GetGlobalForScopeChain(cx);
      if (global) {
        jsval val;
        nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
        if (NS_SUCCEEDED(nsContentUtils::WrapNative(cx, global, supports,
                                                    &val,
                                                    getter_AddRefs(wrapper)))) {
          return JSVAL_TO_OBJECT(val);
        }
      }
    }
  }

  const JSStructuredCloneCallbacks* runtimeCallbacks =
    js::GetContextStructuredCloneCallbacks(cx);

  if (runtimeCallbacks) {
    return runtimeCallbacks->read(cx, reader, tag, data, nullptr);
  }

  return nullptr;
}

static JSBool
PostMessageWriteStructuredClone(JSContext* cx,
                                JSStructuredCloneWriter* writer,
                                JSObject* obj,
                                void *closure)
{
  StructuredCloneInfo* scInfo = static_cast<StructuredCloneInfo*>(closure);
  NS_ASSERTION(scInfo, "Must have scInfo!");

  nsCOMPtr<nsIXPConnectWrappedNative> wrappedNative;
  nsContentUtils::XPConnect()->
    GetWrappedNativeOfJSObject(cx, obj, getter_AddRefs(wrappedNative));
  if (wrappedNative) {
    uint32_t scTag = 0;
    nsISupports* supports = wrappedNative->Native();

    nsCOMPtr<nsIDOMBlob> blob = do_QueryInterface(supports);
    if (blob && scInfo->subsumes)
      scTag = SCTAG_DOM_BLOB;

    nsCOMPtr<nsIDOMFileList> list = do_QueryInterface(supports);
    if (list && scInfo->subsumes)
      scTag = SCTAG_DOM_FILELIST;

    if (scTag)
      return JS_WriteUint32Pair(writer, scTag, 0) &&
             JS_WriteBytes(writer, &supports, sizeof(supports)) &&
             scInfo->event->StoreISupports(supports);
  }

  const JSStructuredCloneCallbacks* runtimeCallbacks =
    js::GetContextStructuredCloneCallbacks(cx);

  if (runtimeCallbacks) {
    return runtimeCallbacks->write(cx, writer, obj, nullptr);
  }

  return JS_FALSE;
}

JSStructuredCloneCallbacks kPostMessageCallbacks = {
  PostMessageReadStructuredClone,
  PostMessageWriteStructuredClone,
  nullptr
};

} 

NS_IMETHODIMP
PostMessageEvent::Run()
{
  NS_ABORT_IF_FALSE(mTargetWindow->IsOuterWindow(),
                    "should have been passed an outer window!");
  NS_ABORT_IF_FALSE(!mSource || mSource->IsOuterWindow(),
                    "should have been passed an outer window!");

  
  nsIScriptContext* scriptContext = mTargetWindow->GetContext();
  AutoPushJSContext cx(scriptContext ? scriptContext->GetNativeContext()
                                     : nsContentUtils::GetSafeJSContext());
  MOZ_ASSERT(cx);

  
  

  
  JSAutoStructuredCloneBuffer buffer;
  buffer.adopt(mMessage, mMessageLen);
  mMessage = nullptr;
  mMessageLen = 0;

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
      ssm->CheckSameOriginURI(mProvidedOrigin, targetURI, true);
    if (NS_FAILED(rv))
      return NS_OK;
  }

  
  jsval messageData;
  {
    JSAutoRequest ar(cx);
    StructuredCloneInfo scInfo;
    scInfo.event = this;

    if (!buffer.read(cx, &messageData, &kPostMessageCallbacks, &scInfo))
      return NS_ERROR_DOM_DATA_CLONE_ERR;
  }

  
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(targetWindow->mDocument);
  if (!domDoc)
    return NS_OK;
  nsCOMPtr<nsIDOMEvent> event;
  domDoc->CreateEvent(NS_LITERAL_STRING("MessageEvent"),
                      getter_AddRefs(event));
  if (!event)
    return NS_OK;

  nsCOMPtr<nsIDOMMessageEvent> message = do_QueryInterface(event);
  nsresult rv = message->InitMessageEvent(NS_LITERAL_STRING("message"),
                                          false ,
                                          true ,
                                          messageData,
                                          mCallerOrigin,
                                          EmptyString(),
                                          mSource);
  if (NS_FAILED(rv))
    return NS_OK;


  
  
  
  

  nsIPresShell *shell = targetWindow->mDoc->GetShell();
  nsRefPtr<nsPresContext> presContext;
  if (shell)
    presContext = shell->GetPresContext();

  message->SetTrusted(mTrustedCaller);
  nsEvent *internalEvent = message->GetInternalNSEvent();

  nsEventStatus status = nsEventStatus_eIgnore;
  nsEventDispatcher::Dispatch(static_cast<nsPIDOMWindow*>(mTargetWindow),
                              presContext,
                              internalEvent,
                              message,
                              &status);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::PostMessageMoz(const jsval& aMessage,
                               const nsAString& aOrigin,
                               const jsval& aTransfer,
                               JSContext* aCx)
{
  FORWARD_TO_OUTER(PostMessageMoz, (aMessage, aOrigin, aTransfer, aCx),
                   NS_ERROR_NOT_INITIALIZED);

  
  
  
  
  
  
  

  
  nsRefPtr<nsGlobalWindow> callerInnerWin = CallerInnerWindow();
  nsIPrincipal* callerPrin;
  if (callerInnerWin) {
    NS_ABORT_IF_FALSE(callerInnerWin->IsInnerWindow(),
                      "should have gotten an inner window here");

    
    
    
    
    
    
    callerPrin = callerInnerWin->GetPrincipal();
  }
  else {
    
    
    JSObject *global = CallerGlobal();
    NS_ASSERTION(global, "Why is there no global object?");
    JSCompartment *compartment = js::GetObjectCompartment(global);
    callerPrin = xpc::GetCompartmentPrincipal(compartment);
  }
  if (!callerPrin)
    return NS_OK;

  nsCOMPtr<nsIURI> callerOuterURI;
  if (NS_FAILED(callerPrin->GetURI(getter_AddRefs(callerOuterURI))))
    return NS_OK;

  nsAutoString origin;
  if (callerOuterURI) {
    
    nsContentUtils::GetUTFOrigin(callerPrin, origin);
  }
  else if (callerInnerWin) {
    
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(callerInnerWin->GetExtantDocument());
    if (!doc)
      return NS_OK;
    callerOuterURI = doc->GetDocumentURI();
    
    nsContentUtils::GetUTFOrigin(callerOuterURI, origin);
  }
  else {
    
    if (!nsContentUtils::IsSystemPrincipal(callerPrin))
      return NS_OK;
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
    new PostMessageEvent(nsContentUtils::IsCallerChrome() || !callerInnerWin
                         ? nullptr
                         : callerInnerWin->GetOuterWindowInternal(),
                         origin,
                         this,
                         providedOrigin,
                         nsContentUtils::IsCallerChrome());

  
  
  JSAutoStructuredCloneBuffer buffer;
  StructuredCloneInfo scInfo;
  scInfo.event = event;

  nsIPrincipal* principal = GetPrincipal();
  if (NS_FAILED(callerPrin->Subsumes(principal, &scInfo.subsumes)))
    return NS_ERROR_DOM_DATA_CLONE_ERR;

  if (!buffer.write(aCx, aMessage, aTransfer, &kPostMessageCallbacks, &scInfo))
    return NS_ERROR_DOM_DATA_CLONE_ERR;

  event->SetJSData(buffer);

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

bool
nsGlobalWindow::CanClose()
{
  if (!mDocShell)
    return true;

  
  
  
  

  nsCOMPtr<nsIContentViewer> cv;
  mDocShell->GetContentViewer(getter_AddRefs(cv));
  if (cv) {
    bool canClose;
    nsresult rv = cv->PermitUnload(false, &canClose);
    if (NS_SUCCEEDED(rv) && !canClose)
      return false;

    rv = cv->RequestWindowClose(&canClose);
    if (NS_SUCCEEDED(rv) && !canClose)
      return false;
  }

  return true;
}

NS_IMETHODIMP
nsGlobalWindow::Close()
{
  FORWARD_TO_OUTER(Close, (), NS_ERROR_NOT_INITIALIZED);

  if (!mDocShell || IsInModalState() ||
      (IsFrame() && !mDocShell->GetIsBrowserOrApp())) {
    
    
    

    return NS_OK;
  }

  if (mHavePendingClose) {
    
    
    return NS_OK;
  }

  if (mBlockScriptedClosingFlag)
  {
    
    
    
    return NS_OK;
  }

  
  
  if (!mDocShell->GetIsApp() &&
      !mHadOriginalOpener && !nsContentUtils::IsCallerChrome()) {
    bool allowClose = mAllowScriptsToClose ||
      Preferences::GetBool("dom.allow_scripts_to_close_windows", true);
    if (!allowClose) {
      
      
      nsContentUtils::ReportToConsole(
          nsIScriptError::warningFlag,
          "DOM Window", mDoc,  
          nsContentUtils::eDOM_PROPERTIES,
          "WindowCloseBlockedWarning");

      return NS_OK;
    }
  }

  if (!mInClose && !mIsClosed && !CanClose())
    return NS_OK;

  
  
  
  
  
  

  bool wasInClose = mInClose;
  mInClose = true;

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

  mInClose = true;

  DispatchCustomEvent("DOMWindowClose");

  return FinalClose();
}

nsresult
nsGlobalWindow::FinalClose()
{
  
  mIsClosed = true;

  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService(sJSStackContractID);

  JSContext *cx = nullptr;

  if (stack) {
    stack->Peek(&cx);
  }

  if (cx) {
    nsIScriptContext *currentCX = nsJSUtils::GetDynamicScriptContext(cx);

    if (currentCX && currentCX == GetContextInternal()) {
      currentCX->SetTerminationFunction(CloseWindow, this);
      mHavePendingClose = true;
      return NS_OK;
    }
  }

  
  
  
  
  if (nsContentUtils::IsCallerChrome() ||
      NS_FAILED(nsCloseEvent::PostCloseEvent(this))) {
    ReallyCloseWindow();
  } else {
    mHavePendingClose = true;
  }

  return NS_OK;
}


void
nsGlobalWindow::ReallyCloseWindow()
{
  FORWARD_TO_OUTER_VOID(ReallyCloseWindow, ());

  
  mHavePendingClose = true;

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));

  

  if (treeOwnerAsWin) {

    
    

    if (mDocShell) {
      nsCOMPtr<nsIBrowserDOMWindow> bwin;
      nsCOMPtr<nsIDocShellTreeItem> rootItem;
      mDocShell->GetRootTreeItem(getter_AddRefs(rootItem));
      nsCOMPtr<nsIDOMWindow> rootWin(do_GetInterface(rootItem));
      nsCOMPtr<nsIDOMChromeWindow> chromeWin(do_QueryInterface(rootWin));
      if (chromeWin)
        chromeWin->GetBrowserDOMWindow(getter_AddRefs(bwin));

      if (rootWin) {
        









        
        bool isTab = false;
        if (rootWin == this ||
            !bwin || (bwin->IsTabContentWindow(GetOuterWindowInternal(),
                                               &isTab), isTab))
          treeOwnerAsWin->Destroy();
      }
    }

    CleanUp(false);
  }
}

nsIDOMWindow *
nsGlobalWindow::EnterModalState()
{
  
  
  nsGlobalWindow* topWin = GetScriptableTop();

  if (!topWin) {
    NS_ERROR("Uh, EnterModalState() called w/o a reachable top window?");

    return nullptr;
  }

  
  
  nsEventStateManager* activeESM =
    static_cast<nsEventStateManager*>(nsEventStateManager::GetActiveEventStateManager());
  if (activeESM && activeESM->GetPresContext()) {
    nsIPresShell* activeShell = activeESM->GetPresContext()->GetPresShell();
    if (activeShell && (
        nsContentUtils::ContentIsCrossDocDescendantOf(activeShell->GetDocument(), mDoc) ||
        nsContentUtils::ContentIsCrossDocDescendantOf(mDoc, activeShell->GetDocument()))) {
      nsEventStateManager::ClearGlobalActiveContent(activeESM);

      activeShell->SetCapturingContent(nullptr, 0);

      if (activeShell) {
        nsRefPtr<nsFrameSelection> frameSelection = activeShell->FrameSelection();
        frameSelection->SetMouseDownState(false);
      }
    }
  }

  if (topWin->mModalStateDepth == 0) {
    NS_ASSERTION(!mSuspendedDoc, "Shouldn't have mSuspendedDoc here!");

    mSuspendedDoc = do_QueryInterface(topWin->GetExtantDocument());
    if (mSuspendedDoc && mSuspendedDoc->EventHandlingSuppressed()) {
      mSuspendedDoc->SuppressEventHandling();
    } else {
      mSuspendedDoc = nullptr;
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

  inner->RunTimeout(nullptr);

  
  
  if (inner->IsFrozen()) {
    return;
  }

  nsCOMPtr<nsIDOMWindowCollection> frames;
  aWindow->GetFrames(getter_AddRefs(frames));

  if (!frames) {
    return;
  }

  uint32_t i, length;
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
  nsGlobalWindow* topWin = GetScriptableTop();

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
      mSuspendedDoc = nullptr;
    }
  }

  if (aCallerWin) {
    nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(aCallerWin));
    if (sgo) {
      nsIScriptContext *scx = sgo->GetContext();
      if (scx)
        scx->LeaveModalState();
    }
  }

  if (mContext) {
    mContext->LeaveModalState();
  }

  
  nsGlobalWindow *inner = topWin->GetCurrentInnerWindowInternal();
  if (inner)
    inner->mLastDialogQuitTime = TimeStamp::Now();
}

bool
nsGlobalWindow::IsInModalState()
{
  nsGlobalWindow *topWin = GetScriptableTop();

  if (!topWin) {
    NS_ERROR("Uh, IsInModalState() called w/o a reachable top window?");

    return false;
  }

  return topWin->mModalStateDepth != 0;
}


void
nsGlobalWindow::NotifyDOMWindowDestroyed(nsGlobalWindow* aWindow) {
  nsCOMPtr<nsIObserverService> observerService =
    services::GetObserverService();
  if (observerService) {
    observerService->
      NotifyObservers(ToSupports(aWindow),
                      DOM_WINDOW_DESTROYED_TOPIC, nullptr);
  }
}

class WindowDestroyedEvent : public nsRunnable
{
public:
  WindowDestroyedEvent(nsPIDOMWindow* aWindow, uint64_t aID,
                       const char* aTopic) :
    mID(aID), mTopic(aTopic)
  {
    mWindow = do_GetWeakReference(aWindow);
  }

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1");
    if (observerService) {
      nsCOMPtr<nsISupportsPRUint64> wrapper =
        do_CreateInstance(NS_SUPPORTS_PRUINT64_CONTRACTID);
      if (wrapper) {
        wrapper->SetData(mID);
        observerService->NotifyObservers(wrapper, mTopic.get(), nullptr);
      }
    }

    bool skipNukeCrossCompartment = false;
#ifndef DEBUG
    nsCOMPtr<nsIAppStartup> appStartup =
      do_GetService(NS_APPSTARTUP_CONTRACTID);

    if (appStartup) {
      appStartup->GetShuttingDown(&skipNukeCrossCompartment);
    }
#endif

    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
    if (!skipNukeCrossCompartment && window) {
      nsGlobalWindow* currentInner =
        window->IsInnerWindow() ? static_cast<nsGlobalWindow*>(window.get()) :
                                  static_cast<nsGlobalWindow*>(window->GetCurrentInnerWindow());
      NS_ENSURE_TRUE(currentInner, NS_OK);

      JSObject* obj = currentInner->FastGetGlobalJSObject();
      
      if (obj && !js::IsSystemCompartment(js::GetObjectCompartment(obj))) {
        JSContext* cx =
          nsContentUtils::ThreadJSContextStack()->GetSafeJSContext();

        JSAutoRequest ar(cx);
        js::NukeCrossCompartmentWrappers(cx, 
                                         js::ChromeCompartmentsOnly(),
                                         js::SingleCompartment(js::GetObjectCompartment(obj)),
                                         window->IsInnerWindow() ? js::DontNukeWindowReferences :
                                                                   js::NukeWindowReferences);
      }
    }

    return NS_OK;
  }

private:
  uint64_t mID;
  nsCString mTopic;
  nsWeakPtr mWindow;
};

void
nsGlobalWindow::NotifyWindowIDDestroyed(const char* aTopic)
{
  nsRefPtr<nsIRunnable> runnable = new WindowDestroyedEvent(this, mWindowID, aTopic);
  nsresult rv = NS_DispatchToCurrentThread(runnable);
  if (NS_SUCCEEDED(rv)) {
    mNotifiedIDDestroyed = true;
  }
}


void
nsGlobalWindow::NotifyDOMWindowFrozen(nsGlobalWindow* aWindow) {
  if (aWindow && aWindow->IsInnerWindow()) {
    nsCOMPtr<nsIObserverService> observerService =
      services::GetObserverService();
    if (observerService) {
      observerService->
        NotifyObservers(ToSupports(aWindow),
                        DOM_WINDOW_FROZEN_TOPIC, nullptr);
    }
  }
}


void
nsGlobalWindow::NotifyDOMWindowThawed(nsGlobalWindow* aWindow) {
  if (aWindow && aWindow->IsInnerWindow()) {
    nsCOMPtr<nsIObserverService> observerService =
      services::GetObserverService();
    if (observerService) {
      observerService->
        NotifyObservers(ToSupports(aWindow),
                        DOM_WINDOW_THAWED_TOPIC, nullptr);
    }
  }
}

JSObject*
nsGlobalWindow::GetCachedXBLPrototypeHandler(nsXBLPrototypeHandler* aKey)
{
  JSObject* handler = nullptr;
  if (mCachedXBLPrototypeHandlers.IsInitialized()) {
    mCachedXBLPrototypeHandlers.Get(aKey, &handler);
  }
  return handler;
}

void
nsGlobalWindow::CacheXBLPrototypeHandler(nsXBLPrototypeHandler* aKey,
                                         JS::Handle<JSObject*> aHandler)
{
  if (!mCachedXBLPrototypeHandlers.IsInitialized()) {
    mCachedXBLPrototypeHandlers.Init();
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

    nsContentUtils::HoldJSObjects(thisSupports, participant);
  }

  mCachedXBLPrototypeHandlers.Put(aKey, aHandler);
}










NS_IMETHODIMP
nsGlobalWindow::GetScriptableFrameElement(nsIDOMElement** aFrameElement)
{
  FORWARD_TO_OUTER(GetScriptableFrameElement, (aFrameElement), NS_ERROR_NOT_INITIALIZED);
  *aFrameElement = NULL;

  if (!mDocShell || mDocShell->GetIsBrowserOrApp()) {
    return NS_OK;
  }

  return GetFrameElement(aFrameElement);
}





NS_IMETHODIMP
nsGlobalWindow::GetRealFrameElement(nsIDOMElement** aFrameElement)
{
  FORWARD_TO_OUTER(GetRealFrameElement, (aFrameElement), NS_ERROR_NOT_INITIALIZED);

  *aFrameElement = NULL;

  if (!mDocShell) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocShell> parent;
  mDocShell->GetSameTypeParentIgnoreBrowserAndAppBoundaries(getter_AddRefs(parent));

  if (!parent || parent == mDocShell) {
    
    
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

  *aRetVal = nullptr;

  if (Preferences::GetBool("dom.disable_window_showModalDialog", false))
    return NS_ERROR_NOT_AVAILABLE;

  
  
  EnsureReflowFlushAndPaint();

  bool needToPromptForAbuse;
  if (DialogsAreBlocked(&needToPromptForAbuse)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (needToPromptForAbuse && !ConfirmDialogIfNeeded()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIDOMWindow> dlgWin;
  nsAutoString options(NS_LITERAL_STRING("-moz-internal-modal=1,status=1"));

  ConvertDialogOptions(aOptions, options);

  options.AppendLiteral(",scrollbars=1,centerscreen=1,resizable=0");

  nsCOMPtr<nsIDOMWindow> callerWin = EnterModalState();
  uint32_t oldMicroTaskLevel = nsContentUtils::MicroTaskLevel();
  nsContentUtils::SetMicroTaskLevel(0);
  nsresult rv = OpenInternal(aURI, EmptyString(), options,
                             false,          
                             true,           
                             true,           
                             true,           
                             true,           
                             nullptr, aArgs, 
                             GetPrincipal(),    
                             nullptr,            
                             getter_AddRefs(dlgWin));
  nsContentUtils::SetMicroTaskLevel(oldMicroTaskLevel);
  LeaveModalState(callerWin);

  NS_ENSURE_SUCCESS(rv, rv);
  
  if (dlgWin) {
    nsCOMPtr<nsIPrincipal> subjectPrincipal;
    rv = nsContentUtils::GetSecurityManager()->
      GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
    if (NS_FAILED(rv)) {
      return rv;
    }

    bool canAccess = true;

    if (subjectPrincipal) {
      nsCOMPtr<nsIScriptObjectPrincipal> objPrincipal =
        do_QueryInterface(dlgWin);
      nsCOMPtr<nsIPrincipal> dialogPrincipal;

      if (objPrincipal) {
        dialogPrincipal = objPrincipal->GetPrincipal();

        rv = subjectPrincipal->Subsumes(dialogPrincipal, &canAccess);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        
        

        canAccess = false;
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
      winInternal->mCallCleanUpAfterModalDialogCloses = false;
      winInternal->CleanUp(true);
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

NS_IMETHODIMP
nsGlobalWindow::GetSelection(nsISelection** aSelection)
{
  FORWARD_TO_OUTER(GetSelection, (aSelection), NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aSelection);
  *aSelection = nullptr;

  if (!mDocShell)
    return NS_OK;

  nsCOMPtr<nsIPresShell> presShell = mDocShell->GetPresShell();

  if (!presShell)
    return NS_OK;
    
  *aSelection = presShell->GetCurrentSelection(nsISelectionController::SELECTION_NORMAL);
  
  NS_IF_ADDREF(*aSelection);

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::Find(const nsAString& aStr, bool aCaseSensitive,
                     bool aBackwards, bool aWrapAround, bool aWholeWord,
                     bool aSearchInFrames, bool aShowDialog,
                     bool *aDidFind)
{
  if (Preferences::GetBool("dom.disable_window_find", false))
    return NS_ERROR_NOT_AVAILABLE;

  FORWARD_TO_OUTER(Find, (aStr, aCaseSensitive, aBackwards, aWrapAround,
                          aWholeWord, aSearchInFrames, aShowDialog, aDidFind),
                   NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_OK;
  *aDidFind = false;

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

    nsCOMPtr<nsIDOMWindow> findDialog;

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

NS_IMETHODIMP
nsGlobalWindow::Atob(const nsAString& aAsciiBase64String,
                     nsAString& aBinaryData)
{
  return nsContentUtils::Atob(aAsciiBase64String, aBinaryData);
}

NS_IMETHODIMP
nsGlobalWindow::Btoa(const nsAString& aBinaryData,
                     nsAString& aAsciiBase64String)
{
  return nsContentUtils::Btoa(aBinaryData, aAsciiBase64String);
}





NS_IMETHODIMP
nsGlobalWindow::RemoveEventListener(const nsAString& aType,
                                    nsIDOMEventListener* aListener,
                                    bool aUseCapture)
{
  nsRefPtr<nsEventListenerManager> elm = GetListenerManager(false);
  if (elm) {
    elm->RemoveEventListener(aType, aListener, aUseCapture);
  }
  return NS_OK;
}

NS_IMPL_REMOVE_SYSTEM_EVENT_LISTENER(nsGlobalWindow)

NS_IMETHODIMP
nsGlobalWindow::DispatchEvent(nsIDOMEvent* aEvent, bool* aRetVal)
{
  FORWARD_TO_INNER(DispatchEvent, (aEvent, aRetVal), NS_OK);

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
    nsEventDispatcher::DispatchDOMEvent(GetOuterWindow(), nullptr, aEvent,
                                        presContext, &status);

  *aRetVal = (status != nsEventStatus_eConsumeNoDefault);
  return rv;
}

NS_IMETHODIMP
nsGlobalWindow::AddEventListener(const nsAString& aType,
                                 nsIDOMEventListener *aListener,
                                 bool aUseCapture, bool aWantsUntrusted,
                                 uint8_t aOptionalArgc)
{
  NS_ASSERTION(!aWantsUntrusted || aOptionalArgc > 1,
               "Won't check if this is chrome, you want to set "
               "aWantsUntrusted to false or make the aWantsUntrusted "
               "explicit by making optional_argc non-zero.");

  if (IsOuterWindow() && mInnerWindow &&
      !nsContentUtils::CanCallerAccess(mInnerWindow)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (!aWantsUntrusted &&
      (aOptionalArgc < 2 && !nsContentUtils::IsChromeDoc(mDoc))) {
    aWantsUntrusted = true;
  }

  nsEventListenerManager* manager = GetListenerManager(true);
  NS_ENSURE_STATE(manager);
  manager->AddEventListener(aType, aListener, aUseCapture, aWantsUntrusted);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::AddSystemEventListener(const nsAString& aType,
                                       nsIDOMEventListener *aListener,
                                       bool aUseCapture,
                                       bool aWantsUntrusted,
                                       uint8_t aOptionalArgc)
{
  NS_ASSERTION(!aWantsUntrusted || aOptionalArgc > 1,
               "Won't check if this is chrome, you want to set "
               "aWantsUntrusted to false or make the aWantsUntrusted "
               "explicit by making optional_argc non-zero.");

  if (IsOuterWindow() && mInnerWindow &&
      !nsContentUtils::CanCallerAccess(mInnerWindow)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (!aWantsUntrusted &&
      (aOptionalArgc < 2 && !nsContentUtils::IsChromeDoc(mDoc))) {
    aWantsUntrusted = true;
  }

  return NS_AddSystemEventListener(this, aType, aListener, aUseCapture,
                                   aWantsUntrusted);
}

nsEventListenerManager*
nsGlobalWindow::GetListenerManager(bool aCreateIfNotFound)
{
  FORWARD_TO_INNER_CREATE(GetListenerManager, (aCreateIfNotFound), nullptr);

  if (!mListenerManager && aCreateIfNotFound) {
    mListenerManager =
      new nsEventListenerManager(static_cast<nsIDOMEventTarget*>(this));
  }

  return mListenerManager;
}

nsIScriptContext*
nsGlobalWindow::GetContextForEventHandlers(nsresult* aRv)
{
  *aRv = NS_ERROR_UNEXPECTED;
  if (IsInnerWindow()) {
    nsPIDOMWindow* outer = GetOuterWindow();
    NS_ENSURE_TRUE(outer && outer->GetCurrentInnerWindow() == this, nullptr);
  }

  nsIScriptContext* scx;
  if ((scx = GetContext())) {
    *aRv = NS_OK;
    return scx;
  }
  return nullptr;
}





nsPIDOMWindow*
nsGlobalWindow::GetPrivateParent()
{
  FORWARD_TO_OUTER(GetPrivateParent, (), nullptr);

  nsCOMPtr<nsIDOMWindow> parent;
  GetParent(getter_AddRefs(parent));

  if (static_cast<nsIDOMWindow *>(this) == parent.get()) {
    nsCOMPtr<nsIContent> chromeElement(do_QueryInterface(mChromeEventHandler));
    if (!chromeElement)
      return nullptr;             

    nsIDocument* doc = chromeElement->GetDocument();
    if (!doc)
      return nullptr;             

    nsIScriptGlobalObject *globalObject = doc->GetScriptGlobalObject();
    if (!globalObject)
      return nullptr;             

    parent = do_QueryInterface(globalObject);
  }

  if (parent) {
    return static_cast<nsGlobalWindow *>
                      (static_cast<nsIDOMWindow*>(parent.get()));
  }

  return nullptr;
}

nsPIDOMWindow*
nsGlobalWindow::GetPrivateRoot()
{
  FORWARD_TO_OUTER(GetPrivateRoot, (), nullptr);

  nsCOMPtr<nsIDOMWindow> top;
  GetTop(getter_AddRefs(top));

  nsCOMPtr<nsPIDOMWindow> ptop = do_QueryInterface(top);
  NS_ASSERTION(ptop, "cannot get ptop");
  if (!ptop)
    return nullptr;

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

  *aLocation = nullptr;

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
nsGlobalWindow::ActivateOrDeactivate(bool aActivate)
{
  
  
  nsCOMPtr<nsIWidget> mainWidget = GetMainWidget();
  if (!mainWidget)
    return;

  
  
  nsCOMPtr<nsIWidget> topLevelWidget = mainWidget->GetSheetWindowParent();
  if (!topLevelWidget) {
    topLevelWidget = mainWidget;
  }

  
  nsCOMPtr<nsIDOMWindow> topLevelWindow;
  if (topLevelWidget == mainWidget) {
    topLevelWindow = static_cast<nsIDOMWindow*>(this);
  } else {
    
    
    
    

    
    nsIWidgetListener* listener = topLevelWidget->GetWidgetListener();
    if (listener) {
      nsCOMPtr<nsIXULWindow> window = listener->GetXULWindow();
      nsCOMPtr<nsIInterfaceRequestor> req(do_QueryInterface(window));
      topLevelWindow = do_GetInterface(req);
    }
  }
  if (topLevelWindow) {
    nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(topLevelWindow));
    piWin->SetActive(aActivate);
  }
}

static bool
NotifyDocumentTree(nsIDocument* aDocument, void* aData)
{
  aDocument->EnumerateSubDocuments(NotifyDocumentTree, nullptr);
  aDocument->DocumentStatesChanged(NS_DOCUMENT_STATE_WINDOW_INACTIVE);
  return true;
}

void
nsGlobalWindow::SetActive(bool aActive)
{
  nsPIDOMWindow::SetActive(aActive);
  NotifyDocumentTree(mDoc, nullptr);
}

void nsGlobalWindow::SetIsBackground(bool aIsBackground)
{
  bool resetTimers = (!aIsBackground && IsBackground());
  nsPIDOMWindow::SetIsBackground(aIsBackground);
  if (resetTimers) {
    ResetTimersForNonBackgroundWindow();
  }
#ifdef MOZ_GAMEPAD
  if (!aIsBackground) {
    nsGlobalWindow* inner = GetCurrentInnerWindowInternal();
    if (inner) {
      inner->SyncGamepadState();
    }
  }
#endif
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

  if (mMayHaveTouchEventListener) {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID);

    if (observerService) {
      observerService->NotifyObservers(static_cast<nsIDOMWindow*>(this),
                                       DOM_TOUCH_LISTENER_ADDED,
                                       nullptr);
    }
  }
}

void nsGlobalWindow::UpdateTouchState()
{
  FORWARD_TO_INNER_VOID(UpdateTouchState, ());

  nsCOMPtr<nsIWidget> mainWidget = GetMainWidget();
  if (!mainWidget) {
    return;
  }

  if (mMayHaveTouchEventListener) {
    mainWidget->RegisterTouchWindow();
  } else {
    mainWidget->UnregisterTouchWindow();
  }
}

void
nsGlobalWindow::EnableGamepadUpdates()
{
  FORWARD_TO_INNER_VOID(EnableGamepadUpdates, ());
  if (mHasGamepad) {
#ifdef MOZ_GAMEPAD
    nsRefPtr<GamepadService> gamepadsvc(GamepadService::GetService());
    if (gamepadsvc) {
      gamepadsvc->AddListener(this);
    }
#endif
  }
}

void
nsGlobalWindow::DisableGamepadUpdates()
{
  FORWARD_TO_INNER_VOID(DisableGamepadUpdates, ());
  if (mHasGamepad) {
#ifdef MOZ_GAMEPAD
    nsRefPtr<GamepadService> gamepadsvc(GamepadService::GetService());
    if (gamepadsvc) {
      gamepadsvc->RemoveListener(this);
    }
#endif
  }
}

void
nsGlobalWindow::SetChromeEventHandler(nsIDOMEventTarget* aChromeEventHandler)
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

static bool IsLink(nsIContent* aContent)
{
  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor = do_QueryInterface(aContent);
  return (anchor || (aContent &&
                     aContent->AttrValueIs(kNameSpaceID_XLink, nsGkAtoms::type,
                                           nsGkAtoms::simple, eCaseMatters)));
}

void
nsGlobalWindow::SetFocusedNode(nsIContent* aNode,
                               uint32_t aFocusMethod,
                               bool aNeedsFocus)
{
  FORWARD_TO_INNER_VOID(SetFocusedNode, (aNode, aFocusMethod, aNeedsFocus));

  if (aNode && aNode->GetCurrentDoc() != mDoc) {
    NS_WARNING("Trying to set focus to a node from a wrong document");
    return;
  }

  if (mCleanedUp) {
    NS_ASSERTION(!aNode, "Trying to focus cleaned up window!");
    aNode = nullptr;
    aNeedsFocus = false;
  }
  if (mFocusedNode != aNode) {
    UpdateCanvasFocus(false, aNode);
    mFocusedNode = aNode;
    mFocusMethod = aFocusMethod & FOCUSMETHOD_MASK;
    mShowFocusRingForContent = false;
  }

  if (mFocusedNode) {
    
    
    if (mFocusMethod & nsIFocusManager::FLAG_BYKEY) {
      mFocusByKeyOccurred = true;
    } else if (
      
      
      
      
#ifndef XP_WIN
      !(mFocusMethod & nsIFocusManager::FLAG_BYMOUSE) || !IsLink(aNode) ||
#endif
      aFocusMethod & nsIFocusManager::FLAG_SHOWRING) {
        mShowFocusRingForContent = true;
    }
  }

  if (aNeedsFocus)
    mNeedsFocus = aNeedsFocus;
}

uint32_t
nsGlobalWindow::GetFocusMethod()
{
  FORWARD_TO_INNER(GetFocusMethod, (), 0);

  return mFocusMethod;
}

bool
nsGlobalWindow::ShouldShowFocusRing()
{
  FORWARD_TO_INNER(ShouldShowFocusRing, (), false);

  return mShowFocusRings || mShowFocusRingForContent || mFocusByKeyOccurred;
}

void
nsGlobalWindow::SetKeyboardIndicators(UIStateChangeType aShowAccelerators,
                                      UIStateChangeType aShowFocusRings)
{
  FORWARD_TO_INNER_VOID(SetKeyboardIndicators, (aShowAccelerators, aShowFocusRings));

  bool oldShouldShowFocusRing = ShouldShowFocusRing();

  
  if (aShowAccelerators != UIStateChangeType_NoChange)
    mShowAccelerators = aShowAccelerators == UIStateChangeType_Set;
  if (aShowFocusRings != UIStateChangeType_NoChange)
    mShowFocusRings = aShowFocusRings == UIStateChangeType_Set;

  
  nsCOMPtr<nsIDocShell> docShell = GetDocShell();
  if (docShell) {
    int32_t childCount = 0;
    docShell->GetChildCount(&childCount);

    for (int32_t i = 0; i < childCount; ++i) {
      nsCOMPtr<nsIDocShellTreeItem> childShell;
      docShell->GetChildAt(i, getter_AddRefs(childShell));
      nsCOMPtr<nsPIDOMWindow> childWindow = do_GetInterface(childShell);
      if (childWindow) {
        childWindow->SetKeyboardIndicators(aShowAccelerators, aShowFocusRings);
      }
    }
  }

  bool newShouldShowFocusRing = ShouldShowFocusRing();
  if (mHasFocus && mFocusedNode &&
      oldShouldShowFocusRing != newShouldShowFocusRing &&
      mFocusedNode->IsElement()) {
    
    if (newShouldShowFocusRing) {
      mFocusedNode->AsElement()->AddStates(NS_EVENT_STATE_FOCUSRING);
    } else {
      mFocusedNode->AsElement()->RemoveStates(NS_EVENT_STATE_FOCUSRING);
    }
  }
}

void
nsGlobalWindow::GetKeyboardIndicators(bool* aShowAccelerators,
                                      bool* aShowFocusRings)
{
  FORWARD_TO_INNER_VOID(GetKeyboardIndicators, (aShowAccelerators, aShowFocusRings));

  *aShowAccelerators = mShowAccelerators;
  *aShowFocusRings = mShowFocusRings;
}

bool
nsGlobalWindow::TakeFocus(bool aFocus, uint32_t aFocusMethod)
{
  FORWARD_TO_INNER(TakeFocus, (aFocus, aFocusMethod), false);

  if (mCleanedUp) {
    return false;
  }
  
  if (aFocus)
    mFocusMethod = aFocusMethod & FOCUSMETHOD_MASK;

  if (mHasFocus != aFocus) {
    mHasFocus = aFocus;
    UpdateCanvasFocus(true, mFocusedNode);
  }

  
  
  
  
  
  if (aFocus && mNeedsFocus && mDoc && mDoc->GetRootElement() != nullptr) {
    mNeedsFocus = false;
    return true;
  }

  mNeedsFocus = false;
  return false;
}

void
nsGlobalWindow::SetReadyForFocus()
{
  FORWARD_TO_INNER_VOID(SetReadyForFocus, ());

  bool oldNeedsFocus = mNeedsFocus;
  mNeedsFocus = false;

  
  
  nsPIDOMWindow* root = GetPrivateRoot();
  if (root) {
    bool showAccelerators, showFocusRings;
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

  mNeedsFocus = true;
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

  
  
  nsAutoCString oldBeforeHash, oldHash, newBeforeHash, newHash;
  nsContentUtils::SplitURIAtHash(aOldURI, oldBeforeHash, oldHash);
  nsContentUtils::SplitURIAtHash(aNewURI, newBeforeHash, newHash);

  NS_ENSURE_STATE(oldBeforeHash.Equals(newBeforeHash));
  NS_ENSURE_STATE(!oldHash.Equals(newHash));

  nsAutoCString oldSpec, newSpec;
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
    nsEventDispatcher::CreateEvent(this, presContext, nullptr,
                                   NS_LITERAL_STRING("hashchangeevent"),
                                   getter_AddRefs(domEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMHashChangeEvent> hashchangeEvent = do_QueryInterface(domEvent);
  NS_ENSURE_TRUE(hashchangeEvent, NS_ERROR_UNEXPECTED);

  
  rv = hashchangeEvent->InitHashChangeEvent(NS_LITERAL_STRING("hashchange"),
                                            true, false,
                                            aOldURL, aNewURL);
  NS_ENSURE_SUCCESS(rv, rv);

  domEvent->SetTrusted(true);

  bool dummy;
  return DispatchEvent(hashchangeEvent, &dummy);
}

nsresult
nsGlobalWindow::DispatchSyncPopState()
{
  FORWARD_TO_INNER(DispatchSyncPopState, (), NS_OK);

  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(),
               "Must be safe to run script here.");

  
  if (!Preferences::GetBool(sPopStatePrefStr, false)) {
    return NS_OK;
  }

  nsresult rv = NS_OK;

  
  if (IsFrozen()) {
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIVariant> stateObj;
  rv = mDoc->GetStateObject(getter_AddRefs(stateObj));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsIPresShell *shell = mDoc->GetShell();
  nsRefPtr<nsPresContext> presContext;
  if (shell) {
    presContext = shell->GetPresContext();
  }

  
  nsCOMPtr<nsIDOMEvent> domEvent;
  rv = nsEventDispatcher::CreateEvent(this, presContext, nullptr,
                                      NS_LITERAL_STRING("popstateevent"),
                                      getter_AddRefs(domEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMPopStateEvent> popstateEvent = do_QueryInterface(domEvent);
  rv = popstateEvent->InitPopStateEvent(NS_LITERAL_STRING("popstate"),
                                        true, false,
                                        stateObj);
  NS_ENSURE_SUCCESS(rv, rv);

  domEvent->SetTrusted(true);

  nsCOMPtr<nsIDOMEventTarget> outerWindow =
    do_QueryInterface(GetOuterWindow());
  NS_ENSURE_TRUE(outerWindow, NS_ERROR_UNEXPECTED);

  rv = domEvent->SetTarget(outerWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  bool dummy; 
  return DispatchEvent(popstateEvent, &dummy);
}



static nsCanvasFrame* FindCanvasFrame(nsIFrame* aFrame)
{
    nsCanvasFrame* canvasFrame = do_QueryFrame(aFrame);
    if (canvasFrame) {
        return canvasFrame;
    }

    nsIFrame* kid = aFrame->GetFirstPrincipalChild();
    while (kid) {
        canvasFrame = FindCanvasFrame(kid);
        if (canvasFrame) {
            return canvasFrame;
        }
        kid = kid->GetNextSibling();
    }

    return nullptr;
}



void
nsGlobalWindow::UpdateCanvasFocus(bool aFocusChanged, nsIContent* aNewContent)
{
  
  nsIDocShell* docShell = GetDocShell();
  if (!docShell)
    return;

  bool editable;
  docShell->GetEditable(&editable);
  if (editable)
    return;

  nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();
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
              canvasFrame->SetHasFocus(false);
          }
      }      
  }
}

NS_IMETHODIMP
nsGlobalWindow::GetComputedStyle(nsIDOMElement* aElt,
                                 const nsAString& aPseudoElt,
                                 nsIDOMCSSStyleDeclaration** aReturn)
{
  return GetComputedStyleHelper(aElt, aPseudoElt, false, aReturn);
}

NS_IMETHODIMP
nsGlobalWindow::GetDefaultComputedStyle(nsIDOMElement* aElt,
                                        const nsAString& aPseudoElt,
                                        nsIDOMCSSStyleDeclaration** aReturn)
{
  return GetComputedStyleHelper(aElt, aPseudoElt, true, aReturn);
}

nsresult
nsGlobalWindow::GetComputedStyleHelper(nsIDOMElement* aElt,
                                       const nsAString& aPseudoElt,
                                       bool aDefaultStylesOnly,
                                       nsIDOMCSSStyleDeclaration** aReturn)
{
  FORWARD_TO_OUTER(GetComputedStyleHelper, (aElt, aPseudoElt,
                                            aDefaultStylesOnly, aReturn),
                   NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nullptr;

  if (!aElt) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  if (!mDocShell) {
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> presShell = mDocShell->GetPresShell();

  if (!presShell) {
    
    
    nsGlobalWindow *parent =
      static_cast<nsGlobalWindow *>(GetPrivateParent());
    if (!parent) {
      return NS_OK;
    }

    parent->FlushPendingNotifications(Flush_Frames);

    
    if (!mDocShell) {
      return NS_OK;
    }

    presShell = mDocShell->GetPresShell();
    if (!presShell) {
      return NS_OK;
    }
  }

  nsCOMPtr<dom::Element> element = do_QueryInterface(aElt);
  NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);
  nsRefPtr<nsComputedDOMStyle> compStyle =
    NS_NewComputedDOMStyle(element, aPseudoElt, presShell,
                           aDefaultStylesOnly ? nsComputedDOMStyle::eDefaultOnly :
                                                nsComputedDOMStyle::eAll);

  *aReturn = compStyle.forget().get();

  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetSessionStorage(nsIDOMStorage ** aSessionStorage)
{
  FORWARD_TO_INNER(GetSessionStorage, (aSessionStorage), NS_ERROR_UNEXPECTED);

  nsIPrincipal *principal = GetPrincipal();
  nsIDocShell* docShell = GetDocShell();

  if (!principal || !docShell) {
    *aSessionStorage = nullptr;
    return NS_OK;
  }

  if (!Preferences::GetBool(kStorageEnabled)) {
    *aSessionStorage = nullptr;
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
      bool canAccess = piStorage->CanAccess(principal);
      NS_ASSERTION(canAccess,
                   "window %x owned sessionStorage "
                   "that could not be accessed!");
      if (!canAccess) {
          mSessionStorage = nullptr;
      }
    }
  }

  if (!mSessionStorage) {
    *aSessionStorage = nullptr;

    nsString documentURI;
    if (mDocument) {
      mDocument->GetDocumentURI(documentURI);
    }

    
    
    if (!mDoc) {
      return NS_ERROR_FAILURE;
    }

    if (mDoc->GetSandboxFlags() & SANDBOXED_ORIGIN) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }

    nsresult rv = docShell->GetSessionStorageForPrincipal(principal,
                                                          documentURI,
                                                          true,
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

    nsCOMPtr<nsIPrivacyTransitionObserver> obs = do_GetInterface(mSessionStorage);
    if (obs) {
      docShell->AddWeakPrivacyTransitionObserver(obs);
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
nsGlobalWindow::GetLocalStorage(nsIDOMStorage ** aLocalStorage)
{
  FORWARD_TO_INNER(GetLocalStorage, (aLocalStorage), NS_ERROR_UNEXPECTED);

  NS_ENSURE_ARG(aLocalStorage);

  if (!Preferences::GetBool(kStorageEnabled)) {
    *aLocalStorage = nullptr;
    return NS_OK;
  }

  if (!mLocalStorage) {
    *aLocalStorage = nullptr;

    nsresult rv;

    if (!nsDOMStorage::CanUseStorage())
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

    
    
    if (mDoc && (mDoc->GetSandboxFlags() & SANDBOXED_ORIGIN)) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }

    nsIDocShell* docShell = GetDocShell();
    nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(docShell);

    rv = storageManager->GetLocalStorageForPrincipal(principal,
                                                     documentURI,
                                                     loadContext && loadContext->UsePrivateBrowsing(),
                                                     getter_AddRefs(mLocalStorage));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrivacyTransitionObserver> obs = do_GetInterface(mLocalStorage);
    if (obs && docShell) {
      docShell->AddWeakPrivacyTransitionObserver(obs);
    }
  }

  NS_ADDREF(*aLocalStorage = mLocalStorage);
  return NS_OK;
}





NS_IMETHODIMP
nsGlobalWindow::GetIndexedDB(nsIIDBFactory** _retval)
{
  if (!mIndexedDB) {
    nsresult rv;

    
    
    if (mDoc && (mDoc->GetSandboxFlags() & SANDBOXED_ORIGIN)) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }

    if (!IsChromeWindow()) {
      nsCOMPtr<mozIThirdPartyUtil> thirdPartyUtil =
        do_GetService(THIRDPARTYUTIL_CONTRACTID);
      NS_ENSURE_TRUE(thirdPartyUtil, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

      bool isThirdParty;
      rv = thirdPartyUtil->IsThirdPartyWindow(this, nullptr, &isThirdParty);
      NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

      if (isThirdParty) {
        NS_WARNING("IndexedDB is not permitted in a third-party window.");
        *_retval = nullptr;
        return NS_OK;
      }
    }

    
    rv = indexedDB::IDBFactory::Create(this, nullptr,
                                       getter_AddRefs(mIndexedDB));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIIDBFactory> request(mIndexedDB);
  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::GetMozIndexedDB(nsIIDBFactory** _retval)
{
  return GetIndexedDB(_retval);
}





NS_IMETHODIMP
nsGlobalWindow::GetInterface(const nsIID & aIID, void **aSink)
{
  NS_ENSURE_ARG_POINTER(aSink);
  *aSink = nullptr;

  if (aIID.Equals(NS_GET_IID(nsIDocCharset))) {
    FORWARD_TO_OUTER(GetInterface, (aIID, aSink), NS_ERROR_NOT_INITIALIZED);

    NS_WARNING("Using deprecated nsIDocCharset: use nsIDocShell.GetCharset() instead ");
    nsCOMPtr<nsIDocCharset> docCharset(do_QueryInterface(mDocShell));
    docCharset.forget(aSink);
  }
  else if (aIID.Equals(NS_GET_IID(nsIWebNavigation))) {
    FORWARD_TO_OUTER(GetInterface, (aIID, aSink), NS_ERROR_NOT_INITIALIZED);

    nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
    webNav.forget(aSink);
  }
  else if (aIID.Equals(NS_GET_IID(nsIDocShell))) {
    FORWARD_TO_OUTER(GetInterface, (aIID, aSink), NS_ERROR_NOT_INITIALIZED);

    nsCOMPtr<nsIDocShell> docShell = mDocShell;
    docShell.forget(aSink);
  }
#ifdef NS_PRINTING
  else if (aIID.Equals(NS_GET_IID(nsIWebBrowserPrint))) {
    FORWARD_TO_OUTER(GetInterface, (aIID, aSink), NS_ERROR_NOT_INITIALIZED);

    if (mDocShell) {
      nsCOMPtr<nsIContentViewer> viewer;
      mDocShell->GetContentViewer(getter_AddRefs(viewer));
      if (viewer) {
        nsCOMPtr<nsIWebBrowserPrint> webBrowserPrint(do_QueryInterface(viewer));
        webBrowserPrint.forget(aSink);
      }
    }
  }
#endif
  else if (aIID.Equals(NS_GET_IID(nsIDOMWindowUtils))) {
    FORWARD_TO_OUTER(GetInterface, (aIID, aSink), NS_ERROR_NOT_INITIALIZED);

    if (!mWindowUtils) {
      mWindowUtils = new nsDOMWindowUtils(this);
    }

    *aSink = mWindowUtils;
    NS_ADDREF(((nsISupports *) *aSink));
  }
  else if (aIID.Equals(NS_GET_IID(nsILoadContext))) {
    FORWARD_TO_OUTER(GetInterface, (aIID, aSink), NS_ERROR_NOT_INITIALIZED);

    nsCOMPtr<nsILoadContext> loadContext(do_QueryInterface(mDocShell));
    loadContext.forget(aSink);
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
  nsContentUtils::DispatchTrustedEvent(mDoc, eventTarget, name, true, false);
}

class NotifyIdleObserverRunnable : public nsRunnable
{
public:
  NotifyIdleObserverRunnable(nsIIdleObserver* aIdleObserver,
                             uint32_t aTimeInS,
                             bool aCallOnidle,
                             nsGlobalWindow* aIdleWindow)
    : mIdleObserver(aIdleObserver), mTimeInS(aTimeInS), mIdleWindow(aIdleWindow),
      mCallOnidle(aCallOnidle)
  { }

  NS_IMETHOD Run()
  {
    if (mIdleWindow->ContainsIdleObserver(mIdleObserver, mTimeInS)) {
      return mCallOnidle ? mIdleObserver->Onidle() : mIdleObserver->Onactive();
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIIdleObserver> mIdleObserver;
  uint32_t mTimeInS;
  nsRefPtr<nsGlobalWindow> mIdleWindow;

  
  bool mCallOnidle;
};

void
nsGlobalWindow::NotifyIdleObserver(IdleObserverHolder* aIdleObserverHolder,
                                   bool aCallOnidle)
{
  MOZ_ASSERT(aIdleObserverHolder);
  aIdleObserverHolder->mPrevNotificationIdle = aCallOnidle;

  nsCOMPtr<nsIRunnable> caller =
    new NotifyIdleObserverRunnable(aIdleObserverHolder->mIdleObserver,
                                   aIdleObserverHolder->mTimeInS,
                                   aCallOnidle, this);
  if (NS_FAILED(NS_DispatchToCurrentThread(caller))) {
    NS_WARNING("Failed to dispatch thread for idle observer notification.");
  }
}

bool
nsGlobalWindow::ContainsIdleObserver(nsIIdleObserver* aIdleObserver, uint32_t aTimeInS)
{
  MOZ_ASSERT(aIdleObserver, "Idle observer not instantiated.");
  bool found = false;
  nsTObserverArray<IdleObserverHolder>::ForwardIterator iter(mIdleObservers);
  while (iter.HasMore()) {
    IdleObserverHolder& idleObserver = iter.GetNext();
    if (idleObserver.mIdleObserver == aIdleObserver &&
        idleObserver.mTimeInS == aTimeInS) {
      found = true;
      break;
    }
  }
  return found;
}

void
IdleActiveTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsRefPtr<nsGlobalWindow> idleWindow = static_cast<nsGlobalWindow*>(aClosure);
  MOZ_ASSERT(idleWindow, "Idle window has not been instantiated.");
  idleWindow->HandleIdleActiveEvent();
}

void
IdleObserverTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsRefPtr<nsGlobalWindow> idleWindow = static_cast<nsGlobalWindow*>(aClosure);
  MOZ_ASSERT(idleWindow, "Idle window has not been instantiated.");
  idleWindow->HandleIdleObserverCallback();
}

void
nsGlobalWindow::HandleIdleObserverCallback()
{
  MOZ_ASSERT(IsInnerWindow(), "Must be an inner window!");
  MOZ_ASSERT(static_cast<uint32_t>(mIdleCallbackIndex) < mIdleObservers.Length(),
                                  "Idle callback index exceeds array bounds!");
  IdleObserverHolder& idleObserver = mIdleObservers.ElementAt(mIdleCallbackIndex);
  NotifyIdleObserver(&idleObserver, true);
  mIdleCallbackIndex++;
  if (NS_FAILED(ScheduleNextIdleObserverCallback())) {
    NS_WARNING("Failed to set next idle observer callback.");
  }
}

nsresult
nsGlobalWindow::ScheduleNextIdleObserverCallback()
{
  MOZ_ASSERT(IsInnerWindow(), "Must be an inner window!");
  MOZ_ASSERT(mIdleService, "No idle service!");

  if (mIdleCallbackIndex < 0 ||
      static_cast<uint32_t>(mIdleCallbackIndex) >= mIdleObservers.Length()) {
    return NS_OK;
  }

  IdleObserverHolder& idleObserver =
    mIdleObservers.ElementAt(mIdleCallbackIndex);

  uint32_t userIdleTimeMS = 0;
  nsresult rv = mIdleService->GetIdleTime(&userIdleTimeMS);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t callbackTimeMS = 0;
  if (idleObserver.mTimeInS * 1000 + mIdleFuzzFactor > userIdleTimeMS) {
    callbackTimeMS = idleObserver.mTimeInS * 1000 - userIdleTimeMS + mIdleFuzzFactor;
  }

  mIdleTimer->Cancel();
  rv = mIdleTimer->InitWithFuncCallback(IdleObserverTimerCallback,
                                        this,
                                        callbackTimeMS,
                                        nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

uint32_t
nsGlobalWindow::GetFuzzTimeMS()
{
  MOZ_ASSERT(IsInnerWindow(), "Must be an inner window!");

  if (sIdleObserversAPIFuzzTimeDisabled) {
    return 0;
  }

  uint32_t randNum = MAX_IDLE_FUZZ_TIME_MS;
  size_t nbytes = PR_GetRandomNoise(&randNum, sizeof(randNum));
  if (nbytes != sizeof(randNum)) {
    NS_WARNING("PR_GetRandomNoise(...) Not implemented or no available noise!");
    return MAX_IDLE_FUZZ_TIME_MS;
  }

  if (randNum > MAX_IDLE_FUZZ_TIME_MS) {
    randNum %= MAX_IDLE_FUZZ_TIME_MS;
  }

  return randNum;
}

nsresult
nsGlobalWindow::ScheduleActiveTimerCallback()
{
  MOZ_ASSERT(IsInnerWindow(), "Must be an inner window!");

  if (!mAddActiveEventFuzzTime) {
    return HandleIdleActiveEvent();
  }

  MOZ_ASSERT(mIdleTimer);
  mIdleTimer->Cancel();

  uint32_t fuzzFactorInMS = GetFuzzTimeMS();
  nsresult rv = mIdleTimer->InitWithFuncCallback(IdleActiveTimerCallback,
                                                 this,
                                                 fuzzFactorInMS,
                                                 nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
nsGlobalWindow::HandleIdleActiveEvent()
{
  MOZ_ASSERT(IsInnerWindow(), "Must be an inner window!");

  if (mCurrentlyIdle) {
    mIdleCallbackIndex = 0;
    mIdleFuzzFactor = GetFuzzTimeMS();
    nsresult rv = ScheduleNextIdleObserverCallback();
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  mIdleCallbackIndex = -1;
  MOZ_ASSERT(mIdleTimer);
  mIdleTimer->Cancel();
  nsTObserverArray<IdleObserverHolder>::ForwardIterator iter(mIdleObservers);
  while (iter.HasMore()) {
    IdleObserverHolder& idleObserver = iter.GetNext();
    if (idleObserver.mPrevNotificationIdle) {
      NotifyIdleObserver(&idleObserver, false);
    }
  }

  return NS_OK;
}

uint32_t
nsGlobalWindow::FindInsertionIndex(IdleObserverHolder* aIdleObserver)
{
  MOZ_ASSERT(aIdleObserver, "Idle observer not instantiated.");

  uint32_t i = 0;
  nsTObserverArray<IdleObserverHolder>::ForwardIterator iter(mIdleObservers);
  while (iter.HasMore()) {
    IdleObserverHolder& idleObserver = iter.GetNext();
    if (idleObserver.mTimeInS > aIdleObserver->mTimeInS) {
      break;
    }
    i++;
    MOZ_ASSERT(i <= mIdleObservers.Length(), "Array index out of bounds error.");
  }

  return i;
}

nsresult
nsGlobalWindow::RegisterIdleObserver(nsIIdleObserver* aIdleObserver)
{
  MOZ_ASSERT(IsInnerWindow(), "Must be an inner window!");

  nsresult rv;
  if (mIdleObservers.IsEmpty()) {
    mIdleService = do_GetService("@mozilla.org/widget/idleservice;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mIdleService->AddIdleObserver(mObserver, MIN_IDLE_NOTIFICATION_TIME_S);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mIdleTimer) {
      mIdleTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      mIdleTimer->Cancel();
    }
  }

  MOZ_ASSERT(mIdleService);
  MOZ_ASSERT(mIdleTimer);

  IdleObserverHolder tmpIdleObserver;
  tmpIdleObserver.mIdleObserver = aIdleObserver;
  rv = aIdleObserver->GetTime(&tmpIdleObserver.mTimeInS);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_MAX(tmpIdleObserver.mTimeInS, UINT32_MAX / 1000);
  NS_ENSURE_ARG_MIN(tmpIdleObserver.mTimeInS, MIN_IDLE_NOTIFICATION_TIME_S);

  uint32_t insertAtIndex = FindInsertionIndex(&tmpIdleObserver);
  if (insertAtIndex == mIdleObservers.Length()) {
    mIdleObservers.AppendElement(tmpIdleObserver);
  }
  else {
    mIdleObservers.InsertElementAt(insertAtIndex, tmpIdleObserver);
  }

  bool userIsIdle = false;
  rv = nsContentUtils::IsUserIdle(MIN_IDLE_NOTIFICATION_TIME_S, &userIsIdle);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  if (userIsIdle && mIdleCallbackIndex == -1) {
    return NS_OK;
  }

  if (!mCurrentlyIdle) {
    return NS_OK;
  }

  MOZ_ASSERT(mIdleCallbackIndex >= 0);

  if (static_cast<int32_t>(insertAtIndex) < mIdleCallbackIndex) {
    IdleObserverHolder& idleObserver = mIdleObservers.ElementAt(insertAtIndex);
    NotifyIdleObserver(&idleObserver, true);
    mIdleCallbackIndex++;
    return NS_OK;
  }

  if (static_cast<int32_t>(insertAtIndex) == mIdleCallbackIndex) {
    mIdleTimer->Cancel();
    rv = ScheduleNextIdleObserverCallback();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
nsGlobalWindow::FindIndexOfElementToRemove(nsIIdleObserver* aIdleObserver,
                                           int32_t* aRemoveElementIndex)
{
  MOZ_ASSERT(IsInnerWindow(), "Must be an inner window!");
  MOZ_ASSERT(aIdleObserver, "Idle observer not instantiated.");

  *aRemoveElementIndex = 0;
  if (mIdleObservers.IsEmpty()) {
    return NS_ERROR_FAILURE;
  }

  uint32_t aIdleObserverTimeInS;
  nsresult rv = aIdleObserver->GetTime(&aIdleObserverTimeInS);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_MIN(aIdleObserverTimeInS, MIN_IDLE_NOTIFICATION_TIME_S);

  nsTObserverArray<IdleObserverHolder>::ForwardIterator iter(mIdleObservers);
  while (iter.HasMore()) {
    IdleObserverHolder& idleObserver = iter.GetNext();
    if (idleObserver.mTimeInS == aIdleObserverTimeInS &&
        idleObserver.mIdleObserver == aIdleObserver ) {
      break;
    }
    (*aRemoveElementIndex)++;
  }
  return static_cast<uint32_t>(*aRemoveElementIndex) >= mIdleObservers.Length() ?
    NS_ERROR_FAILURE : NS_OK;
}

nsresult
nsGlobalWindow::UnregisterIdleObserver(nsIIdleObserver* aIdleObserver)
{
  MOZ_ASSERT(IsInnerWindow(), "Must be an inner window!");

  int32_t removeElementIndex;
  nsresult rv = FindIndexOfElementToRemove(aIdleObserver, &removeElementIndex);
  if (NS_FAILED(rv)) {
    NS_WARNING("Idle observer not found in list of idle observers. No idle observer removed.");
    return NS_OK;
  }
  mIdleObservers.RemoveElementAt(removeElementIndex);

  MOZ_ASSERT(mIdleTimer);
  if (mIdleObservers.IsEmpty() && mIdleService) {
    rv = mIdleService->RemoveIdleObserver(mObserver, MIN_IDLE_NOTIFICATION_TIME_S);
    NS_ENSURE_SUCCESS(rv, rv);
    mIdleService = nullptr;

    mIdleTimer->Cancel();
    mIdleCallbackIndex = -1;
    return NS_OK;
  }

  if (!mCurrentlyIdle) {
    return NS_OK;
  }

  if (removeElementIndex < mIdleCallbackIndex) {
    mIdleCallbackIndex--;
    return NS_OK;
  }

  if (removeElementIndex != mIdleCallbackIndex) {
    return NS_OK;
  }

  mIdleTimer->Cancel();

  
  
  
  
  
  
  
  
  if (static_cast<uint32_t>(mIdleCallbackIndex) == mIdleObservers.Length()) {
    mIdleCallbackIndex--;
  }
  rv = ScheduleNextIdleObserverCallback();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
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

  if (!nsCRT::strcmp(aTopic, OBSERVER_TOPIC_IDLE)) {
    mCurrentlyIdle = true;
    if (IsFrozen()) {
      
      mNotifyIdleObserversIdleOnThaw = true;
      mNotifyIdleObserversActiveOnThaw = false;
    } else if (mOuterWindow && mOuterWindow->GetCurrentInnerWindow() == this) {
      HandleIdleActiveEvent();
    }
    return NS_OK;
  }

  if (!nsCRT::strcmp(aTopic, OBSERVER_TOPIC_ACTIVE)) {
    mCurrentlyIdle = false;
    if (IsFrozen()) {
      mNotifyIdleObserversActiveOnThaw = true;
      mNotifyIdleObserversIdleOnThaw = false;
    } else if (mOuterWindow && mOuterWindow->GetCurrentInnerWindow() == this) {
      ScheduleActiveTimerCallback();
    }
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

    nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(GetDocShell());
    bool isPrivate = loadContext && loadContext->UsePrivateBrowsing();
    if (pistorage->IsPrivate() != isPrivate) {
      return NS_OK;
    }

    bool fireMozStorageChanged = false;
    principal = GetPrincipal();
    switch (storageType)
    {
    case nsPIDOMStorage::SessionStorage:
    {
      nsCOMPtr<nsIDOMStorage> storage = mSessionStorage;
      if (!storage) {
        nsIDocShell* docShell = GetDocShell();
        if (principal && docShell) {
          
          
          docShell->GetSessionStorageForPrincipal(principal,
                                                  EmptyString(),
                                                  false,
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

      fireMozStorageChanged = SameCOMIdentity(mSessionStorage, changingStorage);
      break;
    }
    case nsPIDOMStorage::LocalStorage:
    {
      
      
      nsIPrincipal *storagePrincipal = pistorage->Principal();
      bool equals;

      rv = storagePrincipal->Equals(principal, &equals);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!equals)
        return NS_OK;

      fireMozStorageChanged = SameCOMIdentity(mLocalStorage, changingStorage);
      break;
    }
    default:
      return NS_OK;
    }

    
    
    rv = CloneStorageEvent(fireMozStorageChanged ?
                           NS_LITERAL_STRING("MozStorageChanged") :
                           NS_LITERAL_STRING("storage"),
                           event);
    NS_ENSURE_SUCCESS(rv, rv);

    event->SetTrusted(true);

    if (fireMozStorageChanged) {
      nsEvent *internalEvent = event->GetInternalNSEvent();
      internalEvent->mFlags.mOnlyChromeDispatch = true;
    }

    if (IsFrozen()) {
      
      
      

      mPendingStorageEvents.AppendObject(event);
      return NS_OK;
    }

    bool defaultActionEnabled;
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

#ifdef MOZ_B2G
  if (!nsCRT::strcmp(aTopic, NS_NETWORK_ACTIVITY_BLIP_UPLOAD_TOPIC) ||
      !nsCRT::strcmp(aTopic, NS_NETWORK_ACTIVITY_BLIP_DOWNLOAD_TOPIC)) {
    nsCOMPtr<nsIDOMEvent> event;
    NS_NewDOMEvent(getter_AddRefs(event), this, nullptr, nullptr);
    nsresult rv = event->InitEvent(
      !nsCRT::strcmp(aTopic, NS_NETWORK_ACTIVITY_BLIP_UPLOAD_TOPIC)
        ? NETWORK_UPLOAD_EVENT_NAME
        : NETWORK_DOWNLOAD_EVENT_NAME,
      false, false);
    NS_ENSURE_SUCCESS(rv, rv);

    event->SetTrusted(true);

    bool dummy;
    return DispatchEvent(event, &dummy);
  }
#endif 

  NS_WARNING("unrecognized topic in nsGlobalWindow::Observe");
  return NS_ERROR_FAILURE;
}

nsresult
nsGlobalWindow::CloneStorageEvent(const nsAString& aType,
                                  nsCOMPtr<nsIDOMStorageEvent>& aEvent)
{
  nsresult rv;

  bool canBubble;
  bool cancelable;
  nsAutoString key;
  nsAutoString oldValue;
  nsAutoString newValue;
  nsAutoString url;
  nsCOMPtr<nsIDOMStorage> storageArea;

  nsCOMPtr<nsIDOMEvent> domEvent = do_QueryInterface(aEvent, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  domEvent->GetBubbles(&canBubble);
  domEvent->GetCancelable(&cancelable);

  aEvent->GetKey(key);
  aEvent->GetOldValue(oldValue);
  aEvent->GetNewValue(newValue);
  aEvent->GetUrl(url);
  aEvent->GetStorageArea(getter_AddRefs(storageArea));

  NS_NewDOMStorageEvent(getter_AddRefs(domEvent), this, nullptr, nullptr);
  aEvent = do_QueryInterface(domEvent);
  return aEvent->InitStorageEvent(aType, canBubble, cancelable,
                                  key, oldValue, newValue,
                                  url, storageArea);
}

nsresult
nsGlobalWindow::FireDelayedDOMEvents()
{
  FORWARD_TO_INNER(FireDelayedDOMEvents, (), NS_ERROR_UNEXPECTED);

  for (int32_t i = 0; i < mPendingStorageEvents.Count(); ++i) {
    Observe(mPendingStorageEvents[i], "dom-storage2-changed", nullptr);
  }

  if (mApplicationCache) {
    static_cast<nsDOMOfflineResourceList*>(mApplicationCache.get())->FirePendingEvents();
  }

  if (mFireOfflineStatusChangeEventOnThaw) {
    mFireOfflineStatusChangeEventOnThaw = false;
    FireOfflineStatusEvent();
  }

  if (mNotifyIdleObserversIdleOnThaw) {
    mNotifyIdleObserversIdleOnThaw = false;
    HandleIdleActiveEvent();
  }

  if (mNotifyIdleObserversActiveOnThaw) {
    mNotifyIdleObserversActiveOnThaw = false;
    ScheduleActiveTimerCallback();
  }

  nsCOMPtr<nsIDocShell> docShell = GetDocShell();
  if (docShell) {
    int32_t childCount = 0;
    docShell->GetChildCount(&childCount);

    for (int32_t i = 0; i < childCount; ++i) {
      nsCOMPtr<nsIDocShellTreeItem> childShell;
      docShell->GetChildAt(i, getter_AddRefs(childShell));
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





nsIDOMWindow *
nsGlobalWindow::GetParentInternal()
{
  FORWARD_TO_OUTER(GetParentInternal, (), nullptr);

  nsCOMPtr<nsIDOMWindow> parent;
  GetParent(getter_AddRefs(parent));

  if (parent && parent != static_cast<nsIDOMWindow *>(this)) {
    return parent;
  }

  return NULL;
}


void
nsGlobalWindow::CloseBlockScriptTerminationFunc(nsISupports *aRef)
{
  nsGlobalWindow* pwin = static_cast<nsGlobalWindow*>
                                    (static_cast<nsPIDOMWindow*>(aRef));
  pwin->mBlockScriptedClosingFlag = false;
}

nsresult
nsGlobalWindow::OpenInternal(const nsAString& aUrl, const nsAString& aName,
                             const nsAString& aOptions, bool aDialog,
                             bool aContentModal, bool aCalledNoScript,
                             bool aDoJSFixups, bool aNavigate,
                             nsIArray *argv,
                             nsISupports *aExtraArgument,
                             nsIPrincipal *aCalleePrincipal,
                             JSContext *aJSCallerContext,
                             nsIDOMWindow **aReturn)
{
  FORWARD_TO_OUTER(OpenInternal, (aUrl, aName, aOptions, aDialog,
                                  aContentModal, aCalledNoScript, aDoJSFixups,
                                  aNavigate, argv, aExtraArgument,
                                  aCalleePrincipal, aJSCallerContext, aReturn),
                   NS_ERROR_NOT_INITIALIZED);

#ifdef DEBUG
  uint32_t argc = 0;
  if (argv)
      argv->GetLength(&argc);
#endif
  NS_PRECONDITION(!aExtraArgument || (!argv && argc == 0),
                  "Can't pass in arguments both ways");
  NS_PRECONDITION(!aCalledNoScript || (!argv && argc == 0),
                  "Can't pass JS args when called via the noscript methods");
  NS_PRECONDITION(!aJSCallerContext || !aCalledNoScript,
                  "Shouldn't have caller context when called noscript");

  
  MOZ_ASSERT(aCalledNoScript || aNavigate);

  *aReturn = nullptr;

  nsCOMPtr<nsIWebBrowserChrome> chrome;
  GetWebBrowserChrome(getter_AddRefs(chrome));
  if (!chrome) {
    
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ASSERTION(mDocShell, "Must have docshell here");

  
  bool isApp = false;
  if (mDoc) {
    isApp = mDoc->NodePrincipal()->GetAppStatus() >=
              nsIPrincipal::APP_STATUS_INSTALLED;
  }

  const bool checkForPopup = !nsContentUtils::IsCallerChrome() &&
    !isApp && !aDialog && !WindowExists(aName, !aCalledNoScript);

  
  
  
  nsXPIDLCString url;
  nsresult rv = NS_OK;

  
  
  
  if (!aUrl.IsEmpty()) {
    AppendUTF16toUTF8(aUrl, url);

    
    
    
    
    
    
    if (url.get() && !aDialog && aNavigate)
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
          mBlockScriptedClosingFlag = true;
          mContext->SetTerminationFunction(CloseBlockScriptTerminationFunc,
                                           this);
        }
      }

      FireAbuseEvents(true, false, aUrl, aName, aOptions);
      return aDoJSFixups ? NS_OK : NS_ERROR_FAILURE;
    }
  }    

  nsCOMPtr<nsIDOMWindow> domReturn;

  nsCOMPtr<nsIWindowWatcher> wwatch =
    do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  NS_ENSURE_TRUE(wwatch, rv);

  NS_ConvertUTF16toUTF8 options(aOptions);
  NS_ConvertUTF16toUTF8 name(aName);

  const char *options_ptr = aOptions.IsEmpty() ? nullptr : options.get();
  const char *name_ptr = aName.IsEmpty() ? nullptr : name.get();

  nsCOMPtr<nsPIWindowWatcher> pwwatch(do_QueryInterface(wwatch));
  NS_ENSURE_STATE(pwwatch);

  {
    
    
    
    nsAutoPopupStatePusher popupStatePusher(openAbused, true);

    if (!aCalledNoScript) {
      
      
      rv = pwwatch->OpenWindow2(this, url.get(), name_ptr, options_ptr,
                                 true,
                                aDialog, aNavigate, argv,
                                getter_AddRefs(domReturn));
    } else {
      
      
      
      
      nsCOMPtr<nsIJSContextStack> stack;

      if (!aContentModal) {
        stack = do_GetService(sJSStackContractID);
      }

      if (stack) {
        rv = stack->Push(nullptr);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = pwwatch->OpenWindow2(this, url.get(), name_ptr, options_ptr,
                                 false,
                                aDialog, aNavigate, aExtraArgument,
                                getter_AddRefs(domReturn));

      if (stack) {
        JSContext* cx;
        stack->Pop(&cx);
        NS_ASSERTION(!cx, "Unexpected JSContext popped!");
      }
    }
  }

  NS_ENSURE_SUCCESS(rv, rv);

  

  NS_ENSURE_TRUE(domReturn, NS_OK);
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
        opened->SetPopupSpamWindow(true);
        ++gOpenPopupSpamCount;
      }
    }
    if (abuseLevel >= openAbused)
      FireAbuseEvents(false, true, aUrl, aName, aOptions);
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





uint32_t sNestingLevel;

nsresult
nsGlobalWindow::SetTimeoutOrInterval(nsIScriptTimeoutHandler *aHandler,
                                     int32_t interval,
                                     bool aIsInterval, int32_t *aReturn)
{
  FORWARD_TO_INNER(SetTimeoutOrInterval, (aHandler, interval, aIsInterval, aReturn),
                   NS_ERROR_NOT_INITIALIZED);

  
  
  if (!mDocument) {
    return NS_OK;
  }

  
  
  interval = std::max(aIsInterval ? 1 : 0, interval);

  
  
  
  uint32_t maxTimeoutMs = PR_IntervalToMilliseconds(DOM_MAX_TIMEOUT_VALUE);
  if (static_cast<uint32_t>(interval) > maxTimeoutMs) {
    interval = maxTimeoutMs;
  }

  nsRefPtr<nsTimeout> timeout = new nsTimeout();
  timeout->mIsInterval = aIsInterval;
  timeout->mInterval = interval;
  timeout->mScriptHandler = aHandler;

  
  uint32_t nestingLevel = sNestingLevel + 1;
  uint32_t realInterval = interval;
  if (aIsInterval || nestingLevel >= DOM_CLAMP_TIMEOUT_NESTING_LEVEL) {
    
    
    realInterval = std::max(realInterval, uint32_t(DOMMinTimeoutValue()));
  }

  
  
  
  

  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsresult rv;
  rv = nsContentUtils::GetSecurityManager()->
    GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  bool subsumes = false;
  nsCOMPtr<nsIPrincipal> ourPrincipal = GetPrincipal();

  
  
  
  
  rv = ourPrincipal->Subsumes(subjectPrincipal, &subsumes);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  if (subsumes) {
    timeout->mPrincipal = subjectPrincipal;
  } else {
    timeout->mPrincipal = ourPrincipal;
  }

  ++gTimeoutsRecentlySet;
  TimeDuration delta = TimeDuration::FromMilliseconds(realInterval);

  if (!IsFrozen() && !mTimeoutsSuspendDepth) {
    
    
    

    timeout->mWhen = TimeStamp::Now() + delta;

    timeout->mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_FAILED(rv)) {
      return rv;
    }

    nsRefPtr<nsTimeout> copy = timeout;

    rv = timeout->InitTimer(TimerCallback, realInterval);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    copy.forget();
  } else {
    
    
    
    
    

    timeout->mTimeRemaining = delta;
  }

  timeout->mWindow = this;

  if (!aIsInterval) {
    timeout->mNestingLevel = nestingLevel;
  }

  
  timeout->mPopupState = openAbused;

  if (gRunningTimeoutDepth == 0 && gPopupControlState < openAbused) {
    
    
    
    

    int32_t delay =
      Preferences::GetInt("dom.disable_open_click_delay");

    
    
    
    if (interval <= delay) {
      timeout->mPopupState = gPopupControlState;
    }
  }

  InsertTimeoutIntoList(timeout);

  timeout->mPublicId = ++mTimeoutPublicIdCounter;
  *aReturn = timeout->mPublicId;

  return NS_OK;

}

nsresult
nsGlobalWindow::SetTimeoutOrInterval(bool aIsInterval, int32_t *aReturn)
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

  int32_t interval = 0;
  bool isInterval = aIsInterval;
  nsCOMPtr<nsIScriptTimeoutHandler> handler;
  nsresult rv = NS_CreateJSTimeoutHandler(this,
                                          &isInterval,
                                          &interval,
                                          getter_AddRefs(handler));
  if (NS_FAILED(rv))
    return (rv == NS_ERROR_DOM_TYPE_ERR) ? NS_OK : rv;

  return SetTimeoutOrInterval(handler, interval, isInterval, aReturn);
}

bool
nsGlobalWindow::RunTimeoutHandler(nsTimeout* aTimeout,
                                  nsIScriptContext* aScx)
{
  
  
  nsRefPtr<nsTimeout> timeout = aTimeout;
  nsTimeout* last_running_timeout = mRunningTimeout;
  mRunningTimeout = timeout;
  timeout->mRunning = true;

  
  
  
  
  nsAutoPopupStatePusher popupStatePusher(timeout->mPopupState);

  
  
  timeout->mPopupState = openAbused;

  ++gRunningTimeoutDepth;
  ++mTimeoutFiringDepth;

  bool trackNestingLevel = !timeout->mIsInterval;
  uint32_t nestingLevel;
  if (trackNestingLevel) {
    nestingLevel = sNestingLevel;
    sNestingLevel = timeout->mNestingLevel;
  }

  nsCOMPtr<nsIScriptTimeoutHandler> handler(timeout->mScriptHandler);
  JSObject* scriptObject = handler->GetScriptObject();
  if (!scriptObject) {
    
    const PRUnichar* script = handler->GetHandlerText();
    NS_ASSERTION(script, "timeout has no script nor handler text!");

    const char* filename = nullptr;
    uint32_t lineNo = 0;
    handler->GetLocation(&filename, &lineNo);

    AutoPushJSContext cx(aScx->GetNativeContext());
    JS::CompileOptions options(cx);
    options.setFileAndLine(filename, lineNo)
           .setVersion(JSVERSION_DEFAULT);
    aScx->EvaluateString(nsDependentString(script), *FastGetGlobalJSObject(),
                         options,  false, nullptr);
  } else {
    nsCOMPtr<nsIVariant> dummy;
    nsCOMPtr<nsISupports> me(static_cast<nsIDOMWindow *>(this));
    aScx->CallEventHandler(me, FastGetGlobalJSObject(),
                           scriptObject, handler->GetArgv(),
                           
                           
                           getter_AddRefs(dummy));

  }

  
  
  
  
  
  
  
  

  if (trackNestingLevel) {
    sNestingLevel = nestingLevel;
  }

  --mTimeoutFiringDepth;
  --gRunningTimeoutDepth;

  mRunningTimeout = last_running_timeout;
  timeout->mRunning = false;
  return timeout->mCleared;
}

bool
nsGlobalWindow::RescheduleTimeout(nsTimeout* aTimeout, const TimeStamp& now,
                                  bool aRunningPendingTimeouts)
{
  if (!aTimeout->mIsInterval) {
    if (aTimeout->mTimer) {
      
      
      
      aTimeout->mTimer->Cancel();
      aTimeout->mTimer = nullptr;
      aTimeout->Release();
    }
    return false;
  }

  
  
  TimeDuration nextInterval =
    TimeDuration::FromMilliseconds(std::max(aTimeout->mInterval,
                                          uint32_t(DOMMinTimeoutValue())));

  
  
  
  TimeStamp firingTime;
  if (aRunningPendingTimeouts) {
    firingTime = now + nextInterval;
  } else {
    firingTime = aTimeout->mWhen + nextInterval;
  }

  TimeStamp currentNow = TimeStamp::Now();
  TimeDuration delay = firingTime - currentNow;

  
  
  
  if (delay < TimeDuration(0)) {
    delay = TimeDuration(0);
  }

  if (!aTimeout->mTimer) {
    NS_ASSERTION(IsFrozen() || mTimeoutsSuspendDepth,
                 "How'd our timer end up null if we're not frozen or "
                 "suspended?");

    aTimeout->mTimeRemaining = delay;
    return true;
  }

  aTimeout->mWhen = currentNow + delay;

  
  
  nsresult rv = aTimeout->InitTimer(TimerCallback, delay.ToMilliseconds());

  if (NS_FAILED(rv)) {
    NS_ERROR("Error initializing timer for DOM timeout!");

    
    
    
    
    
    
    aTimeout->mTimer->Cancel();
    aTimeout->mTimer = nullptr;

    
    
    aTimeout->Release();

    return false;
  }

  return true;
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
  uint32_t firingDepth = mTimeoutFiringDepth + 1;

  
  
  nsCOMPtr<nsIScriptGlobalObject> windowKungFuDeathGrip(this);

  
  
  TimeStamp now = TimeStamp::Now();
  TimeStamp deadline;

  if (aTimeout && aTimeout->mWhen > now) {
    
    
    
    
    

    deadline = aTimeout->mWhen;
  } else {
    deadline = now;
  }

  
  
  
  
  last_expired_timeout = nullptr;
  for (timeout = mTimeouts.getFirst(); timeout; timeout = timeout->getNext()) {
    if (((timeout == aTimeout) || (timeout->mWhen <= deadline)) &&
        (timeout->mFiringDepth == 0)) {
      
      
      timeout->mFiringDepth = firingDepth;
      last_expired_timeout = timeout;
    }
  }

  
  
  
  if (!last_expired_timeout) {
    return;
  }

  
  TimeDuration recordingInterval = TimeDuration::FromMilliseconds(STATISTICS_INTERVAL);
  if (gLastRecordedRecentTimeouts.IsNull() ||
      now - gLastRecordedRecentTimeouts > recordingInterval) {
    uint32_t count = gTimeoutsRecentlySet;
    gTimeoutsRecentlySet = 0;
    Telemetry::Accumulate(Telemetry::DOM_TIMERS_RECENTLY_SET, count);
    gLastRecordedRecentTimeouts = now;
  }

  
  
  
  
  
  dummy_timeout.mFiringDepth = firingDepth;
  dummy_timeout.mWhen = now;
  last_expired_timeout->setNext(&dummy_timeout);

  
  
  dummy_timeout.AddRef();
  dummy_timeout.AddRef();

  last_insertion_point = mTimeoutInsertionPoint;
  
  
  mTimeoutInsertionPoint = &dummy_timeout;

  Telemetry::AutoCounter<Telemetry::DOM_TIMERS_FIRED_PER_NATIVE_TIMEOUT> timeoutsRan;

  for (timeout = mTimeouts.getFirst();
       timeout != &dummy_timeout && !IsFrozen();
       timeout = nextTimeout) {
    nextTimeout = timeout->getNext();

    if (timeout->mFiringDepth != firingDepth) {
      
      

      continue;
    }

    if (mTimeoutsSuspendDepth) {
      
      
      timeout->mFiringDepth = 0;
      continue;
    }

    
    

    
    
    nsCOMPtr<nsIScriptContext> scx = GetContextInternal();

    if (!scx) {
      
      
      continue;
    }

    
    
    
    if (!scx->GetScriptsEnabled()) {
      
      
      
      
      
      
      
      continue;
    }

    
    ++timeoutsRan;
    bool timeout_was_cleared = RunTimeoutHandler(timeout, scx);

    if (timeout_was_cleared) {
      
      
      
      

      mTimeoutInsertionPoint = last_insertion_point;

      return;
    }

    
    
    bool needsReinsertion = RescheduleTimeout(timeout, now, !aTimeout);

    
    
    nextTimeout = timeout->getNext();

    timeout->remove();

    if (needsReinsertion) {
      
      
      InsertTimeoutIntoList(timeout);
    }

    
    timeout->Release();
  }

  
  dummy_timeout.remove();

  mTimeoutInsertionPoint = last_insertion_point;
}

nsrefcnt
nsTimeout::Release()
{
  if (--mRefCnt > 0)
    return mRefCnt;

  

  
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
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
nsGlobalWindow::ClearTimeoutOrInterval(int32_t aTimerID)
{
  FORWARD_TO_INNER(ClearTimeoutOrInterval, (aTimerID), NS_ERROR_NOT_INITIALIZED);

  uint32_t public_id = (uint32_t)aTimerID;
  nsTimeout *timeout;

  for (timeout = mTimeouts.getFirst(); timeout; timeout = timeout->getNext()) {
    if (timeout->mPublicId == public_id) {
      if (timeout->mRunning) {
        


        timeout->mIsInterval = false;
      }
      else {
        
        timeout->remove();

        if (timeout->mTimer) {
          timeout->mTimer->Cancel();
          timeout->mTimer = nullptr;
          timeout->Release();
        }
        timeout->Release();
      }
      break;
    }
  }

  return NS_OK;
}

nsresult nsGlobalWindow::ResetTimersForNonBackgroundWindow()
{
  FORWARD_TO_INNER(ResetTimersForNonBackgroundWindow, (),
                   NS_ERROR_NOT_INITIALIZED);

  if (IsFrozen() || mTimeoutsSuspendDepth) {
    return NS_OK;
  }

  TimeStamp now = TimeStamp::Now();

  
  
  
  
  
  
  
  for (nsTimeout *timeout = mTimeoutInsertionPoint ?
         mTimeoutInsertionPoint->getNext() : mTimeouts.getFirst();
       timeout; ) {
    
    
    
    if (timeout->mWhen <= now) {
      timeout = timeout->getNext();
      continue;
    }

    if (timeout->mWhen - now >
        TimeDuration::FromMilliseconds(gMinBackgroundTimeoutValue)) {
      
      
      
      break;
    }

    
    
    
    TimeDuration interval =
      TimeDuration::FromMilliseconds(std::max(timeout->mInterval,
                                            uint32_t(DOMMinTimeoutValue())));
    uint32_t oldIntervalMillisecs = 0;
    timeout->mTimer->GetDelay(&oldIntervalMillisecs);
    TimeDuration oldInterval = TimeDuration::FromMilliseconds(oldIntervalMillisecs);
    if (oldInterval > interval) {
      
      TimeStamp firingTime =
        std::max(timeout->mWhen - oldInterval + interval, now);

      NS_ASSERTION(firingTime < timeout->mWhen,
                   "Our firing time should strictly decrease!");

      TimeDuration delay = firingTime - now;
      timeout->mWhen = firingTime;

      
      
      
      
      
      nsTimeout* nextTimeout = timeout->getNext();

      
      
      
      NS_ASSERTION(!nextTimeout ||
                   timeout->mWhen < nextTimeout->mWhen, "How did that happen?");
      timeout->remove();
      
      
      uint32_t firingDepth = timeout->mFiringDepth;
      InsertTimeoutIntoList(timeout);
      timeout->mFiringDepth = firingDepth;
      timeout->Release();

      nsresult rv = timeout->InitTimer(TimerCallback, delay.ToMilliseconds());

      if (NS_FAILED(rv)) {
        NS_WARNING("Error resetting non background timer for DOM timeout!");
        return rv;
      }

      timeout = nextTimeout;
    } else {
      timeout = timeout->getNext();
    }
  }

  return NS_OK;
}

void
nsGlobalWindow::ClearAllTimeouts()
{
  nsTimeout *timeout, *nextTimeout;

  for (timeout = mTimeouts.getFirst(); timeout; timeout = nextTimeout) {
    




    if (mRunningTimeout == timeout)
      mTimeoutInsertionPoint = nullptr;

    nextTimeout = timeout->getNext();

    if (timeout->mTimer) {
      timeout->mTimer->Cancel();
      timeout->mTimer = nullptr;

      
      
      timeout->Release();
    }

    
    
    timeout->mCleared = true;

    
    timeout->Release();
  }

  
  mTimeouts.clear();
}

void
nsGlobalWindow::InsertTimeoutIntoList(nsTimeout *aTimeout)
{
  NS_ASSERTION(IsInnerWindow(),
               "InsertTimeoutIntoList() called on outer window!");

  
  
  
  nsTimeout* prevSibling;
  for (prevSibling = mTimeouts.getLast();
       prevSibling && prevSibling != mTimeoutInsertionPoint &&
         
         
         ((IsFrozen() || mTimeoutsSuspendDepth) ?
          prevSibling->mTimeRemaining > aTimeout->mTimeRemaining :
          prevSibling->mWhen > aTimeout->mWhen);
       prevSibling = prevSibling->getPrevious()) {
    
  }

  
  if (prevSibling) {
    prevSibling->setNext(aTimeout);
  } else {
    mTimeouts.insertFront(aTimeout);
  }

  aTimeout->mFiringDepth = 0;

  
  
  aTimeout->AddRef();
}


void
nsGlobalWindow::TimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsRefPtr<nsTimeout> timeout = (nsTimeout *)aClosure;

  timeout->mWindow->RunTimeout(timeout);
}





nsresult
nsGlobalWindow::GetTreeOwner(nsIDocShellTreeOwner **aTreeOwner)
{
  FORWARD_TO_OUTER(GetTreeOwner, (aTreeOwner), NS_ERROR_NOT_INITIALIZED);

  
  

  if (!mDocShell) {
    *aTreeOwner = nullptr;

    return NS_OK;
  }

  return mDocShell->GetTreeOwner(aTreeOwner);
}

nsresult
nsGlobalWindow::GetTreeOwner(nsIBaseWindow **aTreeOwner)
{
  FORWARD_TO_OUTER(GetTreeOwner, (aTreeOwner), NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;

  
  

  if (mDocShell) {
    mDocShell->GetTreeOwner(getter_AddRefs(treeOwner));
  }

  if (!treeOwner) {
    *aTreeOwner = nullptr;
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
  FORWARD_TO_OUTER(GetScrollFrame, (), nullptr);

  if (!mDocShell) {
    return nullptr;
  }

  nsCOMPtr<nsIPresShell> presShell = mDocShell->GetPresShell();
  if (presShell) {
    return presShell->GetRootScrollFrameAsScrollable();
  }
  return nullptr;
}

nsresult
nsGlobalWindow::BuildURIfromBase(const char *aURL, nsIURI **aBuiltURI,
                                 bool *aFreeSecurityPass,
                                 JSContext **aCXused)
{
  nsIScriptContext *scx = GetContextInternal();
  JSContext *cx = nullptr;

  *aBuiltURI = nullptr;
  *aFreeSecurityPass = false;
  if (aCXused)
    *aCXused = nullptr;

  
  NS_ASSERTION(scx, "opening window missing its context");
  NS_ASSERTION(mDocument, "opening window missing its document");
  if (!scx || !mDocument)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMChromeWindow> chrome_win = do_QueryObject(this);

  if (nsContentUtils::IsCallerChrome() && !chrome_win) {
    
    
    
    
    

    cx = scx->GetNativeContext();
  } else {
    
    nsCOMPtr<nsIThreadJSContextStack> stack(do_GetService(sJSStackContractID));
    if (stack)
      stack->Peek(&cx);
  }

  


  nsAutoCString charset(NS_LITERAL_CSTRING("UTF-8")); 
  nsIURI* baseURI = nullptr;
  nsCOMPtr<nsIURI> uriToLoad;
  nsCOMPtr<nsIDOMWindow> sourceWindow;

  if (cx) {
    nsIScriptContext *scriptcx = nsJSUtils::GetDynamicScriptContext(cx);
    if (scriptcx)
      sourceWindow = do_QueryInterface(scriptcx->GetGlobalObject());
  }

  if (!sourceWindow) {
    sourceWindow = do_QueryInterface(NS_ISUPPORTS_CAST(nsIDOMWindow *, this));
    *aFreeSecurityPass = true;
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
  bool             freePass;
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

already_AddRefed<nsISupports>
nsGlobalWindow::SaveWindowState()
{
  NS_PRECONDITION(IsOuterWindow(), "Can't save the inner window's state");

  if (!mContext || !mJSObject) {
    
    return nullptr;
  }

  nsGlobalWindow *inner = GetCurrentInnerWindowInternal();
  NS_ASSERTION(inner, "No inner window to save");

  
  
  
  
  
  inner->Freeze();

  nsCOMPtr<nsISupports> state = new WindowStateHolder(inner,
                                                      mInnerWindowHolder);

#ifdef DEBUG_PAGE_CACHE
  printf("saving window state, state = %p\n", (void*)state);
#endif

  return state.forget();
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
nsGlobalWindow::SuspendTimeouts(uint32_t aIncrease,
                                bool aFreezeChildren)
{
  FORWARD_TO_INNER_VOID(SuspendTimeouts, (aIncrease, aFreezeChildren));

  bool suspended = (mTimeoutsSuspendDepth != 0);
  mTimeoutsSuspendDepth += aIncrease;

  if (!suspended) {
    nsCOMPtr<nsIDeviceSensors> ac = do_GetService(NS_DEVICE_SENSORS_CONTRACTID);
    if (ac) {
      for (uint32_t i = 0; i < mEnabledSensors.Length(); i++)
        ac->RemoveWindowListener(mEnabledSensors[i], this);
    }
    DisableGamepadUpdates();

    
    nsIScriptContext *scx = GetContextInternal();
    AutoPushJSContext cx(scx ? scx->GetNativeContext() : nullptr);
    mozilla::dom::workers::SuspendWorkersForWindow(cx, this);

    TimeStamp now = TimeStamp::Now();
    for (nsTimeout *t = mTimeouts.getFirst(); t; t = t->getNext()) {
      
      if (t->mWhen > now)
        t->mTimeRemaining = t->mWhen - now;
      else
        t->mTimeRemaining = TimeDuration(0);
  
      
      if (t->mTimer) {
        t->mTimer->Cancel();
        t->mTimer = nullptr;
  
        
        
        
        
        t->Release();
      }
    }

    
    for (uint32_t i = 0; i < mAudioContexts.Length(); ++i) {
      mAudioContexts[i]->Suspend();
    }
  }

  
  nsCOMPtr<nsIDocShell> docShell = GetDocShell();
  if (docShell) {
    int32_t childCount = 0;
    docShell->GetChildCount(&childCount);

    for (int32_t i = 0; i < childCount; ++i) {
      nsCOMPtr<nsIDocShellTreeItem> childShell;
      docShell->GetChildAt(i, getter_AddRefs(childShell));
      NS_ASSERTION(childShell, "null child shell");

      nsCOMPtr<nsPIDOMWindow> pWin = do_GetInterface(childShell);
      if (pWin) {
        nsGlobalWindow *win =
          static_cast<nsGlobalWindow*>
                     (static_cast<nsPIDOMWindow*>(pWin));
        NS_ASSERTION(win->IsOuterWindow(), "Expected outer window");
        nsGlobalWindow* inner = win->GetCurrentInnerWindowInternal();

        
        
        nsCOMPtr<nsIContent> frame = do_QueryInterface(pWin->GetFrameElementInternal());
        if (!mDoc || !frame || mDoc != frame->OwnerDoc() || !inner) {
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
nsGlobalWindow::ResumeTimeouts(bool aThawChildren)
{
  FORWARD_TO_INNER(ResumeTimeouts, (), NS_ERROR_NOT_INITIALIZED);

  NS_ASSERTION(mTimeoutsSuspendDepth, "Mismatched calls to ResumeTimeouts!");
  --mTimeoutsSuspendDepth;
  bool shouldResume = (mTimeoutsSuspendDepth == 0);
  nsresult rv;

  if (shouldResume) {
    nsCOMPtr<nsIDeviceSensors> ac = do_GetService(NS_DEVICE_SENSORS_CONTRACTID);
    if (ac) {
      for (uint32_t i = 0; i < mEnabledSensors.Length(); i++)
        ac->AddWindowListener(mEnabledSensors[i], this);
    }
    EnableGamepadUpdates();

    
    for (uint32_t i = 0; i < mAudioContexts.Length(); ++i) {
      mAudioContexts[i]->Resume();
    }

    
    nsIScriptContext *scx = GetContextInternal();
    AutoPushJSContext cx(scx ? scx->GetNativeContext() : nullptr);
    mozilla::dom::workers::ResumeWorkersForWindow(cx, this);

    
    

    TimeStamp now = TimeStamp::Now();

#ifdef DEBUG
    bool _seenDummyTimeout = false;
#endif

    for (nsTimeout *t = mTimeouts.getFirst(); t; t = t->getNext()) {
      
      
      
      if (!t->mWindow) {
#ifdef DEBUG
        NS_ASSERTION(!_seenDummyTimeout, "More than one dummy timeout?!");
        _seenDummyTimeout = true;
#endif
        continue;
      }

      
      
      
      uint32_t delay =
        std::max(int32_t(t->mTimeRemaining.ToMilliseconds()),
               DOMMinTimeoutValue());

      
      
      t->mWhen = now + t->mTimeRemaining;

      t->mTimer = do_CreateInstance("@mozilla.org/timer;1");
      NS_ENSURE_TRUE(t->mTimer, NS_ERROR_OUT_OF_MEMORY);

      rv = t->InitTimer(TimerCallback, delay);
      if (NS_FAILED(rv)) {
        t->mTimer = nullptr;
        return rv;
      }

      
      t->AddRef();
    }
  }

  
  nsCOMPtr<nsIDocShell> docShell = GetDocShell();
  if (docShell) {
    int32_t childCount = 0;
    docShell->GetChildCount(&childCount);

    for (int32_t i = 0; i < childCount; ++i) {
      nsCOMPtr<nsIDocShellTreeItem> childShell;
      docShell->GetChildAt(i, getter_AddRefs(childShell));
      NS_ASSERTION(childShell, "null child shell");

      nsCOMPtr<nsPIDOMWindow> pWin = do_GetInterface(childShell);
      if (pWin) {
        nsGlobalWindow *win =
          static_cast<nsGlobalWindow*>
                     (static_cast<nsPIDOMWindow*>(pWin));

        NS_ASSERTION(win->IsOuterWindow(), "Expected outer window");
        nsGlobalWindow* inner = win->GetCurrentInnerWindowInternal();

        
        
        nsCOMPtr<nsIContent> frame = do_QueryInterface(pWin->GetFrameElementInternal());
        if (!mDoc || !frame || mDoc != frame->OwnerDoc() || !inner) {
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

uint32_t
nsGlobalWindow::TimeoutSuspendCount()
{
  FORWARD_TO_INNER(TimeoutSuspendCount, (), 0);
  return mTimeoutsSuspendDepth;
}

void
nsGlobalWindow::EnableDeviceSensor(uint32_t aType)
{
  bool alreadyEnabled = false;
  for (uint32_t i = 0; i < mEnabledSensors.Length(); i++) {
    if (mEnabledSensors[i] == aType) {
      alreadyEnabled = true;
      break;
    }
  }

  mEnabledSensors.AppendElement(aType);

  if (alreadyEnabled) {
    return;
  }

  nsCOMPtr<nsIDeviceSensors> ac = do_GetService(NS_DEVICE_SENSORS_CONTRACTID);
  if (ac) {
    ac->AddWindowListener(aType, this);
  }
}

void
nsGlobalWindow::DisableDeviceSensor(uint32_t aType)
{
  int32_t doomedElement = -1;
  int32_t listenerCount = 0;
  for (uint32_t i = 0; i < mEnabledSensors.Length(); i++) {
    if (mEnabledSensors[i] == aType) {
      doomedElement = i;
      listenerCount++;
    }
  }

  if (doomedElement == -1) {
    return;
  }

  mEnabledSensors.RemoveElementAt(doomedElement);

  if (listenerCount > 1) {
    return;
  }

  nsCOMPtr<nsIDeviceSensors> ac = do_GetService(NS_DEVICE_SENSORS_CONTRACTID);
  if (ac) {
    ac->RemoveWindowListener(aType, this);
  }
}

void
nsGlobalWindow::SetHasGamepadEventListener(bool aHasGamepad)
{
  FORWARD_TO_INNER_VOID(SetHasGamepadEventListener, (aHasGamepad));
  mHasGamepad = aHasGamepad;
  if (aHasGamepad) {
    EnableGamepadUpdates();
  }
}

void
nsGlobalWindow::EnableTimeChangeNotifications()
{
  nsSystemTimeChangeObserver::AddWindowListener(this);
}

void
nsGlobalWindow::DisableTimeChangeNotifications()
{
  nsSystemTimeChangeObserver::RemoveWindowListener(this);
}


bool
nsGlobalWindow::HasIndexedDBSupport()
{
  return Preferences::GetBool("indexedDB.feature.enabled", true);
}


bool
nsPIDOMWindow::HasPerformanceSupport()
{
  return Preferences::GetBool("dom.enable_performance", false);
}

static size_t
SizeOfEventTargetObjectsEntryExcludingThisFun(
  nsPtrHashKey<nsDOMEventTargetHelper> *aEntry,
  nsMallocSizeOfFun aMallocSizeOf,
  void *arg)
{
  nsISupports *supports = aEntry->GetKey();
  nsCOMPtr<nsISizeOfEventTarget> iface = do_QueryInterface(supports);
  return iface ? iface->SizeOfEventTargetIncludingThis(aMallocSizeOf) : 0;
}

void
nsGlobalWindow::SizeOfIncludingThis(nsWindowSizes* aWindowSizes) const
{
  aWindowSizes->mDOMOther += aWindowSizes->mMallocSizeOf(this);

  if (IsInnerWindow()) {
    nsEventListenerManager* elm =
      const_cast<nsGlobalWindow*>(this)->GetListenerManager(false);
    if (elm) {
      aWindowSizes->mDOMOther +=
        elm->SizeOfIncludingThis(aWindowSizes->mMallocSizeOf);
    }
    if (mDoc) {
      mDoc->DocSizeOfIncludingThis(aWindowSizes);
    }
  }

  aWindowSizes->mDOMOther +=
    mNavigator ?
      mNavigator->SizeOfIncludingThis(aWindowSizes->mMallocSizeOf) : 0;

  aWindowSizes->mDOMEventTargets +=
    mEventTargetObjects.SizeOfExcludingThis(
      SizeOfEventTargetObjectsEntryExcludingThisFun,
      aWindowSizes->mMallocSizeOf);
}


#ifdef MOZ_GAMEPAD
void
nsGlobalWindow::AddGamepad(PRUint32 aIndex, nsDOMGamepad* aGamepad)
{
  FORWARD_TO_INNER_VOID(AddGamepad, (aIndex, aGamepad));
  mGamepads.Put(aIndex, aGamepad);
}

void
nsGlobalWindow::RemoveGamepad(PRUint32 aIndex)
{
  FORWARD_TO_INNER_VOID(RemoveGamepad, (aIndex));
  mGamepads.Remove(aIndex);
}

already_AddRefed<nsDOMGamepad>
nsGlobalWindow::GetGamepad(PRUint32 aIndex)
{
  FORWARD_TO_INNER(GetGamepad, (aIndex), nullptr);
  nsRefPtr<nsDOMGamepad> gamepad;
  if (mGamepads.Get(aIndex, getter_AddRefs(gamepad))) {
    return gamepad.forget();
  }

  return nullptr;
}

void
nsGlobalWindow::SetHasSeenGamepadInput(bool aHasSeen)
{
  FORWARD_TO_INNER_VOID(SetHasSeenGamepadInput, (aHasSeen));
  mHasSeenGamepadInput = aHasSeen;
}

bool
nsGlobalWindow::HasSeenGamepadInput()
{
  FORWARD_TO_INNER(HasSeenGamepadInput, (), false);
  return mHasSeenGamepadInput;
}


PLDHashOperator
nsGlobalWindow::EnumGamepadsForSync(const PRUint32& aKey, nsDOMGamepad* aData, void* userArg)
{
  nsRefPtr<GamepadService> gamepadsvc(GamepadService::GetService());
  gamepadsvc->SyncGamepadState(aKey, aData);
  return PL_DHASH_NEXT;
}

void
nsGlobalWindow::SyncGamepadState()
{
  FORWARD_TO_INNER_VOID(SyncGamepadState, ());
  if (mHasSeenGamepadInput) {
    mGamepads.EnumerateRead(EnumGamepadsForSync, NULL);
  }
}
#endif


NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsGlobalChromeWindow,
                                                  nsGlobalWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBrowserDOMWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMessageManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsGlobalChromeWindow,
                                                nsGlobalWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mBrowserDOMWindow)
  if (tmp->mMessageManager) {
    static_cast<nsFrameMessageManager*>(
      tmp->mMessageManager.get())->Disconnect();
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mMessageManager)
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

DOMCI_DATA(ChromeWindow, nsGlobalChromeWindow)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsGlobalChromeWindow)
  NS_INTERFACE_MAP_ENTRY(nsIDOMChromeWindow)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ChromeWindow)
NS_INTERFACE_MAP_END_INHERITING(nsGlobalWindow)

NS_IMPL_ADDREF_INHERITED(nsGlobalChromeWindow, nsGlobalWindow)
NS_IMPL_RELEASE_INHERITED(nsGlobalChromeWindow, nsGlobalWindow)

NS_IMETHODIMP
nsGlobalChromeWindow::GetWindowState(uint16_t* aWindowState)
{
  *aWindowState = nsIDOMChromeWindow::STATE_NORMAL;

  nsCOMPtr<nsIWidget> widget = GetMainWidget();

  int32_t aMode = 0;

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
nsGlobalChromeWindow::GetAttentionWithCycleCount(int32_t aCycleCount)
{
  nsCOMPtr<nsIWidget> widget = GetMainWidget();
  nsresult rv = NS_OK;

  if (widget) {
    rv = widget->GetAttention(aCycleCount);
  }

  return rv;
}

NS_IMETHODIMP
nsGlobalChromeWindow::BeginWindowMove(nsIDOMEvent *aMouseDownEvent, nsIDOMElement* aPanel)
{
  nsCOMPtr<nsIWidget> widget;

  
#ifdef MOZ_XUL
  if (aPanel) {
    nsCOMPtr<nsIContent> panel = do_QueryInterface(aPanel);
    NS_ENSURE_TRUE(panel, NS_ERROR_FAILURE);

    nsIFrame* frame = panel->GetPrimaryFrame();
    NS_ENSURE_TRUE(frame && frame->GetType() == nsGkAtoms::menuPopupFrame, NS_OK);

    widget = (static_cast<nsMenuPopupFrame*>(frame))->GetWidget();
  }
  else {
#endif
    widget = GetMainWidget();
#ifdef MOZ_XUL
  }
#endif

  if (!widget) {
    return NS_OK;
  }

  NS_ENSURE_TRUE(aMouseDownEvent, NS_ERROR_FAILURE);
  nsEvent *internalEvent = aMouseDownEvent->GetInternalNSEvent();
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
  int32_t cursor;

  
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
    
    nsCOMPtr<nsIPresShell> presShell = mDocShell->GetPresShell();
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    nsViewManager* vm = presShell->GetViewManager();
    NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);

    nsView* rootView = vm->GetRootView();
    NS_ENSURE_TRUE(rootView, NS_ERROR_FAILURE);

    nsIWidget* widget = rootView->GetNearestWidget(nullptr);
    NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

    
    rv = presContext->EventStateManager()->SetCursor(cursor, nullptr,
                                                     false, 0.0f, 0.0f,
                                                     widget, true);
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
  bool disabled;
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
nsGlobalChromeWindow::GetMessageManager(nsIMessageBroadcaster** aManager)
{
  FORWARD_TO_INNER_CHROME(GetMessageManager, (aManager), NS_ERROR_FAILURE);
  if (!mMessageManager) {
    nsIScriptContext* scx = GetContextInternal();
    NS_ENSURE_STATE(scx);
    AutoPushJSContext cx(scx->GetNativeContext());
    NS_ENSURE_STATE(cx);
    nsCOMPtr<nsIMessageBroadcaster> globalMM =
      do_GetService("@mozilla.org/globalmessagemanager;1");
    mMessageManager =
      new nsFrameMessageManager(nullptr,
                                static_cast<nsFrameMessageManager*>(globalMM.get()),
                                cx,
                                MM_CHROME | MM_BROADCASTER);
    NS_ENSURE_TRUE(mMessageManager, NS_ERROR_OUT_OF_MEMORY);
  }
  CallQueryInterface(mMessageManager, aManager);
  return NS_OK;
}




NS_IMPL_CYCLE_COLLECTION_INHERITED_1(nsGlobalModalWindow,
                                     nsGlobalWindow,
                                     mReturnValue)

DOMCI_DATA(ModalContentWindow, nsGlobalModalWindow)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsGlobalModalWindow)
  NS_INTERFACE_MAP_ENTRY(nsIDOMModalContentWindow)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ModalContentWindow)
NS_INTERFACE_MAP_END_INHERITING(nsGlobalWindow)

NS_IMPL_ADDREF_INHERITED(nsGlobalModalWindow, nsGlobalWindow)
NS_IMPL_RELEASE_INHERITED(nsGlobalModalWindow, nsGlobalWindow)


NS_IMETHODIMP
nsGlobalModalWindow::GetDialogArguments(nsIArray **aArguments)
{
  FORWARD_TO_INNER_MODAL_CONTENT_WINDOW(GetDialogArguments, (aArguments),
                                        NS_ERROR_NOT_INITIALIZED);

  bool subsumes = false;
  nsIPrincipal *self = GetPrincipal();
  if (self && NS_SUCCEEDED(self->Subsumes(mArgumentsOrigin, &subsumes)) &&
      subsumes) {
    NS_IF_ADDREF(*aArguments = mArguments);
  } else {
    *aArguments = nullptr;
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
                                    bool aForceReuseInnerWindow)
{
  MOZ_ASSERT(aDocument);

  
  
  mReturnValue = nullptr;

  return nsGlobalWindow::SetNewDocument(aDocument, aState,
                                        aForceReuseInnerWindow);
}

void
nsGlobalWindow::SetHasAudioAvailableEventListeners()
{
  if (mDoc) {
    mDoc->NotifyAudioAvailableListener();
  }
}

#ifdef MOZ_B2G
void
nsGlobalWindow::EnableNetworkEvent(uint32_t aType)
{
  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  if (!permMgr) {
    NS_ERROR("No PermissionManager available!");
    return;
  }

  uint32_t permission = nsIPermissionManager::DENY_ACTION;
  permMgr->TestExactPermissionFromPrincipal(GetPrincipal(), "network-events",
                                            &permission);

  if (permission != nsIPermissionManager::ALLOW_ACTION) {
    return;
  }

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (!os) {
    NS_ERROR("ObserverService should be available!");
    return;
  }

  switch (aType) {
    case NS_NETWORK_UPLOAD_EVENT:
      os->AddObserver(mObserver, NS_NETWORK_ACTIVITY_BLIP_UPLOAD_TOPIC, false);
      break;
    case NS_NETWORK_DOWNLOAD_EVENT:
      os->AddObserver(mObserver, NS_NETWORK_ACTIVITY_BLIP_DOWNLOAD_TOPIC, false);
      break;
  }
}

void
nsGlobalWindow::DisableNetworkEvent(uint32_t aType)
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (!os) {
    return;
  }

  switch (aType) {
    case NS_NETWORK_UPLOAD_EVENT:
      os->RemoveObserver(mObserver, NS_NETWORK_ACTIVITY_BLIP_UPLOAD_TOPIC);
      break;
    case NS_NETWORK_DOWNLOAD_EVENT:
      os->RemoveObserver(mObserver, NS_NETWORK_ACTIVITY_BLIP_DOWNLOAD_TOPIC);
      break;
  }
}
#endif 

#define EVENT(name_, id_, type_, struct_)                                    \
  NS_IMETHODIMP nsGlobalWindow::GetOn##name_(JSContext *cx,                  \
                                             jsval *vp) {                    \
    EventHandlerNonNull* h = GetOn##name_();                                 \
    vp->setObjectOrNull(h ? h->Callable() : nullptr);                        \
    return NS_OK;                                                            \
  }                                                                          \
  NS_IMETHODIMP nsGlobalWindow::SetOn##name_(JSContext *cx,                  \
                                             const jsval &v) {               \
    JSObject *obj = mJSObject;                                               \
    if (!obj) {                                                              \
      /* Just silently do nothing */                                         \
      return NS_OK;                                                          \
    }                                                                        \
    nsRefPtr<EventHandlerNonNull> handler;                                   \
    JSObject *callable;                                                      \
    if (v.isObject() &&                                                      \
        JS_ObjectIsCallable(cx, callable = &v.toObject())) {                 \
      bool ok;                                                               \
      handler = new EventHandlerNonNull(cx, obj, callable, &ok);             \
      if (!ok) {                                                             \
        return NS_ERROR_OUT_OF_MEMORY;                                       \
      }                                                                      \
    }                                                                        \
    ErrorResult rv;                                                          \
    SetOn##name_(handler, rv);                                               \
    return rv.ErrorCode();                                                   \
  }
#define ERROR_EVENT(name_, id_, type_, struct_)                              \
  NS_IMETHODIMP nsGlobalWindow::GetOn##name_(JSContext *cx,                  \
                                             jsval *vp) {                    \
    nsEventListenerManager *elm = GetListenerManager(false);                 \
    if (elm) {                                                               \
      OnErrorEventHandlerNonNull* h = elm->GetOnErrorEventHandler();         \
      if (h) {                                                               \
        *vp = JS::ObjectValue(*h->Callable());                               \
        return NS_OK;                                                        \
      }                                                                      \
    }                                                                        \
    *vp = JSVAL_NULL;                                                        \
    return NS_OK;                                                            \
  }                                                                          \
  NS_IMETHODIMP nsGlobalWindow::SetOn##name_(JSContext *cx,                  \
                                             const jsval &v) {               \
    nsEventListenerManager *elm = GetListenerManager(true);                  \
    if (!elm) {                                                              \
      return NS_ERROR_OUT_OF_MEMORY;                                         \
    }                                                                        \
                                                                             \
    JSObject *obj = mJSObject;                                               \
    if (!obj) {                                                              \
      return NS_ERROR_UNEXPECTED;                                            \
    }                                                                        \
    nsRefPtr<OnErrorEventHandlerNonNull> handler;                            \
    JSObject *callable;                                                      \
    if (v.isObject() &&                                                      \
        JS_ObjectIsCallable(cx, callable = &v.toObject())) {                 \
      bool ok;                                                               \
      handler = new OnErrorEventHandlerNonNull(cx, obj, callable, &ok);      \
      if (!ok) {                                                             \
        return NS_ERROR_OUT_OF_MEMORY;                                       \
      }                                                                      \
    }                                                                        \
    return elm->SetEventHandler(handler);                                    \
  }
#define BEFOREUNLOAD_EVENT(name_, id_, type_, struct_)                       \
  NS_IMETHODIMP nsGlobalWindow::GetOn##name_(JSContext *cx,                  \
                                             jsval *vp) {                    \
    nsEventListenerManager *elm = GetListenerManager(false);                 \
    if (elm) {                                                               \
      BeforeUnloadEventHandlerNonNull* h =                                   \
        elm->GetOnBeforeUnloadEventHandler();                                \
      if (h) {                                                               \
        *vp = JS::ObjectValue(*h->Callable());                               \
        return NS_OK;                                                        \
      }                                                                      \
    }                                                                        \
    *vp = JSVAL_NULL;                                                        \
    return NS_OK;                                                            \
  }                                                                          \
  NS_IMETHODIMP nsGlobalWindow::SetOn##name_(JSContext *cx,                  \
                                             const jsval &v) {               \
    nsEventListenerManager *elm = GetListenerManager(true);                  \
    if (!elm) {                                                              \
      return NS_ERROR_OUT_OF_MEMORY;                                         \
    }                                                                        \
                                                                             \
    JSObject *obj = mJSObject;                                               \
    if (!obj) {                                                              \
      return NS_ERROR_UNEXPECTED;                                            \
    }                                                                        \
    nsRefPtr<BeforeUnloadEventHandlerNonNull> handler;                       \
    JSObject *callable;                                                      \
    if (v.isObject() &&                                                      \
        JS_ObjectIsCallable(cx, callable = &v.toObject())) {                 \
      bool ok;                                                               \
      handler = new BeforeUnloadEventHandlerNonNull(cx, obj, callable, &ok); \
      if (!ok) {                                                             \
        return NS_ERROR_OUT_OF_MEMORY;                                       \
      }                                                                      \
    }                                                                        \
    return elm->SetEventHandler(handler);                                    \
  }
#define WINDOW_ONLY_EVENT EVENT
#define TOUCH_EVENT EVENT
#include "nsEventNameList.h"
#undef TOUCH_EVENT
#undef WINDOW_ONLY_EVENT
#undef BEFOREUNLOAD_EVENT
#undef ERROR_EVENT
#undef EVENT

