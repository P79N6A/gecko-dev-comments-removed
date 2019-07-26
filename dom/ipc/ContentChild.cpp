





#ifdef MOZ_WIDGET_GTK
#include <gtk/gtk.h>
#endif

#ifdef MOZ_WIDGET_QT
#include "nsQAppInstance.h"
#endif

#include "ContentChild.h"
#include "CrashReporterChild.h"
#include "FileDescriptorSetChild.h"
#include "TabChild.h"

#include "mozilla/Attributes.h"
#include "mozilla/dom/asmjscache/AsmJSCache.h"
#include "mozilla/dom/asmjscache/PAsmJSCacheEntryChild.h"
#include "mozilla/dom/ExternalHelperAppChild.h"
#include "mozilla/dom/PCrashReporterChild.h"
#include "mozilla/dom/DOMStorageIPC.h"
#include "mozilla/hal_sandbox/PHalChild.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "mozilla/ipc/TestShellChild.h"
#include "mozilla/layers/CompositorChild.h"
#include "mozilla/layers/ImageBridgeChild.h"
#include "mozilla/layers/SharedBufferManagerChild.h"
#include "mozilla/layers/PCompositorChild.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/Preferences.h"

#if defined(MOZ_CONTENT_SANDBOX)
#if defined(XP_WIN)
#define TARGET_SANDBOX_EXPORTS
#include "mozilla/sandboxTarget.h"
#elif defined(XP_LINUX)
#include "mozilla/Sandbox.h"
#endif
#endif

#include "mozilla/unused.h"

#include "nsIConsoleListener.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIMemoryReporter.h"
#include "nsIMemoryInfoDumper.h"
#include "nsIMutable.h"
#include "nsIObserverService.h"
#include "nsIScriptSecurityManager.h"
#include "nsServiceManagerUtils.h"
#include "nsStyleSheetService.h"
#include "nsXULAppAPI.h"
#include "nsIScriptError.h"
#include "nsIConsoleService.h"
#include "nsJSEnvironment.h"
#include "SandboxHal.h"
#include "nsDebugImpl.h"
#include "nsHashPropertyBag.h"
#include "nsLayoutStylesheetCache.h"
#include "nsIJSRuntimeService.h"
#include "nsThreadManager.h"

#include "IHistory.h"
#include "nsNetUtil.h"

#include "base/message_loop.h"
#include "base/process_util.h"
#include "base/task.h"

#include "nsChromeRegistryContent.h"
#include "nsFrameMessageManager.h"

#include "nsIGeolocationProvider.h"
#include "mozilla/dom/PMemoryReportRequestChild.h"

#ifdef MOZ_PERMISSIONS
#include "nsIScriptSecurityManager.h"
#include "nsPermission.h"
#include "nsPermissionManager.h"
#endif

#include "PermissionMessageUtils.h"

#if defined(MOZ_WIDGET_ANDROID)
#include "APKOpen.h"
#endif

#if defined(MOZ_WIDGET_GONK)
#include "nsVolume.h"
#include "nsVolumeService.h"
#include "SpeakerManagerService.h"
#endif

#ifdef XP_WIN
#include <process.h>
#define getpid _getpid
#endif

#ifdef MOZ_X11
#include "mozilla/X11Util.h"
#endif

#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif

#ifdef MOZ_NUWA_PROCESS
#include <setjmp.h>
#include "ipc/Nuwa.h"
#endif

#include "mozilla/dom/indexedDB/PIndexedDBChild.h"
#include "mozilla/dom/mobilemessage/SmsChild.h"
#include "mozilla/dom/devicestorage/DeviceStorageRequestChild.h"
#include "mozilla/dom/PFileSystemRequestChild.h"
#include "mozilla/dom/FileSystemTaskBase.h"
#include "mozilla/dom/bluetooth/PBluetoothChild.h"
#include "mozilla/dom/PFMRadioChild.h"
#include "mozilla/ipc/InputStreamUtils.h"

#ifdef MOZ_WEBSPEECH
#include "mozilla/dom/PSpeechSynthesisChild.h"
#endif

#include "nsDOMFile.h"
#include "nsIRemoteBlob.h"
#include "ProcessUtils.h"
#include "StructuredCloneUtils.h"
#include "URIUtils.h"
#include "nsContentUtils.h"
#include "nsIPrincipal.h"
#include "nsDeviceStorage.h"
#include "AudioChannelService.h"
#include "JavaScriptChild.h"
#include "mozilla/dom/telephony/PTelephonyChild.h"
#include "mozilla/dom/time/DateCacheCleaner.h"
#include "mozilla/net/NeckoMessageUtils.h"

using namespace base;
using namespace mozilla;
using namespace mozilla::docshell;
using namespace mozilla::dom::bluetooth;
using namespace mozilla::dom::devicestorage;
using namespace mozilla::dom::ipc;
using namespace mozilla::dom::mobilemessage;
using namespace mozilla::dom::indexedDB;
using namespace mozilla::dom::telephony;
using namespace mozilla::hal_sandbox;
using namespace mozilla::ipc;
using namespace mozilla::layers;
using namespace mozilla::net;
using namespace mozilla::jsipc;
#if defined(MOZ_WIDGET_GONK)
using namespace mozilla::system;
#endif

#ifdef MOZ_NUWA_PROCESS
static bool sNuwaForking = false;





#define RESERVED_INT_STACK 128


#define STACK_SENTINEL_VALUE 0xdeadbeef
#endif

namespace mozilla {
namespace dom {

class MemoryReportRequestChild : public PMemoryReportRequestChild,
                                 public nsIRunnable
{
public:
    NS_DECL_ISUPPORTS

    MemoryReportRequestChild(uint32_t aGeneration, const nsAString& aDMDDumpIdent);
    virtual ~MemoryReportRequestChild();
    NS_IMETHOD Run();
private:
    uint32_t mGeneration;
    nsString mDMDDumpIdent;
};

NS_IMPL_ISUPPORTS(MemoryReportRequestChild, nsIRunnable)

MemoryReportRequestChild::MemoryReportRequestChild(uint32_t aGeneration, const nsAString& aDMDDumpIdent)
: mGeneration(aGeneration), mDMDDumpIdent(aDMDDumpIdent)
{
    MOZ_COUNT_CTOR(MemoryReportRequestChild);
}

MemoryReportRequestChild::~MemoryReportRequestChild()
{
    MOZ_COUNT_DTOR(MemoryReportRequestChild);
}

class AlertObserver
{
public:

