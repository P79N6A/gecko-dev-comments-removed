





#include "base/basictypes.h"

#include "mozilla/Atomics.h"
#include "mozilla/Poison.h"
#include "mozilla/XPCOM.h"
#include "nsXULAppAPI.h"

#include "nsXPCOMPrivate.h"
#include "nsXPCOMCIDInternal.h"

#include "mozilla/layers/ImageBridgeChild.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/AsyncTransactionTracker.h"
#include "mozilla/layers/SharedBufferManagerChild.h"

#include "prlink.h"

#include "nsCycleCollector.h"
#include "nsObserverList.h"
#include "nsObserverService.h"
#include "nsProperties.h"
#include "nsPersistentProperties.h"
#include "nsScriptableInputStream.h"
#include "nsBinaryStream.h"
#include "nsStorageStream.h"
#include "nsPipe.h"
#include "nsScriptableBase64Encoder.h"

#include "nsMemoryImpl.h"
#include "nsDebugImpl.h"
#include "nsTraceRefcnt.h"
#include "nsErrorService.h"

#include "nsSupportsArray.h"
#include "nsArray.h"
#include "nsINIParserImpl.h"
#include "nsSupportsPrimitives.h"
#include "nsConsoleService.h"

#include "nsComponentManager.h"
#include "nsCategoryManagerUtils.h"
#include "nsIServiceManager.h"

#include "nsThreadManager.h"
#include "nsThreadPool.h"

#include "xptinfo.h"
#include "nsIInterfaceInfoManager.h"
#include "xptiprivate.h"
#include "mozilla/XPTInterfaceInfoManager.h"

#include "nsTimerImpl.h"
#include "TimerThread.h"

#include "nsThread.h"
#include "nsProcess.h"
#include "nsEnvironment.h"
#include "nsVersionComparatorImpl.h"

#include "nsIFile.h"
#include "nsLocalFile.h"
#if defined(XP_UNIX)
#include "nsNativeCharsetUtils.h"
#endif
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCategoryManager.h"
#include "nsICategoryManager.h"
#include "nsMultiplexInputStream.h"

#include "nsStringStream.h"
extern nsresult nsStringInputStreamConstructor(nsISupports*, REFNSIID, void**);

#include "nsAtomService.h"
#include "nsAtomTable.h"
#include "nsISupportsImpl.h"

#include "nsHashPropertyBag.h"

#include "nsUnicharInputStream.h"
#include "nsVariant.h"

#include "nsUUIDGenerator.h"

#include "nsIOUtil.h"

#include "SpecialSystemDirectory.h"

#if defined(XP_WIN)
#include "nsWindowsRegKey.h"
#endif

#ifdef MOZ_WIDGET_COCOA
#include "nsMacUtilsImpl.h"
#endif

#include "nsSystemInfo.h"
#include "nsMemoryReporterManager.h"
#include "nsMemoryInfoDumper.h"
#include "nsSecurityConsoleMessage.h"
#include "nsMessageLoop.h"

#include "nsStatusReporterManager.h"

#include <locale.h>
#include "mozilla/Services.h"
#include "mozilla/Omnijar.h"
#include "mozilla/HangMonitor.h"
#include "mozilla/Telemetry.h"
#include "mozilla/BackgroundHangMonitor.h"

#include "nsChromeRegistry.h"
#include "nsChromeProtocolHandler.h"
#include "mozilla/PoisonIOInterposer.h"
#include "mozilla/LateWriteChecks.h"

#include "mozilla/scache/StartupCache.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/message_loop.h"

#include "mozilla/ipc/BrowserProcessSubThread.h"
#include "mozilla/AvailableMemoryTracker.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/CountingAllocatorBase.h"
#include "mozilla/SystemMemoryReporter.h"
#include "mozilla/UniquePtr.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"

#ifdef MOZ_VISUAL_EVENT_TRACER
#include "mozilla/VisualEventTracer.h"
#endif

#include "ogg/ogg.h"
#if defined(MOZ_VPX) && !defined(MOZ_VPX_NO_MEM_REPORTING)
#if defined(HAVE_STDINT_H)



