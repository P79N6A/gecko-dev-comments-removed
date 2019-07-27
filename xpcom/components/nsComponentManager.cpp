

















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
#include "xptiprivate.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/XPTInterfaceInfoManager.h"
#include "nsIConsoleService.h"
#include "nsIObserverService.h"
#include "nsISimpleEnumerator.h"
#include "nsIStringEnumerator.h"
#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"
#include "nsISupportsPrimitives.h"
#include "nsIClassInfo.h"
#include "nsLocalFile.h"
#include "nsReadableUtils.h"
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

#include "mozilla/GenericFactory.h"
#include "nsSupportsPrimitives.h"
#include "nsArray.h"
#include "nsIMutableArray.h"
#include "nsArrayEnumerator.h"
#include "nsStringEnumerator.h"
#include "mozilla/FileUtils.h"
#include "nsNetUtil.h"
#include "nsDataHashtable.h"

#include <new>     

#include "mozilla/Omnijar.h"

#include "prlog.h"

using namespace mozilla;

PRLogModuleInfo* nsComponentManagerLog = nullptr;

#if 0 || defined (DEBUG_timeless)
 #define SHOW_DENIED_ON_SHUTDOWN
 #define SHOW_CI_ON_EXISTING_SERVICE
#endif




#define BIG_REGISTRY_BUFLEN   (512*1024)


const char xpcomComponentsKeyName[] = "software/mozilla/XPCOM/components";
const char xpcomKeyName[] = "software/mozilla/XPCOM";


const char fileSizeValueName[] = "FileSize";
const char lastModValueName[] = "LastModTimeStamp";
const char nativeComponentType[] = "application/x-mozilla-native";
const char staticComponentType[] = "application/x-mozilla-static";

NS_DEFINE_CID(kCategoryManagerCID, NS_CATEGORYMANAGER_CID);

#define UID_STRING_LENGTH 39

#ifdef MOZ_B2G_LOADER
typedef nsDataHashtable<nsCStringHashKey, bool> XPTIInfosBookType;
static XPTIInfosBookType* sXPTIInfosBook = nullptr;

static XPTIInfosBookType*
GetXPTIInfosBook()
{
  if (!sXPTIInfosBook) {
    sXPTIInfosBook = new XPTIInfosBookType;
  }
  return sXPTIInfosBook;
}

static bool
IsRegisteredXPTIInfo(FileLocation& aFile)
{
  nsAutoCString uri;
  aFile.GetURIString(uri);
  return GetXPTIInfosBook()->Get(uri);
}

static void
MarkRegisteredXPTIInfo(FileLocation& aFile)
{
  nsAutoCString uri;
  aFile.GetURIString(uri);
  GetXPTIInfosBook()->Put(uri, true);
}
#endif 

nsresult
nsGetServiceFromCategory::operator()(const nsIID& aIID,
                                     void** aInstancePtr) const
{
  nsresult rv;
  nsXPIDLCString value;
  nsCOMPtr<nsICategoryManager> catman;
  nsComponentManagerImpl* compMgr = nsComponentManagerImpl::gComponentManager;
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
  if (NS_FAILED(rv)) {
    goto error;
  }

  
  rv = catman->GetCategoryEntry(mCategory, mEntry,
                                getter_Copies(value));
  if (NS_FAILED(rv)) {
    goto error;
  }
  if (!value) {
    rv = NS_ERROR_SERVICE_NOT_AVAILABLE;
    goto error;
  }

  rv = compMgr->nsComponentManagerImpl::GetServiceByContractID(value,
                                                               aIID,
                                                               aInstancePtr);
  if (NS_FAILED(rv)) {
error:
    *aInstancePtr = 0;
  }
  if (mErrorPtr) {
    *mErrorPtr = rv;
  }
  return rv;
}




char*
ArenaStrndup(const char* aStr, uint32_t aLen, PLArenaPool* aArena)
{
  void* mem;
  
  PL_ARENA_ALLOCATE(mem, aArena, aLen + 1);
  if (mem) {
    memcpy(mem, aStr, aLen + 1);
  }
  return static_cast<char*>(mem);
}

char*
ArenaStrdup(const char* aStr, PLArenaPool* aArena)
{
  return ArenaStrndup(aStr, strlen(aStr), aArena);
}





namespace {

class MOZ_STACK_CLASS MutexLock
{
public:
  explicit MutexLock(SafeMutex& aMutex)
    : mMutex(aMutex)
    , mLocked(false)
  {
    Lock();
  }

  ~MutexLock()
  {
    if (mLocked) {
      Unlock();
    }
  }

  void Lock()
  {
    NS_ASSERTION(!mLocked, "Re-entering a mutex");
    mMutex.Lock();
    mLocked = true;
  }

  void Unlock()
  {
    NS_ASSERTION(mLocked, "Exiting a mutex that isn't held!");
    mMutex.Unlock();
    mLocked = false;
  }

private:
  SafeMutex& mMutex;
  bool mLocked;
};

} 


static already_AddRefed<nsIFile>
GetLocationFromDirectoryService(const char* aProp)
{
  nsCOMPtr<nsIProperties> directoryService;
  nsDirectoryService::Create(nullptr,
                             NS_GET_IID(nsIProperties),
                             getter_AddRefs(directoryService));

  if (!directoryService) {
    return nullptr;
  }

  nsCOMPtr<nsIFile> file;
  nsresult rv = directoryService->Get(aProp,
                                      NS_GET_IID(nsIFile),
                                      getter_AddRefs(file));
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  return file.forget();
}

static already_AddRefed<nsIFile>
CloneAndAppend(nsIFile* aBase, const nsACString& aAppend)
{
  nsCOMPtr<nsIFile> f;
  aBase->Clone(getter_AddRefs(f));
  if (!f) {
    return nullptr;
  }

  f->AppendNative(aAppend);
  return f.forget();
}





nsresult
nsComponentManagerImpl::Create(nsISupports* aOuter, REFNSIID aIID,
                               void** aResult)
{
  if (aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }

  if (!gComponentManager) {
    return NS_ERROR_FAILURE;
  }

  return gComponentManager->QueryInterface(aIID, aResult);
}