    AlertObserver(nsIObserver *aObserver, const nsString& aData)
        : mObserver(aObserver)
        , mData(aData)
    {
    }

    ~AlertObserver() {}

    bool ShouldRemoveFrom(nsIObserver* aObserver,
                          const nsString& aData) const
    {
        return (mObserver == aObserver &&
                mData == aData);
    }

    bool Observes(const nsString& aData) const
    {
        return mData.Equals(aData);
    }

    bool Notify(const nsCString& aType) const
    {
        mObserver->Observe(nullptr, aType.get(), mData.get());
        return true;
    }

private:
    nsCOMPtr<nsIObserver> mObserver;
    nsString mData;
};

class ConsoleListener MOZ_FINAL : public nsIConsoleListener
{
public:
    ConsoleListener(ContentChild* aChild)
    : mChild(aChild) {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSICONSOLELISTENER

private:
    ContentChild* mChild;
    friend class ContentChild;
};

NS_IMPL_ISUPPORTS(ConsoleListener, nsIConsoleListener)

NS_IMETHODIMP
ConsoleListener::Observe(nsIConsoleMessage* aMessage)
{
    if (!mChild)
        return NS_OK;
    
    nsCOMPtr<nsIScriptError> scriptError = do_QueryInterface(aMessage);
    if (scriptError) {
        nsString msg, sourceName, sourceLine;
        nsXPIDLCString category;
        uint32_t lineNum, colNum, flags;

        nsresult rv = scriptError->GetErrorMessage(msg);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = scriptError->GetSourceName(sourceName);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = scriptError->GetSourceLine(sourceLine);
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        
        
        
        if (sourceLine.Length() > 1000) {
            sourceLine.Truncate(1000);
        }

        rv = scriptError->GetCategory(getter_Copies(category));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = scriptError->GetLineNumber(&lineNum);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = scriptError->GetColumnNumber(&colNum);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = scriptError->GetFlags(&flags);
        NS_ENSURE_SUCCESS(rv, rv);
        mChild->SendScriptError(msg, sourceName, sourceLine,
                               lineNum, colNum, flags, category);
        return NS_OK;
    }

    nsXPIDLString msg;
    nsresult rv = aMessage->GetMessageMoz(getter_Copies(msg));
    NS_ENSURE_SUCCESS(rv, rv);
    mChild->SendConsoleMessage(msg);
    return NS_OK;
}

class SystemMessageHandledObserver MOZ_FINAL : public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    void Init();
};

void SystemMessageHandledObserver::Init()
{
    nsCOMPtr<nsIObserverService> os =
        mozilla::services::GetObserverService();

    if (os) {
        os->AddObserver(this, "handle-system-messages-done",
                         false);
    }
}

NS_IMETHODIMP
SystemMessageHandledObserver::Observe(nsISupports* aSubject,
                                      const char* aTopic,
                                      const char16_t* aData)
{
    if (ContentChild::GetSingleton()) {
        ContentChild::GetSingleton()->SendSystemMessageHandled();
    }
    return NS_OK;
}

NS_IMPL_ISUPPORTS(SystemMessageHandledObserver, nsIObserver)

class BackgroundChildPrimer MOZ_FINAL :
  public nsIIPCBackgroundChildCreateCallback
{
public:
    BackgroundChildPrimer()
    { }

    NS_DECL_ISUPPORTS

private:
    ~BackgroundChildPrimer()
    { }

    virtual void
    ActorCreated(PBackgroundChild* aActor) MOZ_OVERRIDE
    {
        MOZ_ASSERT(aActor, "Failed to create a PBackgroundChild actor!");
    }

    virtual void
    ActorFailed() MOZ_OVERRIDE
    {
        MOZ_CRASH("Failed to create a PBackgroundChild actor!");
    }
};

NS_IMPL_ISUPPORTS(BackgroundChildPrimer, nsIIPCBackgroundChildCreateCallback)

ContentChild* ContentChild::sSingleton;



static void
InitOnContentProcessCreated()
{
    
    mozilla::dom::time::InitializeDateCacheCleaner();
}

ContentChild::ContentChild()
 : mID(uint64_t(-1))
#ifdef ANDROID
   ,mScreenSize(0, 0)
#endif
   , mCanOverrideProcessName(true)
{
    
    
    nsDebugImpl::SetMultiprocessMode("Child");
}

ContentChild::~ContentChild()
{
}

bool
ContentChild::Init(MessageLoop* aIOLoop,
                   base::ProcessHandle aParentHandle,
                   IPC::Channel* aChannel)
{
#ifdef MOZ_WIDGET_GTK
    
    gtk_init(nullptr, nullptr);
#endif

#ifdef MOZ_WIDGET_QT
    
    nsQAppInstance::AddRef();
#endif

#ifdef MOZ_X11
    
    XRE_InstallX11ErrorHandler();
#endif

#ifdef MOZ_NUWA_PROCESS
    SetTransport(aChannel);
#endif

    NS_ASSERTION(!sSingleton, "only one ContentChild per child");

    
    
    
    nsresult rv = nsThreadManager::get()->Init();
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return false;
    }

    Open(aChannel, aParentHandle, aIOLoop);
    sSingleton = this;

#ifdef MOZ_X11
    
    
    int xSocketFd = ConnectionNumber(DefaultXDisplay());
    SendBackUpXResources(FileDescriptor(xSocketFd));
#endif

#ifdef MOZ_CRASHREPORTER
    SendPCrashReporterConstructor(CrashReporter::CurrentThreadId(),
                                  XRE_GetProcessType());
#endif

    GetCPOWManager();

    InitProcessAttributes();

    return true;
}

void
ContentChild::InitProcessAttributes()
{
    SendGetProcessAttributes(&mID, &mIsForApp, &mIsForBrowser);

#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        SetProcessName(NS_LITERAL_STRING("(Nuwa)"), false);
        return;
    }
#endif
    if (mIsForApp && !mIsForBrowser) {
        SetProcessName(NS_LITERAL_STRING("(Preallocated app)"), false);
    } else {
        SetProcessName(NS_LITERAL_STRING("Browser"), false);
    }

}

void
ContentChild::SetProcessName(const nsAString& aName, bool aDontOverride)
{
    if (!mCanOverrideProcessName) {
        return;
    }

    char* name;
    if ((name = PR_GetEnv("MOZ_DEBUG_APP_PROCESS")) &&
        aName.EqualsASCII(name)) {
#ifdef OS_POSIX
        printf_stderr("\n\nCHILDCHILDCHILDCHILD\n  [%s] debug me @%d\n\n", name, getpid());
        sleep(30);
#elif defined(OS_WIN)
        
        
        NS_DebugBreak(NS_DEBUG_BREAK,
                      "Invoking NS_DebugBreak() to debug child process",
                      nullptr, __FILE__, __LINE__);
#endif
    }

    mProcessName = aName;
    mozilla::ipc::SetThisProcessName(NS_LossyConvertUTF16toASCII(aName).get());

    if (aDontOverride) {
        mCanOverrideProcessName = false;
    }
}

