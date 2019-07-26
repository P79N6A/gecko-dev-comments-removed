





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
#include "mozilla/dom/Element.h"
#include "mozilla/dom/ExternalHelperAppParent.h"
#include "mozilla/dom/PMemoryReportRequestParent.h"
#include "mozilla/dom/power/PowerManagerService.h"
#include "mozilla/dom/DOMStorageIPC.h"
#include "mozilla/dom/bluetooth/PBluetoothParent.h"
#include "mozilla/dom/PFMRadioParent.h"
#include "mozilla/dom/devicestorage/DeviceStorageRequestParent.h"
#include "mozilla/dom/GeolocationBinding.h"
#include "mozilla/dom/telephony/TelephonyParent.h"
#include "SmsParent.h"
#include "mozilla/Hal.h"
#include "mozilla/hal_sandbox/PHalParent.h"
#include "mozilla/ipc/TestShellParent.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/ImageBridgeParent.h"
#include "mozilla/net/NeckoParent.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsAppRunner.h"
#include "nsAutoPtr.h"
#include "nsCDefaultURIFixup.h"
#include "nsCExternalHandlerService.h"
#include "nsCOMPtr.h"
#include "nsChromeRegistryChrome.h"
#include "nsConsoleMessage.h"
#include "nsConsoleService.h"
#include "nsDebugImpl.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDOMFile.h"
#include "nsExternalHelperAppService.h"
#include "nsFrameMessageManager.h"
#include "nsHashPropertyBag.h"
#include "nsIAlertsService.h"
#include "nsIAppsService.h"
#include "nsIClipboard.h"
#include "nsIDOMApplicationRegistry.h"
#include "nsIDOMGeoGeolocation.h"
#include "nsIDOMWakeLock.h"
#include "nsIDOMWindow.h"
#include "nsIFilePicker.h"
#include "nsIMemoryReporter.h"
#include "nsIMozBrowserFrame.h"
#include "nsIMutable.h"
#include "nsIObserverService.h"
#include "nsIPresShell.h"
#include "nsIRemoteBlob.h"
#include "nsIScriptError.h"
#include "nsIScriptSecurityManager.h"
#include "nsIStyleSheet.h"
#include "nsISupportsPrimitives.h"
#include "nsIURIFixup.h"
#include "nsIWindowWatcher.h"
#include "nsServiceManagerUtils.h"
#include "nsStyleSheetService.h"
#include "nsSystemInfo.h"
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

#ifdef ANDROID
# include "gfxAndroidPlatform.h"
#endif

#ifdef MOZ_CRASHREPORTER
# include "nsExceptionHandler.h"
# include "nsICrashReporter.h"
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

namespace mozilla {
namespace dom {

#define NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC "ipc:network:set-offline"


class ChildReporter MOZ_FINAL : public nsIMemoryReporter
{
public:
    ChildReporter(const InfallibleTArray<MemoryReport>& childReports)
    {
        for (uint32_t i = 0; i < childReports.Length(); i++) {
            MemoryReport r(childReports[i].process(),
                           childReports[i].path(),
                           childReports[i].kind(),
                           childReports[i].units(),
                           childReports[i].amount(),
                           childReports[i].desc());

            
            MOZ_ASSERT(!r.process().IsEmpty());

            mChildReports.AppendElement(r);
        }
    }

    NS_DECL_ISUPPORTS

    NS_IMETHOD GetName(nsACString& name)
    {
        name.AssignLiteral("content-child");
        return NS_OK;
    }

    NS_IMETHOD CollectReports(nsIMemoryReporterCallback* aCb,
                              nsISupports* aClosure)
    {
        for (uint32_t i = 0; i < mChildReports.Length(); i++) {
            nsresult rv;
            MemoryReport r = mChildReports[i];
            rv = aCb->Callback(r.process(), r.path(), r.kind(), r.units(),
                               r.amount(), r.desc(), aClosure);
            NS_ENSURE_SUCCESS(rv, rv);
        }
        return NS_OK;
    }

  private:
    InfallibleTArray<MemoryReport> mChildReports;
};

NS_IMPL_ISUPPORTS1(ChildReporter, nsIMemoryReporter)

class MemoryReportRequestParent : public PMemoryReportRequestParent
{
public:
    MemoryReportRequestParent();
    virtual ~MemoryReportRequestParent();

