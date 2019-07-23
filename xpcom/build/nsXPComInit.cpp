






































#include "mozilla/XPCOM.h"

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
#include "nsGenericFactory.h"

#include "nsThreadManager.h"
#include "nsThreadPool.h"

#ifdef DEBUG
#include "BlockingResourceBase.h"
#endif 

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
extern NS_METHOD nsStringInputStreamConstructor(nsISupports *, REFNSIID, void **);
NS_DECL_CLASSINFO(nsStringInputStream)

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

using mozilla::TimeStamp;






extern nsresult NS_RegistryGetFactory(nsIFactory** aFactory);
extern nsresult NS_CategoryManagerGetFactory( nsIFactory** );

#ifdef DEBUG
extern void _FreeAutoLockStatics();
#endif

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kMemoryCID, NS_MEMORY_CID);
static NS_DEFINE_CID(kINIParserFactoryCID, NS_INIPARSERFACTORY_CID);
static NS_DEFINE_CID(kSimpleUnicharStreamFactoryCID, NS_SIMPLE_UNICHAR_STREAM_FACTORY_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsProcess)

#define NS_ENVIRONMENT_CLASSNAME "Environment Service"


#define NS_SUPPORTS_ID_CLASSNAME "Supports ID"
#define NS_SUPPORTS_CSTRING_CLASSNAME "Supports String"
#define NS_SUPPORTS_STRING_CLASSNAME "Supports WString"
#define NS_SUPPORTS_PRBOOL_CLASSNAME "Supports PRBool"
#define NS_SUPPORTS_PRUINT8_CLASSNAME "Supports PRUint8"
#define NS_SUPPORTS_PRUINT16_CLASSNAME "Supports PRUint16"
#define NS_SUPPORTS_PRUINT32_CLASSNAME "Supports PRUint32"
#define NS_SUPPORTS_PRUINT64_CLASSNAME "Supports PRUint64"
#define NS_SUPPORTS_PRTIME_CLASSNAME "Supports PRTime"
#define NS_SUPPORTS_CHAR_CLASSNAME "Supports Char"
#define NS_SUPPORTS_PRINT16_CLASSNAME "Supports PRInt16"
#define NS_SUPPORTS_PRINT32_CLASSNAME "Supports PRInt32"
#define NS_SUPPORTS_PRINT64_CLASSNAME "Supports PRInt64"
#define NS_SUPPORTS_FLOAT_CLASSNAME "Supports float"
#define NS_SUPPORTS_DOUBLE_CLASSNAME "Supports double"
#define NS_SUPPORTS_VOID_CLASSNAME "Supports void"
#define NS_SUPPORTS_INTERFACE_POINTER_CLASSNAME "Supports interface pointer"

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
NS_DECL_CLASSINFO(nsConsoleService)
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

static NS_METHOD
nsThreadManagerGetSingleton(nsISupports* outer,
                            const nsIID& aIID,
                            void* *aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "null outptr");
    NS_ENSURE_TRUE(!outer, NS_ERROR_NO_AGGREGATION);

    return nsThreadManager::get()->QueryInterface(aIID, aInstancePtr);
}
NS_DECL_CLASSINFO(nsThreadManager)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsThreadPool)
NS_DECL_CLASSINFO(nsThreadPool)

static NS_METHOD
nsXPTIInterfaceInfoManagerGetSingleton(nsISupports* outer,
                                       const nsIID& aIID,
                                       void* *aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "null outptr");
    NS_ENSURE_TRUE(!outer, NS_ERROR_NO_AGGREGATION);

    nsCOMPtr<nsIInterfaceInfoManager> iim
        (xptiInterfaceInfoManager::GetInterfaceInfoManagerNoAddRef());
    if (!iim)
        return NS_ERROR_FAILURE;

    return iim->QueryInterface(aIID, aInstancePtr);
}


static nsresult
RegisterGenericFactory(nsIComponentRegistrar* registrar,
                       const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsIGenericFactory* fact;
    rv = NS_NewGenericFactory(&fact, info);
    if (NS_FAILED(rv)) return rv;

    rv = registrar->RegisterFactory(info->mCID, 
                                    info->mDescription,
                                    info->mContractID, 
                                    fact);
    NS_RELEASE(fact);
    return rv;
}


nsComponentManagerImpl* nsComponentManagerImpl::gComponentManager = NULL;
PRBool gXPCOMShuttingDown = PR_FALSE;




