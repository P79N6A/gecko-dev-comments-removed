





#include "mozilla/DebugOnly.h"

#include "base/basictypes.h"

#include "ContentParent.h"

#if defined(ANDROID) || defined(LINUX)
# include <sys/time.h>
# include <sys/resource.h>
#endif

#include "chrome/common/process_watcher.h"

#include "AppProcessChecker.h"
#include "AudioChannelService.h"
#include "CrashReporterParent.h"
#include "IHistory.h"
#include "IDBFactory.h"
#include "IndexedDBParent.h"
#include "IndexedDatabaseManager.h"
#include "mozIApplication.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/asmjscache/AsmJSCache.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/ExternalHelperAppParent.h"
#include "mozilla/dom/PMemoryReportRequestParent.h"
#include "mozilla/dom/power/PowerManagerService.h"
#include "mozilla/dom/DOMStorageIPC.h"
#include "mozilla/dom/bluetooth/PBluetoothParent.h"
#include "mozilla/dom/PFMRadioParent.h"
#include "mozilla/dom/devicestorage/DeviceStorageRequestParent.h"
#include "mozilla/dom/FileSystemRequestParent.h"
#include "mozilla/dom/GeolocationBinding.h"
#include "mozilla/dom/telephony/TelephonyParent.h"
#include "mozilla/dom/time/DateCacheCleaner.h"
#include "SmsParent.h"
#include "mozilla/hal_sandbox/PHalParent.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "mozilla/ipc/TestShellParent.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/ImageBridgeParent.h"
#include "mozilla/net/NeckoParent.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "nsAppRunner.h"
#include "nsAutoPtr.h"
#include "nsCDefaultURIFixup.h"
#include "nsCExternalHandlerService.h"
#include "nsCOMPtr.h"
#include "nsChromeRegistryChrome.h"
#include "nsConsoleMessage.h"
#include "nsConsoleService.h"
#include "nsDebugImpl.h"
#include "nsDOMFile.h"
#include "nsFrameMessageManager.h"
#include "nsHashPropertyBag.h"
#include "nsIAlertsService.h"
#include "nsIAppsService.h"
#include "nsIClipboard.h"
#include "nsIDOMGeoGeolocation.h"
#include "mozilla/dom/WakeLock.h"
#include "nsIDOMWindow.h"
#include "nsIExternalProtocolService.h"
#include "nsIGfxInfo.h"
#include "nsIIdleService.h"
#include "nsIMemoryReporter.h"
#include "nsIMozBrowserFrame.h"
#include "nsIMutable.h"
#include "nsIObserverService.h"
#include "nsIPresShell.h"
#include "nsIRemoteBlob.h"
#include "nsIScriptError.h"
#include "nsIStyleSheet.h"
#include "nsISupportsPrimitives.h"
#include "nsIURIFixup.h"
#include "nsIWindowWatcher.h"
#include "nsIXULRuntime.h"
#include "nsMemoryReporterManager.h"
#include "nsServiceManagerUtils.h"
#include "nsStyleSheetService.h"
#include "nsThreadUtils.h"
#include "nsToolkitCompsCID.h"
#include "nsWidgetsCID.h"
#include "PreallocatedProcessManager.h"
#include "ProcessPriorityManager.h"
#include "SandboxHal.h"
#include "StructuredCloneUtils.h"
#include "TabParent.h"
#include "URIUtils.h"
#include "nsIWebBrowserChrome.h"
#include "nsIDocShell.h"
#include "mozilla/net/NeckoMessageUtils.h"
#include "gfxPrefs.h"

#if defined(ANDROID) || defined(LINUX)
#include "nsSystemInfo.h"
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
#include "nsIVolumeService.h"
#include "SpeakerManagerService.h"
using namespace mozilla::system;
#endif

#ifdef MOZ_B2G_BT
#include "BluetoothParent.h"
#include "BluetoothService.h"
#endif

#include "JavaScriptParent.h"

#ifdef MOZ_B2G_FM
#include "mozilla/dom/FMRadioParent.h"
#endif

#include "Crypto.h"

#ifdef MOZ_WEBSPEECH
#include "mozilla/dom/SpeechSynthesisParent.h"
#endif

#ifdef ENABLE_TESTS
#include "mozilla/ipc/PBackgroundChild.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#endif

static NS_DEFINE_CID(kCClipboardCID, NS_CLIPBOARD_CID);
static const char* sClipboardTextFlavors[] = { kUnicodeMime };

using base::ChildPrivileges;
using base::KillProcess;
using namespace mozilla::dom::bluetooth;
using namespace mozilla::dom::devicestorage;
using namespace mozilla::dom::indexedDB;
using namespace mozilla::dom::power;
using namespace mozilla::dom::mobilemessage;
using namespace mozilla::dom::telephony;
using namespace mozilla::hal;
using namespace mozilla::ipc;
using namespace mozilla::layers;
using namespace mozilla::net;
using namespace mozilla::jsipc;

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