#undef HAVE_STDINT_H
#endif
#include "vpx_mem/vpx_mem.h"
#endif
#ifdef MOZ_WEBM
#include "nestegg/nestegg.h"
#endif

#include "GeckoProfiler.h"

#include "jsapi.h"

#include "gfxPlatform.h"

using namespace mozilla;
using base::AtExitManager;
using mozilla::ipc::BrowserProcessSubThread;
#ifdef MOZ_VISUAL_EVENT_TRACER
using mozilla::eventtracer::VisualEventTracer;
#endif

namespace {

static AtExitManager* sExitManager;
static MessageLoop* sMessageLoop;
static bool sCommandLineWasInitialized;
static BrowserProcessSubThread* sIOThread;
static BackgroundHangMonitor* sMainHangMonitor;

} 






extern nsresult NS_RegistryGetFactory(nsIFactory** aFactory);
extern nsresult NS_CategoryManagerGetFactory(nsIFactory**);

#ifdef XP_WIN
extern nsresult CreateAnonTempFileRemover();
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsProcess)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsIDImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsStringImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsCStringImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRBoolImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRUint8Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRUint16Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRUint32Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRUint64Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRTimeImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsCharImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRInt16Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRInt32Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRInt64Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsFloatImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsDoubleImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsVoidImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsInterfacePointerImpl)

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsConsoleService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAtomService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTimerImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBinaryOutputStream)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBinaryInputStream)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsStorageStream)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsVersionComparatorImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScriptableBase64Encoder)
#ifdef MOZ_VISUAL_EVENT_TRACER
NS_GENERIC_FACTORY_CONSTRUCTOR(VisualEventTracer)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVariant)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsHashPropertyBagCC)

NS_GENERIC_AGGREGATED_CONSTRUCTOR(nsProperties)

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsUUIDGenerator, Init)

#ifdef MOZ_WIDGET_COCOA
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMacUtilsImpl)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSystemInfo, Init)

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMemoryReporterManager, Init)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMemoryInfoDumper)

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsStatusReporterManager, Init)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsIOUtil)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsSecurityConsoleMessage)

static nsresult
nsThreadManagerGetSingleton(nsISupports* aOuter,
                            const nsIID& aIID,
                            void** aInstancePtr)
{
  NS_ASSERTION(aInstancePtr, "null outptr");
  if (NS_WARN_IF(aOuter)) {
    return NS_ERROR_NO_AGGREGATION;
  }

  return nsThreadManager::get()->QueryInterface(aIID, aInstancePtr);
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsThreadPool)

static nsresult
nsXPTIInterfaceInfoManagerGetSingleton(nsISupports* aOuter,
                                       const nsIID& aIID,
                                       void** aInstancePtr)
{
  NS_ASSERTION(aInstancePtr, "null outptr");
  if (NS_WARN_IF(aOuter)) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsCOMPtr<nsIInterfaceInfoManager> iim(XPTInterfaceInfoManager::GetSingleton());
  if (!iim) {
    return NS_ERROR_FAILURE;
  }

  return iim->QueryInterface(aIID, aInstancePtr);
}

nsComponentManagerImpl* nsComponentManagerImpl::gComponentManager = nullptr;
bool gXPCOMShuttingDown = false;
bool gXPCOMThreadsShutDown = false;
char16_t* gGREBinPath = nullptr;

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kINIParserFactoryCID, NS_INIPARSERFACTORY_CID);
static NS_DEFINE_CID(kSimpleUnicharStreamFactoryCID,
                     NS_SIMPLE_UNICHAR_STREAM_FACTORY_CID);

NS_DEFINE_NAMED_CID(NS_CHROMEREGISTRY_CID);
NS_DEFINE_NAMED_CID(NS_CHROMEPROTOCOLHANDLER_CID);

NS_DEFINE_NAMED_CID(NS_SECURITY_CONSOLE_MESSAGE_CID);

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsChromeRegistry,
                                         nsChromeRegistry::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsChromeProtocolHandler)

#define NS_PERSISTENTPROPERTIES_CID NS_IPERSISTENTPROPERTIES_CID /* sigh */

