





#ifdef MOZ_WIDGET_QT
#include "PluginHelperQt.h"
#endif

#include "mozilla/plugins/PluginModuleParent.h"

#include "base/process_util.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/PCrashReporterParent.h"
#include "mozilla/ipc/MessageChannel.h"
#include "mozilla/plugins/BrowserStreamParent.h"
#include "mozilla/plugins/PluginAsyncSurrogate.h"
#include "mozilla/plugins/PluginBridge.h"
#include "mozilla/plugins/PluginInstanceParent.h"
#include "mozilla/Preferences.h"
#include "mozilla/ProcessHangMonitor.h"
#include "mozilla/Services.h"
#include "mozilla/Telemetry.h"
#include "mozilla/unused.h"
#include "nsAutoPtr.h"
#include "nsCRT.h"
#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsNPAPIPlugin.h"
#include "nsPrintfCString.h"
#include "prsystem.h"
#include "GeckoProfiler.h"
#include "nsPluginTags.h"

#ifdef XP_WIN
#include "mozilla/widget/AudioSession.h"
#include "nsWindowsHelpers.h"
#include "PluginHangUIParent.h"
#endif

#ifdef MOZ_ENABLE_PROFILER_SPS
#include "nsIProfiler.h"
#include "nsIProfileSaveEvent.h"
#endif

#ifdef MOZ_WIDGET_GTK
#include <glib.h>
#elif XP_MACOSX
#include "PluginInterposeOSX.h"
#include "PluginUtilsOSX.h"
#endif

using base::KillProcess;

using mozilla::PluginLibrary;
using mozilla::ipc::MessageChannel;
using mozilla::dom::PCrashReporterParent;
using mozilla::dom::CrashReporterParent;

using namespace mozilla;
using namespace mozilla::plugins;
using namespace mozilla::plugins::parent;

#ifdef MOZ_CRASHREPORTER
#include "mozilla/dom/CrashReporterParent.h"

using namespace CrashReporter;
#endif

static const char kContentTimeoutPref[] = "dom.ipc.plugins.contentTimeoutSecs";
static const char kChildTimeoutPref[] = "dom.ipc.plugins.timeoutSecs";
static const char kParentTimeoutPref[] = "dom.ipc.plugins.parentTimeoutSecs";
static const char kLaunchTimeoutPref[] = "dom.ipc.plugins.processLaunchTimeoutSecs";
static const char kAsyncInitPref[] = "dom.ipc.plugins.asyncInit";
#ifdef XP_WIN
static const char kHangUITimeoutPref[] = "dom.ipc.plugins.hangUITimeoutSecs";
static const char kHangUIMinDisplayPref[] = "dom.ipc.plugins.hangUIMinDisplaySecs";
#define CHILD_TIMEOUT_PREF kHangUITimeoutPref
#else
#define CHILD_TIMEOUT_PREF kChildTimeoutPref
#endif

template<>
struct RunnableMethodTraits<mozilla::plugins::PluginModuleParent>
{
    typedef mozilla::plugins::PluginModuleParent Class;
    static void RetainCallee(Class* obj) { }
    static void ReleaseCallee(Class* obj) { }
};

bool
mozilla::plugins::SetupBridge(uint32_t aPluginId,
                              dom::ContentParent* aContentParent,
                              bool aForceBridgeNow)
{
    nsRefPtr<nsPluginHost> host = nsPluginHost::GetInst();
    nsRefPtr<nsNPAPIPlugin> plugin;
    nsresult rv = host->GetPluginForContentProcess(aPluginId, getter_AddRefs(plugin));
    if (NS_FAILED(rv)) {
        return false;
    }
    PluginModuleChromeParent* chromeParent = static_cast<PluginModuleChromeParent*>(plugin->GetLibrary());
    chromeParent->SetContentParent(aContentParent);
    if (!aForceBridgeNow && chromeParent->IsStartingAsync()) {
        
        return true;
    }
    return PPluginModule::Bridge(aContentParent, chromeParent);
}






class PluginModuleMapping : public PRCList
{
public:
    explicit PluginModuleMapping(uint32_t aPluginId)
        : mPluginId(aPluginId)
        , mProcessIdValid(false)
        , mModule(nullptr)
        , mChannelOpened(false)
    {
        MOZ_COUNT_CTOR(PluginModuleMapping);
        PR_INIT_CLIST(this);
        PR_APPEND_LINK(this, &sModuleListHead);
    }

    ~PluginModuleMapping()
    {
        PR_REMOVE_LINK(this);
        MOZ_COUNT_DTOR(PluginModuleMapping);
    }

    bool
    IsChannelOpened() const
    {
        return mChannelOpened;
    }

    void
    SetChannelOpened()
    {
        mChannelOpened = true;
    }

    PluginModuleContentParent*
    GetModule()
    {
        if (!mModule) {
            mModule = new PluginModuleContentParent();
        }
        return mModule;
    }

    static PluginModuleMapping*
    AssociateWithProcessId(uint32_t aPluginId, base::ProcessId aProcessId)
    {
        PluginModuleMapping* mapping =
            static_cast<PluginModuleMapping*>(PR_NEXT_LINK(&sModuleListHead));
        while (mapping != &sModuleListHead) {
            if (mapping->mPluginId == aPluginId) {
                mapping->AssociateWithProcessId(aProcessId);
                return mapping;
            }
            mapping = static_cast<PluginModuleMapping*>(PR_NEXT_LINK(mapping));
        }
        return nullptr;
    }

    static PluginModuleMapping*
    Resolve(base::ProcessId aProcessId)
    {
        PluginModuleMapping* mapping = nullptr;

        if (sIsLoadModuleOnStack) {
            
            
            mapping =
                static_cast<PluginModuleMapping*>(PR_LIST_TAIL(&sModuleListHead));
            MOZ_ASSERT(mapping);
            return mapping;
        }

        mapping =
            static_cast<PluginModuleMapping*>(PR_NEXT_LINK(&sModuleListHead));
        while (mapping != &sModuleListHead) {
            if (mapping->mProcessIdValid && mapping->mProcessId == aProcessId) {
                return mapping;
            }
            mapping = static_cast<PluginModuleMapping*>(PR_NEXT_LINK(mapping));
        }
        return nullptr;
    }

    static PluginModuleMapping*
    FindModuleByPluginId(uint32_t aPluginId)
    {
        PluginModuleMapping* mapping =
            static_cast<PluginModuleMapping*>(PR_NEXT_LINK(&sModuleListHead));
        while (mapping != &sModuleListHead) {
            if (mapping->mPluginId == aPluginId) {
                return mapping;
            }
            mapping = static_cast<PluginModuleMapping*>(PR_NEXT_LINK(mapping));
        }
        return nullptr;
    }

    static bool
    IsLoadModuleOnStack()
    {
        return sIsLoadModuleOnStack;
    }

    class MOZ_STACK_CLASS NotifyLoadingModule
    {
    public:
        explicit NotifyLoadingModule(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
        {
            MOZ_GUARD_OBJECT_NOTIFIER_INIT;
            PluginModuleMapping::sIsLoadModuleOnStack = true;
        }

        ~NotifyLoadingModule()
        {
            PluginModuleMapping::sIsLoadModuleOnStack = false;
        }

    private:
        MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
    };

private:
    void
    AssociateWithProcessId(base::ProcessId aProcessId)
    {
        MOZ_ASSERT(!mProcessIdValid);
        mProcessId = aProcessId;
        mProcessIdValid = true;
    }

    uint32_t mPluginId;
    bool mProcessIdValid;
    base::ProcessId mProcessId;
    PluginModuleContentParent* mModule;
    bool mChannelOpened;

    friend class NotifyLoadingModule;

    static PRCList sModuleListHead;
    static bool sIsLoadModuleOnStack;
};

PRCList PluginModuleMapping::sModuleListHead =
    PR_INIT_STATIC_CLIST(&PluginModuleMapping::sModuleListHead);

bool PluginModuleMapping::sIsLoadModuleOnStack = false;

void
mozilla::plugins::TerminatePlugin(uint32_t aPluginId)
{
    MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

    nsRefPtr<nsPluginHost> host = nsPluginHost::GetInst();
    nsPluginTag* pluginTag = host->PluginWithId(aPluginId);
    if (!pluginTag || !pluginTag->mPlugin) {
        return;
    }

    nsRefPtr<nsNPAPIPlugin> plugin = pluginTag->mPlugin;
    PluginModuleChromeParent* chromeParent = static_cast<PluginModuleChromeParent*>(plugin->GetLibrary());
    chromeParent->TerminateChildProcess(MessageLoop::current());
}

 PluginLibrary*
PluginModuleContentParent::LoadModule(uint32_t aPluginId)
{
    PluginModuleMapping::NotifyLoadingModule loadingModule;
    nsAutoPtr<PluginModuleMapping> mapping(new PluginModuleMapping(aPluginId));

    MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Content);

    






    dom::ContentChild* cp = dom::ContentChild::GetSingleton();
    if (!cp->SendLoadPlugin(aPluginId)) {
        return nullptr;
    }

    PluginModuleContentParent* parent = mapping->GetModule();
    MOZ_ASSERT(parent);

    if (!mapping->IsChannelOpened()) {
        
        
        
        mapping.forget();
    }

    parent->mPluginId = aPluginId;

    return parent;
}

 void
PluginModuleContentParent::AssociatePluginId(uint32_t aPluginId,
                                             base::ProcessId aProcessId)
{
    DebugOnly<PluginModuleMapping*> mapping =
        PluginModuleMapping::AssociateWithProcessId(aPluginId, aProcessId);
    MOZ_ASSERT(mapping);
}

 PluginModuleContentParent*
