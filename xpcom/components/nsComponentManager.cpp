
















#include <stdlib.h>
#include "nscore.h"
#include "nsISupports.h"
#include "nspr.h"
#include "nsCRT.h" 





#define PL_ARENA_CONST_ALIGN_MASK 7
#define NS_CM_BLOCK_SIZE (1024 * 8)

#include "nsCategoryManager.h"
#include "nsCOMPtr.h"
#include "nsComponentManager.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCategoryManager.h"
#include "nsCategoryManagerUtils.h"
#include "nsIEnumerator.h"
#include "xptiprivate.h"
#include "nsIConsoleService.h"
#include "nsIMemoryReporter.h"
#include "nsIObserverService.h"
#include "nsISimpleEnumerator.h"
#include "nsIStringEnumerator.h"
#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"
#include "nsISupportsPrimitives.h"
#include "nsIClassInfo.h"
#include "nsLocalFile.h"
#include "nsReadableUtils.h"
#include "nsStaticComponents.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "prcmon.h"
#include "xptinfo.h" 
#include "nsThreadUtils.h"
#include "prthread.h"
#include "private/pprthred.h"
#include "nsTArray.h"
#include "prio.h"
#include "ManifestParser.h"
#include "mozilla/Services.h"

#include "nsManifestLineReader.h"
#include "mozilla/GenericFactory.h"
#include "nsSupportsPrimitives.h"
#include "nsArrayEnumerator.h"
#include "nsStringEnumerator.h"
#include "mozilla/FileUtils.h"

#include NEW_H     

#include "mozilla/Omnijar.h"

#include "prlog.h"

using namespace mozilla;

PRLogModuleInfo* nsComponentManagerLog = nullptr;

#if 0 || defined (DEBUG_timeless)
 #define SHOW_DENIED_ON_SHUTDOWN
 #define SHOW_CI_ON_EXISTING_SERVICE
#endif




#define BIG_REGISTRY_BUFLEN   (512*1024)


const char classIDKeyName[]="classID";
const char classesKeyName[]="contractID";
const char componentsKeyName[]="components";
const char xpcomComponentsKeyName[]="software/mozilla/XPCOM/components";
const char xpcomKeyName[]="software/mozilla/XPCOM";


const char classIDValueName[]="ClassID";
const char classNameValueName[]="ClassName";
const char componentCountValueName[]="ComponentsCount";
const char componentTypeValueName[]="ComponentType";
const char contractIDValueName[]="ContractID";
const char fileSizeValueName[]="FileSize";
const char inprocServerValueName[]="InprocServer";
const char lastModValueName[]="LastModTimeStamp";
const char nativeComponentType[]="application/x-mozilla-native";
const char staticComponentType[]="application/x-mozilla-static";
const char jarComponentType[]="application/x-mozilla-jarjs";
const char versionValueName[]="VersionString";

const static char XPCOM_ABSCOMPONENT_PREFIX[] = "abs:";
const static char XPCOM_RELCOMPONENT_PREFIX[] = "rel:";
const static char XPCOM_GRECOMPONENT_PREFIX[] = "gre:";

static const char gIDFormat[] =
  "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}";


#define NS_EMPTY_IID                                 \
{                                                    \
    0x00000000,                                      \
    0x0000,                                          \
    0x0000,                                          \
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} \
}

NS_DEFINE_CID(kEmptyCID, NS_EMPTY_IID);
NS_DEFINE_CID(kCategoryManagerCID, NS_CATEGORYMANAGER_CID);

#define UID_STRING_LENGTH 39

nsresult
nsGetServiceFromCategory::operator()(const nsIID& aIID, void** aInstancePtr) const
{
    nsresult rv;
    nsXPIDLCString value;
    nsCOMPtr<nsICategoryManager> catman;
    nsComponentManagerImpl *compMgr = nsComponentManagerImpl::gComponentManager;
    if (!compMgr) {
        rv = NS_ERROR_NOT_INITIALIZED;
        goto error;
    }

    if (!mCategory || !mEntry) {
        
        rv = NS_ERROR_NULL_POINTER;
        goto error;
    }

    rv = compMgr->nsComponentManagerImpl::GetService(kCategoryManagerCID,
                                                     NS_GET_IID(nsICategoryManager),
                                                     getter_AddRefs(catman));
    if (NS_FAILED(rv)) goto error;

    
    rv = catman->GetCategoryEntry(mCategory, mEntry,
                                  getter_Copies(value));
    if (NS_FAILED(rv)) goto error;
    if (!value) {
        rv = NS_ERROR_SERVICE_NOT_AVAILABLE;
        goto error;
    }

    rv = compMgr->
        nsComponentManagerImpl::GetServiceByContractID(value,
                                                       aIID, aInstancePtr);
    if (NS_FAILED(rv)) {
    error:
        *aInstancePtr = 0;
    }
    if (mErrorPtr)
        *mErrorPtr = rv;
    return rv;
}




char *
ArenaStrndup(const char *s, uint32_t len, PLArenaPool *arena)
{
    void *mem;
    
    PL_ARENA_ALLOCATE(mem, arena, len+1);
    if (mem)
        memcpy(mem, s, len+1);
    return static_cast<char *>(mem);
}

char*
ArenaStrdup(const char *s, PLArenaPool *arena)
{
    return ArenaStrndup(s, strlen(s), arena);
}


static already_AddRefed<nsIFile>
GetLocationFromDirectoryService(const char* prop)
{
    nsCOMPtr<nsIProperties> directoryService;
    nsDirectoryService::Create(nullptr,
                               NS_GET_IID(nsIProperties),
                               getter_AddRefs(directoryService));

    if (!directoryService)
        return NULL;

    nsCOMPtr<nsIFile> file;
    nsresult rv = directoryService->Get(prop,
                                        NS_GET_IID(nsIFile),
                                        getter_AddRefs(file));
    if (NS_FAILED(rv))
        return NULL;

    return file.forget();
}

static already_AddRefed<nsIFile>
CloneAndAppend(nsIFile* aBase, const nsACString& append)
{
    nsCOMPtr<nsIFile> f;
    aBase->Clone(getter_AddRefs(f));
    if (!f)
        return NULL;

    f->AppendNative(append);
    return f.forget();
}





NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(ComponentManagerMallocSizeOf)

static int64_t
GetComponentManagerSize()
{
  MOZ_ASSERT(nsComponentManagerImpl::gComponentManager);
  return nsComponentManagerImpl::gComponentManager->SizeOfIncludingThis(
           ComponentManagerMallocSizeOf);
}

NS_MEMORY_REPORTER_IMPLEMENT(ComponentManager,
    "explicit/xpcom/component-manager",
    KIND_HEAP,
    nsIMemoryReporter::UNITS_BYTES,
    GetComponentManagerSize,
    "Memory used for the XPCOM component manager.")