#define COMPONENT(NAME, Ctor)                                                  \
 { NS_##NAME##_CLASSNAME, NS_##NAME##_CID, NS_##NAME##_CONTRACTID, Ctor }

#define COMPONENT_CI(NAME, Ctor, Class)                                        \
 { NS_##NAME##_CLASSNAME, NS_##NAME##_CID, NS_##NAME##_CONTRACTID, Ctor,       \
   NULL, NULL, NULL, NS_CI_INTERFACE_GETTER_NAME(Class), NULL,                 \
   &NS_CLASSINFO_NAME(Class) }

#define COMPONENT_CI_FLAGS(NAME, Ctor, Class, Flags)                           \
 { NS_##NAME##_CLASSNAME, NS_##NAME##_CID, NS_##NAME##_CONTRACTID, Ctor,       \
   NULL, NULL, NULL, NS_CI_INTERFACE_GETTER_NAME(Class), NULL,                 \
   &NS_CLASSINFO_NAME(Class), Flags }

static const nsModuleComponentInfo components[] = {
    COMPONENT(MEMORY, nsMemoryImpl::Create),
    COMPONENT(DEBUG,  nsDebugImpl::Create),
#define NS_ERRORSERVICE_CLASSNAME NS_ERRORSERVICE_NAME
    COMPONENT(ERRORSERVICE, nsErrorService::Create),

    COMPONENT(BYTEBUFFER, ByteBufferImpl::Create),
    COMPONENT(SCRIPTABLEINPUTSTREAM, nsScriptableInputStream::Create),
    COMPONENT(BINARYINPUTSTREAM, nsBinaryInputStreamConstructor),
    COMPONENT(BINARYOUTPUTSTREAM, nsBinaryOutputStreamConstructor),
    COMPONENT(STORAGESTREAM, nsStorageStreamConstructor),
    COMPONENT(VERSIONCOMPARATOR, nsVersionComparatorImplConstructor),
    COMPONENT(PIPE, nsPipeConstructor),

#define NS_PROPERTIES_CLASSNAME  "Properties"
    COMPONENT(PROPERTIES, nsPropertiesConstructor),

#define NS_PERSISTENTPROPERTIES_CID NS_IPERSISTENTPROPERTIES_CID 
    COMPONENT(PERSISTENTPROPERTIES, nsPersistentProperties::Create),

    COMPONENT(SUPPORTSARRAY, nsSupportsArray::Create),
    COMPONENT(ARRAY, nsArrayConstructor),
    COMPONENT_CI_FLAGS(CONSOLESERVICE, nsConsoleServiceConstructor,
                       nsConsoleService,
                       nsIClassInfo::THREADSAFE | nsIClassInfo::SINGLETON),
    COMPONENT(EXCEPTIONSERVICE, nsExceptionServiceConstructor),
    COMPONENT(ATOMSERVICE, nsAtomServiceConstructor),
#ifdef MOZ_TIMELINE
    COMPONENT(TIMELINESERVICE, nsTimelineServiceConstructor),
#endif
    COMPONENT(OBSERVERSERVICE, nsObserverService::Create),
    COMPONENT(GENERICFACTORY, nsGenericFactory::Create),

#define NS_XPCOMPROXY_CID NS_PROXYEVENT_MANAGER_CID
    COMPONENT(XPCOMPROXY, nsProxyObjectManager::Create),

    COMPONENT(TIMER, nsTimerImplConstructor),

#define COMPONENT_SUPPORTS(TYPE, Type)                                         \
  COMPONENT(SUPPORTS_##TYPE, nsSupports##Type##ImplConstructor)

    COMPONENT_SUPPORTS(ID, ID),
    COMPONENT_SUPPORTS(STRING, String),
    COMPONENT_SUPPORTS(CSTRING, CString),
    COMPONENT_SUPPORTS(PRBOOL, PRBool),
    COMPONENT_SUPPORTS(PRUINT8, PRUint8),
    COMPONENT_SUPPORTS(PRUINT16, PRUint16),
    COMPONENT_SUPPORTS(PRUINT32, PRUint32),
    COMPONENT_SUPPORTS(PRUINT64, PRUint64),
    COMPONENT_SUPPORTS(PRTIME, PRTime),
    COMPONENT_SUPPORTS(CHAR, Char),
    COMPONENT_SUPPORTS(PRINT16, PRInt16),
    COMPONENT_SUPPORTS(PRINT32, PRInt32),
    COMPONENT_SUPPORTS(PRINT64, PRInt64),
    COMPONENT_SUPPORTS(FLOAT, Float),
    COMPONENT_SUPPORTS(DOUBLE, Double),
    COMPONENT_SUPPORTS(VOID, Void),
    COMPONENT_SUPPORTS(INTERFACE_POINTER, InterfacePointer),

#undef COMPONENT_SUPPORTS
#define NS_LOCAL_FILE_CLASSNAME "Local File Specification"
    COMPONENT(LOCAL_FILE, nsLocalFile::nsLocalFileConstructor),
#define NS_DIRECTORY_SERVICE_CLASSNAME  "nsIFile Directory Service"
    COMPONENT(DIRECTORY_SERVICE, nsDirectoryService::Create),
    COMPONENT(PROCESS, nsProcessConstructor),
    COMPONENT(ENVIRONMENT, nsEnvironment::Create),

    COMPONENT_CI_FLAGS(THREADMANAGER, nsThreadManagerGetSingleton,
                       nsThreadManager,
                       nsIClassInfo::THREADSAFE | nsIClassInfo::SINGLETON),
    COMPONENT_CI_FLAGS(THREADPOOL, nsThreadPoolConstructor,
                       nsThreadPool, nsIClassInfo::THREADSAFE),

    COMPONENT_CI_FLAGS(STRINGINPUTSTREAM, nsStringInputStreamConstructor,
                       nsStringInputStream, nsIClassInfo::THREADSAFE),
    COMPONENT(MULTIPLEXINPUTSTREAM, nsMultiplexInputStreamConstructor),

#ifndef MOZ_NO_FAST_LOAD
    COMPONENT(FASTLOADSERVICE, nsFastLoadService::Create),
#endif

    COMPONENT(VARIANT, nsVariantConstructor),
    COMPONENT(INTERFACEINFOMANAGER_SERVICE, nsXPTIInterfaceInfoManagerGetSingleton),

    COMPONENT(RECYCLINGALLOCATOR, nsRecyclingAllocatorImplConstructor),

#define NS_HASH_PROPERTY_BAG_CLASSNAME "Hashtable Property Bag"
    COMPONENT(HASH_PROPERTY_BAG, nsHashPropertyBagConstructor),

    COMPONENT(UUID_GENERATOR, nsUUIDGeneratorConstructor),

#if defined(XP_WIN)
    COMPONENT(WINDOWSREGKEY, nsWindowsRegKeyConstructor),
#endif

#ifdef XP_MACOSX
    COMPONENT(MACUTILSIMPL, nsMacUtilsImplConstructor),
#endif

    COMPONENT(SYSTEMINFO, nsSystemInfoConstructor),
#define NS_MEMORY_REPORTER_MANAGER_CLASSNAME "Memory Reporter Manager"
    COMPONENT(MEMORY_REPORTER_MANAGER, nsMemoryReporterManagerConstructor),
    COMPONENT(IOUTIL, nsIOUtilConstructor),
};

#undef COMPONENT

const int components_length = sizeof(components) / sizeof(components[0]);


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
    return NS_InitXPCOM3(result, binDirectory, nsnull, nsnull, 0);
}

EXPORT_XPCOM_API(nsresult)
NS_InitXPCOM2(nsIServiceManager* *result,
                              nsIFile* binDirectory,
                              nsIDirectoryServiceProvider* appFileLocationProvider)
{
    return NS_InitXPCOM3(result, binDirectory, appFileLocationProvider, nsnull, 0);
}

EXPORT_XPCOM_API(nsresult)
NS_InitXPCOM3(nsIServiceManager* *result,
                              nsIFile* binDirectory,
                              nsIDirectoryServiceProvider* appFileLocationProvider,
                              nsStaticModuleInfo const *staticComponents,
                              PRUint32 componentCount)
{
    nsresult rv = NS_OK;

#ifdef MOZ_ENABLE_LIBXUL
    if (!staticComponents) {
        staticComponents = kPStaticModules;
        componentCount = kStaticModuleCount;
    }
#endif

     
    gXPCOMShuttingDown = PR_FALSE;

    NS_LogInit();

    
    rv = TimeStamp::Startup();
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = nsThreadManager::get()->Init();
    if (NS_FAILED(rv)) return rv;

    
    rv = nsTimerImpl::Startup();
    NS_ENSURE_SUCCESS(rv, rv);

#ifndef WINCE
    
    
    if (strcmp(setlocale(LC_ALL, NULL), "C") == 0)
        setlocale(LC_ALL, "");
#endif

#if defined(XP_UNIX) || defined(XP_OS2)
    NS_StartupNativeCharsetUtils();
#endif
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

    NS_ASSERTION(nsComponentManagerImpl::gComponentManager == NULL, "CompMgr not null at init");

    
    nsComponentManagerImpl *compMgr = new nsComponentManagerImpl();
    if (compMgr == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(compMgr);
    
    rv = compMgr->Init(staticComponents, componentCount);
    if (NS_FAILED(rv))
    {
        NS_RELEASE(compMgr);
        return rv;
    }

    nsComponentManagerImpl::gComponentManager = compMgr;

    if (result) {
        nsIServiceManager *serviceManager =
            static_cast<nsIServiceManager*>(compMgr);

        NS_ADDREF(*result = serviceManager);
    }

    nsCOMPtr<nsIMemory> memory;
    NS_GetMemoryManager(getter_AddRefs(memory));
    rv = compMgr->RegisterService(kMemoryCID, memory);
    if (NS_FAILED(rv)) return rv;

    rv = compMgr->RegisterService(kComponentManagerCID, static_cast<nsIComponentManager*>(compMgr));
    if (NS_FAILED(rv)) return rv;

    rv = nsCycleCollector_startup();
    if (NS_FAILED(rv)) return rv;

    
    

    
    {
      nsCOMPtr<nsIFactory> categoryManagerFactory;
      if ( NS_FAILED(rv = NS_CategoryManagerGetFactory(getter_AddRefs(categoryManagerFactory))) )
        return rv;

      NS_DEFINE_CID(kCategoryManagerCID, NS_CATEGORYMANAGER_CID);

      rv = compMgr->RegisterFactory(kCategoryManagerCID,
                                    NS_CATEGORYMANAGER_CLASSNAME,
                                    NS_CATEGORYMANAGER_CONTRACTID,
                                    categoryManagerFactory,
                                    PR_TRUE);
      if ( NS_FAILED(rv) ) return rv;
    }

    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(
        static_cast<nsIComponentManager*>(compMgr), &rv);
    if (registrar) {
        for (int i = 0; i < components_length; i++)
            RegisterGenericFactory(registrar, &components[i]);

        nsCOMPtr<nsIFactory> iniParserFactory(new nsINIParserFactory());
        if (iniParserFactory)
            registrar->RegisterFactory(kINIParserFactoryCID, 
                                       "nsINIParserFactory",
                                       NS_INIPARSERFACTORY_CONTRACTID, 
                                       iniParserFactory);

        registrar->
          RegisterFactory(kSimpleUnicharStreamFactoryCID,
                          "nsSimpleUnicharStreamFactory",
                          NS_SIMPLE_UNICHAR_STREAM_FACTORY_CONTRACTID,
                          nsSimpleUnicharStreamFactory::GetInstance());
    }

    
    nsIInterfaceInfoManager* iim =
        xptiInterfaceInfoManager::GetInterfaceInfoManagerNoAddRef();

    
    rv = nsComponentManagerImpl::gComponentManager->ReadPersistentRegistry();
    if (NS_FAILED(rv)) {
        
        
        (void) iim->AutoRegisterInterfaces();
        nsComponentManagerImpl::gComponentManager->AutoRegister(nsnull);
    }

    
    
    
    nsDirectoryService::gService->RegisterCategoryProviders();

    
    NS_CreateServicesFromCategory(NS_XPCOM_STARTUP_CATEGORY, 
                                  nsnull,
                                  NS_XPCOM_STARTUP_OBSERVER_ID);
    
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

    
    
    
    gXPCOMShuttingDown = PR_TRUE;

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

#ifdef DEBUG
    
    _FreeAutoLockStatics();
#endif

    ShutdownSpecialSystemDirectory();

    NS_PurgeAtomTable();

    NS_IF_RELEASE(gDebug);

    TimeStamp::Shutdown();

#ifdef DEBUG
    











    BlockingResourceBase::Shutdown();
#endif
    
    NS_LogTerm();

    return NS_OK;
}

} 