PluginModuleContentParent::Initialize(mozilla::ipc::Transport* aTransport,
                                      base::ProcessId aOtherProcess)
{
    nsAutoPtr<PluginModuleMapping> moduleMapping(
        PluginModuleMapping::Resolve(aOtherProcess));
    MOZ_ASSERT(moduleMapping);
    PluginModuleContentParent* parent = moduleMapping->GetModule();
    MOZ_ASSERT(parent);

    ProcessHandle handle;
    if (!base::OpenProcessHandle(aOtherProcess, &handle)) {
        
        return nullptr;
    }

    DebugOnly<bool> ok = parent->Open(aTransport, handle, XRE_GetIOMessageLoop(),
                                      mozilla::ipc::ParentSide);
    MOZ_ASSERT(ok);

    moduleMapping->SetChannelOpened();

    
    
    
    parent->GetIPCChannel()->SetChannelFlags(MessageChannel::REQUIRE_DEFERRED_MESSAGE_PROTECTION);

    TimeoutChanged(kContentTimeoutPref, parent);

    
    
    
    moduleMapping.forget();
    return parent;
}

 void
PluginModuleContentParent::OnLoadPluginResult(const uint32_t& aPluginId,
                                              const bool& aResult)
{
    nsAutoPtr<PluginModuleMapping> moduleMapping(
        PluginModuleMapping::FindModuleByPluginId(aPluginId));
    MOZ_ASSERT(moduleMapping);
    PluginModuleContentParent* parent = moduleMapping->GetModule();
    MOZ_ASSERT(parent);
    parent->RecvNP_InitializeResult(aResult ? NPERR_NO_ERROR
                                            : NPERR_GENERIC_ERROR);
}

void
PluginModuleChromeParent::SetContentParent(dom::ContentParent* aContentParent)
{
    MOZ_ASSERT(aContentParent);
    mContentParent = aContentParent;
}

bool
PluginModuleChromeParent::SendAssociatePluginId()
{
    MOZ_ASSERT(mContentParent);
    return mContentParent->SendAssociatePluginId(mPluginId, OtherSidePID());
}


PluginLibrary*
PluginModuleChromeParent::LoadModule(const char* aFilePath, uint32_t aPluginId,
                                     nsPluginTag* aPluginTag)
{
    PLUGIN_LOG_DEBUG_FUNCTION;

    bool enableSandbox = false;
#if defined(XP_WIN) && defined(MOZ_SANDBOX)
    nsAutoCString sandboxPref("dom.ipc.plugins.sandbox.");
    sandboxPref.Append(aPluginTag->GetNiceFileName());
    if (NS_FAILED(Preferences::GetBool(sandboxPref.get(), &enableSandbox))) {
      enableSandbox = Preferences::GetBool("dom.ipc.plugins.sandbox.default");
    }
#endif

    nsAutoPtr<PluginModuleChromeParent> parent(new PluginModuleChromeParent(aFilePath, aPluginId));
    UniquePtr<LaunchCompleteTask> onLaunchedRunnable(new LaunchedTask(parent));
    parent->mSubprocess->SetCallRunnableImmediately(!parent->mIsStartingAsync);
    TimeStamp launchStart = TimeStamp::Now();
    bool launched = parent->mSubprocess->Launch(Move(onLaunchedRunnable),
                                                enableSandbox);
    if (!launched) {
        
        parent->mShutdown = true;
        return nullptr;
    }
    parent->mIsFlashPlugin = aPluginTag->mIsFlashPlugin;
    if (!parent->mIsStartingAsync) {
        int32_t launchTimeoutSecs = Preferences::GetInt(kLaunchTimeoutPref, 0);
        if (!parent->mSubprocess->WaitUntilConnected(launchTimeoutSecs * 1000)) {
            parent->mShutdown = true;
            return nullptr;
        }
    }
    TimeStamp launchEnd = TimeStamp::Now();
    parent->mTimeBlocked = (launchEnd - launchStart);
    return parent.forget();
}

void
PluginModuleChromeParent::OnProcessLaunched(const bool aSucceeded)
{
    if (!aSucceeded) {
        mShutdown = true;
        OnInitFailure();
        return;
    }
    
    
    if (mAsyncInitRv != NS_ERROR_NOT_INITIALIZED || mShutdown) {
        return;
    }
    Open(mSubprocess->GetChannel(), mSubprocess->GetChildProcessHandle());

    
    
    
    GetIPCChannel()->SetChannelFlags(MessageChannel::REQUIRE_DEFERRED_MESSAGE_PROTECTION);

    TimeoutChanged(CHILD_TIMEOUT_PREF, this);

    Preferences::RegisterCallback(TimeoutChanged, kChildTimeoutPref, this);
    Preferences::RegisterCallback(TimeoutChanged, kParentTimeoutPref, this);
#ifdef XP_WIN
    Preferences::RegisterCallback(TimeoutChanged, kHangUITimeoutPref, this);
    Preferences::RegisterCallback(TimeoutChanged, kHangUIMinDisplayPref, this);
#endif

#ifdef MOZ_CRASHREPORTER
    
    if (!CrashReporterParent::CreateCrashReporter(this)) {
        mShutdown = true;
        Close();
        OnInitFailure();
        return;
    }
#ifdef XP_WIN
    { 
        mozilla::MutexAutoLock lock(mCrashReporterMutex);
        mCrashReporter = CrashReporter();
    }
#endif
#endif

#ifdef XP_WIN
    if (mIsFlashPlugin &&
        Preferences::GetBool("dom.ipc.plugins.flash.disable-protected-mode", false)) {
        SendDisableFlashProtectedMode();
    }
#endif

    if (mInitOnAsyncConnect) {
        mInitOnAsyncConnect = false;
#if defined(XP_WIN)
        mAsyncInitRv = NP_GetEntryPoints(mNPPIface,
                                         &mAsyncInitError);
        if (NS_SUCCEEDED(mAsyncInitRv))
#endif
        {
#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(MOZ_WIDGET_GONK)
            mAsyncInitRv = NP_Initialize(mNPNIface,
                                         mNPPIface,
                                         &mAsyncInitError);
#else
            mAsyncInitRv = NP_Initialize(mNPNIface,
                                         &mAsyncInitError);
#endif
        }

#if defined(XP_MACOSX)
        if (NS_SUCCEEDED(mAsyncInitRv)) {
            mAsyncInitRv = NP_GetEntryPoints(mNPPIface,
                                             &mAsyncInitError);
        }
#endif
    }
}

bool
PluginModuleChromeParent::WaitForIPCConnection()
{
    PluginProcessParent* process = Process();
    MOZ_ASSERT(process);
    process->SetCallRunnableImmediately(true);
    if (!process->WaitUntilConnected()) {
        return false;
    }
    return true;
}

PluginModuleParent::PluginModuleParent(bool aIsChrome)
    : mIsChrome(aIsChrome)
    , mShutdown(false)
    , mClearSiteDataSupported(false)
    , mGetSitesWithDataSupported(false)
    , mNPNIface(nullptr)
    , mNPPIface(nullptr)
    , mPlugin(nullptr)
    , mTaskFactory(this)
    , mIsStartingAsync(false)
    , mNPInitialized(false)
    , mAsyncNewRv(NS_ERROR_NOT_INITIALIZED)
{
#if defined(XP_WIN) || defined(XP_MACOSX) || defined(MOZ_WIDGET_GTK)
    mIsStartingAsync = Preferences::GetBool(kAsyncInitPref, false);
#endif
}

PluginModuleParent::~PluginModuleParent()
{
    if (!OkToCleanup()) {
        NS_RUNTIMEABORT("unsafe destruction");
    }

    if (!mShutdown) {
        NS_WARNING("Plugin host deleted the module without shutting down.");
        NPError err;
        NP_Shutdown(&err);
    }
}

PluginModuleContentParent::PluginModuleContentParent()
    : PluginModuleParent(false)
{
    Preferences::RegisterCallback(TimeoutChanged, kContentTimeoutPref, this);
}

PluginModuleContentParent::~PluginModuleContentParent()
{
    Preferences::UnregisterCallback(TimeoutChanged, kContentTimeoutPref, this);
}

PluginModuleChromeParent::PluginModuleChromeParent(const char* aFilePath, uint32_t aPluginId)
    : PluginModuleParent(true)
    , mSubprocess(new PluginProcessParent(aFilePath))
    , mPluginId(aPluginId)
    , mChromeTaskFactory(this)
    , mHangAnnotationFlags(0)
#ifdef XP_WIN
    , mPluginCpuUsageOnHang()
    , mHangUIParent(nullptr)
    , mHangUIEnabled(true)
    , mIsTimerReset(true)
#ifdef MOZ_CRASHREPORTER
    , mCrashReporterMutex("PluginModuleParent::mCrashReporterMutex")
    , mCrashReporter(nullptr)
#endif
#endif
#ifdef MOZ_CRASHREPORTER_INJECTOR
    , mFlashProcess1(0)
    , mFlashProcess2(0)
#endif
    , mInitOnAsyncConnect(false)
    , mAsyncInitRv(NS_ERROR_NOT_INITIALIZED)
    , mAsyncInitError(NPERR_NO_ERROR)
    , mContentParent(nullptr)
    , mIsFlashPlugin(false)
{
    NS_ASSERTION(mSubprocess, "Out of memory!");

    RegisterSettingsCallbacks();

#ifdef MOZ_ENABLE_PROFILER_SPS
    InitPluginProfiling();
#endif

    mozilla::HangMonitor::RegisterAnnotator(*this);
}

