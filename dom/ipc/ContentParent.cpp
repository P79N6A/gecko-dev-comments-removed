





#include "mozilla/DebugOnly.h"

#include "base/basictypes.h"

#include "ContentParent.h"

#if defined(ANDROID) || defined(LINUX)
# include <sys/time.h>
# include <sys/resource.h>
#endif

#ifdef MOZ_WIDGET_GONK
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include "chrome/common/process_watcher.h"

#include <set>

#include "AppProcessChecker.h"
#include "AudioChannelService.h"
#include "BlobParent.h"
#include "CrashReporterParent.h"
#include "IHistory.h"
#include "mozIApplication.h"
#ifdef ACCESSIBILITY
#include "mozilla/a11y/DocAccessibleParent.h"
#include "nsAccessibilityService.h"
#endif
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/docshell/OfflineCacheUpdateParent.h"
#include "mozilla/dom/DataStoreService.h"
#include "mozilla/dom/DOMStorageIPC.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/ExternalHelperAppParent.h"
#include "mozilla/dom/FileSystemRequestParent.h"
#include "mozilla/dom/GeolocationBinding.h"
#include "mozilla/dom/PContentBridgeParent.h"
#include "mozilla/dom/PCycleCollectWithLogsParent.h"
#include "mozilla/dom/PFMRadioParent.h"
#include "mozilla/dom/PMemoryReportRequestParent.h"
#include "mozilla/dom/asmjscache/AsmJSCache.h"
#include "mozilla/dom/bluetooth/PBluetoothParent.h"
#include "mozilla/dom/cellbroadcast/CellBroadcastParent.h"
#include "mozilla/dom/devicestorage/DeviceStorageRequestParent.h"
#include "mozilla/dom/mobileconnection/MobileConnectionParent.h"
#include "mozilla/dom/mobilemessage/SmsParent.h"
#include "mozilla/dom/power/PowerManagerService.h"
#include "mozilla/dom/quota/QuotaManager.h"
#include "mozilla/dom/telephony/TelephonyParent.h"
#include "mozilla/dom/time/DateCacheCleaner.h"
#include "mozilla/dom/voicemail/VoicemailParent.h"
#include "mozilla/embedding/printingui/PrintingParent.h"
#include "mozilla/hal_sandbox/PHalParent.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "mozilla/ipc/FileDescriptorSetParent.h"
#include "mozilla/ipc/FileDescriptorUtils.h"
#include "mozilla/ipc/PFileDescriptorSetParent.h"
#include "mozilla/ipc/TestShellParent.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/ImageBridgeParent.h"
#include "mozilla/layers/SharedBufferManagerParent.h"
#include "mozilla/net/NeckoParent.h"
#include "mozilla/plugins/PluginBridge.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Telemetry.h"
#include "mozilla/unused.h"
#include "nsAnonymousTemporaryFile.h"
#include "nsAppRunner.h"
#include "nsAutoPtr.h"
#include "nsCDefaultURIFixup.h"
#include "nsCExternalHandlerService.h"
#include "nsCOMPtr.h"
#include "nsChromeRegistryChrome.h"
#include "nsConsoleMessage.h"
#include "nsConsoleService.h"
#include "nsContentUtils.h"
#include "nsDebugImpl.h"
#include "nsFrameMessageManager.h"
#include "nsGeolocationSettings.h"
#include "nsHashPropertyBag.h"
#include "nsIAlertsService.h"
#include "nsIAppsService.h"
#include "nsIClipboard.h"
#include "nsICycleCollectorListener.h"
#include "nsIDocument.h"
#include "nsIDOMGeoGeolocation.h"
#include "nsIDOMGeoPositionError.h"
#include "mozilla/dom/WakeLock.h"
#include "nsIDOMWindow.h"
#include "nsIExternalProtocolService.h"
#include "nsIFormProcessor.h"
#include "nsIGfxInfo.h"
#include "nsIIdleService.h"
#include "nsIJSRuntimeService.h"
#include "nsIMemoryInfoDumper.h"
#include "nsIMemoryReporter.h"
#include "nsIMozBrowserFrame.h"
#include "nsIMutable.h"
#include "nsIObserverService.h"
#include "nsIPresShell.h"
#include "nsIScriptError.h"
#include "nsISiteSecurityService.h"
#include "nsISpellChecker.h"
#include "nsIStyleSheet.h"
#include "nsISupportsPrimitives.h"
#include "nsITimer.h"
#include "nsIURIFixup.h"
#include "nsIWindowWatcher.h"
#include "nsIXULRuntime.h"
#include "nsMemoryInfoDumper.h"
#include "nsMemoryReporterManager.h"
#include "nsServiceManagerUtils.h"
#include "nsStyleSheetService.h"
#include "nsThreadUtils.h"
#include "nsThreadManager.h"
#include "nsToolkitCompsCID.h"
#include "nsWidgetsCID.h"
#include "PreallocatedProcessManager.h"
#include "ProcessPriorityManager.h"
#include "SandboxHal.h"
#include "ScreenManagerParent.h"
#include "StructuredCloneUtils.h"
#include "TabParent.h"
#include "URIUtils.h"
#include "nsIWebBrowserChrome.h"
#include "nsIDocShell.h"
#include "mozilla/net/NeckoMessageUtils.h"
#include "gfxPrefs.h"
#include "prio.h"
#include "private/pprio.h"
#include "ContentProcessManager.h"

#if defined(ANDROID) || defined(LINUX)
#include "nsSystemInfo.h"
#endif

#if defined(XP_LINUX)
#include "mozilla/Hal.h"
#endif

#ifdef ANDROID
# include "gfxAndroidPlatform.h"
#endif

#ifdef MOZ_PERMISSIONS
# include "nsPermissionManager.h"
#endif

#ifdef MOZ_WIDGET_ANDROID
# include "AndroidBridge.h"
#endif

#ifdef MOZ_WIDGET_GONK
#include "nsIVolume.h"
#include "nsVolumeService.h"
#include "nsIVolumeService.h"
#include "SpeakerManagerService.h"
using namespace mozilla::system;
#endif

#ifdef MOZ_B2G_BT
#include "BluetoothParent.h"
#include "BluetoothService.h"
#endif

#include "JavaScriptParent.h"

#include "mozilla/RemoteSpellCheckEngineParent.h"

#ifdef MOZ_B2G_FM
#include "mozilla/dom/FMRadioParent.h"
#endif

#include "Crypto.h"

#ifdef MOZ_WEBSPEECH
#include "mozilla/dom/SpeechSynthesisParent.h"
#endif

#ifdef ENABLE_TESTS
#include "BackgroundChildImpl.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#endif


#if defined(MOZ_CONTENT_SANDBOX) && defined(XP_LINUX)
#include "mozilla/SandboxInfo.h"
#endif

#ifdef MOZ_TOOLKIT_SEARCH
#include "nsIBrowserSearchService.h"
#endif

#ifdef MOZ_ENABLE_PROFILER_SPS
#include "nsIProfiler.h"
#include "nsIProfileSaveEvent.h"
#endif

static NS_DEFINE_CID(kCClipboardCID, NS_CLIPBOARD_CID);
static const char* sClipboardTextFlavors[] = { kUnicodeMime };

using base::ChildPrivileges;
using base::KillProcess;

#ifdef MOZ_CRASHREPORTER
using namespace CrashReporter;
#endif
using namespace mozilla::dom::bluetooth;
using namespace mozilla::dom::cellbroadcast;
using namespace mozilla::dom::devicestorage;
using namespace mozilla::dom::indexedDB;
using namespace mozilla::dom::power;
using namespace mozilla::dom::mobileconnection;
using namespace mozilla::dom::mobilemessage;
using namespace mozilla::dom::telephony;
using namespace mozilla::dom::voicemail;
using namespace mozilla::embedding;
using namespace mozilla::hal;
using namespace mozilla::ipc;
using namespace mozilla::layers;
using namespace mozilla::net;
using namespace mozilla::jsipc;
using namespace mozilla::widget;

#ifdef ENABLE_TESTS

class BackgroundTester MOZ_FINAL : public nsIIPCBackgroundChildCreateCallback,
                                   public nsIObserver
{
    static uint32_t sCallbackCount;

private:
    ~BackgroundTester()
    { }

    virtual void
    ActorCreated(PBackgroundChild* aActor) MOZ_OVERRIDE
    {
        MOZ_RELEASE_ASSERT(aActor,
                           "Failed to create a PBackgroundChild actor!");

        NS_NAMED_LITERAL_CSTRING(testStr, "0123456789");

        PBackgroundTestChild* testActor =
            aActor->SendPBackgroundTestConstructor(testStr);
        MOZ_RELEASE_ASSERT(testActor);

        if (!sCallbackCount) {
            PBackgroundChild* existingBackgroundChild =
                BackgroundChild::GetForCurrentThread();

            MOZ_RELEASE_ASSERT(existingBackgroundChild);
            MOZ_RELEASE_ASSERT(existingBackgroundChild == aActor);

            bool ok =
                existingBackgroundChild->
                    SendPBackgroundTestConstructor(testStr);
            MOZ_RELEASE_ASSERT(ok);

            
            ok = BackgroundChild::GetOrCreateForCurrentThread(this);
            MOZ_RELEASE_ASSERT(ok);
        }

        sCallbackCount++;
    }

    virtual void
    ActorFailed() MOZ_OVERRIDE
    {
        MOZ_CRASH("Failed to create a PBackgroundChild actor!");
    }

    NS_IMETHOD
    Observe(nsISupports* aSubject, const char* aTopic, const char16_t* aData)
            MOZ_OVERRIDE
    {
        nsCOMPtr<nsIObserverService> observerService =
            mozilla::services::GetObserverService();
        MOZ_RELEASE_ASSERT(observerService);

        nsresult rv = observerService->RemoveObserver(this, aTopic);
        MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));

        if (!strcmp(aTopic, "profile-after-change")) {
            if (mozilla::Preferences::GetBool("pbackground.testing", false)) {
                rv = observerService->AddObserver(this, "xpcom-shutdown",
                                                  false);
                MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));

                
                bool ok = BackgroundChild::GetOrCreateForCurrentThread(this);
                MOZ_RELEASE_ASSERT(ok);

                BackgroundChildImpl::ThreadLocal* threadLocal =
                  BackgroundChildImpl::GetThreadLocalForCurrentThread();
                MOZ_RELEASE_ASSERT(threadLocal);

                
                ok = BackgroundChild::GetOrCreateForCurrentThread(this);
                MOZ_RELEASE_ASSERT(ok);
            }

            return NS_OK;
        }

        if (!strcmp(aTopic, "xpcom-shutdown")) {
            MOZ_RELEASE_ASSERT(sCallbackCount == 3);

            return NS_OK;
        }

        MOZ_CRASH("Unknown observer topic!");
    }

public:
    NS_DECL_ISUPPORTS
};

uint32_t BackgroundTester::sCallbackCount = 0;

NS_IMPL_ISUPPORTS(BackgroundTester, nsIIPCBackgroundChildCreateCallback,
                  nsIObserver)

#endif 

void
MaybeTestPBackground()
{
#ifdef ENABLE_TESTS
    
    
    if (PR_GetEnv("XPCSHELL_TEST_PROFILE_DIR")) {
        return;
    }

    
    
    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
    MOZ_RELEASE_ASSERT(observerService);

    nsCOMPtr<nsIObserver> observer = new BackgroundTester();
    nsresult rv = observerService->AddObserver(observer, "profile-after-change",
                                               false);
    MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));
#endif
}


template<>
struct nsIConsoleService::COMTypeInfo<nsConsoleService, void> {
  static const nsIID kIID;
};
const nsIID nsIConsoleService::COMTypeInfo<nsConsoleService, void>::kIID = NS_ICONSOLESERVICE_IID;

namespace mozilla {
namespace dom {

#ifdef MOZ_NUWA_PROCESS
int32_t ContentParent::sNuwaPid = 0;
bool ContentParent::sNuwaReady = false;
#endif

#define NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC "ipc:network:set-offline"

class MemoryReportRequestParent : public PMemoryReportRequestParent
{
public:
    MemoryReportRequestParent();
    virtual ~MemoryReportRequestParent();

    virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

    virtual bool Recv__delete__(const uint32_t& aGeneration, const InfallibleTArray<MemoryReport>& aReport) MOZ_OVERRIDE;
private:
    ContentParent* Owner()
    {
        return static_cast<ContentParent*>(Manager());
    }
};

MemoryReportRequestParent::MemoryReportRequestParent()
{
    MOZ_COUNT_CTOR(MemoryReportRequestParent);
}

void
MemoryReportRequestParent::ActorDestroy(ActorDestroyReason aWhy)
{
  
}

bool
MemoryReportRequestParent::Recv__delete__(const uint32_t& generation, const InfallibleTArray<MemoryReport>& childReports)
{
    nsRefPtr<nsMemoryReporterManager> mgr =
        nsMemoryReporterManager::GetOrCreate();
    if (mgr) {
        mgr->HandleChildReports(generation, childReports);
    }
    return true;
}

MemoryReportRequestParent::~MemoryReportRequestParent()
{
    MOZ_COUNT_DTOR(MemoryReportRequestParent);
}


class CycleCollectWithLogsParent MOZ_FINAL : public PCycleCollectWithLogsParent
{
public:
    ~CycleCollectWithLogsParent()
    {
        MOZ_COUNT_DTOR(CycleCollectWithLogsParent);
    }

    static bool AllocAndSendConstructor(ContentParent* aManager,
                                        bool aDumpAllTraces,
                                        nsICycleCollectorLogSink* aSink,
                                        nsIDumpGCAndCCLogsCallback* aCallback)
    {
        CycleCollectWithLogsParent *actor;
        FILE* gcLog;
        FILE* ccLog;
        nsresult rv;

        actor = new CycleCollectWithLogsParent(aSink, aCallback);
        rv = actor->mSink->Open(&gcLog, &ccLog);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            delete actor;
            return false;
        }

        return aManager->
            SendPCycleCollectWithLogsConstructor(actor,
                                                 aDumpAllTraces,
                                                 FILEToFileDescriptor(gcLog),
                                                 FILEToFileDescriptor(ccLog));
    }

private:
    virtual bool RecvCloseGCLog() MOZ_OVERRIDE
    {
        unused << mSink->CloseGCLog();
        return true;
    }

    virtual bool RecvCloseCCLog() MOZ_OVERRIDE
    {
        unused << mSink->CloseCCLog();
        return true;
    }

    virtual bool Recv__delete__() MOZ_OVERRIDE
    {
        
        
        nsCOMPtr<nsIFile> gcLog, ccLog;
        mSink->GetGcLog(getter_AddRefs(gcLog));
        mSink->GetCcLog(getter_AddRefs(ccLog));
        unused << mCallback->OnDump(gcLog, ccLog,  false);
        return true;
    }

    virtual void ActorDestroy(ActorDestroyReason aReason) MOZ_OVERRIDE
    {
        
        
        
        
    }

    CycleCollectWithLogsParent(nsICycleCollectorLogSink *aSink,
                               nsIDumpGCAndCCLogsCallback *aCallback)
        : mSink(aSink), mCallback(aCallback)
    {
        MOZ_COUNT_CTOR(CycleCollectWithLogsParent);
    }