nsresult
nsComponentManagerImpl::Create(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    if (!gComponentManager)
        return NS_ERROR_FAILURE;

    return gComponentManager->QueryInterface(aIID, aResult);
}

nsComponentManagerImpl::nsComponentManagerImpl()
    : mMon("nsComponentManagerImpl.mMon")
    , mStatus(NOT_INITIALIZED)
{
}

#define CONTRACTID_HASHTABLE_INITIAL_SIZE   2048

nsTArray<const mozilla::Module*>* nsComponentManagerImpl::sStaticModules;

 void
nsComponentManagerImpl::InitializeStaticModules()
{
    if (sStaticModules)
        return;

    sStaticModules = new nsTArray<const mozilla::Module*>;
    for (const mozilla::Module *const *const *staticModules = kPStaticModules;
         *staticModules; ++staticModules)
        sStaticModules->AppendElement(**staticModules);
}

nsTArray<nsComponentManagerImpl::ComponentLocation>*
nsComponentManagerImpl::sModuleLocations;

 void
nsComponentManagerImpl::InitializeModuleLocations()
{
    if (sModuleLocations)
        return;

    sModuleLocations = new nsTArray<ComponentLocation>;
}

nsresult nsComponentManagerImpl::Init()
{
    PR_ASSERT(NOT_INITIALIZED == mStatus);

    if (nsComponentManagerLog == nullptr)
    {
        nsComponentManagerLog = PR_NewLogModule("nsComponentManager");
    }

    
    PL_INIT_ARENA_POOL(&mArena, "ComponentManagerArena", NS_CM_BLOCK_SIZE);

    mFactories.Init(CONTRACTID_HASHTABLE_INITIAL_SIZE);
    mContractIDs.Init(CONTRACTID_HASHTABLE_INITIAL_SIZE);
    mLoaderMap.Init();
    mKnownModules.Init();

    nsCOMPtr<nsIFile> greDir =
        GetLocationFromDirectoryService(NS_GRE_DIR);
    nsCOMPtr<nsIFile> appDir =
        GetLocationFromDirectoryService(NS_XPCOM_CURRENT_PROCESS_DIR);

    InitializeStaticModules();
    InitializeModuleLocations();

    ComponentLocation* cl = sModuleLocations->InsertElementAt(0);
    nsCOMPtr<nsIFile> lf = CloneAndAppend(appDir, NS_LITERAL_CSTRING("chrome.manifest"));
    cl->type = NS_COMPONENT_LOCATION;
    cl->location.Init(lf);

    bool equals = false;
    appDir->Equals(greDir, &equals);
    if (!equals) {
        cl = sModuleLocations->InsertElementAt(0);
        cl->type = NS_COMPONENT_LOCATION;
        lf = CloneAndAppend(greDir, NS_LITERAL_CSTRING("chrome.manifest"));
        cl->location.Init(lf);
    }

    PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG,
           ("nsComponentManager: Initialized."));

    nsresult rv = mNativeModuleLoader.Init();
    if (NS_FAILED(rv))
        return rv;

    nsCategoryManager::GetSingleton()->SuppressNotifications(true);

    RegisterModule(&kXPCOMModule, NULL);

    for (uint32_t i = 0; i < sStaticModules->Length(); ++i)
        RegisterModule((*sStaticModules)[i], NULL);

    nsRefPtr<nsZipArchive> appOmnijar = mozilla::Omnijar::GetReader(mozilla::Omnijar::APP);
    if (appOmnijar) {
        cl = sModuleLocations->InsertElementAt(1); 
        cl->type = NS_COMPONENT_LOCATION;
        cl->location.Init(appOmnijar, "chrome.manifest");
    }
    nsRefPtr<nsZipArchive> greOmnijar = mozilla::Omnijar::GetReader(mozilla::Omnijar::GRE);
    if (greOmnijar) {
        cl = sModuleLocations->InsertElementAt(0);
        cl->type = NS_COMPONENT_LOCATION;
        cl->location.Init(greOmnijar, "chrome.manifest");
    }

    RereadChromeManifests(false);

    nsCategoryManager::GetSingleton()->SuppressNotifications(false);

    mReporter = new NS_MEMORY_REPORTER_NAME(ComponentManager);
    (void)::NS_RegisterMemoryReporter(mReporter);

    
    
    
    
    nsCategoryManager::GetSingleton()->InitMemoryReporter();

    mStatus = NORMAL;

    return NS_OK;
}

void
nsComponentManagerImpl::RegisterModule(const mozilla::Module* aModule,
                                       FileLocation* aFile)
{
    ReentrantMonitorAutoEnter mon(mMon);

    KnownModule* m;
    if (aFile) {
        nsCString uri;
        aFile->GetURIString(uri);
        NS_ASSERTION(!mKnownModules.Get(uri),
                     "Must not register a binary module twice.");

        m = new KnownModule(aModule, *aFile);
        mKnownModules.Put(uri, m);
    } else {
        m = new KnownModule(aModule);
        mKnownStaticModules.AppendElement(m);
    }

    if (aModule->mCIDs) {
        const mozilla::Module::CIDEntry* entry;
        for (entry = aModule->mCIDs; entry->cid; ++entry)
            RegisterCIDEntry(entry, m);
    }

    if (aModule->mContractIDs) {
        const mozilla::Module::ContractIDEntry* entry;
        for (entry = aModule->mContractIDs; entry->contractid; ++entry)
            RegisterContractID(entry);
        NS_ASSERTION(!entry->cid, "Incorrectly terminated contract list");
    }
            
    if (aModule->mCategoryEntries) {
        const mozilla::Module::CategoryEntry* entry;
        for (entry = aModule->mCategoryEntries; entry->category; ++entry)
            nsCategoryManager::GetSingleton()->
                AddCategoryEntry(entry->category,
                                 entry->entry,
                                 entry->value);
    }
}

void
nsComponentManagerImpl::RegisterCIDEntry(const mozilla::Module::CIDEntry* aEntry,
                                         KnownModule* aModule)
{
    mMon.AssertCurrentThreadIn();

    nsFactoryEntry* f = mFactories.Get(*aEntry->cid);
    if (f) {
        NS_WARNING("Re-registering a CID?");

        char idstr[NSID_LENGTH];
        aEntry->cid->ToProvidedString(idstr);

        nsCString existing;
        if (f->mModule)
            existing = f->mModule->Description();
        else
            existing = "<unknown module>";

        LogMessage("While registering XPCOM module %s, trying to re-register CID '%s' already registered by %s.",
                   aModule->Description().get(),
                   idstr,
                   existing.get());
        return;
    }

    f = new nsFactoryEntry(aEntry, aModule);
    mFactories.Put(*aEntry->cid, f);
}