    virtual bool Recv__delete__(const InfallibleTArray<MemoryReport>& report);
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
MemoryReportRequestParent::Recv__delete__(const InfallibleTArray<MemoryReport>& childReports)
{
    Owner()->SetChildMemoryReports(childReports);
    return true;
}

MemoryReportRequestParent::~MemoryReportRequestParent()
{
    MOZ_COUNT_DTOR(MemoryReportRequestParent);
}




class ContentParentMemoryReporter MOZ_FINAL : public nsIMemoryReporter
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORYREPORTER
    NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(MallocSizeOf)
};

NS_IMPL_ISUPPORTS1(ContentParentMemoryReporter, nsIMemoryReporter)

NS_IMETHODIMP
ContentParentMemoryReporter::GetName(nsACString& aName)
{
    aName.AssignLiteral("ContentParents");
    return NS_OK;
}

NS_IMETHODIMP
ContentParentMemoryReporter::CollectReports(nsIMemoryReporterCallback* cb,
                                            nsISupports* aClosure)
{
    nsAutoTArray<ContentParent*, 16> cps;
    ContentParent::GetAllEvenIfDead(cps);

    for (uint32_t i = 0; i < cps.Length(); i++) {
        ContentParent* cp = cps[i];
        AsyncChannel* channel = cp->GetIPCChannel();

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
                                   nsIMemoryReporter::KIND_OTHER,
                                   nsIMemoryReporter::UNITS_COUNT,
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





 already_AddRefed<ContentParent>
ContentParent::PreallocateAppProcess()
{
    nsRefPtr<ContentParent> process =
        new ContentParent( nullptr,
                           false,
                           true,
                          
                          
                          base::PRIVILEGES_INHERIT,
                          PROCESS_PRIORITY_BACKGROUND);
    process->Init();
    return process.forget();
}

 already_AddRefed<ContentParent>
ContentParent::MaybeTakePreallocatedAppProcess(const nsAString& aAppManifestURL,
                                               ChildPrivileges aPrivs,
                                               ProcessPriority aInitialPriority)
{
    nsRefPtr<ContentParent> process = PreallocatedProcessManager::Take();
    if (!process) {
        return nullptr;
    }

    if (!process->SetPriorityAndCheckIsAlive(aInitialPriority) ||
        !process->TransformPreallocatedIntoApp(aAppManifestURL, aPrivs)) {
        
        
        process->KillHard();
        return nullptr;
    }

    return process.forget();
}

 void
ContentParent::StartUp()
{
    if (XRE_GetProcessType() != GeckoProcessType_Default) {
        return;
    }

    nsRefPtr<ContentParentMemoryReporter> mr = new ContentParentMemoryReporter();
    NS_RegisterMemoryReporter(mr);

    sCanLaunchSubprocesses = true;

    
    PreallocatedProcessManager::AllocateAfterDelay();
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
                          base::PRIVILEGES_DEFAULT,
                          PROCESS_PRIORITY_FOREGROUND);
    p->Init();
    sNonAppContentParents->AppendElement(p);
    return p.forget();
}

namespace {
struct SpecialPermission {
    const char* perm;           
    ChildPrivileges privs;      
};
}

static ChildPrivileges
PrivilegesForApp(mozIApplication* aApp)
{
    const SpecialPermission specialPermissions[] = {
        
        
        { "camera", base::PRIVILEGES_CAMERA }
    };
    for (size_t i = 0; i < ArrayLength(specialPermissions); ++i) {
        const char* const permission = specialPermissions[i].perm;
        bool hasPermission = false;
        if (NS_FAILED(aApp->HasPermission(permission, &hasPermission))) {
            NS_WARNING("Unable to check permissions.  Breakage may follow.");
            break;
        } else if (hasPermission) {
            return specialPermissions[i].privs;
        }
    }
    return base::PRIVILEGES_DEFAULT;
}

 ProcessPriority
ContentParent::GetInitialProcessPriority(Element* aFrameElement)
{
    
    

    if (!aFrameElement) {
        return PROCESS_PRIORITY_FOREGROUND;
    }

    if (!aFrameElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::mozapptype,
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

 TabParent*
ContentParent::CreateBrowserOrApp(const TabContext& aContext,
                                  Element* aFrameElement)
{
    if (!sCanLaunchSubprocesses) {
        return nullptr;
    }

    if (aContext.IsBrowserElement() || !aContext.HasOwnApp()) {
        if (nsRefPtr<ContentParent> cp = GetNewOrUsed(aContext.IsBrowserElement())) {
            nsRefPtr<TabParent> tp(new TabParent(cp, aContext));
            tp->SetOwnerElement(aFrameElement);
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

            PBrowserParent* browser = cp->SendPBrowserConstructor(
                tp.forget().get(), 
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
        ChildPrivileges privs = PrivilegesForApp(ownApp);
        p = MaybeTakePreallocatedAppProcess(manifestURL, privs,
                                            initialPriority);
        if (!p) {
            NS_WARNING("Unable to use pre-allocated app process");
            p = new ContentParent(ownApp,
                                   false,
                                   false,
                                  privs,
                                  initialPriority);
            p->Init();
        }
        sAppContentParents->Put(manifestURL, p);
    }

    nsRefPtr<TabParent> tp = new TabParent(p, aContext);
    tp->SetOwnerElement(aFrameElement);
    PBrowserParent* browser = p->SendPBrowserConstructor(
        nsRefPtr<TabParent>(tp).forget().get(), 
        aContext.AsIPCTabContext(),
         0);

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
         cp = cp->getNext()) {
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
         cp = cp->getNext()) {
        aArray.AppendElement(cp);
    }
}

void
ContentParent::Init()
{
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->AddObserver(this, "xpcom-shutdown", false);
        obs->AddObserver(this, NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC, false);
        obs->AddObserver(this, "child-memory-reporter-request", false);
        obs->AddObserver(this, "memory-pressure", false);
        obs->AddObserver(this, "child-gc-request", false);
        obs->AddObserver(this, "child-cc-request", false);
        obs->AddObserver(this, "last-pb-context-exited", false);
        obs->AddObserver(this, "file-watcher-update", false);
#ifdef MOZ_WIDGET_GONK
        obs->AddObserver(this, NS_VOLUME_STATE_CHANGED, false);
        obs->AddObserver(this, "phone-state-changed", false);
#endif
#ifdef ACCESSIBILITY
        obs->AddObserver(this, "a11y-init-or-shutdown", false);
#endif
    }
    Preferences::AddStrongObserver(this, "");
    nsCOMPtr<nsIThreadInternal>
            threadInt(do_QueryInterface(NS_GetCurrentThread()));
    if (threadInt) {
        threadInt->AddObserver(this);
    }
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

    void Init(nsIDOMMozWakeLock* aWakeLock)
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

        mWakeLock->Unlock();

        if (mTimer) {
            mTimer->Cancel();
            mTimer = nullptr;
        }
    }

    nsCOMPtr<nsIDOMMozWakeLock> mWakeLock;
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
    nsCOMPtr<nsIDOMMozWakeLock> lock =
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

    nsCOMPtr<mozIDOMApplication> domApp;
    appsService->GetAppByManifestURL(aManifestURL, getter_AddRefs(domApp));

    nsCOMPtr<mozIApplication> app = do_QueryInterface(domApp);
    if (!app) {
        return;
    }

    app->GetName(aName);
}

bool
ContentParent::TransformPreallocatedIntoApp(const nsAString& aAppManifestURL,
                                            ChildPrivileges aPrivs)
{
    MOZ_ASSERT(IsPreallocated());
    mAppManifestURL = aAppManifestURL;
    TryGetNameFromManifestURL(aAppManifestURL, mAppName);

    return SendSetProcessPrivileges(aPrivs);
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
        AsyncChannel* channel = GetIPCChannel();
        if (channel) {
            mCalledCloseWithError = true;
            channel->CloseWithError();
        }
    }

    
    
    MarkAsDead();

    
    
    
    
    mChildReporter = nullptr;
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
                sAppContentParents = NULL;
            }
        }
    } else if (sNonAppContentParents) {
        sNonAppContentParents->RemoveElement(this);
        if (!sNonAppContentParents->Length()) {
            delete sNonAppContentParents;
            sNonAppContentParents = NULL;
        }
    }

