





#include "RuntimeService.h"

#include "nsIChannel.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIDocument.h"
#include "nsIDOMChromeWindow.h"
#include "nsIEffectiveTLDService.h"
#include "nsIObserverService.h"
#include "nsIPrincipal.h"
#include "nsIScriptContext.h"
#include "nsIScriptError.h"
#include "nsIScriptSecurityManager.h"
#include "nsISupportsPriority.h"
#include "nsITimer.h"
#include "nsIURI.h"
#include "nsPIDOMWindow.h"

#include <algorithm>
#include "BackgroundChild.h"
#include "GeckoProfiler.h"
#include "jsfriendapi.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/dom/asmjscache/AsmJSCache.h"
#include "mozilla/dom/AtomList.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/ErrorEventBinding.h"
#include "mozilla/dom/EventTargetBinding.h"
#include "mozilla/dom/MessageEventBinding.h"
#include "mozilla/dom/WorkerBinding.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/indexedDB/IndexedDatabaseManager.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Navigator.h"
#include "nsContentUtils.h"
#include "nsCycleCollector.h"
#include "nsDOMJSUtils.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#include "nsISupportsImpl.h"
#include "nsLayoutStatics.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"
#include "OSFileConstants.h"
#include "xpcpublic.h"

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

#include "Principal.h"
#include "SharedWorker.h"
#include "WorkerDebuggerManager.h"
#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "WorkerThread.h"

#ifdef ENABLE_TESTS
#include "BackgroundChildImpl.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "prrng.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::ipc;

USING_WORKERS_NAMESPACE

using mozilla::MutexAutoLock;
using mozilla::MutexAutoUnlock;
using mozilla::Preferences;
using mozilla::dom::indexedDB::IndexedDatabaseManager;


#define WORKER_DEFAULT_RUNTIME_HEAPSIZE 32 * 1024 * 1024


#define WORKER_DEFAULT_NURSERY_SIZE 1 * 1024 * 1024


#define WORKER_DEFAULT_ALLOCATION_THRESHOLD 30


#define WORKER_CONTEXT_NATIVE_STACK_LIMIT 128 * sizeof(size_t) * 1024


#define MAX_WORKERS_PER_DOMAIN 10

static_assert(MAX_WORKERS_PER_DOMAIN >= 1,
              "We should allow at least one worker per domain.");



#define MAX_SCRIPT_RUN_TIME_SEC 10


#define IDLE_THREAD_TIMEOUT_SEC 30


#define MAX_IDLE_THREADS 20

#define PREF_WORKERS_PREFIX "dom.workers."
#define PREF_WORKERS_MAX_PER_DOMAIN PREF_WORKERS_PREFIX "maxPerDomain"

#define PREF_MAX_SCRIPT_RUN_TIME_CONTENT "dom.max_script_run_time"
#define PREF_MAX_SCRIPT_RUN_TIME_CHROME "dom.max_chrome_script_run_time"

#define GC_REQUEST_OBSERVER_TOPIC "child-gc-request"
#define CC_REQUEST_OBSERVER_TOPIC "child-cc-request"
#define MEMORY_PRESSURE_OBSERVER_TOPIC "memory-pressure"

#define PREF_GENERAL_APPNAME_OVERRIDE "general.appname.override"
#define PREF_GENERAL_APPVERSION_OVERRIDE "general.appversion.override"
#define PREF_GENERAL_PLATFORM_OVERRIDE "general.platform.override"

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
      JSAutoRequest ar(cx);                                                    \
      for (uint32_t index = 0; index < workers.Length(); index++) {            \
        workers[index]-> _func (cx, __VA_ARGS__);                              \
      }                                                                        \
    }                                                                          \
  PR_END_MACRO


#define PREF_JS_OPTIONS_PREFIX "javascript.options."
#define PREF_WORKERS_OPTIONS_PREFIX PREF_WORKERS_PREFIX "options."
#define PREF_MEM_OPTIONS_PREFIX "mem."
#define PREF_GCZEAL "gcZeal"

#if !(defined(DEBUG) || defined(MOZ_ENABLE_JS_DUMP))
#define DUMP_CONTROLLED_BY_PREF 1
#define PREF_DOM_WINDOW_DUMP_ENABLED "browser.dom.window.dump.enabled"
#endif

#define PREF_DOM_CACHES_ENABLED        "dom.caches.enabled"
#define PREF_WORKERS_LATEST_JS_VERSION "dom.workers.latestJSVersion"
#define PREF_INTL_ACCEPT_LANGUAGES     "intl.accept_languages"

namespace {

const uint32_t kNoIndex = uint32_t(-1);

const JS::ContextOptions kRequiredContextOptions =
  JS::ContextOptions().setDontReportUncaught(true);

uint32_t gMaxWorkersPerDomain = MAX_WORKERS_PER_DOMAIN;


RuntimeService* gRuntimeService = nullptr;


bool gRuntimeServiceDuringInit = false;

#ifdef ENABLE_TESTS
bool gTestPBackground = false;
#endif 

class LiteralRebindingCString : public nsDependentCString
{
public:
  template<int N>
  void RebindLiteral(const char (&aStr)[N])
  {
    Rebind(aStr, N-1);
  }
};

template <typename T>
struct PrefTraits;

template <>
struct PrefTraits<bool>
{
  typedef bool PrefValueType;

  static const PrefValueType kDefaultValue = false;

  static inline PrefValueType
  Get(const char* aPref)
  {
    AssertIsOnMainThread();
    return Preferences::GetBool(aPref);
  }

  static inline bool
  Exists(const char* aPref)
  {
    AssertIsOnMainThread();
    return Preferences::GetType(aPref) == nsIPrefBranch::PREF_BOOL;
  }
};

template <>
struct PrefTraits<int32_t>
{
  typedef int32_t PrefValueType;

  static inline PrefValueType
  Get(const char* aPref)
  {
    AssertIsOnMainThread();
    return Preferences::GetInt(aPref);
  }