void
nsComponentManagerImpl::RegisterContractID(const mozilla::Module::ContractIDEntry* aEntry)
{
    mMon.AssertCurrentThreadIn();

    nsFactoryEntry* f = mFactories.Get(*aEntry->cid);
    if (!f) {
        NS_ERROR("No CID found when attempting to map contract ID");

        char idstr[NSID_LENGTH];
        aEntry->cid->ToProvidedString(idstr);

        LogMessage("Could not map contract ID '%s' to CID %s because no implementation of the CID is registered.",
                   aEntry->contractid,
                   idstr);
                   
        return;
    }

    mContractIDs.Put(nsDependentCString(aEntry->contractid), f);
}

static void
CutExtension(nsCString& path)
{
    int32_t dotPos = path.RFindChar('.');
    if (kNotFound == dotPos)
        path.Truncate();
    else
        path.Cut(0, dotPos + 1);
}

void
nsComponentManagerImpl::RegisterManifest(NSLocationType aType,
                                         FileLocation &aFile,
                                         bool aChromeOnly)
{
    uint32_t len;
    FileLocation::Data data;
    nsAutoArrayPtr<char> buf;
    nsresult rv = aFile.GetData(data);
    if (NS_SUCCEEDED(rv)) {
        rv = data.GetSize(&len);
    }
    if (NS_SUCCEEDED(rv)) {
        buf = new char[len + 1];
        rv = data.Copy(buf, len);
    }
    if (NS_SUCCEEDED(rv)) {
        buf[len] = '\0';
        ParseManifest(aType, aFile, buf, aChromeOnly);
    } else if (NS_BOOTSTRAPPED_LOCATION != aType) {
        nsCString uri;
        aFile.GetURIString(uri);
        LogMessage("Could not read chrome manifest '%s'.", uri.get());
    }
}

void
nsComponentManagerImpl::ManifestManifest(ManifestProcessingContext& cx, int lineno, char *const * argv)
{
    char* file = argv[0];
    FileLocation f(cx.mFile, file);
    RegisterManifest(cx.mType, f, cx.mChromeOnly);
}

void
nsComponentManagerImpl::ManifestBinaryComponent(ManifestProcessingContext& cx, int lineno, char *const * argv)
{
    if (cx.mFile.IsZip()) {
        NS_WARNING("Cannot load binary components from a jar.");
        LogMessageWithContext(cx.mFile, lineno,
                              "Cannot load binary components from a jar.");
        return;
    }

    FileLocation f(cx.mFile, argv[0]);
    nsCString uri;
    f.GetURIString(uri);

    if (mKnownModules.Get(uri)) {
        NS_WARNING("Attempting to register a binary component twice.");
        LogMessageWithContext(cx.mFile, lineno,
                              "Attempting to register a binary component twice.");
        return;
    }

    const mozilla::Module* m = mNativeModuleLoader.LoadModule(f);
    
    if (!m)
        return;

    RegisterModule(m, &f);
}

void
nsComponentManagerImpl::ManifestXPT(ManifestProcessingContext& cx, int lineno, char *const * argv)
{
    FileLocation f(cx.mFile, argv[0]);
    uint32_t len;
    FileLocation::Data data;
    nsAutoArrayPtr<char> buf;
    nsresult rv = f.GetData(data);
    if (NS_SUCCEEDED(rv)) {
        rv = data.GetSize(&len);
    }
    if (NS_SUCCEEDED(rv)) {
        buf = new char[len];
        rv = data.Copy(buf, len);
    }
    if (NS_SUCCEEDED(rv)) {
        xptiInterfaceInfoManager::GetSingleton()
            ->RegisterBuffer(buf, len);
    } else {
        nsCString uri;
        f.GetURIString(uri);
        LogMessage("Could not read '%s'.", uri.get());
    }
}

void
nsComponentManagerImpl::ManifestComponent(ManifestProcessingContext& cx, int lineno, char *const * argv)
{
    char* id = argv[0];
    char* file = argv[1];

    nsID cid;
    if (!cid.Parse(id)) {
        LogMessageWithContext(cx.mFile, lineno,
                              "Malformed CID: '%s'.", id);
        return;
    }

    ReentrantMonitorAutoEnter mon(mMon);
    nsFactoryEntry* f = mFactories.Get(cid);
    if (f) {
        char idstr[NSID_LENGTH];
        cid.ToProvidedString(idstr);

        nsCString existing;
        if (f->mModule)
            existing = f->mModule->Description();
        else
            existing = "<unknown module>";

        LogMessageWithContext(cx.mFile, lineno,
                              "Trying to re-register CID '%s' already registered by %s.",
                              idstr,
                              existing.get());
        return;
    }

    KnownModule* km;
    FileLocation fl(cx.mFile, file);

    nsCString hash;
    fl.GetURIString(hash);
    km = mKnownModules.Get(hash);
    if (!km) {
        km = new KnownModule(fl);
        mKnownModules.Put(hash, km);
    }

    void* place;

    PL_ARENA_ALLOCATE(place, &mArena, sizeof(nsCID));
    nsID* permanentCID = static_cast<nsID*>(place);
    *permanentCID = cid;

    PL_ARENA_ALLOCATE(place, &mArena, sizeof(mozilla::Module::CIDEntry));
    mozilla::Module::CIDEntry* e = new (place) mozilla::Module::CIDEntry();
    e->cid = permanentCID;

    f = new nsFactoryEntry(e, km);
    mFactories.Put(cid, f);
}

void
nsComponentManagerImpl::ManifestContract(ManifestProcessingContext& cx, int lineno, char *const * argv)
{
    char* contract = argv[0];
    char* id = argv[1];

    nsID cid;
    if (!cid.Parse(id)) {
        LogMessageWithContext(cx.mFile, lineno,
                              "Malformed CID: '%s'.", id);
        return;
    }

    ReentrantMonitorAutoEnter mon(mMon);
    nsFactoryEntry* f = mFactories.Get(cid);
    if (!f) {
        LogMessageWithContext(cx.mFile, lineno,
                              "Could not map contract ID '%s' to CID %s because no implementation of the CID is registered.",
                              contract, id);
        return;
    }

    mContractIDs.Put(nsDependentCString(contract), f);
}

void
nsComponentManagerImpl::ManifestCategory(ManifestProcessingContext& cx, int lineno, char *const * argv)
{
    char* category = argv[0];
    char* key = argv[1];
    char* value = argv[2];

    nsCategoryManager::GetSingleton()->
        AddCategoryEntry(category, key, value);
}

void
nsComponentManagerImpl::RereadChromeManifests(bool aChromeOnly)
{
    for (uint32_t i = 0; i < sModuleLocations->Length(); ++i) {
        ComponentLocation& l = sModuleLocations->ElementAt(i);
        RegisterManifest(l.type, l.location, aChromeOnly);
    }
}

bool
nsComponentManagerImpl::KnownModule::EnsureLoader()
{
    if (!mLoader) {
        nsCString extension;
        mFile.GetURIString(extension);
        CutExtension(extension);
        mLoader = nsComponentManagerImpl::gComponentManager->LoaderForExtension(extension);
    }
    return !!mLoader;
}

