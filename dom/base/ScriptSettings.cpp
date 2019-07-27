





#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/ThreadLocal.h"
#include "mozilla/Assertions.h"

#include "jsapi.h"
#include "xpcprivate.h" 
#include "xpcpublic.h"
#include "nsIGlobalObject.h"
#include "nsIDocShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsContentUtils.h"
#include "nsGlobalWindow.h"
#include "nsPIDOMWindow.h"
#include "nsTArray.h"
#include "nsJSUtils.h"
#include "nsDOMJSUtils.h"
#include "WorkerPrivate.h"

namespace mozilla {
namespace dom {

static mozilla::ThreadLocal<ScriptSettingsStackEntry*> sScriptSettingsTLS;

class ScriptSettingsStack {
public:
  static ScriptSettingsStackEntry* Top() {
    return sScriptSettingsTLS.get();
  }

  static void Push(ScriptSettingsStackEntry *aEntry) {
    MOZ_ASSERT(!aEntry->mOlder);
    
    
    MOZ_ASSERT_IF(!Top() || Top()->NoJSAPI(), aEntry->mIsCandidateEntryPoint);

    aEntry->mOlder = Top();
    sScriptSettingsTLS.set(aEntry);
  }

  static void Pop(ScriptSettingsStackEntry *aEntry) {
    MOZ_ASSERT(aEntry == Top());
    sScriptSettingsTLS.set(aEntry->mOlder);
  }

  static nsIGlobalObject* IncumbentGlobal() {
    ScriptSettingsStackEntry *entry = Top();
    return entry ? entry->mGlobalObject : nullptr;
  }

  static ScriptSettingsStackEntry* EntryPoint() {
    ScriptSettingsStackEntry *entry = Top();
    if (!entry) {
      return nullptr;
    }
    while (entry) {
      if (entry->mIsCandidateEntryPoint)
        return entry;
      entry = entry->mOlder;
    }
    MOZ_CRASH("Non-empty stack should always have an entry point");
  }