PluginModuleChromeParent::~PluginModuleChromeParent()
{
    if (!OkToCleanup()) {
        NS_RUNTIMEABORT("unsafe destruction");
    }

#ifdef MOZ_ENABLE_PROFILER_SPS
    ShutdownPluginProfiling();
#endif

    if (!mShutdown) {
        NS_WARNING("Plugin host deleted the module without shutting down.");
        NPError err;
        NP_Shutdown(&err);
    }

    NS_ASSERTION(mShutdown, "NP_Shutdown didn't");

    if (mSubprocess) {
        mSubprocess->Delete();
        mSubprocess = nullptr;
    }

#ifdef MOZ_CRASHREPORTER_INJECTOR
    if (mFlashProcess1)
        UnregisterInjectorCallback(mFlashProcess1);
    if (mFlashProcess2)
        UnregisterInjectorCallback(mFlashProcess2);
#endif

    UnregisterSettingsCallbacks();

    Preferences::UnregisterCallback(TimeoutChanged, kChildTimeoutPref, this);
    Preferences::UnregisterCallback(TimeoutChanged, kParentTimeoutPref, this);
#ifdef XP_WIN
    Preferences::UnregisterCallback(TimeoutChanged, kHangUITimeoutPref, this);
    Preferences::UnregisterCallback(TimeoutChanged, kHangUIMinDisplayPref, this);

    if (mHangUIParent) {
        delete mHangUIParent;
        mHangUIParent = nullptr;
    }
#endif

    mozilla::HangMonitor::UnregisterAnnotator(*this);
}

#ifdef MOZ_CRASHREPORTER
void
PluginModuleChromeParent::WriteExtraDataForMinidump(AnnotationTable& notes)
{
#ifdef XP_WIN
    
    mCrashReporterMutex.AssertCurrentThreadOwns();
#endif
    typedef nsDependentCString CS;

    
    const std::string& pluginFile = mSubprocess->GetPluginFilePath();
    size_t filePos = pluginFile.rfind(FILE_PATH_SEPARATOR);
    if (filePos == std::string::npos)
        filePos = 0;
    else
        filePos++;
    notes.Put(NS_LITERAL_CSTRING("PluginFilename"), CS(pluginFile.substr(filePos).c_str()));

    notes.Put(NS_LITERAL_CSTRING("PluginName"), mPluginName);
    notes.Put(NS_LITERAL_CSTRING("PluginVersion"), mPluginVersion);

    CrashReporterParent* crashReporter = CrashReporter();
    if (crashReporter) {
#ifdef XP_WIN
        if (mPluginCpuUsageOnHang.Length() > 0) {
            notes.Put(NS_LITERAL_CSTRING("NumberOfProcessors"),
                      nsPrintfCString("%d", PR_GetNumberOfProcessors()));

            nsCString cpuUsageStr;
            cpuUsageStr.AppendFloat(std::ceil(mPluginCpuUsageOnHang[0] * 100) / 100);
            notes.Put(NS_LITERAL_CSTRING("PluginCpuUsage"), cpuUsageStr);

#ifdef MOZ_CRASHREPORTER_INJECTOR
            for (uint32_t i=1; i<mPluginCpuUsageOnHang.Length(); ++i) {
                nsCString tempStr;
                tempStr.AppendFloat(std::ceil(mPluginCpuUsageOnHang[i] * 100) / 100);
                notes.Put(nsPrintfCString("CpuUsageFlashProcess%d", i), tempStr);
            }
#endif
        }
#endif
    }
}
#endif  

void
PluginModuleParent::SetChildTimeout(const int32_t aChildTimeout)
{
    int32_t timeoutMs = (aChildTimeout > 0) ? (1000 * aChildTimeout) :
                      MessageChannel::kNoTimeout;
    SetReplyTimeoutMs(timeoutMs);
}