NS_IMPL_ISUPPORTS2(BackgroundTester, nsIIPCBackgroundChildCreateCallback,
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

namespace mozilla {
namespace dom {

#define NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC "ipc:network:set-offline"

class MemoryReportRequestParent : public PMemoryReportRequestParent
{
public:
    MemoryReportRequestParent();
    virtual ~MemoryReportRequestParent();

    virtual bool Recv__delete__(const uint32_t& generation, const InfallibleTArray<MemoryReport>& report);
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


class ContentParentsMemoryReporter MOZ_FINAL : public nsIMemoryReporter
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORYREPORTER
};

NS_IMPL_ISUPPORTS1(ContentParentsMemoryReporter, nsIMemoryReporter)

NS_IMETHODIMP
ContentParentsMemoryReporter::CollectReports(nsIMemoryReporterCallback* cb,
                                             nsISupports* aClosure)
{
    nsAutoTArray<ContentParent*, 16> cps;
    ContentParent::GetAllEvenIfDead(cps);

    for (uint32_t i = 0; i < cps.Length(); i++) {
        ContentParent* cp = cps[i];
        MessageChannel* channel = cp->GetIPCChannel();

        nsString friendlyName;
        cp->FriendlyName(friendlyName);

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
                             cp->Pid(), channelStr, cp, refcnt);

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



static bool sCanLaunchSubprocesses;


static uint64_t gContentChildID = 1;




#define MAGIC_PREALLOCATED_APP_MANIFEST_URL NS_LITERAL_STRING("{{template}}")

static const char* sObserverTopics[] = {
    "xpcom-shutdown",
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
};

 already_AddRefed<ContentParent>
ContentParent::RunNuwaProcess()
{
    MOZ_ASSERT(NS_IsMainThread());
    nsRefPtr<ContentParent> nuwaProcess =
        new ContentParent( nullptr,
                           false,
                           true,
                          PROCESS_PRIORITY_BACKGROUND,
                           true);
    nuwaProcess->Init();
    return nuwaProcess.forget();
}




 already_AddRefed<ContentParent>
ContentParent::PreallocateAppProcess()
{
    nsRefPtr<ContentParent> process =
        new ContentParent( nullptr,
                           false,
                           true,
                          PROCESS_PRIORITY_PREALLOC);
    process->Init();
    return process.forget();
}

 already_AddRefed<ContentParent>
ContentParent::MaybeTakePreallocatedAppProcess(const nsAString& aAppManifestURL,
                                               ProcessPriority aInitialPriority)
{
    nsRefPtr<ContentParent> process = PreallocatedProcessManager::Take();
    if (!process) {
        return nullptr;
    }

    if (!process->SetPriorityAndCheckIsAlive(aInitialPriority)) {
        
        
        process->KillHard();
        return nullptr;
    }
    process->TransformPreallocatedIntoApp(aAppManifestURL);

    return process.forget();
}

 void
ContentParent::StartUp()
{
    if (XRE_GetProcessType() != GeckoProcessType_Default) {
        return;
    }

    
    RegisterStrongMemoryReporter(new ContentParentsMemoryReporter());

    mozilla::dom::time::InitializeDateCacheCleaner();

    BackgroundChild::Startup();

    sCanLaunchSubprocesses = true;

    
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
ContentParent::GetNewOrUsed(bool aForBrowserElement)
{
    if (!sNonAppContentParents)
        sNonAppContentParents = new nsTArray<ContentParent*>();

    int32_t maxContentProcesses = Preferences::GetInt("dom.ipc.processCount", 1);
    if (maxContentProcesses < 1)
        maxContentProcesses = 1;

    if (sNonAppContentParents->Length() >= uint32_t(maxContentProcesses)) {
        uint32_t idx = rand() % sNonAppContentParents->Length();
        nsRefPtr<ContentParent> p = (*sNonAppContentParents)[idx];
        NS_ASSERTION(p->IsAlive(), "Non-alive contentparent in sNonAppContentParents?");
        return p.forget();
    }

    nsRefPtr<ContentParent> p =
        new ContentParent( nullptr,
                          aForBrowserElement,
                           false,
                          PROCESS_PRIORITY_FOREGROUND);
    p->Init();
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
                                   NS_LITERAL_STRING("keyboard"), eCaseMatters)) {
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

void
ContentParent::RunAfterPreallocatedProcessReady(nsIRunnable* aRequest)
{
#ifdef MOZ_NUWA_PROCESS
    PreallocatedProcessManager::RunAfterPreallocatedProcessReady(aRequest);
#endif
}

 TabParent*
ContentParent::CreateBrowserOrApp(const TabContext& aContext,
                                  Element* aFrameElement)
{
    if (!sCanLaunchSubprocesses) {
        return nullptr;
    }

    if (aContext.IsBrowserElement() || !aContext.HasOwnApp()) {
        if (nsRefPtr<ContentParent> cp = GetNewOrUsed(aContext.IsBrowserElement())) {
            uint32_t chromeFlags = 0;

            
            
            nsCOMPtr<Element> frameElement = do_QueryInterface(aFrameElement);
            MOZ_ASSERT(frameElement);
            nsIDocShell* docShell =
                frameElement->OwnerDoc()->GetWindow()->GetDocShell();
            MOZ_ASSERT(docShell);
            nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(docShell);
            if (loadContext && loadContext->UsePrivateBrowsing()) {
                chromeFlags |= nsIWebBrowserChrome::CHROME_PRIVATE_WINDOW;
            }
            bool affectLifetime;
            docShell->GetAffectPrivateSessionLifetime(&affectLifetime);
            if (affectLifetime) {
                chromeFlags |= nsIWebBrowserChrome::CHROME_PRIVATE_LIFETIME;
            }

            nsRefPtr<TabParent> tp(new TabParent(cp, aContext, chromeFlags));
            tp->SetOwnerElement(aFrameElement);

            PBrowserParent* browser = cp->SendPBrowserConstructor(
                
                tp.forget().take(),
                aContext.AsIPCTabContext(),
                chromeFlags);
            return static_cast<TabParent*>(browser);
        }
        return nullptr;
    }

    
    
    
    nsCOMPtr<mozIApplication> ownApp = aContext.GetOwnApp();

    if (!sAppContentParents) {
        sAppContentParents =
            new nsDataHashtable<nsStringHashKey, ContentParent*>();
    }

    
    nsAutoString manifestURL;
    if (NS_FAILED(ownApp->GetManifestURL(manifestURL))) {
        NS_ERROR("Failed to get manifest URL");
        return nullptr;
    }

    ProcessPriority initialPriority = GetInitialProcessPriority(aFrameElement);

    nsRefPtr<ContentParent> p = sAppContentParents->Get(manifestURL);
    if (p) {
        
        
        
        if (!p->SetPriorityAndCheckIsAlive(initialPriority)) {
            p = nullptr;
        }
    }

    if (!p) {
        p = MaybeTakePreallocatedAppProcess(manifestURL,
                                            initialPriority);
        if (!p) {
#ifdef MOZ_NUWA_PROCESS
            if (Preferences::GetBool("dom.ipc.processPrelaunch.enabled",
                                     false)) {
                
                
                return nullptr;
            }
#endif
            NS_WARNING("Unable to use pre-allocated app process");
            p = new ContentParent(ownApp,
                                   false,
                                   false,
                                  initialPriority);
            p->Init();
        }
        sAppContentParents->Put(manifestURL, p);
    }

    uint32_t chromeFlags = 0;

    nsRefPtr<TabParent> tp = new TabParent(p, aContext, chromeFlags);
    tp->SetOwnerElement(aFrameElement);
    PBrowserParent* browser = p->SendPBrowserConstructor(
        
        nsRefPtr<TabParent>(tp).forget().take(),
        aContext.AsIPCTabContext(),
        chromeFlags);

    p->MaybeTakeCPUWakeLock(aFrameElement);

    return static_cast<TabParent*>(browser);
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

    NS_IMETHOD Notify(nsITimer* aTimer)
    {
        
        ShutDown();
        return NS_OK;
    }

private:
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

NS_IMPL_ISUPPORTS1(SystemMessageHandledListener,
                   nsITimerCallback)

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

    
    
    
    
    
    
#ifndef XP_WIN
    bool exited = false;
    base::DidProcessCrash(&exited, mSubprocess->GetChildProcessHandle());
    if (exited) {
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
ContentParent::TransformPreallocatedIntoApp(const nsAString& aAppManifestURL)
{
    MOZ_ASSERT(IsPreallocated());
    mAppManifestURL = aAppManifestURL;
    TryGetNameFromManifestURL(aAppManifestURL, mAppName);
}

void
ContentParent::ShutDownProcess(bool aCloseWithError)
{
    const InfallibleTArray<PIndexedDBParent*>& idbParents =
        ManagedPIndexedDBParent();
    for (uint32_t i = 0; i < idbParents.Length(); ++i) {
        static_cast<IndexedDBParent*>(idbParents[i])->Disconnect();
    }

    
    
    

    if (!aCloseWithError && !mCalledClose) {
        
        
        mCalledClose = true;
        Close();
    }

    if (aCloseWithError && !mCalledCloseWithError) {
        MessageChannel* channel = GetIPCChannel();
        if (channel) {
            mCalledCloseWithError = true;
            channel->CloseWithError();
        }
    }

    
    
    MarkAsDead();

    
    
    
    
    if (mMessageManager) {
      mMessageManager->Disconnect();
      mMessageManager = nullptr;
    }
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

    
    
    SetReplyTimeoutMs(Preferences::GetInt("dom.ipc.cpow.timeout", 3000));
}

void
ContentParent::ProcessingError(Result what)
{
    if (MsgDropped == what) {
        
        return;
    }
    
    KillHard();
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
    DelayedDeleteContentParentTask(ContentParent* aObj) : mObj(aObj) { }

    
    NS_IMETHODIMP Run() { return NS_OK; }

    nsRefPtr<ContentParent> mObj;
};

}

void
ContentParent::ActorDestroy(ActorDestroyReason why)
{
    if (mForceKillTask) {
        mForceKillTask->Cancel();
        mForceKillTask = nullptr;
    }

    nsRefPtr<nsFrameMessageManager> ppm = mMessageManager;
    if (ppm) {
      ppm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm.get()),
                          CHILD_PROCESS_SHUTDOWN_MESSAGE, false,
                          nullptr, nullptr, nullptr, nullptr);
    }
    nsRefPtr<ContentParent> kungFuDeathGrip(this);
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        size_t length = ArrayLength(sObserverTopics);
        for (size_t i = 0; i < length; ++i) {
            obs->RemoveObserver(static_cast<nsIObserver*>(this),
                                sObserverTopics[i]);
        }
    }

    if (ppm) {
      ppm->Disconnect();
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

    RecvRemoveGeolocationListener();

    mConsoleService = nullptr;

    MarkAsDead();

    if (obs) {
        nsRefPtr<nsHashPropertyBag> props = new nsHashPropertyBag();

        props->SetPropertyAsUint64(NS_LITERAL_STRING("childID"), mChildID);

        if (AbnormalShutdown == why) {
            props->SetPropertyAsBool(NS_LITERAL_STRING("abnormal"), true);

#ifdef MOZ_CRASHREPORTER
            
            
            
            if (ManagedPCrashReporterParent().Length() > 0) {
                CrashReporterParent* crashReporter =
                    static_cast<CrashReporterParent*>(ManagedPCrashReporterParent()[0]);

                
                
                
                
                
                
                if (!mAppManifestURL.IsEmpty()) {
                    crashReporter->AnnotateCrashReport(NS_LITERAL_CSTRING("URL"),
                                                       NS_ConvertUTF16toUTF8(mAppManifestURL));
                }

                crashReporter->GenerateCrashReport(this, nullptr);

                nsAutoString dumpID(crashReporter->ChildDumpID());
                props->SetPropertyAsAString(NS_LITERAL_STRING("dumpID"), dumpID);
            }
#endif
        }
        obs->NotifyObservers((nsIPropertyBag2*) props, "ipc:content-shutdown", nullptr);
    }

    mIdleListeners.Clear();

    
    
    
    ShutDownProcess( true);

    MessageLoop::current()->
        PostTask(FROM_HERE,
                 NewRunnableFunction(DelayedDeleteSubprocess, mSubprocess));
    mSubprocess = nullptr;

    
    
    
    
    
    
    
    NS_DispatchToCurrentThread(new DelayedDeleteContentParentTask(this));
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

    MOZ_ASSERT(!mForceKillTask);
    int32_t timeoutSecs =
        Preferences::GetInt("dom.ipc.tabs.shutdownTimeoutSecs", 5);
    if (timeoutSecs > 0) {
        MessageLoop::current()->PostDelayedTask(
            FROM_HERE,
            mForceKillTask = NewRunnableMethod(this, &ContentParent::KillHard),
            timeoutSecs * 1000);
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
                               false));
    }
}

jsipc::JavaScriptParent*
ContentParent::GetCPOWManager()
{
    if (ManagedPJavaScriptParent().Length()) {
        return static_cast<JavaScriptParent*>(ManagedPJavaScriptParent()[0]);
    }
    JavaScriptParent* actor = static_cast<JavaScriptParent*>(SendPJavaScriptConstructor());
    return actor;
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
    mForceKillTask = nullptr;
    mNumDestroyingTabs = 0;
    mIsAlive = true;
    mSendPermissionUpdates = false;
    mCalledClose = false;
    mCalledCloseWithError = false;
    mCalledKillHard = false;
}

ContentParent::ContentParent(mozIApplication* aApp,
                             bool aIsForBrowser,
                             bool aIsForPreallocated,
                             ProcessPriority aInitialPriority ,
                             bool aIsNuwaProcess )
    : mIsForBrowser(aIsForBrowser)
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

    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    ChildPrivileges privs = aIsNuwaProcess
        ? base::PRIVILEGES_INHERIT
        : base::PRIVILEGES_DEFAULT;
    mSubprocess = new GeckoChildProcessHost(GeckoProcessType_Content, privs);
    mSubprocess->SetSandboxEnabled(ShouldSandboxContentProcesses());

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

    InitInternal(priority,
                 false, 
                 false  );
}
#endif  

ContentParent::~ContentParent()
{
    if (mForceKillTask) {
        mForceKillTask->Cancel();
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

            if (gfxPrefs::AsyncVideoEnabled()) {
                opened = PImageBridge::Open(this);
                MOZ_ASSERT(opened);
            }
        }
    }

    if (aSendRegisteredChrome) {
        nsCOMPtr<nsIChromeRegistry> registrySvc = nsChromeRegistry::GetService();
        nsChromeRegistryChrome* chromeRegistry =
            static_cast<nsChromeRegistryChrome*>(registrySvc.get());
        chromeRegistry->SendRegisteredChrome(this);
    }

    mMessageManager = nsFrameMessageManager::NewProcessMessageManager(this);

    if (gAppData) {
        nsCString version(gAppData->version);
        nsCString buildID(gAppData->buildID);
        nsCString name(gAppData->name);
        nsCString UAName(gAppData->UAName);

        
        unused << SendAppInfo(version, buildID, name, UAName);
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
ContentParent::IsNuwaProcess()
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
        do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
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

    *showPassword = GeckoAppShell::GetShowPasswordSetting();
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
ContentParent::RecvAudioChannelGetState(const AudioChannelType& aType,
                                        const bool& aElementHidden,
                                        const bool& aElementWasHidden,
                                        AudioChannelState* aState)
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetAudioChannelService();
    *aState = AUDIO_CHANNEL_STATE_NORMAL;
    if (service) {
        *aState = service->GetStateInternal(aType, mChildID,
                                            aElementHidden, aElementWasHidden);
    }
    return true;
}

bool
ContentParent::RecvAudioChannelRegisterType(const AudioChannelType& aType,
                                            const bool& aWithVideo)
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetAudioChannelService();
    if (service) {
        service->RegisterType(aType, mChildID, aWithVideo);
    }
    return true;
}