  static nsIGlobalObject* EntryGlobal() {
    ScriptSettingsStackEntry *entry = EntryPoint();
    return entry ? entry->mGlobalObject : nullptr;
  }

};

static unsigned long gRunToCompletionListeners = 0;

void
UseEntryScriptProfiling()
{
  MOZ_ASSERT(NS_IsMainThread());
  ++gRunToCompletionListeners;
}

void
UnuseEntryScriptProfiling()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(gRunToCompletionListeners > 0);
  --gRunToCompletionListeners;
}

void
InitScriptSettings()
{
  if (!sScriptSettingsTLS.initialized()) {
    bool success = sScriptSettingsTLS.init();
    if (!success) {
      MOZ_CRASH();
    }
  }

  sScriptSettingsTLS.set(nullptr);
}

void
DestroyScriptSettings()
{
  MOZ_ASSERT(sScriptSettingsTLS.get() == nullptr);
}

bool
ScriptSettingsInitialized()
{
  return sScriptSettingsTLS.initialized();
}

ScriptSettingsStackEntry::ScriptSettingsStackEntry(nsIGlobalObject *aGlobal,
                                                   bool aCandidate)
  : mGlobalObject(aGlobal)
  , mIsCandidateEntryPoint(aCandidate)
  , mOlder(nullptr)
{
  MOZ_ASSERT(mGlobalObject);
  MOZ_ASSERT(mGlobalObject->GetGlobalJSObject(),
             "Must have an actual JS global for the duration on the stack");
  MOZ_ASSERT(JS_IsGlobalObject(mGlobalObject->GetGlobalJSObject()),
             "No outer windows allowed");

  ScriptSettingsStack::Push(this);
}


ScriptSettingsStackEntry::ScriptSettingsStackEntry()
   : mGlobalObject(nullptr)
   , mIsCandidateEntryPoint(true)
   , mOlder(nullptr)
{
  ScriptSettingsStack::Push(this);
}

ScriptSettingsStackEntry::~ScriptSettingsStackEntry()
{
  
  MOZ_ASSERT_IF(mGlobalObject, mGlobalObject->GetGlobalJSObject());

  ScriptSettingsStack::Pop(this);
}



















static nsIGlobalObject*
ClampToSubject(nsIGlobalObject* aGlobalOrNull)
{
  if (!aGlobalOrNull || !NS_IsMainThread()) {
    return aGlobalOrNull;
  }

  nsIPrincipal* globalPrin = aGlobalOrNull->PrincipalOrNull();
  NS_ENSURE_TRUE(globalPrin, GetCurrentGlobal());
  if (!nsContentUtils::SubjectPrincipal()->SubsumesConsideringDomain(globalPrin)) {
    return GetCurrentGlobal();
  }

  return aGlobalOrNull;
}

nsIGlobalObject*
GetEntryGlobal()
{
  return ClampToSubject(ScriptSettingsStack::EntryGlobal());
}

nsIDocument*
GetEntryDocument()
{
  nsIGlobalObject* global = GetEntryGlobal();
  nsCOMPtr<nsPIDOMWindow> entryWin = do_QueryInterface(global);

  
  
  
  if (!entryWin && global) {
    entryWin = xpc::AddonWindowOrNull(global->GetGlobalJSObject());
  }

  return entryWin ? entryWin->GetExtantDoc() : nullptr;
}

nsIGlobalObject*
GetIncumbentGlobal()
{
  
  
  
  
  
  JSContext *cx = nsContentUtils::GetCurrentJSContextForThread();
  if (!cx) {
    MOZ_ASSERT(ScriptSettingsStack::EntryGlobal() == nullptr);
    return nullptr;
  }

  
  
  
  
  if (JSObject *global = JS::GetScriptedCallerGlobal(cx)) {
    return ClampToSubject(xpc::NativeGlobal(global));
  }

  
  
  return ClampToSubject(ScriptSettingsStack::IncumbentGlobal());
}

nsIGlobalObject*
GetCurrentGlobal()
{
  JSContext *cx = nsContentUtils::GetCurrentJSContextForThread();
  if (!cx) {
    return nullptr;
  }

  JSObject *global = JS::CurrentGlobalOrNull(cx);
  if (!global) {
    return nullptr;
  }

  return xpc::NativeGlobal(global);
}

nsIPrincipal*
GetWebIDLCallerPrincipal()
{
  MOZ_ASSERT(NS_IsMainThread());
  ScriptSettingsStackEntry *entry = ScriptSettingsStack::EntryPoint();

  
  
  if (!entry || entry->NoJSAPI()) {
    return nullptr;
  }
  AutoEntryScript* aes = static_cast<AutoEntryScript*>(entry);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (!aes->CxPusherIsStackTop()) {
    return nullptr;
  }

  return aes->mWebIDLCallerPrincipal;
}

static JSContext*
FindJSContext(nsIGlobalObject* aGlobalObject)
{
  MOZ_ASSERT(NS_IsMainThread());
  JSContext *cx = nullptr;
  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aGlobalObject);
  if (sgo && sgo->GetScriptContext()) {
    cx = sgo->GetScriptContext()->GetNativeContext();
  }
  if (!cx) {
    cx = nsContentUtils::GetSafeJSContext();
  }
  return cx;
}

AutoJSAPI::AutoJSAPI()
  : mCx(nullptr)
  , mOwnErrorReporting(false)
  , mOldAutoJSAPIOwnsErrorReporting(false)
{
}