bool
nsComponentManagerImpl::KnownModule::Load()
{
    if (mFailed)
        return false;
    if (!mModule) {
        if (!EnsureLoader())
            return false;

        mModule = mLoader->LoadModule(mFile);

        if (!mModule) {
            mFailed = true;
            return false;
        }
    }
    if (!mLoaded) {
        if (mModule->loadProc) {
            nsresult rv = mModule->loadProc();
            if (NS_FAILED(rv)) {
                mFailed = true;
                return false;
            }
        }
        mLoaded = true;
    }
    return true;
}

nsCString
nsComponentManagerImpl::KnownModule::Description() const
{
    nsCString s;
    if (mFile)
        mFile.GetURIString(s);
    else
        s = "<static module>";
    return s;
}

nsresult nsComponentManagerImpl::Shutdown(void)
{
    PR_ASSERT(NORMAL == mStatus);

    mStatus = SHUTDOWN_IN_PROGRESS;

    
    PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG, ("nsComponentManager: Beginning Shutdown."));

    (void)::NS_UnregisterMemoryReporter(mReporter);
    mReporter = nullptr;

    
    mContractIDs.Clear();
    mFactories.Clear(); 
    mLoaderMap.Clear();
    mKnownModules.Clear();
    mKnownStaticModules.Clear();

    delete sStaticModules;
    delete sModuleLocations;

    
    mNativeModuleLoader.UnloadLibraries();

    
    PL_FinishArenaPool(&mArena);

    mStatus = SHUTDOWN_COMPLETE;

    PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG, ("nsComponentManager: Shutdown complete."));

    return NS_OK;
}

nsComponentManagerImpl::~nsComponentManagerImpl()
{
    PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG, ("nsComponentManager: Beginning destruction."));

    if (SHUTDOWN_COMPLETE != mStatus)
        Shutdown();

    PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG, ("nsComponentManager: Destroyed."));
}

NS_IMPL_THREADSAFE_ISUPPORTS5(nsComponentManagerImpl,
                              nsIComponentManager,
                              nsIServiceManager,
                              nsIComponentRegistrar,
                              nsISupportsWeakReference,
                              nsIInterfaceRequestor)


nsresult
nsComponentManagerImpl::GetInterface(const nsIID & uuid, void **result)
{
    NS_WARNING("This isn't supported");
    
    
    return  QueryInterface(uuid, result);
}

nsFactoryEntry *
nsComponentManagerImpl::GetFactoryEntry(const char *aContractID,
                                        uint32_t aContractIDLen)
{
    ReentrantMonitorAutoEnter mon(mMon);
    return mContractIDs.Get(nsDependentCString(aContractID, aContractIDLen));
}


nsFactoryEntry *
nsComponentManagerImpl::GetFactoryEntry(const nsCID &aClass)
{
    ReentrantMonitorAutoEnter mon(mMon);
    return mFactories.Get(aClass);
}

already_AddRefed<nsIFactory>
nsComponentManagerImpl::FindFactory(const nsCID& aClass)
{
    nsFactoryEntry* e = GetFactoryEntry(aClass);
    if (!e)
        return NULL;

    return e->GetFactory();
}

already_AddRefed<nsIFactory>
nsComponentManagerImpl::FindFactory(const char *contractID,
                                    uint32_t aContractIDLen)
{
    nsFactoryEntry *entry = GetFactoryEntry(contractID, aContractIDLen);
    if (!entry)
        return NULL;

    return entry->GetFactory();
}







NS_IMETHODIMP
nsComponentManagerImpl::GetClassObject(const nsCID &aClass, const nsIID &aIID,
                                       void **aResult)
{
    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_DEBUG))
    {
        char *buf = aClass.ToString();
        PR_LogPrint("nsComponentManager: GetClassObject(%s)", buf);
        if (buf)
            NS_Free(buf);
    }
#endif

    PR_ASSERT(aResult != nullptr);

    nsCOMPtr<nsIFactory> factory = FindFactory(aClass);
    if (!factory)
        return NS_ERROR_FACTORY_NOT_REGISTERED;

    rv = factory->QueryInterface(aIID, aResult);

    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("\t\tGetClassObject() %s", NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));

    return rv;
}


NS_IMETHODIMP
nsComponentManagerImpl::GetClassObjectByContractID(const char *contractID,
                                                   const nsIID &aIID,
                                                   void **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(contractID);

    nsresult rv;


#ifdef PR_LOGGING
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_DEBUG))
    {
        PR_LogPrint("nsComponentManager: GetClassObject(%s)", contractID);
    }
#endif

    nsCOMPtr<nsIFactory> factory = FindFactory(contractID, strlen(contractID));
    if (!factory)
        return NS_ERROR_FACTORY_NOT_REGISTERED;

    rv = factory->QueryInterface(aIID, aResult);

    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("\t\tGetClassObject() %s", NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));

    return rv;
}








NS_IMETHODIMP
nsComponentManagerImpl::CreateInstance(const nsCID &aClass,
                                       nsISupports *aDelegate,
                                       const nsIID &aIID,
                                       void **aResult)
{
    
    
    
    if (gXPCOMShuttingDown) {
        
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString cid, iid;
        cid.Adopt(aClass.ToString());
        iid.Adopt(aIID.ToString());
        fprintf(stderr, "Creating new instance on shutdown. Denied.\n"
               "         CID: %s\n         IID: %s\n", cid.get(), iid.get());
#endif 
        return NS_ERROR_UNEXPECTED;
    }

    if (aResult == nullptr)
    {
        return NS_ERROR_NULL_POINTER;
    }
    *aResult = nullptr;

    nsFactoryEntry *entry = GetFactoryEntry(aClass);

    if (!entry)
        return NS_ERROR_FACTORY_NOT_REGISTERED;

#ifdef SHOW_CI_ON_EXISTING_SERVICE
    if (entry->mServiceObject) {
        nsXPIDLCString cid;
        cid.Adopt(aClass.ToString());
        nsAutoCString message;
        message = NS_LITERAL_CSTRING("You are calling CreateInstance \"") +
                  cid + NS_LITERAL_CSTRING("\" when a service for this CID already exists!");
        NS_ERROR(message.get());
    }
#endif

    nsresult rv;
    nsCOMPtr<nsIFactory> factory = entry->GetFactory();
    if (factory)
    {
        rv = factory->CreateInstance(aDelegate, aIID, aResult);
        if (NS_SUCCEEDED(rv) && !*aResult) {
            NS_ERROR("Factory did not return an object but returned success!");
            rv = NS_ERROR_SERVICE_NOT_FOUND;
        }
    }
    else {
        
        rv = NS_ERROR_FACTORY_NOT_REGISTERED;
    }

#ifdef PR_LOGGING
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_WARNING))
    {
        char *buf = aClass.ToString();
        PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
               ("nsComponentManager: CreateInstance(%s) %s", buf,
                NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));
        if (buf)
            NS_Free(buf);
    }
