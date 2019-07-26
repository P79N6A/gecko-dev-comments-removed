





#include "mozilla/Util.h"

#include "RuntimeService.h"

#include "nsIContentSecurityPolicy.h"
#include "nsIDOMChromeWindow.h"
#include "nsIEffectiveTLDService.h"
#include "nsIObserverService.h"
#include "nsIPlatformCharset.h"
#include "nsIPrincipal.h"
#include "nsIJSContextStack.h"
#include "nsIScriptSecurityManager.h"
#include "nsISupportsPriority.h"
#include "nsITimer.h"
#include "nsPIDOMWindow.h"
#include "nsLayoutStatics.h"

#include "jsdbgapi.h"
#include "jsfriendapi.h"
#include "mozilla/dom/EventTargetBinding.h"
#include "mozilla/Preferences.h"
#include "nsContentUtils.h"
#include "nsDOMJSUtils.h"
#include <Navigator.h>
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"
#include "xpcpublic.h"

#include "Events.h"
#include "Worker.h"
#include "WorkerPrivate.h"

#include "OSFileConstants.h"
#include <algorithm>

#include "GeckoProfiler.h"

using namespace mozilla;
using namespace mozilla::dom;

USING_WORKERS_NAMESPACE

using mozilla::MutexAutoLock;
using mozilla::MutexAutoUnlock;
using mozilla::Preferences;


#define WORKER_DEFAULT_RUNTIME_HEAPSIZE 32 * 1024 * 1024


#define WORKER_DEFAULT_ALLOCATION_THRESHOLD 30



#define WORKER_STACK_SIZE 256 * sizeof(size_t) * 1024


#define WORKER_CONTEXT_NATIVE_STACK_LIMIT 128 * sizeof(size_t) * 1024


#define MAX_WORKERS_PER_DOMAIN 10

MOZ_STATIC_ASSERT(MAX_WORKERS_PER_DOMAIN >= 1,
                  "We should allow at least one worker per domain.");


#define MAX_SCRIPT_RUN_TIME_SEC 10


#define IDLE_THREAD_TIMEOUT_SEC 30


#define MAX_IDLE_THREADS 20

#define PREF_WORKERS_ENABLED "dom.workers.enabled"
#define PREF_WORKERS_MAX_PER_DOMAIN "dom.workers.maxPerDomain"
#define PREF_WORKERS_GCZEAL "dom.workers.gczeal"
#define PREF_MAX_SCRIPT_RUN_TIME "dom.max_script_run_time"

#define GC_REQUEST_OBSERVER_TOPIC "child-gc-request"
#define MEMORY_PRESSURE_OBSERVER_TOPIC "memory-pressure"

#define BROADCAST_ALL_WORKERS(_func, ...)                                      \
  PR_BEGIN_MACRO                                                               \
    AssertIsOnMainThread();                                                    \
                                                                               \
    nsAutoTArray<WorkerPrivate*, 100> workers;                                 \
    {                                                                          \
      MutexAutoLock lock(mMutex);                                              \
                                                                               \
      mDomainMap.EnumerateRead(AddAllTopLevelWorkersToArray, &workers);        \
    }                                                                          \
                                                                               \
    if (!workers.IsEmpty()) {                                                  \
      AutoSafeJSContext cx;                                                    \
      for (uint32_t index = 0; index < workers.Length(); index++) {            \
        workers[index]-> _func (cx, __VA_ARGS__);                              \
      }                                                                        \
    }                                                                          \
  PR_END_MACRO

namespace {

const uint32_t kNoIndex = uint32_t(-1);

const uint32_t kRequiredJSContextOptions =
  JSOPTION_DONT_REPORT_UNCAUGHT | JSOPTION_NO_SCRIPT_RVAL;

uint32_t gMaxWorkersPerDomain = MAX_WORKERS_PER_DOMAIN;


RuntimeService* gRuntimeService = nullptr;

enum {
  ID_Worker = 0,
  ID_ChromeWorker,
  ID_Event,
  ID_MessageEvent,
  ID_ErrorEvent,

  ID_COUNT
};


jsid gStringIDs[ID_COUNT] = { JSID_VOID };

const char* gStringChars[] = {
  "Worker",
  "ChromeWorker",
  "WorkerEvent",
  "WorkerMessageEvent",
  "WorkerErrorEvent"

  
  
};

MOZ_STATIC_ASSERT(NS_ARRAY_LENGTH(gStringChars) == ID_COUNT,
                  "gStringChars should have the right length.");

enum {
  PREF_strict = 0,
  PREF_werror,
  PREF_methodjit,
  PREF_methodjit_always,
  PREF_typeinference,
  PREF_jit_hardening,
  PREF_mem_max,
  PREF_ion,
  PREF_asmjs,
  PREF_mem_gc_allocation_threshold_mb,

#ifdef JS_GC_ZEAL
  PREF_gczeal,
#endif