    nsCOMPtr<nsICycleCollectorLogSink> mSink;
    nsCOMPtr<nsIDumpGCAndCCLogsCallback> mCallback;
};


class ContentParentsMemoryReporter MOZ_FINAL : public nsIMemoryReporter
{
    ~ContentParentsMemoryReporter() {}
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORYREPORTER
};

NS_IMPL_ISUPPORTS(ContentParentsMemoryReporter, nsIMemoryReporter)

NS_IMETHODIMP
ContentParentsMemoryReporter::CollectReports(nsIMemoryReporterCallback* cb,
                                             nsISupports* aClosure,
                                             bool aAnonymize)
{
    nsAutoTArray<ContentParent*, 16> cps;
    ContentParent::GetAllEvenIfDead(cps);

    for (uint32_t i = 0; i < cps.Length(); i++) {
        ContentParent* cp = cps[i];
        MessageChannel* channel = cp->GetIPCChannel();

        nsString friendlyName;
        cp->FriendlyName(friendlyName, aAnonymize);

        cp->AddRef();
        nsrefcnt refcnt = cp->Release();

        const char* channelStr = "no channel";
        uint32_t numQueuedMessages = 0;
        if (channel) {
            if (channel->Unsound_IsClosed()) {
                channelStr = "closed channel";
            } else {
                channelStr = "open channel";
            }
            numQueuedMessages = channel->Unsound_NumQueuedMessages();
        }

        nsPrintfCString path("queued-ipc-messages/content-parent"
                             "(%s, pid=%d, %s, 0x%p, refcnt=%d)",
                             NS_ConvertUTF16toUTF8(friendlyName).get(),
                             cp->Pid(), channelStr,
                             static_cast<nsIContentParent*>(cp), refcnt);

        NS_NAMED_LITERAL_CSTRING(desc,
            "The number of unset IPC messages held in this ContentParent's "
            "channel.  A large value here might indicate that we're leaking "
            "messages.  Similarly, a ContentParent object for a process that's no "
            "longer running could indicate that we're leaking ContentParents.");

        nsresult rv = cb->Callback( EmptyCString(),
                                   path,
                                   KIND_OTHER,
                                   UNITS_COUNT,
                                   numQueuedMessages,
                                   desc,
                                   aClosure);

        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

nsDataHashtable<nsStringHashKey, ContentParent*>* ContentParent::sAppContentParents;
nsTArray<ContentParent*>* ContentParent::sNonAppContentParents;
nsTArray<ContentParent*>* ContentParent::sPrivateContent;
StaticAutoPtr<LinkedList<ContentParent> > ContentParent::sContentParents;

#ifdef MOZ_NUWA_PROCESS

static nsTArray<PrefSetting>* sNuwaPrefUpdates;
#endif



static bool sCanLaunchSubprocesses;


static uint64_t gContentChildID = 1;




#define MAGIC_PREALLOCATED_APP_MANIFEST_URL NS_LITERAL_STRING("{{template}}")

static const char* sObserverTopics[] = {
    "xpcom-shutdown",
    "profile-before-change",
    NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC,
    "child-memory-reporter-request",
    "memory-pressure",
    "child-gc-request",
    "child-cc-request",
    "child-mmu-request",
    "last-pb-context-exited",
    "file-watcher-update",
#ifdef MOZ_WIDGET_GONK
    NS_VOLUME_STATE_CHANGED,
    "phone-state-changed",
#endif
#ifdef ACCESSIBILITY
    "a11y-init-or-shutdown",
#endif
    "app-theme-changed",
#ifdef MOZ_ENABLE_PROFILER_SPS
    "profiler-started",
    "profiler-stopped",
    "profiler-subprocess",
#endif
};

 already_AddRefed<ContentParent>
ContentParent::RunNuwaProcess()
{
    MOZ_ASSERT(NS_IsMainThread());
    nsRefPtr<ContentParent> nuwaProcess =
        new ContentParent( nullptr,
                           nullptr,
                           false,
                           true,
                          PROCESS_PRIORITY_BACKGROUND,
                           true);
    nuwaProcess->Init();
#ifdef MOZ_NUWA_PROCESS
    sNuwaPid = nuwaProcess->Pid();
    sNuwaReady = false;
#endif
    return nuwaProcess.forget();
}




 already_AddRefed<ContentParent>
ContentParent::PreallocateAppProcess()
{
    nsRefPtr<ContentParent> process =
        new ContentParent( nullptr,
                           nullptr,
                           false,
                           true,
                          PROCESS_PRIORITY_PREALLOC);
    process->Init();
    return process.forget();
}

 already_AddRefed<ContentParent>
ContentParent::GetNewOrPreallocatedAppProcess(mozIApplication* aApp,
                                              ProcessPriority aInitialPriority,
                                              ContentParent* aOpener,
                                               bool* aTookPreAllocated)
{
    MOZ_ASSERT(aApp);
    nsRefPtr<ContentParent> process = PreallocatedProcessManager::Take();

    if (process) {
        if (!process->SetPriorityAndCheckIsAlive(aInitialPriority)) {
            
            
            process->KillHard();
        }
        else {
            nsAutoString manifestURL;
            if (NS_FAILED(aApp->GetManifestURL(manifestURL))) {
                NS_ERROR("Failed to get manifest URL");
                return nullptr;
            }
            process->TransformPreallocatedIntoApp(aOpener,
                                                  manifestURL);
            if (aTookPreAllocated) {
                *aTookPreAllocated = true;
            }
            return process.forget();
        }
    }

    
    
    NS_WARNING("Unable to use pre-allocated app process");
    process = new ContentParent(aApp,
                                 aOpener,
                                 false,
                                 false,
                                aInitialPriority);
    process->Init();

    if (aTookPreAllocated) {
        *aTookPreAllocated = false;
    }

    return process.forget();
}

 void
ContentParent::StartUp()
{
    
    
    
    sCanLaunchSubprocesses = true;

    if (XRE_GetProcessType() != GeckoProcessType_Default) {
        return;
    }

#if defined(MOZ_CONTENT_SANDBOX) && defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 19
    
    
    if (!SandboxInfo::Get().CanSandboxContent()) {
        
        
        printf_stderr("Sandboxing support is required on this platform.  "
                      "Recompile kernel with CONFIG_SECCOMP_FILTER=y\n");
        MOZ_CRASH("Sandboxing support is required on this platform.");
    }
#endif

    
    RegisterStrongMemoryReporter(new ContentParentsMemoryReporter());

    mozilla::dom::time::InitializeDateCacheCleaner();

    BlobParent::Startup(BlobParent::FriendKey());

    BackgroundChild::Startup();

    
    PreallocatedProcessManager::AllocateAfterDelay();

    
    
    MaybeTestPBackground();
}

 void
ContentParent::ShutDown()
{
    
    
    sCanLaunchSubprocesses = false;
}

 void
ContentParent::JoinProcessesIOThread(const nsTArray<ContentParent*>* aProcesses,
                                     Monitor* aMonitor, bool* aDone)
{
    const nsTArray<ContentParent*>& processes = *aProcesses;
    for (uint32_t i = 0; i < processes.Length(); ++i) {
        if (GeckoChildProcessHost* process = processes[i]->mSubprocess) {
            process->Join();
        }
    }
    {
        MonitorAutoLock lock(*aMonitor);
        *aDone = true;
        lock.Notify();
    }
    
}

 void
ContentParent::JoinAllSubprocesses()
{
    MOZ_ASSERT(NS_IsMainThread());

    nsAutoTArray<ContentParent*, 8> processes;
    GetAll(processes);
    if (processes.IsEmpty()) {
        printf_stderr("There are no live subprocesses.");
        return;
    }

    printf_stderr("Subprocesses are still alive.  Doing emergency join.\n");

    bool done = false;
    Monitor monitor("mozilla.dom.ContentParent.JoinAllSubprocesses");
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     NewRunnableFunction(
                                         &ContentParent::JoinProcessesIOThread,
                                         &processes, &monitor, &done));
    {
        MonitorAutoLock lock(monitor);
        while (!done) {
            lock.Wait();
        }
    }

    sCanLaunchSubprocesses = false;
}

 already_AddRefed<ContentParent>
ContentParent::GetNewOrUsedBrowserProcess(bool aForBrowserElement,
                                          ProcessPriority aPriority,
                                          ContentParent* aOpener)
{
    if (!sNonAppContentParents)
        sNonAppContentParents = new nsTArray<ContentParent*>();

    int32_t maxContentProcesses = Preferences::GetInt("dom.ipc.processCount", 1);
    if (maxContentProcesses < 1)
        maxContentProcesses = 1;

    if (sNonAppContentParents->Length() >= uint32_t(maxContentProcesses)) {
      uint32_t startIdx = rand() % sNonAppContentParents->Length();
      uint32_t currIdx = startIdx;
      do {
        nsRefPtr<ContentParent> p = (*sNonAppContentParents)[currIdx];
        NS_ASSERTION(p->IsAlive(), "Non-alive contentparent in sNonAppContntParents?");
        if (p->mOpener == aOpener) {
          return p.forget();
        }
        currIdx = (currIdx + 1) % sNonAppContentParents->Length();
      } while (currIdx != startIdx);
    }

    
    nsRefPtr<ContentParent> p = PreallocatedProcessManager::Take();
    if (p) {
        p->TransformPreallocatedIntoBrowser(aOpener);
    } else {
      
        p = new ContentParent( nullptr,
                              aOpener,
                              aForBrowserElement,
                               false,
                              aPriority);
        p->Init();
    }

    sNonAppContentParents->AppendElement(p);
    return p.forget();
}

 ProcessPriority
ContentParent::GetInitialProcessPriority(Element* aFrameElement)
{
    
    

    if (!aFrameElement) {
        return PROCESS_PRIORITY_FOREGROUND;
    }

    if (aFrameElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::mozapptype,
                                   NS_LITERAL_STRING("inputmethod"), eCaseMatters)) {
        return PROCESS_PRIORITY_FOREGROUND_KEYBOARD;
    } else if (!aFrameElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::mozapptype,
                                           NS_LITERAL_STRING("critical"), eCaseMatters)) {
        return PROCESS_PRIORITY_FOREGROUND;
    }

    nsCOMPtr<nsIMozBrowserFrame> browserFrame =
        do_QueryInterface(aFrameElement);
    if (!browserFrame) {
        return PROCESS_PRIORITY_FOREGROUND;
    }

    return browserFrame->GetIsExpectingSystemMessage() ?
               PROCESS_PRIORITY_FOREGROUND_HIGH :
               PROCESS_PRIORITY_FOREGROUND;
}

bool
ContentParent::PreallocatedProcessReady()
{
#ifdef MOZ_NUWA_PROCESS
    return PreallocatedProcessManager::PreallocatedProcessReady();
#else
    return true;
#endif
}

bool
ContentParent::RecvCreateChildProcess(const IPCTabContext& aContext,
                                      const hal::ProcessPriority& aPriority,
                                      const TabId& aOpenerTabId,
                                      ContentParentId* aCpId,
                                      bool* aIsForApp,
                                      bool* aIsForBrowser,
                                      TabId* aTabId)
{
#if 0
    if (!CanOpenBrowser(aContext)) {
        return false;
    }
#endif
    nsRefPtr<ContentParent> cp;
    MaybeInvalidTabContext tc(aContext);
    if (!tc.IsValid()) {
        NS_ERROR(nsPrintfCString("Received an invalid TabContext from "
                                 "the child process. (%s)",
                                 tc.GetInvalidReason()).get());
        return false;
    }

    nsCOMPtr<mozIApplication> ownApp = tc.GetTabContext().GetOwnApp();
    if (ownApp) {
        cp = GetNewOrPreallocatedAppProcess(ownApp,
                                            aPriority,
                                            this);
    }
    else {
        cp = GetNewOrUsedBrowserProcess( true,
                                        aPriority,
                                        this);
    }

    if (!cp) {
        *aCpId = 0;
        *aIsForApp = false;
        *aIsForBrowser = false;
        return true;
    }

    *aCpId = cp->ChildID();
    *aIsForApp = cp->IsForApp();
    *aIsForBrowser = cp->IsForBrowser();

    ContentProcessManager *cpm = ContentProcessManager::GetSingleton();
    cpm->AddContentProcess(cp, this->ChildID());

    if (cpm->AddGrandchildProcess(this->ChildID(), cp->ChildID())) {
        
        *aTabId = AllocateTabId(aOpenerTabId,
                                aContext,
                                cp->ChildID());
        return (*aTabId != 0);
    }

    return false;
}

bool
ContentParent::RecvBridgeToChildProcess(const ContentParentId& aCpId)
{
    ContentProcessManager *cpm = ContentProcessManager::GetSingleton();
    ContentParent* cp = cpm->GetContentProcessById(aCpId);

    if (cp) {
        ContentParentId parentId;
        if (cpm->GetParentProcessId(cp->ChildID(), &parentId) &&
            parentId == this->ChildID()) {
            return PContentBridge::Bridge(this, cp);
        }
    }

    
    KillHard();
    return false;
}

static nsIDocShell* GetOpenerDocShellHelper(Element* aFrameElement)
{
    
    
    nsCOMPtr<Element> frameElement = do_QueryInterface(aFrameElement);
    MOZ_ASSERT(frameElement);
    nsPIDOMWindow* win = frameElement->OwnerDoc()->GetWindow();
    if (!win) {
        NS_WARNING("Remote frame has no window");
        return nullptr;
    }
    nsIDocShell* docShell = win->GetDocShell();
    if (!docShell) {
        NS_WARNING("Remote frame has no docshell");
        return nullptr;
    }

    return docShell;
}

bool
ContentParent::RecvLoadPlugin(const uint32_t& aPluginId)
{
    return mozilla::plugins::SetupBridge(aPluginId, this);
}

bool
ContentParent::RecvFindPlugins(const uint32_t& aPluginEpoch,
                               nsTArray<PluginTag>* aPlugins,
                               uint32_t* aNewPluginEpoch)
{
    return mozilla::plugins::FindPluginsForContent(aPluginEpoch, aPlugins, aNewPluginEpoch);
}

 TabParent*
ContentParent::CreateBrowserOrApp(const TabContext& aContext,
                                  Element* aFrameElement,
                                  ContentParent* aOpenerContentParent)
{
    if (!sCanLaunchSubprocesses) {
        return nullptr;
    }

    ProcessPriority initialPriority = GetInitialProcessPriority(aFrameElement);
    bool isInContentProcess = (XRE_GetProcessType() != GeckoProcessType_Default);
    TabId tabId;

    nsIDocShell* docShell = GetOpenerDocShellHelper(aFrameElement);
    TabId openerTabId;
    if (docShell) {
        openerTabId = TabParent::GetTabIdFrom(docShell);
    }

    if (aContext.IsBrowserElement() || !aContext.HasOwnApp()) {
        nsRefPtr<TabParent> tp;
        nsRefPtr<nsIContentParent> constructorSender;
        if (isInContentProcess) {
            MOZ_ASSERT(aContext.IsBrowserElement());
            constructorSender = CreateContentBridgeParent(aContext,
                                                          initialPriority,
                                                          openerTabId,
                                                          &tabId);
        } else {
            if (aOpenerContentParent) {
                constructorSender = aOpenerContentParent;
            } else {
                constructorSender =
                    GetNewOrUsedBrowserProcess(aContext.IsBrowserElement(),
                                               initialPriority);
            }
            tabId = AllocateTabId(openerTabId,
                                  aContext.AsIPCTabContext(),
                                  constructorSender->ChildID());
        }
        if (constructorSender) {
            uint32_t chromeFlags = 0;

            nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(docShell);
            if (loadContext && loadContext->UsePrivateBrowsing()) {
                chromeFlags |= nsIWebBrowserChrome::CHROME_PRIVATE_WINDOW;
            }
            bool affectLifetime;
            docShell->GetAffectPrivateSessionLifetime(&affectLifetime);
            if (affectLifetime) {
                chromeFlags |= nsIWebBrowserChrome::CHROME_PRIVATE_LIFETIME;
            }

            if (tabId == 0) {
                return nullptr;
            }
            nsRefPtr<TabParent> tp(new TabParent(constructorSender, tabId,
                                                 aContext, chromeFlags));
            tp->SetInitedByParent();
            tp->SetOwnerElement(aFrameElement);

            PBrowserParent* browser = constructorSender->SendPBrowserConstructor(
                
                tp.forget().take(),
                tabId,
                aContext.AsIPCTabContext(),
                chromeFlags,
                constructorSender->ChildID(),
                constructorSender->IsForApp(),
                constructorSender->IsForBrowser());
            return static_cast<TabParent*>(browser);
        }
        return nullptr;
    }

    
    
    
    nsRefPtr<nsIContentParent> parent;
    bool reused = false;
    bool tookPreallocated = false;
    nsAutoString manifestURL;

    if (isInContentProcess) {
      parent = CreateContentBridgeParent(aContext,
                                         initialPriority,
                                         openerTabId,
                                         &tabId);
    }
    else {
        nsCOMPtr<mozIApplication> ownApp = aContext.GetOwnApp();

        if (!sAppContentParents) {
            sAppContentParents =
                new nsDataHashtable<nsStringHashKey, ContentParent*>();
        }

        
        
        if (NS_FAILED(ownApp->GetManifestURL(manifestURL))) {
            NS_ERROR("Failed to get manifest URL");
            return nullptr;
        }

        nsRefPtr<ContentParent> p = sAppContentParents->Get(manifestURL);

        if (!p && Preferences::GetBool("dom.ipc.reuse_parent_app")) {
            nsAutoString parentAppManifestURL;
            aFrameElement->GetAttr(kNameSpaceID_None,
                                   nsGkAtoms::parentapp, parentAppManifestURL);
            nsAdoptingString systemAppManifestURL =
                Preferences::GetString("b2g.system_manifest_url");
            nsCOMPtr<nsIAppsService> appsService =
                do_GetService(APPS_SERVICE_CONTRACTID);
            if (!parentAppManifestURL.IsEmpty() &&
                !parentAppManifestURL.Equals(systemAppManifestURL) &&
                appsService) {
                nsCOMPtr<mozIApplication> parentApp;
                nsCOMPtr<mozIApplication> app;
                appsService->GetAppByManifestURL(parentAppManifestURL,
                                                getter_AddRefs(parentApp));
                appsService->GetAppByManifestURL(manifestURL,
                                                getter_AddRefs(app));

                
                unsigned short parentAppStatus = 0;
                unsigned short appStatus = 0;
                if (app &&
                    NS_SUCCEEDED(app->GetAppStatus(&appStatus)) &&
                    appStatus == nsIPrincipal::APP_STATUS_CERTIFIED &&
                    parentApp &&
                    NS_SUCCEEDED(parentApp->GetAppStatus(&parentAppStatus)) &&
                    parentAppStatus == nsIPrincipal::APP_STATUS_CERTIFIED) {
                    
                    p = sAppContentParents->Get(parentAppManifestURL);
                }
            }
        }

        if (p) {
            
            
            
            if (!p->SetPriorityAndCheckIsAlive(initialPriority)) {
                p = nullptr;
            }
        }

        reused = !!p;
        if (!p) {
            p = GetNewOrPreallocatedAppProcess(ownApp,
                                               initialPriority,
                                               nullptr,
                                               &tookPreallocated);
            MOZ_ASSERT(p);
            sAppContentParents->Put(manifestURL, p);
        }
        tabId = AllocateTabId(openerTabId,
                              aContext.AsIPCTabContext(),
                              p->ChildID());
        parent = static_cast<nsIContentParent*>(p);
    }

    if (!parent || (tabId == 0)) {
        return nullptr;
    }

    uint32_t chromeFlags = 0;

    nsRefPtr<TabParent> tp = new TabParent(parent, tabId, aContext, chromeFlags);
    tp->SetInitedByParent();
    tp->SetOwnerElement(aFrameElement);
    PBrowserParent* browser = parent->SendPBrowserConstructor(
        
        nsRefPtr<TabParent>(tp).forget().take(),
        tabId,
        aContext.AsIPCTabContext(),
        chromeFlags,
        parent->ChildID(),
        parent->IsForApp(),
        parent->IsForBrowser());

    if (isInContentProcess) {
        
        return static_cast<TabParent*>(browser);
    }

    if (!browser) {
        
        
        if (!reused) {
            
            parent->AsContentParent()->KillHard();
            sAppContentParents->Remove(manifestURL);
            parent = nullptr;
        }

        
        
        
        
        if (tookPreallocated) {
          return ContentParent::CreateBrowserOrApp(aContext,
                                                   aFrameElement,
                                                   aOpenerContentParent);
        }

        
        return nullptr;
    }

    parent->AsContentParent()->MaybeTakeCPUWakeLock(aFrameElement);

    return static_cast<TabParent*>(browser);
}

 ContentBridgeParent*