static const int CONTRACTID_HASHTABLE_INITIAL_LENGTH = 1024;

nsComponentManagerImpl::nsComponentManagerImpl()
  : mFactories(CONTRACTID_HASHTABLE_INITIAL_LENGTH)
  , mContractIDs(CONTRACTID_HASHTABLE_INITIAL_LENGTH)
  , mLock("nsComponentManagerImpl.mLock")
  , mStatus(NOT_INITIALIZED)
{
}

nsTArray<const mozilla::Module*>* nsComponentManagerImpl::sStaticModules;

NSMODULE_DEFN(start_kPStaticModules);
NSMODULE_DEFN(end_kPStaticModules);






MOZ_ASAN_BLACKLIST
 void
nsComponentManagerImpl::InitializeStaticModules()
{
  if (sStaticModules) {
    return;
  }

  sStaticModules = new nsTArray<const mozilla::Module*>;
  for (const mozilla::Module * const* staticModules =
         &NSMODULE_NAME(start_kPStaticModules) + 1;
       staticModules < &NSMODULE_NAME(end_kPStaticModules); ++staticModules)
    if (*staticModules) { 
      sStaticModules->AppendElement(*staticModules);
    }
}

nsTArray<nsComponentManagerImpl::ComponentLocation>*
nsComponentManagerImpl::sModuleLocations;

 void
nsComponentManagerImpl::InitializeModuleLocations()
{
  if (sModuleLocations) {
    return;
  }

  sModuleLocations = new nsTArray<ComponentLocation>;
}

nsresult
nsComponentManagerImpl::Init()
{
  PR_ASSERT(NOT_INITIALIZED == mStatus);

  if (!nsComponentManagerLog) {
    nsComponentManagerLog = PR_NewLogModule("nsComponentManager");
  }

  
  PL_INIT_ARENA_POOL(&mArena, "ComponentManagerArena", NS_CM_BLOCK_SIZE);

  nsCOMPtr<nsIFile> greDir =
    GetLocationFromDirectoryService(NS_GRE_DIR);
  nsCOMPtr<nsIFile> appDir =
    GetLocationFromDirectoryService(NS_XPCOM_CURRENT_PROCESS_DIR);

  InitializeStaticModules();

  nsresult rv = mNativeModuleLoader.Init();
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCategoryManager::GetSingleton()->SuppressNotifications(true);

  RegisterModule(&kXPCOMModule, nullptr);

  for (uint32_t i = 0; i < sStaticModules->Length(); ++i) {
    RegisterModule((*sStaticModules)[i], nullptr);
  }

  
  
  
  
  
  

  InitializeModuleLocations();
  ComponentLocation* cl = sModuleLocations->AppendElement();
  nsCOMPtr<nsIFile> lf = CloneAndAppend(greDir,
                                        NS_LITERAL_CSTRING("chrome.manifest"));
  cl->type = NS_COMPONENT_LOCATION;
  cl->location.Init(lf);

  nsRefPtr<nsZipArchive> greOmnijar =
    mozilla::Omnijar::GetReader(mozilla::Omnijar::GRE);
  if (greOmnijar) {
    cl = sModuleLocations->AppendElement();
    cl->type = NS_COMPONENT_LOCATION;
    cl->location.Init(greOmnijar, "chrome.manifest");
  }

  bool equals = false;
  appDir->Equals(greDir, &equals);
  if (!equals) {
    cl = sModuleLocations->AppendElement();
    cl->type = NS_COMPONENT_LOCATION;
    lf = CloneAndAppend(appDir, NS_LITERAL_CSTRING("chrome.manifest"));
    cl->location.Init(lf);
  }

  nsRefPtr<nsZipArchive> appOmnijar =
    mozilla::Omnijar::GetReader(mozilla::Omnijar::APP);
  if (appOmnijar) {
    cl = sModuleLocations->AppendElement();
    cl->type = NS_COMPONENT_LOCATION;
    cl->location.Init(appOmnijar, "chrome.manifest");
  }

  RereadChromeManifests(false);

  nsCategoryManager::GetSingleton()->SuppressNotifications(false);

  RegisterWeakMemoryReporter(this);

  
  
  
  
  nsCategoryManager::GetSingleton()->InitMemoryReporter();

  PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG,
         ("nsComponentManager: Initialized."));

  mStatus = NORMAL;

  return NS_OK;
}

void
nsComponentManagerImpl::RegisterModule(const mozilla::Module* aModule,
                                       FileLocation* aFile)
{
  mLock.AssertNotCurrentThreadOwns();

  {
    
    
    MutexLock lock(mLock);

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
      for (entry = aModule->mCIDs; entry->cid; ++entry) {
        RegisterCIDEntryLocked(entry, m);
      }
    }

    if (aModule->mContractIDs) {
      const mozilla::Module::ContractIDEntry* entry;
      for (entry = aModule->mContractIDs; entry->contractid; ++entry) {
        RegisterContractIDLocked(entry);
      }
      MOZ_ASSERT(!entry->cid, "Incorrectly terminated contract list");
    }
  }

  if (aModule->mCategoryEntries) {
    const mozilla::Module::CategoryEntry* entry;
    for (entry = aModule->mCategoryEntries; entry->category; ++entry)
      nsCategoryManager::GetSingleton()->AddCategoryEntry(entry->category,
                                                          entry->entry,
                                                          entry->value);
  }
}

static bool
ProcessSelectorMatches(Module::ProcessSelector aSelector)
{
  if (aSelector == Module::ANY_PROCESS) {
    return true;
  }

  GeckoProcessType type = XRE_GetProcessType();
  switch (aSelector) {
    case Module::MAIN_PROCESS_ONLY:
      return type == GeckoProcessType_Default;
    case Module::CONTENT_PROCESS_ONLY:
      return type == GeckoProcessType_Content;
    default:
      MOZ_CRASH("invalid process aSelector");
  }
}