void
ContentChild::GetProcessName(nsAString& aName)
{
    aName.Assign(mProcessName);
}

void
ContentChild::GetProcessName(nsACString& aName)
{
    aName.Assign(NS_ConvertUTF16toUTF8(mProcessName));
}

 void
ContentChild::AppendProcessId(nsACString& aName)
{
    if (!aName.IsEmpty()) {
        aName.AppendLiteral(" ");
    }
    unsigned pid = getpid();
    aName.Append(nsPrintfCString("(pid %u)", pid));
}

void
ContentChild::InitXPCOM()
{
    
    
    BackgroundChild::Startup();

    nsCOMPtr<nsIIPCBackgroundChildCreateCallback> callback =
        new BackgroundChildPrimer();
    if (!BackgroundChild::GetOrCreateForCurrentThread(callback)) {
        MOZ_CRASH("Failed to create PBackgroundChild!");
    }

    nsCOMPtr<nsIConsoleService> svc(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
    if (!svc) {
        NS_WARNING("Couldn't acquire console service");
        return;
    }

    mConsoleListener = new ConsoleListener(this);
    if (NS_FAILED(svc->RegisterListener(mConsoleListener)))
        NS_WARNING("Couldn't register console listener for child process");

    bool isOffline;
    SendGetXPCOMProcessAttributes(&isOffline);
    RecvSetOffline(isOffline);

    DebugOnly<FileUpdateDispatcher*> observer = FileUpdateDispatcher::GetSingleton();
    NS_ASSERTION(observer, "FileUpdateDispatcher is null");

    
    nsRefPtr<SystemMessageHandledObserver> sysMsgObserver =
        new SystemMessageHandledObserver();
    sysMsgObserver->Init();

#ifndef MOZ_NUWA_PROCESS
    InitOnContentProcessCreated();
#endif
}

PMemoryReportRequestChild*
ContentChild::AllocPMemoryReportRequestChild(const uint32_t& generation,
                                             const bool &minimizeMemoryUsage,
                                             const nsString& aDMDDumpIdent)
{
    MemoryReportRequestChild *actor = new MemoryReportRequestChild(generation, aDMDDumpIdent);
    actor->AddRef();
    return actor;
}



class MemoryReportsWrapper MOZ_FINAL : public nsISupports {
public:
    NS_DECL_ISUPPORTS
    MemoryReportsWrapper(InfallibleTArray<MemoryReport> *r) : mReports(r) { }
    InfallibleTArray<MemoryReport> *mReports;
};
NS_IMPL_ISUPPORTS0(MemoryReportsWrapper)

class MemoryReportCallback MOZ_FINAL : public nsIMemoryReporterCallback
{
public:
    NS_DECL_ISUPPORTS

    MemoryReportCallback(const nsACString &aProcess)
    : mProcess(aProcess)
    {
    }

    NS_IMETHOD Callback(const nsACString &aProcess, const nsACString &aPath,
                        int32_t aKind, int32_t aUnits, int64_t aAmount,
                        const nsACString &aDescription,
                        nsISupports *aiWrappedReports)
    {
        MemoryReportsWrapper *wrappedReports =
            static_cast<MemoryReportsWrapper *>(aiWrappedReports);

        MemoryReport memreport(mProcess, nsCString(aPath), aKind, aUnits,
                               aAmount, nsCString(aDescription));
        wrappedReports->mReports->AppendElement(memreport);
        return NS_OK;
    }
private:
    const nsCString mProcess;
};
NS_IMPL_ISUPPORTS(
  MemoryReportCallback
, nsIMemoryReporterCallback
)

bool
ContentChild::RecvPMemoryReportRequestConstructor(
    PMemoryReportRequestChild* child,
    const uint32_t& generation,
    const bool& minimizeMemoryUsage,
    const nsString& aDMDDumpIdent)
{
    MemoryReportRequestChild *actor = static_cast<MemoryReportRequestChild*>(child);
    nsresult rv;

    if (minimizeMemoryUsage) {
        nsCOMPtr<nsIMemoryReporterManager> mgr = do_GetService("@mozilla.org/memory-reporter-manager;1");
        rv = mgr->MinimizeMemoryUsage(actor);
        
    } else {
        rv = actor->Run();
    }

    return !NS_WARN_IF(NS_FAILED(rv));
}

NS_IMETHODIMP MemoryReportRequestChild::Run()
{
    ContentChild *child = static_cast<ContentChild*>(Manager());
    nsCOMPtr<nsIMemoryReporterManager> mgr = do_GetService("@mozilla.org/memory-reporter-manager;1");

    InfallibleTArray<MemoryReport> reports;

    nsCString process;
    child->GetProcessName(process);
    child->AppendProcessId(process);

    
    
    nsRefPtr<MemoryReportsWrapper> wrappedReports =
        new MemoryReportsWrapper(&reports);
    nsRefPtr<MemoryReportCallback> cb = new MemoryReportCallback(process);
    mgr->GetReportsForThisProcessExtended(cb, wrappedReports, mDMDDumpIdent);

    bool sent = Send__delete__(this, mGeneration, reports);
    return sent ? NS_OK : NS_ERROR_FAILURE;
}

bool
ContentChild::RecvAudioChannelNotify()
{
    nsRefPtr<AudioChannelService> service =
        AudioChannelService::GetAudioChannelService();
    if (service) {
        service->Notify();
    }
    return true;
}

bool
ContentChild::DeallocPMemoryReportRequestChild(PMemoryReportRequestChild* actor)
{
    static_cast<MemoryReportRequestChild*>(actor)->Release();
    return true;
}

bool
ContentChild::RecvDumpGCAndCCLogsToFile(const nsString& aIdentifier,
                                        const bool& aDumpAllTraces,
                                        const bool& aDumpChildProcesses)
{
    nsCOMPtr<nsIMemoryInfoDumper> dumper = do_GetService("@mozilla.org/memory-info-dumper;1");

    nsString gcLogPath, ccLogPath;
    dumper->DumpGCAndCCLogsToFile(aIdentifier, aDumpAllTraces,
                                  aDumpChildProcesses, gcLogPath, ccLogPath);
    return true;
}

PCompositorChild*
ContentChild::AllocPCompositorChild(mozilla::ipc::Transport* aTransport,
                                    base::ProcessId aOtherProcess)
{
    return CompositorChild::Create(aTransport, aOtherProcess);
}

PSharedBufferManagerChild*
ContentChild::AllocPSharedBufferManagerChild(mozilla::ipc::Transport* aTransport,
                                              base::ProcessId aOtherProcess)
{
    return SharedBufferManagerChild::StartUpInChildProcess(aTransport, aOtherProcess);
}

PImageBridgeChild*
ContentChild::AllocPImageBridgeChild(mozilla::ipc::Transport* aTransport,
                                     base::ProcessId aOtherProcess)
{
    return ImageBridgeChild::StartUpInChildProcess(aTransport, aOtherProcess);
}

PBackgroundChild*
ContentChild::AllocPBackgroundChild(Transport* aTransport,
                                    ProcessId aOtherProcess)
{
    return BackgroundChild::Alloc(aTransport, aOtherProcess);
}

bool
ContentChild::RecvSetProcessSandbox()
{
  
  
#if defined(MOZ_CONTENT_SANDBOX)
#if defined(XP_LINUX)
  SetCurrentProcessSandbox();
#elif defined(XP_WIN)
  mozilla::SandboxTarget::Instance()->StartSandbox();
#endif
#endif
  return true;
}

bool
ContentChild::RecvSpeakerManagerNotify()
{
#ifdef MOZ_WIDGET_GONK
  nsRefPtr<SpeakerManagerService> service =
    SpeakerManagerService::GetSpeakerManagerService();
  if (service) {
    service->Notify();
  }
  return true;
#endif
  return false;
}

static CancelableTask* sFirstIdleTask;

static void FirstIdle(void)
{
    MOZ_ASSERT(sFirstIdleTask);
    sFirstIdleTask = nullptr;
    ContentChild::GetSingleton()->SendFirstIdle();
}

mozilla::jsipc::PJavaScriptChild *
ContentChild::AllocPJavaScriptChild()
{
    nsCOMPtr<nsIJSRuntimeService> svc = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
    NS_ENSURE_TRUE(svc, nullptr);

    JSRuntime *rt;
    svc->GetRuntime(&rt);
    NS_ENSURE_TRUE(svc, nullptr);

    mozilla::jsipc::JavaScriptChild *child = new mozilla::jsipc::JavaScriptChild(rt);
    if (!child->init()) {
        delete child;
        return nullptr;
    }
    return child;
}

bool
ContentChild::DeallocPJavaScriptChild(PJavaScriptChild *child)
{
    delete child;
    return true;
}

PBrowserChild*
ContentChild::AllocPBrowserChild(const IPCTabContext& aContext,
                                 const uint32_t& aChromeFlags)
{
    
    
    

    MaybeInvalidTabContext tc(aContext);
    if (!tc.IsValid()) {
        NS_ERROR(nsPrintfCString("Received an invalid TabContext from "
                                 "the parent process. (%s)  Crashing...",
                                 tc.GetInvalidReason()).get());
        MOZ_CRASH("Invalid TabContext received from the parent process.");
    }

    nsRefPtr<TabChild> child = TabChild::Create(this, tc.GetTabContext(), aChromeFlags);

    
    return child.forget().take();
}

bool
ContentChild::RecvPBrowserConstructor(PBrowserChild* actor,
                                      const IPCTabContext& context,
                                      const uint32_t& chromeFlags)
{
    
    

    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (os) {
        nsITabChild* tc =
            static_cast<nsITabChild*>(static_cast<TabChild*>(actor));
        os->NotifyObservers(tc, "tab-child-created", nullptr);
    }

    static bool hasRunOnce = false;
    if (!hasRunOnce) {
        hasRunOnce = true;

        MOZ_ASSERT(!sFirstIdleTask);
        sFirstIdleTask = NewRunnableFunction(FirstIdle);
        MessageLoop::current()->PostIdleTask(FROM_HERE, sFirstIdleTask);

        
        
        InitProcessAttributes();
    }

    return true;
}

PFileDescriptorSetChild*
ContentChild::AllocPFileDescriptorSetChild(const FileDescriptor& aFD)
{
    return new FileDescriptorSetChild(aFD);
}

bool
ContentChild::DeallocPFileDescriptorSetChild(PFileDescriptorSetChild* aActor)
{
    delete static_cast<FileDescriptorSetChild*>(aActor);
    return true;
}

bool
ContentChild::DeallocPBrowserChild(PBrowserChild* iframe)
{
    TabChild* child = static_cast<TabChild*>(iframe);
    NS_RELEASE(child);
    return true;
}

PBlobChild*
ContentChild::AllocPBlobChild(const BlobConstructorParams& aParams)
{
  return BlobChild::Create(this, aParams);
}

bool
ContentChild::DeallocPBlobChild(PBlobChild* aActor)
{
  delete aActor;
  return true;
}

BlobChild*
ContentChild::GetOrCreateActorForBlob(nsIDOMBlob* aBlob)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aBlob);

  
  
  if (nsCOMPtr<nsIRemoteBlob> remoteBlob = do_QueryInterface(aBlob)) {
    BlobChild* actor =
      static_cast<BlobChild*>(
        static_cast<PBlobChild*>(remoteBlob->GetPBlob()));
    MOZ_ASSERT(actor);
    return actor;
  }

  
  nsCOMPtr<nsIMutable> mutableBlob = do_QueryInterface(aBlob);
  if (!mutableBlob || NS_FAILED(mutableBlob->SetMutable(false))) {
    NS_WARNING("Failed to make blob immutable!");
    return nullptr;
  }