void
PluginModuleParent::TimeoutChanged(const char* aPref, void* aModule)
{
    PluginModuleParent* module = static_cast<PluginModuleParent*>(aModule);

    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
#ifndef XP_WIN
    if (!strcmp(aPref, kChildTimeoutPref)) {
      MOZ_ASSERT(module->IsChrome());
      
      int32_t timeoutSecs = Preferences::GetInt(kChildTimeoutPref, 0);
      module->SetChildTimeout(timeoutSecs);
#else
    if (!strcmp(aPref, kChildTimeoutPref) ||
        !strcmp(aPref, kHangUIMinDisplayPref) ||
        !strcmp(aPref, kHangUITimeoutPref)) {
      MOZ_ASSERT(module->IsChrome());
      static_cast<PluginModuleChromeParent*>(module)->EvaluateHangUIState(true);
#endif
    } else if (!strcmp(aPref, kParentTimeoutPref)) {
      
      MOZ_ASSERT(module->IsChrome());
      int32_t timeoutSecs = Preferences::GetInt(kParentTimeoutPref, 0);
      unused << static_cast<PluginModuleChromeParent*>(module)->SendSetParentHangTimeout(timeoutSecs);
    } else if (!strcmp(aPref, kContentTimeoutPref)) {
      MOZ_ASSERT(!module->IsChrome());
      int32_t timeoutSecs = Preferences::GetInt(kContentTimeoutPref, 0);
      module->SetChildTimeout(timeoutSecs);
    }
}

void
PluginModuleChromeParent::CleanupFromTimeout(const bool aFromHangUI)
{
    if (mShutdown) {
      return;
    }

    if (!OkToCleanup()) {
        
        MessageLoop::current()->PostDelayedTask(
            FROM_HERE,
            mChromeTaskFactory.NewRunnableMethod(
                &PluginModuleChromeParent::CleanupFromTimeout, aFromHangUI), 10);
        return;
    }

    






    if (aFromHangUI) {
        GetIPCChannel()->CloseWithError();
    } else {
        Close();
    }
}

#ifdef XP_WIN
namespace {

uint64_t
FileTimeToUTC(const FILETIME& ftime) 
{
  ULARGE_INTEGER li;
  li.LowPart = ftime.dwLowDateTime;
  li.HighPart = ftime.dwHighDateTime;
  return li.QuadPart;
}

struct CpuUsageSamples
{
  uint64_t sampleTimes[2];
  uint64_t cpuTimes[2];
};

bool 
GetProcessCpuUsage(const InfallibleTArray<base::ProcessHandle>& processHandles, InfallibleTArray<float>& cpuUsage)
{
  InfallibleTArray<CpuUsageSamples> samples(processHandles.Length());
  FILETIME creationTime, exitTime, kernelTime, userTime, currentTime;
  BOOL res;

  for (uint32_t i = 0; i < processHandles.Length(); ++i) {
    ::GetSystemTimeAsFileTime(&currentTime);
    res = ::GetProcessTimes(processHandles[i], &creationTime, &exitTime, &kernelTime, &userTime);
    if (!res) {
      NS_WARNING("failed to get process times");
      return false;
    }
  
    CpuUsageSamples s;
    s.sampleTimes[0] = FileTimeToUTC(currentTime);
    s.cpuTimes[0]    = FileTimeToUTC(kernelTime) + FileTimeToUTC(userTime);
    samples.AppendElement(s);
  }

  
  ::Sleep(50);

  const int32_t numberOfProcessors = PR_GetNumberOfProcessors();

  for (uint32_t i = 0; i < processHandles.Length(); ++i) {
    ::GetSystemTimeAsFileTime(&currentTime);
    res = ::GetProcessTimes(processHandles[i], &creationTime, &exitTime, &kernelTime, &userTime);
    if (!res) {
      NS_WARNING("failed to get process times");
      return false;
    }

    samples[i].sampleTimes[1] = FileTimeToUTC(currentTime);
    samples[i].cpuTimes[1]    = FileTimeToUTC(kernelTime) + FileTimeToUTC(userTime);    

    const uint64_t deltaSampleTime = samples[i].sampleTimes[1] - samples[i].sampleTimes[0];
    const uint64_t deltaCpuTime    = samples[i].cpuTimes[1]    - samples[i].cpuTimes[0];
    const float usage = 100.f * (float(deltaCpuTime) / deltaSampleTime) / numberOfProcessors;
    cpuUsage.AppendElement(usage);
  }

  return true;
}

} 

#endif 

void
PluginModuleChromeParent::EnteredCxxStack()
{
    mHangAnnotationFlags |= kInPluginCall;
}

void
PluginModuleChromeParent::ExitedCxxStack()
{
    mHangAnnotationFlags = 0;
#ifdef XP_WIN
    FinishHangUI();
#endif
}




void
PluginModuleChromeParent::AnnotateHang(mozilla::HangMonitor::HangAnnotations& aAnnotations)
{
    uint32_t flags = mHangAnnotationFlags;
    if (flags) {
        



        if (flags & kHangUIShown) {
            aAnnotations.AddAnnotation(NS_LITERAL_STRING("HangUIShown"),
                                       true);
        }
        if (flags & kHangUIContinued) {
            aAnnotations.AddAnnotation(NS_LITERAL_STRING("HangUIContinued"),
                                       true);
        }
        if (flags & kHangUIDontShow) {
            aAnnotations.AddAnnotation(NS_LITERAL_STRING("HangUIDontShow"),
                                       true);
        }
        aAnnotations.AddAnnotation(NS_LITERAL_STRING("pluginName"), mPluginName);
        aAnnotations.AddAnnotation(NS_LITERAL_STRING("pluginVersion"),
                                   mPluginVersion);
    }
}

#ifdef MOZ_CRASHREPORTER_INJECTOR
static bool
CreateFlashMinidump(DWORD processId, ThreadId childThread,
                    nsIFile* parentMinidump, const nsACString& name)
{
  if (processId == 0) {
    return false;
  }

  base::ProcessHandle handle;
  if (!base::OpenPrivilegedProcessHandle(processId, &handle)) {
    return false;
  }

  bool res = CreateAdditionalChildMinidump(handle, 0, parentMinidump, name);
  base::CloseProcessHandle(handle);

  return res;
}
#endif

bool
PluginModuleChromeParent::ShouldContinueFromReplyTimeout()
{
#ifdef XP_WIN
    if (LaunchHangUI()) {
        return true;
    }
    
    
    FinishHangUI();
#endif 
    TerminateChildProcess(MessageLoop::current());
    GetIPCChannel()->CloseWithTimeout();
    return false;
}

bool
PluginModuleContentParent::ShouldContinueFromReplyTimeout()
{
    nsRefPtr<ProcessHangMonitor> monitor = ProcessHangMonitor::Get();
    if (!monitor) {
        return true;
    }
    monitor->NotifyPluginHang(mPluginId);
    return true;
}

void
PluginModuleContentParent::OnExitedSyncSend()
{
    ProcessHangMonitor::ClearHang();
}

void
PluginModuleChromeParent::TerminateChildProcess(MessageLoop* aMsgLoop)
{
#ifdef MOZ_CRASHREPORTER
#ifdef XP_WIN
    mozilla::MutexAutoLock lock(mCrashReporterMutex);
    CrashReporterParent* crashReporter = mCrashReporter;
    if (!crashReporter) {
        
        
        return;
    }
#else
    CrashReporterParent* crashReporter = CrashReporter();
#endif
    crashReporter->AnnotateCrashReport(NS_LITERAL_CSTRING("PluginHang"),
                                       NS_LITERAL_CSTRING("1"));
#ifdef XP_WIN
    if (mHangUIParent) {
        unsigned int hangUIDuration = mHangUIParent->LastShowDurationMs();
        if (hangUIDuration) {
            nsPrintfCString strHangUIDuration("%u", hangUIDuration);
            crashReporter->AnnotateCrashReport(
                    NS_LITERAL_CSTRING("PluginHangUIDuration"),
                    strHangUIDuration);
        }
    }
#endif 
    if (crashReporter->GeneratePairedMinidump(this)) {
        mPluginDumpID = crashReporter->ChildDumpID();
        PLUGIN_LOG_DEBUG(
                ("generated paired browser/plugin minidumps: %s)",
                 NS_ConvertUTF16toUTF8(mPluginDumpID).get()));

        nsAutoCString additionalDumps("browser");

#ifdef MOZ_CRASHREPORTER_INJECTOR
        nsCOMPtr<nsIFile> pluginDumpFile;

        if (GetMinidumpForID(mPluginDumpID, getter_AddRefs(pluginDumpFile)) &&
            pluginDumpFile) {
          nsCOMPtr<nsIFile> childDumpFile;

          if (CreateFlashMinidump(mFlashProcess1, 0, pluginDumpFile,
                                  NS_LITERAL_CSTRING("flash1"))) {
            additionalDumps.AppendLiteral(",flash1");
          }
          if (CreateFlashMinidump(mFlashProcess2, 0, pluginDumpFile,
                                  NS_LITERAL_CSTRING("flash2"))) {
            additionalDumps.AppendLiteral(",flash2");
          }
        }
#endif

        crashReporter->AnnotateCrashReport(
            NS_LITERAL_CSTRING("additional_minidumps"),
            additionalDumps);
    } else {
        NS_WARNING("failed to capture paired minidumps from hang");
    }
#endif

#ifdef XP_WIN
    

    InfallibleTArray<base::ProcessHandle> processHandles;

    processHandles.AppendElement(OtherProcess());

#ifdef MOZ_CRASHREPORTER_INJECTOR
    {
      base::ProcessHandle handle;
      if (mFlashProcess1 && base::OpenProcessHandle(mFlashProcess1, &handle)) {
        processHandles.AppendElement(handle);
      }
      if (mFlashProcess2 && base::OpenProcessHandle(mFlashProcess2, &handle)) {
        processHandles.AppendElement(handle);
      }
    }
#endif

    if (!GetProcessCpuUsage(processHandles, mPluginCpuUsageOnHang)) {
      mPluginCpuUsageOnHang.Clear();
    }
#endif

    
    
    bool isFromHangUI = aMsgLoop != MessageLoop::current();
    aMsgLoop->PostTask(
        FROM_HERE,
        mChromeTaskFactory.NewRunnableMethod(
            &PluginModuleChromeParent::CleanupFromTimeout, isFromHangUI));

    if (!KillProcess(OtherProcess(), 1, false))
        NS_WARNING("failed to kill subprocess!");
}

bool
PluginModuleParent::GetPluginDetails(nsACString& aPluginName,
                                     nsACString& aPluginVersion)
{
    nsRefPtr<nsPluginHost> host = nsPluginHost::GetInst();
    if (!host) {
        return false;
    }
    nsPluginTag* pluginTag = host->TagForPlugin(mPlugin);
    if (!pluginTag) {
        return false;
    }
    aPluginName = pluginTag->mName;
    aPluginVersion = pluginTag->mVersion;
    return true;
}

#ifdef XP_WIN
void
PluginModuleChromeParent::EvaluateHangUIState(const bool aReset)
{
    int32_t minDispSecs = Preferences::GetInt(kHangUIMinDisplayPref, 10);
    int32_t autoStopSecs = Preferences::GetInt(kChildTimeoutPref, 0);
    int32_t timeoutSecs = 0;
    if (autoStopSecs > 0 && autoStopSecs < minDispSecs) {
        


        mHangUIEnabled = false;
    } else {
        timeoutSecs = Preferences::GetInt(kHangUITimeoutPref, 0);
        mHangUIEnabled = timeoutSecs > 0;
    }
    if (mHangUIEnabled) {
        if (aReset) {
            mIsTimerReset = true;
            SetChildTimeout(timeoutSecs);
            return;
        } else if (mIsTimerReset) {
            





            autoStopSecs *= 2;
        }
    }
    mIsTimerReset = false;
    SetChildTimeout(autoStopSecs);
}

bool
PluginModuleChromeParent::LaunchHangUI()
{
    if (!mHangUIEnabled) {
        return false;
    }
    if (mHangUIParent) {
        if (mHangUIParent->IsShowing()) {
            
            return false;
        }
        if (mHangUIParent->DontShowAgain()) {
            mHangAnnotationFlags |= kHangUIDontShow;
            bool wasLastHangStopped = mHangUIParent->WasLastHangStopped();
            if (!wasLastHangStopped) {
                mHangAnnotationFlags |= kHangUIContinued;
            }
            return !wasLastHangStopped;
        }
        delete mHangUIParent;
        mHangUIParent = nullptr;
    }
    mHangUIParent = new PluginHangUIParent(this, 
            Preferences::GetInt(kHangUITimeoutPref, 0),
            Preferences::GetInt(kChildTimeoutPref, 0));
    bool retval = mHangUIParent->Init(NS_ConvertUTF8toUTF16(mPluginName));
    if (retval) {
        mHangAnnotationFlags |= kHangUIShown;
        



        EvaluateHangUIState(false);
    }
    return retval;
}

void
PluginModuleChromeParent::FinishHangUI()
{
    if (mHangUIEnabled && mHangUIParent) {
        bool needsCancel = mHangUIParent->IsShowing();
        
        if (needsCancel) {
            mHangUIParent->Cancel();
        }
        

        if (needsCancel ||
            (!mIsTimerReset && mHangUIParent->WasShown())) {
            


            EvaluateHangUIState(true);
        }
    }
}

void
PluginModuleChromeParent::OnHangUIContinue()
{
    mHangAnnotationFlags |= kHangUIContinued;
}
#endif 

#ifdef MOZ_CRASHREPORTER
CrashReporterParent*
PluginModuleChromeParent::CrashReporter()
{
    return static_cast<CrashReporterParent*>(ManagedPCrashReporterParent()[0]);
}

#ifdef MOZ_CRASHREPORTER_INJECTOR
static void
RemoveMinidump(nsIFile* minidump)
{
    if (!minidump)
        return;

    minidump->Remove(false);
    nsCOMPtr<nsIFile> extraFile;
    if (GetExtraFileForMinidump(minidump,
                                getter_AddRefs(extraFile))) {
        extraFile->Remove(true);
    }
}
#endif 

void
PluginModuleChromeParent::ProcessFirstMinidump()
{
#ifdef XP_WIN
    mozilla::MutexAutoLock lock(mCrashReporterMutex);
#endif
    CrashReporterParent* crashReporter = CrashReporter();
    if (!crashReporter)
        return;

    AnnotationTable notes(4);
    WriteExtraDataForMinidump(notes);

    if (!mPluginDumpID.IsEmpty()) {
        crashReporter->GenerateChildData(&notes);
        return;
    }

    uint32_t sequence = UINT32_MAX;
    nsCOMPtr<nsIFile> dumpFile;
    nsAutoCString flashProcessType;
    TakeMinidump(getter_AddRefs(dumpFile), &sequence);

#ifdef MOZ_CRASHREPORTER_INJECTOR
    nsCOMPtr<nsIFile> childDumpFile;
    uint32_t childSequence;

    if (mFlashProcess1 &&
        TakeMinidumpForChild(mFlashProcess1,
                             getter_AddRefs(childDumpFile),
                             &childSequence)) {
        if (childSequence < sequence) {
            RemoveMinidump(dumpFile);
            dumpFile = childDumpFile;
            sequence = childSequence;
            flashProcessType.AssignLiteral("Broker");
        }
        else {
            RemoveMinidump(childDumpFile);
        }
    }
    if (mFlashProcess2 &&
        TakeMinidumpForChild(mFlashProcess2,
                             getter_AddRefs(childDumpFile),
                             &childSequence)) {
        if (childSequence < sequence) {
            RemoveMinidump(dumpFile);
            dumpFile = childDumpFile;
            sequence = childSequence;
            flashProcessType.AssignLiteral("Sandbox");
        }
        else {
            RemoveMinidump(childDumpFile);
        }
    }
#endif

    if (!dumpFile) {
        NS_WARNING("[PluginModuleParent::ActorDestroy] abnormal shutdown without minidump!");
        return;
    }

    PLUGIN_LOG_DEBUG(("got child minidump: %s",
                      NS_ConvertUTF16toUTF8(mPluginDumpID).get()));

    GetIDFromMinidump(dumpFile, mPluginDumpID);
    if (!flashProcessType.IsEmpty()) {
        notes.Put(NS_LITERAL_CSTRING("FlashProcessDump"), flashProcessType);
    }
    crashReporter->GenerateCrashReportForMinidump(dumpFile, &notes);
}
#endif

void
PluginModuleParent::ActorDestroy(ActorDestroyReason why)
{
    switch (why) {
    case AbnormalShutdown: {
        mShutdown = true;
        
        
        if (mPlugin)
            MessageLoop::current()->PostTask(
                FROM_HERE,
                mTaskFactory.NewRunnableMethod(
                    &PluginModuleParent::NotifyPluginCrashed));
        break;
    }
    case NormalShutdown:
        mShutdown = true;
        break;

    default:
        NS_RUNTIMEABORT("Unexpected shutdown reason for toplevel actor.");
    }
}

void
PluginModuleChromeParent::ActorDestroy(ActorDestroyReason why)
{
    if (why == AbnormalShutdown) {
#ifdef MOZ_CRASHREPORTER
        ProcessFirstMinidump();
#endif
        Telemetry::Accumulate(Telemetry::SUBPROCESS_ABNORMAL_ABORT,
                              NS_LITERAL_CSTRING("plugin"), 1);
    }

    
    UnregisterSettingsCallbacks();

    PluginModuleParent::ActorDestroy(why);
}

void
PluginModuleParent::NotifyPluginCrashed()
{
    if (!OkToCleanup()) {
        
        MessageLoop::current()->PostDelayedTask(
            FROM_HERE,
            mTaskFactory.NewRunnableMethod(
                &PluginModuleParent::NotifyPluginCrashed), 10);
        return;
    }

    if (mPlugin)
        mPlugin->PluginCrashed(mPluginDumpID, mBrowserDumpID);
}

PPluginInstanceParent*
PluginModuleParent::AllocPPluginInstanceParent(const nsCString& aMimeType,
                                               const uint16_t& aMode,
                                               const InfallibleTArray<nsCString>& aNames,
                                               const InfallibleTArray<nsCString>& aValues)
{
    NS_ERROR("Not reachable!");
    return nullptr;
}

bool
PluginModuleParent::DeallocPPluginInstanceParent(PPluginInstanceParent* aActor)
{
    PLUGIN_LOG_DEBUG_METHOD;
    delete aActor;
    return true;
}

void
PluginModuleParent::SetPluginFuncs(NPPluginFuncs* aFuncs)
{
    aFuncs->version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    aFuncs->javaClass = nullptr;

    
    aFuncs->newp = nullptr;
    aFuncs->clearsitedata = nullptr;
    aFuncs->getsiteswithdata = nullptr;

    aFuncs->destroy = NPP_Destroy;
    aFuncs->setwindow = NPP_SetWindow;
    aFuncs->newstream = NPP_NewStream;
    aFuncs->destroystream = NPP_DestroyStream;
    aFuncs->asfile = NPP_StreamAsFile;
    aFuncs->writeready = NPP_WriteReady;
    aFuncs->write = NPP_Write;
    aFuncs->print = NPP_Print;
    aFuncs->event = NPP_HandleEvent;
    aFuncs->urlnotify = NPP_URLNotify;
    aFuncs->getvalue = NPP_GetValue;
    aFuncs->setvalue = NPP_SetValue;
    aFuncs->gotfocus = nullptr;
    aFuncs->lostfocus = nullptr;
    aFuncs->urlredirectnotify = nullptr;

    
    
    bool urlRedirectSupported = false;
    unused << CallOptionalFunctionsSupported(&urlRedirectSupported,
                                             &mClearSiteDataSupported,
                                             &mGetSitesWithDataSupported);
    if (urlRedirectSupported) {
      aFuncs->urlredirectnotify = NPP_URLRedirectNotify;
    }
}

#define RESOLVE_AND_CALL(instance, func)                                       \
NP_BEGIN_MACRO                                                                 \
    PluginAsyncSurrogate* surrogate = nullptr;                                 \
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance, &surrogate);\
    if (surrogate && (!i || i->UseSurrogate())) {                              \
        return surrogate->func;                                                \
    }                                                                          \
    if (!i) {                                                                  \
        return NPERR_GENERIC_ERROR;                                            \
    }                                                                          \
    return i->func;                                                            \