  PREF_COUNT
};

#define JS_OPTIONS_DOT_STR "javascript.options."

const char* gPrefsToWatch[] = {
  JS_OPTIONS_DOT_STR "strict",
  JS_OPTIONS_DOT_STR "werror",
  JS_OPTIONS_DOT_STR "methodjit.content",
  JS_OPTIONS_DOT_STR "methodjit_always",
  JS_OPTIONS_DOT_STR "typeinference",
  JS_OPTIONS_DOT_STR "jit_hardening",
  JS_OPTIONS_DOT_STR "mem.max",
  JS_OPTIONS_DOT_STR "ion.content",
  JS_OPTIONS_DOT_STR "experimental_asmjs",
  "dom.workers.mem.gc_allocation_threshold_mb"

#ifdef JS_GC_ZEAL
  , PREF_WORKERS_GCZEAL
#endif
};

MOZ_STATIC_ASSERT(NS_ARRAY_LENGTH(gPrefsToWatch) == PREF_COUNT,
                  "gPrefsToWatch should have the right length.");

int
PrefCallback(const char* aPrefName, void* aClosure)
{
  AssertIsOnMainThread();

  RuntimeService* rts = static_cast<RuntimeService*>(aClosure);
  NS_ASSERTION(rts, "This should never be null!");

  NS_NAMED_LITERAL_CSTRING(jsOptionStr, JS_OPTIONS_DOT_STR);

  if (!strcmp(aPrefName, gPrefsToWatch[PREF_mem_max])) {
    int32_t pref = Preferences::GetInt(aPrefName, -1);
    uint32_t maxBytes = (pref <= 0 || pref >= 0x1000) ?
                        uint32_t(-1) :
                        uint32_t(pref) * 1024 * 1024;
    RuntimeService::SetDefaultJSWorkerMemoryParameter(JSGC_MAX_BYTES, maxBytes);
    rts->UpdateAllWorkerMemoryParameter(JSGC_MAX_BYTES);
  } else if (!strcmp(aPrefName, gPrefsToWatch[PREF_mem_gc_allocation_threshold_mb])) {
    int32_t pref = Preferences::GetInt(aPrefName, 30);
    uint32_t threshold = (pref <= 0 || pref >= 0x1000) ?
                          uint32_t(30) :
                          uint32_t(pref);
    RuntimeService::SetDefaultJSWorkerMemoryParameter(JSGC_ALLOCATION_THRESHOLD, threshold);
    rts->UpdateAllWorkerMemoryParameter(JSGC_ALLOCATION_THRESHOLD);
  } else if (StringBeginsWith(nsDependentCString(aPrefName), jsOptionStr)) {
    uint32_t newOptions = kRequiredJSContextOptions;
    if (Preferences::GetBool(gPrefsToWatch[PREF_strict])) {
      newOptions |= JSOPTION_STRICT;
    }
    if (Preferences::GetBool(gPrefsToWatch[PREF_werror])) {
      newOptions |= JSOPTION_WERROR;
    }
    if (Preferences::GetBool(gPrefsToWatch[PREF_methodjit])) {
      newOptions |= JSOPTION_METHODJIT;
    }
    if (Preferences::GetBool(gPrefsToWatch[PREF_methodjit_always])) {
      newOptions |= JSOPTION_METHODJIT_ALWAYS;
    }
    if (Preferences::GetBool(gPrefsToWatch[PREF_typeinference])) {
      newOptions |= JSOPTION_TYPE_INFERENCE;
    }
    if (Preferences::GetBool(gPrefsToWatch[PREF_ion])) {
      newOptions |= JSOPTION_ION;
    }
    if (Preferences::GetBool(gPrefsToWatch[PREF_asmjs])) {
      newOptions |= JSOPTION_ASMJS;
    }

    RuntimeService::SetDefaultJSContextOptions(newOptions);
    rts->UpdateAllWorkerJSContextOptions();
  }
#ifdef JS_GC_ZEAL
  else if (!strcmp(aPrefName, gPrefsToWatch[PREF_gczeal])) {
    int32_t gczeal = Preferences::GetInt(gPrefsToWatch[PREF_gczeal]);
    RuntimeService::SetDefaultGCZeal(uint8_t(clamped(gczeal, 0, 3)));
    rts->UpdateAllWorkerGCZeal();
  }
#endif
  return 0;
}

void
ErrorReporter(JSContext* aCx, const char* aMessage, JSErrorReport* aReport)
{
  WorkerPrivate* worker = GetWorkerPrivateFromContext(aCx);
  return worker->ReportError(aCx, aMessage, aReport);
}

JSBool
OperationCallback(JSContext* aCx)
{
  WorkerPrivate* worker = GetWorkerPrivateFromContext(aCx);
  return worker->OperationCallback(aCx);
}

class LogViolationDetailsRunnable : public nsRunnable
{
  WorkerPrivate* mWorkerPrivate;
  nsString mFileName;
  uint32_t mLineNum;
  uint32_t mSyncQueueKey;

private:
  class LogViolationDetailsResponseRunnable : public WorkerSyncRunnable
  {
    uint32_t mSyncQueueKey;

  public:
    LogViolationDetailsResponseRunnable(WorkerPrivate* aWorkerPrivate,
                                        uint32_t aSyncQueueKey)
    : WorkerSyncRunnable(aWorkerPrivate, aSyncQueueKey, false),
      mSyncQueueKey(aSyncQueueKey)
    {
      NS_ASSERTION(aWorkerPrivate, "Don't hand me a null WorkerPrivate!");
    }

    bool
    WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
    {
      aWorkerPrivate->StopSyncLoop(mSyncQueueKey, true);
      return true;
    }

    bool
    PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
    {
      AssertIsOnMainThread();
      return true;
    }

    void
    PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
                 bool aDispatchResult)
    {
      AssertIsOnMainThread();
    }
  };

public:
  LogViolationDetailsRunnable(WorkerPrivate* aWorker,
                              const nsString& aFileName,
                              uint32_t aLineNum)
  : mWorkerPrivate(aWorker),
    mFileName(aFileName),
    mLineNum(aLineNum),
    mSyncQueueKey(0)
  {
    NS_ASSERTION(aWorker, "WorkerPrivate cannot be null");
  }

  bool
  Dispatch(JSContext* aCx)
  {
    AutoSyncLoopHolder syncLoop(mWorkerPrivate);
    mSyncQueueKey = syncLoop.SyncQueueKey();

    if (NS_FAILED(NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL))) {
      JS_ReportError(aCx, "Failed to dispatch to main thread!");
      return false;
    }