#ifdef DEBUG
  {
    
    
    
    const auto* blob = static_cast<nsDOMFileBase*>(aBlob);

    MOZ_ASSERT(!blob->IsSizeUnknown());
    MOZ_ASSERT(!blob->IsDateUnknown());
  }
#endif

  ParentBlobConstructorParams params;

  nsString contentType;
  nsresult rv = aBlob->GetType(contentType);
  NS_ENSURE_SUCCESS(rv, nullptr);

  uint64_t length;
  rv = aBlob->GetSize(&length);
  NS_ENSURE_SUCCESS(rv, nullptr);

  nsCOMPtr<nsIInputStream> stream;
  rv = aBlob->GetInternalStream(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, nullptr);

  InputStreamParams inputStreamParams;
  nsTArray<mozilla::ipc::FileDescriptor> fds;
  SerializeInputStream(stream, inputStreamParams, fds);

  MOZ_ASSERT(fds.IsEmpty());

  params.optionalInputStreamParams() = inputStreamParams;

  nsCOMPtr<nsIDOMFile> file = do_QueryInterface(aBlob);
  if (file) {
    FileBlobConstructorParams fileParams;

    rv = file->GetName(fileParams.name());
    NS_ENSURE_SUCCESS(rv, nullptr);

    rv = file->GetMozLastModifiedDate(&fileParams.modDate());
    NS_ENSURE_SUCCESS(rv, nullptr);

    fileParams.contentType() = contentType;
    fileParams.length() = length;

    params.blobParams() = fileParams;
  } else {
    NormalBlobConstructorParams blobParams;
    blobParams.contentType() = contentType;
    blobParams.length() = length;
    params.blobParams() = blobParams;
  }

  BlobChild* actor = BlobChild::Create(this, aBlob);
  NS_ENSURE_TRUE(actor, nullptr);

  return SendPBlobConstructor(actor, params) ? actor : nullptr;
}