void
nsComponentManagerImpl::RegisterCIDEntryLocked(
    const mozilla::Module::CIDEntry* aEntry,
    KnownModule* aModule)
{
  mLock.AssertCurrentThreadOwns();

  if (!ProcessSelectorMatches(aEntry->processSelector)) {
    return;
  }

  nsFactoryEntry* f = mFactories.Get(*aEntry->cid);
  if (f) {
    NS_WARNING("Re-registering a CID?");

    char idstr[NSID_LENGTH];
    aEntry->cid->ToProvidedString(idstr);

    nsCString existing;
    if (f->mModule) {
      existing = f->mModule->Description();
    } else {
      existing = "<unknown module>";
    }
    SafeMutexAutoUnlock unlock(mLock);
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
nsComponentManagerImpl::RegisterContractIDLocked(
    const mozilla::Module::ContractIDEntry* aEntry)
{
  mLock.AssertCurrentThreadOwns();

  if (!ProcessSelectorMatches(aEntry->processSelector)) {
    return;
  }

  nsFactoryEntry* f = mFactories.Get(*aEntry->cid);
  if (!f) {
    NS_WARNING("No CID found when attempting to map contract ID");

    char idstr[NSID_LENGTH];
    aEntry->cid->ToProvidedString(idstr);

    SafeMutexAutoUnlock unlock(mLock);
    LogMessage("Could not map contract ID '%s' to CID %s because no implementation of the CID is registered.",
               aEntry->contractid,
               idstr);

    return;
  }

  mContractIDs.Put(nsDependentCString(aEntry->contractid), f);
}

static void
CutExtension(nsCString& aPath)
{
  int32_t dotPos = aPath.RFindChar('.');
  if (kNotFound == dotPos) {
    aPath.Truncate();
  } else {
    aPath.Cut(0, dotPos + 1);
  }
}

static void
DoRegisterManifest(NSLocationType aType,
                   FileLocation& aFile,
                   bool aChromeOnly,
                   bool aXPTOnly)
{
  MOZ_ASSERT(!aXPTOnly || !nsComponentManagerImpl::gComponentManager);
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
    ParseManifest(aType, aFile, buf, aChromeOnly, aXPTOnly);
  } else if (NS_BOOTSTRAPPED_LOCATION != aType) {
    nsCString uri;
    aFile.GetURIString(uri);
    LogMessage("Could not read chrome manifest '%s'.", uri.get());
  }
}

void
nsComponentManagerImpl::RegisterManifest(NSLocationType aType,
                                         FileLocation& aFile,
                                         bool aChromeOnly)
{
  DoRegisterManifest(aType, aFile, aChromeOnly, false);
}

void
nsComponentManagerImpl::ManifestManifest(ManifestProcessingContext& aCx,
                                         int aLineNo, char* const* aArgv)
{
  char* file = aArgv[0];
  FileLocation f(aCx.mFile, file);
  RegisterManifest(aCx.mType, f, aCx.mChromeOnly);
}

void
nsComponentManagerImpl::ManifestBinaryComponent(ManifestProcessingContext& aCx,
                                                int aLineNo,
                                                char* const* aArgv)
{
  if (aCx.mFile.IsZip()) {
    NS_WARNING("Cannot load binary components from a jar.");
    LogMessageWithContext(aCx.mFile, aLineNo,
                          "Cannot load binary components from a jar.");
    return;
  }

  FileLocation f(aCx.mFile, aArgv[0]);
  nsCString uri;
  f.GetURIString(uri);

  if (mKnownModules.Get(uri)) {
    NS_WARNING("Attempting to register a binary component twice.");
    LogMessageWithContext(aCx.mFile, aLineNo,
                          "Attempting to register a binary component twice.");
    return;
  }

  const mozilla::Module* m = mNativeModuleLoader.LoadModule(f);
  
  if (!m) {
    return;
  }

  RegisterModule(m, &f);
}

static void
DoRegisterXPT(FileLocation& aFile)
{
#ifdef MOZ_B2G_LOADER
  if (IsRegisteredXPTIInfo(aFile)) {
    return;
  }
#endif

  uint32_t len;
  FileLocation::Data data;
  nsAutoArrayPtr<char> buf;
  nsresult rv = aFile.GetData(data);
  if (NS_SUCCEEDED(rv)) {
    rv = data.GetSize(&len);
  }
  if (NS_SUCCEEDED(rv)) {
    buf = new char[len];
    rv = data.Copy(buf, len);
  }
  if (NS_SUCCEEDED(rv)) {
    XPTInterfaceInfoManager::GetSingleton()->RegisterBuffer(buf, len);
#ifdef MOZ_B2G_LOADER
    MarkRegisteredXPTIInfo(aFile);
#endif
  } else {
    nsCString uri;
    aFile.GetURIString(uri);
    LogMessage("Could not read '%s'.", uri.get());
  }
}

void
nsComponentManagerImpl::ManifestXPT(ManifestProcessingContext& aCx,
                                    int aLineNo, char* const* aArgv)
{
  FileLocation f(aCx.mFile, aArgv[0]);
  DoRegisterXPT(f);
}

void
nsComponentManagerImpl::ManifestComponent(ManifestProcessingContext& aCx,
                                          int aLineNo, char* const* aArgv)
{
  mLock.AssertNotCurrentThreadOwns();

  char* id = aArgv[0];
  char* file = aArgv[1];

  nsID cid;
  if (!cid.Parse(id)) {
    LogMessageWithContext(aCx.mFile, aLineNo,
                          "Malformed CID: '%s'.", id);
    return;
  }

  
  FileLocation fl(aCx.mFile, file);
  nsCString hash;
  fl.GetURIString(hash);

  MutexLock lock(mLock);
  nsFactoryEntry* f = mFactories.Get(cid);
  if (f) {
    char idstr[NSID_LENGTH];
    cid.ToProvidedString(idstr);

    nsCString existing;
    if (f->mModule) {
      existing = f->mModule->Description();
    } else {
      existing = "<unknown module>";
    }

    lock.Unlock();

    LogMessageWithContext(aCx.mFile, aLineNo,
                          "Trying to re-register CID '%s' already registered by %s.",
                          idstr,
                          existing.get());
    return;
  }

  KnownModule* km;

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
nsComponentManagerImpl::ManifestContract(ManifestProcessingContext& aCx,
                                         int aLineNo, char* const* aArgv)
{
  mLock.AssertNotCurrentThreadOwns();

  char* contract = aArgv[0];
  char* id = aArgv[1];

  nsID cid;
  if (!cid.Parse(id)) {
    LogMessageWithContext(aCx.mFile, aLineNo,
                          "Malformed CID: '%s'.", id);
    return;
  }

  MutexLock lock(mLock);
  nsFactoryEntry* f = mFactories.Get(cid);
  if (!f) {
    lock.Unlock();
    LogMessageWithContext(aCx.mFile, aLineNo,
                          "Could not map contract ID '%s' to CID %s because no implementation of the CID is registered.",
                          contract, id);
    return;
  }

  mContractIDs.Put(nsDependentCString(contract), f);
}

void
nsComponentManagerImpl::ManifestCategory(ManifestProcessingContext& aCx,
                                         int aLineNo, char* const* aArgv)
{
  char* category = aArgv[0];
  char* key = aArgv[1];
  char* value = aArgv[2];

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
    mLoader =
      nsComponentManagerImpl::gComponentManager->LoaderForExtension(extension);
  }
  return !!mLoader;
}

bool
nsComponentManagerImpl::KnownModule::Load()
{
  if (mFailed) {
    return false;
  }
  if (!mModule) {
    if (!EnsureLoader()) {
      return false;
    }

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
  if (mFile) {
    mFile.GetURIString(s);
  } else {
    s = "<static module>";
  }
  return s;
}

nsresult nsComponentManagerImpl::Shutdown(void)
{
  PR_ASSERT(NORMAL == mStatus);

  mStatus = SHUTDOWN_IN_PROGRESS;

  
  PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG,
         ("nsComponentManager: Beginning Shutdown."));

  UnregisterWeakMemoryReporter(this);

  
  mContractIDs.Clear();
  mFactories.Clear(); 
  mLoaderMap.Clear();
  mKnownModules.Clear();
  mKnownStaticModules.Clear();

  delete sStaticModules;
  delete sModuleLocations;
#ifdef MOZ_B2G_LOADER
  delete sXPTIInfosBook;
  sXPTIInfosBook = nullptr;
#endif

  
  mNativeModuleLoader.UnloadLibraries();

  
  PL_FinishArenaPool(&mArena);

  mStatus = SHUTDOWN_COMPLETE;

  PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG,
         ("nsComponentManager: Shutdown complete."));

  return NS_OK;
}