bool
ContentParent::RecvAudioChannelUnregisterType(const AudioChannelType& aType,
                                              const bool& aElementHidden,
                                              const bool& aWithVideo)
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetAudioChannelService();
    if (service) {
        service->UnregisterType(aType, aElementHidden, mChildID, aWithVideo);
    }
    return true;
}

bool
ContentParent::RecvAudioChannelChangedNotification()
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetAudioChannelService();
    if (service) {
       service->SendAudioChannelChangedNotification(ChildID());
    }
    return true;
}

bool
ContentParent::RecvAudioChannelChangeDefVolChannel(
  const AudioChannelType& aType, const bool& aHidden)
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetAudioChannelService();
    if (service) {
       service->SetDefaultVolumeControlChannelInternal(aType,
                                                       aHidden, mChildID);
    }
    return true;
}

bool
ContentParent::RecvBroadcastVolume(const nsString& aVolumeName)
{
#ifdef MOZ_WIDGET_GONK
    nsresult rv;
    nsCOMPtr<nsIVolumeService> vs = do_GetService(NS_VOLUMESERVICE_CONTRACTID, &rv);
    if (vs) {
        vs->BroadcastVolume(aVolumeName);
    }
    return true;
#else
    NS_WARNING("ContentParent::RecvBroadcastVolume shouldn't be called when MOZ_WIDGET_GONK is not defined");
    return false;
#endif
}