PCrashReporterChild*
ContentChild::AllocPCrashReporterChild(const mozilla::dom::NativeThreadId& id,
                                       const uint32_t& processType)
{
#ifdef MOZ_CRASHREPORTER
    return new CrashReporterChild();
#else
    return nullptr;
#endif
}

bool
ContentChild::DeallocPCrashReporterChild(PCrashReporterChild* crashreporter)
{
    delete crashreporter;
    return true;
}

PHalChild*
ContentChild::AllocPHalChild()
{
    return CreateHalChild();
}

bool
ContentChild::DeallocPHalChild(PHalChild* aHal)
{
    delete aHal;
    return true;
}

PIndexedDBChild*
ContentChild::AllocPIndexedDBChild()
{
  NS_NOTREACHED("Should never get here!");
  return nullptr;
}

bool
ContentChild::DeallocPIndexedDBChild(PIndexedDBChild* aActor)
{
  delete aActor;
  return true;
}

asmjscache::PAsmJSCacheEntryChild*
ContentChild::AllocPAsmJSCacheEntryChild(
                                    const asmjscache::OpenMode& aOpenMode,
                                    const asmjscache::WriteParams& aWriteParams,
                                    const IPC::Principal& aPrincipal)
{
  NS_NOTREACHED("Should never get here!");
  return nullptr;
}

bool
ContentChild::DeallocPAsmJSCacheEntryChild(PAsmJSCacheEntryChild* aActor)
{
  asmjscache::DeallocEntryChild(aActor);
  return true;
}

PTestShellChild*
ContentChild::AllocPTestShellChild()
{
    return new TestShellChild();
}

bool
ContentChild::DeallocPTestShellChild(PTestShellChild* shell)
{
    delete shell;
    return true;
}

jsipc::JavaScriptChild *
ContentChild::GetCPOWManager()
{
    if (ManagedPJavaScriptChild().Length()) {
        return static_cast<JavaScriptChild*>(ManagedPJavaScriptChild()[0]);
    }
    JavaScriptChild* actor = static_cast<JavaScriptChild*>(SendPJavaScriptConstructor());
    return actor;
}

bool
ContentChild::RecvPTestShellConstructor(PTestShellChild* actor)
{
    return true;
}

PDeviceStorageRequestChild*
ContentChild::AllocPDeviceStorageRequestChild(const DeviceStorageParams& aParams)
{
    return new DeviceStorageRequestChild();
}

bool
ContentChild::DeallocPDeviceStorageRequestChild(PDeviceStorageRequestChild* aDeviceStorage)
{
    delete aDeviceStorage;
    return true;
}

PFileSystemRequestChild*
ContentChild::AllocPFileSystemRequestChild(const FileSystemParams& aParams)
{
    NS_NOTREACHED("Should never get here!");
    return nullptr;
}

bool
ContentChild::DeallocPFileSystemRequestChild(PFileSystemRequestChild* aFileSystem)
{
    mozilla::dom::FileSystemTaskBase* child =
      static_cast<mozilla::dom::FileSystemTaskBase*>(aFileSystem);
    
    
    NS_RELEASE(child);
    return true;
}

PNeckoChild*
ContentChild::AllocPNeckoChild()
{
    return new NeckoChild();
}

bool
ContentChild::DeallocPNeckoChild(PNeckoChild* necko)
{
    delete necko;
    return true;
}

PExternalHelperAppChild*
ContentChild::AllocPExternalHelperAppChild(const OptionalURIParams& uri,
                                           const nsCString& aMimeContentType,
                                           const nsCString& aContentDisposition,
                                           const uint32_t& aContentDispositionHint,
                                           const nsString& aContentDispositionFilename,
                                           const bool& aForceSave,
                                           const int64_t& aContentLength,
                                           const OptionalURIParams& aReferrer,
                                           PBrowserChild* aBrowser)
{
    ExternalHelperAppChild *child = new ExternalHelperAppChild();
    child->AddRef();
    return child;
}

bool
ContentChild::DeallocPExternalHelperAppChild(PExternalHelperAppChild* aService)
{
    ExternalHelperAppChild *child = static_cast<ExternalHelperAppChild*>(aService);
    child->Release();
    return true;
}

PSmsChild*
ContentChild::AllocPSmsChild()
{
    return new SmsChild();
}

bool
ContentChild::DeallocPSmsChild(PSmsChild* aSms)
{
    delete aSms;
    return true;
}

PTelephonyChild*
ContentChild::AllocPTelephonyChild()
{
    MOZ_CRASH("No one should be allocating PTelephonyChild actors");
}

bool
ContentChild::DeallocPTelephonyChild(PTelephonyChild* aActor)
{
    delete aActor;
    return true;
}

PStorageChild*
ContentChild::AllocPStorageChild()
{
    NS_NOTREACHED("We should never be manually allocating PStorageChild actors");
    return nullptr;
}

bool
ContentChild::DeallocPStorageChild(PStorageChild* aActor)
{
    DOMStorageDBChild* child = static_cast<DOMStorageDBChild*>(aActor);
    child->ReleaseIPDLReference();
    return true;
}

PBluetoothChild*
ContentChild::AllocPBluetoothChild()
{
#ifdef MOZ_B2G_BT
    MOZ_CRASH("No one should be allocating PBluetoothChild actors");
#else
    MOZ_CRASH("No support for bluetooth on this platform!");
#endif
}

bool
ContentChild::DeallocPBluetoothChild(PBluetoothChild* aActor)
{
#ifdef MOZ_B2G_BT
    delete aActor;
    return true;
#else
    MOZ_CRASH("No support for bluetooth on this platform!");
#endif
}