nsComponentManagerImpl::~nsComponentManagerImpl()
{
  PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG,
         ("nsComponentManager: Beginning destruction."));

  if (SHUTDOWN_COMPLETE != mStatus) {
    Shutdown();
  }

  PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG,
         ("nsComponentManager: Destroyed."));
}

NS_IMPL_ISUPPORTS(nsComponentManagerImpl,
                  nsIComponentManager,
                  nsIServiceManager,
                  nsIComponentRegistrar,
                  nsISupportsWeakReference,
                  nsIInterfaceRequestor,
                  nsIMemoryReporter)

nsresult
nsComponentManagerImpl::GetInterface(const nsIID& aUuid, void** aResult)
{
  NS_WARNING("This isn't supported");
  
  
  return  QueryInterface(aUuid, aResult);
}

nsFactoryEntry*
nsComponentManagerImpl::GetFactoryEntry(const char* aContractID,
                                        uint32_t aContractIDLen)
{
  SafeMutexAutoLock lock(mLock);
  return mContractIDs.Get(nsDependentCString(aContractID, aContractIDLen));
}


nsFactoryEntry*
nsComponentManagerImpl::GetFactoryEntry(const nsCID& aClass)
{
  SafeMutexAutoLock lock(mLock);
  return mFactories.Get(aClass);
}

already_AddRefed<nsIFactory>
nsComponentManagerImpl::FindFactory(const nsCID& aClass)
{
  nsFactoryEntry* e = GetFactoryEntry(aClass);
  if (!e) {
    return nullptr;
  }

  return e->GetFactory();
}

already_AddRefed<nsIFactory>
nsComponentManagerImpl::FindFactory(const char* aContractID,
                                    uint32_t aContractIDLen)
{
  nsFactoryEntry* entry = GetFactoryEntry(aContractID, aContractIDLen);
  if (!entry) {
    return nullptr;
  }

  return entry->GetFactory();
}







NS_IMETHODIMP
nsComponentManagerImpl::GetClassObject(const nsCID& aClass, const nsIID& aIID,
                                       void** aResult)
{
  nsresult rv;

#ifdef PR_LOGGING
  if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_DEBUG)) {
    char* buf = aClass.ToString();
    PR_LogPrint("nsComponentManager: GetClassObject(%s)", buf);
    if (buf) {
      NS_Free(buf);
    }
  }
#endif

  PR_ASSERT(aResult != nullptr);

  nsCOMPtr<nsIFactory> factory = FindFactory(aClass);
  if (!factory) {
    return NS_ERROR_FACTORY_NOT_REGISTERED;
  }

  rv = factory->QueryInterface(aIID, aResult);

  PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
         ("\t\tGetClassObject() %s", NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));

  return rv;
}


NS_IMETHODIMP
nsComponentManagerImpl::GetClassObjectByContractID(const char* aContractID,
                                                   const nsIID& aIID,
                                                   void** aResult)
{
  if (NS_WARN_IF(!aResult) ||
      NS_WARN_IF(!aContractID)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;


#ifdef PR_LOGGING
  if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_DEBUG)) {
    PR_LogPrint("nsComponentManager: GetClassObject(%s)", aContractID);
  }
#endif

  nsCOMPtr<nsIFactory> factory = FindFactory(aContractID, strlen(aContractID));
  if (!factory) {
    return NS_ERROR_FACTORY_NOT_REGISTERED;
  }

  rv = factory->QueryInterface(aIID, aResult);

  PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
         ("\t\tGetClassObject() %s", NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));

  return rv;
}