  static inline bool
  Exists(const char* aPref)
  {
    AssertIsOnMainThread();
    return Preferences::GetType(aPref) == nsIPrefBranch::PREF_INT;
  }
};

template <typename T>
T
GetWorkerPref(const nsACString& aPref,
              const T aDefault = PrefTraits<T>::kDefaultValue)
{
  AssertIsOnMainThread();

  typedef PrefTraits<T> PrefHelper;

  T result;

  nsAutoCString prefName;
  prefName.AssignLiteral(PREF_WORKERS_OPTIONS_PREFIX);
  prefName.Append(aPref);

  if (PrefHelper::Exists(prefName.get())) {
    result = PrefHelper::Get(prefName.get());
  }
  else {
    prefName.AssignLiteral(PREF_JS_OPTIONS_PREFIX);
    prefName.Append(aPref);

    if (PrefHelper::Exists(prefName.get())) {
      result = PrefHelper::Get(prefName.get());
    }
    else {
      result = aDefault;
    }
  }

  return result;
}




void
GenerateSharedWorkerKey(const nsACString& aScriptSpec, const nsACString& aName,
                        WorkerType aWorkerType, nsCString& aKey)
{
  aKey.Truncate();
  NS_NAMED_LITERAL_CSTRING(sharedPrefix, "shared|");
  NS_NAMED_LITERAL_CSTRING(servicePrefix, "service|");
  MOZ_ASSERT(servicePrefix.Length() > sharedPrefix.Length());
  MOZ_ASSERT(aWorkerType == WorkerTypeShared ||
             aWorkerType == WorkerTypeService);
  aKey.SetCapacity(servicePrefix.Length() + aScriptSpec.Length() +
                   aName.Length() + 1);

  aKey.Append(aWorkerType == WorkerTypeService ? servicePrefix : sharedPrefix);

  nsACString::const_iterator start, end;
  aName.BeginReading(start);
  aName.EndReading(end);
  for (; start != end; ++start) {
    if (*start == '|') {
      aKey.AppendASCII("||");
    } else {
      aKey.Append(*start);
    }
  }

  aKey.Append('|');
  aKey.Append(aScriptSpec);
}

void
LoadRuntimeOptions(const char* aPrefName, void* )
{
  AssertIsOnMainThread();

  RuntimeService* rts = RuntimeService::GetService();
  if (!rts) {
    
    return;
  }

  const nsDependentCString prefName(aPrefName);

  
  
  if (StringBeginsWith(prefName,
                       NS_LITERAL_CSTRING(PREF_JS_OPTIONS_PREFIX
                                          PREF_MEM_OPTIONS_PREFIX)) ||
      StringBeginsWith(prefName,
                       NS_LITERAL_CSTRING(PREF_WORKERS_OPTIONS_PREFIX
                                          PREF_MEM_OPTIONS_PREFIX))) {
    return;
  }

#ifdef JS_GC_ZEAL
  if (prefName.EqualsLiteral(PREF_JS_OPTIONS_PREFIX PREF_GCZEAL) ||
      prefName.EqualsLiteral(PREF_WORKERS_OPTIONS_PREFIX PREF_GCZEAL)) {
    return;
  }
#endif

  
  JS::RuntimeOptions runtimeOptions;
  if (GetWorkerPref<bool>(NS_LITERAL_CSTRING("asmjs"))) {
    runtimeOptions.setAsmJS(true);
  }
  if (GetWorkerPref<bool>(NS_LITERAL_CSTRING("baselinejit"))) {
    runtimeOptions.setBaseline(true);
  }
  if (GetWorkerPref<bool>(NS_LITERAL_CSTRING("ion"))) {
    runtimeOptions.setIon(true);
  }
  if (GetWorkerPref<bool>(NS_LITERAL_CSTRING("native_regexp"))) {
    runtimeOptions.setNativeRegExp(true);
  }
  if (GetWorkerPref<bool>(NS_LITERAL_CSTRING("werror"))) {
    runtimeOptions.setWerror(true);
  }

  if (GetWorkerPref<bool>(NS_LITERAL_CSTRING("strict"))) {
    runtimeOptions.setExtraWarnings(true);
  }

  RuntimeService::SetDefaultRuntimeOptions(runtimeOptions);

  if (rts) {
    rts->UpdateAllWorkerRuntimeOptions();
  }
}

#ifdef JS_GC_ZEAL
void
LoadGCZealOptions(const char* , void* )
{
  AssertIsOnMainThread();

  RuntimeService* rts = RuntimeService::GetService();
  if (!rts) {
    
    return;
  }

  int32_t gczeal = GetWorkerPref<int32_t>(NS_LITERAL_CSTRING(PREF_GCZEAL), -1);
  if (gczeal < 0) {
    gczeal = 0;
  }

  int32_t frequency =
    GetWorkerPref<int32_t>(NS_LITERAL_CSTRING("gcZeal.frequency"), -1);
  if (frequency < 0) {
    frequency = JS_DEFAULT_ZEAL_FREQ;
  }

  RuntimeService::SetDefaultGCZeal(uint8_t(gczeal), uint32_t(frequency));

  if (rts) {
    rts->UpdateAllWorkerGCZeal();
  }
}
#endif

void
UpdateCommonJSGCMemoryOption(RuntimeService* aRuntimeService,
                             const nsACString& aPrefName, JSGCParamKey aKey)
{
  AssertIsOnMainThread();
  NS_ASSERTION(!aPrefName.IsEmpty(), "Empty pref name!");

  int32_t prefValue = GetWorkerPref(aPrefName, -1);
  uint32_t value =
    (prefValue < 0 || prefValue >= 10000) ? 0 : uint32_t(prefValue);

  RuntimeService::SetDefaultJSGCSettings(aKey, value);

  if (aRuntimeService) {
    aRuntimeService->UpdateAllWorkerMemoryParameter(aKey, value);
  }
}

void
UpdateOtherJSGCMemoryOption(RuntimeService* aRuntimeService,
                            JSGCParamKey aKey, uint32_t aValue)
{
  AssertIsOnMainThread();

  RuntimeService::SetDefaultJSGCSettings(aKey, aValue);

  if (aRuntimeService) {
    aRuntimeService->UpdateAllWorkerMemoryParameter(aKey, aValue);
  }
}


void
LoadJSGCMemoryOptions(const char* aPrefName, void* )
{
  AssertIsOnMainThread();

  RuntimeService* rts = RuntimeService::GetService();

  if (!rts) {
    
    return;
  }

  NS_NAMED_LITERAL_CSTRING(jsPrefix, PREF_JS_OPTIONS_PREFIX);
  NS_NAMED_LITERAL_CSTRING(workersPrefix, PREF_WORKERS_OPTIONS_PREFIX);

  const nsDependentCString fullPrefName(aPrefName);

  
  
  nsDependentCSubstring memPrefName;
  if (StringBeginsWith(fullPrefName, jsPrefix)) {
    memPrefName.Rebind(fullPrefName, jsPrefix.Length());
  }
  else if (StringBeginsWith(fullPrefName, workersPrefix)) {
    memPrefName.Rebind(fullPrefName, workersPrefix.Length());
  }
  else {
    NS_ERROR("Unknown pref name!");
    return;
  }

#ifdef DEBUG
  
  
  if (!rts) {
    NS_ASSERTION(memPrefName.EqualsLiteral(PREF_MEM_OPTIONS_PREFIX), "Huh?!");
  }
#endif

  
  
  for (uint32_t index = !gRuntimeServiceDuringInit
                          ? JSSettings::kGCSettingsArraySize - 1 : 0;
       index < JSSettings::kGCSettingsArraySize;
       index++) {
    LiteralRebindingCString matchName;

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX "max");
    if (memPrefName == matchName || (gRuntimeServiceDuringInit && index == 0)) {
      int32_t prefValue = GetWorkerPref(matchName, -1);
      uint32_t value = (prefValue <= 0 || prefValue >= 0x1000) ?
                       uint32_t(-1) :
                       uint32_t(prefValue) * 1024 * 1024;
      UpdateOtherJSGCMemoryOption(rts, JSGC_MAX_BYTES, value);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX "high_water_mark");
    if (memPrefName == matchName || (gRuntimeServiceDuringInit && index == 1)) {
      int32_t prefValue = GetWorkerPref(matchName, 128);
      UpdateOtherJSGCMemoryOption(rts, JSGC_MAX_MALLOC_BYTES,
                                 uint32_t(prefValue) * 1024 * 1024);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX
                            "gc_high_frequency_time_limit_ms");
    if (memPrefName == matchName || (gRuntimeServiceDuringInit && index == 2)) {
      UpdateCommonJSGCMemoryOption(rts, matchName,
                                   JSGC_HIGH_FREQUENCY_TIME_LIMIT);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX
                            "gc_low_frequency_heap_growth");
    if (memPrefName == matchName || (gRuntimeServiceDuringInit && index == 3)) {
      UpdateCommonJSGCMemoryOption(rts, matchName,
                                   JSGC_LOW_FREQUENCY_HEAP_GROWTH);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX
                            "gc_high_frequency_heap_growth_min");
    if (memPrefName == matchName || (gRuntimeServiceDuringInit && index == 4)) {
      UpdateCommonJSGCMemoryOption(rts, matchName,
                                   JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MIN);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX
                            "gc_high_frequency_heap_growth_max");
    if (memPrefName == matchName || (gRuntimeServiceDuringInit && index == 5)) {
      UpdateCommonJSGCMemoryOption(rts, matchName,
                                   JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MAX);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX
                            "gc_high_frequency_low_limit_mb");
    if (memPrefName == matchName || (gRuntimeServiceDuringInit && index == 6)) {
      UpdateCommonJSGCMemoryOption(rts, matchName,
                                   JSGC_HIGH_FREQUENCY_LOW_LIMIT);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX
                            "gc_high_frequency_high_limit_mb");
    if (memPrefName == matchName || (gRuntimeServiceDuringInit && index == 7)) {
      UpdateCommonJSGCMemoryOption(rts, matchName,
                                   JSGC_HIGH_FREQUENCY_HIGH_LIMIT);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX
                            "gc_allocation_threshold_mb");
    if (memPrefName == matchName || (gRuntimeServiceDuringInit && index == 8)) {
      UpdateCommonJSGCMemoryOption(rts, matchName, JSGC_ALLOCATION_THRESHOLD);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX "gc_incremental_slice_ms");
    if (memPrefName == matchName || (gRuntimeServiceDuringInit && index == 9)) {
      int32_t prefValue = GetWorkerPref(matchName, -1);
      uint32_t value =
        (prefValue <= 0 || prefValue >= 100000) ? 0 : uint32_t(prefValue);
      UpdateOtherJSGCMemoryOption(rts, JSGC_SLICE_TIME_BUDGET, value);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX "gc_dynamic_heap_growth");
    if (memPrefName == matchName ||
        (gRuntimeServiceDuringInit && index == 10)) {
      bool prefValue = GetWorkerPref(matchName, false);
      UpdateOtherJSGCMemoryOption(rts, JSGC_DYNAMIC_HEAP_GROWTH,
                                 prefValue ? 0 : 1);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX "gc_dynamic_mark_slice");
    if (memPrefName == matchName ||
        (gRuntimeServiceDuringInit && index == 11)) {
      bool prefValue = GetWorkerPref(matchName, false);
      UpdateOtherJSGCMemoryOption(rts, JSGC_DYNAMIC_MARK_SLICE,
                                 prefValue ? 0 : 1);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX "gc_min_empty_chunk_count");
    if (memPrefName == matchName ||
        (gRuntimeServiceDuringInit && index == 12)) {
      UpdateCommonJSGCMemoryOption(rts, matchName, JSGC_MIN_EMPTY_CHUNK_COUNT);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX "gc_max_empty_chunk_count");
    if (memPrefName == matchName ||
        (gRuntimeServiceDuringInit && index == 13)) {
      UpdateCommonJSGCMemoryOption(rts, matchName, JSGC_MAX_EMPTY_CHUNK_COUNT);
      continue;
    }

    matchName.RebindLiteral(PREF_MEM_OPTIONS_PREFIX "gc_compacting");
    if (memPrefName == matchName ||
        (gRuntimeServiceDuringInit && index == 14)) {
      bool prefValue = GetWorkerPref(matchName, false);
      UpdateOtherJSGCMemoryOption(rts, JSGC_COMPACTING_ENABLED,
                                 prefValue ? 0 : 1);
      continue;
    }

#ifdef DEBUG
    nsAutoCString message("Workers don't support the 'mem.");
    message.Append(memPrefName);
    message.AppendLiteral("' preference!");
    NS_WARNING(message.get());
#endif
  }
}

void
ErrorReporter(JSContext* aCx, const char* aMessage, JSErrorReport* aReport)
{
  WorkerPrivate* worker = GetWorkerPrivateFromContext(aCx);
  MOZ_ASSERT(worker);

  return worker->ReportError(aCx, aMessage, aReport);
}

bool
InterruptCallback(JSContext* aCx)
{
  WorkerPrivate* worker = GetWorkerPrivateFromContext(aCx);
  MOZ_ASSERT(worker);

  
  profiler_js_operation_callback();

  return worker->InterruptCallback(aCx);
}

class LogViolationDetailsRunnable final : public nsRunnable
{
  WorkerPrivate* mWorkerPrivate;
  nsCOMPtr<nsIEventTarget> mSyncLoopTarget;
  nsString mFileName;
  uint32_t mLineNum;

public:
  LogViolationDetailsRunnable(WorkerPrivate* aWorker,
                              const nsString& aFileName,
                              uint32_t aLineNum)
  : mWorkerPrivate(aWorker), mFileName(aFileName), mLineNum(aLineNum)
  {
    MOZ_ASSERT(aWorker);
  }

  NS_DECL_ISUPPORTS_INHERITED