#endif

    return rv;
}










NS_IMETHODIMP
nsComponentManagerImpl::CreateInstanceByContractID(const char *aContractID,
                                                   nsISupports *aDelegate,
                                                   const nsIID &aIID,
                                                   void **aResult)
{
    NS_ENSURE_ARG_POINTER(aContractID);

    
    
    
    if (gXPCOMShuttingDown) {
        
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString iid;
        iid.Adopt(aIID.ToString());
        fprintf(stderr, "Creating new instance on shutdown. Denied.\n"
               "  ContractID: %s\n         IID: %s\n", aContractID, iid.get());
#endif 
        return NS_ERROR_UNEXPECTED;
    }

    if (aResult == nullptr)
    {
        return NS_ERROR_NULL_POINTER;
    }
    *aResult = nullptr;

    nsFactoryEntry *entry = GetFactoryEntry(aContractID, strlen(aContractID));

    if (!entry)
        return NS_ERROR_FACTORY_NOT_REGISTERED;

#ifdef SHOW_CI_ON_EXISTING_SERVICE
    if (entry->mServiceObject) {
        nsAutoCString message;
        message =
          NS_LITERAL_CSTRING("You are calling CreateInstance \"") +
          nsDependentCString(aContractID) +
          NS_LITERAL_CSTRING("\" when a service for this CID already exists! "
            "Add it to abusedContracts to track down the service consumer.");
        NS_ERROR(message.get());
    }
#endif

    nsresult rv;
    nsCOMPtr<nsIFactory> factory = entry->GetFactory();
    if (factory)
    {

        rv = factory->CreateInstance(aDelegate, aIID, aResult);
        if (NS_SUCCEEDED(rv) && !*aResult) {
            NS_ERROR("Factory did not return an object but returned success!");
            rv = NS_ERROR_SERVICE_NOT_FOUND;
        }
    }
    else {
        
        rv = NS_ERROR_FACTORY_NOT_REGISTERED;
    }

    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("nsComponentManager: CreateInstanceByContractID(%s) %s", aContractID,
            NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));

    return rv;
}

static PLDHashOperator
FreeFactoryEntries(const nsID& aCID,
                   nsFactoryEntry* aEntry,
                   void* arg)
{
    aEntry->mFactory = NULL;
    aEntry->mServiceObject = NULL;
    return PL_DHASH_NEXT;
}

nsresult
nsComponentManagerImpl::FreeServices()
{
    NS_ASSERTION(gXPCOMShuttingDown, "Must be shutting down in order to free all services");

    if (!gXPCOMShuttingDown)
        return NS_ERROR_FAILURE;

    mFactories.EnumerateRead(FreeFactoryEntries, NULL);
    return NS_OK;
}


nsComponentManagerImpl::PendingServiceInfo*
nsComponentManagerImpl::AddPendingService(const nsCID& aServiceCID,
                                          PRThread* aThread)
{
  PendingServiceInfo* newInfo = mPendingServices.AppendElement();
  if (newInfo) {
    newInfo->cid = &aServiceCID;
    newInfo->thread = aThread;
  }
  return newInfo;
}


void
nsComponentManagerImpl::RemovePendingService(const nsCID& aServiceCID)
{
  uint32_t pendingCount = mPendingServices.Length();
  for (uint32_t index = 0; index < pendingCount; ++index) {
    const PendingServiceInfo& info = mPendingServices.ElementAt(index);
    if (info.cid->Equals(aServiceCID)) {
      mPendingServices.RemoveElementAt(index);
      return;
    }
  }
}


PRThread*
nsComponentManagerImpl::GetPendingServiceThread(const nsCID& aServiceCID) const
{
  uint32_t pendingCount = mPendingServices.Length();
  for (uint32_t index = 0; index < pendingCount; ++index) {
    const PendingServiceInfo& info = mPendingServices.ElementAt(index);
    if (info.cid->Equals(aServiceCID)) {
      return info.thread;
    }
  }
  return nullptr;
}












struct NS_STACK_CLASS AutoReentrantMonitor
{
    AutoReentrantMonitor(ReentrantMonitor& aReentrantMonitor) : mReentrantMonitor(&aReentrantMonitor), mEnterCount(0)
    {
        Enter();
    }

    ~AutoReentrantMonitor()
    {
        if (mEnterCount) {
            Exit();
        }
    }

    void Enter()
    {
        mReentrantMonitor->Enter();
        ++mEnterCount;
    }

    void Exit()
    {
        --mEnterCount;
        mReentrantMonitor->Exit();
    }

    ReentrantMonitor* mReentrantMonitor;
    int32_t mEnterCount;
};

NS_IMETHODIMP
nsComponentManagerImpl::GetService(const nsCID& aClass,
                                   const nsIID& aIID,
                                   void* *result)
{
    
    
    
    if (gXPCOMShuttingDown) {
        
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString cid, iid;
        cid.Adopt(aClass.ToString());
        iid.Adopt(aIID.ToString());
        fprintf(stderr, "Getting service on shutdown. Denied.\n"
               "         CID: %s\n         IID: %s\n", cid.get(), iid.get());
#endif 
        return NS_ERROR_UNEXPECTED;
    }

    AutoReentrantMonitor mon(mMon);

    nsFactoryEntry* entry = mFactories.Get(aClass);
    if (!entry)
        return NS_ERROR_FACTORY_NOT_REGISTERED;

    if (entry->mServiceObject) {
        nsCOMPtr<nsISupports> supports = entry->mServiceObject;
        mon.Exit();
        return supports->QueryInterface(aIID, result);
    }

    PRThread* currentPRThread = PR_GetCurrentThread();
    NS_ASSERTION(currentPRThread, "This should never be null!");

    
    nsIThread* currentThread = nullptr;

    PRThread* pendingPRThread;
    while ((pendingPRThread = GetPendingServiceThread(aClass))) {
        if (pendingPRThread == currentPRThread) {
            NS_ERROR("Recursive GetService!");
            return NS_ERROR_NOT_AVAILABLE;
        }

        mon.Exit();

        if (!currentThread) {
            currentThread = NS_GetCurrentThread();
            NS_ASSERTION(currentThread, "This should never be null!");
        }

        
        
        if (!NS_ProcessNextEvent(currentThread, false)) {
            PR_Sleep(PR_INTERVAL_NO_WAIT);
        }

        mon.Enter();
    }

    
    
    if (entry->mServiceObject) {
        nsCOMPtr<nsISupports> supports = entry->mServiceObject;
        mon.Exit();
        return supports->QueryInterface(aIID, result);
    }

#ifdef DEBUG
    PendingServiceInfo* newInfo =
#endif
    AddPendingService(aClass, currentPRThread);
    NS_ASSERTION(newInfo, "Failed to add info to the array!");

    nsCOMPtr<nsISupports> service;
    
    
    
    mon.Exit();

    nsresult rv = CreateInstance(aClass, nullptr, aIID, getter_AddRefs(service));

    mon.Enter();

#ifdef DEBUG
    pendingPRThread = GetPendingServiceThread(aClass);
    NS_ASSERTION(pendingPRThread == currentPRThread,
                 "Pending service array has been changed!");
#endif
    RemovePendingService(aClass);

    if (NS_FAILED(rv))
        return rv;

    NS_ASSERTION(!entry->mServiceObject, "Created two instances of a service!");

    entry->mServiceObject = service;
    *result = service.get();
    if (!*result) {
        NS_ERROR("Factory did not return an object but returned success!");
        return NS_ERROR_SERVICE_NOT_FOUND;
    }
    NS_ADDREF(static_cast<nsISupports*>((*result)));
    return rv;
}