NS_IMETHODIMP
nsComponentManagerImpl::CreateInstance(const nsCID& aClass,
                                       nsISupports* aDelegate,
                                       const nsIID& aIID,
                                       void** aResult)
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

  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = nullptr;

  nsFactoryEntry* entry = GetFactoryEntry(aClass);

  if (!entry) {
    return NS_ERROR_FACTORY_NOT_REGISTERED;
  }

#ifdef SHOW_CI_ON_EXISTING_SERVICE
  if (entry->mServiceObject) {
    nsXPIDLCString cid;
    cid.Adopt(aClass.ToString());
    nsAutoCString message;
    message = NS_LITERAL_CSTRING("You are calling CreateInstance \"") +
              cid +
              NS_LITERAL_CSTRING("\" when a service for this CID already exists!");
    NS_ERROR(message.get());
  }
#endif

  nsresult rv;
  nsCOMPtr<nsIFactory> factory = entry->GetFactory();
  if (factory) {
    rv = factory->CreateInstance(aDelegate, aIID, aResult);
    if (NS_SUCCEEDED(rv) && !*aResult) {
      NS_ERROR("Factory did not return an object but returned success!");
      rv = NS_ERROR_SERVICE_NOT_FOUND;
    }
  } else {
    
    rv = NS_ERROR_FACTORY_NOT_REGISTERED;
  }

#ifdef PR_LOGGING
  if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_WARNING)) {
    char* buf = aClass.ToString();
    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("nsComponentManager: CreateInstance(%s) %s", buf,
            NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));
    if (buf) {
      NS_Free(buf);
    }
  }
#endif

  return rv;
}










NS_IMETHODIMP
nsComponentManagerImpl::CreateInstanceByContractID(const char* aContractID,
                                                   nsISupports* aDelegate,
                                                   const nsIID& aIID,
                                                   void** aResult)
{
  if (NS_WARN_IF(!aContractID)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  
  if (gXPCOMShuttingDown) {
    
#ifdef SHOW_DENIED_ON_SHUTDOWN
    nsXPIDLCString iid;
    iid.Adopt(aIID.ToString());
    fprintf(stderr, "Creating new instance on shutdown. Denied.\n"
            "  ContractID: %s\n         IID: %s\n", aContractID, iid.get());
#endif 
    return NS_ERROR_UNEXPECTED;
  }

  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = nullptr;

  nsFactoryEntry* entry = GetFactoryEntry(aContractID, strlen(aContractID));

  if (!entry) {
    return NS_ERROR_FACTORY_NOT_REGISTERED;
  }

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
  if (factory) {

    rv = factory->CreateInstance(aDelegate, aIID, aResult);
    if (NS_SUCCEEDED(rv) && !*aResult) {
      NS_ERROR("Factory did not return an object but returned success!");
      rv = NS_ERROR_SERVICE_NOT_FOUND;
    }
  } else {
    
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
                   void* aArg)
{
  aEntry->mFactory = nullptr;
  aEntry->mServiceObject = nullptr;
  return PL_DHASH_NEXT;
}

nsresult
nsComponentManagerImpl::FreeServices()
{
  NS_ASSERTION(gXPCOMShuttingDown,
               "Must be shutting down in order to free all services");

  if (!gXPCOMShuttingDown) {
    return NS_ERROR_FAILURE;
  }

  mFactories.EnumerateRead(FreeFactoryEntries, nullptr);
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

NS_IMETHODIMP
nsComponentManagerImpl::GetService(const nsCID& aClass,
                                   const nsIID& aIID,
                                   void** aResult)
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

  
  
  nsCOMPtr<nsISupports> service;
  MutexLock lock(mLock);

  nsFactoryEntry* entry = mFactories.Get(aClass);
  if (!entry) {
    return NS_ERROR_FACTORY_NOT_REGISTERED;
  }

  if (entry->mServiceObject) {
    lock.Unlock();
    return entry->mServiceObject->QueryInterface(aIID, aResult);
  }

  PRThread* currentPRThread = PR_GetCurrentThread();
  MOZ_ASSERT(currentPRThread, "This should never be null!");

  
  nsIThread* currentThread = nullptr;

  PRThread* pendingPRThread;
  while ((pendingPRThread = GetPendingServiceThread(aClass))) {
    if (pendingPRThread == currentPRThread) {
      NS_ERROR("Recursive GetService!");
      return NS_ERROR_NOT_AVAILABLE;
    }


    SafeMutexAutoUnlock unlockPending(mLock);

    if (!currentThread) {
      currentThread = NS_GetCurrentThread();
      MOZ_ASSERT(currentThread, "This should never be null!");
    }

    
    
    if (!NS_ProcessNextEvent(currentThread, false)) {
      PR_Sleep(PR_INTERVAL_NO_WAIT);
    }
  }

  
  
  if (entry->mServiceObject) {
    lock.Unlock();
    return entry->mServiceObject->QueryInterface(aIID, aResult);
  }

#ifdef DEBUG
  PendingServiceInfo* newInfo =
#endif
    AddPendingService(aClass, currentPRThread);
  NS_ASSERTION(newInfo, "Failed to add info to the array!");

  
  
  

  nsresult rv;
  {
    SafeMutexAutoUnlock unlock(mLock);
    rv = CreateInstance(aClass, nullptr, aIID, getter_AddRefs(service));
  }
  if (NS_SUCCEEDED(rv) && !service) {
    NS_ERROR("Factory did not return an object but returned success");
    return NS_ERROR_SERVICE_NOT_FOUND;
  }

#ifdef DEBUG
  pendingPRThread = GetPendingServiceThread(aClass);
  MOZ_ASSERT(pendingPRThread == currentPRThread,
             "Pending service array has been changed!");
#endif
  RemovePendingService(aClass);

  if (NS_FAILED(rv)) {
    return rv;
  }

  NS_ASSERTION(!entry->mServiceObject, "Created two instances of a service!");

  entry->mServiceObject = service.forget();

  lock.Unlock();
  nsISupports** sresult = reinterpret_cast<nsISupports**>(aResult);
  *sresult = entry->mServiceObject;
  (*sresult)->AddRef();

  return NS_OK;
}

NS_IMETHODIMP
nsComponentManagerImpl::IsServiceInstantiated(const nsCID& aClass,
                                              const nsIID& aIID,
                                              bool* aResult)
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
    SafeMutexAutoLock lock(mLock);
    entry = mFactories.Get(aClass);
  }

  if (entry && entry->mServiceObject) {
    nsCOMPtr<nsISupports> service;
    rv = entry->mServiceObject->QueryInterface(aIID, getter_AddRefs(service));
    *aResult = (service != nullptr);
  }

  return rv;
}

