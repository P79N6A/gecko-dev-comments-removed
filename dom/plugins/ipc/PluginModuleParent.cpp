





#ifdef MOZ_WIDGET_GTK
#include <glib.h>
#elif XP_MACOSX
#include "PluginInterposeOSX.h"
#include "PluginUtilsOSX.h"
#endif
#ifdef MOZ_WIDGET_QT
#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include "NestedLoopTimer.h"
#endif

#include "base/process_util.h"

#include "mozilla/Preferences.h"
#include "mozilla/unused.h"
#include "mozilla/ipc/SyncChannel.h"
#include "mozilla/plugins/PluginModuleParent.h"
#include "mozilla/plugins/BrowserStreamParent.h"
#include "mozilla/dom/PCrashReporterParent.h"
#include "PluginIdentifierParent.h"

#include "nsAutoPtr.h"
#include "nsCRT.h"
#include "nsNPAPIPlugin.h"
#include "nsIFile.h"
#include "nsPrintfCString.h"

#include "prsystem.h"

#ifdef XP_WIN
#include "PluginHangUIParent.h"
#include "mozilla/widget/AudioSession.h"
#endif
#include "GeckoProfiler.h"

using base::KillProcess;

using mozilla::PluginLibrary;
using mozilla::ipc::SyncChannel;
using mozilla::dom::PCrashReporterParent;
using mozilla::dom::CrashReporterParent;

using namespace mozilla;
using namespace mozilla::plugins;
using namespace mozilla::plugins::parent;

#ifdef MOZ_CRASHREPORTER
#include "mozilla/dom/CrashReporterParent.h"

using namespace CrashReporter;
#endif

static const char kChildTimeoutPref[] = "dom.ipc.plugins.timeoutSecs";
static const char kParentTimeoutPref[] = "dom.ipc.plugins.parentTimeoutSecs";
static const char kLaunchTimeoutPref[] = "dom.ipc.plugins.processLaunchTimeoutSecs";
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


PluginLibrary*
PluginModuleParent::LoadModule(const char* aFilePath)
{
    PLUGIN_LOG_DEBUG_FUNCTION;

    int32_t prefSecs = Preferences::GetInt(kLaunchTimeoutPref, 0);

    
    nsAutoPtr<PluginModuleParent> parent(new PluginModuleParent(aFilePath));
    bool launched = parent->mSubprocess->Launch(prefSecs * 1000);
    if (!launched) {
        
        parent->mShutdown = true;
        return nullptr;
    }
    parent->Open(parent->mSubprocess->GetChannel(),
                 parent->mSubprocess->GetChildProcessHandle());

    TimeoutChanged(CHILD_TIMEOUT_PREF, parent);

#ifdef MOZ_CRASHREPORTER
    
    if (!CrashReporterParent::CreateCrashReporter(parent.get())) {
        parent->mShutdown = true;
        return nullptr;
    }
#endif

    return parent.forget();
}


PluginModuleParent::PluginModuleParent(const char* aFilePath)
    : mSubprocess(new PluginProcessParent(aFilePath))
    , mShutdown(false)
    , mClearSiteDataSupported(false)
    , mGetSitesWithDataSupported(false)
    , mNPNIface(NULL)
    , mPlugin(NULL)
    , ALLOW_THIS_IN_INITIALIZER_LIST(mTaskFactory(this))
#ifdef XP_WIN
    , mPluginCpuUsageOnHang()
    , mHangUIParent(nullptr)
    , mHangUIEnabled(true)
    , mIsTimerReset(true)
#endif
#ifdef MOZ_CRASHREPORTER_INJECTOR
    , mFlashProcess1(0)
    , mFlashProcess2(0)
#endif
{
    NS_ASSERTION(mSubprocess, "Out of memory!");

    mIdentifiers.Init();

    Preferences::RegisterCallback(TimeoutChanged, kChildTimeoutPref, this);
    Preferences::RegisterCallback(TimeoutChanged, kParentTimeoutPref, this);
#ifdef XP_WIN
    Preferences::RegisterCallback(TimeoutChanged, kHangUITimeoutPref, this);
    Preferences::RegisterCallback(TimeoutChanged, kHangUIMinDisplayPref, this);
#endif
}