NP_END_MACRO

NPError
PluginModuleParent::NPP_Destroy(NPP instance,
                                NPSavedData** )
{
    
    
    
    
    

    PLUGIN_LOG_DEBUG_FUNCTION;
    PluginInstanceParent* parentInstance = PluginInstanceParent::Cast(instance);

    if (!parentInstance)
        return NPERR_NO_ERROR;

    NPError retval = parentInstance->Destroy();
    instance->pdata = nullptr;

    unused << PluginInstanceParent::Call__delete__(parentInstance);
    return retval;
}

NPError
PluginModuleParent::NPP_NewStream(NPP instance, NPMIMEType type,
                                  NPStream* stream, NPBool seekable,
                                  uint16_t* stype)
{
    PROFILER_LABEL("PluginModuleParent", "NPP_NewStream",
      js::ProfileEntry::Category::OTHER);
    RESOLVE_AND_CALL(instance, NPP_NewStream(type, stream, seekable, stype));
}

NPError
PluginModuleParent::NPP_SetWindow(NPP instance, NPWindow* window)
{
    RESOLVE_AND_CALL(instance, NPP_SetWindow(window));
}

NPError
PluginModuleParent::NPP_DestroyStream(NPP instance,
                                      NPStream* stream,
                                      NPReason reason)
{
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_DestroyStream(stream, reason);
}

int32_t
PluginModuleParent::NPP_WriteReady(NPP instance,
                                   NPStream* stream)
{
    PluginAsyncSurrogate* surrogate = nullptr;
    BrowserStreamParent* s = StreamCast(instance, stream, &surrogate);
    if (!s) {
        if (surrogate) {
            return surrogate->NPP_WriteReady(stream);
        }
        return -1;
    }

    return s->WriteReady();
}

int32_t
PluginModuleParent::NPP_Write(NPP instance,
                              NPStream* stream,
                              int32_t offset,
                              int32_t len,
                              void* buffer)
{
    BrowserStreamParent* s = StreamCast(instance, stream);
    if (!s)
        return -1;

    return s->Write(offset, len, buffer);
}

void
PluginModuleParent::NPP_StreamAsFile(NPP instance,
                                     NPStream* stream,
                                     const char* fname)
{
    BrowserStreamParent* s = StreamCast(instance, stream);
    if (!s)
        return;

    s->StreamAsFile(fname);
}

void
PluginModuleParent::NPP_Print(NPP instance, NPPrint* platformPrint)
{

    PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
    i->NPP_Print(platformPrint);
}

int16_t
PluginModuleParent::NPP_HandleEvent(NPP instance, void* event)
{
    RESOLVE_AND_CALL(instance, NPP_HandleEvent(event));
}

void
PluginModuleParent::NPP_URLNotify(NPP instance, const char* url,
                                  NPReason reason, void* notifyData)
{
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
    if (!i)
        return;

    i->NPP_URLNotify(url, reason, notifyData);
}

NPError
PluginModuleParent::NPP_GetValue(NPP instance,
                                 NPPVariable variable, void *ret_value)
{
    
    
    PluginAsyncSurrogate* surrogate = nullptr;
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance, &surrogate);
    if (surrogate) {
        return surrogate->NPP_GetValue(variable, ret_value);
    }
    if (!i) {
        return NPERR_GENERIC_ERROR;
    }
    return i->NPP_GetValue(variable, ret_value);
}

NPError
PluginModuleParent::NPP_SetValue(NPP instance, NPNVariable variable,
                                 void *value)
{
    RESOLVE_AND_CALL(instance, NPP_SetValue(variable, value));
}

bool
PluginModuleParent::RecvBackUpXResources(const FileDescriptor& aXSocketFd)
{
#ifndef MOZ_X11
    NS_RUNTIMEABORT("This message only makes sense on X11 platforms");
#else
    NS_ABORT_IF_FALSE(0 > mPluginXSocketFdDup.get(),
                      "Already backed up X resources??");
    mPluginXSocketFdDup.forget();
    if (aXSocketFd.IsValid()) {
      mPluginXSocketFdDup.reset(aXSocketFd.PlatformHandle());
    }
#endif
    return true;
}

void
PluginModuleParent::NPP_URLRedirectNotify(NPP instance, const char* url,
                                          int32_t status, void* notifyData)
{
  PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
  if (!i)
    return;

  i->NPP_URLRedirectNotify(url, status, notifyData);
}

BrowserStreamParent*
PluginModuleParent::StreamCast(NPP instance, NPStream* s,
                               PluginAsyncSurrogate** aSurrogate)
{
    PluginInstanceParent* ip = PluginInstanceParent::Cast(instance, aSurrogate);
    if (!ip || (aSurrogate && *aSurrogate && ip->UseSurrogate())) {
        return nullptr;
    }

    BrowserStreamParent* sp =
        static_cast<BrowserStreamParent*>(static_cast<AStream*>(s->pdata));
    if (sp->mNPP != ip || s != sp->mStream) {
        NS_RUNTIMEABORT("Corrupted plugin stream data.");
    }
    return sp;
}

bool
PluginModuleParent::HasRequiredFunctions()
{
    return true;
}

nsresult
PluginModuleParent::AsyncSetWindow(NPP instance, NPWindow* window)
{
    PluginAsyncSurrogate* surrogate = nullptr;
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance, &surrogate);
    if (surrogate && (!i || i->UseSurrogate())) {
        return surrogate->AsyncSetWindow(window);
    } else if (!i) {
        return NS_ERROR_FAILURE;
    }
    return i->AsyncSetWindow(window);
}