  bool
  Dispatch(JSContext* aCx)
  {
    AutoSyncLoopHolder syncLoop(mWorkerPrivate);

    mSyncLoopTarget = syncLoop.EventTarget();
    MOZ_ASSERT(mSyncLoopTarget);

    if (NS_FAILED(NS_DispatchToMainThread(this))) {
      JS_ReportError(aCx, "Failed to dispatch to main thread!");
      return false;
    }

    return syncLoop.Run();
  }

private:
  ~LogViolationDetailsRunnable() {}

  NS_DECL_NSIRUNNABLE
};

bool
ContentSecurityPolicyAllows(JSContext* aCx)
{
  WorkerPrivate* worker = GetWorkerPrivateFromContext(aCx);
  worker->AssertIsOnWorkerThread();

  if (worker->GetReportCSPViolations()) {
    nsString fileName;
    uint32_t lineNum = 0;

    JS::AutoFilename file;
    if (JS::DescribeScriptedCaller(aCx, &file, &lineNum) && file.get()) {
      fileName = NS_ConvertUTF8toUTF16(file.get());
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
      MOZ_CRASH("Unknown type flag!");
  }
}

static nsIPrincipal*
GetPrincipalForAsmJSCacheOp()
{
  WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
  if (!workerPrivate) {
    return nullptr;
  }

  
  
  return workerPrivate->GetPrincipalDontAssertMainThread();
}

static bool
AsmJSCacheOpenEntryForRead(JS::Handle<JSObject*> aGlobal,
                           const char16_t* aBegin,
                           const char16_t* aLimit,
                           size_t* aSize,
                           const uint8_t** aMemory,
                           intptr_t *aHandle)
{
  nsIPrincipal* principal = GetPrincipalForAsmJSCacheOp();
  if (!principal) {
    return false;
  }

  return asmjscache::OpenEntryForRead(principal, aBegin, aLimit, aSize, aMemory,
                                      aHandle);
}

static JS::AsmJSCacheResult
AsmJSCacheOpenEntryForWrite(JS::Handle<JSObject*> aGlobal,
                            bool aInstalled,
                            const char16_t* aBegin,
                            const char16_t* aEnd,
                            size_t aSize,
                            uint8_t** aMemory,
                            intptr_t* aHandle)
{
  nsIPrincipal* principal = GetPrincipalForAsmJSCacheOp();
  if (!principal) {
    return JS::AsmJSCache_InternalError;
  }

  return asmjscache::OpenEntryForWrite(principal, aInstalled, aBegin, aEnd,
                                       aSize, aMemory, aHandle);
}

struct WorkerThreadRuntimePrivate : public PerThreadAtomCache
{
  WorkerPrivate* mWorkerPrivate;
};

JSContext*
CreateJSContextForWorker(WorkerPrivate* aWorkerPrivate, JSRuntime* aRuntime)
{
  aWorkerPrivate->AssertIsOnWorkerThread();
  NS_ASSERTION(!aWorkerPrivate->GetJSContext(), "Already has a context!");

  JSSettings settings;
  aWorkerPrivate->CopyJSSettings(settings);

  JS::RuntimeOptionsRef(aRuntime) = settings.runtimeOptions;

  JSSettings::JSGCSettingsArray& gcSettings = settings.gcSettings;

  
  for (uint32_t index = 0; index < ArrayLength(gcSettings); index++) {
    const JSSettings::JSGCSetting& setting = gcSettings[index];
    if (setting.IsSet()) {
      NS_ASSERTION(setting.value, "Can't handle 0 values!");
      JS_SetGCParameter(aRuntime, setting.key, setting.value);
    }
  }

  JS_SetNativeStackQuota(aRuntime, WORKER_CONTEXT_NATIVE_STACK_LIMIT);

  
  static const JSSecurityCallbacks securityCallbacks = {
    ContentSecurityPolicyAllows
  };
  JS_SetSecurityCallbacks(aRuntime, &securityCallbacks);

  
  static const JS::AsmJSCacheOps asmJSCacheOps = {
    AsmJSCacheOpenEntryForRead,
    asmjscache::CloseEntryForRead,
    AsmJSCacheOpenEntryForWrite,
    asmjscache::CloseEntryForWrite,
    asmjscache::GetBuildId
  };
  JS::SetAsmJSCacheOps(aRuntime, &asmJSCacheOps);

  JSContext* workerCx = JS_NewContext(aRuntime, 0);
  if (!workerCx) {
    NS_WARNING("Could not create new context!");
    return nullptr;
  }

  auto rtPrivate = new WorkerThreadRuntimePrivate();
  memset(rtPrivate, 0, sizeof(WorkerThreadRuntimePrivate));
  rtPrivate->mWorkerPrivate = aWorkerPrivate;
  JS_SetRuntimePrivate(aRuntime, rtPrivate);

  JS_SetErrorReporter(aRuntime, ErrorReporter);

  JS_SetInterruptCallback(aRuntime, InterruptCallback);

  js::SetCTypesActivityCallback(aRuntime, CTypesActivityCallback);

  JS::ContextOptionsRef(workerCx) = kRequiredContextOptions;

#ifdef JS_GC_ZEAL
  JS_SetGCZeal(workerCx, settings.gcZeal, settings.gcZealFrequency);
#endif

  return workerCx;
}

static bool
PreserveWrapper(JSContext *cx, JSObject *obj)
{
    MOZ_ASSERT(cx);
    MOZ_ASSERT(obj);
    MOZ_ASSERT(mozilla::dom::IsDOMObject(obj));

    return mozilla::dom::TryPreserveWrapper(obj);
}

class DebuggeeGlobalSecurityWrapper : public js::CrossCompartmentSecurityWrapper {
public:
  DebuggeeGlobalSecurityWrapper()
  : js::CrossCompartmentSecurityWrapper(CROSS_COMPARTMENT, false)
  {
  }

  bool enter(JSContext* cx, JS::HandleObject wrapper, JS::HandleId id,
             js::Wrapper::Action act, bool* bp) const
  {
    *bp = false;
    return false;
  }

  static const DebuggeeGlobalSecurityWrapper singleton;
};

const DebuggeeGlobalSecurityWrapper DebuggeeGlobalSecurityWrapper::singleton;

JSObject*
Wrap(JSContext *cx, JS::HandleObject existing, JS::HandleObject obj)
{
  JSObject* targetGlobal = JS::CurrentGlobalOrNull(cx);
  if (!IsDebuggerGlobal(targetGlobal) && !IsDebuggerSandbox(targetGlobal)) {
    MOZ_CRASH("There should be no edges from the debuggee to the debugger.");
  }

  JSObject* originGlobal = js::GetGlobalForObjectCrossCompartment(obj);

  const js::Wrapper* wrapper = nullptr;
  if (IsDebuggerGlobal(originGlobal) || IsDebuggerSandbox(originGlobal)) {
    wrapper = &js::CrossCompartmentWrapper::singleton;
  } else {
    if (obj != originGlobal) {
      MOZ_CRASH("The should be only edges from the debugger to the debuggee global.");
    }

    wrapper = &DebuggeeGlobalSecurityWrapper::singleton;
  }

  if (existing) {
    js::Wrapper::Renew(cx, existing, obj, wrapper);
  }
  return js::Wrapper::New(cx, obj, wrapper);
}

static const JSWrapObjectCallbacks WrapObjectCallbacks = {
  Wrap,
  nullptr,
};

class WorkerJSRuntime : public mozilla::CycleCollectedJSRuntime
{
public:
  
  
  WorkerJSRuntime(JSRuntime* aParentRuntime, WorkerPrivate* aWorkerPrivate)
    : CycleCollectedJSRuntime(aParentRuntime,
                              WORKER_DEFAULT_RUNTIME_HEAPSIZE,
                              WORKER_DEFAULT_NURSERY_SIZE),
    mWorkerPrivate(aWorkerPrivate)
  {
    js::SetPreserveWrapperCallback(Runtime(), PreserveWrapper);
    JS_InitDestroyPrincipalsCallback(Runtime(), DestroyWorkerPrincipals);
    JS_SetWrapObjectCallbacks(Runtime(), &WrapObjectCallbacks);
  }

  ~WorkerJSRuntime()
  {
    auto rtPrivate = static_cast<WorkerThreadRuntimePrivate*>(JS_GetRuntimePrivate(Runtime()));
    delete rtPrivate;
    JS_SetRuntimePrivate(Runtime(), nullptr);

    
    
    
    
    nsCycleCollector_shutdown();

    
    
    mWorkerPrivate = nullptr;
  }

  virtual void
  PrepareForForgetSkippable() override
  {
  }

  virtual void
  BeginCycleCollectionCallback() override
  {
  }

  virtual void
  EndCycleCollectionCallback(CycleCollectorResults &aResults) override
  {
  }

  void
  DispatchDeferredDeletion(bool aContinuation) override
  {
    MOZ_ASSERT(!aContinuation);

    
    nsCycleCollector_doDeferredDeletion();
  }

  virtual void CustomGCCallback(JSGCStatus aStatus) override
  {
    if (!mWorkerPrivate) {
      
      return;
    }

    mWorkerPrivate->AssertIsOnWorkerThread();

    if (aStatus == JSGC_END) {
      nsCycleCollector_collect(nullptr);
    }
  }

private:
  WorkerPrivate* mWorkerPrivate;
};

#ifdef ENABLE_TESTS

class TestPBackgroundCreateCallback final :
  public nsIIPCBackgroundChildCreateCallback
{
public:
  NS_DECL_ISUPPORTS

  virtual void
  ActorCreated(PBackgroundChild* aActor) override
  {
    MOZ_RELEASE_ASSERT(aActor);
  }

  virtual void
  ActorFailed() override
  {
    MOZ_CRASH("TestPBackground() should not fail "
              "GetOrCreateForCurrentThread()");
  }

private:
  ~TestPBackgroundCreateCallback()
  { }
};

NS_IMPL_ISUPPORTS(TestPBackgroundCreateCallback,
                  nsIIPCBackgroundChildCreateCallback);

void
TestPBackground()
{
  using namespace mozilla::ipc;

  if (gTestPBackground) {
    
    uint32_t testValue;
    size_t randomSize = PR_GetRandomNoise(&testValue, sizeof(testValue));
    MOZ_RELEASE_ASSERT(randomSize == sizeof(testValue));
    nsCString testStr;
    testStr.AppendInt(testValue);
    testStr.AppendInt(reinterpret_cast<int64_t>(PR_GetCurrentThread()));
    PBackgroundChild* existingBackgroundChild =
      BackgroundChild::GetForCurrentThread();
    MOZ_RELEASE_ASSERT(existingBackgroundChild);
    bool ok = existingBackgroundChild->SendPBackgroundTestConstructor(testStr);
    MOZ_RELEASE_ASSERT(ok);
  }
}

#endif 

class WorkerBackgroundChildCallback final :
  public nsIIPCBackgroundChildCreateCallback
{
  bool* mDone;

public:
  explicit WorkerBackgroundChildCallback(bool* aDone)
  : mDone(aDone)
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(mDone);
  }

  NS_DECL_ISUPPORTS

private:
  ~WorkerBackgroundChildCallback()
  { }

  virtual void
  ActorCreated(PBackgroundChild* aActor) override
  {
    *mDone = true;
  }

  virtual void
  ActorFailed() override
  {
    *mDone = true;
  }
};