bool
ContentParent::RecvNuwaReady()
{
#ifdef MOZ_NUWA_PROCESS
    if (!IsNuwaProcess()) {
        printf_stderr("Terminating child process %d for unauthorized IPC message: "
                      "NuwaReady", Pid());
        KillHard();
        return false;
    }
    PreallocatedProcessManager::OnNuwaReady();
    return true;
#else
    NS_ERROR("ContentParent::RecvNuwaReady() not implemented!");
    return false;
#endif
}

bool
ContentParent::RecvAddNewProcess(const uint32_t& aPid,
                                 const InfallibleTArray<ProtocolFdMapping>& aFds)
{
#ifdef MOZ_NUWA_PROCESS
    if (!IsNuwaProcess()) {
        printf_stderr("Terminating child process %d for unauthorized IPC message: "
                      "AddNewProcess(%d)", Pid(), aPid);
        KillHard();
        return false;
    }
    nsRefPtr<ContentParent> content;
    content = new ContentParent(this,
                                MAGIC_PREALLOCATED_APP_MANIFEST_URL,
                                aPid,
                                aFds);
    content->Init();
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
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPositionCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
ContentParent::Observe(nsISupports* aSubject,
                       const char* aTopic,
                       const char16_t* aData)
{
    if (!strcmp(aTopic, "xpcom-shutdown") && mSubprocess) {
        ShutDownProcess( false);
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
        if (!SendPreferenceUpdate(pref)) {
            return NS_ERROR_NOT_AVAILABLE;
        }
    }
    else if (!strcmp(aTopic, NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC)) {
      NS_ConvertUTF16toUTF8 dataStr(aData);
      const char *offline = dataStr.get();
      if (!SendSetOffline(!strcmp(offline, "true") ? true : false))
          return NS_ERROR_NOT_AVAILABLE;
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
            int minimize, identOffset = -1;
            nsDependentString msg(aData);
            NS_ConvertUTF16toUTF8 cmsg(msg);

            if (sscanf(cmsg.get(),
                       "generation=%x minimize=%d DMDident=%n",
                       &generation, &minimize, &identOffset) < 2
                || identOffset < 0) {
                return NS_ERROR_INVALID_ARG;
            }
            
            
            MOZ_ASSERT(cmsg[identOffset - 1] == '=');
            unused << SendPMemoryReportRequestConstructor(
              generation, minimize, nsString(Substring(msg, identOffset)));
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

        unused << SendFilePathUpdate(file->mStorageType, file->mStorageName, file->mPath, creason);
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

        vol->GetName(volName);
        vol->GetMountPoint(mountPoint);
        vol->GetState(&state);
        vol->GetMountGeneration(&mountGeneration);
        vol->GetIsMediaPresent(&isMediaPresent);
        vol->GetIsSharing(&isSharing);
        vol->GetIsFormatting(&isFormatting);

        unused << SendFileSystemUpdate(volName, mountPoint, state,
                                       mountGeneration, isMediaPresent,
                                       isSharing, isFormatting);
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

    return NS_OK;
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

bool
ContentParent::RecvGetProcessAttributes(uint64_t* aId,
                                        bool* aIsForApp, bool* aIsForBrowser)
{
    *aId = mChildID;
    *aIsForApp = IsForApp();
    *aIsForBrowser = mIsForBrowser;

    return true;
}

bool
ContentParent::RecvGetXPCOMProcessAttributes(bool* aIsOffline)
{
    nsCOMPtr<nsIIOService> io(do_GetIOService());
    NS_ASSERTION(io, "No IO service?");
    DebugOnly<nsresult> rv = io->GetOffline(aIsOffline);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed getting offline?");

    return true;
}

mozilla::jsipc::PJavaScriptParent *
ContentParent::AllocPJavaScriptParent()
{
    mozilla::jsipc::JavaScriptParent *parent = new mozilla::jsipc::JavaScriptParent();
    if (!parent->init()) {
        delete parent;
        return nullptr;
    }
    return parent;
}

bool
ContentParent::DeallocPJavaScriptParent(PJavaScriptParent *parent)
{
    static_cast<mozilla::jsipc::JavaScriptParent *>(parent)->decref();
    return true;
}

PBrowserParent*
ContentParent::AllocPBrowserParent(const IPCTabContext& aContext,
                                   const uint32_t &aChromeFlags)
{
    unused << aChromeFlags;

    const IPCTabAppBrowserContext& appBrowser = aContext.appBrowserContext();

    
    
    
    
    if (appBrowser.type() != IPCTabAppBrowserContext::TPopupIPCTabContext) {
        NS_ERROR("Unexpected IPCTabContext type.  Aborting AllocPBrowserParent.");
        return nullptr;
    }

    const PopupIPCTabContext& popupContext = appBrowser.get_PopupIPCTabContext();
    TabParent* opener = static_cast<TabParent*>(popupContext.openerParent());
    if (!opener) {
        NS_ERROR("Got null opener from child; aborting AllocPBrowserParent.");
        return nullptr;
    }

    
    
    
    if (!popupContext.isBrowserElement() && opener->IsBrowserElement()) {
        NS_ERROR("Child trying to escalate privileges!  Aborting AllocPBrowserParent.");
        return nullptr;
    }

    MaybeInvalidTabContext tc(aContext);
    if (!tc.IsValid()) {
        NS_ERROR(nsPrintfCString("Child passed us an invalid TabContext.  (%s)  "
                                 "Aborting AllocPBrowserParent.",
                                 tc.GetInvalidReason()).get());
        return nullptr;
    }

    TabParent* parent = new TabParent(this, tc.GetTabContext(), aChromeFlags);

    
    NS_ADDREF(parent);
    return parent;
}

bool
ContentParent::DeallocPBrowserParent(PBrowserParent* frame)
{
    TabParent* parent = static_cast<TabParent*>(frame);
    NS_RELEASE(parent);
    return true;
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
  return BlobParent::Create(this, aParams);
}

bool
ContentParent::DeallocPBlobParent(PBlobParent* aActor)
{
  delete aActor;
  return true;
}

BlobParent*
ContentParent::GetOrCreateActorForBlob(nsIDOMBlob* aBlob)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aBlob);

  
  
  if (nsCOMPtr<nsIRemoteBlob> remoteBlob = do_QueryInterface(aBlob)) {
    if (BlobParent* actor = static_cast<BlobParent*>(
          static_cast<PBlobParent*>(remoteBlob->GetPBlob()))) {
      if (static_cast<ContentParent*>(actor->Manager()) == this) {
        return actor;
      }
    }
  }

  
  
  
  const nsDOMFileBase* blob = static_cast<nsDOMFileBase*>(aBlob);

  
  
  
  const nsTArray<nsCOMPtr<nsIDOMBlob> >* subBlobs = blob->GetSubBlobs();
  if (subBlobs && subBlobs->Length() == 1) {
    const nsCOMPtr<nsIDOMBlob>& subBlob = subBlobs->ElementAt(0);
    MOZ_ASSERT(subBlob);

    
    
    nsCOMPtr<nsIDOMFile> multipartBlobAsFile = do_QueryInterface(aBlob);
    nsCOMPtr<nsIDOMFile> subBlobAsFile = do_QueryInterface(subBlob);
    if (!multipartBlobAsFile == !subBlobAsFile) {
      
      
      if (nsCOMPtr<nsIRemoteBlob> remoteSubBlob = do_QueryInterface(subBlob)) {
        BlobParent* actor =
          static_cast<BlobParent*>(
            static_cast<PBlobParent*>(remoteSubBlob->GetPBlob()));
        MOZ_ASSERT(actor);

        if (static_cast<ContentParent*>(actor->Manager()) == this) {
          return actor;
        }
      }

      
      
      
      aBlob = subBlob;
      blob = static_cast<nsDOMFileBase*>(aBlob);
      subBlobs = blob->GetSubBlobs();
    }
  }

  
  nsCOMPtr<nsIMutable> mutableBlob = do_QueryInterface(aBlob);
  if (!mutableBlob || NS_FAILED(mutableBlob->SetMutable(false))) {
    NS_WARNING("Failed to make blob immutable!");
    return nullptr;
  }

  ChildBlobConstructorParams params;

  if (blob->IsSizeUnknown() || blob->IsDateUnknown()) {
    
    
    
    
    params = MysteryBlobConstructorParams();
  }
  else {
    nsString contentType;
    nsresult rv = aBlob->GetType(contentType);
    NS_ENSURE_SUCCESS(rv, nullptr);

    uint64_t length;
    rv = aBlob->GetSize(&length);
    NS_ENSURE_SUCCESS(rv, nullptr);

    nsCOMPtr<nsIDOMFile> file = do_QueryInterface(aBlob);
    if (file) {
      FileBlobConstructorParams fileParams;

      rv = file->GetMozLastModifiedDate(&fileParams.modDate());
      NS_ENSURE_SUCCESS(rv, nullptr);

      rv = file->GetName(fileParams.name());
      NS_ENSURE_SUCCESS(rv, nullptr);

      fileParams.contentType() = contentType;
      fileParams.length() = length;

      params = fileParams;
    } else {
      NormalBlobConstructorParams blobParams;
      blobParams.contentType() = contentType;
      blobParams.length() = length;
      params = blobParams;
    }
  }

  BlobParent* actor = BlobParent::Create(this, aBlob);
  NS_ENSURE_TRUE(actor, nullptr);

  return SendPBlobConstructor(actor, params) ? actor : nullptr;
}

void
ContentParent::KillHard()
{
    
    
    
    if (mCalledKillHard) {
        return;
    }
    mCalledKillHard = true;
    mForceKillTask = nullptr;
    
    
    
    
    if (!KillProcess(OtherProcess(), 1, false)) {
        NS_WARNING("failed to kill subprocess!");
    }
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(&ProcessWatcher::EnsureProcessTerminated,
                            OtherProcess(), true));
    
    MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        NewRunnableMethod(this, &ContentParent::ShutDownProcess,
                           true),
        3000);
    
    
    SetOtherProcess(0);
}

