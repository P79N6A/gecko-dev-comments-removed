






































#ifdef MOZ_IPC
#include "base/basictypes.h"
#endif

#include "mozilla/XPCOM.h"
#include "nsXULAppAPI.h"

#include "nsXPCOMPrivate.h"
#include "nsXPCOMCIDInternal.h"

#include "nsStaticComponents.h"
#include "prlink.h"

#include "nsObserverList.h"
#include "nsObserverService.h"
#include "nsProperties.h"
#include "nsPersistentProperties.h"
#include "nsScriptableInputStream.h"
#include "nsBinaryStream.h"
#include "nsStorageStream.h"
#include "nsPipe.h"

#include "nsMemoryImpl.h"
#include "nsDebugImpl.h"
#include "nsTraceRefcntImpl.h"
#include "nsErrorService.h"
#include "nsByteBuffer.h"

#include "nsSupportsArray.h"
#include "nsArray.h"
#include "nsINIParserImpl.h"
#include "nsSupportsPrimitives.h"
#include "nsConsoleService.h"
#include "nsExceptionService.h"

#include "nsComponentManager.h"
#include "nsCategoryManagerUtils.h"
#include "nsIServiceManager.h"

#include "nsThreadManager.h"
#include "nsThreadPool.h"

#include "nsIProxyObjectManager.h"
#include "nsProxyEventPrivate.h"  

#include "xptinfo.h"
#include "nsIInterfaceInfoManager.h"
#include "xptiprivate.h"

#include "nsTimerImpl.h"
#include "TimerThread.h"

#include "nsThread.h"
#include "nsProcess.h"
#include "nsEnvironment.h"
#include "nsVersionComparatorImpl.h"

#include "nsILocalFile.h"
#include "nsLocalFile.h"
#if defined(XP_UNIX) || defined(XP_OS2)
#include "nsNativeCharsetUtils.h"
#endif
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCategoryManager.h"
#include "nsICategoryManager.h"
#include "nsMultiplexInputStream.h"

#include "nsStringStream.h"
extern nsresult nsStringInputStreamConstructor(nsISupports *, REFNSIID, void **);

#include "nsFastLoadService.h"

#include "nsAtomService.h"
#include "nsAtomTable.h"
#include "nsTraceRefcnt.h"
#include "nsTimelineService.h"

#include "nsHashPropertyBag.h"

#include "nsUnicharInputStream.h"
#include "nsVariant.h"

#include "nsUUIDGenerator.h"

#include "nsIOUtil.h"

#include "nsRecyclingAllocator.h"

#include "SpecialSystemDirectory.h"

#if defined(XP_WIN)
#include "nsWindowsRegKey.h"
#endif

#ifdef XP_MACOSX
#include "nsMacUtilsImpl.h"
#endif

#include "nsSystemInfo.h"
#include "nsMemoryReporterManager.h"

#include <locale.h>
#include "mozilla/Services.h"
#include "mozilla/FunctionTimer.h"
#include "mozilla/Omnijar.h"

#include "nsChromeRegistry.h"
#include "nsChromeProtocolHandler.h"

#ifdef MOZ_ENABLE_LIBXUL
#include "mozilla/scache/StartupCache.h"
#endif

#ifdef MOZ_IPC
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/message_loop.h"

#include "mozilla/ipc/BrowserProcessSubThread.h"

using base::AtExitManager;
using mozilla::ipc::BrowserProcessSubThread;

namespace {

static AtExitManager* sExitManager;
static MessageLoop* sMessageLoop;
static bool sCommandLineWasInitialized;
static BrowserProcessSubThread* sIOThread;

} 
#endif






extern nsresult NS_RegistryGetFactory(nsIFactory** aFactory);
extern nsresult NS_CategoryManagerGetFactory( nsIFactory** );

#ifdef XP_WIN
extern nsresult nsMediaCacheRemover_Startup();
#endif

#ifdef DEBUG
extern void _FreeAutoLockStatics();
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
NS_GENERIC_FACTORY_CONSTRUCTOR(nsExceptionService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTimerImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBinaryOutputStream)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBinaryInputStream)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsStorageStream)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsVersionComparatorImpl)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVariant)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsRecyclingAllocatorImpl)

#ifdef MOZ_TIMELINE
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTimelineService)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsHashPropertyBag, Init)