ContentParent::CreateContentBridgeParent(const TabContext& aContext,
                                         const hal::ProcessPriority& aPriority,
                                         const TabId& aOpenerTabId,
                                          TabId* aTabId)
{
    MOZ_ASSERT(aTabId);

    ContentChild* child = ContentChild::GetSingleton();
    ContentParentId cpId;
    bool isForApp;
    bool isForBrowser;
    if (!child->SendCreateChildProcess(aContext.AsIPCTabContext(),
                                       aPriority,
                                       aOpenerTabId,
                                       &cpId,
                                       &isForApp,
                                       &isForBrowser,
                                       aTabId)) {
        return nullptr;
    }
    if (cpId == 0) {
        return nullptr;
    }
    if (!child->SendBridgeToChildProcess(cpId)) {
        return nullptr;
    }
    ContentBridgeParent* parent = child->GetLastBridge();
    parent->SetChildID(cpId);
    parent->SetIsForApp(isForApp);
    parent->SetIsForBrowser(isForBrowser);
    return parent;
}

void
ContentParent::GetAll(nsTArray<ContentParent*>& aArray)
{
    aArray.Clear();

    if (!sContentParents) {
        return;
    }

    for (ContentParent* cp = sContentParents->getFirst(); cp;
         cp = cp->LinkedListElement<ContentParent>::getNext()) {
        if (cp->mIsAlive) {
            aArray.AppendElement(cp);
        }
    }
}

void
ContentParent::GetAllEvenIfDead(nsTArray<ContentParent*>& aArray)
{
    aArray.Clear();

    if (!sContentParents) {
        return;
    }

    for (ContentParent* cp = sContentParents->getFirst(); cp;
         cp = cp->LinkedListElement<ContentParent>::getNext()) {
        aArray.AppendElement(cp);
    }
}

void
ContentParent::Init()
{
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        size_t length = ArrayLength(sObserverTopics);
        for (size_t i = 0; i < length; ++i) {
            obs->AddObserver(this, sObserverTopics[i], false);
        }
    }
    Preferences::AddStrongObserver(this, "");
    if (obs) {
        obs->NotifyObservers(static_cast<nsIObserver*>(this), "ipc:content-created", nullptr);
    }

#ifdef ACCESSIBILITY
    
    
    if (nsIPresShell::IsAccessibilityActive()) {
        unused << SendActivateA11y();
    }
#endif

    DebugOnly<FileUpdateDispatcher*> observer = FileUpdateDispatcher::GetSingleton();
    NS_ASSERTION(observer, "FileUpdateDispatcher is null");
}

namespace {

class SystemMessageHandledListener MOZ_FINAL
    : public nsITimerCallback
    , public LinkedListElement<SystemMessageHandledListener>
{
public:
    NS_DECL_ISUPPORTS

    SystemMessageHandledListener() {}

    static void OnSystemMessageHandled()
    {
        if (!sListeners) {
            return;
        }

        SystemMessageHandledListener* listener = sListeners->popFirst();
        if (!listener) {
            return;
        }

        
        listener->ShutDown();
    }

    void Init(WakeLock* aWakeLock)
    {
        MOZ_ASSERT(!mWakeLock);
        MOZ_ASSERT(!mTimer);

        
        

        if (!sListeners) {
            sListeners = new LinkedList<SystemMessageHandledListener>();
            ClearOnShutdown(&sListeners);
        }
        sListeners->insertBack(this);

        mWakeLock = aWakeLock;

        mTimer = do_CreateInstance("@mozilla.org/timer;1");

        uint32_t timeoutSec =
            Preferences::GetInt("dom.ipc.systemMessageCPULockTimeoutSec", 30);
        mTimer->InitWithCallback(this, timeoutSec * 1000,
                                 nsITimer::TYPE_ONE_SHOT);
    }

    NS_IMETHOD Notify(nsITimer* aTimer) MOZ_OVERRIDE
    {
        
        ShutDown();
        return NS_OK;
    }

private:
    ~SystemMessageHandledListener() {}

    static StaticAutoPtr<LinkedList<SystemMessageHandledListener> > sListeners;

    void ShutDown()
    {
        nsRefPtr<SystemMessageHandledListener> kungFuDeathGrip = this;

        ErrorResult rv;
        mWakeLock->Unlock(rv);

        if (mTimer) {
            mTimer->Cancel();
            mTimer = nullptr;
        }
    }

    nsRefPtr<WakeLock> mWakeLock;
    nsCOMPtr<nsITimer> mTimer;
};

StaticAutoPtr<LinkedList<SystemMessageHandledListener> >
    SystemMessageHandledListener::sListeners;

NS_IMPL_ISUPPORTS(SystemMessageHandledListener,
                  nsITimerCallback)

#ifdef MOZ_NUWA_PROCESS
class NuwaFreezeListener : public nsThreadManager::AllThreadsWereIdleListener
{
public:
    NuwaFreezeListener(ContentParent* parent)
        : mParent(parent)
    {
    }

    void OnAllThreadsWereIdle()
    {
        unused << mParent->SendNuwaFreeze();
        nsThreadManager::get()->RemoveAllThreadsWereIdleListener(this);
    }
private:
    nsRefPtr<ContentParent> mParent;
    virtual ~NuwaFreezeListener()
    {
    }
};
#endif 

} 

void
ContentParent::MaybeTakeCPUWakeLock(Element* aFrameElement)
{
    
    
    

    nsCOMPtr<nsIMozBrowserFrame> browserFrame =
        do_QueryInterface(aFrameElement);
    if (!browserFrame ||
        !browserFrame->GetIsExpectingSystemMessage()) {
        return;
    }

    nsRefPtr<PowerManagerService> pms = PowerManagerService::GetInstance();
    nsRefPtr<WakeLock> lock =
        pms->NewWakeLockOnBehalfOfProcess(NS_LITERAL_STRING("cpu"), this);

    
    nsRefPtr<SystemMessageHandledListener> listener =
        new SystemMessageHandledListener();
    listener->Init(lock);
}

bool
ContentParent::SetPriorityAndCheckIsAlive(ProcessPriority aPriority)
{
    ProcessPriorityManager::SetProcessPriority(this, aPriority);

    
    
    
    
    
    
    
    
    
    
#ifdef MOZ_WIDGET_GONK
    siginfo_t info;
    info.si_pid = 0;
    if (waitid(P_PID, Pid(), &info, WNOWAIT | WNOHANG | WEXITED) == 0
        && info.si_pid != 0) {
        return false;
    }
#endif

    return true;
}


static void
TryGetNameFromManifestURL(const nsAString& aManifestURL,
                          nsAString& aName)
{
    aName.Truncate();
    if (aManifestURL.IsEmpty() ||
        aManifestURL == MAGIC_PREALLOCATED_APP_MANIFEST_URL) {
        return;
    }

    nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
    NS_ENSURE_TRUE_VOID(appsService);

    nsCOMPtr<mozIApplication> app;
    appsService->GetAppByManifestURL(aManifestURL, getter_AddRefs(app));

    if (!app) {
        return;
    }

    app->GetName(aName);
}

void
ContentParent::TransformPreallocatedIntoApp(ContentParent* aOpener,
                                            const nsAString& aAppManifestURL)
{
    MOZ_ASSERT(IsPreallocated());
    mOpener = aOpener;
    mAppManifestURL = aAppManifestURL;
    TryGetNameFromManifestURL(aAppManifestURL, mAppName);
}

void
ContentParent::TransformPreallocatedIntoBrowser(ContentParent* aOpener)
{
    
    mOpener = aOpener;
    mAppManifestURL.Truncate();
    mIsForBrowser = true;
}

void
ContentParent::ShutDownProcess(ShutDownMethod aMethod)
{
    
    
    
    if (aMethod == SEND_SHUTDOWN_MESSAGE) {
        if (mIPCOpen && !mShutdownPending && SendShutdown()) {
            mShutdownPending = true;
            
            StartForceKillTimer();
        }

        
        
        return;
    }

    using mozilla::dom::quota::QuotaManager;

    if (QuotaManager* quotaManager = QuotaManager::Get()) {
        quotaManager->AbortCloseStoragesForProcess(this);
    }

    
    
    

    if (aMethod == CLOSE_CHANNEL && !mCalledClose) {
        
        
        mCalledClose = true;
        Close();
#ifdef MOZ_NUWA_PROCESS
        
        
        if (IsNuwaProcess()) {
            KillHard();
        }
#endif
    }

    if (aMethod == CLOSE_CHANNEL_WITH_ERROR && !mCalledCloseWithError) {
        MessageChannel* channel = GetIPCChannel();
        if (channel) {
            mCalledCloseWithError = true;
            channel->CloseWithError();
        }
    }

    const InfallibleTArray<POfflineCacheUpdateParent*>& ocuParents =
        ManagedPOfflineCacheUpdateParent();
    for (uint32_t i = 0; i < ocuParents.Length(); ++i) {
        nsRefPtr<mozilla::docshell::OfflineCacheUpdateParent> ocuParent =
            static_cast<mozilla::docshell::OfflineCacheUpdateParent*>(ocuParents[i]);
        ocuParent->StopSendingMessagesToChild();
    }

    
    
    MarkAsDead();

    
    
    
    
    ShutDownMessageManager();
}

bool
ContentParent::RecvFinishShutdown()
{
    
    
    
    MOZ_ASSERT(mShutdownPending);
    ShutDownProcess(CLOSE_CHANNEL);
    return true;
}

void
ContentParent::ShutDownMessageManager()
{
  if (!mMessageManager) {
    return;
  }

  mMessageManager->ReceiveMessage(
            static_cast<nsIContentFrameMessageManager*>(mMessageManager.get()),
            CHILD_PROCESS_SHUTDOWN_MESSAGE, false,
            nullptr, nullptr, nullptr, nullptr);

  mMessageManager->Disconnect();
  mMessageManager = nullptr;
}

void
ContentParent::MarkAsDead()
{
    if (!mAppManifestURL.IsEmpty()) {
        if (sAppContentParents) {
            sAppContentParents->Remove(mAppManifestURL);
            if (!sAppContentParents->Count()) {
                delete sAppContentParents;
                sAppContentParents = nullptr;
            }
        }
    } else if (sNonAppContentParents) {
        sNonAppContentParents->RemoveElement(this);
        if (!sNonAppContentParents->Length()) {
            delete sNonAppContentParents;
            sNonAppContentParents = nullptr;
        }
    }

    if (sPrivateContent) {
        sPrivateContent->RemoveElement(this);
        if (!sPrivateContent->Length()) {
            delete sPrivateContent;
            sPrivateContent = nullptr;
        }
    }

    mIsAlive = false;
}

void
ContentParent::OnChannelError()
{
    nsRefPtr<ContentParent> content(this);
#ifdef MOZ_NUWA_PROCESS
    
    PreallocatedProcessManager::MaybeForgetSpare(this);
#endif
    PContentParent::OnChannelError();
}

void
ContentParent::OnBeginSyncTransaction() {
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        nsCOMPtr<nsIConsoleService> console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
        JSContext *cx = nsContentUtils::GetCurrentJSContext();
        if (console && cx) {
            nsAutoString filename;
            uint32_t lineno = 0;
            nsJSUtils::GetCallingLocation(cx, filename, &lineno);
            nsCOMPtr<nsIScriptError> error(do_CreateInstance(NS_SCRIPTERROR_CONTRACTID));
            error->Init(NS_LITERAL_STRING("unsafe CPOW usage"), filename, EmptyString(),
                        lineno, 0, nsIScriptError::warningFlag, "chrome javascript");
            console->LogMessage(error);
        } else {
            NS_WARNING("Unsafe synchronous IPC message");
        }
    }
}

void
ContentParent::OnChannelConnected(int32_t pid)
{
    ProcessHandle handle;
    if (!base::OpenPrivilegedProcessHandle(pid, &handle)) {
        NS_WARNING("Can't open handle to child process.");
    }
    else {
        
        base::CloseProcessHandle(OtherProcess());
        SetOtherProcess(handle);

#if defined(ANDROID) || defined(LINUX)
        
        int32_t nice = Preferences::GetInt("dom.ipc.content.nice", 0);

        
        char* relativeNicenessStr = getenv("MOZ_CHILD_PROCESS_RELATIVE_NICENESS");
        if (relativeNicenessStr) {
            nice = atoi(relativeNicenessStr);
        }

        
        nsCOMPtr<nsIPropertyBag2> infoService = do_GetService(NS_SYSTEMINFO_CONTRACTID);
        if (infoService) {
            int32_t cpus;
            nsresult rv = infoService->GetPropertyAsInt32(NS_LITERAL_STRING("cpucount"), &cpus);
            if (NS_FAILED(rv)) {
                cpus = 1;
            }
            if (nice != 0 && cpus == 1) {
                setpriority(PRIO_PROCESS, pid, getpriority(PRIO_PROCESS, pid) + nice);
            }
        }
#endif
    }
}

void
ContentParent::ProcessingError(Result what)
{
    if (MsgDropped == what) {
        
        return;
    }
    
    KillHard();
}

typedef std::pair<ContentParent*, std::set<uint64_t> > IDPair;

namespace {
std::map<ContentParent*, std::set<uint64_t> >&
NestedBrowserLayerIds()
{
  MOZ_ASSERT(NS_IsMainThread());
  static std::map<ContentParent*, std::set<uint64_t> > sNestedBrowserIds;
  return sNestedBrowserIds;
}
} 

bool
ContentParent::RecvAllocateLayerTreeId(uint64_t* aId)
{
    *aId = CompositorParent::AllocateLayerTreeId();

    auto iter = NestedBrowserLayerIds().find(this);
    if (iter == NestedBrowserLayerIds().end()) {
        std::set<uint64_t> ids;
        ids.insert(*aId);
        NestedBrowserLayerIds().insert(IDPair(this, ids));
    } else {
        iter->second.insert(*aId);
    }
    return true;
}

bool
ContentParent::RecvDeallocateLayerTreeId(const uint64_t& aId)
{
    auto iter = NestedBrowserLayerIds().find(this);
    if (iter != NestedBrowserLayerIds().end() &&
        iter->second.find(aId) != iter->second.end()) {
        CompositorParent::DeallocateLayerTreeId(aId);
    } else {
        
        KillHard();
    }
    return true;
}

namespace {

void
DelayedDeleteSubprocess(GeckoChildProcessHost* aSubprocess)
{
    XRE_GetIOMessageLoop()
        ->PostTask(FROM_HERE,
                   new DeleteTask<GeckoChildProcessHost>(aSubprocess));
}




struct DelayedDeleteContentParentTask : public nsRunnable
{
    explicit DelayedDeleteContentParentTask(ContentParent* aObj) : mObj(aObj) { }

    
    NS_IMETHODIMP Run() { return NS_OK; }

    nsRefPtr<ContentParent> mObj;
};

}

void
ContentParent::ActorDestroy(ActorDestroyReason why)
{
    if (mForceKillTimer) {
        mForceKillTimer->Cancel();
        mForceKillTimer = nullptr;
    }

    
    
    mIPCOpen = false;

    if (why == NormalShutdown && !mCalledClose) {
        
        
        
        mCalledClose = true;
    }

    
    ShutDownProcess(why == NormalShutdown ? CLOSE_CHANNEL
                                          : CLOSE_CHANNEL_WITH_ERROR);

    nsRefPtr<ContentParent> kungFuDeathGrip(this);
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        size_t length = ArrayLength(sObserverTopics);
        for (size_t i = 0; i < length; ++i) {
            obs->RemoveObserver(static_cast<nsIObserver*>(this),
                                sObserverTopics[i]);
        }
    }

    
    nsRefPtr<nsMemoryReporterManager> mgr =
        nsMemoryReporterManager::GetOrCreate();