class WorkerThreadPrimaryRunnable final : public nsRunnable
{
  WorkerPrivate* mWorkerPrivate;
  nsRefPtr<WorkerThread> mThread;
  JSRuntime* mParentRuntime;

  class FinishedRunnable final : public nsRunnable
  {
    nsRefPtr<WorkerThread> mThread;

  public:
    explicit FinishedRunnable(already_AddRefed<WorkerThread> aThread)
    : mThread(aThread)
    {
      MOZ_ASSERT(mThread);
    }

    NS_DECL_ISUPPORTS_INHERITED

  private:
    ~FinishedRunnable()
    { }

    NS_DECL_NSIRUNNABLE
  };

public:
  WorkerThreadPrimaryRunnable(WorkerPrivate* aWorkerPrivate,
                              WorkerThread* aThread,
                              JSRuntime* aParentRuntime)
  : mWorkerPrivate(aWorkerPrivate), mThread(aThread), mParentRuntime(aParentRuntime)
  {
    MOZ_ASSERT(aWorkerPrivate);
    MOZ_ASSERT(aThread);
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~WorkerThreadPrimaryRunnable()
  { }

  nsresult
  SynchronouslyCreatePBackground();

  NS_DECL_NSIRUNNABLE
};

class WorkerTaskRunnable final : public WorkerRunnable
{
  nsRefPtr<WorkerTask> mTask;

public:
  WorkerTaskRunnable(WorkerPrivate* aWorkerPrivate, WorkerTask* aTask)
  : WorkerRunnable(aWorkerPrivate, WorkerThreadUnchangedBusyCount), mTask(aTask)
  {
    MOZ_ASSERT(aTask);
  }

private:
  virtual bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    
    return true;
  }

  virtual void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) override
  {
    
  }

  virtual bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    return mTask->RunTask(aCx);
  }
};

void
PrefLanguagesChanged(const char* , void* )
{
  AssertIsOnMainThread();

  nsTArray<nsString> languages;
  Navigator::GetAcceptLanguages(languages);

  RuntimeService* runtime = RuntimeService::GetService();
  if (runtime) {
    runtime->UpdateAllWorkerLanguages(languages);
  }
}

void
AppNameOverrideChanged(const char* , void* )
{
  AssertIsOnMainThread();

  const nsAdoptingString& override =
    mozilla::Preferences::GetString(PREF_GENERAL_APPNAME_OVERRIDE);

  RuntimeService* runtime = RuntimeService::GetService();
  if (runtime) {
    runtime->UpdateAppNameOverridePreference(override);
  }
}

void
AppVersionOverrideChanged(const char* , void* )
{
  AssertIsOnMainThread();

  const nsAdoptingString& override =
    mozilla::Preferences::GetString(PREF_GENERAL_APPVERSION_OVERRIDE);

  RuntimeService* runtime = RuntimeService::GetService();
  if (runtime) {
    runtime->UpdateAppVersionOverridePreference(override);
  }
}

void
PlatformOverrideChanged(const char* , void* )
{
  AssertIsOnMainThread();

  const nsAdoptingString& override =
    mozilla::Preferences::GetString(PREF_GENERAL_PLATFORM_OVERRIDE);

  RuntimeService* runtime = RuntimeService::GetService();
  if (runtime) {
    runtime->UpdatePlatformOverridePreference(override);
  }
}

class BackgroundChildCallback final
  : public nsIIPCBackgroundChildCreateCallback
{
public:
  BackgroundChildCallback()
  {
    AssertIsOnMainThread();
  }

  NS_DECL_ISUPPORTS

private:
  ~BackgroundChildCallback()
  {
    AssertIsOnMainThread();
  }

  virtual void
  ActorCreated(PBackgroundChild* aActor) override
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(aActor);
  }

  virtual void
  ActorFailed() override
  {
    AssertIsOnMainThread();
    MOZ_CRASH("Unable to connect PBackground actor for the main thread!");
  }
};

NS_IMPL_ISUPPORTS(BackgroundChildCallback, nsIIPCBackgroundChildCreateCallback)

} 

BEGIN_WORKERS_NAMESPACE

void
CancelWorkersForWindow(nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();
  RuntimeService* runtime = RuntimeService::GetService();
  if (runtime) {
    runtime->CancelWorkersForWindow(aWindow);
  }
}

void
SuspendWorkersForWindow(nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();
  RuntimeService* runtime = RuntimeService::GetService();
  if (runtime) {
    runtime->SuspendWorkersForWindow(aWindow);
  }
}

void
ResumeWorkersForWindow(nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();
  RuntimeService* runtime = RuntimeService::GetService();
  if (runtime) {
    runtime->ResumeWorkersForWindow(aWindow);
  }
}

WorkerCrossThreadDispatcher::WorkerCrossThreadDispatcher(
                                                  WorkerPrivate* aWorkerPrivate)
: mMutex("WorkerCrossThreadDispatcher::mMutex"),
  mWorkerPrivate(aWorkerPrivate)
{
  MOZ_ASSERT(aWorkerPrivate);
}

bool
WorkerCrossThreadDispatcher::PostTask(WorkerTask* aTask)
{
  MOZ_ASSERT(aTask);

  MutexAutoLock lock(mMutex);

  if (!mWorkerPrivate) {
    NS_WARNING("Posted a task to a WorkerCrossThreadDispatcher that is no "
               "longer accepting tasks!");
    return false;
  }

  nsRefPtr<WorkerTaskRunnable> runnable =
    new WorkerTaskRunnable(mWorkerPrivate, aTask);
  return runnable->Dispatch(nullptr);
}

WorkerPrivate*
GetWorkerPrivateFromContext(JSContext* aCx)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aCx);

  JSRuntime* rt = JS_GetRuntime(aCx);
  MOZ_ASSERT(rt);

  void* rtPrivate = JS_GetRuntimePrivate(rt);
  MOZ_ASSERT(rtPrivate);

  return static_cast<WorkerThreadRuntimePrivate*>(rtPrivate)->mWorkerPrivate;
}

WorkerPrivate*
GetCurrentThreadWorkerPrivate()
{
  MOZ_ASSERT(!NS_IsMainThread());

  CycleCollectedJSRuntime* ccrt = CycleCollectedJSRuntime::Get();
  if (!ccrt) {
    return nullptr;
  }

  JSRuntime* rt = ccrt->Runtime();
  MOZ_ASSERT(rt);

  void* rtPrivate = JS_GetRuntimePrivate(rt);
  MOZ_ASSERT(rtPrivate);

  return static_cast<WorkerThreadRuntimePrivate*>(rtPrivate)->mWorkerPrivate;
}

bool
IsCurrentThreadRunningChromeWorker()
{
  return GetCurrentThreadWorkerPrivate()->UsesSystemPrincipal();
}

JSContext*
GetCurrentThreadJSContext()
{
  return GetCurrentThreadWorkerPrivate()->GetJSContext();
}

END_WORKERS_NAMESPACE

struct RuntimeService::IdleThreadInfo
{
  nsRefPtr<WorkerThread> mThread;
  mozilla::TimeStamp mExpirationTime;
};


JSSettings RuntimeService::sDefaultJSSettings;
bool RuntimeService::sDefaultPreferences[WORKERPREF_COUNT] = { false };

RuntimeService::RuntimeService()
: mMutex("RuntimeService::mMutex"), mObserved(false),
  mShuttingDown(false), mNavigatorPropertiesLoaded(false)
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
    
    gRuntimeService = new RuntimeService();
    if (NS_FAILED(gRuntimeService->Init())) {
      NS_WARNING("Failed to initialize!");
      gRuntimeService->Cleanup();
      gRuntimeService = nullptr;
      return nullptr;
    }

#ifdef ENABLE_TESTS
    gTestPBackground = mozilla::Preferences::GetBool("pbackground.testing", false);