NS_IMETHODIMP
nsComponentManagerImpl::IsServiceInstantiated(const nsCID & aClass,
                                              const nsIID& aIID,
                                              bool *result)
{
    
    

    
    
    
    if (gXPCOMShuttingDown) {
        
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString cid, iid;
        cid.Adopt(aClass.ToString());
        iid.Adopt(aIID.ToString());
        fprintf(stderr, "Checking for service on shutdown. Denied.\n"
               "         CID: %s\n         IID: %s\n", cid.get(), iid.get());
#endif 
        return NS_ERROR_UNEXPECTED;
    }

    nsresult rv = NS_ERROR_SERVICE_NOT_AVAILABLE;
    nsFactoryEntry* entry;

    {
        ReentrantMonitorAutoEnter mon(mMon);
        entry = mFactories.Get(aClass);
    }

    if (entry && entry->mServiceObject) {
        nsCOMPtr<nsISupports> service;
        rv = entry->mServiceObject->QueryInterface(aIID, getter_AddRefs(service));
        *result = (service!=nullptr);
    }

    return rv;
}

NS_IMETHODIMP nsComponentManagerImpl::IsServiceInstantiatedByContractID(const char *aContractID,
                                                                        const nsIID& aIID,
                                                                        bool *result)
{
    
    

    
    
    
    if (gXPCOMShuttingDown) {
        
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString iid;
        iid.Adopt(aIID.ToString());
        fprintf(stderr, "Checking for service on shutdown. Denied.\n"
               "  ContractID: %s\n         IID: %s\n", aContractID, iid.get());
#endif 
        return NS_ERROR_UNEXPECTED;
    }

    nsresult rv = NS_ERROR_SERVICE_NOT_AVAILABLE;
    nsFactoryEntry *entry;
    {
        ReentrantMonitorAutoEnter mon(mMon);
        entry = mContractIDs.Get(nsDependentCString(aContractID));
    }

    if (entry && entry->mServiceObject) {
        nsCOMPtr<nsISupports> service;
        rv = entry->mServiceObject->QueryInterface(aIID, getter_AddRefs(service));
        *result = (service!=nullptr);
    }
    return rv;
}


NS_IMETHODIMP
nsComponentManagerImpl::GetServiceByContractID(const char* aContractID,
                                               const nsIID& aIID,
                                               void* *result)
{
    
    
    
    if (gXPCOMShuttingDown) {
        
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString iid;
        iid.Adopt(aIID.ToString());
        fprintf(stderr, "Getting service on shutdown. Denied.\n"
               "  ContractID: %s\n         IID: %s\n", aContractID, iid.get());
#endif 
        return NS_ERROR_UNEXPECTED;
    }

    AutoReentrantMonitor mon(mMon);

    nsFactoryEntry *entry = mContractIDs.Get(nsDependentCString(aContractID));
    if (!entry)
        return NS_ERROR_FACTORY_NOT_REGISTERED;

    if (entry->mServiceObject) {
        nsCOMPtr<nsISupports> serviceObject = entry->mServiceObject;

        
        
        
        
        mon.Exit();
        return serviceObject->QueryInterface(aIID, result);
    }

    PRThread* currentPRThread = PR_GetCurrentThread();
    NS_ASSERTION(currentPRThread, "This should never be null!");

    
    nsIThread* currentThread = nullptr;

    PRThread* pendingPRThread;
    while ((pendingPRThread = GetPendingServiceThread(*entry->mCIDEntry->cid))) {
        if (pendingPRThread == currentPRThread) {
            NS_ERROR("Recursive GetService!");
            return NS_ERROR_NOT_AVAILABLE;
        }

        mon.Exit();

        if (!currentThread) {
            currentThread = NS_GetCurrentThread();
            NS_ASSERTION(currentThread, "This should never be null!");
        }

        
        
        if (!NS_ProcessNextEvent(currentThread, false)) {
            PR_Sleep(PR_INTERVAL_NO_WAIT);
        }

        mon.Enter();
    }

    if (currentThread && entry->mServiceObject) {
        
        
        nsCOMPtr<nsISupports> serviceObject = entry->mServiceObject;
        mon.Exit();
        return serviceObject->QueryInterface(aIID, result);
    }

#ifdef DEBUG
    PendingServiceInfo* newInfo =
#endif
    AddPendingService(*entry->mCIDEntry->cid, currentPRThread);
    NS_ASSERTION(newInfo, "Failed to add info to the array!");

    nsCOMPtr<nsISupports> service;
    
    
    
    mon.Exit();

    nsresult rv = CreateInstanceByContractID(aContractID, nullptr, aIID,
                                             getter_AddRefs(service));

    mon.Enter();

#ifdef DEBUG
    pendingPRThread = GetPendingServiceThread(*entry->mCIDEntry->cid);
    NS_ASSERTION(pendingPRThread == currentPRThread,
                 "Pending service array has been changed!");
#endif
    RemovePendingService(*entry->mCIDEntry->cid);

    if (NS_FAILED(rv))
        return rv;

    NS_ASSERTION(!entry->mServiceObject, "Created two instances of a service!");

    entry->mServiceObject = service;
    *result = service.get();
    NS_ADDREF(static_cast<nsISupports*>(*result));
    return rv;
}

already_AddRefed<mozilla::ModuleLoader>
nsComponentManagerImpl::LoaderForExtension(const nsACString& aExt)
{
    nsCOMPtr<mozilla::ModuleLoader> loader = mLoaderMap.Get(aExt);
    if (!loader) {
        loader = do_GetServiceFromCategory("module-loader",
                                           PromiseFlatCString(aExt).get());
        if (!loader)
            return NULL;

        mLoaderMap.Put(aExt, loader);
    }

    return loader.forget();
}