#ifdef MOZ_NUWA_PROCESS
    bool isMemoryChild = !IsNuwaProcess();
#else
    bool isMemoryChild = true;
#endif
    if (mgr && isMemoryChild) {
        mgr->DecrementNumChildProcesses();
    }

    
    Preferences::RemoveObserver(this, "");

#ifdef MOZ_NUWA_PROCESS
    
    if (IsNuwaProcess() && sNuwaPrefUpdates) {
        delete sNuwaPrefUpdates;
        sNuwaPrefUpdates = nullptr;
    }
#endif

    RecvRemoveGeolocationListener();

    mConsoleService = nullptr;

    if (obs) {
        nsRefPtr<nsHashPropertyBag> props = new nsHashPropertyBag();

        props->SetPropertyAsUint64(NS_LITERAL_STRING("childID"), mChildID);

        if (AbnormalShutdown == why) {
            Telemetry::Accumulate(Telemetry::SUBPROCESS_ABNORMAL_ABORT,
                                  NS_LITERAL_CSTRING("content"), 1);

            props->SetPropertyAsBool(NS_LITERAL_STRING("abnormal"), true);

#ifdef MOZ_CRASHREPORTER
            
            
            
            if (ManagedPCrashReporterParent().Length() > 0) {
                CrashReporterParent* crashReporter =
                    static_cast<CrashReporterParent*>(ManagedPCrashReporterParent()[0]);

                
                
                
                
                
                
                if (!mAppManifestURL.IsEmpty()) {
                    crashReporter->AnnotateCrashReport(NS_LITERAL_CSTRING("URL"),
                                                       NS_ConvertUTF16toUTF8(mAppManifestURL));
                }

                if (mCreatedPairedMinidumps) {
                    
                    
                    
                    
                    
                    crashReporter->GenerateChildData(nullptr);
                } else {
                    crashReporter->GenerateCrashReport(this, nullptr);
                }

                nsAutoString dumpID(crashReporter->ChildDumpID());
                props->SetPropertyAsAString(NS_LITERAL_STRING("dumpID"), dumpID);
            }
#endif
        }
        obs->NotifyObservers((nsIPropertyBag2*) props, "ipc:content-shutdown", nullptr);
    }

    mIdleListeners.Clear();

    MessageLoop::current()->
        PostTask(FROM_HERE,
                 NewRunnableFunction(DelayedDeleteSubprocess, mSubprocess));
    mSubprocess = nullptr;

    
    
    
    
    
    
    
    NS_DispatchToCurrentThread(new DelayedDeleteContentParentTask(this));

    
    ContentProcessManager *cpm = ContentProcessManager::GetSingleton();
    nsTArray<ContentParentId> childIDArray =
        cpm->GetAllChildProcessById(this->ChildID());
    for(uint32_t i = 0; i < childIDArray.Length(); i++) {
        ContentParent* cp = cpm->GetContentProcessById(childIDArray[i]);
        MessageLoop::current()->PostTask(
            FROM_HERE,
            NewRunnableMethod(cp, &ContentParent::ShutDownProcess,
                              CLOSE_CHANNEL));
    }
    cpm->RemoveContentProcess(this->ChildID());
}

void
ContentParent::NotifyTabDestroying(PBrowserParent* aTab)
{
    
    
    
    
    
    int32_t numLiveTabs = ManagedPBrowserParent().Length();
    ++mNumDestroyingTabs;
    if (mNumDestroyingTabs != numLiveTabs) {
        return;
    }

    
    
    MarkAsDead();
    StartForceKillTimer();
}

void
ContentParent::StartForceKillTimer()
{
    if (mForceKillTimer || !mIPCOpen) {
        return;
    }

    int32_t timeoutSecs =
        Preferences::GetInt("dom.ipc.tabs.shutdownTimeoutSecs", 5);
    if (timeoutSecs > 0) {
        mForceKillTimer = do_CreateInstance("@mozilla.org/timer;1");
        MOZ_ASSERT(mForceKillTimer);
        mForceKillTimer->InitWithFuncCallback(ContentParent::ForceKillTimerCallback,
                                              this,
                                              timeoutSecs * 1000,
                                              nsITimer::TYPE_ONE_SHOT);
    }
}

void
ContentParent::NotifyTabDestroyed(PBrowserParent* aTab,
                                  bool aNotifiedDestroying)
{
    if (aNotifiedDestroying) {
        --mNumDestroyingTabs;
    }

    
    
    
    if (ManagedPBrowserParent().Length() == 1) {
        
        
        MessageLoop::current()->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &ContentParent::ShutDownProcess,
                              SEND_SHUTDOWN_MESSAGE));
    }
}

jsipc::JavaScriptShared*
ContentParent::GetCPOWManager()
{
    if (ManagedPJavaScriptParent().Length()) {
        return static_cast<JavaScriptParent*>(ManagedPJavaScriptParent()[0]);
    }
    return nullptr;
}

TestShellParent*
ContentParent::CreateTestShell()
{
  return static_cast<TestShellParent*>(SendPTestShellConstructor());
}

bool
ContentParent::DestroyTestShell(TestShellParent* aTestShell)
{
    return PTestShellParent::Send__delete__(aTestShell);
}

TestShellParent*
ContentParent::GetTestShellSingleton()
{
    if (!ManagedPTestShellParent().Length())
        return nullptr;
    return static_cast<TestShellParent*>(ManagedPTestShellParent()[0]);
}

void
ContentParent::InitializeMembers()
{
    mSubprocess = nullptr;
    mChildID = gContentChildID++;
    mGeolocationWatchID = -1;
    mNumDestroyingTabs = 0;
    mIsAlive = true;
    mSendPermissionUpdates = false;
    mSendDataStoreInfos = false;
    mCalledClose = false;
    mCalledCloseWithError = false;
    mCalledKillHard = false;
    mCreatedPairedMinidumps = false;
    mShutdownPending = false;
    mIPCOpen = true;
}

ContentParent::ContentParent(mozIApplication* aApp,
                             ContentParent* aOpener,
                             bool aIsForBrowser,
                             bool aIsForPreallocated,
                             ProcessPriority aInitialPriority ,
                             bool aIsNuwaProcess )
    : nsIContentParent()
    , mOpener(aOpener)
    , mIsForBrowser(aIsForBrowser)
    , mIsNuwaProcess(aIsNuwaProcess)
{
    InitializeMembers();  

    
    
    MOZ_ASSERT(!!aApp + aIsForBrowser + aIsForPreallocated <= 1);

    
    MOZ_ASSERT_IF(aIsNuwaProcess, aIsForPreallocated);

    
    if (!sContentParents) {
        sContentParents = new LinkedList<ContentParent>();
    }
    if (!aIsNuwaProcess) {
        sContentParents->insertBack(this);
    }

    if (aApp) {
        aApp->GetManifestURL(mAppManifestURL);
        aApp->GetName(mAppName);
    } else if (aIsForPreallocated) {
        mAppManifestURL = MAGIC_PREALLOCATED_APP_MANIFEST_URL;
    }

    
    
    nsDebugImpl::SetMultiprocessMode("Parent");

#if defined(XP_WIN) && !defined(MOZ_B2G)
    
    
    
    GetIPCChannel()->SetChannelFlags(MessageChannel::REQUIRE_DEFERRED_MESSAGE_PROTECTION);
#endif

    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    ChildPrivileges privs = aIsNuwaProcess
        ? base::PRIVILEGES_INHERIT
        : base::PRIVILEGES_DEFAULT;
    mSubprocess = new GeckoChildProcessHost(GeckoProcessType_Content, privs);

    IToplevelProtocol::SetTransport(mSubprocess->GetChannel());

    if (!aIsNuwaProcess) {
        
        nsRefPtr<nsMemoryReporterManager> mgr =
            nsMemoryReporterManager::GetOrCreate();
        if (mgr) {
            mgr->IncrementNumChildProcesses();
        }
    }

    std::vector<std::string> extraArgs;
    if (aIsNuwaProcess) {
        extraArgs.push_back("-nuwa");
    }
    mSubprocess->LaunchAndWaitForProcessHandle(extraArgs);

    Open(mSubprocess->GetChannel(), mSubprocess->GetOwnedChildProcessHandle());

    InitInternal(aInitialPriority,
                 true, 
                 true  );

    ContentProcessManager::GetSingleton()->AddContentProcess(this);

    
    SetReplyTimeoutMs(Preferences::GetInt("dom.ipc.cpow.timeout", 0));
}

#ifdef MOZ_NUWA_PROCESS
static const mozilla::ipc::FileDescriptor*
FindFdProtocolFdMapping(const nsTArray<ProtocolFdMapping>& aFds,
                        ProtocolId aProtoId)
{
    for (unsigned int i = 0; i < aFds.Length(); i++) {
        if (aFds[i].protocolId() == aProtoId) {
            return &aFds[i].fd();
        }
    }
    return nullptr;
}






ContentParent::ContentParent(ContentParent* aTemplate,
                             const nsAString& aAppManifestURL,
                             base::ProcessHandle aPid,
                             const nsTArray<ProtocolFdMapping>& aFds)
    : mAppManifestURL(aAppManifestURL)
    , mIsForBrowser(false)
    , mIsNuwaProcess(false)
{
    InitializeMembers();  

    sContentParents->insertBack(this);

    
    
    nsDebugImpl::SetMultiprocessMode("Parent");

    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    const FileDescriptor* fd = FindFdProtocolFdMapping(aFds, GetProtocolId());

    NS_ASSERTION(fd != nullptr, "IPC Channel for PContent is necessary!");
    mSubprocess = new GeckoExistingProcessHost(GeckoProcessType_Content,
                                               aPid,
                                               *fd);

    
    nsRefPtr<nsMemoryReporterManager> mgr =
        nsMemoryReporterManager::GetOrCreate();
    if (mgr) {
        mgr->IncrementNumChildProcesses();
    }

    mSubprocess->LaunchAndWaitForProcessHandle();

    
    IToplevelProtocol::SetTransport(mSubprocess->GetChannel());
    ProtocolCloneContext cloneContext;
    cloneContext.SetContentParent(this);
    CloneManagees(aTemplate, &cloneContext);
    CloneOpenedToplevels(aTemplate, aFds, aPid, &cloneContext);

    Open(mSubprocess->GetChannel(),
         mSubprocess->GetChildProcessHandle());

    
    
    
    ProcessPriority priority;
    if (IsPreallocated()) {
        priority = PROCESS_PRIORITY_PREALLOC;
    } else {
        priority = PROCESS_PRIORITY_FOREGROUND;
    }

    mSendPermissionUpdates = aTemplate->mSendPermissionUpdates;

    InitInternal(priority,
                 false, 
                 false  );

    ContentProcessManager::GetSingleton()->AddContentProcess(this);
}
#endif  

ContentParent::~ContentParent()
{
    if (mForceKillTimer) {
        mForceKillTimer->Cancel();
    }

    if (OtherProcess())
        base::CloseProcessHandle(OtherProcess());

    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    
    MOZ_ASSERT(!sPrivateContent || !sPrivateContent->Contains(this));
    if (mAppManifestURL.IsEmpty()) {
        MOZ_ASSERT(!sNonAppContentParents ||
                   !sNonAppContentParents->Contains(this));
    } else {
        
        
        
        
        MOZ_ASSERT(!sAppContentParents ||
                   sAppContentParents->Get(mAppManifestURL) != this);
    }

#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        sNuwaReady = false;
        sNuwaPid = 0;
    }
#endif
}

void
ContentParent::InitInternal(ProcessPriority aInitialPriority,
                            bool aSetupOffMainThreadCompositing,
                            bool aSendRegisteredChrome)
{
    
    
    
    
    
    
    ProcessPriorityManager::SetProcessPriority(this, aInitialPriority);

    if (aSetupOffMainThreadCompositing) {
        
        
        
        
        
        
        
        
        bool useOffMainThreadCompositing = !!CompositorParent::CompositorLoop();
        if (useOffMainThreadCompositing) {
            DebugOnly<bool> opened = PCompositor::Open(this);
            MOZ_ASSERT(opened);

#ifndef MOZ_WIDGET_GONK
            if (gfxPrefs::AsyncVideoOOPEnabled()) {
                opened = PImageBridge::Open(this);
                MOZ_ASSERT(opened);
            }
#else
            opened = PImageBridge::Open(this);
            MOZ_ASSERT(opened);
#endif
        }
#ifdef MOZ_WIDGET_GONK
        DebugOnly<bool> opened = PSharedBufferManager::Open(this);
        MOZ_ASSERT(opened);
#endif
    }

    if (aSendRegisteredChrome) {
        nsCOMPtr<nsIChromeRegistry> registrySvc = nsChromeRegistry::GetService();
        nsChromeRegistryChrome* chromeRegistry =
            static_cast<nsChromeRegistryChrome*>(registrySvc.get());
        chromeRegistry->SendRegisteredChrome(this);
    }

    if (gAppData) {
        nsCString version(gAppData->version);
        nsCString buildID(gAppData->buildID);
        nsCString name(gAppData->name);
        nsCString UAName(gAppData->UAName);
        nsCString ID(gAppData->ID);
        nsCString vendor(gAppData->vendor);

        
        unused << SendAppInfo(version, buildID, name, UAName, ID, vendor);
    }

    nsStyleSheetService *sheetService = nsStyleSheetService::GetInstance();
    if (sheetService) {
        
        

        nsCOMArray<nsIStyleSheet>& agentSheets = *sheetService->AgentStyleSheets();
        for (uint32_t i = 0; i < agentSheets.Length(); i++) {
            URIParams uri;
            SerializeURI(agentSheets[i]->GetSheetURI(), uri);
            unused << SendLoadAndRegisterSheet(uri, nsIStyleSheetService::AGENT_SHEET);
        }

        nsCOMArray<nsIStyleSheet>& userSheets = *sheetService->UserStyleSheets();
        for (uint32_t i = 0; i < userSheets.Length(); i++) {
            URIParams uri;
            SerializeURI(userSheets[i]->GetSheetURI(), uri);
            unused << SendLoadAndRegisterSheet(uri, nsIStyleSheetService::USER_SHEET);
        }

        nsCOMArray<nsIStyleSheet>& authorSheets = *sheetService->AuthorStyleSheets();
        for (uint32_t i = 0; i < authorSheets.Length(); i++) {
            URIParams uri;
            SerializeURI(authorSheets[i]->GetSheetURI(), uri);
            unused << SendLoadAndRegisterSheet(uri, nsIStyleSheetService::AUTHOR_SHEET);
        }
    }

#ifdef MOZ_CONTENT_SANDBOX
    bool shouldSandbox = true;
#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        shouldSandbox = false;
    }
#endif
    if (shouldSandbox && !SendSetProcessSandbox()) {
        KillHard();
    }
#endif
}

bool
ContentParent::IsAlive()
{
    return mIsAlive;
}

bool
ContentParent::IsForApp()
{
    return !mAppManifestURL.IsEmpty();
}

#ifdef MOZ_NUWA_PROCESS
bool
ContentParent::IsNuwaProcess() const
{
    return mIsNuwaProcess;
}
#endif

int32_t
ContentParent::Pid()
{
    if (!mSubprocess || !mSubprocess->GetChildProcessHandle()) {
        return -1;
    }
    return base::GetProcId(mSubprocess->GetChildProcessHandle());
}

bool
ContentParent::RecvReadPrefsArray(InfallibleTArray<PrefSetting>* aPrefs)
{
    Preferences::GetPreferences(aPrefs);
    return true;
}

bool
ContentParent::RecvReadFontList(InfallibleTArray<FontListEntry>* retValue)
{
#ifdef ANDROID
    gfxAndroidPlatform::GetPlatform()->GetFontList(retValue);
#endif
    return true;
}

bool
ContentParent::RecvReadPermissions(InfallibleTArray<IPC::Permission>* aPermissions)
{
#ifdef MOZ_PERMISSIONS
    nsCOMPtr<nsIPermissionManager> permissionManagerIface =
        services::GetPermissionManager();
    nsPermissionManager* permissionManager =
        static_cast<nsPermissionManager*>(permissionManagerIface.get());
    NS_ABORT_IF_FALSE(permissionManager,
                 "We have no permissionManager in the Chrome process !");

    nsCOMPtr<nsISimpleEnumerator> enumerator;
    DebugOnly<nsresult> rv = permissionManager->GetEnumerator(getter_AddRefs(enumerator));
    NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "Could not get enumerator!");
    while(1) {
        bool hasMore;
        enumerator->HasMoreElements(&hasMore);
        if (!hasMore)
            break;

        nsCOMPtr<nsISupports> supp;
        enumerator->GetNext(getter_AddRefs(supp));
        nsCOMPtr<nsIPermission> perm = do_QueryInterface(supp);

        nsCString host;
        perm->GetHost(host);
        uint32_t appId;
        perm->GetAppId(&appId);
        bool isInBrowserElement;
        perm->GetIsInBrowserElement(&isInBrowserElement);
        nsCString type;
        perm->GetType(type);
        uint32_t capability;
        perm->GetCapability(&capability);
        uint32_t expireType;
        perm->GetExpireType(&expireType);
        int64_t expireTime;
        perm->GetExpireTime(&expireTime);

        aPermissions->AppendElement(IPC::Permission(host, appId,
                                                    isInBrowserElement, type,
                                                    capability, expireType,
                                                    expireTime));
    }

    
    mSendPermissionUpdates = true;