    return syncLoop.RunAndForget(aCx);
  }

  NS_IMETHOD
  Run()
  {
    AssertIsOnMainThread();

    nsIContentSecurityPolicy* csp = mWorkerPrivate->GetCSP();
    if (csp) {
      NS_NAMED_LITERAL_STRING(scriptSample,
         "Call to eval() or related function blocked by CSP.");
      csp->LogViolationDetails(nsIContentSecurityPolicy::VIOLATION_TYPE_EVAL,
                               mFileName, scriptSample, mLineNum);
    }

    nsRefPtr<LogViolationDetailsResponseRunnable> response =
        new LogViolationDetailsResponseRunnable(mWorkerPrivate, mSyncQueueKey);
    if (!response->Dispatch(nullptr)) {
      NS_WARNING("Failed to dispatch response!");
    }

    return NS_OK;
  }
};

JSBool
ContentSecurityPolicyAllows(JSContext* aCx)
{
  WorkerPrivate* worker = GetWorkerPrivateFromContext(aCx);
  worker->AssertIsOnWorkerThread();

  if (worker->GetReportCSPViolations()) {
    nsString fileName;
    uint32_t lineNum = 0;

    JSScript* script;
    const char* file;
    if (JS_DescribeScriptedCaller(aCx, &script, &lineNum) &&
        (file = JS_GetScriptFilename(aCx, script))) {
      fileName.AssignASCII(file);
    } else {
      JS_ReportPendingException(aCx);
    }

    nsRefPtr<LogViolationDetailsRunnable> runnable =
        new LogViolationDetailsRunnable(worker, fileName, lineNum);

    if (!runnable->Dispatch(aCx)) {
      JS_ReportPendingException(aCx);
    }
  }

  return worker->IsEvalAllowed();
}

void
CTypesActivityCallback(JSContext* aCx,
                       js::CTypesActivityType aType)
{
  WorkerPrivate* worker = GetWorkerPrivateFromContext(aCx);
  worker->AssertIsOnWorkerThread();

  switch (aType) {
    case js::CTYPES_CALL_BEGIN:
      worker->BeginCTypesCall();
      break;

    case js::CTYPES_CALL_END:
      worker->EndCTypesCall();
      break;

    case js::CTYPES_CALLBACK_BEGIN:
      worker->BeginCTypesCallback();
      break;

    case js::CTYPES_CALLBACK_END:
      worker->EndCTypesCallback();
      break;

    default:
      MOZ_NOT_REACHED("Unknown type flag!");
  }
}

JSContext*
CreateJSContextForWorker(WorkerPrivate* aWorkerPrivate)
{
  aWorkerPrivate->AssertIsOnWorkerThread();
  NS_ASSERTION(!aWorkerPrivate->GetJSContext(), "Already has a context!");

  
  
  JSRuntime* runtime = JS_NewRuntime(WORKER_DEFAULT_RUNTIME_HEAPSIZE, JS_NO_HELPER_THREADS);
  if (!runtime) {
    NS_WARNING("Could not create new runtime!");
    return nullptr;
  }

  
  JS_SetGCParameter(runtime, JSGC_MAX_BYTES,
                    aWorkerPrivate->GetJSRuntimeHeapSize());
  JS_SetGCParameter(runtime, JSGC_ALLOCATION_THRESHOLD,
                    aWorkerPrivate->GetJSWorkerAllocationThreshold());

  JS_SetNativeStackQuota(runtime, WORKER_CONTEXT_NATIVE_STACK_LIMIT);

  
  static JSSecurityCallbacks securityCallbacks = {
    NULL,
    ContentSecurityPolicyAllows
  };
  JS_SetSecurityCallbacks(runtime, &securityCallbacks);

  
  static js::DOMCallbacks DOMCallbacks = {
    InstanceClassHasProtoAtDepth
  };
  SetDOMCallbacks(runtime, &DOMCallbacks);

  JSContext* workerCx = JS_NewContext(runtime, 0);
  if (!workerCx) {
    JS_DestroyRuntime(runtime);
    NS_WARNING("Could not create new context!");
    return nullptr;
  }

  JS_SetRuntimePrivate(runtime, aWorkerPrivate);

  JS_SetErrorReporter(workerCx, ErrorReporter);

  JS_SetOperationCallback(workerCx, OperationCallback);

  js::SetCTypesActivityCallback(runtime, CTypesActivityCallback);

  NS_ASSERTION((aWorkerPrivate->GetJSContextOptions() &
                kRequiredJSContextOptions) == kRequiredJSContextOptions,
               "Somehow we lost our required options!");
  JS_SetOptions(workerCx, aWorkerPrivate->GetJSContextOptions());

#ifdef JS_GC_ZEAL
  {
    uint8_t zeal = aWorkerPrivate->GetGCZeal();
    NS_ASSERTION(zeal <= 3, "Bad zeal value!");

    uint32_t frequency = zeal <= 2 ? JS_DEFAULT_ZEAL_FREQ : 1;
    JS_SetGCZeal(workerCx, zeal, frequency);
  }
#endif

  if (aWorkerPrivate->IsChromeWorker()) {
    JS_SetVersion(workerCx, JSVERSION_LATEST);
  }

  return workerCx;
}

class WorkerThreadRunnable : public nsRunnable
{
  WorkerPrivate* mWorkerPrivate;

public:
  WorkerThreadRunnable(WorkerPrivate* aWorkerPrivate)
  : mWorkerPrivate(aWorkerPrivate)
  {
    NS_ASSERTION(mWorkerPrivate, "This should never be null!");
  }