    if (sPrivateContent) {
        sPrivateContent->RemoveElement(this);
        if (!sPrivateContent->Length()) {
            delete sPrivateContent;
            sPrivateContent = NULL;
        }
    }

    mIsAlive = false;
}

void
ContentParent::OnChannelConnected(int32_t pid)
{
    ProcessHandle handle;
    if (!base::OpenPrivilegedProcessHandle(pid, &handle)) {
        NS_WARNING("Can't open handle to child process.");
    }
    else {
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
                          nullptr, nullptr, nullptr);
    }
    nsCOMPtr<nsIThreadObserver>
        kungFuDeathGrip(static_cast<nsIThreadObserver*>(this));
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->RemoveObserver(static_cast<nsIObserver*>(this), "xpcom-shutdown");
        obs->RemoveObserver(static_cast<nsIObserver*>(this), "memory-pressure");
        obs->RemoveObserver(static_cast<nsIObserver*>(this), "child-memory-reporter-request");
        obs->RemoveObserver(static_cast<nsIObserver*>(this), NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC);
        obs->RemoveObserver(static_cast<nsIObserver*>(this), "child-gc-request");
        obs->RemoveObserver(static_cast<nsIObserver*>(this), "child-cc-request");
        obs->RemoveObserver(static_cast<nsIObserver*>(this), "last-pb-context-exited");
        obs->RemoveObserver(static_cast<nsIObserver*>(this), "file-watcher-update");