#endif

    return true;
}

bool
ContentParent::RecvSetClipboardText(const nsString& text,
                                       const bool& isPrivateData,
                                       const int32_t& whichClipboard)
{
    nsresult rv;
    nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
    NS_ENSURE_SUCCESS(rv, true);

    nsCOMPtr<nsISupportsString> dataWrapper =
        do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, true);

    rv = dataWrapper->SetData(text);
    NS_ENSURE_SUCCESS(rv, true);

    nsCOMPtr<nsITransferable> trans = do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
    NS_ENSURE_SUCCESS(rv, true);
    trans->Init(nullptr);

    
    trans->AddDataFlavor(kUnicodeMime);
    trans->SetIsPrivateData(isPrivateData);

    nsCOMPtr<nsISupports> nsisupportsDataWrapper =
        do_QueryInterface(dataWrapper);

    rv = trans->SetTransferData(kUnicodeMime, nsisupportsDataWrapper,
                                text.Length() * sizeof(char16_t));
    NS_ENSURE_SUCCESS(rv, true);

    clipboard->SetData(trans, nullptr, whichClipboard);
    return true;
}

bool
ContentParent::RecvGetClipboardText(const int32_t& whichClipboard, nsString* text)
{
    nsresult rv;
    nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
    NS_ENSURE_SUCCESS(rv, true);

    nsCOMPtr<nsITransferable> trans = do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
    NS_ENSURE_SUCCESS(rv, true);
    trans->Init(nullptr);
    trans->AddDataFlavor(kUnicodeMime);

    clipboard->GetData(trans, whichClipboard);
    nsCOMPtr<nsISupports> tmp;
    uint32_t len;
    rv = trans->GetTransferData(kUnicodeMime, getter_AddRefs(tmp), &len);
    if (NS_FAILED(rv))
        return true;

    nsCOMPtr<nsISupportsString> supportsString = do_QueryInterface(tmp);
    
    if (!supportsString)
        return true;
    supportsString->GetData(*text);
    return true;
}

bool
ContentParent::RecvEmptyClipboard(const int32_t& whichClipboard)
{
    nsresult rv;
    nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
    NS_ENSURE_SUCCESS(rv, true);

    clipboard->EmptyClipboard(whichClipboard);

    return true;
}

bool
ContentParent::RecvClipboardHasText(const int32_t& whichClipboard, bool* hasText)
{
    nsresult rv;
    nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
    NS_ENSURE_SUCCESS(rv, true);

    clipboard->HasDataMatchingFlavors(sClipboardTextFlavors, 1,
                                      whichClipboard, hasText);
    return true;
}

bool
ContentParent::RecvGetSystemColors(const uint32_t& colorsCount, InfallibleTArray<uint32_t>* colors)
{
#ifdef MOZ_WIDGET_ANDROID
    NS_ASSERTION(AndroidBridge::Bridge() != nullptr, "AndroidBridge is not available");
    if (AndroidBridge::Bridge() == nullptr) {
        
        return true;
    }

    colors->AppendElements(colorsCount);

    
    
    AndroidBridge::Bridge()->GetSystemColors((AndroidSystemColors*)colors->Elements());
#endif
    return true;
}

bool
ContentParent::RecvGetIconForExtension(const nsCString& aFileExt, const uint32_t& aIconSize, InfallibleTArray<uint8_t>* bits)
{
#ifdef MOZ_WIDGET_ANDROID
    NS_ASSERTION(AndroidBridge::Bridge() != nullptr, "AndroidBridge is not available");
    if (AndroidBridge::Bridge() == nullptr) {
        
        return true;
    }

    bits->AppendElements(aIconSize * aIconSize * 4);

    AndroidBridge::Bridge()->GetIconForExtension(aFileExt, aIconSize, bits->Elements());
#endif
    return true;
}

bool
ContentParent::RecvGetShowPasswordSetting(bool* showPassword)
{
    
    *showPassword = true;
#ifdef MOZ_WIDGET_ANDROID
    NS_ASSERTION(AndroidBridge::Bridge() != nullptr, "AndroidBridge is not available");

    *showPassword = mozilla::widget::GeckoAppShell::GetShowPasswordSetting();
#endif
    return true;
}

bool
ContentParent::RecvFirstIdle()
{
    
    
    
    
    PreallocatedProcessManager::AllocateAfterDelay();
    return true;
}

bool
ContentParent::RecvAudioChannelGetState(const AudioChannel& aChannel,
                                        const bool& aElementHidden,
                                        const bool& aElementWasHidden,
                                        AudioChannelState* aState)
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetOrCreateAudioChannelService();
    *aState = AUDIO_CHANNEL_STATE_NORMAL;
    MOZ_ASSERT(service);
    *aState = service->GetStateInternal(aChannel, mChildID,
                                        aElementHidden, aElementWasHidden);

    return true;
}

bool
ContentParent::RecvAudioChannelRegisterType(const AudioChannel& aChannel,
                                            const bool& aWithVideo)
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetOrCreateAudioChannelService();
    MOZ_ASSERT(service);
    service->RegisterType(aChannel, mChildID, aWithVideo);

    return true;
}

bool
ContentParent::RecvAudioChannelUnregisterType(const AudioChannel& aChannel,
                                              const bool& aElementHidden,
                                              const bool& aWithVideo)
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetOrCreateAudioChannelService();
    MOZ_ASSERT(service);
    service->UnregisterType(aChannel, aElementHidden, mChildID, aWithVideo);

    return true;
}

bool
ContentParent::RecvAudioChannelChangedNotification()
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetOrCreateAudioChannelService();
    MOZ_ASSERT(service);
    service->SendAudioChannelChangedNotification(ChildID());

    return true;
}

bool
ContentParent::RecvAudioChannelChangeDefVolChannel(const int32_t& aChannel,
                                                   const bool& aHidden)
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetOrCreateAudioChannelService();
    MOZ_ASSERT(service);
    service->SetDefaultVolumeControlChannelInternal(aChannel,
                                                    aHidden, mChildID);
    return true;
}

bool
ContentParent::RecvDataStoreGetStores(
                                    const nsString& aName,
                                    const nsString& aOwner,
                                    const IPC::Principal& aPrincipal,
                                    InfallibleTArray<DataStoreSetting>* aValue)
{
  nsRefPtr<DataStoreService> service = DataStoreService::GetOrCreate();
  if (NS_WARN_IF(!service)) {
    return false;
  }

  nsresult rv = service->GetDataStoresFromIPC(aName, aOwner, aPrincipal, aValue);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  mSendDataStoreInfos = true;
  return true;
}

bool
ContentParent::RecvGetVolumes(InfallibleTArray<VolumeInfo>* aResult)
{
#ifdef MOZ_WIDGET_GONK
    nsRefPtr<nsVolumeService> vs = nsVolumeService::GetSingleton();
    vs->GetVolumesForIPC(aResult);
    return true;
#else
    NS_WARNING("ContentParent::RecvGetVolumes shouldn't be called when MOZ_WIDGET_GONK is not defined");
    return false;
#endif
}

bool
ContentParent::RecvNuwaReady()
{
#ifdef MOZ_NUWA_PROCESS
    if (!IsNuwaProcess()) {
        NS_ERROR(
            nsPrintfCString(
                "Terminating child process %d for unauthorized IPC message: NuwaReady",
                Pid()).get());

        KillHard();
        return false;
    }
    sNuwaReady = true;
    PreallocatedProcessManager::OnNuwaReady();
    return true;
#else
    NS_ERROR("ContentParent::RecvNuwaReady() not implemented!");
    return false;
#endif
}

bool
ContentParent::RecvNuwaWaitForFreeze()
{
#ifdef MOZ_NUWA_PROCESS
    nsRefPtr<NuwaFreezeListener> listener = new NuwaFreezeListener(this);
    nsThreadManager::get()->AddAllThreadsWereIdleListener(listener);
    return true;
#else 
    NS_ERROR("ContentParent::RecvNuwaWaitForFreeze() not implemented!");
    return false;
#endif 
}

bool
ContentParent::RecvAddNewProcess(const uint32_t& aPid,
                                 const InfallibleTArray<ProtocolFdMapping>& aFds)
{
#ifdef MOZ_NUWA_PROCESS
    if (!IsNuwaProcess()) {
        NS_ERROR(
            nsPrintfCString(
                "Terminating child process %d for unauthorized IPC message: "
                "AddNewProcess(%d)", Pid(), aPid).get());

        KillHard();
        return false;
    }
    nsRefPtr<ContentParent> content;
    content = new ContentParent(this,
                                MAGIC_PREALLOCATED_APP_MANIFEST_URL,
                                aPid,
                                aFds);
    content->Init();

    size_t numNuwaPrefUpdates = sNuwaPrefUpdates ?
                                sNuwaPrefUpdates->Length() : 0;
    
    for (size_t i = 0; i < numNuwaPrefUpdates; i++) {
        mozilla::unused << content->SendPreferenceUpdate(sNuwaPrefUpdates->ElementAt(i));
    }

    
    bool isOffline;
    InfallibleTArray<nsString> unusedDictionaries;
    ClipboardCapabilities clipboardCaps;
    RecvGetXPCOMProcessAttributes(&isOffline, &unusedDictionaries,
                                  &clipboardCaps);
    mozilla::unused << content->SendSetOffline(isOffline);
    MOZ_ASSERT(!clipboardCaps.supportsSelectionClipboard() &&
               !clipboardCaps.supportsFindClipboard(),
               "Unexpected values");

    PreallocatedProcessManager::PublishSpareProcess(content);
    return true;
#else
    NS_ERROR("ContentParent::RecvAddNewProcess() not implemented!");
    return false;
#endif
}



NS_IMPL_CYCLE_COLLECTION_0(ContentParent)

NS_IMPL_CYCLE_COLLECTING_ADDREF(ContentParent)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ContentParent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ContentParent)
  NS_INTERFACE_MAP_ENTRY(nsIContentParent)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPositionCallback)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPositionErrorCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
ContentParent::Observe(nsISupports* aSubject,
                       const char* aTopic,
                       const char16_t* aData)
{
    if (mSubprocess && (!strcmp(aTopic, "profile-before-change") ||
                        !strcmp(aTopic, "xpcom-shutdown"))) {
        
        ShutDownProcess(SEND_SHUTDOWN_MESSAGE);

        
        
        
        while (mIPCOpen) {
            NS_ProcessNextEvent(nullptr, true);
        }
        NS_ASSERTION(!mSubprocess, "Close should have nulled mSubprocess");
    }

    if (!mIsAlive || !mSubprocess)
        return NS_OK;

    
    if (!strcmp(aTopic, "memory-pressure") &&
        !StringEndsWith(nsDependentString(aData),
                        NS_LITERAL_STRING("-no-forward"))) {
        unused << SendFlushMemory(nsDependentString(aData));
    }
    
    else if (!strcmp(aTopic, "nsPref:changed")) {
        
        NS_LossyConvertUTF16toASCII strData(aData);

        PrefSetting pref(strData, null_t(), null_t());
        Preferences::GetPreference(&pref);
#ifdef MOZ_NUWA_PROCESS
        if (IsNuwaProcess() && PreallocatedProcessManager::IsNuwaReady()) {
            
            
            if (!sNuwaPrefUpdates) {
                sNuwaPrefUpdates = new nsTArray<PrefSetting>();
            }
            sNuwaPrefUpdates->AppendElement(pref);
        } else if (!SendPreferenceUpdate(pref)) {
            return NS_ERROR_NOT_AVAILABLE;
        }
#else
        if (!SendPreferenceUpdate(pref)) {
            return NS_ERROR_NOT_AVAILABLE;
        }
#endif
    }
    else if (!strcmp(aTopic, NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC)) {
      NS_ConvertUTF16toUTF8 dataStr(aData);
      const char *offline = dataStr.get();
#ifdef MOZ_NUWA_PROCESS
      if (!(IsNuwaReady() && IsNuwaProcess())) {
#endif
          if (!SendSetOffline(!strcmp(offline, "true") ? true : false)) {
              return NS_ERROR_NOT_AVAILABLE;
          }
#ifdef MOZ_NUWA_PROCESS
      }
#endif
    }
    
    else if (!strcmp(aTopic, "alertfinished") ||
             !strcmp(aTopic, "alertclickcallback") ||
             !strcmp(aTopic, "alertshow") ) {
        if (!SendNotifyAlertsObserver(nsDependentCString(aTopic),
                                      nsDependentString(aData)))
            return NS_ERROR_NOT_AVAILABLE;
    }
    else if (!strcmp(aTopic, "child-memory-reporter-request")) {
        bool isNuwa = false;
#ifdef MOZ_NUWA_PROCESS
        isNuwa = IsNuwaProcess();
#endif
        if (!isNuwa) {
            unsigned generation;
            int anonymize, minimize, identOffset = -1;
            nsDependentString msg(aData);
            NS_ConvertUTF16toUTF8 cmsg(msg);

            if (sscanf(cmsg.get(),
                       "generation=%x anonymize=%d minimize=%d DMDident=%n",
                       &generation, &anonymize, &minimize, &identOffset) < 3
                || identOffset < 0) {
                return NS_ERROR_INVALID_ARG;
            }
            
            
            MOZ_ASSERT(cmsg[identOffset - 1] == '=');
            MaybeFileDesc dmdFileDesc = void_t();
#ifdef MOZ_DMD
            nsAutoString dmdIdent(Substring(msg, identOffset));
            if (!dmdIdent.IsEmpty()) {
                FILE *dmdFile = nullptr;
                nsresult rv = nsMemoryInfoDumper::OpenDMDFile(dmdIdent, Pid(), &dmdFile);
                if (NS_WARN_IF(NS_FAILED(rv))) {
                    
                    dmdFile = nullptr;
                }
                if (dmdFile) {
                    dmdFileDesc = FILEToFileDescriptor(dmdFile);
                    fclose(dmdFile);
                }
            }
#endif
            unused << SendPMemoryReportRequestConstructor(
              generation, anonymize, minimize, dmdFileDesc);
        }
    }
    else if (!strcmp(aTopic, "child-gc-request")){
        unused << SendGarbageCollect();
    }
    else if (!strcmp(aTopic, "child-cc-request")){
        unused << SendCycleCollect();
    }
    else if (!strcmp(aTopic, "child-mmu-request")){
        unused << SendMinimizeMemoryUsage();
    }
    else if (!strcmp(aTopic, "last-pb-context-exited")) {
        unused << SendLastPrivateDocShellDestroyed();
    }
    else if (!strcmp(aTopic, "file-watcher-update")) {
        nsCString creason;
        CopyUTF16toUTF8(aData, creason);
        DeviceStorageFile* file = static_cast<DeviceStorageFile*>(aSubject);

#ifdef MOZ_NUWA_PROCESS
        if (!(IsNuwaReady() && IsNuwaProcess()))
#endif
        {
            unused << SendFilePathUpdate(file->mStorageType, file->mStorageName, file->mPath, creason);
        }
    }
#ifdef MOZ_WIDGET_GONK
    else if(!strcmp(aTopic, NS_VOLUME_STATE_CHANGED)) {
        nsCOMPtr<nsIVolume> vol = do_QueryInterface(aSubject);
        if (!vol) {
            return NS_ERROR_NOT_AVAILABLE;
        }

        nsString volName;
        nsString mountPoint;
        int32_t  state;
        int32_t  mountGeneration;
        bool     isMediaPresent;
        bool     isSharing;
        bool     isFormatting;
        bool     isFake;
        bool     isUnmounting;
        bool     isRemovable;
        bool     isHotSwappable;

        vol->GetName(volName);
        vol->GetMountPoint(mountPoint);
        vol->GetState(&state);
        vol->GetMountGeneration(&mountGeneration);
        vol->GetIsMediaPresent(&isMediaPresent);
        vol->GetIsSharing(&isSharing);
        vol->GetIsFormatting(&isFormatting);
        vol->GetIsFake(&isFake);
        vol->GetIsUnmounting(&isUnmounting);
        vol->GetIsRemovable(&isRemovable);
        vol->GetIsHotSwappable(&isHotSwappable);

#ifdef MOZ_NUWA_PROCESS
        if (!(IsNuwaReady() && IsNuwaProcess()))
#endif
        {
            unused << SendFileSystemUpdate(volName, mountPoint, state,
                                           mountGeneration, isMediaPresent,
                                           isSharing, isFormatting, isFake,
                                           isUnmounting, isRemovable, isHotSwappable);
        }
    } else if (!strcmp(aTopic, "phone-state-changed")) {
        nsString state(aData);
        unused << SendNotifyPhoneStateChange(state);
    }
#endif
#ifdef ACCESSIBILITY
    
    
    else if (aData && (*aData == '1') &&
             !strcmp(aTopic, "a11y-init-or-shutdown")) {
        unused << SendActivateA11y();
    }
#endif
    else if (!strcmp(aTopic, "app-theme-changed")) {
        unused << SendOnAppThemeChanged();
    }
#ifdef MOZ_ENABLE_PROFILER_SPS
    else if (!strcmp(aTopic, "profiler-started")) {
        nsCOMPtr<nsIProfilerStartParams> params(do_QueryInterface(aSubject));
        uint32_t entries;
        double interval;
        params->GetEntries(&entries);
        params->GetInterval(&interval);
        const nsTArray<nsCString>& features = params->GetFeatures();
        const nsTArray<nsCString>& threadFilterNames = params->GetThreadFilterNames();
        unused << SendStartProfiler(entries, interval, features, threadFilterNames);
    }
    else if (!strcmp(aTopic, "profiler-stopped")) {
        unused << SendStopProfiler();
    }
    else if (!strcmp(aTopic, "profiler-subprocess")) {
        nsCOMPtr<nsIProfileSaveEvent> pse = do_QueryInterface(aSubject);
        if (pse) {
            nsCString result;
            unused << SendGetProfile(&result);
            if (!result.IsEmpty()) {
                pse->AddSubProfile(result.get());
            }
        }
    }
#endif
    return NS_OK;
}

  a11y::PDocAccessibleParent*