NS_IMETHODIMP
nsComponentManagerImpl::IsServiceInstantiatedByContractID(
    const char* aContractID,
    const nsIID& aIID,
    bool* aResult)
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
  nsFactoryEntry* entry;
  {
    SafeMutexAutoLock lock(mLock);
    entry = mContractIDs.Get(nsDependentCString(aContractID));
  }

  if (entry && entry->mServiceObject) {
    nsCOMPtr<nsISupports> service;
    rv = entry->mServiceObject->QueryInterface(aIID, getter_AddRefs(service));
    *aResult = (service != nullptr);
  }
  return rv;
}


NS_IMETHODIMP
nsComponentManagerImpl::GetServiceByContractID(const char* aContractID,
                                               const nsIID& aIID,
                                               void** aResult)
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

  
  
  nsCOMPtr<nsISupports> service;
  MutexLock lock(mLock);

  nsFactoryEntry* entry = mContractIDs.Get(nsDependentCString(aContractID));
  if (!entry) {
    return NS_ERROR_FACTORY_NOT_REGISTERED;
  }

  if (entry->mServiceObject) {
    
    
    
    
    
    
    lock.Unlock();
    return entry->mServiceObject->QueryInterface(aIID, aResult);
  }

  PRThread* currentPRThread = PR_GetCurrentThread();
  MOZ_ASSERT(currentPRThread, "This should never be null!");

  
  nsIThread* currentThread = nullptr;

  PRThread* pendingPRThread;
  while ((pendingPRThread = GetPendingServiceThread(*entry->mCIDEntry->cid))) {
    if (pendingPRThread == currentPRThread) {
      NS_ERROR("Recursive GetService!");
      return NS_ERROR_NOT_AVAILABLE;
    }

    SafeMutexAutoUnlock unlockPending(mLock);

    if (!currentThread) {
      currentThread = NS_GetCurrentThread();
      MOZ_ASSERT(currentThread, "This should never be null!");
    }

    
    
    if (!NS_ProcessNextEvent(currentThread, false)) {
      PR_Sleep(PR_INTERVAL_NO_WAIT);
    }
  }

  if (currentThread && entry->mServiceObject) {
    
    
    lock.Unlock();
    return entry->mServiceObject->QueryInterface(aIID, aResult);
  }

#ifdef DEBUG
  PendingServiceInfo* newInfo =
#endif
    AddPendingService(*entry->mCIDEntry->cid, currentPRThread);
  NS_ASSERTION(newInfo, "Failed to add info to the array!");

  
  
  

  nsresult rv;
  {
    SafeMutexAutoUnlock unlock(mLock);
    rv = CreateInstanceByContractID(aContractID, nullptr, aIID,
                                    getter_AddRefs(service));
  }
  if (NS_SUCCEEDED(rv) && !service) {
    NS_ERROR("Factory did not return an object but returned success");
    return NS_ERROR_SERVICE_NOT_FOUND;
  }

#ifdef DEBUG
  pendingPRThread = GetPendingServiceThread(*entry->mCIDEntry->cid);
  MOZ_ASSERT(pendingPRThread == currentPRThread,
             "Pending service array has been changed!");
#endif
  RemovePendingService(*entry->mCIDEntry->cid);

  if (NS_FAILED(rv)) {
    return rv;
  }

  NS_ASSERTION(!entry->mServiceObject, "Created two instances of a service!");

  entry->mServiceObject = service.forget();

  lock.Unlock();

  nsISupports** sresult = reinterpret_cast<nsISupports**>(aResult);
  *sresult = entry->mServiceObject;
  (*sresult)->AddRef();

  return NS_OK;
}