NS_IMETHODIMP
nsComponentManagerImpl::RegisterFactory(const nsCID& aClass,
                                        const char* aName,
                                        const char* aContractID,
                                        nsIFactory* aFactory)
{
    if (!aFactory) {
        
        
        if (!aContractID)
            return NS_ERROR_INVALID_ARG;

        ReentrantMonitorAutoEnter mon(mMon);
        nsFactoryEntry* oldf = mFactories.Get(aClass);
        if (!oldf)
            return NS_ERROR_FACTORY_NOT_REGISTERED;

        mContractIDs.Put(nsDependentCString(aContractID), oldf);
        return NS_OK;
    }

    nsAutoPtr<nsFactoryEntry> f(new nsFactoryEntry(aClass, aFactory));

    ReentrantMonitorAutoEnter mon(mMon);
    nsFactoryEntry* oldf = mFactories.Get(aClass);
    if (oldf)
        return NS_ERROR_FACTORY_EXISTS;

    if (aContractID)
        mContractIDs.Put(nsDependentCString(aContractID), f);

    mFactories.Put(aClass, f.forget());

    return NS_OK;
}

NS_IMETHODIMP
nsComponentManagerImpl::UnregisterFactory(const nsCID& aClass,
                                          nsIFactory* aFactory)
{
    
    
    nsCOMPtr<nsIFactory> dyingFactory;
    nsCOMPtr<nsISupports> dyingServiceObject;

    {
        ReentrantMonitorAutoEnter mon(mMon);
        nsFactoryEntry* f = mFactories.Get(aClass);
        if (!f || f->mFactory != aFactory)
            return NS_ERROR_FACTORY_NOT_REGISTERED;

        mFactories.Remove(aClass);

        
        
        
        f->mFactory.swap(dyingFactory);
        f->mServiceObject.swap(dyingServiceObject);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsComponentManagerImpl::AutoRegister(nsIFile* aLocation)
{
    XRE_AddManifestLocation(NS_COMPONENT_LOCATION, aLocation);
    return NS_OK;
}

NS_IMETHODIMP
nsComponentManagerImpl::AutoUnregister(nsIFile* aLocation)
{
    NS_ERROR("AutoUnregister not implemented.");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsComponentManagerImpl::RegisterFactoryLocation(const nsCID& aCID,
                                                const char* aClassName,
                                                const char* aContractID,
                                                nsIFile* aFile,
                                                const char* aLoaderStr,
                                                const char* aType)
{
    NS_ERROR("RegisterFactoryLocation not implemented.");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsComponentManagerImpl::UnregisterFactoryLocation(const nsCID& aCID,
                                                  nsIFile* aFile)
{
    NS_ERROR("UnregisterFactoryLocation not implemented.");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsComponentManagerImpl::IsCIDRegistered(const nsCID & aClass,
                                        bool *_retval)
{
    *_retval = (nullptr != GetFactoryEntry(aClass));
    return NS_OK;
}

NS_IMETHODIMP
nsComponentManagerImpl::IsContractIDRegistered(const char *aClass,
                                               bool *_retval)
{
    NS_ENSURE_ARG_POINTER(aClass);
    nsFactoryEntry *entry = GetFactoryEntry(aClass, strlen(aClass));

    if (entry)
        *_retval = true;
    else
        *_retval = false;
    return NS_OK;
}

static PLDHashOperator
EnumerateCIDHelper(const nsID& id, nsFactoryEntry* entry, void* closure)
{
    nsCOMArray<nsISupports> *array = static_cast<nsCOMArray<nsISupports>*>(closure);
    nsCOMPtr<nsISupportsID> wrapper = new nsSupportsIDImpl();
    wrapper->SetData(&id);
    array->AppendObject(wrapper);
    return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsComponentManagerImpl::EnumerateCIDs(nsISimpleEnumerator **aEnumerator)
{
    nsCOMArray<nsISupports> array;
    mFactories.EnumerateRead(EnumerateCIDHelper, &array);

    return NS_NewArrayEnumerator(aEnumerator, array);
}

static PLDHashOperator
EnumerateContractsHelper(const nsACString& contract, nsFactoryEntry* entry, void* closure)
{
    nsTArray<nsCString>* array = static_cast<nsTArray<nsCString>*>(closure);
    array->AppendElement(contract);
    return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsComponentManagerImpl::EnumerateContractIDs(nsISimpleEnumerator **aEnumerator)
{
    nsTArray<nsCString>* array = new nsTArray<nsCString>;
    mContractIDs.EnumerateRead(EnumerateContractsHelper, array);

    nsCOMPtr<nsIUTF8StringEnumerator> e;
    nsresult rv = NS_NewAdoptingUTF8StringEnumerator(getter_AddRefs(e), array);
    if (NS_FAILED(rv))
        return rv;

    return CallQueryInterface(e, aEnumerator);
}

NS_IMETHODIMP
nsComponentManagerImpl::CIDToContractID(const nsCID & aClass,
                                        char **_retval)
{
    NS_ERROR("CIDTOContractID not implemented");
    return NS_ERROR_FACTORY_NOT_REGISTERED;
}

NS_IMETHODIMP
nsComponentManagerImpl::ContractIDToCID(const char *aContractID,
                                        nsCID * *_retval)
{
    {
        ReentrantMonitorAutoEnter mon(mMon);
        nsFactoryEntry* entry = mContractIDs.Get(nsDependentCString(aContractID));
        if (entry) {
            *_retval = (nsCID*) NS_Alloc(sizeof(nsCID));
            **_retval = *entry->mCIDEntry->cid;
            return NS_OK;
        }
    }
    *_retval = NULL;
    return NS_ERROR_FACTORY_NOT_REGISTERED;
}

static size_t
SizeOfFactoriesEntryExcludingThis(nsIDHashKey::KeyType aKey,
                                  nsFactoryEntry* const &aData,
                                  nsMallocSizeOfFun aMallocSizeOf,
                                  void* aUserArg)
{
    return aData->SizeOfIncludingThis(aMallocSizeOf);
}

static size_t
SizeOfContractIDsEntryExcludingThis(nsCStringHashKey::KeyType aKey,
                                    nsFactoryEntry* const &aData,
                                    nsMallocSizeOfFun aMallocSizeOf,
                                    void* aUserArg)
{
    
    
    return aKey.SizeOfExcludingThisMustBeUnshared(aMallocSizeOf);
}

size_t
nsComponentManagerImpl::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)
{
    size_t n = aMallocSizeOf(this);
    n += mLoaderMap.SizeOfExcludingThis(nullptr, aMallocSizeOf);
    n += mFactories.SizeOfExcludingThis(SizeOfFactoriesEntryExcludingThis, aMallocSizeOf);
    n += mContractIDs.SizeOfExcludingThis(SizeOfContractIDsEntryExcludingThis, aMallocSizeOf);

    n += sStaticModules->SizeOfIncludingThis(aMallocSizeOf);
    n += sModuleLocations->SizeOfIncludingThis(aMallocSizeOf);

    n += mKnownStaticModules.SizeOfExcludingThis(aMallocSizeOf);
    n += mKnownModules.SizeOfExcludingThis(nullptr, aMallocSizeOf);

    
    
    
    const PLArena *arena = mArena.first.next;
    while (arena) {
        n += aMallocSizeOf(arena);
        arena = arena->next;
    }

    n += mPendingServices.SizeOfExcludingThis(aMallocSizeOf);

    
    
    
    
    
    
    
    
    

    return n;
}





nsFactoryEntry::nsFactoryEntry(const mozilla::Module::CIDEntry* entry,
                               nsComponentManagerImpl::KnownModule* module)
    : mCIDEntry(entry)
    , mModule(module)
{
}

nsFactoryEntry::nsFactoryEntry(const nsCID& aCID, nsIFactory* factory)
    : mCIDEntry(NULL)
    , mModule(NULL)
    , mFactory(factory)
{
    mozilla::Module::CIDEntry* e = new mozilla::Module::CIDEntry();
    nsCID* cid = new nsCID;
    *cid = aCID;
    e->cid = cid;
    mCIDEntry = e;
}        

nsFactoryEntry::~nsFactoryEntry()
{
    
    if (!mModule) {
        delete mCIDEntry->cid;
        delete mCIDEntry;
    }
}

already_AddRefed<nsIFactory>
nsFactoryEntry::GetFactory()
{
    if (!mFactory) {
        
        
        if (!mModule)
            return NULL;

        if (!mModule->Load())
            return NULL;

        if (mModule->Module()->getFactoryProc) {
            mFactory = mModule->Module()->getFactoryProc(*mModule->Module(),
                                                         *mCIDEntry);
        }
        else if (mCIDEntry->getFactoryProc) {
            mFactory = mCIDEntry->getFactoryProc(*mModule->Module(), *mCIDEntry);
        }
        else {
            NS_ASSERTION(mCIDEntry->constructorProc, "no getfactory or constructor");
            mFactory = new mozilla::GenericFactory(mCIDEntry->constructorProc);
        }
        if (!mFactory)
            return NULL;
    }
    nsIFactory* factory = mFactory.get();
    NS_ADDREF(factory);
    return factory;
}

size_t
nsFactoryEntry::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)
{
    size_t n = aMallocSizeOf(this);

    
    
    
    
    
    

    return n;
}





nsresult
NS_GetComponentManager(nsIComponentManager* *result)
{
    if (!nsComponentManagerImpl::gComponentManager)
        return NS_ERROR_NOT_INITIALIZED;

    NS_ADDREF(*result = nsComponentManagerImpl::gComponentManager);
    return NS_OK;
}

nsresult
NS_GetServiceManager(nsIServiceManager* *result)
{
    if (!nsComponentManagerImpl::gComponentManager)
        return NS_ERROR_NOT_INITIALIZED;

    NS_ADDREF(*result = nsComponentManagerImpl::gComponentManager);
    return NS_OK;
}


nsresult
NS_GetComponentRegistrar(nsIComponentRegistrar* *result)
{
    if (!nsComponentManagerImpl::gComponentManager)
        return NS_ERROR_NOT_INITIALIZED;

    NS_ADDREF(*result = nsComponentManagerImpl::gComponentManager);
    return NS_OK;
}

EXPORT_XPCOM_API(nsresult)
XRE_AddStaticComponent(const mozilla::Module* aComponent)
{
    nsComponentManagerImpl::InitializeStaticModules();
    nsComponentManagerImpl::sStaticModules->AppendElement(aComponent);

    if (nsComponentManagerImpl::gComponentManager &&
        nsComponentManagerImpl::NORMAL == nsComponentManagerImpl::gComponentManager->mStatus)
        nsComponentManagerImpl::gComponentManager->RegisterModule(aComponent, NULL);

    return NS_OK;
}

NS_IMETHODIMP
nsComponentManagerImpl::AddBootstrappedManifestLocation(nsIFile* aLocation)
{
  nsString path;
  nsresult rv = aLocation->GetPath(path);
  if (NS_FAILED(rv))
    return rv;

  if (Substring(path, path.Length() - 4).Equals(NS_LITERAL_STRING(".xpi"))) {
    return XRE_AddJarManifestLocation(NS_BOOTSTRAPPED_LOCATION, aLocation);
  }

  nsCOMPtr<nsIFile> manifest =
    CloneAndAppend(aLocation, NS_LITERAL_CSTRING("chrome.manifest"));
  return XRE_AddManifestLocation(NS_BOOTSTRAPPED_LOCATION, manifest);
}

NS_IMETHODIMP
nsComponentManagerImpl::RemoveBootstrappedManifestLocation(nsIFile* aLocation)
{
  nsCOMPtr<nsIChromeRegistry> cr = mozilla::services::GetChromeRegistryService();
  if (!cr)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIFile> manifest;
  nsString path;
  nsresult rv = aLocation->GetPath(path);
  if (NS_FAILED(rv))
    return rv;

  nsComponentManagerImpl::ComponentLocation elem;
  elem.type = NS_BOOTSTRAPPED_LOCATION;

  if (Substring(path, path.Length() - 4).Equals(NS_LITERAL_STRING(".xpi"))) {
    elem.location.Init(aLocation, "chrome.manifest");
  } else {
    nsCOMPtr<nsIFile> lf = CloneAndAppend(aLocation, NS_LITERAL_CSTRING("chrome.manifest"));
    elem.location.Init(lf);
  }

  
  nsComponentManagerImpl::sModuleLocations->RemoveElement(elem, ComponentLocationComparator());

  rv = cr->CheckForNewChrome();
  return rv;
}

EXPORT_XPCOM_API(nsresult)
XRE_AddManifestLocation(NSLocationType aType, nsIFile* aLocation)
{
    nsComponentManagerImpl::InitializeModuleLocations();
    nsComponentManagerImpl::ComponentLocation* c = 
        nsComponentManagerImpl::sModuleLocations->AppendElement();
    c->type = aType;
    c->location.Init(aLocation);

    if (nsComponentManagerImpl::gComponentManager &&
        nsComponentManagerImpl::NORMAL == nsComponentManagerImpl::gComponentManager->mStatus)
        nsComponentManagerImpl::gComponentManager->RegisterManifest(aType, c->location, false);

    return NS_OK;
}

EXPORT_XPCOM_API(nsresult)
XRE_AddJarManifestLocation(NSLocationType aType, nsIFile* aLocation)
{
    nsComponentManagerImpl::InitializeModuleLocations();
    nsComponentManagerImpl::ComponentLocation* c = 
        nsComponentManagerImpl::sModuleLocations->AppendElement();

    c->type = aType;
    c->location.Init(aLocation, "chrome.manifest");

    if (nsComponentManagerImpl::gComponentManager &&
        nsComponentManagerImpl::NORMAL == nsComponentManagerImpl::gComponentManager->mStatus)
        nsComponentManagerImpl::gComponentManager->RegisterManifest(aType, c->location, false);

    return NS_OK;
}