PluginModuleParent::~PluginModuleParent()
{
    NS_ASSERTION(OkToCleanup(), "unsafe destruction");

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
}

#ifdef MOZ_CRASHREPORTER
void
PluginModuleParent::WriteExtraDataForMinidump(AnnotationTable& notes)
{
    typedef nsDependentCString CS;

    
    const std::string& pluginFile = mSubprocess->GetPluginFilePath();
    size_t filePos = pluginFile.rfind(FILE_PATH_SEPARATOR);
    if (filePos == std::string::npos)
        filePos = 0;
    else
        filePos++;
    notes.Put(NS_LITERAL_CSTRING("PluginFilename"), CS(pluginFile.substr(filePos).c_str()));

    nsCString pluginName;
    nsCString pluginVersion;

    nsRefPtr<nsPluginHost> ph = nsPluginHost::GetInst();
    if (ph) {
        nsPluginTag* tag = ph->TagForPlugin(mPlugin);
        if (tag) {
            pluginName = tag->mName;
            pluginVersion = tag->mVersion;
        }
    }
        
    notes.Put(NS_LITERAL_CSTRING("PluginName"), pluginName);
    notes.Put(NS_LITERAL_CSTRING("PluginVersion"), pluginVersion);

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
                      SyncChannel::kNoTimeout;
    SetReplyTimeoutMs(timeoutMs);
}

int
PluginModuleParent::TimeoutChanged(const char* aPref, void* aModule)
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
#ifndef XP_WIN
    if (!strcmp(aPref, kChildTimeoutPref)) {
      
      int32_t timeoutSecs = Preferences::GetInt(kChildTimeoutPref, 0);
      static_cast<PluginModuleParent*>(aModule)->SetChildTimeout(timeoutSecs);
#else
    if (!strcmp(aPref, kChildTimeoutPref) ||
        !strcmp(aPref, kHangUIMinDisplayPref) ||
        !strcmp(aPref, kHangUITimeoutPref)) {
      static_cast<PluginModuleParent*>(aModule)->EvaluateHangUIState(true);
#endif
    } else if (!strcmp(aPref, kParentTimeoutPref)) {
      
      int32_t timeoutSecs = Preferences::GetInt(kParentTimeoutPref, 0);
      unused << static_cast<PluginModuleParent*>(aModule)->SendSetParentHangTimeout(timeoutSecs);
    }
    return 0;
}