NS_GENERIC_AGGREGATED_CONSTRUCTOR_INIT(nsProperties, Init)

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsUUIDGenerator, Init)

#ifdef XP_MACOSX
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMacUtilsImpl)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSystemInfo, Init)

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMemoryReporterManager, Init)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsIOUtil)

static nsresult
nsThreadManagerGetSingleton(nsISupports* outer,
                            const nsIID& aIID,
                            void* *aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "null outptr");
    NS_ENSURE_TRUE(!outer, NS_ERROR_NO_AGGREGATION);

    return nsThreadManager::get()->QueryInterface(aIID, aInstancePtr);
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsThreadPool)

static nsresult
nsXPTIInterfaceInfoManagerGetSingleton(nsISupports* outer,
                                       const nsIID& aIID,
                                       void* *aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "null outptr");
    NS_ENSURE_TRUE(!outer, NS_ERROR_NO_AGGREGATION);

    nsCOMPtr<nsIInterfaceInfoManager> iim
        (xptiInterfaceInfoManager::GetSingleton());
    if (!iim)
        return NS_ERROR_FAILURE;

    return iim->QueryInterface(aIID, aInstancePtr);
}

nsComponentManagerImpl* nsComponentManagerImpl::gComponentManager = NULL;
PRBool gXPCOMShuttingDown = PR_FALSE;

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kINIParserFactoryCID, NS_INIPARSERFACTORY_CID);
static NS_DEFINE_CID(kSimpleUnicharStreamFactoryCID, NS_SIMPLE_UNICHAR_STREAM_FACTORY_CID);

NS_DEFINE_NAMED_CID(NS_CHROMEREGISTRY_CID);
NS_DEFINE_NAMED_CID(NS_CHROMEPROTOCOLHANDLER_CID);

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsChromeRegistry,
                                         nsChromeRegistry::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsChromeProtocolHandler)

#define NS_PERSISTENTPROPERTIES_CID NS_IPERSISTENTPROPERTIES_CID /* sigh */
#define NS_XPCOMPROXY_CID NS_PROXYEVENT_MANAGER_CID

static already_AddRefed<nsIFactory>
CreateINIParserFactory(const mozilla::Module& module,
                       const mozilla::Module::CIDEntry& entry)
{
    nsIFactory* f = new nsINIParserFactory();
    f->AddRef();
    return f;
}

static already_AddRefed<nsIFactory>
CreateUnicharStreamFactory(const mozilla::Module& module,
                           const mozilla::Module::CIDEntry& entry)
{
    return nsSimpleUnicharStreamFactory::GetInstance();
}

#define COMPONENT(NAME, Ctor) static NS_DEFINE_CID(kNS_##NAME##_CID, NS_##NAME##_CID);
#include "XPCOMModule.inc"
#undef COMPONENT

#define COMPONENT(NAME, Ctor) { &kNS_##NAME##_CID, false, NULL, Ctor },
const mozilla::Module::CIDEntry kXPCOMCIDEntries[] = {
    { &kComponentManagerCID, true, NULL, nsComponentManagerImpl::Create },
    { &kINIParserFactoryCID, false, CreateINIParserFactory },
    { &kSimpleUnicharStreamFactoryCID, false, CreateUnicharStreamFactory },
#include "XPCOMModule.inc"
    { &kNS_CHROMEREGISTRY_CID, false, NULL, nsChromeRegistryConstructor },
    { &kNS_CHROMEPROTOCOLHANDLER_CID, false, NULL, nsChromeProtocolHandlerConstructor },
    { NULL }
};
#undef COMPONENT

#define COMPONENT(NAME, Ctor) { NS_##NAME##_CONTRACTID, &kNS_##NAME##_CID },
const mozilla::Module::ContractIDEntry kXPCOMContracts[] = {
#include "XPCOMModule.inc"
    { NS_CHROMEREGISTRY_CONTRACTID, &kNS_CHROMEREGISTRY_CID },
    { NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "chrome", &kNS_CHROMEPROTOCOLHANDLER_CID },
    { NS_INIPARSERFACTORY_CONTRACTID, &kINIParserFactoryCID },
    { NULL }
};
#undef COMPONENT

const mozilla::Module kXPCOMModule = { mozilla::Module::kVersion, kXPCOMCIDEntries, kXPCOMContracts };