static already_AddRefed<nsIFactory>
CreateINIParserFactory(const mozilla::Module& aModule,
                       const mozilla::Module::CIDEntry& aEntry)
{
  nsCOMPtr<nsIFactory> f = new nsINIParserFactory();
  return f.forget();
}

static already_AddRefed<nsIFactory>
CreateUnicharStreamFactory(const mozilla::Module& aModule,
                           const mozilla::Module::CIDEntry& aEntry)
{
  return already_AddRefed<nsIFactory>(
           nsSimpleUnicharStreamFactory::GetInstance());
}

#define COMPONENT(NAME, Ctor) static NS_DEFINE_CID(kNS_##NAME##_CID, NS_##NAME##_CID);
#include "XPCOMModule.inc"
#undef COMPONENT

#define COMPONENT(NAME, Ctor) { &kNS_##NAME##_CID, false, nullptr, Ctor },
const mozilla::Module::CIDEntry kXPCOMCIDEntries[] = {
  { &kComponentManagerCID, true, nullptr, nsComponentManagerImpl::Create },
  { &kINIParserFactoryCID, false, CreateINIParserFactory },
  { &kSimpleUnicharStreamFactoryCID, false, CreateUnicharStreamFactory },
#include "XPCOMModule.inc"
  { &kNS_CHROMEREGISTRY_CID, false, nullptr, nsChromeRegistryConstructor },
  { &kNS_CHROMEPROTOCOLHANDLER_CID, false, nullptr, nsChromeProtocolHandlerConstructor },
  { &kNS_SECURITY_CONSOLE_MESSAGE_CID, false, nullptr, nsSecurityConsoleMessageConstructor },
  { nullptr }
};
#undef COMPONENT

#define COMPONENT(NAME, Ctor) { NS_##NAME##_CONTRACTID, &kNS_##NAME##_CID },
const mozilla::Module::ContractIDEntry kXPCOMContracts[] = {
#include "XPCOMModule.inc"
  { NS_CHROMEREGISTRY_CONTRACTID, &kNS_CHROMEREGISTRY_CID },
  { NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "chrome", &kNS_CHROMEPROTOCOLHANDLER_CID },
  { NS_INIPARSERFACTORY_CONTRACTID, &kINIParserFactoryCID },
  { NS_SECURITY_CONSOLE_MESSAGE_CONTRACTID, &kNS_SECURITY_CONSOLE_MESSAGE_CID },
  { nullptr }
};
#undef COMPONENT

const mozilla::Module kXPCOMModule = {
  mozilla::Module::kVersion, kXPCOMCIDEntries, kXPCOMContracts
};


static nsIDebug* gDebug = nullptr;

EXPORT_XPCOM_API(nsresult)
NS_GetDebug(nsIDebug** aResult)
{
  return nsDebugImpl::Create(nullptr,  NS_GET_IID(nsIDebug), (void**)aResult);
}

EXPORT_XPCOM_API(nsresult)
NS_InitXPCOM(nsIServiceManager** aResult,
             nsIFile* aBinDirectory)
{
  return NS_InitXPCOM2(aResult, aBinDirectory, nullptr);
}

class ICUReporter final
  : public nsIMemoryReporter
  , public CountingAllocatorBase<ICUReporter>
{
public:
  NS_DECL_ISUPPORTS

  static void* Alloc(const void*, size_t aSize)
  {
    return CountingMalloc(aSize);
  }

  static void* Realloc(const void*, void* aPtr, size_t aSize)
  {
    return CountingRealloc(aPtr, aSize);
  }

  static void Free(const void*, void* aPtr)
  {
    return CountingFree(aPtr);
  }

private:
  NS_IMETHODIMP
  CollectReports(nsIHandleReportCallback* aHandleReport, nsISupports* aData,
                 bool aAnonymize) override
  {
    return MOZ_COLLECT_REPORT(
      "explicit/icu", KIND_HEAP, UNITS_BYTES, MemoryAllocated(),
      "Memory used by ICU, a Unicode and globalization support library.");
  }

  ~ICUReporter() {}
};

NS_IMPL_ISUPPORTS(ICUReporter, nsIMemoryReporter)

 template<> Atomic<size_t>
CountingAllocatorBase<ICUReporter>::sAmount(0);