void
PluginModuleParent::CleanupFromTimeout(const bool aFromHangUI)
{
    if (mShutdown) {
      return;
    }

    if (!OkToCleanup()) {
        
        MessageLoop::current()->PostDelayedTask(
            FROM_HERE,
            mTaskFactory.NewRunnableMethod(
                &PluginModuleParent::CleanupFromTimeout, aFromHangUI), 10);
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

void
PluginModuleParent::ExitedCxxStack()
{
    FinishHangUI();
}

#endif 

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
PluginModuleParent::ShouldContinueFromReplyTimeout()
{
#ifdef XP_WIN
    if (LaunchHangUI()) {
        return true;
    }
    
    
    FinishHangUI();
#endif 
    TerminateChildProcess(MessageLoop::current());
    return false;
}

void
PluginModuleParent::TerminateChildProcess(MessageLoop* aMsgLoop)
{
#ifdef MOZ_CRASHREPORTER
    CrashReporterParent* crashReporter = CrashReporter();
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
            additionalDumps.Append(",flash1");
          }
          if (CreateFlashMinidump(mFlashProcess2, 0, pluginDumpFile,
                                  NS_LITERAL_CSTRING("flash2"))) {
            additionalDumps.Append(",flash2");
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
    if (isFromHangUI) {
        
        
        aMsgLoop->PostTask(FROM_HERE,
                           NewRunnableMethod(this,
                               &PluginModuleParent::CleanupFromTimeout,
                               isFromHangUI));
    } else {
        aMsgLoop->PostTask(
            FROM_HERE,
            mTaskFactory.NewRunnableMethod(
                &PluginModuleParent::CleanupFromTimeout, isFromHangUI));
    }

    if (!KillProcess(OtherProcess(), 1, false))
        NS_WARNING("failed to kill subprocess!");
}

#ifdef XP_WIN
void
PluginModuleParent::EvaluateHangUIState(const bool aReset)
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
PluginModuleParent::GetPluginName(nsAString& aPluginName)
{
    nsRefPtr<nsPluginHost> host = nsPluginHost::GetInst();
    if (!host) {
        return false;
    }
    nsPluginTag* pluginTag = host->TagForPlugin(mPlugin);
    if (!pluginTag) {
        return false;
    }
    CopyUTF8toUTF16(pluginTag->mName, aPluginName);
    return true;
}

bool
PluginModuleParent::LaunchHangUI()
{
    if (!mHangUIEnabled) {
        return false;
    }
    if (mHangUIParent) {
        if (mHangUIParent->IsShowing()) {
            
            return false;
        }
        if (mHangUIParent->DontShowAgain()) {
            return !mHangUIParent->WasLastHangStopped();
        }
        delete mHangUIParent;
        mHangUIParent = nullptr;
    }
    mHangUIParent = new PluginHangUIParent(this, 
            Preferences::GetInt(kHangUITimeoutPref, 0));
    nsAutoString pluginName;
    if (!GetPluginName(pluginName)) {
        return false;
    }
    bool retval = mHangUIParent->Init(pluginName);
    if (retval) {
        



        EvaluateHangUIState(false);
    }
    return retval;
}

void
PluginModuleParent::FinishHangUI()
{
    if (mHangUIEnabled && mHangUIParent) {
        bool needsCancel = mHangUIParent->IsShowing();
        
        if (needsCancel) {
            mHangUIParent->Cancel();
        }
        

        if (needsCancel ||
            !mIsTimerReset && mHangUIParent->WasShown()) {
            


            EvaluateHangUIState(true);
        }
    }
}
#endif 

#ifdef MOZ_CRASHREPORTER
CrashReporterParent*
PluginModuleParent::CrashReporter()
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
PluginModuleParent::ProcessFirstMinidump()
{
    CrashReporterParent* crashReporter = CrashReporter();
    if (!crashReporter)
        return;

    AnnotationTable notes;
    notes.Init(4);
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
#ifdef MOZ_CRASHREPORTER
        ProcessFirstMinidump();
#endif

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
        NS_ERROR("Unexpected shutdown reason for toplevel actor.");
    }
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

PPluginIdentifierParent*
PluginModuleParent::AllocPPluginIdentifier(const nsCString& aString,
                                           const int32_t& aInt,
                                           const bool& aTemporary)
{
    if (aTemporary) {
        NS_ERROR("Plugins don't create temporary identifiers.");
        return NULL; 
    }

    NPIdentifier npident = aString.IsVoid() ?
        mozilla::plugins::parent::_getintidentifier(aInt) :
        mozilla::plugins::parent::_getstringidentifier(aString.get());

    if (!npident) {
        NS_WARNING("Failed to get identifier!");
        return nullptr;
    }

    PluginIdentifierParent* ident = new PluginIdentifierParent(npident, false);
    mIdentifiers.Put(npident, ident);
    return ident;
}

bool
PluginModuleParent::DeallocPPluginIdentifier(PPluginIdentifierParent* aActor)
{
    delete aActor;
    return true;
}

PPluginInstanceParent*
PluginModuleParent::AllocPPluginInstance(const nsCString& aMimeType,
                                         const uint16_t& aMode,
                                         const InfallibleTArray<nsCString>& aNames,
                                         const InfallibleTArray<nsCString>& aValues,
                                         NPError* rv)
{
    NS_ERROR("Not reachable!");
    return NULL;
}

bool
PluginModuleParent::DeallocPPluginInstance(PPluginInstanceParent* aActor)
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

    
    aFuncs->newp = NULL;
    aFuncs->clearsitedata = NULL;
    aFuncs->getsiteswithdata = NULL;

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
    aFuncs->gotfocus = NULL;
    aFuncs->lostfocus = NULL;
    aFuncs->urlredirectnotify = NULL;

    
    
    bool urlRedirectSupported = false;
    unused << CallOptionalFunctionsSupported(&urlRedirectSupported,
                                             &mClearSiteDataSupported,
                                             &mGetSitesWithDataSupported);
    if (urlRedirectSupported) {
      aFuncs->urlredirectnotify = NPP_URLRedirectNotify;
    }
}

NPError
PluginModuleParent::NPP_Destroy(NPP instance,
                                NPSavedData** )
{
    
    
    
    
    
    PLUGIN_LOG_DEBUG_FUNCTION;

    PluginInstanceParent* parentInstance =
        static_cast<PluginInstanceParent*>(instance->pdata);

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
    PROFILER_LABEL("PluginModuleParent", "NPP_NewStream");
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_NewStream(type, stream, seekable,
                            stype);
}

NPError
PluginModuleParent::NPP_SetWindow(NPP instance, NPWindow* window)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_SetWindow(window);
}