static nsIDebug* gDebug = nsnull;

EXPORT_XPCOM_API(nsresult)
NS_GetDebug(nsIDebug** result)
{
    return nsDebugImpl::Create(nsnull, 
                               NS_GET_IID(nsIDebug), 
                               (void**) result);
}

EXPORT_XPCOM_API(nsresult)
NS_GetTraceRefcnt(nsITraceRefcnt** result)
{
    return nsTraceRefcntImpl::Create(nsnull, 
                                     NS_GET_IID(nsITraceRefcnt), 
                                     (void**) result);
}

EXPORT_XPCOM_API(nsresult)
NS_InitXPCOM(nsIServiceManager* *result,
                             nsIFile* binDirectory)
{
    return NS_InitXPCOM2(result, binDirectory, nsnull);
}

EXPORT_XPCOM_API(nsresult)
NS_InitXPCOM2(nsIServiceManager* *result,
              nsIFile* binDirectory,
              nsIDirectoryServiceProvider* appFileLocationProvider)
{
    NS_TIME_FUNCTION;

    nsresult rv = NS_OK;

     
    gXPCOMShuttingDown = PR_FALSE;

    NS_TIME_FUNCTION_MARK("Next: log init");

    NS_LogInit();

#ifdef MOZ_IPC
    NS_TIME_FUNCTION_MARK("Next: IPC init");

    
    NS_ASSERTION(!sExitManager && !sMessageLoop, "Bad logic!");

    if (!AtExitManager::AlreadyRegistered()) {
        sExitManager = new AtExitManager();
        NS_ENSURE_STATE(sExitManager);
    }

    if (!MessageLoop::current()) {
        sMessageLoop = new MessageLoopForUI(MessageLoop::TYPE_MOZILLA_UI);
        NS_ENSURE_STATE(sMessageLoop);
    }

    if (XRE_GetProcessType() == GeckoProcessType_Default &&
        !BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO)) {
        scoped_ptr<BrowserProcessSubThread> ioThread(
            new BrowserProcessSubThread(BrowserProcessSubThread::IO));
        NS_ENSURE_TRUE(ioThread.get(), NS_ERROR_OUT_OF_MEMORY);

        base::Thread::Options options;
        options.message_loop_type = MessageLoop::TYPE_IO;
        NS_ENSURE_TRUE(ioThread->StartWithOptions(options), NS_ERROR_FAILURE);

        sIOThread = ioThread.release();
    }
#endif

    NS_TIME_FUNCTION_MARK("Next: thread manager init");

    
    rv = nsThreadManager::get()->Init();
    if (NS_FAILED(rv)) return rv;

    NS_TIME_FUNCTION_MARK("Next: timer startup");

    
    rv = nsTimerImpl::Startup();
    NS_ENSURE_SUCCESS(rv, rv);

#if !defined(WINCE) && !defined(ANDROID)
    NS_TIME_FUNCTION_MARK("Next: setlocale");

    
    
    if (strcmp(setlocale(LC_ALL, NULL), "C") == 0)
        setlocale(LC_ALL, "");
#endif

#if defined(XP_UNIX) || defined(XP_OS2)
    NS_TIME_FUNCTION_MARK("Next: startup native charset utils");

    NS_StartupNativeCharsetUtils();
#endif

    NS_TIME_FUNCTION_MARK("Next: startup local file");

    NS_StartupLocalFile();

    StartupSpecialSystemDirectory();

    rv = nsDirectoryService::RealInit();
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIFile> xpcomLib;
            
    PRBool value;
    if (binDirectory)
    {
        rv = binDirectory->IsDirectory(&value);

        if (NS_SUCCEEDED(rv) && value) {
            nsDirectoryService::gService->Set(NS_XPCOM_INIT_CURRENT_PROCESS_DIR, binDirectory);
            binDirectory->Clone(getter_AddRefs(xpcomLib));
        }
    }
    else {
        nsDirectoryService::gService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, 
                                          NS_GET_IID(nsIFile), 
                                          getter_AddRefs(xpcomLib));
    }

    if (xpcomLib) {
        xpcomLib->AppendNative(nsDependentCString(XPCOM_DLL));
        nsDirectoryService::gService->Set(NS_XPCOM_LIBRARY_FILE, xpcomLib);
    }
    
    if (appFileLocationProvider) {
        rv = nsDirectoryService::gService->RegisterProvider(appFileLocationProvider);
        if (NS_FAILED(rv)) return rv;
    }