class OggReporter final
  : public nsIMemoryReporter
  , public CountingAllocatorBase<OggReporter>
{
public:
  NS_DECL_ISUPPORTS

private:
  NS_IMETHODIMP
  CollectReports(nsIHandleReportCallback* aHandleReport, nsISupports* aData,
                 bool aAnonymize) override
  {
    return MOZ_COLLECT_REPORT(
      "explicit/media/libogg", KIND_HEAP, UNITS_BYTES, MemoryAllocated(),
      "Memory allocated through libogg for Ogg, Theora, and related media files.");
  }

  ~OggReporter() {}
};

NS_IMPL_ISUPPORTS(OggReporter, nsIMemoryReporter)

 template<> Atomic<size_t>
CountingAllocatorBase<OggReporter>::sAmount(0);

#ifdef MOZ_VPX
class VPXReporter final
  : public nsIMemoryReporter
  , public CountingAllocatorBase<VPXReporter>
{
public:
  NS_DECL_ISUPPORTS

private:
  NS_IMETHODIMP
  CollectReports(nsIHandleReportCallback* aHandleReport, nsISupports* aData,
                 bool aAnonymize) override
  {
    return MOZ_COLLECT_REPORT(
      "explicit/media/libvpx", KIND_HEAP, UNITS_BYTES, MemoryAllocated(),
      "Memory allocated through libvpx for WebM media files.");
  }

  ~VPXReporter() {}
};

NS_IMPL_ISUPPORTS(VPXReporter, nsIMemoryReporter)

 template<> Atomic<size_t>
CountingAllocatorBase<VPXReporter>::sAmount(0);
#endif 

#ifdef MOZ_WEBM
class NesteggReporter final
  : public nsIMemoryReporter
  , public CountingAllocatorBase<NesteggReporter>
{
public:
  NS_DECL_ISUPPORTS

private:
  NS_IMETHODIMP
  CollectReports(nsIHandleReportCallback* aHandleReport, nsISupports* aData,
                 bool aAnonymize) override
  {
    return MOZ_COLLECT_REPORT(
      "explicit/media/libnestegg", KIND_HEAP, UNITS_BYTES, MemoryAllocated(),
      "Memory allocated through libnestegg for WebM media files.");
  }

  ~NesteggReporter() {}
};

NS_IMPL_ISUPPORTS(NesteggReporter, nsIMemoryReporter)

 template<> Atomic<size_t>
CountingAllocatorBase<NesteggReporter>::sAmount(0);
#endif 


EXPORT_XPCOM_API(nsresult)
NS_InitXPCOM2(nsIServiceManager** aResult,
              nsIFile* aBinDirectory,
              nsIDirectoryServiceProvider* aAppFileLocationProvider)
{
  static bool sInitialized = false;
  if (sInitialized) {
    return NS_ERROR_FAILURE;
  }

  sInitialized = true;

  mozPoisonValueInit();

  char aLocal;
  profiler_init(&aLocal);
  nsresult rv = NS_OK;

  
  gXPCOMShuttingDown = false;

  
  
  mozilla::AvailableMemoryTracker::Init();

#ifdef XP_UNIX
  
  
  
  
  
  
  nsSystemInfo::gUserUmask = ::umask(0777);
  ::umask(nsSystemInfo::gUserUmask);
#endif

  NS_LogInit();

  
  NS_ASSERTION(!sExitManager && !sMessageLoop, "Bad logic!");

  if (!AtExitManager::AlreadyRegistered()) {
    sExitManager = new AtExitManager();
  }

  if (!MessageLoop::current()) {
    sMessageLoop = new MessageLoopForUI(MessageLoop::TYPE_MOZILLA_UI);
    sMessageLoop->set_thread_name("Gecko");
    
    
    sMessageLoop->set_hang_timeouts(128, 8192);
  }

  if (XRE_GetProcessType() == GeckoProcessType_Default &&
      !BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO)) {
    UniquePtr<BrowserProcessSubThread> ioThread = MakeUnique<BrowserProcessSubThread>(BrowserProcessSubThread::IO);

    base::Thread::Options options;
    options.message_loop_type = MessageLoop::TYPE_IO;
    if (NS_WARN_IF(!ioThread->StartWithOptions(options))) {
      return NS_ERROR_FAILURE;
    }

    sIOThread = ioThread.release();
  }

  
  rv = nsThreadManager::get()->Init();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = nsTimerImpl::Startup();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