ContentParent::AllocPDocAccessibleParent(PDocAccessibleParent* aParent, const uint64_t&)
{
#ifdef ACCESSIBILITY
  return new a11y::DocAccessibleParent();
#else
  return nullptr;
#endif
}

bool
ContentParent::DeallocPDocAccessibleParent(PDocAccessibleParent* aParent)
{
#ifdef ACCESSIBILITY
  delete static_cast<a11y::DocAccessibleParent*>(aParent);
#endif
  return true;
}

bool
ContentParent::RecvPDocAccessibleConstructor(PDocAccessibleParent* aDoc, PDocAccessibleParent* aParentDoc, const uint64_t& aParentID)
{
#ifdef ACCESSIBILITY
  auto doc = static_cast<a11y::DocAccessibleParent*>(aDoc);
  if (aParentDoc) {
    MOZ_ASSERT(aParentID);
    auto parentDoc = static_cast<a11y::DocAccessibleParent*>(aParentDoc);
    return parentDoc->AddChildDoc(doc, aParentID);
  } else {
    MOZ_ASSERT(!aParentID);
    a11y::DocManager::RemoteDocAdded(doc);
  }
#endif
  return true;
}

PCompositorParent*
ContentParent::AllocPCompositorParent(mozilla::ipc::Transport* aTransport,
                                      base::ProcessId aOtherProcess)
{
    return CompositorParent::Create(aTransport, aOtherProcess);
}

PImageBridgeParent*
ContentParent::AllocPImageBridgeParent(mozilla::ipc::Transport* aTransport,
                                       base::ProcessId aOtherProcess)
{
    return ImageBridgeParent::Create(aTransport, aOtherProcess);
}

PBackgroundParent*
ContentParent::AllocPBackgroundParent(Transport* aTransport,
                                      ProcessId aOtherProcess)
{
    return BackgroundParent::Alloc(this, aTransport, aOtherProcess);
}

PSharedBufferManagerParent*
ContentParent::AllocPSharedBufferManagerParent(mozilla::ipc::Transport* aTransport,
                                                base::ProcessId aOtherProcess)
{
    return SharedBufferManagerParent::Create(aTransport, aOtherProcess);
}

bool
ContentParent::RecvGetProcessAttributes(ContentParentId* aCpId,
                                        bool* aIsForApp, bool* aIsForBrowser)
{
    *aCpId = mChildID;
    *aIsForApp = IsForApp();
    *aIsForBrowser = mIsForBrowser;

    return true;
}

bool
ContentParent::RecvGetXPCOMProcessAttributes(bool* aIsOffline,
                                             InfallibleTArray<nsString>* dictionaries,
                                             ClipboardCapabilities* clipboardCaps)
{
    nsCOMPtr<nsIIOService> io(do_GetIOService());
    MOZ_ASSERT(io, "No IO service?");
    DebugOnly<nsresult> rv = io->GetOffline(aIsOffline);
    MOZ_ASSERT(NS_SUCCEEDED(rv), "Failed getting offline?");

    nsCOMPtr<nsISpellChecker> spellChecker(do_GetService(NS_SPELLCHECKER_CONTRACTID));
    MOZ_ASSERT(spellChecker, "No spell checker?");

    spellChecker->GetDictionaryList(dictionaries);

    nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1"));
    MOZ_ASSERT(clipboard, "No clipboard?");

    rv = clipboard->SupportsSelectionClipboard(&clipboardCaps->supportsSelectionClipboard());
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    rv = clipboard->SupportsFindClipboard(&clipboardCaps->supportsFindClipboard());
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    return true;
}

mozilla::jsipc::PJavaScriptParent *
ContentParent::AllocPJavaScriptParent()
{
    MOZ_ASSERT(!ManagedPJavaScriptParent().Length());
    return nsIContentParent::AllocPJavaScriptParent();
}

bool
ContentParent::DeallocPJavaScriptParent(PJavaScriptParent *parent)
{
    return nsIContentParent::DeallocPJavaScriptParent(parent);
}

PBrowserParent*
ContentParent::AllocPBrowserParent(const TabId& aTabId,
                                   const IPCTabContext& aContext,
                                   const uint32_t& aChromeFlags,
                                   const ContentParentId& aCpId,
                                   const bool& aIsForApp,
                                   const bool& aIsForBrowser)
{
    return nsIContentParent::AllocPBrowserParent(aTabId,
                                                 aContext,
                                                 aChromeFlags,
                                                 aCpId,
                                                 aIsForApp,
                                                 aIsForBrowser);
}

bool
ContentParent::DeallocPBrowserParent(PBrowserParent* frame)
{
    return nsIContentParent::DeallocPBrowserParent(frame);
}

PDeviceStorageRequestParent*
ContentParent::AllocPDeviceStorageRequestParent(const DeviceStorageParams& aParams)
{
    nsRefPtr<DeviceStorageRequestParent> result = new DeviceStorageRequestParent(aParams);
    if (!result->EnsureRequiredPermissions(this)) {
        return nullptr;
    }
    result->Dispatch();
    return result.forget().take();
}

bool
ContentParent::DeallocPDeviceStorageRequestParent(PDeviceStorageRequestParent* doomed)
{
    DeviceStorageRequestParent *parent = static_cast<DeviceStorageRequestParent*>(doomed);
    NS_RELEASE(parent);
    return true;
}

PFileSystemRequestParent*
ContentParent::AllocPFileSystemRequestParent(const FileSystemParams& aParams)
{
    nsRefPtr<FileSystemRequestParent> result = new FileSystemRequestParent();
    if (!result->Dispatch(this, aParams)) {
        return nullptr;
    }
    return result.forget().take();
}

bool
ContentParent::DeallocPFileSystemRequestParent(PFileSystemRequestParent* doomed)
{
    FileSystemRequestParent* parent = static_cast<FileSystemRequestParent*>(doomed);
    NS_RELEASE(parent);
    return true;
}

PBlobParent*
ContentParent::AllocPBlobParent(const BlobConstructorParams& aParams)
{
    return nsIContentParent::AllocPBlobParent(aParams);
}

bool
ContentParent::DeallocPBlobParent(PBlobParent* aActor)
{
    return nsIContentParent::DeallocPBlobParent(aActor);
}

mozilla::PRemoteSpellcheckEngineParent *
ContentParent::AllocPRemoteSpellcheckEngineParent()
{
    mozilla::RemoteSpellcheckEngineParent *parent = new mozilla::RemoteSpellcheckEngineParent();
    return parent;
}

bool
ContentParent::DeallocPRemoteSpellcheckEngineParent(PRemoteSpellcheckEngineParent *parent)
{
    delete parent;
    return true;
}

 void
ContentParent::ForceKillTimerCallback(nsITimer* aTimer, void* aClosure)
{
    auto self = static_cast<ContentParent*>(aClosure);
    self->KillHard();
}

void
ContentParent::KillHard()
{
    
    
    
    if (mCalledKillHard) {
        return;
    }
    mCalledKillHard = true;
    mForceKillTimer = nullptr;

#if defined(MOZ_CRASHREPORTER) && !defined(MOZ_B2G)
    if (ManagedPCrashReporterParent().Length() > 0) {
        CrashReporterParent* crashReporter =
            static_cast<CrashReporterParent*>(ManagedPCrashReporterParent()[0]);

        
        
        
        
        
        
        if (crashReporter->GeneratePairedMinidump(this)) {
            mCreatedPairedMinidumps = true;
            
            
            
            
            
            nsAutoCString additionalDumps("browser");
            crashReporter->AnnotateCrashReport(
                NS_LITERAL_CSTRING("additional_minidumps"),
                additionalDumps);
        }
    }
#endif
    if (!KillProcess(OtherProcess(), 1, false)) {
        NS_WARNING("failed to kill subprocess!");
    }
    if (mSubprocess) {
        mSubprocess->SetAlreadyDead();
    }
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(&ProcessWatcher::EnsureProcessTerminated,
                            OtherProcess(), true));
    
    
    SetOtherProcess(0);
}

bool
ContentParent::IsPreallocated()
{
    return mAppManifestURL == MAGIC_PREALLOCATED_APP_MANIFEST_URL;
}

void
ContentParent::FriendlyName(nsAString& aName, bool aAnonymize)
{
    aName.Truncate();
#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        aName.AssignLiteral("(Nuwa)");
    } else
#endif
    if (IsPreallocated()) {
        aName.AssignLiteral("(Preallocated)");
    } else if (mIsForBrowser) {
        aName.AssignLiteral("Browser");
    } else if (aAnonymize) {
        aName.AssignLiteral("<anonymized-name>");
    } else if (!mAppName.IsEmpty()) {
        aName = mAppName;
    } else if (!mAppManifestURL.IsEmpty()) {
        aName.AssignLiteral("Unknown app: ");
        aName.Append(mAppManifestURL);
    } else {
        aName.AssignLiteral("???");
    }
}

PCrashReporterParent*
ContentParent::AllocPCrashReporterParent(const NativeThreadId& tid,
                                         const uint32_t& processType)
{
#ifdef MOZ_CRASHREPORTER
    return new CrashReporterParent();
#else
    return nullptr;
#endif
}

bool
ContentParent::RecvPCrashReporterConstructor(PCrashReporterParent* actor,
                                             const NativeThreadId& tid,
                                             const uint32_t& processType)
{
    static_cast<CrashReporterParent*>(actor)->SetChildData(tid, processType);
    return true;
}

bool
ContentParent::DeallocPCrashReporterParent(PCrashReporterParent* crashreporter)
{
    delete crashreporter;
    return true;
}

hal_sandbox::PHalParent*
ContentParent::AllocPHalParent()
{
    return hal_sandbox::CreateHalParent();
}

bool
ContentParent::DeallocPHalParent(hal_sandbox::PHalParent* aHal)
{
    delete aHal;
    return true;
}

PMemoryReportRequestParent*
ContentParent::AllocPMemoryReportRequestParent(const uint32_t& aGeneration,
                                               const bool &aAnonymize,
                                               const bool &aMinimizeMemoryUsage,
                                               const MaybeFileDesc &aDMDFile)
{
    MemoryReportRequestParent* parent = new MemoryReportRequestParent();
    return parent;
}

bool
ContentParent::DeallocPMemoryReportRequestParent(PMemoryReportRequestParent* actor)
{
    delete actor;
    return true;
}

PCycleCollectWithLogsParent*
ContentParent::AllocPCycleCollectWithLogsParent(const bool& aDumpAllTraces,
                                                const FileDescriptor& aGCLog,
                                                const FileDescriptor& aCCLog)
{
    MOZ_CRASH("Don't call this; use ContentParent::CycleCollectWithLogs");
}

bool
ContentParent::DeallocPCycleCollectWithLogsParent(PCycleCollectWithLogsParent* aActor)
{
    delete aActor;
    return true;
}

bool
ContentParent::CycleCollectWithLogs(bool aDumpAllTraces,
                                    nsICycleCollectorLogSink* aSink,
                                    nsIDumpGCAndCCLogsCallback* aCallback)
{
    return CycleCollectWithLogsParent::AllocAndSendConstructor(this,
                                                               aDumpAllTraces,
                                                               aSink,
                                                               aCallback);
}

PTestShellParent*
ContentParent::AllocPTestShellParent()
{
    return new TestShellParent();
}

bool
ContentParent::DeallocPTestShellParent(PTestShellParent* shell)
{
    delete shell;
    return true;
}

PMobileConnectionParent*
ContentParent::AllocPMobileConnectionParent(const uint32_t& aClientId)
{
#ifdef MOZ_B2G_RIL
    nsRefPtr<MobileConnectionParent> parent = new MobileConnectionParent(aClientId);
    
    parent->AddRef();

    return parent;
#else
    MOZ_CRASH("No support for mobileconnection on this platform!");
#endif
}

bool
ContentParent::DeallocPMobileConnectionParent(PMobileConnectionParent* aActor)
{
#ifdef MOZ_B2G_RIL
    
    static_cast<MobileConnectionParent*>(aActor)->Release();
    return true;
#else
    MOZ_CRASH("No support for mobileconnection on this platform!");
#endif
}

PNeckoParent*
ContentParent::AllocPNeckoParent()
{
    return new NeckoParent();
}

bool
ContentParent::DeallocPNeckoParent(PNeckoParent* necko)
{
    delete necko;
    return true;
}

PPrintingParent*
ContentParent::AllocPPrintingParent()
{
#ifdef NS_PRINTING
    return new PrintingParent();
#else
    return nullptr;
#endif
}

bool
ContentParent::RecvPPrintingConstructor(PPrintingParent* aActor)
{
    return true;
}

bool
ContentParent::DeallocPPrintingParent(PPrintingParent* printing)
{
    delete printing;
    return true;
}

PScreenManagerParent*
ContentParent::AllocPScreenManagerParent(uint32_t* aNumberOfScreens,
                                         float* aSystemDefaultScale,
                                         bool* aSuccess)
{
    return new ScreenManagerParent(aNumberOfScreens, aSystemDefaultScale, aSuccess);
}

bool
ContentParent::DeallocPScreenManagerParent(PScreenManagerParent* aActor)
{
    delete aActor;
    return true;
}

PExternalHelperAppParent*
ContentParent::AllocPExternalHelperAppParent(const OptionalURIParams& uri,
                                             const nsCString& aMimeContentType,
                                             const nsCString& aContentDisposition,
                                             const uint32_t& aContentDispositionHint,
                                             const nsString& aContentDispositionFilename,
                                             const bool& aForceSave,
                                             const int64_t& aContentLength,
                                             const OptionalURIParams& aReferrer,
                                             PBrowserParent* aBrowser)
{
    ExternalHelperAppParent *parent = new ExternalHelperAppParent(uri, aContentLength);
    parent->AddRef();
    parent->Init(this,
                 aMimeContentType,
                 aContentDisposition,
                 aContentDispositionHint,
                 aContentDispositionFilename,
                 aForceSave,
                 aReferrer,
                 aBrowser);
    return parent;
}

bool
ContentParent::DeallocPExternalHelperAppParent(PExternalHelperAppParent* aService)
{
    ExternalHelperAppParent *parent = static_cast<ExternalHelperAppParent *>(aService);
    parent->Release();
    return true;
}

PCellBroadcastParent*
ContentParent::AllocPCellBroadcastParent()
{
    if (!AssertAppProcessPermission(this, "cellbroadcast")) {
        return nullptr;
    }

    CellBroadcastParent* actor = new CellBroadcastParent();
    actor->AddRef();
    return actor;
}

bool
ContentParent::DeallocPCellBroadcastParent(PCellBroadcastParent* aActor)
{
    static_cast<CellBroadcastParent*>(aActor)->Release();
    return true;
}

bool
ContentParent::RecvPCellBroadcastConstructor(PCellBroadcastParent* aActor)
{
    return static_cast<CellBroadcastParent*>(aActor)->Init();
}

PSmsParent*
ContentParent::AllocPSmsParent()
{
    if (!AssertAppProcessPermission(this, "sms")) {
        return nullptr;
    }

    SmsParent* parent = new SmsParent();
    parent->AddRef();
    return parent;
}

bool
ContentParent::DeallocPSmsParent(PSmsParent* aSms)
{
    static_cast<SmsParent*>(aSms)->Release();
    return true;
}

PTelephonyParent*
ContentParent::AllocPTelephonyParent()
{
    if (!AssertAppProcessPermission(this, "telephony")) {
        return nullptr;
    }

    TelephonyParent* actor = new TelephonyParent();
    NS_ADDREF(actor);
    return actor;
}

bool
ContentParent::DeallocPTelephonyParent(PTelephonyParent* aActor)
{
    static_cast<TelephonyParent*>(aActor)->Release();
    return true;
}