already_AddRefed<mozilla::ModuleLoader>
nsComponentManagerImpl::LoaderForExtension(const nsACString& aExt)
{
  nsCOMPtr<mozilla::ModuleLoader> loader = mLoaderMap.Get(aExt);
  if (!loader) {
    loader = do_GetServiceFromCategory("module-loader",
                                       PromiseFlatCString(aExt).get());
    if (!loader) {
      return nullptr;
    }

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
    
    
    if (!aContractID) {
      return NS_ERROR_INVALID_ARG;
    }

    SafeMutexAutoLock lock(mLock);
    nsFactoryEntry* oldf = mFactories.Get(aClass);
    if (!oldf) {
      return NS_ERROR_FACTORY_NOT_REGISTERED;
    }

    mContractIDs.Put(nsDependentCString(aContractID), oldf);
    return NS_OK;
  }

  nsAutoPtr<nsFactoryEntry> f(new nsFactoryEntry(aClass, aFactory));

  SafeMutexAutoLock lock(mLock);
  nsFactoryEntry* oldf = mFactories.Get(aClass);
  if (oldf) {
    return NS_ERROR_FACTORY_EXISTS;
  }

  if (aContractID) {
    mContractIDs.Put(nsDependentCString(aContractID), f);
  }

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
    SafeMutexAutoLock lock(mLock);
    nsFactoryEntry* f = mFactories.Get(aClass);
    if (!f || f->mFactory != aFactory) {
      return NS_ERROR_FACTORY_NOT_REGISTERED;
    }

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
nsComponentManagerImpl::IsCIDRegistered(const nsCID& aClass,
                                        bool* aResult)
{
  *aResult = (nullptr != GetFactoryEntry(aClass));
  return NS_OK;
}

NS_IMETHODIMP
nsComponentManagerImpl::IsContractIDRegistered(const char* aClass,
                                               bool* aResult)
{
  if (NS_WARN_IF(!aClass)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsFactoryEntry* entry = GetFactoryEntry(aClass, strlen(aClass));

  if (entry) {
    *aResult = true;
  } else {
    *aResult = false;
  }
  return NS_OK;
}

static PLDHashOperator
EnumerateCIDHelper(const nsID& aId, nsFactoryEntry* aEntry, void* aClosure)
{
  nsCOMArray<nsISupports>* array =
    static_cast<nsCOMArray<nsISupports>*>(aClosure);
  nsCOMPtr<nsISupportsID> wrapper = new nsSupportsIDImpl();
  wrapper->SetData(&aId);
  array->AppendObject(wrapper);
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsComponentManagerImpl::EnumerateCIDs(nsISimpleEnumerator** aEnumerator)
{
  nsCOMArray<nsISupports> array;
  mFactories.EnumerateRead(EnumerateCIDHelper, &array);

  return NS_NewArrayEnumerator(aEnumerator, array);
}

static PLDHashOperator
EnumerateContractsHelper(const nsACString& aContract, nsFactoryEntry* aEntry,
                         void* aClosure)
{
  nsTArray<nsCString>* array = static_cast<nsTArray<nsCString>*>(aClosure);
  array->AppendElement(aContract);
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsComponentManagerImpl::EnumerateContractIDs(nsISimpleEnumerator** aEnumerator)
{
  nsTArray<nsCString>* array = new nsTArray<nsCString>;
  mContractIDs.EnumerateRead(EnumerateContractsHelper, array);

  nsCOMPtr<nsIUTF8StringEnumerator> e;
  nsresult rv = NS_NewAdoptingUTF8StringEnumerator(getter_AddRefs(e), array);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return CallQueryInterface(e, aEnumerator);
}

NS_IMETHODIMP
nsComponentManagerImpl::CIDToContractID(const nsCID& aClass,
                                        char** aResult)
{
  NS_ERROR("CIDTOContractID not implemented");
  return NS_ERROR_FACTORY_NOT_REGISTERED;
}

NS_IMETHODIMP
nsComponentManagerImpl::ContractIDToCID(const char* aContractID,
                                        nsCID** aResult)
{
  {
    SafeMutexAutoLock lock(mLock);
    nsFactoryEntry* entry = mContractIDs.Get(nsDependentCString(aContractID));
    if (entry) {
      *aResult = (nsCID*)NS_Alloc(sizeof(nsCID));
      **aResult = *entry->mCIDEntry->cid;
      return NS_OK;
    }
  }
  *aResult = nullptr;
  return NS_ERROR_FACTORY_NOT_REGISTERED;
}

static size_t
SizeOfFactoriesEntryExcludingThis(nsIDHashKey::KeyType aKey,
                                  nsFactoryEntry* const& aData,
                                  MallocSizeOf aMallocSizeOf,
                                  void* aUserArg)
{
  return aData->SizeOfIncludingThis(aMallocSizeOf);
}

static size_t
SizeOfContractIDsEntryExcludingThis(nsCStringHashKey::KeyType aKey,
                                    nsFactoryEntry* const& aData,
                                    MallocSizeOf aMallocSizeOf,
                                    void* aUserArg)
{
  
  
  return aKey.SizeOfExcludingThisMustBeUnshared(aMallocSizeOf);
}

MOZ_DEFINE_MALLOC_SIZE_OF(ComponentManagerMallocSizeOf)

NS_IMETHODIMP
nsComponentManagerImpl::CollectReports(nsIHandleReportCallback* aHandleReport,
                                       nsISupports* aData, bool aAnonymize)
{
  return MOZ_COLLECT_REPORT("explicit/xpcom/component-manager",
                            KIND_HEAP, UNITS_BYTES,
                            SizeOfIncludingThis(ComponentManagerMallocSizeOf),
                            "Memory used for the XPCOM component manager.");
}

size_t
nsComponentManagerImpl::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
{
  size_t n = aMallocSizeOf(this);
  n += mLoaderMap.SizeOfExcludingThis(nullptr, aMallocSizeOf);
  n += mFactories.SizeOfExcludingThis(SizeOfFactoriesEntryExcludingThis,
                                      aMallocSizeOf);
  n += mContractIDs.SizeOfExcludingThis(SizeOfContractIDsEntryExcludingThis,
                                        aMallocSizeOf);

  n += sStaticModules->SizeOfIncludingThis(aMallocSizeOf);
  n += sModuleLocations->SizeOfIncludingThis(aMallocSizeOf);

  n += mKnownStaticModules.SizeOfExcludingThis(aMallocSizeOf);
  n += mKnownModules.SizeOfExcludingThis(nullptr, aMallocSizeOf);

  n += PL_SizeOfArenaPoolExcludingPool(&mArena, aMallocSizeOf);

  n += mPendingServices.SizeOfExcludingThis(aMallocSizeOf);

  
  
  
  
  
  
  
  
  

  return n;
}





nsFactoryEntry::nsFactoryEntry(const mozilla::Module::CIDEntry* aEntry,
                               nsComponentManagerImpl::KnownModule* aModule)
  : mCIDEntry(aEntry)
  , mModule(aModule)
{
}

nsFactoryEntry::nsFactoryEntry(const nsCID& aCID, nsIFactory* aFactory)
  : mCIDEntry(nullptr)
  , mModule(nullptr)
  , mFactory(aFactory)
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
  nsComponentManagerImpl::gComponentManager->mLock.AssertNotCurrentThreadOwns();

  if (!mFactory) {
    
    
    if (!mModule) {
      return nullptr;
    }

    if (!mModule->Load()) {
      return nullptr;
    }

    
    nsCOMPtr<nsIFactory> factory;

    if (mModule->Module()->getFactoryProc) {
      factory = mModule->Module()->getFactoryProc(*mModule->Module(),
                                                  *mCIDEntry);
    } else if (mCIDEntry->getFactoryProc) {
      factory = mCIDEntry->getFactoryProc(*mModule->Module(), *mCIDEntry);
    } else {
      NS_ASSERTION(mCIDEntry->constructorProc, "no getfactory or constructor");
      factory = new mozilla::GenericFactory(mCIDEntry->constructorProc);
    }
    if (!factory) {
      return nullptr;
    }

    SafeMutexAutoLock lock(nsComponentManagerImpl::gComponentManager->mLock);
    
    if (!mFactory) {
      factory.swap(mFactory);
    }
  }
  nsCOMPtr<nsIFactory> factory = mFactory;
  return factory.forget();
}

size_t
nsFactoryEntry::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf)
{
  size_t n = aMallocSizeOf(this);

  
  
  
  
  
  

  return n;
}





nsresult
NS_GetComponentManager(nsIComponentManager** aResult)
{
  if (!nsComponentManagerImpl::gComponentManager) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  NS_ADDREF(*aResult = nsComponentManagerImpl::gComponentManager);
  return NS_OK;
}

nsresult
NS_GetServiceManager(nsIServiceManager** aResult)
{
  if (!nsComponentManagerImpl::gComponentManager) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  NS_ADDREF(*aResult = nsComponentManagerImpl::gComponentManager);
  return NS_OK;
}


nsresult
NS_GetComponentRegistrar(nsIComponentRegistrar** aResult)
{
  if (!nsComponentManagerImpl::gComponentManager) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  NS_ADDREF(*aResult = nsComponentManagerImpl::gComponentManager);
  return NS_OK;
}

EXPORT_XPCOM_API(nsresult)
XRE_AddStaticComponent(const mozilla::Module* aComponent)
{
  nsComponentManagerImpl::InitializeStaticModules();
  nsComponentManagerImpl::sStaticModules->AppendElement(aComponent);

  if (nsComponentManagerImpl::gComponentManager &&
      nsComponentManagerImpl::NORMAL ==
        nsComponentManagerImpl::gComponentManager->mStatus) {
    nsComponentManagerImpl::gComponentManager->RegisterModule(aComponent,
                                                              nullptr);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsComponentManagerImpl::AddBootstrappedManifestLocation(nsIFile* aLocation)
{
  nsString path;
  nsresult rv = aLocation->GetPath(path);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (Substring(path, path.Length() - 4).EqualsLiteral(".xpi")) {
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
  if (!cr) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIFile> manifest;
  nsString path;
  nsresult rv = aLocation->GetPath(path);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsComponentManagerImpl::ComponentLocation elem;
  elem.type = NS_BOOTSTRAPPED_LOCATION;

  if (Substring(path, path.Length() - 4).EqualsLiteral(".xpi")) {
    elem.location.Init(aLocation, "chrome.manifest");
  } else {
    nsCOMPtr<nsIFile> lf =
      CloneAndAppend(aLocation, NS_LITERAL_CSTRING("chrome.manifest"));
    elem.location.Init(lf);
  }

  
  nsComponentManagerImpl::sModuleLocations->RemoveElement(elem,
                                                          ComponentLocationComparator());

  rv = cr->CheckForNewChrome();
  return rv;
}

NS_IMETHODIMP
nsComponentManagerImpl::GetManifestLocations(nsIArray** aLocations)
{
  NS_ENSURE_ARG_POINTER(aLocations);
  *aLocations = nullptr;

  if (!sModuleLocations) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsCOMPtr<nsIMutableArray> locations = nsArray::Create();
  nsresult rv;
  for (uint32_t i = 0; i < sModuleLocations->Length(); ++i) {
    ComponentLocation& l = sModuleLocations->ElementAt(i);
    FileLocation loc = l.location;
    nsCString uriString;
    loc.GetURIString(uriString);
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), uriString);
    if (NS_SUCCEEDED(rv)) {
      locations->AppendElement(uri, false);
    }
  }

  locations.forget(aLocations);
  return NS_OK;
}

#ifdef MOZ_B2G_LOADER


void
nsComponentManagerImpl::XPTOnlyManifestManifest(
    XPTOnlyManifestProcessingContext&  aCx, int aLineNo, char* const* aArgv)
{
  char* file = aArgv[0];
  FileLocation f(aCx.mFile, file);

  DoRegisterManifest(NS_COMPONENT_LOCATION, f, false, true);
}


void
nsComponentManagerImpl::XPTOnlyManifestXPT(
    XPTOnlyManifestProcessingContext& aCx, int aLineNo, char* const* aArgv)
{
  FileLocation f(aCx.mFile, aArgv[0]);
  DoRegisterXPT(f);
}







 void
nsComponentManagerImpl::PreloadXPT(nsIFile* aFile)
{
  MOZ_ASSERT(!nsComponentManagerImpl::gComponentManager);
  FileLocation location(aFile, "chrome.manifest");

  DoRegisterManifest(NS_COMPONENT_LOCATION, location,
                     false, true );
}

void
PreloadXPT(nsIFile* aOmnijarFile)
{
  nsComponentManagerImpl::PreloadXPT(aOmnijarFile);
}

#endif 

EXPORT_XPCOM_API(nsresult)
XRE_AddManifestLocation(NSLocationType aType, nsIFile* aLocation)
{
  nsComponentManagerImpl::InitializeModuleLocations();
  nsComponentManagerImpl::ComponentLocation* c =
    nsComponentManagerImpl::sModuleLocations->AppendElement();
  c->type = aType;
  c->location.Init(aLocation);

  if (nsComponentManagerImpl::gComponentManager &&
      nsComponentManagerImpl::NORMAL ==
        nsComponentManagerImpl::gComponentManager->mStatus) {
    nsComponentManagerImpl::gComponentManager->RegisterManifest(aType,
                                                                c->location,
                                                                false);
  }

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
      nsComponentManagerImpl::NORMAL ==
        nsComponentManagerImpl::gComponentManager->mStatus) {
    nsComponentManagerImpl::gComponentManager->RegisterManifest(aType,
                                                                c->location,
                                                                false);
  }

  return NS_OK;
}