bool
ContentParent::IsPreallocated()
{
    return mAppManifestURL == MAGIC_PREALLOCATED_APP_MANIFEST_URL;
}

void
ContentParent::FriendlyName(nsAString& aName)
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

PIndexedDBParent*
ContentParent::AllocPIndexedDBParent()
{
  return new IndexedDBParent(this);
}

bool
ContentParent::DeallocPIndexedDBParent(PIndexedDBParent* aActor)
{
  delete aActor;
  return true;
}

bool
ContentParent::RecvPIndexedDBConstructor(PIndexedDBParent* aActor)
{
  nsRefPtr<IndexedDatabaseManager> mgr = IndexedDatabaseManager::GetOrCreate();
  NS_ENSURE_TRUE(mgr, false);

  if (!IndexedDatabaseManager::IsMainProcess()) {
    NS_RUNTIMEABORT("Not supported yet!");
  }

  nsRefPtr<IDBFactory> factory;
  nsresult rv = IDBFactory::Create(this, getter_AddRefs(factory));
  NS_ENSURE_SUCCESS(rv, false);

  NS_ASSERTION(factory, "This should never be null!");

  IndexedDBParent* actor = static_cast<IndexedDBParent*>(aActor);
  actor->mFactory = factory;
  actor->mASCIIOrigin = factory->GetASCIIOrigin();

  return true;
}