#ifdef MOZ_WIDGET_GONK
        obs->RemoveObserver(static_cast<nsIObserver*>(this), NS_VOLUME_STATE_CHANGED);
#endif
#ifdef ACCESSIBILITY
        obs->RemoveObserver(static_cast<nsIObserver*>(this), "a11y-init-or-shutdown");
#endif
    }

    if (ppm) {
      ppm->Disconnect();
    }

    
    UnregisterChildMemoryReporter();

    
    Preferences::RemoveObserver(this, "");

    RecvRemoveGeolocationListener();

    mConsoleService = nullptr;

    nsCOMPtr<nsIThreadInternal>
        threadInt(do_QueryInterface(NS_GetCurrentThread()));
    if (threadInt)
        threadInt->RemoveObserver(this);

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

                crashReporter->GenerateCrashReport(this, NULL);

                nsAutoString dumpID(crashReporter->ChildDumpID());
                props->SetPropertyAsAString(NS_LITERAL_STRING("dumpID"), dumpID);
            }
#endif
        }
        obs->NotifyObservers((nsIPropertyBag2*) props, "ipc:content-shutdown", nullptr);
    }

    
    
    
    ShutDownProcess( true);

    MessageLoop::current()->
        PostTask(FROM_HERE,
                 NewRunnableFunction(DelayedDeleteSubprocess, mSubprocess));
    mSubprocess = NULL;

    
    
    
    
    
    
    
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

ContentParent::ContentParent(mozIApplication* aApp,
                             bool aIsForBrowser,
                             bool aIsForPreallocated,
                             ChildPrivileges aOSPrivileges,
                             ProcessPriority aInitialPriority )
    : mSubprocess(nullptr)
    , mOSPrivileges(aOSPrivileges)
    , mChildID(gContentChildID++)
    , mGeolocationWatchID(-1)
    , mForceKillTask(nullptr)
    , mNumDestroyingTabs(0)
    , mIsAlive(true)
    , mSendPermissionUpdates(false)
    , mIsForBrowser(aIsForBrowser)
    , mCalledClose(false)
    , mCalledCloseWithError(false)
    , mCalledKillHard(false)
{
    
    
    MOZ_ASSERT(!!aApp + aIsForBrowser + aIsForPreallocated <= 1);

    
    if (!sContentParents) {
        sContentParents = new LinkedList<ContentParent>();
    }
    sContentParents->insertBack(this);

    if (aApp) {
        aApp->GetManifestURL(mAppManifestURL);
        aApp->GetName(mAppName);
    } else if (aIsForPreallocated) {
        mAppManifestURL = MAGIC_PREALLOCATED_APP_MANIFEST_URL;
    }

    
    
    nsDebugImpl::SetMultiprocessMode("Parent");

    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    mSubprocess = new GeckoChildProcessHost(GeckoProcessType_Content,
                                            aOSPrivileges);

    mSubprocess->LaunchAndWaitForProcessHandle();

    Open(mSubprocess->GetChannel(), mSubprocess->GetChildProcessHandle());

    
    
    
    
    
    
    ProcessPriorityManager::SetProcessPriority(this, aInitialPriority);

    
    
    
    
    
    
    
    
    bool useOffMainThreadCompositing = !!CompositorParent::CompositorLoop();
    if (useOffMainThreadCompositing) {
        DebugOnly<bool> opened = PCompositor::Open(this);
        MOZ_ASSERT(opened);

        if (Preferences::GetBool("layers.async-video.enabled",false)) {
            opened = PImageBridge::Open(this);
            MOZ_ASSERT(opened);
        }
    }

    nsCOMPtr<nsIChromeRegistry> registrySvc = nsChromeRegistry::GetService();
    nsChromeRegistryChrome* chromeRegistry =
        static_cast<nsChromeRegistryChrome*>(registrySvc.get());
    chromeRegistry->SendRegisteredChrome(this);
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

}

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
                                text.Length() * sizeof(PRUnichar));
    NS_ENSURE_SUCCESS(rv, true);
    
    clipboard->SetData(trans, NULL, whichClipboard);
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
    
    clipboard->GetData(trans, whichClipboard);
    nsCOMPtr<nsISupports> tmp;
    uint32_t len;
    rv = trans->GetTransferData(kUnicodeMime, getter_AddRefs(tmp), &len);
    if (NS_FAILED(rv))
        return false;

    nsCOMPtr<nsISupportsString> supportsString = do_QueryInterface(tmp);
    
    if (!supportsString)
        return false;
    supportsString->GetData(*text);
    return true;
}