PVoicemailParent*
ContentParent::AllocPVoicemailParent()
{
    if (!AssertAppProcessPermission(this, "voicemail")) {
        return nullptr;
    }

    VoicemailParent* actor = new VoicemailParent();
    actor->AddRef();
    return actor;
}

bool
ContentParent::RecvPVoicemailConstructor(PVoicemailParent* aActor)
{
    return static_cast<VoicemailParent*>(aActor)->Init();
}

bool
ContentParent::DeallocPVoicemailParent(PVoicemailParent* aActor)
{
    static_cast<VoicemailParent*>(aActor)->Release();
    return true;
}

PStorageParent*
ContentParent::AllocPStorageParent()
{
    return new DOMStorageDBParent();
}

bool
ContentParent::DeallocPStorageParent(PStorageParent* aActor)
{
    DOMStorageDBParent* child = static_cast<DOMStorageDBParent*>(aActor);
    child->ReleaseIPDLReference();
    return true;
}

PBluetoothParent*
ContentParent::AllocPBluetoothParent()
{
#ifdef MOZ_B2G_BT
    if (!AssertAppProcessPermission(this, "bluetooth")) {
        return nullptr;
    }
    return new mozilla::dom::bluetooth::BluetoothParent();
#else
    MOZ_CRASH("No support for bluetooth on this platform!");
#endif
}

bool
ContentParent::DeallocPBluetoothParent(PBluetoothParent* aActor)
{
#ifdef MOZ_B2G_BT
    delete aActor;
    return true;
#else
    MOZ_CRASH("No support for bluetooth on this platform!");
#endif
}

bool
ContentParent::RecvPBluetoothConstructor(PBluetoothParent* aActor)
{
#ifdef MOZ_B2G_BT
    nsRefPtr<BluetoothService> btService = BluetoothService::Get();
    NS_ENSURE_TRUE(btService, false);

    return static_cast<BluetoothParent*>(aActor)->InitWithService(btService);
#else
    MOZ_CRASH("No support for bluetooth on this platform!");
#endif
}

PFMRadioParent*
ContentParent::AllocPFMRadioParent()
{
#ifdef MOZ_B2G_FM
    if (!AssertAppProcessPermission(this, "fmradio")) {
        return nullptr;
    }
    return new FMRadioParent();
#else
    NS_WARNING("No support for FMRadio on this platform!");
    return nullptr;
#endif
}

bool
ContentParent::DeallocPFMRadioParent(PFMRadioParent* aActor)
{
#ifdef MOZ_B2G_FM
    delete aActor;
    return true;
#else
    NS_WARNING("No support for FMRadio on this platform!");
    return false;
#endif
}

asmjscache::PAsmJSCacheEntryParent*
ContentParent::AllocPAsmJSCacheEntryParent(
                                    const asmjscache::OpenMode& aOpenMode,
                                    const asmjscache::WriteParams& aWriteParams,
                                    const IPC::Principal& aPrincipal)
{
    return asmjscache::AllocEntryParent(aOpenMode, aWriteParams, aPrincipal);
}

bool
ContentParent::DeallocPAsmJSCacheEntryParent(PAsmJSCacheEntryParent* aActor)
{
    asmjscache::DeallocEntryParent(aActor);
    return true;
}

PSpeechSynthesisParent*
ContentParent::AllocPSpeechSynthesisParent()
{
#ifdef MOZ_WEBSPEECH
    return new mozilla::dom::SpeechSynthesisParent();
#else
    return nullptr;
#endif
}

bool
ContentParent::DeallocPSpeechSynthesisParent(PSpeechSynthesisParent* aActor)
{
#ifdef MOZ_WEBSPEECH
    delete aActor;
    return true;
#else
    return false;
#endif
}

bool
ContentParent::RecvPSpeechSynthesisConstructor(PSpeechSynthesisParent* aActor)
{
#ifdef MOZ_WEBSPEECH
    return true;
#else
    return false;
#endif
}

bool
ContentParent::RecvSpeakerManagerGetSpeakerStatus(bool* aValue)
{
#ifdef MOZ_WIDGET_GONK
    *aValue = false;
    nsRefPtr<SpeakerManagerService> service =
      SpeakerManagerService::GetOrCreateSpeakerManagerService();
    MOZ_ASSERT(service);

    *aValue = service->GetSpeakerStatus();
    return true;
#endif
    return false;
}

bool
ContentParent::RecvSpeakerManagerForceSpeaker(const bool& aEnable)
{
#ifdef MOZ_WIDGET_GONK
    nsRefPtr<SpeakerManagerService> service =
      SpeakerManagerService::GetOrCreateSpeakerManagerService();
    MOZ_ASSERT(service);
    service->ForceSpeaker(aEnable, mChildID);

    return true;
#endif
    return false;
}

bool
ContentParent::RecvStartVisitedQuery(const URIParams& aURI)
{
    nsCOMPtr<nsIURI> newURI = DeserializeURI(aURI);
    if (!newURI) {
        return false;
    }
    nsCOMPtr<IHistory> history = services::GetHistoryService();
    if (history) {
        history->RegisterVisitedCallback(newURI, nullptr);
    }
    return true;
}


bool
ContentParent::RecvVisitURI(const URIParams& uri,
                            const OptionalURIParams& referrer,
                            const uint32_t& flags)
{
    nsCOMPtr<nsIURI> ourURI = DeserializeURI(uri);
    if (!ourURI) {
        return false;
    }
    nsCOMPtr<nsIURI> ourReferrer = DeserializeURI(referrer);
    nsCOMPtr<IHistory> history = services::GetHistoryService();
    if (history) {
        history->VisitURI(ourURI, ourReferrer, flags);
    }
    return true;
}


bool
ContentParent::RecvSetURITitle(const URIParams& uri,
                               const nsString& title)
{
    nsCOMPtr<nsIURI> ourURI = DeserializeURI(uri);
    if (!ourURI) {
        return false;
    }
    nsCOMPtr<IHistory> history = services::GetHistoryService();
    if (history) {
        history->SetURITitle(ourURI, title);
    }
    return true;
}

bool
ContentParent::RecvGetRandomValues(const uint32_t& length,
                                   InfallibleTArray<uint8_t>* randomValues)
{
    uint8_t* buf = Crypto::GetRandomValues(length);
    if (!buf) {
        return true;
    }

    randomValues->SetCapacity(length);
    randomValues->SetLength(length);

    memcpy(randomValues->Elements(), buf, length);

    NS_Free(buf);

    return true;
}

bool
ContentParent::RecvGetSystemMemory(const uint64_t& aGetterId)
{
    uint32_t memoryTotal = 0;

#if defined(XP_LINUX)
    memoryTotal = mozilla::hal::GetTotalSystemMemoryLevel();
#endif

    unused << SendSystemMemoryAvailable(aGetterId, memoryTotal);

    return true;
}

bool
ContentParent::RecvIsSecureURI(const uint32_t& type,
                               const URIParams& uri,
                               const uint32_t& flags,
                               bool* isSecureURI)
{
    nsCOMPtr<nsISiteSecurityService> sss(do_GetService(NS_SSSERVICE_CONTRACTID));
    if (!sss) {
        return false;
    }
    nsCOMPtr<nsIURI> ourURI = DeserializeURI(uri);
    if (!ourURI) {
        return false;
    }
    nsresult rv = sss->IsSecureURI(type, ourURI, flags, isSecureURI);
    return NS_SUCCEEDED(rv);
}

bool
ContentParent::RecvLoadURIExternal(const URIParams& uri)
{
    nsCOMPtr<nsIExternalProtocolService> extProtService(do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID));
    if (!extProtService) {
        return true;
    }
    nsCOMPtr<nsIURI> ourURI = DeserializeURI(uri);
    if (!ourURI) {
        return false;
    }
    extProtService->LoadURI(ourURI, nullptr);
    return true;
}

bool
ContentParent::RecvShowAlertNotification(const nsString& aImageUrl, const nsString& aTitle,
                                         const nsString& aText, const bool& aTextClickable,
                                         const nsString& aCookie, const nsString& aName,
                                         const nsString& aBidi, const nsString& aLang,
                                         const nsString& aData,
                                         const IPC::Principal& aPrincipal,
                                         const bool& aInPrivateBrowsing)
{
#ifdef MOZ_CHILD_PERMISSIONS
    uint32_t permission = mozilla::CheckPermission(this, aPrincipal,
                                                   "desktop-notification");
    if (permission != nsIPermissionManager::ALLOW_ACTION) {
        return true;
    }
#endif 

    nsCOMPtr<nsIAlertsService> sysAlerts(do_GetService(NS_ALERTSERVICE_CONTRACTID));
    if (sysAlerts) {
        sysAlerts->ShowAlertNotification(aImageUrl, aTitle, aText, aTextClickable,
                                         aCookie, this, aName, aBidi, aLang,
                                         aData, aPrincipal, aInPrivateBrowsing);
    }
    return true;
}

bool
ContentParent::RecvCloseAlert(const nsString& aName,
                              const IPC::Principal& aPrincipal)
{
#ifdef MOZ_CHILD_PERMISSIONS
    uint32_t permission = mozilla::CheckPermission(this, aPrincipal,
                                                   "desktop-notification");
    if (permission != nsIPermissionManager::ALLOW_ACTION) {
        return true;
    }
#endif

    nsCOMPtr<nsIAlertsService> sysAlerts(do_GetService(NS_ALERTSERVICE_CONTRACTID));
    if (sysAlerts) {
        sysAlerts->CloseAlert(aName, aPrincipal);
    }

    return true;
}

bool
ContentParent::RecvSyncMessage(const nsString& aMsg,
                               const ClonedMessageData& aData,
                               const InfallibleTArray<CpowEntry>& aCpows,
                               const IPC::Principal& aPrincipal,
                               InfallibleTArray<nsString>* aRetvals)
{
    return nsIContentParent::RecvSyncMessage(aMsg, aData, aCpows, aPrincipal,
                                             aRetvals);
}

bool
ContentParent::RecvRpcMessage(const nsString& aMsg,
                              const ClonedMessageData& aData,
                              const InfallibleTArray<CpowEntry>& aCpows,
                              const IPC::Principal& aPrincipal,
                              InfallibleTArray<nsString>* aRetvals)
{
    return nsIContentParent::RecvRpcMessage(aMsg, aData, aCpows, aPrincipal,
                                            aRetvals);
}

bool
ContentParent::RecvAsyncMessage(const nsString& aMsg,
                                const ClonedMessageData& aData,
                                const InfallibleTArray<CpowEntry>& aCpows,
                                const IPC::Principal& aPrincipal)
{
    return nsIContentParent::RecvAsyncMessage(aMsg, aData, aCpows, aPrincipal);
}

bool
ContentParent::RecvFilePathUpdateNotify(const nsString& aType,
                                        const nsString& aStorageName,
                                        const nsString& aFilePath,
                                        const nsCString& aReason)
{
    nsRefPtr<DeviceStorageFile> dsf = new DeviceStorageFile(aType,
                                                            aStorageName,
                                                            aFilePath);

    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (!obs) {
        return false;
    }
    obs->NotifyObservers(dsf, "file-watcher-update",
                         NS_ConvertASCIItoUTF16(aReason).get());
    return true;
}

static int32_t
AddGeolocationListener(nsIDOMGeoPositionCallback* watcher, nsIDOMGeoPositionErrorCallback* errorCallBack, bool highAccuracy)
{
    nsCOMPtr<nsIDOMGeoGeolocation> geo = do_GetService("@mozilla.org/geolocation;1");
    if (!geo) {
        return -1;
    }

    PositionOptions* options = new PositionOptions();
    options->mTimeout = 0;
    options->mMaximumAge = 0;
    options->mEnableHighAccuracy = highAccuracy;
    int32_t retval = 1;
    geo->WatchPosition(watcher, errorCallBack, options, &retval);
    return retval;
}

bool
ContentParent::RecvAddGeolocationListener(const IPC::Principal& aPrincipal,
                                          const bool& aHighAccuracy)
{
#ifdef MOZ_CHILD_PERMISSIONS
    if (!ContentParent::IgnoreIPCPrincipal()) {
        uint32_t permission = mozilla::CheckPermission(this, aPrincipal,
                                                       "geolocation");
        if (permission != nsIPermissionManager::ALLOW_ACTION) {
            return true;
        }
    }
#endif 

    
    
    RecvRemoveGeolocationListener();
    mGeolocationWatchID = AddGeolocationListener(this, this, aHighAccuracy);

    
    nsAutoCString origin;
    
    nsCOMPtr<nsIPrincipal> principal = static_cast<nsIPrincipal*>(aPrincipal);
    if (!principal) {
      return true;
    }
    principal->GetOrigin(getter_Copies(origin));
    nsRefPtr<nsGeolocationSettings> gs = nsGeolocationSettings::GetGeolocationSettings();
    if (gs) {
      gs->PutWatchOrigin(mGeolocationWatchID, origin);
    }
    return true;
}

bool
ContentParent::RecvRemoveGeolocationListener()
{
    if (mGeolocationWatchID != -1) {
        nsCOMPtr<nsIDOMGeoGeolocation> geo = do_GetService("@mozilla.org/geolocation;1");
        if (!geo) {
            return true;
        }
        geo->ClearWatch(mGeolocationWatchID);

        nsRefPtr<nsGeolocationSettings> gs = nsGeolocationSettings::GetGeolocationSettings();
        if (gs) {
          gs->RemoveWatchOrigin(mGeolocationWatchID);
        }
        mGeolocationWatchID = -1;
    }
    return true;
}

bool
ContentParent::RecvSetGeolocationHigherAccuracy(const bool& aEnable)
{
    
    
    if (mGeolocationWatchID != -1) {
        nsCString origin;
        nsRefPtr<nsGeolocationSettings> gs = nsGeolocationSettings::GetGeolocationSettings();
        
        if (gs) {
          gs->GetWatchOrigin(mGeolocationWatchID, origin);
        }

        
        RecvRemoveGeolocationListener();
        mGeolocationWatchID = AddGeolocationListener(this, this, aEnable);

        
        if (gs) {
          gs->PutWatchOrigin(mGeolocationWatchID, origin);
        }
    }
    return true;
}

NS_IMETHODIMP
ContentParent::HandleEvent(nsIDOMGeoPosition* postion)
{
    unused << SendGeolocationUpdate(GeoPosition(postion));
    return NS_OK;
}

NS_IMETHODIMP
ContentParent::HandleEvent(nsIDOMGeoPositionError* postionError)
{
    int16_t errorCode;
    nsresult rv;
    rv = postionError->GetCode(&errorCode);
    NS_ENSURE_SUCCESS(rv,rv);
    unused << SendGeolocationError(errorCode);
    return NS_OK;
}

nsConsoleService *
ContentParent::GetConsoleService()
{
    if (mConsoleService) {
        return mConsoleService.get();
    }

    
    
    
    
    NS_DEFINE_CID(consoleServiceCID, NS_CONSOLESERVICE_CID);
    nsCOMPtr<nsConsoleService>  consoleService(do_GetService(consoleServiceCID));
    mConsoleService = consoleService;
    return mConsoleService.get();
}

bool
ContentParent::RecvConsoleMessage(const nsString& aMessage)
{
    nsRefPtr<nsConsoleService> consoleService = GetConsoleService();
    if (!consoleService) {
        return true;
    }

    nsRefPtr<nsConsoleMessage> msg(new nsConsoleMessage(aMessage.get()));
    consoleService->LogMessageWithMode(msg, nsConsoleService::SuppressLog);
    return true;
}

bool
ContentParent::RecvScriptError(const nsString& aMessage,
                                      const nsString& aSourceName,
                                      const nsString& aSourceLine,
                                      const uint32_t& aLineNumber,
                                      const uint32_t& aColNumber,
                                      const uint32_t& aFlags,
                                      const nsCString& aCategory)
{
    nsRefPtr<nsConsoleService> consoleService = GetConsoleService();
    if (!consoleService) {
        return true;
    }

    nsCOMPtr<nsIScriptError> msg(do_CreateInstance(NS_SCRIPTERROR_CONTRACTID));
    nsresult rv = msg->Init(aMessage, aSourceName, aSourceLine,
                            aLineNumber, aColNumber, aFlags, aCategory.get());
    if (NS_FAILED(rv))
        return true;

    consoleService->LogMessageWithMode(msg, nsConsoleService::SuppressLog);
    return true;
}

bool
ContentParent::RecvPrivateDocShellsExist(const bool& aExist)
{
    if (!sPrivateContent)
        sPrivateContent = new nsTArray<ContentParent*>();
    if (aExist) {
        sPrivateContent->AppendElement(this);
    } else {
        sPrivateContent->RemoveElement(this);
        if (!sPrivateContent->Length()) {
            nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
            obs->NotifyObservers(nullptr, "last-pb-context-exited", nullptr);
            delete sPrivateContent;
            sPrivateContent = nullptr;
        }
    }
    return true;
}

bool
ContentParent::DoSendAsyncMessage(JSContext* aCx,
                                  const nsAString& aMessage,
                                  const mozilla::dom::StructuredCloneData& aData,
                                  JS::Handle<JSObject *> aCpows,
                                  nsIPrincipal* aPrincipal)
{
    ClonedMessageData data;
    if (!BuildClonedMessageDataForParent(this, aData, data)) {
        return false;
    }
    InfallibleTArray<CpowEntry> cpows;
    if (aCpows && !GetCPOWManager()->Wrap(aCx, aCpows, &cpows)) {
        return false;
    }
#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess() && IsNuwaReady()) {
        
        return true;
    }