PMemoryReportRequestParent*
ContentParent::AllocPMemoryReportRequestParent(const uint32_t& generation,
                                               const bool &minimizeMemoryUsage,
                                               const nsString &aDMDDumpIdent)
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
    SpeakerManagerService::GetSpeakerManagerService();
  if (service) {
    *aValue = service->GetSpeakerStatus();
  }
  return true;
#endif
  return false;
}

bool
ContentParent::RecvSpeakerManagerForceSpeaker(const bool& aEnable)
{
#ifdef MOZ_WIDGET_GONK
  nsRefPtr<SpeakerManagerService> service =
    SpeakerManagerService::GetSpeakerManagerService();
  if (service) {
    service->ForceSpeaker(aEnable, mChildID);
  }
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
        sysAlerts->ShowAlertNotification(aImageUrl, aTitle, aText, aTextClickable,
                                         aCookie, this, aName, aBidi, aLang, aPrincipal);
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
  nsIPrincipal* principal = aPrincipal;
  if (!Preferences::GetBool("dom.testing.ignore_ipc_principal", false) &&
      principal && !AssertAppPrincipal(this, principal)) {
    return false;
  }

  nsRefPtr<nsFrameMessageManager> ppm = mMessageManager;
  if (ppm) {
    StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForParent(aData);
    CpowIdHolder cpows(GetCPOWManager(), aCpows);

    ppm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm.get()),
                        aMsg, true, &cloneData, &cpows, aPrincipal, aRetvals);
  }
  return true;
}