nsresult
PluginModuleParent::GetImageContainer(NPP instance,
                             mozilla::layers::ImageContainer** aContainer)
{
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
    return !i ? NS_ERROR_FAILURE : i->GetImageContainer(aContainer);
}

nsresult
PluginModuleParent::GetImageSize(NPP instance,
                                 nsIntSize* aSize)
{
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
    return !i ? NS_ERROR_FAILURE : i->GetImageSize(aSize);
}

nsresult
PluginModuleParent::SetBackgroundUnknown(NPP instance)
{
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->SetBackgroundUnknown();
}

nsresult
PluginModuleParent::BeginUpdateBackground(NPP instance,
                                          const nsIntRect& aRect,
                                          gfxContext** aCtx)
{
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->BeginUpdateBackground(aRect, aCtx);
}

nsresult
PluginModuleParent::EndUpdateBackground(NPP instance,
                                        gfxContext* aCtx,
                                        const nsIntRect& aRect)
{
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->EndUpdateBackground(aCtx, aRect);
}

void
PluginModuleParent::OnInitFailure()
{
    if (GetIPCChannel()->CanSend()) {
        Close();
    }

    if (mIsStartingAsync) {
        

        uint32_t len = mSurrogateInstances.Length();
        for (uint32_t i = 0; i < len; ++i) {
            mSurrogateInstances[i]->NotifyAsyncInitFailed();
        }
        mSurrogateInstances.Clear();
    }
}

class OfflineObserver MOZ_FINAL : public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    explicit OfflineObserver(PluginModuleChromeParent* pmp)
      : mPmp(pmp)
    {}

private:
    ~OfflineObserver() {}
    PluginModuleChromeParent* mPmp;
};

NS_IMPL_ISUPPORTS(OfflineObserver, nsIObserver)

NS_IMETHODIMP
OfflineObserver::Observe(nsISupports *aSubject,
                         const char *aTopic,
                         const char16_t *aData)
{
    MOZ_ASSERT(!strcmp(aTopic, "ipc:network:set-offline"));
    mPmp->CachedSettingChanged();
    return NS_OK;
}

static const char* kSettingsPrefs[] =
    {"javascript.enabled",
     "dom.ipc.plugins.nativeCursorSupport"};

void
PluginModuleChromeParent::RegisterSettingsCallbacks()
{
    for (size_t i = 0; i < ArrayLength(kSettingsPrefs); i++) {
        Preferences::RegisterCallback(CachedSettingChanged, kSettingsPrefs[i], this);
    }

    nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
    if (observerService) {
        mOfflineObserver = new OfflineObserver(this);
        observerService->AddObserver(mOfflineObserver, "ipc:network:set-offline", false);
    }
}

void
PluginModuleChromeParent::UnregisterSettingsCallbacks()
{
    for (size_t i = 0; i < ArrayLength(kSettingsPrefs); i++) {
        Preferences::UnregisterCallback(CachedSettingChanged, kSettingsPrefs[i], this);
    }

    nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
    if (observerService) {
        observerService->RemoveObserver(mOfflineObserver, "ipc:network:set-offline");
        mOfflineObserver = nullptr;
    }
}

bool
PluginModuleParent::GetSetting(NPNVariable aVariable)
{
    NPBool boolVal = false;
    mozilla::plugins::parent::_getvalue(nullptr, aVariable, &boolVal);
    return boolVal;
}

void
PluginModuleParent::GetSettings(PluginSettings* aSettings)
{
    aSettings->javascriptEnabled() = GetSetting(NPNVjavascriptEnabledBool);
    aSettings->asdEnabled() = GetSetting(NPNVasdEnabledBool);
    aSettings->isOffline() = GetSetting(NPNVisOfflineBool);
    aSettings->supportsXembed() = GetSetting(NPNVSupportsXEmbedBool);
    aSettings->supportsWindowless() = GetSetting(NPNVSupportsWindowless);
    aSettings->userAgent() = NullableString(mNPNIface->uagent(nullptr));

#if defined(XP_MACOSX)
    aSettings->nativeCursorsSupported() =
      Preferences::GetBool("dom.ipc.plugins.nativeCursorSupport", false);
#else
    
    aSettings->nativeCursorsSupported() = false;
#endif
}

void
PluginModuleChromeParent::CachedSettingChanged()
{
    PluginSettings settings;
    GetSettings(&settings);
    unused << SendSettingChanged(settings);
}

 void
PluginModuleChromeParent::CachedSettingChanged(const char* aPref, void* aModule)
{
    PluginModuleChromeParent *module = static_cast<PluginModuleChromeParent*>(aModule);
    module->CachedSettingChanged();
}

#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(MOZ_WIDGET_GONK)
nsresult
PluginModuleParent::NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    mNPNIface = bFuncs;
    mNPPIface = pFuncs;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    if (mIsStartingAsync) {
        PluginAsyncSurrogate::NP_GetEntryPoints(pFuncs);
    } else {
        SetPluginFuncs(pFuncs);
    }

    *error = NPERR_NO_ERROR;
    return NS_OK;
}

nsresult
PluginModuleChromeParent::NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    *error = NPERR_NO_ERROR;

    mNPNIface = bFuncs;
    mNPPIface = pFuncs;

    
    
    if (mIsStartingAsync) {
        PluginAsyncSurrogate::NP_GetEntryPoints(pFuncs);
    }

    if (!mSubprocess->IsConnected()) {
        
        
        mInitOnAsyncConnect = true;
        return NS_OK;
    }

    PluginSettings settings;
    GetSettings(&settings);

    TimeStamp callNpInitStart = TimeStamp::Now();
    
    if (mIsStartingAsync) {
        if (!SendAsyncNP_Initialize(settings)) {
            Close();
            return NS_ERROR_FAILURE;
        }
        TimeStamp callNpInitEnd = TimeStamp::Now();
        mTimeBlocked += (callNpInitEnd - callNpInitStart);
        return NS_OK;
    }

    
    if (!CallNP_Initialize(settings, error)) {
        Close();
        return NS_ERROR_FAILURE;
    }
    else if (*error != NPERR_NO_ERROR) {
        Close();
        return NS_ERROR_FAILURE;
    }
    TimeStamp callNpInitEnd = TimeStamp::Now();
    mTimeBlocked += (callNpInitEnd - callNpInitStart);

    RecvNP_InitializeResult(*error);

    return NS_OK;
}

bool
PluginModuleParent::RecvNP_InitializeResult(const NPError& aError)
{
    if (aError != NPERR_NO_ERROR) {
        OnInitFailure();
        return true;
    }

    SetPluginFuncs(mNPPIface);
    if (mIsStartingAsync) {
        InitAsyncSurrogates();
    }

    mNPInitialized = true;
    return true;
}

bool
PluginModuleChromeParent::RecvNP_InitializeResult(const NPError& aError)
{
    if (!mContentParent) {
        return PluginModuleParent::RecvNP_InitializeResult(aError);
    }
    bool initOk = aError == NPERR_NO_ERROR;
    if (initOk) {
        SetPluginFuncs(mNPPIface);
        if (mIsStartingAsync && !SendAssociatePluginId()) {
            initOk = false;
        }
    }
    mNPInitialized = initOk;
    return mContentParent->SendLoadPluginResult(mPluginId, initOk);
}

#else

nsresult
PluginModuleParent::NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    mNPNIface = bFuncs;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    *error = NPERR_NO_ERROR;
    return NS_OK;
}

nsresult
PluginModuleChromeParent::NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error)
{
    nsresult rv = PluginModuleParent::NP_Initialize(bFuncs, error);
    if (NS_FAILED(rv))
        return rv;

#if defined(XP_MACOSX)
    if (!mSubprocess->IsConnected()) {
        
        
        mInitOnAsyncConnect = true;
        *error = NPERR_NO_ERROR;
        return NS_OK;
    }
#else
    if (mInitOnAsyncConnect) {
        *error = NPERR_NO_ERROR;
        return NS_OK;
    }
#endif

    PluginSettings settings;
    GetSettings(&settings);

    TimeStamp callNpInitStart = TimeStamp::Now();
    if (mIsStartingAsync) {
        if (!SendAsyncNP_Initialize(settings)) {
            return NS_ERROR_FAILURE;
        }
        TimeStamp callNpInitEnd = TimeStamp::Now();
        mTimeBlocked += (callNpInitEnd - callNpInitStart);
        return NS_OK;
    }

    if (!CallNP_Initialize(settings, error)) {
        Close();
        return NS_ERROR_FAILURE;
    }
    TimeStamp callNpInitEnd = TimeStamp::Now();
    mTimeBlocked += (callNpInitEnd - callNpInitStart);
    RecvNP_InitializeResult(*error);
    return NS_OK;
}

bool
PluginModuleParent::RecvNP_InitializeResult(const NPError& aError)
{
    if (aError != NPERR_NO_ERROR) {
        OnInitFailure();
        return true;
    }

    if (mIsStartingAsync) {
#if defined(XP_WIN)
        SetPluginFuncs(mNPPIface);
#endif
        InitAsyncSurrogates();
    }

    mNPInitialized = true;
    return true;
}

bool
PluginModuleChromeParent::RecvNP_InitializeResult(const NPError& aError)
{
    bool ok = true;
    if (mContentParent) {
        if ((ok = SendAssociatePluginId())) {
            ok = mContentParent->SendLoadPluginResult(mPluginId,
                                                      aError == NPERR_NO_ERROR);
        }
    } else if (aError == NPERR_NO_ERROR) {
        
#if defined XP_WIN
        if (mIsStartingAsync) {
            SetPluginFuncs(mNPPIface);
        }

        
        
        nsID id;
        nsString sessionName;
        nsString iconPath;

        if (NS_SUCCEEDED(mozilla::widget::GetAudioSessionData(id, sessionName,
                                                              iconPath))) {
            unused << SendSetAudioSessionData(id, sessionName, iconPath);
        }
#endif

#ifdef MOZ_CRASHREPORTER_INJECTOR
        InitializeInjector();
#endif
    }

    return PluginModuleParent::RecvNP_InitializeResult(aError) && ok;
}