PFMRadioChild*
ContentChild::AllocPFMRadioChild()
{
#ifdef MOZ_B2G_FM
    NS_RUNTIMEABORT("No one should be allocating PFMRadioChild actors");
    return nullptr;
#else
    NS_RUNTIMEABORT("No support for FMRadio on this platform!");
    return nullptr;
#endif
}

bool
ContentChild::DeallocPFMRadioChild(PFMRadioChild* aActor)
{
#ifdef MOZ_B2G_FM
    delete aActor;
    return true;
#else
    NS_RUNTIMEABORT("No support for FMRadio on this platform!");
    return false;
#endif
}

PSpeechSynthesisChild*
ContentChild::AllocPSpeechSynthesisChild()
{
#ifdef MOZ_WEBSPEECH
    MOZ_CRASH("No one should be allocating PSpeechSynthesisChild actors");
#else
    return nullptr;
#endif
}

bool
ContentChild::DeallocPSpeechSynthesisChild(PSpeechSynthesisChild* aActor)
{
#ifdef MOZ_WEBSPEECH
    delete aActor;
    return true;
#else
    return false;
#endif
}

bool
ContentChild::RecvRegisterChrome(const InfallibleTArray<ChromePackage>& packages,
                                 const InfallibleTArray<ResourceMapping>& resources,
                                 const InfallibleTArray<OverrideMapping>& overrides,
                                 const nsCString& locale)
{
    nsCOMPtr<nsIChromeRegistry> registrySvc = nsChromeRegistry::GetService();
    nsChromeRegistryContent* chromeRegistry =
        static_cast<nsChromeRegistryContent*>(registrySvc.get());
    chromeRegistry->RegisterRemoteChrome(packages, resources, overrides, locale);
    return true;
}

bool
ContentChild::RecvSetOffline(const bool& offline)
{
  nsCOMPtr<nsIIOService> io (do_GetIOService());
  NS_ASSERTION(io, "IO Service can not be null");

  io->SetOffline(offline);

  return true;
}

void
ContentChild::ActorDestroy(ActorDestroyReason why)
{
    if (AbnormalShutdown == why) {
        NS_WARNING("shutting down early because of crash!");
        QuickExit();
    }

#ifndef DEBUG
    
    
    
    QuickExit();
#endif

    if (sFirstIdleTask) {
        sFirstIdleTask->Cancel();
    }

    mAlertObservers.Clear();

    mIdleObservers.Clear();

    nsCOMPtr<nsIConsoleService> svc(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
    if (svc) {
        svc->UnregisterListener(mConsoleListener);
        mConsoleListener->mChild = nullptr;
    }

    XRE_ShutdownChildProcess();
}

void
ContentChild::ProcessingError(Result what)
{
    switch (what) {
    case MsgDropped:
        QuickExit();

    case MsgNotKnown:
        NS_RUNTIMEABORT("aborting because of MsgNotKnown");
    case MsgNotAllowed:
        NS_RUNTIMEABORT("aborting because of MsgNotAllowed");
    case MsgPayloadError:
        NS_RUNTIMEABORT("aborting because of MsgPayloadError");
    case MsgProcessingError:
        NS_RUNTIMEABORT("aborting because of MsgProcessingError");
    case MsgRouteError:
        NS_RUNTIMEABORT("aborting because of MsgRouteError");
    case MsgValueError:
        NS_RUNTIMEABORT("aborting because of MsgValueError");

    default:
        NS_RUNTIMEABORT("not reached");
    }
}

void
ContentChild::QuickExit()
{
    NS_WARNING("content process _exit()ing");
    _exit(0);
}

nsresult
ContentChild::AddRemoteAlertObserver(const nsString& aData,
                                     nsIObserver* aObserver)
{
    NS_ASSERTION(aObserver, "Adding a null observer?");
    mAlertObservers.AppendElement(new AlertObserver(aObserver, aData));
    return NS_OK;
}

bool
ContentChild::RecvPreferenceUpdate(const PrefSetting& aPref)
{
    Preferences::SetPreference(aPref);
    return true;
}

bool
ContentChild::RecvNotifyAlertsObserver(const nsCString& aType, const nsString& aData)
{
    for (uint32_t i = 0; i < mAlertObservers.Length();
         ) {
        AlertObserver* observer = mAlertObservers[i];
        if (observer->Observes(aData) && observer->Notify(aType)) {
            
            
            if (aType.Equals(nsDependentCString("alertfinished"))) {
                mAlertObservers.RemoveElementAt(i);
                continue;
            }
        }
        ++i;
    }
    return true;
}

bool
ContentChild::RecvNotifyVisited(const URIParams& aURI)
{
    nsCOMPtr<nsIURI> newURI = DeserializeURI(aURI);
    if (!newURI) {
        return false;
    }
    nsCOMPtr<IHistory> history = services::GetHistoryService();
    if (history) {
      history->NotifyVisited(newURI);
    }
    return true;
}

bool
ContentChild::RecvAsyncMessage(const nsString& aMsg,
                               const ClonedMessageData& aData,
                               const InfallibleTArray<CpowEntry>& aCpows,
                               const IPC::Principal& aPrincipal)
{
  nsRefPtr<nsFrameMessageManager> cpm = nsFrameMessageManager::sChildProcessManager;
  if (cpm) {
    StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForChild(aData);
    CpowIdHolder cpows(GetCPOWManager(), aCpows);
    cpm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(cpm.get()),
                        aMsg, false, &cloneData, &cpows, aPrincipal, nullptr);
  }
  return true;
}

bool
ContentChild::RecvGeolocationUpdate(const GeoPosition& somewhere)
{
  nsCOMPtr<nsIGeolocationUpdate> gs = do_GetService("@mozilla.org/geolocation/service;1");
  if (!gs) {
    return true;
  }
  nsCOMPtr<nsIDOMGeoPosition> position = somewhere;
  gs->Update(position);
  return true;
}

bool
ContentChild::RecvAddPermission(const IPC::Permission& permission)
{
#if MOZ_PERMISSIONS
  nsCOMPtr<nsIPermissionManager> permissionManagerIface =
      services::GetPermissionManager();
  nsPermissionManager* permissionManager =
      static_cast<nsPermissionManager*>(permissionManagerIface.get());
  NS_ABORT_IF_FALSE(permissionManager, 
                   "We have no permissionManager in the Content process !");

  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), NS_LITERAL_CSTRING("http://") + nsCString(permission.host));
  NS_ENSURE_TRUE(uri, true);

  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  MOZ_ASSERT(secMan);

  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = secMan->GetAppCodebasePrincipal(uri, permission.appId,
                                                permission.isInBrowserElement,
                                                getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, true);

  permissionManager->AddInternal(principal,
                                 nsCString(permission.type),
                                 permission.capability,
                                 0,
                                 permission.expireType,
                                 permission.expireTime,
                                 nsPermissionManager::eNotify,
                                 nsPermissionManager::eNoDBOperation);