NPError
PluginModuleParent::NPP_DestroyStream(NPP instance,
                                      NPStream* stream,
                                      NPReason reason)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_DestroyStream(stream, reason);
}

int32_t
PluginModuleParent::NPP_WriteReady(NPP instance,
                                   NPStream* stream)
{
    BrowserStreamParent* s = StreamCast(instance, stream);
    if (!s)
        return -1;

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
    PluginInstanceParent* i = InstCast(instance);
    if (i)
        i->NPP_Print(platformPrint);
}

int16_t
PluginModuleParent::NPP_HandleEvent(NPP instance, void* event)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return false;

    return i->NPP_HandleEvent(event);
}

void
PluginModuleParent::NPP_URLNotify(NPP instance, const char* url,
                                  NPReason reason, void* notifyData)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return;

    i->NPP_URLNotify(url, reason, notifyData);
}

NPError
PluginModuleParent::NPP_GetValue(NPP instance,
                                 NPPVariable variable, void *ret_value)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_GetValue(variable, ret_value);
}

NPError
PluginModuleParent::NPP_SetValue(NPP instance, NPNVariable variable,
                                 void *value)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_SetValue(variable, value);
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
  PluginInstanceParent* i = InstCast(instance);
  if (!i)
    return;

  i->NPP_URLRedirectNotify(url, status, notifyData);
}

bool
PluginModuleParent::AnswerNPN_UserAgent(nsCString* userAgent)
{
    *userAgent = NullableString(mNPNIface->uagent(nullptr));
    return true;
}

PluginIdentifierParent*
PluginModuleParent::GetIdentifierForNPIdentifier(NPP npp, NPIdentifier aIdentifier)
{
    PluginIdentifierParent* ident;
    if (mIdentifiers.Get(aIdentifier, &ident)) {
        if (ident->IsTemporary()) {
            ident->AddTemporaryRef();
        }
        return ident;
    }

    nsCString string;
    int32_t intval = -1;
    bool temporary = false;
    if (mozilla::plugins::parent::_identifierisstring(aIdentifier)) {
        NPUTF8* chars =
            mozilla::plugins::parent::_utf8fromidentifier(aIdentifier);
        if (!chars) {
            return nullptr;
        }
        string.Adopt(chars);
        temporary = !NPStringIdentifierIsPermanent(npp, aIdentifier);
    }
    else {
        intval = mozilla::plugins::parent::_intfromidentifier(aIdentifier);
        string.SetIsVoid(true);
    }

    ident = new PluginIdentifierParent(aIdentifier, temporary);
    if (!SendPPluginIdentifierConstructor(ident, string, intval, temporary))
        return nullptr;

    if (!temporary) {
        mIdentifiers.Put(aIdentifier, ident);
    }
    return ident;
}

PluginInstanceParent*
PluginModuleParent::InstCast(NPP instance)
{
    PluginInstanceParent* ip =
        static_cast<PluginInstanceParent*>(instance->pdata);

    
    
    if (!ip)
        return NULL;

    if (instance != ip->mNPP) {
        NS_RUNTIMEABORT("Corrupted plugin data.");
    }
    return ip;
}