#ifndef ANDROID
  
  
  if (strcmp(setlocale(LC_ALL, nullptr), "C") == 0) {
    setlocale(LC_ALL, "");
  }
#endif

#if defined(XP_UNIX)
  NS_StartupNativeCharsetUtils();
#endif

  NS_StartupLocalFile();

  StartupSpecialSystemDirectory();

  nsDirectoryService::RealInit();

  bool value;

  if (aBinDirectory) {
    rv = aBinDirectory->IsDirectory(&value);

    if (NS_SUCCEEDED(rv) && value) {
      nsDirectoryService::gService->Set(NS_XPCOM_INIT_CURRENT_PROCESS_DIR,
                                        aBinDirectory);
    }
  }

  if (aAppFileLocationProvider) {
    rv = nsDirectoryService::gService->RegisterProvider(aAppFileLocationProvider);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  nsCOMPtr<nsIFile> xpcomLib;
  nsDirectoryService::gService->Get(NS_GRE_BIN_DIR,
                                    NS_GET_IID(nsIFile),
                                    getter_AddRefs(xpcomLib));
  MOZ_ASSERT(xpcomLib);

  
  nsAutoString path;
  xpcomLib->GetPath(path);
  gGREBinPath = ToNewUnicode(path);

  xpcomLib->AppendNative(nsDependentCString(XPCOM_DLL));
  nsDirectoryService::gService->Set(NS_XPCOM_LIBRARY_FILE, xpcomLib);

  if (!mozilla::Omnijar::IsInitialized()) {
    mozilla::Omnijar::Init();
  }

  if ((sCommandLineWasInitialized = !CommandLine::IsInitialized())) {
#ifdef OS_WIN
    CommandLine::Init(0, nullptr);
#else
    nsCOMPtr<nsIFile> binaryFile;
    nsDirectoryService::gService->Get(NS_XPCOM_CURRENT_PROCESS_DIR,
                                      NS_GET_IID(nsIFile),
                                      getter_AddRefs(binaryFile));
    if (NS_WARN_IF(!binaryFile)) {
      return NS_ERROR_FAILURE;
    }

    rv = binaryFile->AppendNative(NS_LITERAL_CSTRING("nonexistent-executable"));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsCString binaryPath;
    rv = binaryFile->GetNativePath(binaryPath);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    static char const* const argv = { strdup(binaryPath.get()) };
    CommandLine::Init(1, &argv);
#endif
  }

  NS_ASSERTION(nsComponentManagerImpl::gComponentManager == nullptr,
               "CompMgr not null at init");

  
  nsComponentManagerImpl::gComponentManager = new nsComponentManagerImpl();
  NS_ADDREF(nsComponentManagerImpl::gComponentManager);

  
  if (!nsCycleCollector_init()) {
    return NS_ERROR_UNEXPECTED;
  }

  
  nsCycleCollector_startup();

  
  
  
  
  
  
  mozilla::SetICUMemoryFunctions();

  
  ogg_set_mem_functions(OggReporter::CountingMalloc,
                        OggReporter::CountingCalloc,
                        OggReporter::CountingRealloc,
                        OggReporter::CountingFree);

#if defined(MOZ_VPX) && !defined(MOZ_VPX_NO_MEM_REPORTING)
  
  vpx_mem_set_functions(VPXReporter::CountingMalloc,
                        VPXReporter::CountingCalloc,
                        VPXReporter::CountingRealloc,
                        VPXReporter::CountingFree,
                        memcpy,
                        memset,
                        memmove);
#endif

#ifdef MOZ_WEBM
  
  
  
  
  nestegg_set_halloc_func(NesteggReporter::CountingFreeingRealloc);
#endif

  
  if (!JS_Init()) {
    NS_RUNTIMEABORT("JS_Init failed");
  }

  rv = nsComponentManagerImpl::gComponentManager->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(nsComponentManagerImpl::gComponentManager);
    return rv;
  }

  if (aResult) {
    NS_ADDREF(*aResult = nsComponentManagerImpl::gComponentManager);
  }

  
  
  (void)XPTInterfaceInfoManager::GetSingleton();

  
  
  
  nsDirectoryService::gService->RegisterCategoryProviders();

  
  
  nsCOMPtr<nsISupports> componentLoader =
    do_GetService("@mozilla.org/moz/jsloader;1");

  mozilla::scache::StartupCache::GetSingleton();
  mozilla::AvailableMemoryTracker::Activate();

  
  NS_CreateServicesFromCategory(NS_XPCOM_STARTUP_CATEGORY,
                                nullptr,
                                NS_XPCOM_STARTUP_OBSERVER_ID);