#endif

  return true;
}

bool
ContentChild::RecvScreenSizeChanged(const gfxIntSize& size)
{
#ifdef ANDROID
    mScreenSize = size;
#else
    NS_RUNTIMEABORT("Message currently only expected on android");
#endif
  return true;
}

bool
ContentChild::RecvFlushMemory(const nsString& reason)
{
#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        
        return true;
    }
#endif
    nsCOMPtr<nsIObserverService> os =
        mozilla::services::GetObserverService();
    if (os)
        os->NotifyObservers(nullptr, "memory-pressure", reason.get());
    return true;
}

bool
ContentChild::RecvActivateA11y()
{
#ifdef ACCESSIBILITY
    
    
    nsCOMPtr<nsIAccessibilityService> accService =
        do_GetService("@mozilla.org/accessibilityService;1");
#endif
    return true;
}

bool
ContentChild::RecvGarbageCollect()
{
    
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->NotifyObservers(nullptr, "child-gc-request", nullptr);
    }
    nsJSContext::GarbageCollectNow(JS::gcreason::DOM_IPC);
    return true;
}

bool
ContentChild::RecvCycleCollect()
{
    
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->NotifyObservers(nullptr, "child-cc-request", nullptr);
    }
    nsJSContext::CycleCollectNow();
    return true;
}

#ifdef MOZ_NUWA_PROCESS
static void
OnFinishNuwaPreparation ()
{
    MakeNuwaProcess();
}
#endif

static void
PreloadSlowThings()
{
    
    nsLayoutStylesheetCache::UserContentSheet();

    TabChild::PreloadSlowThings();

}

bool
ContentChild::RecvAppInfo(const nsCString& version, const nsCString& buildID,
                          const nsCString& name, const nsCString& UAName)
{
    mAppInfo.version.Assign(version);
    mAppInfo.buildID.Assign(buildID);
    mAppInfo.name.Assign(name);
    mAppInfo.UAName.Assign(UAName);

    if (!Preferences::GetBool("dom.ipc.processPrelaunch.enabled", false)) {
        return true;
    }

    
    
    
    
    
    if ((mIsForApp || mIsForBrowser)
#ifdef MOZ_NUWA_PROCESS
        && !IsNuwaProcess()
#endif
       ) {
        PreloadSlowThings();
    }

#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        ContentChild::GetSingleton()->RecvGarbageCollect();
        MessageLoop::current()->PostTask(
            FROM_HERE, NewRunnableFunction(OnFinishNuwaPreparation));
    }
#endif

    return true;
}

bool
ContentChild::RecvLastPrivateDocShellDestroyed()
{
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    obs->NotifyObservers(nullptr, "last-pb-context-exited", nullptr);
    return true;
}

bool
ContentChild::RecvFilePathUpdate(const nsString& aStorageType,
                                 const nsString& aStorageName,
                                 const nsString& aPath,
                                 const nsCString& aReason)
{
    nsRefPtr<DeviceStorageFile> dsf = new DeviceStorageFile(aStorageType, aStorageName, aPath);

    nsString reason;
    CopyASCIItoUTF16(aReason, reason);
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    obs->NotifyObservers(dsf, "file-watcher-update", reason.get());
    return true;
}

bool
ContentChild::RecvFileSystemUpdate(const nsString& aFsName,
                                   const nsString& aVolumeName,
                                   const int32_t& aState,
                                   const int32_t& aMountGeneration,
                                   const bool& aIsMediaPresent,
                                   const bool& aIsSharing,
                                   const bool& aIsFormatting)
{
#ifdef MOZ_WIDGET_GONK
    nsRefPtr<nsVolume> volume = new nsVolume(aFsName, aVolumeName, aState,
                                             aMountGeneration, aIsMediaPresent,
                                             aIsSharing, aIsFormatting);

    nsRefPtr<nsVolumeService> vs = nsVolumeService::GetSingleton();
    if (vs) {
        vs->UpdateVolume(volume);
    }
#else
    
    unused << aFsName;
    unused << aVolumeName;
    unused << aState;
    unused << aMountGeneration;
    unused << aIsMediaPresent;
    unused << aIsSharing;
    unused << aIsFormatting;
#endif
    return true;
}

bool
ContentChild::RecvNotifyProcessPriorityChanged(
    const hal::ProcessPriority& aPriority)
{
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    NS_ENSURE_TRUE(os, true);

    nsRefPtr<nsHashPropertyBag> props = new nsHashPropertyBag();
    props->SetPropertyAsInt32(NS_LITERAL_STRING("priority"),
                              static_cast<int32_t>(aPriority));

    os->NotifyObservers(static_cast<nsIPropertyBag2*>(props),
                        "ipc:process-priority-changed",  nullptr);
    return true;
}

bool
ContentChild::RecvMinimizeMemoryUsage()
{
#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        
        
        return true;
    }
#endif
    nsCOMPtr<nsIMemoryReporterManager> mgr =
        do_GetService("@mozilla.org/memory-reporter-manager;1");
    NS_ENSURE_TRUE(mgr, true);

    mgr->MinimizeMemoryUsage( nullptr);
    return true;
}

bool
ContentChild::RecvNotifyPhoneStateChange(const nsString& aState)
{
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->NotifyObservers(nullptr, "phone-state-changed", aState.get());
  }
  return true;
}

void
ContentChild::AddIdleObserver(nsIObserver* aObserver, uint32_t aIdleTimeInS)
{
  MOZ_ASSERT(aObserver, "null idle observer");
  
  aObserver->AddRef();
  SendAddIdleObserver(reinterpret_cast<uint64_t>(aObserver), aIdleTimeInS);
  mIdleObservers.PutEntry(aObserver);
}

void
ContentChild::RemoveIdleObserver(nsIObserver* aObserver, uint32_t aIdleTimeInS)
{
  MOZ_ASSERT(aObserver, "null idle observer");
  SendRemoveIdleObserver(reinterpret_cast<uint64_t>(aObserver), aIdleTimeInS);
  aObserver->Release();
  mIdleObservers.RemoveEntry(aObserver);
}