  NS_IMETHOD
  Run()
  {
    WorkerPrivate* workerPrivate = mWorkerPrivate;
    mWorkerPrivate = nullptr;

    workerPrivate->AssertIsOnWorkerThread();

    JSContext* cx = CreateJSContextForWorker(workerPrivate);
    if (!cx) {
      
      NS_ERROR("Failed to create runtime and context!");
      return NS_ERROR_FAILURE;
    }

    profiler_register_thread("WebWorker");

    {
      JSAutoRequest ar(cx);
      workerPrivate->DoRunLoop(cx);
    }

    JSRuntime* rt = JS_GetRuntime(cx);

    
    
    
    
    
    
    
    
    JSContext* dummyCx = JS_NewContext(rt, 0);
    if (dummyCx) {
      JS_DestroyContext(cx);
      JS_DestroyContext(dummyCx);
    }
    else {
      NS_WARNING("Failed to create dummy context!");
      JS_DestroyContext(cx);
    }

    JS_DestroyRuntime(rt);

    workerPrivate->ScheduleDeletion(false);
    profiler_unregister_thread();
    return NS_OK;
  }
};

} 

BEGIN_WORKERS_NAMESPACE


JSBool
ResolveWorkerClasses(JSContext* aCx, JSHandleObject aObj, JSHandleId aId, unsigned aFlags,
                     JSMutableHandleObject aObjp)
{
  AssertIsOnMainThread();

  
  if (JSID_IS_VOID(gStringIDs[0])) {
    for (uint32_t i = 0; i < ID_COUNT; i++) {
      JSString* str = JS_InternString(aCx, gStringChars[i]);
      if (!str) {
        while (i) {
          gStringIDs[--i] = JSID_VOID;
        }
        return false;
      }
      gStringIDs[i] = INTERNED_STRING_TO_JSID(aCx, str);
    }
  }

  bool isChrome = false;
  bool shouldResolve = false;

  for (uint32_t i = 0; i < ID_COUNT; i++) {
    if (gStringIDs[i] == aId) {
      isChrome = nsContentUtils::IsCallerChrome();

      
      
      shouldResolve = gStringIDs[ID_ChromeWorker] == aId ? isChrome : true;
      break;
    }
  }

  if (shouldResolve) {
    
    if (!isChrome && !Preferences::GetBool(PREF_WORKERS_ENABLED)) {
      aObjp.set(nullptr);
      return true;
    }

    JSObject* eventTarget = EventTargetBinding_workers::GetProtoObject(aCx, aObj);
    if (!eventTarget) {
      return false;
    }

    JSObject* worker = worker::InitClass(aCx, aObj, eventTarget, true);
    if (!worker) {
      return false;
    }

    if (isChrome && !chromeworker::InitClass(aCx, aObj, worker, true)) {
      return false;
    }

    if (!events::InitClasses(aCx, aObj, true)) {
      return false;
    }

    aObjp.set(aObj);
    return true;
  }

  
  aObjp.set(nullptr);
  return true;
}

void
CancelWorkersForWindow(JSContext* aCx, nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();
  RuntimeService* runtime = RuntimeService::GetService();
  if (runtime) {
    runtime->CancelWorkersForWindow(aCx, aWindow);
  }
}

void
SuspendWorkersForWindow(JSContext* aCx, nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();
  RuntimeService* runtime = RuntimeService::GetService();
  if (runtime) {
    runtime->SuspendWorkersForWindow(aCx, aWindow);
  }
}

void
ResumeWorkersForWindow(JSContext* aCx, nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();
  RuntimeService* runtime = RuntimeService::GetService();
  if (runtime) {
    runtime->ResumeWorkersForWindow(aCx, aWindow);
  }
}

namespace {

class WorkerTaskRunnable : public WorkerRunnable
{
public:
  WorkerTaskRunnable(WorkerPrivate* aPrivate, WorkerTask* aTask)
    : WorkerRunnable(aPrivate, WorkerThread, UnchangedBusyCount,
                     SkipWhenClearing),
      mTask(aTask)
  { }

  virtual bool PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) {
    return true;
  }

  virtual void PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
                            bool aDispatchResult)
  { }

  virtual bool WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate);

private:
  nsRefPtr<WorkerTask> mTask;
};

bool
WorkerTaskRunnable::WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
{
  return mTask->RunTask(aCx);
}

}

bool
WorkerCrossThreadDispatcher::PostTask(WorkerTask* aTask)
{
  mozilla::MutexAutoLock lock(mMutex);
  if (!mPrivate) {
    return false;
  }

  nsRefPtr<WorkerTaskRunnable> runnable = new WorkerTaskRunnable(mPrivate, aTask);
  runnable->Dispatch(nullptr);
  return true;
}

END_WORKERS_NAMESPACE

uint32_t RuntimeService::sDefaultJSContextOptions = kRequiredJSContextOptions;

uint32_t RuntimeService::sDefaultJSRuntimeHeapSize =
  WORKER_DEFAULT_RUNTIME_HEAPSIZE;

uint32_t RuntimeService::sDefaultJSAllocationThreshold =
  WORKER_DEFAULT_ALLOCATION_THRESHOLD;

int32_t RuntimeService::sCloseHandlerTimeoutSeconds = MAX_SCRIPT_RUN_TIME_SEC;

#ifdef JS_GC_ZEAL
uint8_t RuntimeService::sDefaultGCZeal = 0;
#endif

RuntimeService::RuntimeService()
: mMutex("RuntimeService::mMutex"), mObserved(false),
  mShuttingDown(false), mNavigatorStringsLoaded(false)
{
  AssertIsOnMainThread();
  NS_ASSERTION(!gRuntimeService, "More than one service!");
}

RuntimeService::~RuntimeService()
{
  AssertIsOnMainThread();

  
  NS_ASSERTION(!gRuntimeService || gRuntimeService == this,
               "More than one service!");

  gRuntimeService = nullptr;
}


RuntimeService*
RuntimeService::GetOrCreateService()
{
  AssertIsOnMainThread();

  if (!gRuntimeService) {
    nsRefPtr<RuntimeService> service = new RuntimeService();
    if (NS_FAILED(service->Init())) {
      NS_WARNING("Failed to initialize!");
      service->Cleanup();
      return nullptr;
    }

    
    gRuntimeService = service;
  }

  return gRuntimeService;
}