#endif

void
PluginModuleParent::InitAsyncSurrogates()
{
    uint32_t len = mSurrogateInstances.Length();
    for (uint32_t i = 0; i < len; ++i) {
        NPError err;
        mAsyncNewRv = mSurrogateInstances[i]->NPP_New(&err);
        if (NS_FAILED(mAsyncNewRv)) {
            mSurrogateInstances[i]->NotifyAsyncInitFailed();
            continue;
        }
    }
    mSurrogateInstances.Clear();
}

bool
PluginModuleParent::RemovePendingSurrogate(
                            const nsRefPtr<PluginAsyncSurrogate>& aSurrogate)
{
    return mSurrogateInstances.RemoveElement(aSurrogate);
}

nsresult
PluginModuleParent::NP_Shutdown(NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    bool ok = true;
    if (IsChrome()) {
        ok = CallNP_Shutdown(error);
    }

    
    
    
    
    Close();

    return ok ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
PluginModuleParent::NP_GetMIMEDescription(const char** mimeDesc)
{
    PLUGIN_LOG_DEBUG_METHOD;

    *mimeDesc = "application/x-foobar";
    return NS_OK;
}

nsresult
PluginModuleParent::NP_GetValue(void *future, NPPVariable aVariable,
                                   void *aValue, NPError* error)
{
    PR_LOG(GetPluginLog(), PR_LOG_WARNING, ("%s Not implemented, requested variable %i", __FUNCTION__,
                                        (int) aVariable));

    
    *error = NPERR_GENERIC_ERROR;
    return NS_OK;
}

#if defined(XP_WIN) || defined(XP_MACOSX)
nsresult
PluginModuleParent::NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error)
{
    NS_ASSERTION(pFuncs, "Null pointer!");

    *error = NPERR_NO_ERROR;
    if (mIsStartingAsync && !IsChrome()) {
        PluginAsyncSurrogate::NP_GetEntryPoints(pFuncs);
        mNPPIface = pFuncs;
    } else {
        SetPluginFuncs(pFuncs);
    }

    return NS_OK;
}

nsresult
PluginModuleChromeParent::NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error)
{
#if defined(XP_MACOSX)
    if (mInitOnAsyncConnect) {
        PluginAsyncSurrogate::NP_GetEntryPoints(pFuncs);
        mNPPIface = pFuncs;
        *error = NPERR_NO_ERROR;
        return NS_OK;
    }
#else
    if (mIsStartingAsync) {
        PluginAsyncSurrogate::NP_GetEntryPoints(pFuncs);
    }
    if (!mSubprocess->IsConnected()) {
        mNPPIface = pFuncs;
        mInitOnAsyncConnect = true;
        *error = NPERR_NO_ERROR;
        return NS_OK;
    }
#endif

    
    
    
    

    if (!CallNP_GetEntryPoints(error)) {
        return NS_ERROR_FAILURE;
    }
    else if (*error != NPERR_NO_ERROR) {
        return NS_OK;
    }

    return PluginModuleParent::NP_GetEntryPoints(pFuncs, error);
}

#endif

nsresult
PluginModuleParent::NPP_New(NPMIMEType pluginType, NPP instance,
                            uint16_t mode, int16_t argc, char* argn[],
                            char* argv[], NPSavedData* saved,
                            NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    if (mIsStartingAsync) {
        if (!PluginAsyncSurrogate::Create(this, pluginType, instance, mode,
                                          argc, argn, argv)) {
            *error = NPERR_GENERIC_ERROR;
            return NS_ERROR_FAILURE;
        }

        if (!mNPInitialized) {
            nsRefPtr<PluginAsyncSurrogate> surrogate =
                PluginAsyncSurrogate::Cast(instance);
            mSurrogateInstances.AppendElement(surrogate);
            *error = NPERR_NO_ERROR;
            return NS_PLUGIN_INIT_PENDING;
        }
    }

    if (mPluginName.IsEmpty()) {
        GetPluginDetails(mPluginName, mPluginVersion);
        





        Telemetry::Accumulate(Telemetry::BLOCKED_ON_PLUGIN_MODULE_INIT_MS,
                              GetHistogramKey(),
                              static_cast<uint32_t>(mTimeBlocked.ToMilliseconds()));
        mTimeBlocked = TimeDuration();
    }

    
    InfallibleTArray<nsCString> names;
    InfallibleTArray<nsCString> values;

    for (int i = 0; i < argc; ++i) {
        names.AppendElement(NullableString(argn[i]));
        values.AppendElement(NullableString(argv[i]));
    }

    nsresult rv = NPP_NewInternal(pluginType, instance, mode, names, values,
                                  saved, error);
    if (NS_FAILED(rv) || !mIsStartingAsync) {
        return rv;
    }
    return NS_PLUGIN_INIT_PENDING;
}

nsresult
PluginModuleParent::NPP_NewInternal(NPMIMEType pluginType, NPP instance,
                                    uint16_t mode,
                                    InfallibleTArray<nsCString>& names,
                                    InfallibleTArray<nsCString>& values,
                                    NPSavedData* saved, NPError* error)
{
    PluginInstanceParent* parentInstance =
        new PluginInstanceParent(this, instance,
                                 nsDependentCString(pluginType), mNPNIface);

    if (!parentInstance->Init()) {
        delete parentInstance;
        return NS_ERROR_FAILURE;
    }

    
    nsRefPtr<PluginAsyncSurrogate> surrogate(
        dont_AddRef(PluginAsyncSurrogate::Cast(instance)));
    
    instance->pdata = static_cast<PluginDataResolver*>(parentInstance);

    if (!SendPPluginInstanceConstructor(parentInstance,
                                        nsDependentCString(pluginType), mode,
                                        names, values)) {
        
        instance->pdata = nullptr;
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    {   
        Telemetry::AutoTimer<Telemetry::BLOCKED_ON_PLUGIN_INSTANCE_INIT_MS>
            timer(GetHistogramKey());
        if (mIsStartingAsync) {
            MOZ_ASSERT(surrogate);
            surrogate->AsyncCallDeparting();
            if (!SendAsyncNPP_New(parentInstance)) {
                *error = NPERR_GENERIC_ERROR;
                return NS_ERROR_FAILURE;
            }
            *error = NPERR_NO_ERROR;
        } else {
            if (!CallSyncNPP_New(parentInstance, error)) {
                
                
                
                if (NPERR_NO_ERROR == *error) {
                    *error = NPERR_GENERIC_ERROR;
                }
                return NS_ERROR_FAILURE;
            }
        }
    }

    if (*error != NPERR_NO_ERROR) {
        if (!mIsStartingAsync) {
            NPP_Destroy(instance, 0);
        }
        return NS_ERROR_FAILURE;
    }

    UpdatePluginTimeout();

    return NS_OK;
}

void
PluginModuleChromeParent::UpdatePluginTimeout()
{
    TimeoutChanged(kParentTimeoutPref, this);
}

nsresult
PluginModuleParent::NPP_ClearSiteData(const char* site, uint64_t flags,
                                      uint64_t maxAge)
{
    if (!mClearSiteDataSupported)
        return NS_ERROR_NOT_AVAILABLE;

    NPError result;
    if (!CallNPP_ClearSiteData(NullableString(site), flags, maxAge, &result))
        return NS_ERROR_FAILURE;

    switch (result) {
    case NPERR_NO_ERROR:
        return NS_OK;
    case NPERR_TIME_RANGE_NOT_SUPPORTED:
        return NS_ERROR_PLUGIN_TIME_RANGE_NOT_SUPPORTED;
    case NPERR_MALFORMED_SITE:
        return NS_ERROR_INVALID_ARG;
    default:
        return NS_ERROR_FAILURE;
    }
}

nsresult
PluginModuleParent::NPP_GetSitesWithData(InfallibleTArray<nsCString>& result)
{
    if (!mGetSitesWithDataSupported)
        return NS_ERROR_NOT_AVAILABLE;

    if (!CallNPP_GetSitesWithData(&result))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

#if defined(XP_MACOSX)
nsresult
PluginModuleParent::IsRemoteDrawingCoreAnimation(NPP instance, bool *aDrawing)
{
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->IsRemoteDrawingCoreAnimation(aDrawing);
}

nsresult
PluginModuleParent::ContentsScaleFactorChanged(NPP instance, double aContentsScaleFactor)
{
    PluginInstanceParent* i = PluginInstanceParent::Cast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->ContentsScaleFactorChanged(aContentsScaleFactor);
}
#endif 

#if defined(MOZ_WIDGET_QT)
bool
PluginModuleParent::AnswerProcessSomeEvents()
{
    PLUGIN_LOG_DEBUG(("Spinning mini nested loop ..."));
    PluginHelperQt::AnswerProcessSomeEvents();
    PLUGIN_LOG_DEBUG(("... quitting mini nested loop"));

    return true;
}

#elif defined(XP_MACOSX)
bool
PluginModuleParent::AnswerProcessSomeEvents()
{
    mozilla::plugins::PluginUtilsOSX::InvokeNativeEventLoop();
    return true;
}

#elif !defined(MOZ_WIDGET_GTK)
bool
PluginModuleParent::AnswerProcessSomeEvents()
{
    NS_RUNTIMEABORT("unreached");
    return false;
}

#else
static const int kMaxChancesToProcessEvents = 20;

bool
PluginModuleParent::AnswerProcessSomeEvents()
{
    PLUGIN_LOG_DEBUG(("Spinning mini nested loop ..."));

    int i = 0;
    for (; i < kMaxChancesToProcessEvents; ++i)
        if (!g_main_context_iteration(nullptr, FALSE))
            break;

    PLUGIN_LOG_DEBUG(("... quitting mini nested loop; processed %i tasks", i));

    return true;
}
#endif

bool
PluginModuleParent::RecvProcessNativeEventsInInterruptCall()
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));
#if defined(OS_WIN)
    ProcessNativeEventsInInterruptCall();
    return true;