#endif 
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

  nsCString sharedWorkerScriptSpec;

  const bool isServiceWorker = aWorkerPrivate->IsServiceWorker();
  if (isServiceWorker) {
    AssertIsOnMainThread();
    Telemetry::Accumulate(Telemetry::SERVICE_WORKER_SPAWN_ATTEMPTS, 1);
  }

  bool isSharedOrServiceWorker = aWorkerPrivate->IsSharedWorker() ||
                                 aWorkerPrivate->IsServiceWorker();
  if (isSharedOrServiceWorker) {
    AssertIsOnMainThread();

    nsCOMPtr<nsIURI> scriptURI = aWorkerPrivate->GetResolvedScriptURI();
    NS_ASSERTION(scriptURI, "Null script URI!");

    nsresult rv = scriptURI->GetSpec(sharedWorkerScriptSpec);
    if (NS_FAILED(rv)) {
      NS_WARNING("GetSpec failed?!");
      xpc::Throw(aCx, rv);
      return false;
    }

    NS_ASSERTION(!sharedWorkerScriptSpec.IsEmpty(), "Empty spec!");
  }

  bool exemptFromPerDomainMax = false;
  if (aWorkerPrivate->IsServiceWorker()) {
    AssertIsOnMainThread();
    exemptFromPerDomainMax = Preferences::GetBool("dom.serviceWorkers.exemptFromPerDomainMax",
                                                  false);
  }

  const nsCString& domain = aWorkerPrivate->Domain();

  WorkerDomainInfo* domainInfo;
  bool queued = false;
  {
    MutexAutoLock lock(mMutex);

    if (!mDomainMap.Get(domain, &domainInfo)) {
      NS_ASSERTION(!parent, "Shouldn't have a parent here!");

      domainInfo = new WorkerDomainInfo();
      domainInfo->mDomain = domain;
      mDomainMap.Put(domain, domainInfo);
    }

    queued = gMaxWorkersPerDomain &&
             domainInfo->ActiveWorkerCount() >= gMaxWorkersPerDomain &&
             !domain.IsEmpty() &&
             !exemptFromPerDomainMax;

    if (queued) {
      domainInfo->mQueuedWorkers.AppendElement(aWorkerPrivate);
      if (isServiceWorker) {
        AssertIsOnMainThread();
        
        
        
        
        nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                        NS_LITERAL_CSTRING("DOM"),
                                        aWorkerPrivate->GetDocument(),
                                        nsContentUtils::eDOM_PROPERTIES,
                                        "HittingMaxWorkersPerDomain");
        Telemetry::Accumulate(Telemetry::SERVICE_WORKER_SPAWN_GETS_QUEUED, 1);
      }
    }
    else if (parent) {
      domainInfo->mChildWorkerCount++;
    }
    else {
      domainInfo->mActiveWorkers.AppendElement(aWorkerPrivate);
    }

    if (isSharedOrServiceWorker) {
      const nsCString& sharedWorkerName = aWorkerPrivate->SharedWorkerName();

      nsAutoCString key;
      GenerateSharedWorkerKey(sharedWorkerScriptSpec, sharedWorkerName,
                              aWorkerPrivate->Type(), key);
      MOZ_ASSERT(!domainInfo->mSharedWorkerInfos.Get(key));

      SharedWorkerInfo* sharedWorkerInfo =
        new SharedWorkerInfo(aWorkerPrivate, sharedWorkerScriptSpec,
                             sharedWorkerName);
      domainInfo->mSharedWorkerInfos.Put(key, sharedWorkerInfo);
    }
  }

  
  if (parent) {
    if (!parent->AddChildWorker(aCx, aWorkerPrivate)) {
      UnregisterWorker(aCx, aWorkerPrivate);
      return false;
    }
  }
  else {
    if (!mNavigatorPropertiesLoaded) {
      Navigator::AppName(mNavigatorProperties.mAppName,
                         false );
      if (NS_FAILED(Navigator::GetAppVersion(mNavigatorProperties.mAppVersion,
                                             false )) ||
          NS_FAILED(Navigator::GetPlatform(mNavigatorProperties.mPlatform,
                                           false ))) {
        JS_ReportError(aCx, "Failed to load navigator strings!");
        UnregisterWorker(aCx, aWorkerPrivate);
        return false;
      }

      

      Navigator::GetAcceptLanguages(mNavigatorProperties.mLanguages);
      mNavigatorPropertiesLoaded = true;
    }

    nsPIDOMWindow* window = aWorkerPrivate->GetWindow();

    nsTArray<WorkerPrivate*>* windowArray;
    if (!mWindowMap.Get(window, &windowArray)) {
      windowArray = new nsTArray<WorkerPrivate*>(1);
      mWindowMap.Put(window, windowArray);
    }

    if (!windowArray->Contains(aWorkerPrivate)) {
      windowArray->AppendElement(aWorkerPrivate);
    } else {
      MOZ_ASSERT(aWorkerPrivate->IsSharedWorker());
    }
  }

  if (!queued && !ScheduleWorker(aCx, aWorkerPrivate)) {
    return false;
  }

  if (isServiceWorker) {
    AssertIsOnMainThread();
    Telemetry::Accumulate(Telemetry::SERVICE_WORKER_WAS_SPAWNED, 1);
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

  const nsCString& domain = aWorkerPrivate->Domain();

  WorkerPrivate* queuedWorker = nullptr;
  {
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


    if (aWorkerPrivate->IsSharedWorker() ||
        aWorkerPrivate->IsServiceWorker()) {
      MatchSharedWorkerInfo match(aWorkerPrivate);
      domainInfo->mSharedWorkerInfos.EnumerateRead(FindSharedWorkerInfo,
                                                   &match);

      if (match.mSharedWorkerInfo) {
        nsAutoCString key;
        GenerateSharedWorkerKey(match.mSharedWorkerInfo->mScriptSpec,
                                match.mSharedWorkerInfo->mName,
                                aWorkerPrivate->Type(), key);
        domainInfo->mSharedWorkerInfos.Remove(key);
      }
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
      MOZ_ASSERT(domainInfo->mQueuedWorkers.IsEmpty());
      mDomainMap.Remove(domain);
    }
  }

  if (aWorkerPrivate->IsSharedWorker()) {
    AssertIsOnMainThread();

    nsAutoTArray<nsRefPtr<SharedWorker>, 5> sharedWorkersToNotify;
    aWorkerPrivate->GetAllSharedWorkers(sharedWorkersToNotify);

    for (uint32_t index = 0; index < sharedWorkersToNotify.Length(); index++) {
      MOZ_ASSERT(sharedWorkersToNotify[index]);
      sharedWorkersToNotify[index]->NoteDeadWorker(aCx);
    }
  }

  if (parent) {
    parent->RemoveChildWorker(aCx, aWorkerPrivate);
  }
  else if (aWorkerPrivate->IsSharedWorker() || aWorkerPrivate->IsServiceWorker()) {
    mWindowMap.Enumerate(RemoveSharedWorkerFromWindowMap, aWorkerPrivate);
  }
  else {
    
    nsPIDOMWindow* window = aWorkerPrivate->GetWindow();

    nsTArray<WorkerPrivate*>* windowArray;
    MOZ_ALWAYS_TRUE(mWindowMap.Get(window, &windowArray));

    MOZ_ALWAYS_TRUE(windowArray->RemoveElement(aWorkerPrivate));

    if (windowArray->IsEmpty()) {
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

  nsRefPtr<WorkerThread> thread;
  {
    MutexAutoLock lock(mMutex);
    if (!mIdleThreadArray.IsEmpty()) {
      uint32_t index = mIdleThreadArray.Length() - 1;
      mIdleThreadArray[index].mThread.swap(thread);
      mIdleThreadArray.RemoveElementAt(index);
    }
  }

  const WorkerThreadFriendKey friendKey;

  if (!thread) {
    thread = WorkerThread::Create(friendKey);
    if (!thread) {
      UnregisterWorker(aCx, aWorkerPrivate);
      JS_ReportError(aCx, "Could not create new thread!");
      return false;
    }
  }

  int32_t priority = aWorkerPrivate->IsChromeWorker() ?
                     nsISupportsPriority::PRIORITY_NORMAL :
                     nsISupportsPriority::PRIORITY_LOW;

  if (NS_FAILED(thread->SetPriority(priority))) {
    NS_WARNING("Could not set the thread's priority!");
  }

  nsCOMPtr<nsIRunnable> runnable =
    new WorkerThreadPrimaryRunnable(aWorkerPrivate, thread, JS_GetParentRuntime(aCx));
  if (NS_FAILED(thread->DispatchPrimaryRunnable(friendKey, runnable))) {
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

  nsAutoTArray<nsRefPtr<WorkerThread>, 20> expiredThreads;
  {
    MutexAutoLock lock(runtime->mMutex);

    for (uint32_t index = 0; index < runtime->mIdleThreadArray.Length();
         index++) {
      IdleThreadInfo& info = runtime->mIdleThreadArray[index];
      if (info.mExpirationTime > now) {
        nextExpiration = info.mExpirationTime;
        break;
      }

      nsRefPtr<WorkerThread>* thread = expiredThreads.AppendElement();
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

  
  
  if (!BackgroundChild::GetForCurrentThread()) {
    nsRefPtr<BackgroundChildCallback> callback = new BackgroundChildCallback();
    if (!BackgroundChild::GetOrCreateForCurrentThread(callback)) {
      MOZ_CRASH("Unable to connect PBackground actor for the main thread!");
    }
  }

  
  if (!sDefaultJSSettings.gcSettings[0].IsSet()) {
    sDefaultJSSettings.runtimeOptions = JS::RuntimeOptions();
    sDefaultJSSettings.chrome.maxScriptRuntime = -1;
    sDefaultJSSettings.chrome.compartmentOptions.setVersion(JSVERSION_LATEST);
    sDefaultJSSettings.content.maxScriptRuntime = MAX_SCRIPT_RUN_TIME_SEC;
#ifdef JS_GC_ZEAL
    sDefaultJSSettings.gcZealFrequency = JS_DEFAULT_ZEAL_FREQ;
    sDefaultJSSettings.gcZeal = 0;
#endif
    SetDefaultJSGCSettings(JSGC_MAX_BYTES, WORKER_DEFAULT_RUNTIME_HEAPSIZE);
    SetDefaultJSGCSettings(JSGC_ALLOCATION_THRESHOLD,
                           WORKER_DEFAULT_ALLOCATION_THRESHOLD);
  }


#ifndef DUMP_CONTROLLED_BY_PREF
  sDefaultPreferences[WORKERPREF_DUMP] = true;
#endif

  mIdleThreadTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  NS_ENSURE_STATE(mIdleThreadTimer);

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE(obs, NS_ERROR_FAILURE);

  nsresult rv =
    obs->AddObserver(this, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID, false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  NS_ENSURE_SUCCESS(rv, rv);

  mObserved = true;

  if (NS_FAILED(obs->AddObserver(this, GC_REQUEST_OBSERVER_TOPIC, false))) {
    NS_WARNING("Failed to register for GC request notifications!");
  }

  if (NS_FAILED(obs->AddObserver(this, CC_REQUEST_OBSERVER_TOPIC, false))) {
    NS_WARNING("Failed to register for CC request notifications!");
  }

  if (NS_FAILED(obs->AddObserver(this, MEMORY_PRESSURE_OBSERVER_TOPIC,
                                 false))) {
    NS_WARNING("Failed to register for memory pressure notifications!");
  }

  if (NS_FAILED(obs->AddObserver(this, NS_IOSERVICE_OFFLINE_STATUS_TOPIC, false))) {
    NS_WARNING("Failed to register for offline notification event!");
  }

  MOZ_ASSERT(!gRuntimeServiceDuringInit, "This should be false!");
  gRuntimeServiceDuringInit = true;

  if (NS_FAILED(Preferences::RegisterCallback(
                                 LoadJSGCMemoryOptions,
                                 PREF_JS_OPTIONS_PREFIX PREF_MEM_OPTIONS_PREFIX,
                                 nullptr)) ||
      NS_FAILED(Preferences::RegisterCallbackAndCall(
                            LoadJSGCMemoryOptions,
                            PREF_WORKERS_OPTIONS_PREFIX PREF_MEM_OPTIONS_PREFIX,
                            nullptr)) ||
#ifdef JS_GC_ZEAL
      NS_FAILED(Preferences::RegisterCallback(
                                             LoadGCZealOptions,
                                             PREF_JS_OPTIONS_PREFIX PREF_GCZEAL,
                                             nullptr)) ||
      NS_FAILED(Preferences::RegisterCallbackAndCall(
                                        LoadGCZealOptions,
                                        PREF_WORKERS_OPTIONS_PREFIX PREF_GCZEAL,
                                        nullptr)) ||
#endif
#if DUMP_CONTROLLED_BY_PREF
      NS_FAILED(Preferences::RegisterCallbackAndCall(
                                  WorkerPrefChanged,
                                  PREF_DOM_WINDOW_DUMP_ENABLED,
                                  reinterpret_cast<void *>(WORKERPREF_DUMP))) ||
#endif
      NS_FAILED(Preferences::RegisterCallbackAndCall(
                                  WorkerPrefChanged,
                                  PREF_DOM_CACHES_ENABLED,
                                  reinterpret_cast<void *>(WORKERPREF_DOM_CACHES))) ||
      NS_FAILED(Preferences::RegisterCallback(LoadRuntimeOptions,
                                              PREF_JS_OPTIONS_PREFIX,
                                              nullptr)) ||
      NS_FAILED(Preferences::RegisterCallbackAndCall(
                                                   LoadRuntimeOptions,
                                                   PREF_WORKERS_OPTIONS_PREFIX,
                                                   nullptr)) ||
      NS_FAILED(Preferences::RegisterCallbackAndCall(PrefLanguagesChanged,
                                                     PREF_INTL_ACCEPT_LANGUAGES,
                                                     nullptr)) ||
      NS_FAILED(Preferences::RegisterCallbackAndCall(
                                                  AppNameOverrideChanged,
                                                  PREF_GENERAL_APPNAME_OVERRIDE,
                                                  nullptr)) ||
      NS_FAILED(Preferences::RegisterCallbackAndCall(
                                               AppVersionOverrideChanged,
                                               PREF_GENERAL_APPVERSION_OVERRIDE,
                                               nullptr)) ||
      NS_FAILED(Preferences::RegisterCallbackAndCall(
                                                 PlatformOverrideChanged,
                                                 PREF_GENERAL_PLATFORM_OVERRIDE,
                                                 nullptr)) ||
      NS_FAILED(Preferences::RegisterCallbackAndCall(
                                                 JSVersionChanged,
                                                 PREF_WORKERS_LATEST_JS_VERSION,
                                                 nullptr))) {
    NS_WARNING("Failed to register pref callbacks!");
  }

  MOZ_ASSERT(gRuntimeServiceDuringInit, "Should be true!");
  gRuntimeServiceDuringInit = false;

  
  
  
  if (NS_FAILED(Preferences::AddIntVarCache(
                                   &sDefaultJSSettings.content.maxScriptRuntime,
                                   PREF_MAX_SCRIPT_RUN_TIME_CONTENT,
                                   MAX_SCRIPT_RUN_TIME_SEC)) ||
      NS_FAILED(Preferences::AddIntVarCache(
                                    &sDefaultJSSettings.chrome.maxScriptRuntime,
                                    PREF_MAX_SCRIPT_RUN_TIME_CHROME, -1))) {
    NS_WARNING("Failed to register timeout cache!");
  }

  int32_t maxPerDomain = Preferences::GetInt(PREF_WORKERS_MAX_PER_DOMAIN,
                                             MAX_WORKERS_PER_DOMAIN);
  gMaxWorkersPerDomain = std::max(0, maxPerDomain);

  rv = InitOSFileConstants();
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (NS_WARN_IF(!IndexedDatabaseManager::GetOrCreate())) {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

void
RuntimeService::Shutdown()
{
  AssertIsOnMainThread();

  MOZ_ASSERT(!mShuttingDown);
  
  mShuttingDown = true;

  
  
  if (NS_FAILED(ClearWorkerDebuggerManagerListeners())) {
    NS_WARNING("Failed to clear worker debugger manager listeners!");
  }

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_WARN_IF_FALSE(obs, "Failed to get observer service?!");

  
  if (obs && NS_FAILED(obs->NotifyObservers(nullptr, WORKERS_SHUTDOWN_TOPIC,
                                            nullptr))) {
    NS_WARNING("NotifyObservers failed!");
  }

  {
    MutexAutoLock lock(mMutex);

    nsAutoTArray<WorkerPrivate*, 100> workers;
    mDomainMap.EnumerateRead(AddAllTopLevelWorkersToArray, &workers);

    if (!workers.IsEmpty()) {
      
      {
        MutexAutoUnlock unlock(mMutex);

        AutoSafeJSContext cx;
        JSAutoRequest ar(cx);

        for (uint32_t index = 0; index < workers.Length(); index++) {
          if (!workers[index]->Kill(cx)) {
            NS_WARNING("Failed to cancel worker!");
          }
        }
      }
    }
  }
}



void
RuntimeService::Cleanup()
{
  AssertIsOnMainThread();

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_WARN_IF_FALSE(obs, "Failed to get observer service?!");

  if (mIdleThreadTimer) {
    if (NS_FAILED(mIdleThreadTimer->Cancel())) {
      NS_WARNING("Failed to cancel idle timer!");
    }
    mIdleThreadTimer = nullptr;
  }

  {
    MutexAutoLock lock(mMutex);

    nsAutoTArray<WorkerPrivate*, 100> workers;
    mDomainMap.EnumerateRead(AddAllTopLevelWorkersToArray, &workers);

    if (!workers.IsEmpty()) {
      nsIThread* currentThread = NS_GetCurrentThread();
      NS_ASSERTION(currentThread, "This should never be null!");

      
      if (!mIdleThreadArray.IsEmpty()) {
        nsAutoTArray<nsRefPtr<WorkerThread>, 20> idleThreads;

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

  NS_ASSERTION(!mWindowMap.Count(), "All windows should have been released!");

  if (mObserved) {
    if (NS_FAILED(Preferences::UnregisterCallback(JSVersionChanged,
                                                  PREF_WORKERS_LATEST_JS_VERSION,
                                                  nullptr)) ||
        NS_FAILED(Preferences::UnregisterCallback(AppNameOverrideChanged,
                                                  PREF_GENERAL_APPNAME_OVERRIDE,
                                                  nullptr)) ||
        NS_FAILED(Preferences::UnregisterCallback(
                                               AppVersionOverrideChanged,
                                               PREF_GENERAL_APPVERSION_OVERRIDE,
                                               nullptr)) ||
        NS_FAILED(Preferences::UnregisterCallback(
                                                 PlatformOverrideChanged,
                                                 PREF_GENERAL_PLATFORM_OVERRIDE,
                                                 nullptr)) ||
        NS_FAILED(Preferences::UnregisterCallback(LoadRuntimeOptions,
                                                  PREF_JS_OPTIONS_PREFIX,
                                                  nullptr)) ||
        NS_FAILED(Preferences::UnregisterCallback(LoadRuntimeOptions,
                                                  PREF_WORKERS_OPTIONS_PREFIX,
                                                  nullptr)) ||
        NS_FAILED(Preferences::UnregisterCallback(
                                  WorkerPrefChanged,
                                  PREF_DOM_CACHES_ENABLED,
                                  reinterpret_cast<void *>(WORKERPREF_DOM_CACHES))) ||
#if DUMP_CONTROLLED_BY_PREF
        NS_FAILED(Preferences::UnregisterCallback(
                                  WorkerPrefChanged,
                                  PREF_DOM_WINDOW_DUMP_ENABLED,
                                  reinterpret_cast<void *>(WORKERPREF_DUMP))) ||
#endif
#ifdef JS_GC_ZEAL
        NS_FAILED(Preferences::UnregisterCallback(
                                             LoadGCZealOptions,
                                             PREF_JS_OPTIONS_PREFIX PREF_GCZEAL,
                                             nullptr)) ||
        NS_FAILED(Preferences::UnregisterCallback(
                                        LoadGCZealOptions,
                                        PREF_WORKERS_OPTIONS_PREFIX PREF_GCZEAL,
                                        nullptr)) ||
#endif
        NS_FAILED(Preferences::UnregisterCallback(
                                 LoadJSGCMemoryOptions,
                                 PREF_JS_OPTIONS_PREFIX PREF_MEM_OPTIONS_PREFIX,
                                 nullptr)) ||
        NS_FAILED(Preferences::UnregisterCallback(
                            LoadJSGCMemoryOptions,
                            PREF_WORKERS_OPTIONS_PREFIX PREF_MEM_OPTIONS_PREFIX,
                            nullptr))) {
      NS_WARNING("Failed to unregister pref callbacks!");
    }

    if (obs) {
      if (NS_FAILED(obs->RemoveObserver(this, GC_REQUEST_OBSERVER_TOPIC))) {
        NS_WARNING("Failed to unregister for GC request notifications!");
      }

      if (NS_FAILED(obs->RemoveObserver(this, CC_REQUEST_OBSERVER_TOPIC))) {
        NS_WARNING("Failed to unregister for CC request notifications!");
      }

      if (NS_FAILED(obs->RemoveObserver(this,
                                        MEMORY_PRESSURE_OBSERVER_TOPIC))) {
        NS_WARNING("Failed to unregister for memory pressure notifications!");
      }

      if (NS_FAILED(obs->RemoveObserver(this,
                                        NS_IOSERVICE_OFFLINE_STATUS_TOPIC))) {
        NS_WARNING("Failed to unregister for offline notification event!");
      }
      obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID);
      obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
      mObserved = false;
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


PLDHashOperator
RuntimeService::RemoveSharedWorkerFromWindowMap(
                                  nsPIDOMWindow* aKey,
                                  nsAutoPtr<nsTArray<WorkerPrivate*> >& aData,
                                  void* aUserArg)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aData.get());
  MOZ_ASSERT(aUserArg);

  auto workerPrivate = static_cast<WorkerPrivate*>(aUserArg);

  MOZ_ASSERT(workerPrivate->IsSharedWorker() || workerPrivate->IsServiceWorker());

  if (aData->RemoveElement(workerPrivate)) {
    MOZ_ASSERT(!aData->Contains(workerPrivate), "Added worker more than once!");

    if (aData->IsEmpty()) {
      return PL_DHASH_REMOVE;
    }
  }

  return PL_DHASH_NEXT;
}


PLDHashOperator
RuntimeService::FindSharedWorkerInfo(const nsACString& aKey,
                                     SharedWorkerInfo* aData,
                                     void* aUserArg)
{
  auto match = static_cast<MatchSharedWorkerInfo*>(aUserArg);

  if (aData->mWorkerPrivate == match->mWorkerPrivate) {
    match->mSharedWorkerInfo = aData;
    return PL_DHASH_STOP;
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
RuntimeService::CancelWorkersForWindow(nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();

  nsAutoTArray<WorkerPrivate*, MAX_WORKERS_PER_DOMAIN> workers;
  GetWorkersForWindow(aWindow, workers);

  if (!workers.IsEmpty()) {
    AutoJSAPI jsapi;
    if (NS_WARN_IF(!jsapi.InitWithLegacyErrorReporting(aWindow))) {
      return;
    }
    JSContext* cx = jsapi.cx();

    for (uint32_t index = 0; index < workers.Length(); index++) {
      WorkerPrivate*& worker = workers[index];

      if (worker->IsSharedWorker() || worker->IsServiceWorker()) {
        worker->CloseSharedWorkersForWindow(aWindow);
      } else if (!worker->Cancel(cx)) {
        JS_ReportPendingException(cx);
      }
    }
  }
}

void
RuntimeService::SuspendWorkersForWindow(nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aWindow);

  nsAutoTArray<WorkerPrivate*, MAX_WORKERS_PER_DOMAIN> workers;
  GetWorkersForWindow(aWindow, workers);

  if (!workers.IsEmpty()) {
    AutoJSAPI jsapi;
    if (NS_WARN_IF(!jsapi.InitWithLegacyErrorReporting(aWindow))) {
      return;
    }
    JSContext* cx = jsapi.cx();

    for (uint32_t index = 0; index < workers.Length(); index++) {
      if (!workers[index]->Suspend(cx, aWindow)) {
        JS_ReportPendingException(cx);
      }
    }
  }
}

void
RuntimeService::ResumeWorkersForWindow(nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aWindow);

  nsAutoTArray<WorkerPrivate*, MAX_WORKERS_PER_DOMAIN> workers;
  GetWorkersForWindow(aWindow, workers);

  if (!workers.IsEmpty()) {
    AutoJSAPI jsapi;
    if (NS_WARN_IF(!jsapi.InitWithLegacyErrorReporting(aWindow))) {
      return;
    }
    JSContext* cx = jsapi.cx();

    for (uint32_t index = 0; index < workers.Length(); index++) {
      if (!workers[index]->SynchronizeAndResume(cx, aWindow)) {
        JS_ReportPendingException(cx);
      }
    }
  }
}

nsresult
RuntimeService::CreateSharedWorkerInternal(const GlobalObject& aGlobal,
                                           const nsAString& aScriptURL,
                                           const nsACString& aName,
                                           WorkerType aType,
                                           SharedWorker** aSharedWorker)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aType == WorkerTypeShared || aType == WorkerTypeService);

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  MOZ_ASSERT(window);

  JSContext* cx = aGlobal.Context();

  WorkerLoadInfo loadInfo;
  nsresult rv = WorkerPrivate::GetLoadInfo(cx, window, nullptr, aScriptURL,
                                           false,
                                           WorkerPrivate::OverrideLoadGroup,
                                           &loadInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  return CreateSharedWorkerFromLoadInfo(cx, &loadInfo, aScriptURL, aName, aType,
                                        aSharedWorker);
}

nsresult
RuntimeService::CreateSharedWorkerFromLoadInfo(JSContext* aCx,
                                               WorkerLoadInfo* aLoadInfo,
                                               const nsAString& aScriptURL,
                                               const nsACString& aName,
                                               WorkerType aType,
                                               SharedWorker** aSharedWorker)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aLoadInfo);
  MOZ_ASSERT(aLoadInfo->mResolvedScriptURI);

  nsRefPtr<WorkerPrivate> workerPrivate;
  {
    MutexAutoLock lock(mMutex);

    WorkerDomainInfo* domainInfo;
    SharedWorkerInfo* sharedWorkerInfo;

    nsCString scriptSpec;
    nsresult rv = aLoadInfo->mResolvedScriptURI->GetSpec(scriptSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString key;
    GenerateSharedWorkerKey(scriptSpec, aName, aType, key);

    if (mDomainMap.Get(aLoadInfo->mDomain, &domainInfo) &&
        domainInfo->mSharedWorkerInfos.Get(key, &sharedWorkerInfo)) {
      workerPrivate = sharedWorkerInfo->mWorkerPrivate;
    }
  }

  
  
  
  
  nsCOMPtr<nsPIDOMWindow> window = aLoadInfo->mWindow;

  bool created = false;
  if (!workerPrivate) {
    ErrorResult rv;
    workerPrivate =
      WorkerPrivate::Constructor(aCx, aScriptURL, false,
                                 aType, aName, aLoadInfo, rv);
    NS_ENSURE_TRUE(workerPrivate, rv.ErrorCode());

    created = true;
  } else {
    
    
    
    workerPrivate->UpdateOverridenLoadGroup(aLoadInfo->mLoadGroup);
  }

  nsRefPtr<SharedWorker> sharedWorker = new SharedWorker(window, workerPrivate);

  if (!workerPrivate->RegisterSharedWorker(aCx, sharedWorker)) {
    NS_WARNING("Worker is unreachable, this shouldn't happen!");
    sharedWorker->Close();
    return NS_ERROR_FAILURE;
  }

  
  
  if (!created) {
    nsTArray<WorkerPrivate*>* windowArray;
    if (!mWindowMap.Get(window, &windowArray)) {
      windowArray = new nsTArray<WorkerPrivate*>(1);
      mWindowMap.Put(window, windowArray);
    }

    if (!windowArray->Contains(workerPrivate)) {
      windowArray->AppendElement(workerPrivate);
    }
  }

  sharedWorker.forget(aSharedWorker);
  return NS_OK;
}

void
RuntimeService::ForgetSharedWorker(WorkerPrivate* aWorkerPrivate)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aWorkerPrivate);
  MOZ_ASSERT(aWorkerPrivate->IsSharedWorker() ||
             aWorkerPrivate->IsServiceWorker());

  MutexAutoLock lock(mMutex);

  WorkerDomainInfo* domainInfo;
  if (mDomainMap.Get(aWorkerPrivate->Domain(), &domainInfo)) {
    MatchSharedWorkerInfo match(aWorkerPrivate);
    domainInfo->mSharedWorkerInfos.EnumerateRead(FindSharedWorkerInfo,
                                                 &match);

    if (match.mSharedWorkerInfo) {
      nsAutoCString key;
      GenerateSharedWorkerKey(match.mSharedWorkerInfo->mScriptSpec,
                              match.mSharedWorkerInfo->mName,
                              aWorkerPrivate->Type(), key);
      domainInfo->mSharedWorkerInfos.Remove(key);
    }
  }
}

void
RuntimeService::NoteIdleThread(WorkerThread* aThread)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aThread);

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
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aThread->Shutdown()));
    return;
  }

  
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mIdleThreadTimer->InitWithFuncCallback(
                                                 ShutdownIdleThreads, nullptr,
                                                 IDLE_THREAD_TIMEOUT_SEC * 1000,
                                                 nsITimer::TYPE_ONE_SHOT)));
}