RuntimeService*
RuntimeService::GetService()
{
  return gRuntimeService;
}

bool
RuntimeService::RegisterWorker(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
{
  aWorkerPrivate->AssertIsOnParentThread();

  WorkerPrivate* parent = aWorkerPrivate->GetParent();
  if (!parent) {
    AssertIsOnMainThread();

    if (mShuttingDown) {
      JS_ReportError(aCx, "Cannot create worker during shutdown!");
      return false;
    }
  }

  WorkerDomainInfo* domainInfo;
  bool queued = false;
  {
    const nsCString& domain = aWorkerPrivate->Domain();

    MutexAutoLock lock(mMutex);

    if (!mDomainMap.Get(domain, &domainInfo)) {
      NS_ASSERTION(!parent, "Shouldn't have a parent here!");

      domainInfo = new WorkerDomainInfo();
      domainInfo->mDomain = domain;
      mDomainMap.Put(domain, domainInfo);
    }

    if (domainInfo) {
      queued = gMaxWorkersPerDomain &&
               domainInfo->ActiveWorkerCount() >= gMaxWorkersPerDomain &&
               !domain.IsEmpty();

      if (queued) {
        domainInfo->mQueuedWorkers.AppendElement(aWorkerPrivate);
      }
      else if (parent) {
        domainInfo->mChildWorkerCount++;
      }
      else {
        domainInfo->mActiveWorkers.AppendElement(aWorkerPrivate);
      }
    }
  }

  if (!domainInfo) {
    JS_ReportOutOfMemory(aCx);
    return false;
  }

  
  if (parent) {
    if (!parent->AddChildWorker(aCx, aWorkerPrivate)) {
      UnregisterWorker(aCx, aWorkerPrivate);
      return false;
    }
  }
  else {
    if (!mNavigatorStringsLoaded) {
      if (NS_FAILED(NS_GetNavigatorAppName(mNavigatorStrings.mAppName)) ||
          NS_FAILED(NS_GetNavigatorAppVersion(mNavigatorStrings.mAppVersion)) ||
          NS_FAILED(NS_GetNavigatorPlatform(mNavigatorStrings.mPlatform)) ||
          NS_FAILED(NS_GetNavigatorUserAgent(mNavigatorStrings.mUserAgent))) {
        JS_ReportError(aCx, "Failed to load navigator strings!");
        UnregisterWorker(aCx, aWorkerPrivate);
        return false;
      }

      mNavigatorStringsLoaded = true;
    }

    nsPIDOMWindow* window = aWorkerPrivate->GetWindow();

    nsTArray<WorkerPrivate*>* windowArray;
    if (!mWindowMap.Get(window, &windowArray)) {
      NS_ASSERTION(!parent, "Shouldn't have a parent here!");

      windowArray = new nsTArray<WorkerPrivate*>(1);
      mWindowMap.Put(window, windowArray);
    }

    NS_ASSERTION(!windowArray->Contains(aWorkerPrivate),
                 "Already know about this worker!");
    windowArray->AppendElement(aWorkerPrivate);
  }

  if (!queued && !ScheduleWorker(aCx, aWorkerPrivate)) {
    return false;
  }

  return true;
}

void
RuntimeService::UnregisterWorker(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
{
  aWorkerPrivate->AssertIsOnParentThread();

  WorkerPrivate* parent = aWorkerPrivate->GetParent();
  if (!parent) {
    AssertIsOnMainThread();
  }

  WorkerPrivate* queuedWorker = nullptr;
  {
    const nsCString& domain = aWorkerPrivate->Domain();

    MutexAutoLock lock(mMutex);

    WorkerDomainInfo* domainInfo;
    if (!mDomainMap.Get(domain, &domainInfo)) {
      NS_ERROR("Don't have an entry for this domain!");
    }

    
    uint32_t index = domainInfo->mQueuedWorkers.IndexOf(aWorkerPrivate);
    if (index != kNoIndex) {
      
      domainInfo->mQueuedWorkers.RemoveElementAt(index);
    }
    else if (parent) {
      NS_ASSERTION(domainInfo->mChildWorkerCount, "Must be non-zero!");
      domainInfo->mChildWorkerCount--;
    }
    else {
      NS_ASSERTION(domainInfo->mActiveWorkers.Contains(aWorkerPrivate),
                   "Don't know about this worker!");
      domainInfo->mActiveWorkers.RemoveElement(aWorkerPrivate);
    }

    
    if (domainInfo->ActiveWorkerCount() < gMaxWorkersPerDomain &&
        !domainInfo->mQueuedWorkers.IsEmpty()) {
      queuedWorker = domainInfo->mQueuedWorkers[0];
      domainInfo->mQueuedWorkers.RemoveElementAt(0);

      if (queuedWorker->GetParent()) {
        domainInfo->mChildWorkerCount++;
      }
      else {
        domainInfo->mActiveWorkers.AppendElement(queuedWorker);
      }
    }

    if (!domainInfo->ActiveWorkerCount()) {
      NS_ASSERTION(domainInfo->mQueuedWorkers.IsEmpty(), "Huh?!");
      mDomainMap.Remove(domain);
    }
  }

  if (parent) {
    parent->RemoveChildWorker(aCx, aWorkerPrivate);
  }
  else {
    nsPIDOMWindow* window = aWorkerPrivate->GetWindow();

    nsTArray<WorkerPrivate*>* windowArray;
    if (!mWindowMap.Get(window, &windowArray)) {
      NS_ERROR("Don't have an entry for this window!");
    }

    NS_ASSERTION(windowArray->Contains(aWorkerPrivate),
                 "Don't know about this worker!");
    windowArray->RemoveElement(aWorkerPrivate);

    if (windowArray->IsEmpty()) {
      NS_ASSERTION(!queuedWorker, "How can this be?!");
      mWindowMap.Remove(window);
    }
  }

  if (queuedWorker && !ScheduleWorker(aCx, queuedWorker)) {
    UnregisterWorker(aCx, queuedWorker);
  }
}

bool
RuntimeService::ScheduleWorker(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
{
  if (!aWorkerPrivate->Start()) {
    
    return true;
  }

  nsCOMPtr<nsIThread> thread;
  {
    MutexAutoLock lock(mMutex);
    if (!mIdleThreadArray.IsEmpty()) {
      uint32_t index = mIdleThreadArray.Length() - 1;
      mIdleThreadArray[index].mThread.swap(thread);
      mIdleThreadArray.RemoveElementAt(index);
    }
  }

  if (!thread) {
    if (NS_FAILED(NS_NewNamedThread("DOM Worker",
                                    getter_AddRefs(thread), nullptr,
                                    WORKER_STACK_SIZE))) {
      UnregisterWorker(aCx, aWorkerPrivate);
      JS_ReportError(aCx, "Could not create new thread!");
      return false;
    }

    nsCOMPtr<nsISupportsPriority> priority = do_QueryInterface(thread);
    if (!priority ||
        NS_FAILED(priority->SetPriority(nsISupportsPriority::PRIORITY_LOW))) {
      NS_WARNING("Could not lower the new thread's priority!");
    }
  }

#ifdef DEBUG
  aWorkerPrivate->SetThread(thread);
#endif

  nsCOMPtr<nsIRunnable> runnable = new WorkerThreadRunnable(aWorkerPrivate);
  if (NS_FAILED(thread->Dispatch(runnable, NS_DISPATCH_NORMAL))) {
    UnregisterWorker(aCx, aWorkerPrivate);
    JS_ReportError(aCx, "Could not dispatch to thread!");
    return false;
  }

  return true;
}


void
RuntimeService::ShutdownIdleThreads(nsITimer* aTimer, void* )
{
  AssertIsOnMainThread();

  RuntimeService* runtime = RuntimeService::GetService();
  NS_ASSERTION(runtime, "This should never be null!");

  NS_ASSERTION(aTimer == runtime->mIdleThreadTimer, "Wrong timer!");

  
  TimeStamp now = TimeStamp::Now() + TimeDuration::FromSeconds(1);

  TimeStamp nextExpiration;

  nsAutoTArray<nsCOMPtr<nsIThread>, 20> expiredThreads;
  {
    MutexAutoLock lock(runtime->mMutex);

    for (uint32_t index = 0; index < runtime->mIdleThreadArray.Length();
         index++) {
      IdleThreadInfo& info = runtime->mIdleThreadArray[index];
      if (info.mExpirationTime > now) {
        nextExpiration = info.mExpirationTime;
        break;
      }

      nsCOMPtr<nsIThread>* thread = expiredThreads.AppendElement();
      thread->swap(info.mThread);
    }

    if (!expiredThreads.IsEmpty()) {
      runtime->mIdleThreadArray.RemoveElementsAt(0, expiredThreads.Length());
    }
  }

  NS_ASSERTION(nextExpiration.IsNull() || !expiredThreads.IsEmpty(),
               "Should have a new time or there should be some threads to shut "
               "down");

  for (uint32_t index = 0; index < expiredThreads.Length(); index++) {
    if (NS_FAILED(expiredThreads[index]->Shutdown())) {
      NS_WARNING("Failed to shutdown thread!");
    }
  }

  if (!nextExpiration.IsNull()) {
    TimeDuration delta = nextExpiration - TimeStamp::Now();
    uint32_t delay(delta > TimeDuration(0) ? delta.ToMilliseconds() : 0);

    
    if (NS_FAILED(aTimer->InitWithFuncCallback(ShutdownIdleThreads, nullptr,
                                               delay,
                                               nsITimer::TYPE_ONE_SHOT))) {
      NS_ERROR("Can't schedule timer!");
    }
  }
}

nsresult
RuntimeService::Init()
{
  AssertIsOnMainThread();
  nsLayoutStatics::AddRef();

  mIdleThreadTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  NS_ENSURE_STATE(mIdleThreadTimer);

  mDomainMap.Init();
  mWindowMap.Init();

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE(obs, NS_ERROR_FAILURE);

  nsresult rv =
    obs->AddObserver(this, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID, false);
  NS_ENSURE_SUCCESS(rv, rv);

  mObserved = true;

  if (NS_FAILED(obs->AddObserver(this, GC_REQUEST_OBSERVER_TOPIC, false))) {
    NS_WARNING("Failed to register for GC request notifications!");
  }

  if (NS_FAILED(obs->AddObserver(this, MEMORY_PRESSURE_OBSERVER_TOPIC,
                                 false))) {
    NS_WARNING("Failed to register for memory pressure notifications!");
  }

  for (uint32_t index = 0; index < ArrayLength(gPrefsToWatch); index++) {
    if (NS_FAILED(Preferences::RegisterCallback(PrefCallback,
                                                gPrefsToWatch[index], this))) {
      NS_WARNING("Failed to register pref callback?!");
    }
    PrefCallback(gPrefsToWatch[index], this);
  }

  
  
  
  if (NS_FAILED(Preferences::AddIntVarCache(&sCloseHandlerTimeoutSeconds,
                                            PREF_MAX_SCRIPT_RUN_TIME,
                                            MAX_SCRIPT_RUN_TIME_SEC))) {
      NS_WARNING("Failed to register timeout cache?!");
  }

  int32_t maxPerDomain = Preferences::GetInt(PREF_WORKERS_MAX_PER_DOMAIN,
                                             MAX_WORKERS_PER_DOMAIN);
  gMaxWorkersPerDomain = std::max(0, maxPerDomain);

  mDetectorName = Preferences::GetLocalizedCString("intl.charset.detector");

  nsCOMPtr<nsIPlatformCharset> platformCharset =
    do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = platformCharset->GetCharset(kPlatformCharsetSel_PlainTextInFile,
                                     mSystemCharset);
  }

  rv = InitOSFileConstants();
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}



void
RuntimeService::Cleanup()
{
  AssertIsOnMainThread();

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_WARN_IF_FALSE(obs, "Failed to get observer service?!");

  
  if (obs && NS_FAILED(obs->NotifyObservers(nullptr, WORKERS_SHUTDOWN_TOPIC,
                                            nullptr))) {
    NS_WARNING("NotifyObservers failed!");
  }

  
  mShuttingDown = true;

  if (mIdleThreadTimer) {
    if (NS_FAILED(mIdleThreadTimer->Cancel())) {
      NS_WARNING("Failed to cancel idle timer!");
    }
    mIdleThreadTimer = nullptr;
  }

  if (mDomainMap.IsInitialized()) {
    MutexAutoLock lock(mMutex);

    nsAutoTArray<WorkerPrivate*, 100> workers;
    mDomainMap.EnumerateRead(AddAllTopLevelWorkersToArray, &workers);

    if (!workers.IsEmpty()) {
      nsIThread* currentThread;

      
      {
        MutexAutoUnlock unlock(mMutex);

        currentThread = NS_GetCurrentThread();
        NS_ASSERTION(currentThread, "This should never be null!");

        AutoSafeJSContext cx;

        for (uint32_t index = 0; index < workers.Length(); index++) {
          if (!workers[index]->Kill(cx)) {
            NS_WARNING("Failed to cancel worker!");
          }
        }
      }

      
      if (!mIdleThreadArray.IsEmpty()) {
        nsAutoTArray<nsCOMPtr<nsIThread>, 20> idleThreads;

        uint32_t idleThreadCount = mIdleThreadArray.Length();
        idleThreads.SetLength(idleThreadCount);

        for (uint32_t index = 0; index < idleThreadCount; index++) {
          NS_ASSERTION(mIdleThreadArray[index].mThread, "Null thread!");
          idleThreads[index].swap(mIdleThreadArray[index].mThread);
        }

        mIdleThreadArray.Clear();

        MutexAutoUnlock unlock(mMutex);

        for (uint32_t index = 0; index < idleThreadCount; index++) {
          if (NS_FAILED(idleThreads[index]->Shutdown())) {
            NS_WARNING("Failed to shutdown thread!");
          }
        }
      }

      
      
      while (mDomainMap.Count()) {
        MutexAutoUnlock unlock(mMutex);

        if (!NS_ProcessNextEvent(currentThread)) {
          NS_WARNING("Something bad happened!");
          break;
        }
      }
    }
  }

  if (mWindowMap.IsInitialized()) {
    NS_ASSERTION(!mWindowMap.Count(), "All windows should have been released!");
  }

  if (mObserved) {
    for (uint32_t index = 0; index < ArrayLength(gPrefsToWatch); index++) {
      Preferences::UnregisterCallback(PrefCallback, gPrefsToWatch[index], this);
    }

    if (obs) {
      if (NS_FAILED(obs->RemoveObserver(this, GC_REQUEST_OBSERVER_TOPIC))) {
        NS_WARNING("Failed to unregister for GC request notifications!");
      }

      if (NS_FAILED(obs->RemoveObserver(this,
                                        MEMORY_PRESSURE_OBSERVER_TOPIC))) {
        NS_WARNING("Failed to unregister for memory pressure notifications!");
      }

      nsresult rv =
        obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID);
      mObserved = NS_FAILED(rv);
    }
  }

  CleanupOSFileConstants();
  nsLayoutStatics::Release();
}


PLDHashOperator
RuntimeService::AddAllTopLevelWorkersToArray(const nsACString& aKey,
                                             WorkerDomainInfo* aData,
                                             void* aUserArg)
{
  nsTArray<WorkerPrivate*>* array =
    static_cast<nsTArray<WorkerPrivate*>*>(aUserArg);

#ifdef DEBUG
  for (uint32_t index = 0; index < aData->mActiveWorkers.Length(); index++) {
    NS_ASSERTION(!aData->mActiveWorkers[index]->GetParent(),
                 "Shouldn't have a parent in this list!");
  }
#endif

  array->AppendElements(aData->mActiveWorkers);

  
  for (uint32_t index = 0; index < aData->mQueuedWorkers.Length(); index++) {
    WorkerPrivate* worker = aData->mQueuedWorkers[index];
    if (!worker->GetParent()) {
      array->AppendElement(worker);
    }
  }

  return PL_DHASH_NEXT;
}

void
RuntimeService::GetWorkersForWindow(nsPIDOMWindow* aWindow,
                                    nsTArray<WorkerPrivate*>& aWorkers)
{
  AssertIsOnMainThread();

  nsTArray<WorkerPrivate*>* workers;
  if (mWindowMap.Get(aWindow, &workers)) {
    NS_ASSERTION(!workers->IsEmpty(), "Should have been removed!");
    aWorkers.AppendElements(*workers);
  }
  else {
    NS_ASSERTION(aWorkers.IsEmpty(), "Should be empty!");
  }
}

void
RuntimeService::CancelWorkersForWindow(JSContext* aCx,
                                       nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();

  nsAutoTArray<WorkerPrivate*, 100> workers;
  GetWorkersForWindow(aWindow, workers);

  if (!workers.IsEmpty()) {
    AutoSafeJSContext cx(aCx);
    for (uint32_t index = 0; index < workers.Length(); index++) {
      if (!workers[index]->Cancel(aCx)) {
        NS_WARNING("Failed to cancel worker!");
      }
    }
  }
}

void
RuntimeService::SuspendWorkersForWindow(JSContext* aCx,
                                        nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();

  nsAutoTArray<WorkerPrivate*, 100> workers;
  GetWorkersForWindow(aWindow, workers);

  if (!workers.IsEmpty()) {
    AutoSafeJSContext cx(aCx);
    for (uint32_t index = 0; index < workers.Length(); index++) {
      if (!workers[index]->Suspend(aCx)) {
        NS_WARNING("Failed to cancel worker!");
      }
    }
  }
}

void
RuntimeService::ResumeWorkersForWindow(JSContext* aCx,
                                       nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();

  nsAutoTArray<WorkerPrivate*, 100> workers;
  GetWorkersForWindow(aWindow, workers);

  if (!workers.IsEmpty()) {
    AutoSafeJSContext cx(aCx);
    for (uint32_t index = 0; index < workers.Length(); index++) {
      if (!workers[index]->Resume(aCx)) {
        NS_WARNING("Failed to cancel worker!");
      }
    }
  }
}

void
RuntimeService::NoteIdleThread(nsIThread* aThread)
{
  AssertIsOnMainThread();
  NS_ASSERTION(aThread, "Null pointer!");

  static TimeDuration timeout =
    TimeDuration::FromSeconds(IDLE_THREAD_TIMEOUT_SEC);

  TimeStamp expirationTime = TimeStamp::Now() + timeout;

  bool shutdown;
  if (mShuttingDown) {
    shutdown = true;
  }
  else {
    MutexAutoLock lock(mMutex);

    if (mIdleThreadArray.Length() < MAX_IDLE_THREADS) {
      IdleThreadInfo* info = mIdleThreadArray.AppendElement();
      info->mThread = aThread;
      info->mExpirationTime = expirationTime;
      shutdown = false;
    }
    else {
      shutdown = true;
    }
  }

  
  if (shutdown) {
    if (NS_FAILED(aThread->Shutdown())) {
      NS_WARNING("Failed to shutdown thread!");
    }
    return;
  }

  
  if (NS_FAILED(mIdleThreadTimer->
                  InitWithFuncCallback(ShutdownIdleThreads, nullptr,
                                       IDLE_THREAD_TIMEOUT_SEC * 1000,
                                       nsITimer::TYPE_ONE_SHOT))) {
    NS_ERROR("Can't schedule timer!");
  }
}

void
RuntimeService::UpdateAllWorkerJSContextOptions()
{
  BROADCAST_ALL_WORKERS(UpdateJSContextOptions, GetDefaultJSContextOptions());
}

void
RuntimeService::UpdateAllWorkerMemoryParameter(JSGCParamKey key)
{
  BROADCAST_ALL_WORKERS(UpdateJSWorkerMemoryParameter,
                        key,
                        GetDefaultJSWorkerMemoryParameter(key));
}

#ifdef JS_GC_ZEAL
void
RuntimeService::UpdateAllWorkerGCZeal()
{
  BROADCAST_ALL_WORKERS(UpdateGCZeal, GetDefaultGCZeal());
}
#endif

void
RuntimeService::GarbageCollectAllWorkers(bool aShrinking)
{
  BROADCAST_ALL_WORKERS(GarbageCollect, aShrinking);
}


NS_IMPL_ISUPPORTS1(RuntimeService, nsIObserver)


NS_IMETHODIMP
RuntimeService::Observe(nsISupports* aSubject, const char* aTopic,
                        const PRUnichar* aData)
{
  AssertIsOnMainThread();

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID)) {
    Cleanup();
    return NS_OK;
  }
  if (!strcmp(aTopic, GC_REQUEST_OBSERVER_TOPIC)) {
    GarbageCollectAllWorkers(false);
    return NS_OK;
  }
  if (!strcmp(aTopic, MEMORY_PRESSURE_OBSERVER_TOPIC)) {
    GarbageCollectAllWorkers(true);
    return NS_OK;
  }

  NS_NOTREACHED("Unknown observer topic!");
  return NS_OK;
}