bool
ContentParent::AnswerRpcMessage(const nsString& aMsg,
                                const ClonedMessageData& aData,
                                const InfallibleTArray<CpowEntry>& aCpows,
                                const IPC::Principal& aPrincipal,
                                InfallibleTArray<nsString>* aRetvals)
{
  nsIPrincipal* principal = aPrincipal;
  if (!Preferences::GetBool("dom.testing.ignore_ipc_principal", false) &&
      principal && !AssertAppPrincipal(this, principal)) {
    return false;
  }

  nsRefPtr<nsFrameMessageManager> ppm = mMessageManager;
  if (ppm) {
    StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForParent(aData);
    CpowIdHolder cpows(GetCPOWManager(), aCpows);
    ppm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm.get()),
                        aMsg, true, &cloneData, &cpows, aPrincipal, aRetvals);
  }
  return true;
}

bool
ContentParent::RecvAsyncMessage(const nsString& aMsg,
                                const ClonedMessageData& aData,
                                const InfallibleTArray<CpowEntry>& aCpows,
                                const IPC::Principal& aPrincipal)
{
  nsIPrincipal* principal = aPrincipal;
  if (!Preferences::GetBool("dom.testing.ignore_ipc_principal", false) &&
      principal && !AssertAppPrincipal(this, principal)) {
    return false;
  }

  nsRefPtr<nsFrameMessageManager> ppm = mMessageManager;
  if (ppm) {
    StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForParent(aData);
    CpowIdHolder cpows(GetCPOWManager(), aCpows);
    ppm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm.get()),
                        aMsg, false, &cloneData, &cpows, aPrincipal, nullptr);
  }
  return true;
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
AddGeolocationListener(nsIDOMGeoPositionCallback* watcher, bool highAccuracy)
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
  geo->WatchPosition(watcher, nullptr, options, &retval);
  return retval;
}