#ifdef MOZ_OMNIJAR
    NS_TIME_FUNCTION_MARK("Next: Omnijar init");

    if (!mozilla::OmnijarPath()) {
        nsCOMPtr<nsILocalFile> omnijar;
        nsCOMPtr<nsIFile> file;

        rv = NS_ERROR_FAILURE;
        nsDirectoryService::gService->Get(NS_GRE_DIR,
                                          NS_GET_IID(nsIFile),
                                          getter_AddRefs(file));
        if (file)
            rv = file->Append(NS_LITERAL_STRING("omni.jar"));
        if (NS_SUCCEEDED(rv))
            omnijar = do_QueryInterface(file);
        if (NS_SUCCEEDED(rv))
            mozilla::SetOmnijar(omnijar);
    }
#endif

#ifdef MOZ_IPC
    if ((sCommandLineWasInitialized = !CommandLine::IsInitialized())) {
        NS_TIME_FUNCTION_MARK("Next: IPC command line init");

#ifdef OS_WIN
        CommandLine::Init(0, nsnull);
#else
        nsCOMPtr<nsIFile> binaryFile;
        nsDirectoryService::gService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, 
                                          NS_GET_IID(nsIFile), 
                                          getter_AddRefs(binaryFile));
        NS_ENSURE_STATE(binaryFile);
        
        rv = binaryFile->AppendNative(NS_LITERAL_CSTRING("nonexistent-executable"));
        NS_ENSURE_SUCCESS(rv, rv);
        
        nsCString binaryPath;
        rv = binaryFile->GetNativePath(binaryPath);
        NS_ENSURE_SUCCESS(rv, rv);
        
        static char const *const argv = { strdup(binaryPath.get()) };
        CommandLine::Init(1, &argv);
#endif
    }
#endif

    NS_ASSERTION(nsComponentManagerImpl::gComponentManager == NULL, "CompMgr not null at init");

    NS_TIME_FUNCTION_MARK("Next: component manager init");

    
    nsComponentManagerImpl::gComponentManager = new nsComponentManagerImpl();
    NS_ADDREF(nsComponentManagerImpl::gComponentManager);
    
    rv = nsCycleCollector_startup();
    if (NS_FAILED(rv)) return rv;

    rv = nsComponentManagerImpl::gComponentManager->Init();
    if (NS_FAILED(rv))
    {
        NS_RELEASE(nsComponentManagerImpl::gComponentManager);
        return rv;
    }

    if (result) {
        NS_ADDREF(*result = nsComponentManagerImpl::gComponentManager);
    }

    NS_TIME_FUNCTION_MARK("Next: cycle collector startup");

    NS_TIME_FUNCTION_MARK("Next: interface info manager init");

    
    
    (void) xptiInterfaceInfoManager::GetSingleton();

    NS_TIME_FUNCTION_MARK("Next: register category providers");

    
    
    
    nsDirectoryService::gService->RegisterCategoryProviders();

#ifdef MOZ_ENABLE_LIBXUL
    mozilla::scache::StartupCache::GetSingleton();
#endif
    NS_TIME_FUNCTION_MARK("Next: create services from category");

    
    NS_CreateServicesFromCategory(NS_XPCOM_STARTUP_CATEGORY, 
                                  nsnull,
                                  NS_XPCOM_STARTUP_OBSERVER_ID);
#ifdef XP_WIN
    nsMediaCacheRemover_Startup();
#endif

    return NS_OK;
}























EXPORT_XPCOM_API(nsresult)
NS_ShutdownXPCOM(nsIServiceManager* servMgr)
{
    return mozilla::ShutdownXPCOM(servMgr);
}