RuntimeService::AutoSafeJSContext::AutoSafeJSContext(JSContext* aCx)
: mContext(aCx ? aCx : GetSafeContext())
{
  AssertIsOnMainThread();

  if (mContext) {
    nsIThreadJSContextStack* stack = nsContentUtils::ThreadJSContextStack();
    NS_ASSERTION(stack, "This should never be null!");

    if (NS_FAILED(stack->Push(mContext))) {
      NS_ERROR("Couldn't push safe JSContext!");
      mContext = nullptr;
      return;
    }

    JS_BeginRequest(mContext);
  }
}

RuntimeService::AutoSafeJSContext::~AutoSafeJSContext()
{
  AssertIsOnMainThread();

  if (mContext) {
    JS_ReportPendingException(mContext);

    JS_EndRequest(mContext);

    nsIThreadJSContextStack* stack = nsContentUtils::ThreadJSContextStack();
    NS_ASSERTION(stack, "This should never be null!");

    JSContext* cx;
    if (NS_FAILED(stack->Pop(&cx))) {
      NS_ERROR("Failed to pop safe context!");
    }
    if (cx != mContext) {
      NS_ERROR("Mismatched context!");
    }
  }
}


JSContext*
RuntimeService::AutoSafeJSContext::GetSafeContext()
{
  AssertIsOnMainThread();

  nsIThreadJSContextStack* stack = nsContentUtils::ThreadJSContextStack();
  NS_ASSERTION(stack, "This should never be null!");

  JSContext* cx = stack->GetSafeJSContext();
  if (!cx) {
    NS_ERROR("Couldn't get safe JSContext!");
    return nullptr;
  }

  NS_ASSERTION(!JS_IsExceptionPending(cx), "Already has an exception?!");
  return cx;
}