bool
ContentParent::RecvEmptyClipboard()
{
    nsresult rv;
    nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
    NS_ENSURE_SUCCESS(rv, true);

    clipboard->EmptyClipboard(nsIClipboard::kGlobalClipboard);

    return true;
}

bool
ContentParent::RecvClipboardHasText(bool* hasText)
{
    nsresult rv;
    nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
    NS_ENSURE_SUCCESS(rv, true);

    clipboard->HasDataMatchingFlavors(sClipboardTextFlavors, 1, 
                                      nsIClipboard::kGlobalClipboard, hasText);
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
    if (AndroidBridge::Bridge() != nullptr)
        *showPassword = AndroidBridge::Bridge()->GetShowPasswordSetting();
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
ContentParent::RecvAudioChannelRegisterType(const AudioChannelType& aType)
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetAudioChannelService();
    if (service) {
        service->RegisterType(aType, mChildID);
    }
    return true;
}

bool
ContentParent::RecvAudioChannelUnregisterType(const AudioChannelType& aType,
                                              const bool& aElementHidden)
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetAudioChannelService();
    if (service) {
        service->UnregisterType(aType, aElementHidden, mChildID);
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
ContentParent::RecvRecordingDeviceEvents(const nsString& aRecordingStatus)
{
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->NotifyObservers(nullptr, "recording-device-events", aRecordingStatus.get());
    } else {
        NS_WARNING("Could not get the Observer service for ContentParent::RecvRecordingDeviceEvents.");
    }
    return true;
}

NS_IMPL_ISUPPORTS3(ContentParent,
                   nsIObserver,
                   nsIThreadObserver,
                   nsIDOMGeoPositionCallback)

NS_IMETHODIMP
ContentParent::Observe(nsISupports* aSubject,
                       const char* aTopic,
                       const PRUnichar* aData)
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
        unused << SendPMemoryReportRequestConstructor();
    }
    else if (!strcmp(aTopic, "child-gc-request")){
        unused << SendGarbageCollect();
    }
    else if (!strcmp(aTopic, "child-cc-request")){
        unused << SendCycleCollect();
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

        vol->GetName(volName);
        vol->GetMountPoint(mountPoint);
        vol->GetState(&state);
        vol->GetMountGeneration(&mountGeneration);
        vol->GetIsMediaPresent(&isMediaPresent);
        vol->GetIsSharing(&isSharing);

        unused << SendFileSystemUpdate(volName, mountPoint, state,
                                       mountGeneration, isMediaPresent,
                                       isSharing);
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
        return NULL;
    }
    return parent;
}

bool
ContentParent::DeallocPJavaScriptParent(PJavaScriptParent *parent)
{
    static_cast<mozilla::jsipc::JavaScriptParent *>(parent)->destroyFromContent();
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

    TabParent* parent = new TabParent(this, tc.GetTabContext());

    
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
  return result.forget().get();
}