#ifdef XP_WIN
  CreateAnonTempFileRemover();
#endif

  
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    mozilla::SystemMemoryReporter::Init();
  }

  
  RegisterStrongMemoryReporter(new ICUReporter());
  RegisterStrongMemoryReporter(new OggReporter());
#ifdef MOZ_VPX
  RegisterStrongMemoryReporter(new VPXReporter());
#endif
#ifdef MOZ_WEBM
  RegisterStrongMemoryReporter(new NesteggReporter());
#endif

  mozilla::Telemetry::Init();

  mozilla::HangMonitor::Startup();
  mozilla::BackgroundHangMonitor::Startup();

  const MessageLoop* const loop = MessageLoop::current();
  sMainHangMonitor = new mozilla::BackgroundHangMonitor(
    loop->thread_name().c_str(),
    loop->transient_hang_timeout(),
    loop->permanent_hang_timeout());

#ifdef MOZ_VISUAL_EVENT_TRACER
  mozilla::eventtracer::Init();
#endif

  return NS_OK;
}























EXPORT_XPCOM_API(nsresult)
NS_ShutdownXPCOM(nsIServiceManager* aServMgr)
{
  return mozilla::ShutdownXPCOM(aServMgr);
}

namespace mozilla {

void
SetICUMemoryFunctions()
{
  static bool sICUReporterInitialized = false;
  if (!sICUReporterInitialized) {
    if (!JS_SetICUMemoryFunctions(ICUReporter::Alloc, ICUReporter::Realloc,
                                  ICUReporter::Free)) {
      NS_RUNTIMEABORT("JS_SetICUMemoryFunctions failed.");
    }
    sICUReporterInitialized = true;
  }
}

nsresult
ShutdownXPCOM(nsIServiceManager* aServMgr)
{
  
  HangMonitor::NotifyActivity();

  if (!NS_IsMainThread()) {
    NS_RUNTIMEABORT("Shutdown on wrong thread");
  }

  nsresult rv;
  nsCOMPtr<nsISimpleEnumerator> moduleLoaders;

  
  {
    
    

    nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
    if (NS_WARN_IF(!thread)) {
      return NS_ERROR_UNEXPECTED;
    }

    nsRefPtr<nsObserverService> observerService;
    CallGetService("@mozilla.org/observer-service;1",
                   (nsObserverService**)getter_AddRefs(observerService));

    if (observerService) {
      observerService->NotifyObservers(nullptr,
                                       NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID,
                                       nullptr);

      nsCOMPtr<nsIServiceManager> mgr;
      rv = NS_GetServiceManager(getter_AddRefs(mgr));
      if (NS_SUCCEEDED(rv)) {
        observerService->NotifyObservers(mgr, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                         nullptr);
      }
    }

    
    
    NS_ProcessPendingEvents(thread);
    gfxPlatform::ShutdownLayersIPC();

    mozilla::scache::StartupCache::DeleteSingleton();
    if (observerService)
      observerService->NotifyObservers(nullptr,
                                       NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID,
                                       nullptr);

    gXPCOMThreadsShutDown = true;
    NS_ProcessPendingEvents(thread);

    
    
    nsTimerImpl::Shutdown();

    NS_ProcessPendingEvents(thread);

    
    
    
    nsThreadManager::get()->Shutdown();

    NS_ProcessPendingEvents(thread);

    HangMonitor::NotifyActivity();

    
    
    
    mozilla::InitLateWriteChecks();

    
    
    if (observerService) {
      observerService->EnumerateObservers(NS_XPCOM_SHUTDOWN_LOADERS_OBSERVER_ID,
                                          getter_AddRefs(moduleLoaders));

      observerService->Shutdown();
    }
  }

  
  
  
  mozilla::KillClearOnShutdown();

  
  
  
  mozilla::services::Shutdown();

#ifdef DEBUG_dougt
  fprintf(stderr, "* * * * XPCOM shutdown. Access will be denied * * * * \n");
#endif
  
  
  NS_IF_RELEASE(aServMgr);

  
  if (nsComponentManagerImpl::gComponentManager) {
    nsComponentManagerImpl::gComponentManager->FreeServices();
  }

  
  NS_IF_RELEASE(nsDirectoryService::gService);

  free(gGREBinPath);
  gGREBinPath = nullptr;

  if (moduleLoaders) {
    bool more;
    nsCOMPtr<nsISupports> el;
    while (NS_SUCCEEDED(moduleLoaders->HasMoreElements(&more)) && more) {
      moduleLoaders->GetNext(getter_AddRefs(el));

      
      
      

      
      
      
      
      nsCOMPtr<nsIObserver> obs(do_QueryInterface(el));
      if (obs) {
        obs->Observe(nullptr, NS_XPCOM_SHUTDOWN_LOADERS_OBSERVER_ID, nullptr);
      }
    }

    moduleLoaders = nullptr;
  }

  nsCycleCollector_shutdown();

  layers::AsyncTransactionTrackersHolder::Finalize();

  PROFILER_MARKER("Shutdown xpcom");
  
  if (gShutdownChecks != SCM_NOTHING) {
#ifdef XP_MACOSX
    mozilla::OnlyReportDirtyWrites();
#endif 
    mozilla::BeginLateWriteChecks();
  }

  
  NS_ShutdownLocalFile();
#ifdef XP_UNIX
  NS_ShutdownNativeCharsetUtils();
#endif

#if defined(XP_WIN)
  
  
  
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
      NS_WARNING("Exiting child process early!");
      exit(0);
  }
#endif

  
  