AutoJSAPI::~AutoJSAPI()
{
  if (mOwnErrorReporting) {
    MOZ_ASSERT(NS_IsMainThread(), "See corresponding assertion in TakeOwnershipOfErrorReporting()");

    if (HasException()) {

      
      
      
      
      
      JS::Rooted<JSObject*> errorGlobal(cx(), JS::CurrentGlobalOrNull(cx()));
      if (!errorGlobal)
        errorGlobal = xpc::PrivilegedJunkScope();
      JSAutoCompartment ac(cx(), errorGlobal);
      nsCOMPtr<nsPIDOMWindow> win = xpc::WindowGlobalOrNull(errorGlobal);
      JS::Rooted<JS::Value> exn(cx());
      js::ErrorReport jsReport(cx());
      if (StealException(&exn) && jsReport.init(cx(), exn)) {
        nsRefPtr<xpc::ErrorReport> xpcReport = new xpc::ErrorReport();
        xpcReport->Init(jsReport.report(), jsReport.message(),
                        nsContentUtils::IsCallerChrome(),
                        win ? win->WindowID() : 0);
        if (win) {
          DispatchScriptErrorEvent(win, JS_GetRuntime(cx()), xpcReport, exn);
        } else {
          xpcReport->LogToConsole();
        }
      } else {
        NS_WARNING("OOMed while acquiring uncaught exception from JSAPI");
      }
    }

    
    
    
    
    
    JS::ContextOptionsRef(cx()).setAutoJSAPIOwnsErrorReporting(mOldAutoJSAPIOwnsErrorReporting);
  }

  if (mOldErrorReporter.isSome()) {
    JS_SetErrorReporter(JS_GetRuntime(cx()), mOldErrorReporter.value());
  }
}

void
AutoJSAPI::InitInternal(JSObject* aGlobal, JSContext* aCx, bool aIsMainThread)
{
  MOZ_ASSERT(aCx);
  mCx = aCx;
  if (aIsMainThread) {
    
    
    
    
    JS::Rooted<JSObject*> global(JS_GetRuntime(aCx), aGlobal);
    mCxPusher.emplace(mCx);
    mAutoNullableCompartment.emplace(mCx, global);
  } else {
    mAutoNullableCompartment.emplace(mCx, aGlobal);
  }

  if (aIsMainThread) {
    JSRuntime* rt = JS_GetRuntime(aCx);
    mOldErrorReporter.emplace(JS_GetErrorReporter(rt));
    JS_SetErrorReporter(rt, xpc::SystemErrorReporter);
  }
}

AutoJSAPI::AutoJSAPI(nsIGlobalObject* aGlobalObject,
                     bool aIsMainThread,
                     JSContext* aCx)
  : mOwnErrorReporting(false)
  , mOldAutoJSAPIOwnsErrorReporting(false)
{
  MOZ_ASSERT(aGlobalObject);
  MOZ_ASSERT(aGlobalObject->GetGlobalJSObject(), "Must have a JS global");
  MOZ_ASSERT(aCx);
  MOZ_ASSERT_IF(aIsMainThread, NS_IsMainThread());

  InitInternal(aGlobalObject->GetGlobalJSObject(), aCx, aIsMainThread);
}

void
AutoJSAPI::Init()
{
  MOZ_ASSERT(!mCx, "An AutoJSAPI should only be initialised once");

  InitInternal( nullptr,
               nsContentUtils::GetDefaultJSContextForThread(),
               NS_IsMainThread());
}

bool
AutoJSAPI::Init(nsIGlobalObject* aGlobalObject, JSContext* aCx)
{
  MOZ_ASSERT(!mCx, "An AutoJSAPI should only be initialised once");
  MOZ_ASSERT(aCx);

  if (NS_WARN_IF(!aGlobalObject)) {
    return false;
  }

  JSObject* global = aGlobalObject->GetGlobalJSObject();
  if (NS_WARN_IF(!global)) {
    return false;
  }

  InitInternal(global, aCx, NS_IsMainThread());
  return true;
}

bool
AutoJSAPI::Init(nsIGlobalObject* aGlobalObject)
{
  return Init(aGlobalObject, nsContentUtils::GetDefaultJSContextForThread());
}

bool
AutoJSAPI::Init(JSObject* aObject)
{
  return Init(xpc::NativeGlobal(aObject));
}

bool
AutoJSAPI::InitWithLegacyErrorReporting(nsIGlobalObject* aGlobalObject)
{
  MOZ_ASSERT(NS_IsMainThread());

  return Init(aGlobalObject, FindJSContext(aGlobalObject));
}

bool
AutoJSAPI::Init(nsPIDOMWindow* aWindow, JSContext* aCx)
{
  return Init(static_cast<nsGlobalWindow*>(aWindow), aCx);
}

bool
AutoJSAPI::Init(nsPIDOMWindow* aWindow)
{
  return Init(static_cast<nsGlobalWindow*>(aWindow));
}