bool
ContentChild::RecvNotifyIdleObserver(const uint64_t& aObserver,
                                     const nsCString& aTopic,
                                     const nsString& aTimeStr)
{
  nsIObserver* observer = reinterpret_cast<nsIObserver*>(aObserver);
  if (mIdleObservers.Contains(observer)) {
    observer->Observe(nullptr, aTopic.get(), aTimeStr.get());
  } else {
    NS_WARNING("Received notification for an idle observer that was removed.");
  }
  return true;
}

bool
ContentChild::RecvLoadAndRegisterSheet(const URIParams& aURI, const uint32_t& aType)
{
    nsCOMPtr<nsIURI> uri = DeserializeURI(aURI);
    if (!uri) {
        return true;
    }

    nsStyleSheetService *sheetService = nsStyleSheetService::GetInstance();
    if (sheetService) {
        sheetService->LoadAndRegisterSheet(uri, aType);
    }

    return true;
}

bool
ContentChild::RecvUnregisterSheet(const URIParams& aURI, const uint32_t& aType)
{
    nsCOMPtr<nsIURI> uri = DeserializeURI(aURI);
    if (!uri) {
        return true;
    }

    nsStyleSheetService *sheetService = nsStyleSheetService::GetInstance();
    if (sheetService) {
        sheetService->UnregisterSheet(uri, aType);
    }

    return true;
}

#ifdef MOZ_NUWA_PROCESS
class CallNuwaSpawn : public nsRunnable
{
public:
    NS_IMETHOD Run()
    {
        NuwaSpawn();
        if (IsNuwaProcess()) {
            return NS_OK;
        }

        
        ContentChild* child = ContentChild::GetSingleton();
        child->SetProcessName(NS_LITERAL_STRING("(Preallocated app)"), false);
        mozilla::ipc::Transport* transport = child->GetTransport();
        int fd = transport->GetFileDescriptor();
        transport->ResetFileDescriptor(fd);

        IToplevelProtocol* toplevel = child->GetFirstOpenedActors();
        while (toplevel != nullptr) {
            transport = toplevel->GetTransport();
            fd = transport->GetFileDescriptor();
            transport->ResetFileDescriptor(fd);

            toplevel = toplevel->getNext();
        }

        
        InitOnContentProcessCreated();

        return NS_OK;
    }
};

static void
DoNuwaFork()
{
    NS_ASSERTION(NuwaSpawnPrepare != nullptr,
                 "NuwaSpawnPrepare() is not available!");
    NuwaSpawnPrepare();       

    {
        nsCOMPtr<nsIRunnable> callSpawn(new CallNuwaSpawn());
        NS_DispatchToMainThread(callSpawn);
    }

    
    NS_ASSERTION(NuwaSpawnWait != nullptr,
                 "NuwaSpawnWait() is not available!");
    NuwaSpawnWait();        
    
}





static void
RunNuwaFork()
{
    if (NuwaCheckpointCurrentThread()) {
      DoNuwaFork();
    }
}
#endif

bool
ContentChild::RecvNuwaFork()
{
#ifdef MOZ_NUWA_PROCESS
    if (sNuwaForking) {           
        return true;
    }
    sNuwaForking = true;

    
    
    
    
    
    
    
    
    while (!BackgroundChild::GetForCurrentThread()) {
        if (NS_WARN_IF(!NS_ProcessNextEvent())) {
            return false;
        }
    }

    MessageLoop* ioloop = XRE_GetIOMessageLoop();
    ioloop->PostTask(FROM_HERE, NewRunnableFunction(RunNuwaFork));
    return true;
#else
    return false; 
#endif
}

} 
} 

extern "C" {

#if defined(MOZ_NUWA_PROCESS)
NS_EXPORT void
GetProtoFdInfos(NuwaProtoFdInfo* aInfoList,
                size_t aInfoListSize,
                size_t* aInfoSize)
{
    size_t i = 0;

    mozilla::dom::ContentChild* content =
        mozilla::dom::ContentChild::GetSingleton();
    aInfoList[i].protoId = content->GetProtocolId();
    aInfoList[i].originFd =
        content->GetTransport()->GetFileDescriptor();
    i++;

    for (IToplevelProtocol* actor = content->GetFirstOpenedActors();
         actor != nullptr;
         actor = actor->getNext()) {
        if (i >= aInfoListSize) {
            NS_RUNTIMEABORT("Too many top level protocols!");
        }

        aInfoList[i].protoId = actor->GetProtocolId();
        aInfoList[i].originFd =
            actor->GetTransport()->GetFileDescriptor();
        i++;
    }

    if (i > NUWA_TOPLEVEL_MAX) {
        NS_RUNTIMEABORT("Too many top level protocols!");
    }
    *aInfoSize = i;
}

class RunAddNewIPCProcess : public nsRunnable
{
public:
    RunAddNewIPCProcess(pid_t aPid,
                        nsTArray<mozilla::ipc::ProtocolFdMapping>& aMaps)
        : mPid(aPid)
    {
        mMaps.SwapElements(aMaps);
    }

    NS_IMETHOD Run()
    {
        mozilla::dom::ContentChild::GetSingleton()->
            SendAddNewProcess(mPid, mMaps);

        MOZ_ASSERT(sNuwaForking);
        sNuwaForking = false;

        return NS_OK;
    }

private:
    pid_t mPid;
    nsTArray<mozilla::ipc::ProtocolFdMapping> mMaps;
};








NS_EXPORT void
AddNewIPCProcess(pid_t aPid, NuwaProtoFdInfo* aInfoList, size_t aInfoListSize)
{
    nsTArray<mozilla::ipc::ProtocolFdMapping> maps;

    for (size_t i = 0; i < aInfoListSize; i++) {
        int _fd = aInfoList[i].newFds[NUWA_NEWFD_PARENT];
        mozilla::ipc::FileDescriptor fd(_fd);
        mozilla::ipc::ProtocolFdMapping map(aInfoList[i].protoId, fd);
        maps.AppendElement(map);
    }

    nsRefPtr<RunAddNewIPCProcess> runner = new RunAddNewIPCProcess(aPid, maps);
    NS_DispatchToMainThread(runner);
}

NS_EXPORT void
OnNuwaProcessReady()
{
    mozilla::dom::ContentChild* content =
        mozilla::dom::ContentChild::GetSingleton();
    content->SendNuwaReady();
}

NS_EXPORT void
AfterNuwaFork()
{
    SetCurrentProcessPrivileges(base::PRIVILEGES_DEFAULT);
}

#endif 

}