bool
ContentParent::DeallocPDeviceStorageRequestParent(PDeviceStorageRequestParent* doomed)
{
  DeviceStorageRequestParent *parent = static_cast<DeviceStorageRequestParent*>(doomed);
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
ContentParent::AllocPMemoryReportRequestParent()
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

void
ContentParent::SetChildMemoryReports(const InfallibleTArray<MemoryReport>& childReports)
{
    nsCOMPtr<nsIMemoryReporterManager> mgr =
        do_GetService("@mozilla.org/memory-reporter-manager;1");

    if (mChildReporter)
        mgr->UnregisterReporter(mChildReporter);

    mChildReporter = new ChildReporter(childReports);
    mgr->RegisterReporter(mChildReporter);

    nsCOMPtr<nsIObserverService> obs =
        do_GetService("@mozilla.org/observer-service;1");
    if (obs)
        obs->NotifyObservers(nullptr, "child-memory-reporter-update", nullptr);
}

void
ContentParent::UnregisterChildMemoryReporter()
{
    nsCOMPtr<nsIMemoryReporterManager> mgr =
        do_GetService("@mozilla.org/memory-reporter-manager;1");
    mgr->UnregisterReporter(mChildReporter);
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
                                             const bool& aForceSave,
                                             const int64_t& aContentLength,
                                             const OptionalURIParams& aReferrer,
                                             PBrowserParent* aBrowser)
{
    ExternalHelperAppParent *parent = new ExternalHelperAppParent(uri, aContentLength);
    parent->AddRef();
    parent->Init(this, aMimeContentType, aContentDisposition, aForceSave, aReferrer, aBrowser);
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
ContentParent::RecvShowFilePicker(const int16_t& mode,
                                  const int16_t& selectedType,
                                  const bool& addToRecentDocs,
                                  const nsString& title,
                                  const nsString& defaultFile,
                                  const nsString& defaultExtension,
                                  const InfallibleTArray<nsString>& filters,
                                  const InfallibleTArray<nsString>& filterNames,
                                  InfallibleTArray<nsString>* files,
                                  int16_t* retValue,
                                  nsresult* result)
{
    nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1");
    if (!filePicker) {
        *result = NS_ERROR_NOT_AVAILABLE;
        return true;
    }

    
    
    nsCOMPtr<nsIWindowWatcher> ww = do_GetService(NS_WINDOWWATCHER_CONTRACTID);
    nsCOMPtr<nsIDOMWindow> window;
    ww->GetActiveWindow(getter_AddRefs(window));

    
    *result = filePicker->Init(window, title, mode);
    if (NS_FAILED(*result))
        return true;

    filePicker->SetAddToRecentDocs(addToRecentDocs);

    uint32_t count = filters.Length();
    for (uint32_t i = 0; i < count; ++i) {
        filePicker->AppendFilter(filterNames[i], filters[i]);
    }

    filePicker->SetDefaultString(defaultFile);
    filePicker->SetDefaultExtension(defaultExtension);
    filePicker->SetFilterIndex(selectedType);

    
    *result = filePicker->Show(retValue);
    if (NS_FAILED(*result))
        return true;

    if (mode == nsIFilePicker::modeOpenMultiple) {
        nsCOMPtr<nsISimpleEnumerator> fileIter;
        *result = filePicker->GetFiles(getter_AddRefs(fileIter));

        nsCOMPtr<nsIFile> singleFile;
        bool loop = true;
        while (NS_SUCCEEDED(fileIter->HasMoreElements(&loop)) && loop) {
            fileIter->GetNext(getter_AddRefs(singleFile));
            if (singleFile) {
                nsAutoString filePath;
                singleFile->GetPath(filePath);
                files->AppendElement(filePath);
            }
        }
        return true;
    }
    nsCOMPtr<nsIFile> file;
    filePicker->GetFile(getter_AddRefs(file));

    
    if (file) {
        nsAutoString filePath;
        file->GetPath(filePath);
        files->AppendElement(filePath);
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


NS_IMETHODIMP
ContentParent::OnDispatchedEvent(nsIThreadInternal *thread)
{
   NS_NOTREACHED("OnDispatchedEvent unimplemented");
   return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
ContentParent::OnProcessNextEvent(nsIThreadInternal *thread,
                                  bool mayWait,
                                  uint32_t recursionDepth)
{
    return NS_OK;
}


NS_IMETHODIMP
ContentParent::AfterProcessNextEvent(nsIThreadInternal *thread,
                                     uint32_t recursionDepth)
{
    return NS_OK;
}

bool
ContentParent::RecvShowAlertNotification(const nsString& aImageUrl, const nsString& aTitle,
                                         const nsString& aText, const bool& aTextClickable,
                                         const nsString& aCookie, const nsString& aName,
                                         const nsString& aBidi, const nsString& aLang)
{
    if (!AssertAppProcessPermission(this, "desktop-notification")) {
        return false;
    }
    nsCOMPtr<nsIAlertsService> sysAlerts(do_GetService(NS_ALERTSERVICE_CONTRACTID));
    if (sysAlerts) {
        sysAlerts->ShowAlertNotification(aImageUrl, aTitle, aText, aTextClickable,
                                         aCookie, this, aName, aBidi, aLang);
    }

    return true;
}

bool
ContentParent::RecvCloseAlert(const nsString& aName)
{
    if (!AssertAppProcessPermission(this, "desktop-notification")) {
        return false;
    }
    nsCOMPtr<nsIAlertsService> sysAlerts(do_GetService(NS_ALERTSERVICE_CONTRACTID));
    if (sysAlerts) {
        sysAlerts->CloseAlert(aName);
    }

    return true;
}

bool
ContentParent::RecvSyncMessage(const nsString& aMsg,
                               const ClonedMessageData& aData,
                               const InfallibleTArray<CpowEntry>& aCpows,
                               InfallibleTArray<nsString>* aRetvals)
{
  nsRefPtr<nsFrameMessageManager> ppm = mMessageManager;
  if (ppm) {
    StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForParent(aData);
    CpowIdHolder cpows(GetCPOWManager(), aCpows);
    ppm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm.get()),
                        aMsg, true, &cloneData, &cpows, aRetvals);
  }
  return true;
}

bool
ContentParent::RecvAsyncMessage(const nsString& aMsg,
                                const ClonedMessageData& aData,
                                const InfallibleTArray<CpowEntry>& aCpows)
{
  nsRefPtr<nsFrameMessageManager> ppm = mMessageManager;
  if (ppm) {
    StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForParent(aData);
    CpowIdHolder cpows(GetCPOWManager(), aCpows);
    ppm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm.get()),
                        aMsg, false, &cloneData, &cpows, nullptr);
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
  if (Preferences::GetBool("geo.testing.ignore_ipc_principal", false) == false) {
    nsIPrincipal* principal = aPrincipal;
    if (principal == nullptr) {
      KillHard();
      return true;
    }

    uint32_t principalAppId;
    nsresult rv = principal->GetAppId(&principalAppId);
    if (NS_FAILED(rv)) {
      return true;
    }

    bool found = false;
    const InfallibleTArray<PBrowserParent*>& browsers = ManagedPBrowserParent();
    for (uint32_t i = 0; i < browsers.Length(); ++i) {
      TabParent* tab = static_cast<TabParent*>(browsers[i]);
      nsCOMPtr<mozIApplication> app = tab->GetOwnOrContainingApp();
      uint32_t appId;
      app->GetLocalId(&appId);
      if (appId == principalAppId) {
        found = true;
        break;
      }
    }

    if (!found) {
      return true;
    }

    
    
    nsCOMPtr<nsIPermissionManager> pm = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
    if (!pm) {
      return false;
    }

    uint32_t permission = nsIPermissionManager::UNKNOWN_ACTION;
    rv = pm->TestPermissionFromPrincipal(principal, "geolocation", &permission);
    if (NS_FAILED(rv) || permission != nsIPermissionManager::ALLOW_ACTION) {
      KillHard();
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
      sPrivateContent = NULL;
    }
  }
  return true;
}

bool
ContentParent::DoSendAsyncMessage(JSContext* aCx,
                                  const nsAString& aMessage,
                                  const mozilla::dom::StructuredCloneData& aData,
                                  JS::Handle<JSObject *> aCpows)
{
  ClonedMessageData data;
  if (!BuildClonedMessageDataForParent(this, aData, data)) {
    return false;
  }
  InfallibleTArray<CpowEntry> cpows;
  if (!GetCPOWManager()->Wrap(aCx, aCpows, &cpows)) {
    return false;
  }
  return SendAsyncMessage(nsString(aMessage), data, cpows);
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
  
  
  MOZ_ASSERT(Preferences::GetBool("browser.tabs.remote", false));
  return false;
}

} 
} 