bool
AutoJSAPI::Init(nsGlobalWindow* aWindow, JSContext* aCx)
{
  return Init(static_cast<nsIGlobalObject*>(aWindow), aCx);
}

bool
AutoJSAPI::Init(nsGlobalWindow* aWindow)
{
  return Init(static_cast<nsIGlobalObject*>(aWindow));
}

bool
AutoJSAPI::InitWithLegacyErrorReporting(nsPIDOMWindow* aWindow)
{
  return InitWithLegacyErrorReporting(static_cast<nsGlobalWindow*>(aWindow));
}

bool
AutoJSAPI::InitWithLegacyErrorReporting(nsGlobalWindow* aWindow)
{
  return InitWithLegacyErrorReporting(static_cast<nsIGlobalObject*>(aWindow));
}







void
WarningOnlyErrorReporter(JSContext* aCx, const char* aMessage, JSErrorReport* aRep)
{
  MOZ_ASSERT(JSREPORT_IS_WARNING(aRep->flags));
  nsRefPtr<xpc::ErrorReport> xpcReport = new xpc::ErrorReport();
  nsPIDOMWindow* win = xpc::CurrentWindowOrNull(aCx);
  xpcReport->Init(aRep, aMessage, nsContentUtils::IsCallerChrome(),
                  win ? win->WindowID() : 0);
  xpcReport->LogToConsole();
}

void
AutoJSAPI::TakeOwnershipOfErrorReporting()
{
  MOZ_ASSERT(NS_IsMainThread(), "Can't own error reporting off-main-thread yet");
  MOZ_ASSERT(!mOwnErrorReporting);
  mOwnErrorReporting = true;

  JSRuntime *rt = JS_GetRuntime(cx());
  mOldAutoJSAPIOwnsErrorReporting = JS::ContextOptionsRef(cx()).autoJSAPIOwnsErrorReporting();
  JS::ContextOptionsRef(cx()).setAutoJSAPIOwnsErrorReporting(true);
  JS_SetErrorReporter(rt, WarningOnlyErrorReporter);
}

bool
AutoJSAPI::StealException(JS::MutableHandle<JS::Value> aVal)
{
    MOZ_ASSERT(CxPusherIsStackTop());
    MOZ_ASSERT(HasException());
    MOZ_ASSERT(js::GetContextCompartment(cx()));
    if (!JS_GetPendingException(cx(), aVal)) {
      return false;
    }
    JS_ClearPendingException(cx());
    return true;
}

AutoEntryScript::AutoEntryScript(nsIGlobalObject* aGlobalObject,
                                 const char *aReason,
                                 bool aIsMainThread,
                                 JSContext* aCx)
  : AutoJSAPI(aGlobalObject, aIsMainThread,
              aCx ? aCx : FindJSContext(aGlobalObject))
  , ScriptSettingsStackEntry(aGlobalObject,  true)
  , mWebIDLCallerPrincipal(nullptr)
  , mDocShellForJSRunToCompletion(nullptr)
  , mIsMainThread(aIsMainThread)
{
  MOZ_ASSERT(aGlobalObject);
  MOZ_ASSERT_IF(!aCx, aIsMainThread); 
  MOZ_ASSERT_IF(aCx && aIsMainThread, aCx == FindJSContext(aGlobalObject));
  if (aIsMainThread) {
    nsContentUtils::EnterMicroTask();
  }

  if (aIsMainThread && gRunToCompletionListeners > 0) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobalObject);
    if (window) {
        mDocShellForJSRunToCompletion = window->GetDocShell();
    }
  }

  if (mDocShellForJSRunToCompletion) {
    mDocShellForJSRunToCompletion->NotifyJSRunToCompletionStart(aReason);
  }
}

AutoEntryScript::~AutoEntryScript()
{
  if (mDocShellForJSRunToCompletion) {
    mDocShellForJSRunToCompletion->NotifyJSRunToCompletionStop();
  }

  if (mIsMainThread) {
    nsContentUtils::LeaveMicroTask();
  }

  
  
  
  JS_MaybeGC(cx());
}