void
RuntimeService::UpdateAllWorkerRuntimeOptions()
{
  BROADCAST_ALL_WORKERS(UpdateRuntimeOptions, sDefaultJSSettings.runtimeOptions);
}

void
RuntimeService::UpdateAppNameOverridePreference(const nsAString& aValue)
{
  AssertIsOnMainThread();
  mNavigatorProperties.mAppNameOverridden = aValue;
}

void
RuntimeService::UpdateAppVersionOverridePreference(const nsAString& aValue)
{
  AssertIsOnMainThread();
  mNavigatorProperties.mAppVersionOverridden = aValue;
}

void
RuntimeService::UpdatePlatformOverridePreference(const nsAString& aValue)
{
  AssertIsOnMainThread();
  mNavigatorProperties.mPlatformOverridden = aValue;
}

void
RuntimeService::UpdateAllWorkerPreference(WorkerPreference aPref, bool aValue)
{
  BROADCAST_ALL_WORKERS(UpdatePreference, aPref, aValue);
}

void
RuntimeService::UpdateAllWorkerLanguages(const nsTArray<nsString>& aLanguages)
{
  MOZ_ASSERT(NS_IsMainThread());

  mNavigatorProperties.mLanguages = aLanguages;
  BROADCAST_ALL_WORKERS(UpdateLanguages, aLanguages);
}