  if (nsComponentManagerImpl::gComponentManager) {
    rv = (nsComponentManagerImpl::gComponentManager)->Shutdown();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Component Manager shutdown failed.");
  } else {
    NS_WARNING("Component Manager was never created ...");
  }

#ifdef MOZ_ENABLE_PROFILER_SPS
  
  
  
  
  
  
  
  
  if (PseudoStack* stack = mozilla_get_pseudo_stack()) {
    stack->sampleRuntime(nullptr);
  }
#endif

  
  JS_ShutDown();

  
  
  
  
  XPTInterfaceInfoManager::FreeInterfaceInfoManager();

  
  
  if (nsComponentManagerImpl::gComponentManager) {
    nsrefcnt cnt;
    NS_RELEASE2(nsComponentManagerImpl::gComponentManager, cnt);
    NS_ASSERTION(cnt == 0, "Component Manager being held past XPCOM shutdown.");
  }
  nsComponentManagerImpl::gComponentManager = nullptr;
  nsCategoryManager::Destroy();

  NS_PurgeAtomTable();

  NS_IF_RELEASE(gDebug);

  delete sIOThread;
  sIOThread = nullptr;

  delete sMessageLoop;
  sMessageLoop = nullptr;

  if (sCommandLineWasInitialized) {
    CommandLine::Terminate();
    sCommandLineWasInitialized = false;
  }

  delete sExitManager;
  sExitManager = nullptr;

  Omnijar::CleanUp();

  HangMonitor::Shutdown();

  delete sMainHangMonitor;
  sMainHangMonitor = nullptr;

  BackgroundHangMonitor::Shutdown();

#ifdef MOZ_VISUAL_EVENT_TRACER
  eventtracer::Shutdown();
#endif

  profiler_shutdown();

  NS_LogTerm();

#if defined(MOZ_WIDGET_GONK)
  
  
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
      NS_WARNING("Exiting child process early!");
      exit(0);
  }
#endif

  return NS_OK;
}

} 