BrowserStreamParent*
PluginModuleParent::StreamCast(NPP instance,
                               NPStream* s)
{
    PluginInstanceParent* ip = InstCast(instance);
    if (!ip)
        return NULL;

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
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->AsyncSetWindow(window);
}

#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
nsresult
PluginModuleParent::HandleGUIEvent(NPP instance,
                                   const nsGUIEvent& anEvent,
                                   bool* handled)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->HandleGUIEvent(anEvent, handled);
}
#endif

nsresult
PluginModuleParent::GetImageContainer(NPP instance,
                             mozilla::layers::ImageContainer** aContainer)
{
    PluginInstanceParent* i = InstCast(instance);
    return !i ? NS_ERROR_FAILURE : i->GetImageContainer(aContainer);
}

nsresult
PluginModuleParent::GetImageSize(NPP instance,
                                 nsIntSize* aSize)
{
    PluginInstanceParent* i = InstCast(instance);
    return !i ? NS_ERROR_FAILURE : i->GetImageSize(aSize);
}

nsresult
PluginModuleParent::SetBackgroundUnknown(NPP instance)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->SetBackgroundUnknown();
}

nsresult
PluginModuleParent::BeginUpdateBackground(NPP instance,
                                          const nsIntRect& aRect,
                                          gfxContext** aCtx)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->BeginUpdateBackground(aRect, aCtx);
}

nsresult
PluginModuleParent::EndUpdateBackground(NPP instance,
                                        gfxContext* aCtx,
                                        const nsIntRect& aRect)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->EndUpdateBackground(aCtx, aRect);
}

#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(MOZ_WIDGET_GONK)
nsresult
PluginModuleParent::NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    mNPNIface = bFuncs;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    uint32_t flags = 0;

    if (!CallNP_Initialize(flags, error)) {
        return NS_ERROR_FAILURE;
    }
    else if (*error != NPERR_NO_ERROR) {
        return NS_OK;
    }

    SetPluginFuncs(pFuncs);

    return NS_OK;
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

    uint32_t flags = 0;
#ifdef XP_WIN
    flags |= kAllowAsyncDrawing;
#endif

    if (!CallNP_Initialize(flags, error))
        return NS_ERROR_FAILURE;

#if defined XP_WIN
    
    
    nsID id;
    nsString sessionName;
    nsString iconPath;

    if (NS_SUCCEEDED(mozilla::widget::GetAudioSessionData(id, sessionName,
                                                          iconPath)))
        unused << SendSetAudioSessionData(id, sessionName, iconPath);
#endif

#ifdef MOZ_CRASHREPORTER_INJECTOR
    InitializeInjector();
#endif

    return NS_OK;
}
#endif

nsresult
PluginModuleParent::NP_Shutdown(NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    bool ok = CallNP_Shutdown(error);

    
    
    
    
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

#if defined(XP_WIN) || defined(XP_MACOSX) || defined(XP_OS2)
nsresult
PluginModuleParent::NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error)
{
    NS_ASSERTION(pFuncs, "Null pointer!");

    
    
    
    if (!CallNP_GetEntryPoints(error)) {
        return NS_ERROR_FAILURE;
    }
    else if (*error != NPERR_NO_ERROR) {
        return NS_OK;
    }

    SetPluginFuncs(pFuncs);

    return NS_OK;
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

    
    InfallibleTArray<nsCString> names;
    InfallibleTArray<nsCString> values;

    for (int i = 0; i < argc; ++i) {
        names.AppendElement(NullableString(argn[i]));
        values.AppendElement(NullableString(argv[i]));
    }

    PluginInstanceParent* parentInstance =
        new PluginInstanceParent(this, instance,
                                 nsDependentCString(pluginType), mNPNIface);

    if (!parentInstance->Init()) {
        delete parentInstance;
        return NS_ERROR_FAILURE;
    }

    instance->pdata = parentInstance;

    if (!CallPPluginInstanceConstructor(parentInstance,
                                        nsDependentCString(pluginType), mode,
                                        names, values, error)) {
        
        instance->pdata = nullptr;
        
        
        
        if (NPERR_NO_ERROR == *error)
            *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    if (*error != NPERR_NO_ERROR) {
        NPP_Destroy(instance, 0);
        return NS_ERROR_FAILURE;
    }

    TimeoutChanged(kParentTimeoutPref, this);
    
    return NS_OK;
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
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->IsRemoteDrawingCoreAnimation(aDrawing);
}

nsresult
PluginModuleParent::ContentsScaleFactorChanged(NPP instance, double aContentsScaleFactor)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NS_ERROR_FAILURE;

    return i->ContentsScaleFactorChanged(aContentsScaleFactor);
}
#endif 