#else
    NS_NOTREACHED(
        "PluginModuleParent::RecvProcessNativeEventsInInterruptCall not implemented!");
    return false;
#endif
}

void
PluginModuleParent::ProcessRemoteNativeEventsInInterruptCall()
{
#if defined(OS_WIN)
    unused << SendProcessNativeEventsInInterruptCall();
    return;
#endif
    NS_NOTREACHED(
        "PluginModuleParent::ProcessRemoteNativeEventsInInterruptCall not implemented!");
}

bool
PluginModuleParent::RecvPluginShowWindow(const uint32_t& aWindowId, const bool& aModal,
                                         const int32_t& aX, const int32_t& aY,
                                         const size_t& aWidth, const size_t& aHeight)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));
#if defined(XP_MACOSX)
    CGRect windowBound = ::CGRectMake(aX, aY, aWidth, aHeight);
    mac_plugin_interposing::parent::OnPluginShowWindow(aWindowId, windowBound, aModal);
    return true;
#else
    NS_NOTREACHED(
        "PluginInstanceParent::RecvPluginShowWindow not implemented!");
    return false;
#endif
}

bool
PluginModuleParent::RecvPluginHideWindow(const uint32_t& aWindowId)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));
#if defined(XP_MACOSX)
    mac_plugin_interposing::parent::OnPluginHideWindow(aWindowId, OtherSidePID());
    return true;
#else
    NS_NOTREACHED(
        "PluginInstanceParent::RecvPluginHideWindow not implemented!");
    return false;
#endif
}

PCrashReporterParent*
PluginModuleParent::AllocPCrashReporterParent(mozilla::dom::NativeThreadId* id,
                                              uint32_t* processType)
{
    MOZ_CRASH("unreachable");
}

bool
PluginModuleParent::DeallocPCrashReporterParent(PCrashReporterParent* actor)
{
    MOZ_CRASH("unreachable");
}

PCrashReporterParent*
PluginModuleChromeParent::AllocPCrashReporterParent(mozilla::dom::NativeThreadId* id,
                                                    uint32_t* processType)
{
#ifdef MOZ_CRASHREPORTER
    return new CrashReporterParent();
#else
    return nullptr;
#endif
}

bool
PluginModuleChromeParent::DeallocPCrashReporterParent(PCrashReporterParent* actor)
{
#ifdef MOZ_CRASHREPORTER
#ifdef XP_WIN
    mozilla::MutexAutoLock lock(mCrashReporterMutex);
    if (actor == static_cast<PCrashReporterParent*>(mCrashReporter)) {
        mCrashReporter = nullptr;
    }
#endif
#endif
    delete actor;
    return true;
}

bool
PluginModuleParent::RecvSetCursor(const NSCursorInfo& aCursorInfo)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));
#if defined(XP_MACOSX)
    mac_plugin_interposing::parent::OnSetCursor(aCursorInfo);
    return true;
#else
    NS_NOTREACHED(
        "PluginInstanceParent::RecvSetCursor not implemented!");
    return false;
#endif
}

bool
PluginModuleParent::RecvShowCursor(const bool& aShow)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));
#if defined(XP_MACOSX)
    mac_plugin_interposing::parent::OnShowCursor(aShow);
    return true;
#else
    NS_NOTREACHED(
        "PluginInstanceParent::RecvShowCursor not implemented!");
    return false;
#endif
}

bool
PluginModuleParent::RecvPushCursor(const NSCursorInfo& aCursorInfo)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));
#if defined(XP_MACOSX)
    mac_plugin_interposing::parent::OnPushCursor(aCursorInfo);
    return true;
#else
    NS_NOTREACHED(
        "PluginInstanceParent::RecvPushCursor not implemented!");
    return false;
#endif
}

bool
PluginModuleParent::RecvPopCursor()
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));
#if defined(XP_MACOSX)
    mac_plugin_interposing::parent::OnPopCursor();
    return true;
#else
    NS_NOTREACHED(
        "PluginInstanceParent::RecvPopCursor not implemented!");
    return false;
#endif
}

bool
PluginModuleParent::RecvNPN_SetException(const nsCString& aMessage)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));

    
    mozilla::plugins::parent::_setexception(nullptr, NullableStringGet(aMessage));
    return true;
}

bool
PluginModuleParent::RecvNPN_ReloadPlugins(const bool& aReloadPages)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));

    mozilla::plugins::parent::_reloadplugins(aReloadPages);
    return true;
}

bool
PluginModuleChromeParent::RecvNotifyContentModuleDestroyed()
{
    nsRefPtr<nsPluginHost> host = nsPluginHost::GetInst();
    if (host) {
        host->NotifyContentModuleDestroyed(mPluginId);
    }
    return true;
}

#ifdef MOZ_CRASHREPORTER_INJECTOR



#define FLASH_PROCESS_PREFIX "FLASHPLAYERPLUGIN"

static DWORD
GetFlashChildOfPID(DWORD pid, HANDLE snapshot)
{
    PROCESSENTRY32 entry = {
        sizeof(entry)
    };
    for (BOOL ok = Process32First(snapshot, &entry);
         ok;
         ok = Process32Next(snapshot, &entry)) {
        if (entry.th32ParentProcessID == pid) {
            nsString name(entry.szExeFile);
            ToUpperCase(name);
            if (StringBeginsWith(name, NS_LITERAL_STRING(FLASH_PROCESS_PREFIX))) {
                return entry.th32ProcessID;
            }
        }
    }
    return 0;
}


#define FLASH_PLUGIN_PREFIX "NPSWF"

void
PluginModuleChromeParent::InitializeInjector()
{
    if (!Preferences::GetBool("dom.ipc.plugins.flash.subprocess.crashreporter.enabled", false))
        return;

    nsCString path(Process()->GetPluginFilePath().c_str());
    ToUpperCase(path);
    int32_t lastSlash = path.RFindCharInSet("\\/");
    if (kNotFound == lastSlash)
        return;

    if (!StringBeginsWith(Substring(path, lastSlash + 1),
                          NS_LITERAL_CSTRING(FLASH_PLUGIN_PREFIX)))
        return;

    TimeStamp th32Start = TimeStamp::Now();
    nsAutoHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (INVALID_HANDLE_VALUE == snapshot)
        return;
    TimeStamp th32End = TimeStamp::Now();
    mTimeBlocked += (th32End - th32Start);

    DWORD pluginProcessPID = GetProcessId(Process()->GetChildProcessHandle());
    mFlashProcess1 = GetFlashChildOfPID(pluginProcessPID, snapshot);
    if (mFlashProcess1) {
        InjectCrashReporterIntoProcess(mFlashProcess1, this);

        mFlashProcess2 = GetFlashChildOfPID(mFlashProcess1, snapshot);
        if (mFlashProcess2) {
            InjectCrashReporterIntoProcess(mFlashProcess2, this);
        }
    }
}

void
PluginModuleChromeParent::OnCrash(DWORD processID)
{
    if (!mShutdown) {
        GetIPCChannel()->CloseWithError();
        KillProcess(OtherProcess(), 1, false);
    }
}

#endif 

#ifdef MOZ_ENABLE_PROFILER_SPS
class PluginProfilerObserver MOZ_FINAL : public nsIObserver,
                                         public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    explicit PluginProfilerObserver(PluginModuleParent* pmp)
      : mPmp(pmp)
    {}

private:
    ~PluginProfilerObserver() {}
    PluginModuleParent* mPmp;
};

NS_IMPL_ISUPPORTS(PluginProfilerObserver, nsIObserver, nsISupportsWeakReference)

NS_IMETHODIMP
PluginProfilerObserver::Observe(nsISupports *aSubject,
                                const char *aTopic,
                                const char16_t *aData)
{
    if (!strcmp(aTopic, "profiler-started")) {
        nsCOMPtr<nsIProfilerStartParams> params(do_QueryInterface(aSubject));
        uint32_t entries;
        double interval;
        params->GetEntries(&entries);
        params->GetInterval(&interval);
        const nsTArray<nsCString>& features = params->GetFeatures();
        const nsTArray<nsCString>& threadFilterNames = params->GetThreadFilterNames();
        unused << mPmp->SendStartProfiler(entries, interval, features, threadFilterNames);
    } else if (!strcmp(aTopic, "profiler-stopped")) {
        unused << mPmp->SendStopProfiler();
    } else if (!strcmp(aTopic, "profiler-subprocess")) {
        nsCOMPtr<nsIProfileSaveEvent> pse = do_QueryInterface(aSubject);
        if (pse) {
            nsCString result;
            bool success = mPmp->CallGetProfile(&result);
            if (success && !result.IsEmpty()) {
                pse->AddSubProfile(result.get());
            }
        }
    }
    return NS_OK;
}

void
PluginModuleChromeParent::InitPluginProfiling()
{
    nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
    if (observerService) {
        mProfilerObserver = new PluginProfilerObserver(this);
        observerService->AddObserver(mProfilerObserver, "profiler-started", false);
        observerService->AddObserver(mProfilerObserver, "profiler-stopped", false);
        observerService->AddObserver(mProfilerObserver, "profiler-subprocess", false);
    }
}

void
PluginModuleChromeParent::ShutdownPluginProfiling()
{
    nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
    if (observerService) {
        observerService->RemoveObserver(mProfilerObserver, "profiler-started");
        observerService->RemoveObserver(mProfilerObserver, "profiler-stopped");
        observerService->RemoveObserver(mProfilerObserver, "profiler-subprocess");
    }
}
#endif