AutoIncumbentScript::AutoIncumbentScript(nsIGlobalObject* aGlobalObject)
  : ScriptSettingsStackEntry(aGlobalObject,  false)
  , mCallerOverride(nsContentUtils::GetCurrentJSContextForThread())
{
}

AutoNoJSAPI::AutoNoJSAPI(bool aIsMainThread)
  : ScriptSettingsStackEntry()
{
  if (aIsMainThread) {
    mCxPusher.emplace(static_cast<JSContext*>(nullptr),
                       true);
  }
}

danger::AutoCxPusher::AutoCxPusher(JSContext* cx, bool allowNull)
{
  MOZ_ASSERT_IF(!allowNull, cx);

  
  
  
  if (cx)
    mScx = GetScriptContextFromJSContext(cx);

  XPCJSContextStack *stack = XPCJSRuntime::Get()->GetJSContextStack();
  if (!stack->Push(cx)) {
    MOZ_CRASH();
  }
  mStackDepthAfterPush = stack->Count();

#ifdef DEBUG
  mPushedContext = cx;
  mCompartmentDepthOnEntry = cx ? js::GetEnterCompartmentDepth(cx) : 0;
#endif

  
  
  if (cx) {
    mAutoRequest.emplace(cx);
  }
}

danger::AutoCxPusher::~AutoCxPusher()
{
  
  mAutoRequest.reset();

  
  
  
  
  
  MOZ_ASSERT_IF(mPushedContext, mCompartmentDepthOnEntry ==
                                js::GetEnterCompartmentDepth(mPushedContext));
  DebugOnly<JSContext*> stackTop;
  MOZ_ASSERT(mPushedContext == nsXPConnect::XPConnect()->GetCurrentJSContext());
  XPCJSRuntime::Get()->GetJSContextStack()->Pop();
  mScx = nullptr;
}

bool
danger::AutoCxPusher::IsStackTop() const
{
  uint32_t currentDepth = XPCJSRuntime::Get()->GetJSContextStack()->Count();
  MOZ_ASSERT(currentDepth >= mStackDepthAfterPush);
  return currentDepth == mStackDepthAfterPush;
}

} 

AutoJSContext::AutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
  : mCx(nullptr)
{
  Init(false MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT);
}

AutoJSContext::AutoJSContext(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : mCx(nullptr)
{
  Init(aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT);
}

void
AutoJSContext::Init(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
{
  JS::AutoSuppressGCAnalysis nogc;
  MOZ_ASSERT(!mCx, "mCx should not be initialized!");

  MOZ_GUARD_OBJECT_NOTIFIER_INIT;

  nsXPConnect *xpc = nsXPConnect::XPConnect();
  if (!aSafe) {
    mCx = xpc->GetCurrentJSContext();
  }

  if (!mCx) {
    mJSAPI.Init();
    mCx = mJSAPI.cx();
  }
}

AutoJSContext::operator JSContext*() const
{
  return mCx;
}

ThreadsafeAutoJSContext::ThreadsafeAutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;

  if (NS_IsMainThread()) {
    mCx = nullptr;
    mAutoJSContext.emplace();
  } else {
    mCx = mozilla::dom::workers::GetCurrentThreadJSContext();
    mRequest.emplace(mCx);
  }
}

ThreadsafeAutoJSContext::operator JSContext*() const
{
  if (mCx) {
    return mCx;
  } else {
    return *mAutoJSContext;
  }
}

AutoSafeJSContext::AutoSafeJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
  : AutoJSContext(true MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)
  , mAc(mCx, xpc::UnprivilegedJunkScope())
{
}

ThreadsafeAutoSafeJSContext::ThreadsafeAutoSafeJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;

  if (NS_IsMainThread()) {
    mCx = nullptr;
    mAutoSafeJSContext.emplace();
  } else {
    mCx = mozilla::dom::workers::GetCurrentThreadJSContext();
    mRequest.emplace(mCx);
  }
}

ThreadsafeAutoSafeJSContext::operator JSContext*() const
{
  if (mCx) {
    return mCx;
  } else {
    return *mAutoSafeJSContext;
  }
}

} 