void
RuntimeService::UpdateAllWorkerMemoryParameter(JSGCParamKey aKey,
                                               uint32_t aValue)
{
  BROADCAST_ALL_WORKERS(UpdateJSWorkerMemoryParameter, aKey, aValue);
}

#ifdef JS_GC_ZEAL
void
RuntimeService::UpdateAllWorkerGCZeal()
{
  BROADCAST_ALL_WORKERS(UpdateGCZeal, sDefaultJSSettings.gcZeal,
                        sDefaultJSSettings.gcZealFrequency);
}
#endif

void
RuntimeService::GarbageCollectAllWorkers(bool aShrinking)
{
  BROADCAST_ALL_WORKERS(GarbageCollect, aShrinking);
}

void
RuntimeService::CycleCollectAllWorkers()
{
  BROADCAST_ALL_WORKERS(CycleCollect,  false);
}

void
RuntimeService::SendOfflineStatusChangeEventToAllWorkers(bool aIsOffline)
{
  BROADCAST_ALL_WORKERS(OfflineStatusChangeEvent, aIsOffline);
}


NS_IMPL_ISUPPORTS(RuntimeService, nsIObserver)


NS_IMETHODIMP
RuntimeService::Observe(nsISupports* aSubject, const char* aTopic,
                        const char16_t* aData)
{
  AssertIsOnMainThread();

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    Shutdown();
    return NS_OK;
  }
  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID)) {
    Cleanup();
    return NS_OK;
  }
  if (!strcmp(aTopic, GC_REQUEST_OBSERVER_TOPIC)) {
    GarbageCollectAllWorkers( false);
    return NS_OK;
  }
  if (!strcmp(aTopic, CC_REQUEST_OBSERVER_TOPIC)) {
    CycleCollectAllWorkers();
    return NS_OK;
  }
  if (!strcmp(aTopic, MEMORY_PRESSURE_OBSERVER_TOPIC)) {
    GarbageCollectAllWorkers( true);
    CycleCollectAllWorkers();
    return NS_OK;
  }
  if (!strcmp(aTopic, NS_IOSERVICE_OFFLINE_STATUS_TOPIC)) {
    SendOfflineStatusChangeEventToAllWorkers(NS_IsOffline());
    return NS_OK;
  }
  if (!strcmp(aTopic, NS_IOSERVICE_APP_OFFLINE_STATUS_TOPIC)) {
    BROADCAST_ALL_WORKERS(OfflineStatusChangeEvent,
                          NS_IsOffline() ||
                          NS_IsAppOffline(workers[index]->GetPrincipal()));
    return NS_OK;
  }

  NS_NOTREACHED("Unknown observer topic!");
  return NS_OK;
}

 void