bool
ContentParent::RecvAddGeolocationListener(const IPC::Principal& aPrincipal,
                                          const bool& aHighAccuracy)
{
#ifdef MOZ_CHILD_PERMISSIONS
  if (!Preferences::GetBool("dom.testing.ignore_ipc_principal", false)) {
    uint32_t permission = mozilla::CheckPermission(this, aPrincipal,
                                                   "geolocation");
    if (permission != nsIPermissionManager::ALLOW_ACTION) {
      return true;
    }
  }
#endif 

  
  
  RecvRemoveGeolocationListener();
  mGeolocationWatchID = AddGeolocationListener(this, aHighAccuracy);
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
    mGeolocationWatchID = -1;
  }
  return true;
}

bool
ContentParent::RecvSetGeolocationHigherAccuracy(const bool& aEnable)
{
  
  
  if (mGeolocationWatchID != -1) {
    RecvRemoveGeolocationListener();
    mGeolocationWatchID = AddGeolocationListener(this, aEnable);
  }
  return true;
}

NS_IMETHODIMP
ContentParent::HandleEvent(nsIDOMGeoPosition* postion)
{
  unused << SendGeolocationUpdate(GeoPosition(postion));
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
  if (!GetCPOWManager()->Wrap(aCx, aCpows, &cpows)) {
    return false;
  }
  return SendAsyncMessage(nsString(aMessage), data, cpows, aPrincipal);
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
ContentParent::RecvSystemMessageHandled()
{
    SystemMessageHandledListener::OnSystemMessageHandled();
    return true;
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
ContentParent::RecvKeywordToURI(const nsCString& aKeyword, OptionalInputStreamParams* aPostData,
                                OptionalURIParams* aURI)
{
  nsCOMPtr<nsIURIFixup> fixup = do_GetService(NS_URIFIXUP_CONTRACTID);
  if (!fixup) {
    return true;
  }

  nsCOMPtr<nsIInputStream> postData;
  nsCOMPtr<nsIURI> uri;
  if (NS_FAILED(fixup->KeywordToURI(aKeyword, getter_AddRefs(postData),
                                    getter_AddRefs(uri)))) {
    return true;
  }

  SerializeInputStream(postData, *aPostData);
  SerializeURI(uri, *aURI);
  return true;
}

bool
ContentParent::ShouldContinueFromReplyTimeout()
{
  
  
  MOZ_ASSERT(BrowserTabsRemote());
  return false;
}

bool
ContentParent::ShouldSandboxContentProcesses()
{
#ifdef MOZ_CONTENT_SANDBOX
  return !PR_GetEnv("MOZ_DISABLE_CONTENT_SANDBOX");
#else
  return true;
#endif
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

} 
} 

NS_IMPL_ISUPPORTS1(ParentIdleListener, nsIObserver)

NS_IMETHODIMP
ParentIdleListener::Observe(nsISupports*, const char* aTopic, const char16_t* aData) {
  mozilla::unused << mParent->SendNotifyIdleObserver(mObserver,
                                                     nsDependentCString(aTopic),
                                                     nsDependentString(aData));
  return NS_OK;
}