bool
PluginModuleParent::AnswerNPN_GetValue_WithBoolReturn(const NPNVariable& aVariable,
                                                      NPError* aError,
                                                      bool* aBoolVal)
{
    NPBool boolVal = false;
    *aError = mozilla::plugins::parent::_getvalue(nullptr, aVariable, &boolVal);
    *aBoolVal = boolVal ? true : false;
    return true;
}

#if defined(MOZ_WIDGET_QT)
static const int kMaxtimeToProcessEvents = 30;
bool
PluginModuleParent::AnswerProcessSomeEvents()
{
    PLUGIN_LOG_DEBUG(("Spinning mini nested loop ..."));
    QCoreApplication::processEvents(QEventLoop::AllEvents, kMaxtimeToProcessEvents);

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
        if (!g_main_context_iteration(NULL, FALSE))
            break;

    PLUGIN_LOG_DEBUG(("... quitting mini nested loop; processed %i tasks", i));

    return true;
}
#endif

bool
PluginModuleParent::RecvProcessNativeEventsInRPCCall()
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));
#if defined(OS_WIN)
    ProcessNativeEventsInRPCCall();
    return true;
#else
    NS_NOTREACHED(
        "PluginModuleParent::RecvProcessNativeEventsInRPCCall not implemented!");
    return false;
#endif
}

void
PluginModuleParent::ProcessRemoteNativeEventsInRPCCall()
{
#if defined(OS_WIN)
    unused << SendProcessNativeEventsInRPCCall();
    return;
#endif
    NS_NOTREACHED(
        "PluginModuleParent::ProcessRemoteNativeEventsInRPCCall not implemented!");
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
PluginModuleParent::AllocPCrashReporter(mozilla::dom::NativeThreadId* id,
                                        uint32_t* processType)
{
#ifdef MOZ_CRASHREPORTER
    return new CrashReporterParent();
#else
    return nullptr;
#endif
}

bool
PluginModuleParent::DeallocPCrashReporter(PCrashReporterParent* actor)
{
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
PluginModuleParent::RecvGetNativeCursorsSupported(bool* supported)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));
#if defined(XP_MACOSX)
    *supported =
      Preferences::GetBool("dom.ipc.plugins.nativeCursorSupport", false);
    return true;
#else
    NS_NOTREACHED(
        "PluginInstanceParent::RecvGetNativeCursorSupportLevel not implemented!");
    return false;
#endif
}

bool
PluginModuleParent::RecvNPN_SetException(PPluginScriptableObjectParent* aActor,
                                         const nsCString& aMessage)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));

    NPObject* aNPObj = NULL;
    if (aActor) {
        aNPObj = static_cast<PluginScriptableObjectParent*>(aActor)->GetObject(true);
        if (!aNPObj) {
            NS_ERROR("Failed to get object!");
            return false;
        }
    }
    mozilla::plugins::parent::_setexception(aNPObj, NullableStringGet(aMessage));
    return true;
}

bool
PluginModuleParent::RecvNPN_ReloadPlugins(const bool& aReloadPages)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));

    mozilla::plugins::parent::_reloadplugins(aReloadPages);
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
PluginModuleParent::InitializeInjector()
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

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == snapshot)
        return;

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
PluginModuleParent::OnCrash(DWORD processID)
{
    if (!mShutdown) {
        GetIPCChannel()->CloseWithError();
        KillProcess(OtherProcess(), 1, false);
    }
}

#endif 