#endif
    return SendAsyncMessage(nsString(aMessage), data, cpows, Principal(aPrincipal));
}

bool
ContentParent::CheckPermission(const nsAString& aPermission)
{
    return AssertAppProcessPermission(this, NS_ConvertUTF16toUTF8(aPermission).get());
}

bool
ContentParent::CheckManifestURL(const nsAString& aManifestURL)
{
    return AssertAppProcessManifestURL(this, NS_ConvertUTF16toUTF8(aManifestURL).get());
}

bool
ContentParent::CheckAppHasPermission(const nsAString& aPermission)
{
    return AssertAppHasPermission(this, NS_ConvertUTF16toUTF8(aPermission).get());
}

bool
ContentParent::CheckAppHasStatus(unsigned short aStatus)
{
    return AssertAppHasStatus(this, aStatus);
}

bool
ContentParent::KillChild()
{
  KillHard();
  return true;
}

PBlobParent*
ContentParent::SendPBlobConstructor(PBlobParent* aActor,
                                    const BlobConstructorParams& aParams)
{
    return PContentParent::SendPBlobConstructor(aActor, aParams);
}

bool
ContentParent::RecvSystemMessageHandled()
{
    SystemMessageHandledListener::OnSystemMessageHandled();
    return true;
}

PBrowserParent*
ContentParent::SendPBrowserConstructor(PBrowserParent* aActor,
                                       const TabId& aTabId,
                                       const IPCTabContext& aContext,
                                       const uint32_t& aChromeFlags,
                                       const ContentParentId& aCpId,
                                       const bool& aIsForApp,
                                       const bool& aIsForBrowser)
{
    return PContentParent::SendPBrowserConstructor(aActor,
                                                   aTabId,
                                                   aContext,
                                                   aChromeFlags,
                                                   aCpId,
                                                   aIsForApp,
                                                   aIsForBrowser);
}

bool
ContentParent::RecvCreateFakeVolume(const nsString& fsName, const nsString& mountPoint)
{
#ifdef MOZ_WIDGET_GONK
    nsresult rv;
    nsCOMPtr<nsIVolumeService> vs = do_GetService(NS_VOLUMESERVICE_CONTRACTID, &rv);
    if (vs) {
        vs->CreateFakeVolume(fsName, mountPoint);
    }
    return true;
#else
    NS_WARNING("ContentParent::RecvCreateFakeVolume shouldn't be called when MOZ_WIDGET_GONK is not defined");
    return false;
#endif
}

bool
ContentParent::RecvSetFakeVolumeState(const nsString& fsName, const int32_t& fsState)
{
#ifdef MOZ_WIDGET_GONK
    nsresult rv;
    nsCOMPtr<nsIVolumeService> vs = do_GetService(NS_VOLUMESERVICE_CONTRACTID, &rv);
    if (vs) {
        vs->SetFakeVolumeState(fsName, fsState);
    }
    return true;
#else
    NS_WARNING("ContentParent::RecvSetFakeVolumeState shouldn't be called when MOZ_WIDGET_GONK is not defined");
    return false;
#endif
}

bool
ContentParent::RecvKeywordToURI(const nsCString& aKeyword,
                                nsString* aProviderName,
                                OptionalInputStreamParams* aPostData,
                                OptionalURIParams* aURI)
{
    nsCOMPtr<nsIURIFixup> fixup = do_GetService(NS_URIFIXUP_CONTRACTID);
    if (!fixup) {
        return true;
    }

    nsCOMPtr<nsIInputStream> postData;
    nsCOMPtr<nsIURIFixupInfo> info;

    if (NS_FAILED(fixup->KeywordToURI(aKeyword, getter_AddRefs(postData),
                                      getter_AddRefs(info)))) {
        return true;
    }
    info->GetKeywordProviderName(*aProviderName);

    nsTArray<mozilla::ipc::FileDescriptor> fds;
    SerializeInputStream(postData, *aPostData, fds);
    MOZ_ASSERT(fds.IsEmpty());

    nsCOMPtr<nsIURI> uri;
    info->GetPreferredURI(getter_AddRefs(uri));
    SerializeURI(uri, *aURI);
    return true;
}

bool
ContentParent::RecvNotifyKeywordSearchLoading(const nsString &aProvider,
                                              const nsString &aKeyword) {
#ifdef MOZ_TOOLKIT_SEARCH
    nsCOMPtr<nsIBrowserSearchService> searchSvc = do_GetService("@mozilla.org/browser/search-service;1");
    if (searchSvc) {
        nsCOMPtr<nsISearchEngine> searchEngine;
        searchSvc->GetEngineByName(aProvider, getter_AddRefs(searchEngine));
        if (searchEngine) {
            nsCOMPtr<nsIObserverService> obsSvc = mozilla::services::GetObserverService();
            if (obsSvc) {
                
                
                obsSvc->NotifyObservers(searchEngine, "keyword-search", aKeyword.get());
            }
        }
    }
#endif
    return true;
}

bool
ContentParent::ShouldContinueFromReplyTimeout()
{
    return false;
}

bool
ContentParent::RecvRecordingDeviceEvents(const nsString& aRecordingStatus,
                                         const nsString& aPageURL,
                                         const bool& aIsAudio,
                                         const bool& aIsVideo)
{
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        
        nsRefPtr<nsHashPropertyBag> props = new nsHashPropertyBag();
        props->SetPropertyAsUint64(NS_LITERAL_STRING("childID"), ChildID());
        props->SetPropertyAsBool(NS_LITERAL_STRING("isApp"), IsForApp());
        props->SetPropertyAsBool(NS_LITERAL_STRING("isAudio"), aIsAudio);
        props->SetPropertyAsBool(NS_LITERAL_STRING("isVideo"), aIsVideo);

        nsString requestURL = IsForApp() ? AppManifestURL() : aPageURL;
        props->SetPropertyAsAString(NS_LITERAL_STRING("requestURL"), requestURL);

        obs->NotifyObservers((nsIPropertyBag2*) props,
                             "recording-device-ipc-events",
                             aRecordingStatus.get());
    } else {
        NS_WARNING("Could not get the Observer service for ContentParent::RecvRecordingDeviceEvents.");
    }
    return true;
}

bool
ContentParent::RecvGetGraphicsFeatureStatus(const int32_t& aFeature,
                                            int32_t* aStatus,
                                            bool* aSuccess)
{
    nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
    if (!gfxInfo) {
        *aSuccess = false;
        return true;
    }

    *aSuccess = NS_SUCCEEDED(gfxInfo->GetFeatureStatus(aFeature, aStatus));
    return true;
}

bool
ContentParent::RecvAddIdleObserver(const uint64_t& aObserver, const uint32_t& aIdleTimeInS)
{
    nsresult rv;
    nsCOMPtr<nsIIdleService> idleService =
      do_GetService("@mozilla.org/widget/idleservice;1", &rv);
    NS_ENSURE_SUCCESS(rv, false);

    nsRefPtr<ParentIdleListener> listener = new ParentIdleListener(this, aObserver);
    mIdleListeners.Put(aObserver, listener);
    idleService->AddIdleObserver(listener, aIdleTimeInS);
    return true;
}

bool
ContentParent::RecvRemoveIdleObserver(const uint64_t& aObserver, const uint32_t& aIdleTimeInS)
{
    nsresult rv;
    nsCOMPtr<nsIIdleService> idleService =
      do_GetService("@mozilla.org/widget/idleservice;1", &rv);
    NS_ENSURE_SUCCESS(rv, false);

    nsRefPtr<ParentIdleListener> listener;
    bool found = mIdleListeners.Get(aObserver, &listener);
    if (found) {
        mIdleListeners.Remove(aObserver);
        idleService->RemoveIdleObserver(listener, aIdleTimeInS);
    }

    return true;
}

bool
ContentParent::RecvBackUpXResources(const FileDescriptor& aXSocketFd)
{
#ifndef MOZ_X11
    NS_RUNTIMEABORT("This message only makes sense on X11 platforms");
#else
    NS_ABORT_IF_FALSE(0 > mChildXSocketFdDup.get(),
                      "Already backed up X resources??");
    mChildXSocketFdDup.forget();
    if (aXSocketFd.IsValid()) {
        mChildXSocketFdDup.reset(aXSocketFd.PlatformHandle());
    }
#endif
    return true;
}

bool
ContentParent::RecvOpenAnonymousTemporaryFile(FileDescriptor *aFD)
{
    PRFileDesc *prfd;
    nsresult rv = NS_OpenAnonymousTemporaryFile(&prfd);
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return false;
    }
    *aFD = FileDescriptor(FileDescriptor::PlatformHandleType(PR_FileDesc2NativeHandle(prfd)));
    
    
    PR_Close(prfd);
    return true;
}

static NS_DEFINE_CID(kFormProcessorCID, NS_FORMPROCESSOR_CID);

bool
ContentParent::RecvKeygenProcessValue(const nsString& oldValue,
                                      const nsString& challenge,
                                      const nsString& keytype,
                                      const nsString& keyparams,
                                      nsString* newValue)
{
    nsCOMPtr<nsIFormProcessor> formProcessor =
      do_GetService(kFormProcessorCID);
    if (!formProcessor) {
        newValue->Truncate();
        return true;
    }

    formProcessor->ProcessValueIPC(oldValue, challenge, keytype, keyparams,
                                   *newValue);
    return true;
}

bool
ContentParent::RecvKeygenProvideContent(nsString* aAttribute,
                                        nsTArray<nsString>* aContent)
{
    nsCOMPtr<nsIFormProcessor> formProcessor =
      do_GetService(kFormProcessorCID);
    if (!formProcessor) {
        return true;
    }

    formProcessor->ProvideContent(NS_LITERAL_STRING("SELECT"), *aContent,
                                  *aAttribute);
    return true;
}

PFileDescriptorSetParent*
ContentParent::AllocPFileDescriptorSetParent(const FileDescriptor& aFD)
{
    return new FileDescriptorSetParent(aFD);
}

bool
ContentParent::DeallocPFileDescriptorSetParent(PFileDescriptorSetParent* aActor)
{
    delete static_cast<FileDescriptorSetParent*>(aActor);
    return true;
}

bool
ContentParent::RecvGetFileReferences(const PersistenceType& aPersistenceType,
                                     const nsCString& aOrigin,
                                     const nsString& aDatabaseName,
                                     const int64_t& aFileId,
                                     int32_t* aRefCnt,
                                     int32_t* aDBRefCnt,
                                     int32_t* aSliceRefCnt,
                                     bool* aResult)
{
    MOZ_ASSERT(aRefCnt);
    MOZ_ASSERT(aDBRefCnt);
    MOZ_ASSERT(aSliceRefCnt);
    MOZ_ASSERT(aResult);

    if (NS_WARN_IF(aPersistenceType != quota::PERSISTENCE_TYPE_PERSISTENT &&
                   aPersistenceType != quota::PERSISTENCE_TYPE_TEMPORARY &&
                   aPersistenceType != quota::PERSISTENCE_TYPE_DEFAULT)) {
        return false;
    }

    if (NS_WARN_IF(aOrigin.IsEmpty())) {
        return false;
    }

    if (NS_WARN_IF(aDatabaseName.IsEmpty())) {
        return false;
    }

    if (NS_WARN_IF(aFileId < 1)) {
        return false;
    }

    nsRefPtr<IndexedDatabaseManager> mgr = IndexedDatabaseManager::Get();
    if (NS_WARN_IF(!mgr)) {
        return false;
    }

    if (NS_WARN_IF(!mgr->IsMainProcess())) {
        return false;
    }

    nsresult rv =
        mgr->BlockAndGetFileReferences(aPersistenceType,
                                       aOrigin,
                                       aDatabaseName,
                                       aFileId,
                                       aRefCnt,
                                       aDBRefCnt,
                                       aSliceRefCnt,
                                       aResult);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return false;
    }

    return true;
}

bool
ContentParent::IgnoreIPCPrincipal()
{
  static bool sDidAddVarCache = false;
  static bool sIgnoreIPCPrincipal = false;
  if (!sDidAddVarCache) {
    sDidAddVarCache = true;
    Preferences::AddBoolVarCache(&sIgnoreIPCPrincipal,
                                 "dom.testing.ignore_ipc_principal", false);
  }
  return sIgnoreIPCPrincipal;
}

void
ContentParent::NotifyUpdatedDictionaries()
{
    nsAutoTArray<ContentParent*, 8> processes;
    GetAll(processes);

    nsCOMPtr<nsISpellChecker> spellChecker(do_GetService(NS_SPELLCHECKER_CONTRACTID));
    MOZ_ASSERT(spellChecker, "No spell checker?");

    InfallibleTArray<nsString> dictionaries;
    spellChecker->GetDictionaryList(&dictionaries);

    for (size_t i = 0; i < processes.Length(); ++i) {
        unused << processes[i]->SendUpdateDictionaryList(dictionaries);
    }
}

 TabId
ContentParent::AllocateTabId(const TabId& aOpenerTabId,
                             const IPCTabContext& aContext,
                             const ContentParentId& aCpId)
{
    TabId tabId;
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        ContentProcessManager *cpm = ContentProcessManager::GetSingleton();
        tabId = cpm->AllocateTabId(aOpenerTabId, aContext, aCpId);
    }
    else {
        ContentChild::GetSingleton()->SendAllocateTabId(aOpenerTabId,
                                                        aContext,
                                                        aCpId,
                                                        &tabId);
    }
    return tabId;
}

 void
ContentParent::DeallocateTabId(const TabId& aTabId,
                               const ContentParentId& aCpId)
{
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        ContentProcessManager::GetSingleton()->DeallocateTabId(aCpId,
                                                               aTabId);
    }
    else {
        ContentChild::GetSingleton()->SendDeallocateTabId(aTabId);
    }
}

bool
ContentParent::RecvAllocateTabId(const TabId& aOpenerTabId,
                                 const IPCTabContext& aContext,
                                 const ContentParentId& aCpId,
                                 TabId* aTabId)
{
    *aTabId = AllocateTabId(aOpenerTabId, aContext, aCpId);
    if (!(*aTabId)) {
        return false;
    }
    return true;
}

bool
ContentParent::RecvDeallocateTabId(const TabId& aTabId)
{
    DeallocateTabId(aTabId, this->ChildID());
    return true;
}

nsTArray<TabContext>
ContentParent::GetManagedTabContext()
{
    return Move(ContentProcessManager::GetSingleton()->
        GetTabContextByContentProcess(this->ChildID()));
}

mozilla::docshell::POfflineCacheUpdateParent*
ContentParent::AllocPOfflineCacheUpdateParent(const URIParams& aManifestURI,
                                              const URIParams& aDocumentURI,
                                              const bool& aStickDocument,
                                              const TabId& aTabId)
{
    TabContext tabContext;
    if (!ContentProcessManager::GetSingleton()->
        GetTabContextByProcessAndTabId(this->ChildID(), aTabId, &tabContext)) {
        return nullptr;
    }
    nsRefPtr<mozilla::docshell::OfflineCacheUpdateParent> update =
        new mozilla::docshell::OfflineCacheUpdateParent(
            tabContext.OwnOrContainingAppId(),
            tabContext.IsBrowserElement());
    
    return update.forget().take();
}

bool
ContentParent::RecvPOfflineCacheUpdateConstructor(POfflineCacheUpdateParent* aActor,
                                                  const URIParams& aManifestURI,
                                                  const URIParams& aDocumentURI,
                                                  const bool& aStickDocument,
                                                  const TabId& aTabId)
{
    MOZ_ASSERT(aActor);

    nsRefPtr<mozilla::docshell::OfflineCacheUpdateParent> update =
        static_cast<mozilla::docshell::OfflineCacheUpdateParent*>(aActor);

    nsresult rv = update->Schedule(aManifestURI, aDocumentURI, aStickDocument);
    if (NS_FAILED(rv) && IsAlive()) {
        
        unused << update->SendFinish(false, false);
    }

    return true;
}

bool
ContentParent::DeallocPOfflineCacheUpdateParent(POfflineCacheUpdateParent* aActor)
{
    
    nsRefPtr<mozilla::docshell::OfflineCacheUpdateParent> update =
        dont_AddRef(static_cast<mozilla::docshell::OfflineCacheUpdateParent*>(aActor));
    return true;
}

bool
ContentParent::RecvSetOfflinePermission(const Principal& aPrincipal)
{
    nsIPrincipal* principal = aPrincipal;
    nsContentUtils::MaybeAllowOfflineAppByDefault(principal, nullptr);
    return true;
}

} 
} 

NS_IMPL_ISUPPORTS(ParentIdleListener, nsIObserver)

NS_IMETHODIMP
ParentIdleListener::Observe(nsISupports*, const char* aTopic, const char16_t* aData) {
    mozilla::unused << mParent->SendNotifyIdleObserver(mObserver,
                                                       nsDependentCString(aTopic),
                                                       nsDependentString(aData));
    return NS_OK;
}