namespace mozilla {

nsresult
ShutdownXPCOM(nsIServiceManager* servMgr)
{
    NS_ENSURE_STATE(NS_IsMainThread());

    nsresult rv;
    nsCOMPtr<nsISimpleEnumerator> moduleLoaders;

    
    {
        
        

        nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
        NS_ENSURE_STATE(thread);

        nsRefPtr<nsObserverService> observerService;
        CallGetService("@mozilla.org/observer-service;1",
                       (nsObserverService**) getter_AddRefs(observerService));

        if (observerService)
        {
            (void) observerService->
                NotifyObservers(nsnull, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID,
                                nsnull);

            nsCOMPtr<nsIServiceManager> mgr;
            rv = NS_GetServiceManager(getter_AddRefs(mgr));
            if (NS_SUCCEEDED(rv))
            {
                (void) observerService->
                    NotifyObservers(mgr, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                    nsnull);
            }
        }

        NS_ProcessPendingEvents(thread);
#ifdef MOZ_ENABLE_LIBXUL
        mozilla::scache::StartupCache::DeleteSingleton();
#endif
        if (observerService)
            (void) observerService->
                NotifyObservers(nsnull, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID,
                                nsnull);

        NS_ProcessPendingEvents(thread);

        
        
        nsTimerImpl::Shutdown();

        NS_ProcessPendingEvents(thread);

        
        
        
        nsThreadManager::get()->Shutdown();

        NS_ProcessPendingEvents(thread);

        
        
        if (observerService) {
            observerService->
                EnumerateObservers(NS_XPCOM_SHUTDOWN_LOADERS_OBSERVER_ID,
                                   getter_AddRefs(moduleLoaders));

            observerService->Shutdown();
        }
    }

    
    
    
    mozilla::services::Shutdown();

#ifdef DEBUG_dougt
    fprintf(stderr, "* * * * XPCOM shutdown. Access will be denied * * * * \n");
#endif
    
    
    NS_IF_RELEASE(servMgr);

    
    if (nsComponentManagerImpl::gComponentManager) {
        nsComponentManagerImpl::gComponentManager->FreeServices();
    }

    nsProxyObjectManager::Shutdown();

    
    NS_IF_RELEASE(nsDirectoryService::gService);

    nsCycleCollector_shutdown();

    if (moduleLoaders) {
        PRBool more;
        nsCOMPtr<nsISupports> el;
        while (NS_SUCCEEDED(moduleLoaders->HasMoreElements(&more)) &&
               more) {
            moduleLoaders->GetNext(getter_AddRefs(el));

            
            
            

            nsCOMPtr<nsIObserver> obs(do_QueryInterface(el));
            if (obs)
                (void) obs->Observe(nsnull,
                                    NS_XPCOM_SHUTDOWN_LOADERS_OBSERVER_ID,
                                    nsnull);
        }

        moduleLoaders = nsnull;
    }

    
    NS_ShutdownLocalFile();
#ifdef XP_UNIX
    NS_ShutdownNativeCharsetUtils();
#endif

    
    
    if (nsComponentManagerImpl::gComponentManager) {
        rv = (nsComponentManagerImpl::gComponentManager)->Shutdown();
        NS_ASSERTION(NS_SUCCEEDED(rv), "Component Manager shutdown failed.");
    } else
        NS_WARNING("Component Manager was never created ...");

    
    
    
    
    xptiInterfaceInfoManager::FreeInterfaceInfoManager();

    
    
    if (nsComponentManagerImpl::gComponentManager) {
      nsrefcnt cnt;
      NS_RELEASE2(nsComponentManagerImpl::gComponentManager, cnt);
      NS_ASSERTION(cnt == 0, "Component Manager being held past XPCOM shutdown.");
    }
    nsComponentManagerImpl::gComponentManager = nsnull;
    nsCategoryManager::Destroy();

#ifdef DEBUG
    
    _FreeAutoLockStatics();
#endif

    ShutdownSpecialSystemDirectory();

    NS_PurgeAtomTable();

    NS_IF_RELEASE(gDebug);

#ifdef MOZ_IPC
    if (sIOThread) {
        delete sIOThread;
        sIOThread = nsnull;
    }
    if (sMessageLoop) {
        delete sMessageLoop;
        sMessageLoop = nsnull;
    }
    if (sCommandLineWasInitialized) {
        CommandLine::Terminate();
        sCommandLineWasInitialized = false;
    }
    if (sExitManager) {
        delete sExitManager;
        sExitManager = nsnull;
    }
#endif

#ifdef MOZ_OMNIJAR
    mozilla::SetOmnijar(nsnull);
#endif

    NS_LogTerm();

    return NS_OK;
}

} 