RuntimeService::WorkerPrefChanged(const char* aPrefName, void* aClosure)
{
  AssertIsOnMainThread();

  uintptr_t tmp = reinterpret_cast<uintptr_t>(aClosure);
  MOZ_ASSERT(tmp < WORKERPREF_COUNT);
  WorkerPreference key = static_cast<WorkerPreference>(tmp);

#ifdef DUMP_CONTROLLED_BY_PREF
  if (key == WORKERPREF_DUMP) {
    key = WORKERPREF_DUMP;
    sDefaultPreferences[WORKERPREF_DUMP] =
      Preferences::GetBool(PREF_DOM_WINDOW_DUMP_ENABLED, false);
  }
#endif

  if (key == WORKERPREF_DOM_CACHES) {
    key = WORKERPREF_DOM_CACHES;
    sDefaultPreferences[WORKERPREF_DOM_CACHES] =
      Preferences::GetBool(PREF_DOM_CACHES_ENABLED, false);
  }
  
  
  MOZ_ASSERT(key != WORKERPREF_COUNT);

  RuntimeService* rts = RuntimeService::GetService();
  if (rts) {
    rts->UpdateAllWorkerPreference(key, sDefaultPreferences[key]);
  }
}

void
RuntimeService::JSVersionChanged(const char* , void* )
{
  AssertIsOnMainThread();

  bool useLatest = Preferences::GetBool(PREF_WORKERS_LATEST_JS_VERSION, false);
  JS::CompartmentOptions& options = sDefaultJSSettings.content.compartmentOptions;
  options.setVersion(useLatest ? JSVERSION_LATEST : JSVERSION_DEFAULT);
}

NS_IMPL_ISUPPORTS_INHERITED0(LogViolationDetailsRunnable, nsRunnable)

NS_IMETHODIMP
LogViolationDetailsRunnable::Run()
{
  AssertIsOnMainThread();

  nsIContentSecurityPolicy* csp = mWorkerPrivate->GetCSP();
  if (csp) {
    NS_NAMED_LITERAL_STRING(scriptSample,
        "Call to eval() or related function blocked by CSP.");
    if (mWorkerPrivate->GetReportCSPViolations()) {
      csp->LogViolationDetails(nsIContentSecurityPolicy::VIOLATION_TYPE_EVAL,
                               mFileName, scriptSample, mLineNum,
                               EmptyString(), EmptyString());
    }
  }

  nsRefPtr<MainThreadStopSyncLoopRunnable> response =
    new MainThreadStopSyncLoopRunnable(mWorkerPrivate, mSyncLoopTarget.forget(),
                                       true);
  MOZ_ALWAYS_TRUE(response->Dispatch(nullptr));

  return NS_OK;
}

NS_IMPL_ISUPPORTS(WorkerBackgroundChildCallback, nsIIPCBackgroundChildCreateCallback)

NS_IMPL_ISUPPORTS_INHERITED0(WorkerThreadPrimaryRunnable, nsRunnable)

NS_IMETHODIMP
WorkerThreadPrimaryRunnable::Run()
{
  using mozilla::ipc::BackgroundChild;

#ifdef MOZ_NUWA_PROCESS
  if (IsNuwaProcess()) {
    NuwaMarkCurrentThread(nullptr, nullptr);
    NuwaFreezeCurrentThread();
  }
#endif

  char stackBaseGuess;

  PR_SetCurrentThreadName("DOM Worker");

  nsAutoCString threadName;
  threadName.AssignLiteral("DOM Worker '");
  threadName.Append(NS_LossyConvertUTF16toASCII(mWorkerPrivate->ScriptURL()));
  threadName.Append('\'');

  profiler_register_thread(threadName.get(), &stackBaseGuess);

  
  
  
  nsresult rv = SynchronouslyCreatePBackground();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    
    return rv;
  }

  mWorkerPrivate->SetThread(mThread);

#ifdef ENABLE_TESTS
  TestPBackground();
#endif

  mWorkerPrivate->AssertIsOnWorkerThread();

  {
    nsCycleCollector_startup();

    WorkerJSRuntime runtime(mParentRuntime, mWorkerPrivate);
    JSRuntime* rt = runtime.Runtime();

    JSContext* cx = CreateJSContextForWorker(mWorkerPrivate, rt);
    if (!cx) {
      
      NS_ERROR("Failed to create runtime and context!");
      return NS_ERROR_FAILURE;
    }

    {
#ifdef MOZ_ENABLE_PROFILER_SPS
      PseudoStack* stack = mozilla_get_pseudo_stack();
      if (stack) {
        stack->sampleRuntime(rt);
      }
#endif

      {
        JSAutoRequest ar(cx);

        mWorkerPrivate->DoRunLoop(cx);

        JS_ReportPendingException(cx);
      }

#ifdef ENABLE_TESTS
      TestPBackground();
#endif

      BackgroundChild::CloseForCurrentThread();

#ifdef MOZ_ENABLE_PROFILER_SPS
      if (stack) {
        stack->sampleRuntime(nullptr);
      }
#endif
    }

    
    
    
    JS_DestroyContext(cx);

    
    
    
    
  }

  mWorkerPrivate->SetThread(nullptr);

  mWorkerPrivate->ScheduleDeletion(WorkerPrivate::WorkerRan);

  
  mWorkerPrivate = nullptr;

  
  nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
  MOZ_ASSERT(mainThread);

  nsRefPtr<FinishedRunnable> finishedRunnable =
    new FinishedRunnable(mThread.forget());
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mainThread->Dispatch(finishedRunnable,
                                                    NS_DISPATCH_NORMAL)));

  profiler_unregister_thread();
  return NS_OK;
}

nsresult
WorkerThreadPrimaryRunnable::SynchronouslyCreatePBackground()
{
  using mozilla::ipc::BackgroundChild;

  MOZ_ASSERT(!BackgroundChild::GetForCurrentThread());

  bool done = false;
  nsCOMPtr<nsIIPCBackgroundChildCreateCallback> callback =
    new WorkerBackgroundChildCallback(&done);

  if (NS_WARN_IF(!BackgroundChild::GetOrCreateForCurrentThread(callback))) {
    return NS_ERROR_FAILURE;
  }

  while (!done) {
    if (NS_WARN_IF(!NS_ProcessNextEvent(mThread, true ))) {
      return NS_ERROR_FAILURE;
    }
  }

  if (NS_WARN_IF(!BackgroundChild::GetForCurrentThread())) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(WorkerThreadPrimaryRunnable::FinishedRunnable,
                             nsRunnable)

NS_IMETHODIMP
WorkerThreadPrimaryRunnable::FinishedRunnable::Run()
{
  AssertIsOnMainThread();

  nsRefPtr<WorkerThread> thread;
  mThread.swap(thread);

  RuntimeService* rts = RuntimeService::GetService();
  if (rts) {
    rts->NoteIdleThread(thread);
  }
  else if (thread->ShutdownRequired()) {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(thread->Shutdown()));
  }

  return NS_OK;
}
