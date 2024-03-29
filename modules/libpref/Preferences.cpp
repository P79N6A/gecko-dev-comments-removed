





#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/ContentChild.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/HashFunctions.h"

#include "nsXULAppAPI.h"

#include "mozilla/Preferences.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDataHashtable.h"
#include "nsDirectoryServiceDefs.h"
#include "nsICategoryManager.h"
#include "nsCategoryManagerUtils.h"
#include "nsNetUtil.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIObserverService.h"
#include "nsIOutputStream.h"
#include "nsISafeOutputStream.h"
#include "nsISimpleEnumerator.h"
#include "nsIStringEnumerator.h"
#include "nsIZipReader.h"
#include "nsPrefBranch.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"
#include "nsCOMArray.h"
#include "nsXPCOMCID.h"
#include "nsAutoPtr.h"
#include "nsPrintfCString.h"

#include "nsQuickSort.h"
#include "pldhash.h"

#include "prefapi.h"
#include "prefread.h"
#include "prefapi_private_data.h"

#include "mozilla/Omnijar.h"
#include "nsZipArchive.h"

#include "nsTArray.h"
#include "nsRefPtrHashtable.h"
#include "nsIMemoryReporter.h"
#include "nsThreadUtils.h"

#ifdef DEBUG
#define ENSURE_MAIN_PROCESS(message, pref) do {                                \
  if (MOZ_UNLIKELY(!XRE_IsParentProcess())) {        \
    nsPrintfCString msg("ENSURE_MAIN_PROCESS failed. %s %s", message, pref);   \
    NS_WARNING(msg.get());                                                     \
    return NS_ERROR_NOT_AVAILABLE;                                             \
  }                                                                            \
} while (0);
#else
#define ENSURE_MAIN_PROCESS(message, pref)                                     \
  if (MOZ_UNLIKELY(!XRE_IsParentProcess())) {        \
    return NS_ERROR_NOT_AVAILABLE;                                             \
  }
#endif

class PrefCallback;

namespace mozilla {


#define INITIAL_PREF_FILES 10
static NS_DEFINE_CID(kZipReaderCID, NS_ZIPREADER_CID);


static nsresult openPrefFile(nsIFile* aFile);
static nsresult pref_InitInitialObjects(void);
static nsresult pref_LoadPrefsInDirList(const char *listId);
static nsresult ReadExtensionPrefs(nsIFile *aFile);

static const char kTelemetryPref[] = "toolkit.telemetry.enabled";
static const char kOldTelemetryPref[] = "toolkit.telemetry.enabledPreRelease";
static const char kChannelPref[] = "app.update.channel";

Preferences* Preferences::sPreferences = nullptr;
nsIPrefBranch* Preferences::sRootBranch = nullptr;
nsIPrefBranch* Preferences::sDefaultRootBranch = nullptr;
bool Preferences::sShutdown = false;

class ValueObserverHashKey : public PLDHashEntryHdr {
public:
  typedef ValueObserverHashKey* KeyType;
  typedef const ValueObserverHashKey* KeyTypePointer;

  static const ValueObserverHashKey* KeyToPointer(ValueObserverHashKey *aKey)
  {
    return aKey;
  }

  static PLDHashNumber HashKey(const ValueObserverHashKey *aKey)
  {
    PLDHashNumber hash = HashString(aKey->mPrefName);
    return AddToHash(hash, aKey->mCallback);
  }

  ValueObserverHashKey(const char *aPref, PrefChangedFunc aCallback) :
    mPrefName(aPref), mCallback(aCallback) { }

  explicit ValueObserverHashKey(const ValueObserverHashKey *aOther) :
    mPrefName(aOther->mPrefName), mCallback(aOther->mCallback)
  { }

  bool KeyEquals(const ValueObserverHashKey *aOther) const
  {
    return mCallback == aOther->mCallback && mPrefName == aOther->mPrefName;
  }

  ValueObserverHashKey *GetKey() const
  {
    return const_cast<ValueObserverHashKey*>(this);
  }

  enum { ALLOW_MEMMOVE = true };

  nsCString mPrefName;
  PrefChangedFunc mCallback;
};

class ValueObserver final : public nsIObserver,
                            public ValueObserverHashKey
{
  ~ValueObserver() {
    Preferences::RemoveObserver(this, mPrefName.get());
  }

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  ValueObserver(const char *aPref, PrefChangedFunc aCallback)
    : ValueObserverHashKey(aPref, aCallback) { }

  void AppendClosure(void *aClosure) {
    mClosures.AppendElement(aClosure);
  }

  void RemoveClosure(void *aClosure) {
    mClosures.RemoveElement(aClosure);
  }

  bool HasNoClosures() {
    return mClosures.Length() == 0;
  }

  nsTArray<void*> mClosures;
};

NS_IMPL_ISUPPORTS(ValueObserver, nsIObserver)

NS_IMETHODIMP
ValueObserver::Observe(nsISupports     *aSubject,
                       const char      *aTopic,
                       const char16_t *aData)
{
  NS_ASSERTION(!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID),
               "invalid topic");
  NS_ConvertUTF16toUTF8 data(aData);
  for (uint32_t i = 0; i < mClosures.Length(); i++) {
    mCallback(data.get(), mClosures.ElementAt(i));
  }

  return NS_OK;
}

struct CacheData {
  void* cacheLocation;
  union {
    bool defaultValueBool;
    int32_t defaultValueInt;
    uint32_t defaultValueUint;
    float defaultValueFloat;
  };
};

static nsTArray<nsAutoPtr<CacheData> >* gCacheData = nullptr;
static nsRefPtrHashtable<ValueObserverHashKey,
                         ValueObserver>* gObserverTable = nullptr;

#ifdef DEBUG
static bool
HaveExistingCacheFor(void* aPtr)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (gCacheData) {
    for (size_t i = 0, count = gCacheData->Length(); i < count; ++i) {
      if ((*gCacheData)[i]->cacheLocation == aPtr) {
        return true;
      }
    }
  }
  return false;
}

static void
AssertNotAlreadyCached(const char* aPrefType,
                       const char* aPref,
                       void* aPtr)
{
  if (HaveExistingCacheFor(aPtr)) {
    fprintf_stderr(stderr,
      "Attempt to add a %s pref cache for preference '%s' at address '%p'"
      "was made. However, a pref was already cached at this address.\n",
      aPrefType, aPref, aPtr);
    MOZ_ASSERT(false, "Should not have an existing pref cache for this address");
  }
}
#endif

static size_t
SizeOfObserverEntryExcludingThis(ValueObserverHashKey* aKey,
                                 const nsRefPtr<ValueObserver>& aData,
                                 mozilla::MallocSizeOf aMallocSizeOf,
                                 void*)
{
  size_t n = 0;
  n += aKey->mPrefName.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
  n += aData->mClosures.SizeOfExcludingThis(aMallocSizeOf);
  return n;
}



 int64_t
Preferences::SizeOfIncludingThisAndOtherStuff(mozilla::MallocSizeOf aMallocSizeOf)
{
  NS_ENSURE_TRUE(InitStaticMembers(), 0);

  size_t n = aMallocSizeOf(sPreferences);
  if (gHashTable) {
    
    
    n += PL_DHashTableSizeOfExcludingThis(gHashTable, nullptr, aMallocSizeOf);
  }
  if (gCacheData) {
    n += gCacheData->SizeOfIncludingThis(aMallocSizeOf);
    for (uint32_t i = 0, count = gCacheData->Length(); i < count; ++i) {
      n += aMallocSizeOf((*gCacheData)[i]);
    }
  }
  if (gObserverTable) {
    n += aMallocSizeOf(gObserverTable);
    n += gObserverTable->SizeOfExcludingThis(SizeOfObserverEntryExcludingThis,
                                             aMallocSizeOf);
  }
  
  
  n += pref_SizeOfPrivateData(aMallocSizeOf);
  return n;
}

class PreferenceServiceReporter final : public nsIMemoryReporter
{
  ~PreferenceServiceReporter() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTER

protected:
  static const uint32_t kSuspectReferentCount = 1000;
  static PLDHashOperator CountReferents(PrefCallback* aKey,
                                        nsAutoPtr<PrefCallback>& aCallback,
                                        void* aClosure);
};

NS_IMPL_ISUPPORTS(PreferenceServiceReporter, nsIMemoryReporter)

struct PreferencesReferentCount {
  PreferencesReferentCount() : numStrong(0), numWeakAlive(0), numWeakDead(0) {}
  size_t numStrong;
  size_t numWeakAlive;
  size_t numWeakDead;
  nsTArray<nsCString> suspectPreferences;
  
  nsDataHashtable<nsCStringHashKey, uint32_t> prefCounter;
};

PLDHashOperator
PreferenceServiceReporter::CountReferents(PrefCallback* aKey,
                                          nsAutoPtr<PrefCallback>& aCallback,
                                          void* aClosure)
{
  PreferencesReferentCount* referentCount =
    static_cast<PreferencesReferentCount*>(aClosure);

  nsPrefBranch* prefBranch = aCallback->GetPrefBranch();
  const char* pref = prefBranch->getPrefName(aCallback->GetDomain().get());

  if (aCallback->IsWeak()) {
    nsCOMPtr<nsIObserver> callbackRef = do_QueryReferent(aCallback->mWeakRef);
    if (callbackRef) {
      referentCount->numWeakAlive++;
    } else {
      referentCount->numWeakDead++;
    }
  } else {
    referentCount->numStrong++;
  }

  nsDependentCString prefString(pref);
  uint32_t oldCount = 0;
  referentCount->prefCounter.Get(prefString, &oldCount);
  uint32_t currentCount = oldCount + 1;
  referentCount->prefCounter.Put(prefString, currentCount);

  
  
  if (currentCount == kSuspectReferentCount) {
    referentCount->suspectPreferences.AppendElement(prefString);
  }

  return PL_DHASH_NEXT;
}

MOZ_DEFINE_MALLOC_SIZE_OF(PreferenceServiceMallocSizeOf)

NS_IMETHODIMP
PreferenceServiceReporter::CollectReports(nsIMemoryReporterCallback* aCb,
                                          nsISupports* aClosure,
                                          bool aAnonymize)
{
#define REPORT(_path, _kind, _units, _amount, _desc)                          \
    do {                                                                      \
      nsresult rv;                                                            \
      rv = aCb->Callback(EmptyCString(), _path, _kind,                        \
                         _units, _amount, NS_LITERAL_CSTRING(_desc),          \
                         aClosure);                                           \
      NS_ENSURE_SUCCESS(rv, rv);                                              \
    } while (0)

  REPORT(NS_LITERAL_CSTRING("explicit/preferences"),
         KIND_HEAP, UNITS_BYTES,
         Preferences::SizeOfIncludingThisAndOtherStuff(PreferenceServiceMallocSizeOf),
         "Memory used by the preferences system.");

  nsPrefBranch* rootBranch =
    static_cast<nsPrefBranch*>(Preferences::GetRootBranch());
  if (!rootBranch) {
    return NS_OK;
  }

  PreferencesReferentCount referentCount;
  rootBranch->mObservers.Enumerate(&CountReferents, &referentCount);

  for (uint32_t i = 0; i < referentCount.suspectPreferences.Length(); i++) {
    nsCString& suspect = referentCount.suspectPreferences[i];
    uint32_t totalReferentCount = 0;
    referentCount.prefCounter.Get(suspect, &totalReferentCount);

    nsPrintfCString suspectPath("preference-service-suspect/"
                                "referent(pref=%s)", suspect.get());

    REPORT(suspectPath,
           KIND_OTHER, UNITS_COUNT, totalReferentCount,
           "A preference with a suspiciously large number "
           "referents (symptom of a leak).");
  }

  REPORT(NS_LITERAL_CSTRING("preference-service/referent/strong"),
         KIND_OTHER, UNITS_COUNT, referentCount.numStrong,
         "The number of strong referents held by the preference service.");

  REPORT(NS_LITERAL_CSTRING("preference-service/referent/weak/alive"),
         KIND_OTHER, UNITS_COUNT, referentCount.numWeakAlive,
         "The number of weak referents held by the preference service "
         "that are still alive.");

  REPORT(NS_LITERAL_CSTRING("preference-service/referent/weak/dead"),
         KIND_OTHER, UNITS_COUNT, referentCount.numWeakDead,
         "The number of weak referents held by the preference service "
         "that are dead.");

#undef REPORT

  return NS_OK;
}

namespace {
class AddPreferencesMemoryReporterRunnable : public nsRunnable
{
  NS_IMETHOD Run()
  {
    return RegisterStrongMemoryReporter(new PreferenceServiceReporter());
  }
};
} 


Preferences*
Preferences::GetInstanceForService()
{
  if (sPreferences) {
    NS_ADDREF(sPreferences);
    return sPreferences;
  }

  NS_ENSURE_TRUE(!sShutdown, nullptr);

  sRootBranch = new nsPrefBranch("", false);
  NS_ADDREF(sRootBranch);
  sDefaultRootBranch = new nsPrefBranch("", true);
  NS_ADDREF(sDefaultRootBranch);

  sPreferences = new Preferences();
  NS_ADDREF(sPreferences);

  if (NS_FAILED(sPreferences->Init())) {
    
    NS_RELEASE(sPreferences);
    return nullptr;
  }

  gCacheData = new nsTArray<nsAutoPtr<CacheData> >();

  gObserverTable = new nsRefPtrHashtable<ValueObserverHashKey, ValueObserver>();

  
  
  
  
  nsRefPtr<AddPreferencesMemoryReporterRunnable> runnable =
    new AddPreferencesMemoryReporterRunnable();
  NS_DispatchToMainThread(runnable);

  NS_ADDREF(sPreferences);
  return sPreferences;
}


bool
Preferences::InitStaticMembers()
{
#ifndef MOZ_B2G
  MOZ_ASSERT(NS_IsMainThread());
#endif

  if (!sShutdown && !sPreferences) {
    nsCOMPtr<nsIPrefService> prefService =
      do_GetService(NS_PREFSERVICE_CONTRACTID);
  }

  return sPreferences != nullptr;
}


void
Preferences::Shutdown()
{
  if (!sShutdown) {
    sShutdown = true; 

    
    
    
    if (sPreferences) {
      sPreferences->Release();
    }
  }
}







Preferences::Preferences()
{
}

Preferences::~Preferences()
{
  NS_ASSERTION(sPreferences == this, "Isn't this the singleton instance?");

  delete gObserverTable;
  gObserverTable = nullptr;

  delete gCacheData;
  gCacheData = nullptr;

  NS_RELEASE(sRootBranch);
  NS_RELEASE(sDefaultRootBranch);

  sPreferences = nullptr;

  PREF_Cleanup();
}






NS_IMPL_ADDREF(Preferences)
NS_IMPL_RELEASE(Preferences)

NS_INTERFACE_MAP_BEGIN(Preferences)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPrefService)
    NS_INTERFACE_MAP_ENTRY(nsIPrefService)
    NS_INTERFACE_MAP_ENTRY(nsIObserver)
    NS_INTERFACE_MAP_ENTRY(nsIPrefBranch)
    NS_INTERFACE_MAP_ENTRY(nsIPrefBranch2)
    NS_INTERFACE_MAP_ENTRY(nsIPrefBranchInternal)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END






nsresult
Preferences::Init()
{
  nsresult rv;

  PREF_Init();

  rv = pref_InitInitialObjects();
  NS_ENSURE_SUCCESS(rv, rv);

  using mozilla::dom::ContentChild;
  if (XRE_IsContentProcess()) {
    InfallibleTArray<PrefSetting> prefs;
    ContentChild::GetSingleton()->SendReadPrefsArray(&prefs);

    
    for (uint32_t i = 0; i < prefs.Length(); ++i) {
      pref_SetPref(prefs[i]);
    }
    return NS_OK;
  }

  nsXPIDLCString lockFileName;
  







  rv = PREF_CopyCharPref("general.config.filename", getter_Copies(lockFileName), false);
  if (NS_SUCCEEDED(rv))
    NS_CreateServicesFromCategory("pref-config-startup",
                                  static_cast<nsISupports *>(static_cast<void *>(this)),
                                  "pref-config-startup");    

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  rv = observerService->AddObserver(this, "profile-before-change", true);

  observerService->AddObserver(this, "load-extension-defaults", true);
  observerService->AddObserver(this, "suspend_process_notification", true);

  return(rv);
}


nsresult
Preferences::ResetAndReadUserPrefs()
{
  sPreferences->ResetUserPrefs();
  return sPreferences->ReadUserPrefs(nullptr);
}

NS_IMETHODIMP
Preferences::Observe(nsISupports *aSubject, const char *aTopic,
                     const char16_t *someData)
{
  if (XRE_IsContentProcess())
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = NS_OK;

  if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    rv = SavePrefFile(nullptr);
  } else if (!strcmp(aTopic, "load-extension-defaults")) {
    pref_LoadPrefsInDirList(NS_EXT_PREFS_DEFAULTS_DIR_LIST);
  } else if (!nsCRT::strcmp(aTopic, "reload-default-prefs")) {
    
    pref_InitInitialObjects();
  } else if (!nsCRT::strcmp(aTopic, "suspend_process_notification")) {
    
    
    
    rv = SavePrefFile(nullptr);
  }
  return rv;
}


NS_IMETHODIMP
Preferences::ReadUserPrefs(nsIFile *aFile)
{
  if (XRE_IsContentProcess()) {
    NS_ERROR("cannot load prefs from content process");
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv;

  if (nullptr == aFile) {
    rv = UseDefaultPrefFile();
    
    
    (void) UseUserPrefFile();

    
    if (!Preferences::GetBool(kOldTelemetryPref, true)) {
      Preferences::SetBool(kTelemetryPref, false);
      Preferences::ClearUser(kOldTelemetryPref);
    }

    NotifyServiceObservers(NS_PREFSERVICE_READ_TOPIC_ID);
  } else {
    rv = ReadAndOwnUserPrefFile(aFile);
  }

  return rv;
}

NS_IMETHODIMP
Preferences::ResetPrefs()
{
  if (XRE_IsContentProcess()) {
    NS_ERROR("cannot reset prefs from content process");
    return NS_ERROR_NOT_AVAILABLE;
  }

  NotifyServiceObservers(NS_PREFSERVICE_RESET_TOPIC_ID);
  PREF_CleanupPrefs();

  PREF_Init();

  return pref_InitInitialObjects();
}

NS_IMETHODIMP
Preferences::ResetUserPrefs()
{
  if (XRE_IsContentProcess()) {
    NS_ERROR("cannot reset user prefs from content process");
    return NS_ERROR_NOT_AVAILABLE;
  }

  PREF_ClearAllUserPrefs();
  return NS_OK;    
}

NS_IMETHODIMP
Preferences::SavePrefFile(nsIFile *aFile)
{
  if (XRE_IsContentProcess()) {
    NS_ERROR("cannot save pref file from content process");
    return NS_ERROR_NOT_AVAILABLE;
  }

  return SavePrefFileInternal(aFile);
}

static nsresult
ReadExtensionPrefs(nsIFile *aFile)
{
  nsresult rv;
  nsCOMPtr<nsIZipReader> reader = do_CreateInstance(kZipReaderCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = reader->Open(aFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUTF8StringEnumerator> files;
  rv = reader->FindEntries(nsDependentCString("defaults/preferences/*.(J|j)(S|s)$"),
                           getter_AddRefs(files));
  NS_ENSURE_SUCCESS(rv, rv);

  char buffer[4096];

  bool more;
  while (NS_SUCCEEDED(rv = files->HasMore(&more)) && more) {
    nsAutoCString entry;
    rv = files->GetNext(entry);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIInputStream> stream;
    rv = reader->GetInputStream(entry, getter_AddRefs(stream));
    NS_ENSURE_SUCCESS(rv, rv);

    uint64_t avail;
    uint32_t read;

    PrefParseState ps;
    PREF_InitParseState(&ps, PREF_ReaderCallback, nullptr);
    while (NS_SUCCEEDED(rv = stream->Available(&avail)) && avail) {
      rv = stream->Read(buffer, 4096, &read);
      if (NS_FAILED(rv)) {
        NS_WARNING("Pref stream read failed");
        break;
      }

      PREF_ParseBuf(&ps, buffer, read);
    }
    PREF_FinalizeParseState(&ps);
  }
  return rv;
}

void
Preferences::SetPreference(const PrefSetting& aPref)
{
  pref_SetPref(aPref);
}

void
Preferences::GetPreference(PrefSetting* aPref)
{
  PrefHashEntry *entry = pref_HashTableLookup(aPref->name().get());
  if (!entry)
    return;

  pref_GetPrefFromEntry(entry, aPref);
}

void
Preferences::GetPreferences(InfallibleTArray<PrefSetting>* aPrefs)
{
  aPrefs->SetCapacity(gHashTable->Capacity());
  for (auto iter = gHashTable->Iter(); !iter.Done(); iter.Next()) {
    auto entry = static_cast<PrefHashEntry*>(iter.Get());
    dom::PrefSetting *pref = aPrefs->AppendElement();
    pref_GetPrefFromEntry(entry, pref);
  }
}

NS_IMETHODIMP
Preferences::GetBranch(const char *aPrefRoot, nsIPrefBranch **_retval)
{
  nsresult rv;

  if ((nullptr != aPrefRoot) && (*aPrefRoot != '\0')) {
    
    nsRefPtr<nsPrefBranch> prefBranch = new nsPrefBranch(aPrefRoot, false);
    prefBranch.forget(_retval);
    rv = NS_OK;
  } else {
    
    nsCOMPtr<nsIPrefBranch> root(sRootBranch);
    root.forget(_retval);
    rv = NS_OK;
  }
  return rv;
}

NS_IMETHODIMP
Preferences::GetDefaultBranch(const char *aPrefRoot, nsIPrefBranch **_retval)
{
  if (!aPrefRoot || !aPrefRoot[0]) {
    nsCOMPtr<nsIPrefBranch> root(sDefaultRootBranch);
    root.forget(_retval);
    return NS_OK;
  }

  
  nsRefPtr<nsPrefBranch> prefBranch = new nsPrefBranch(aPrefRoot, true);
  if (!prefBranch)
    return NS_ERROR_OUT_OF_MEMORY;

  prefBranch.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
Preferences::GetDirty(bool *_retval) {
  *_retval = gDirty;
  return NS_OK;
}

nsresult
Preferences::NotifyServiceObservers(const char *aTopic)
{
  nsCOMPtr<nsIObserverService> observerService = 
    mozilla::services::GetObserverService();  
  if (!observerService)
    return NS_ERROR_FAILURE;

  nsISupports *subject = (nsISupports *)((nsIPrefService *)this);
  observerService->NotifyObservers(subject, aTopic, nullptr);
  
  return NS_OK;
}

nsresult
Preferences::UseDefaultPrefFile()
{
  nsCOMPtr<nsIFile> aFile;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_PREFS_50_FILE, getter_AddRefs(aFile));

  if (NS_SUCCEEDED(rv)) {
    rv = ReadAndOwnUserPrefFile(aFile);
    
    
    
    if (NS_FAILED(rv)) {
      if (NS_FAILED(SavePrefFileInternal(aFile)))
        NS_ERROR("Failed to save new shared pref file");
      else
        rv = NS_OK;
    }
  }
  
  return rv;
}

nsresult
Preferences::UseUserPrefFile()
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIFile> aFile;
  nsDependentCString prefsDirProp(NS_APP_PREFS_50_DIR);

  rv = NS_GetSpecialDirectory(prefsDirProp.get(), getter_AddRefs(aFile));
  if (NS_SUCCEEDED(rv) && aFile) {
    rv = aFile->AppendNative(NS_LITERAL_CSTRING("user.js"));
    if (NS_SUCCEEDED(rv)) {
      bool exists = false;
      aFile->Exists(&exists);
      if (exists) {
        rv = openPrefFile(aFile);
      } else {
        rv = NS_ERROR_FILE_NOT_FOUND;
      }
    }
  }
  return rv;
}

nsresult
Preferences::MakeBackupPrefFile(nsIFile *aFile)
{
  
  
  nsAutoString newFilename;
  nsresult rv = aFile->GetLeafName(newFilename);
  NS_ENSURE_SUCCESS(rv, rv);
  newFilename.Insert(NS_LITERAL_STRING("Invalid"), 0);
  nsCOMPtr<nsIFile> newFile;
  rv = aFile->GetParent(getter_AddRefs(newFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = newFile->Append(newFilename);
  NS_ENSURE_SUCCESS(rv, rv);
  bool exists = false;
  newFile->Exists(&exists);
  if (exists) {
    rv = newFile->Remove(false);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = aFile->CopyTo(nullptr, newFilename);
  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

nsresult
Preferences::ReadAndOwnUserPrefFile(nsIFile *aFile)
{
  NS_ENSURE_ARG(aFile);
  
  if (mCurrentFile == aFile)
    return NS_OK;
  mCurrentFile = aFile;

  nsresult rv = NS_OK;
  bool exists = false;
  mCurrentFile->Exists(&exists);
  if (exists) {
    rv = openPrefFile(mCurrentFile);
    if (NS_FAILED(rv)) {
      
      
      
      MakeBackupPrefFile(mCurrentFile);
    }
  } else {
    rv = NS_ERROR_FILE_NOT_FOUND;
  }

  return rv;
}

nsresult
Preferences::SavePrefFileInternal(nsIFile *aFile)
{
  if (nullptr == aFile) {
    
    
    if (!gDirty)
      return NS_OK;
    
    
    nsresult rv = NS_OK;
    if (mCurrentFile)
      rv = WritePrefFile(mCurrentFile);

    return rv;
  } else {
    return WritePrefFile(aFile);
  }
}

nsresult
Preferences::WritePrefFile(nsIFile* aFile)
{
  const char                outHeader[] =
    "# Mozilla User Preferences"
    NS_LINEBREAK
    NS_LINEBREAK
    "











"
    NS_LINEBREAK
    NS_LINEBREAK;

  nsCOMPtr<nsIOutputStream> outStreamSink;
  nsCOMPtr<nsIOutputStream> outStream;
  uint32_t                  writeAmount;
  nsresult                  rv;

  if (!gHashTable)
    return NS_ERROR_NOT_INITIALIZED;

  
  rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(outStreamSink),
                                       aFile,
                                       -1,
                                       0600);
  if (NS_FAILED(rv)) 
      return rv;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(outStream), outStreamSink, 4096);
  if (NS_FAILED(rv)) 
      return rv;  

  nsAutoArrayPtr<char*> valueArray(new char*[gHashTable->EntryCount()]);
  memset(valueArray, 0, gHashTable->EntryCount() * sizeof(char*));

  
  pref_savePrefs(gHashTable, valueArray);

  
  NS_QuickSort(valueArray, gHashTable->EntryCount(), sizeof(char *),
               pref_CompareStrings, nullptr);

  
  outStream->Write(outHeader, sizeof(outHeader) - 1, &writeAmount);

  char** walker = valueArray;
  for (uint32_t valueIdx = 0; valueIdx < gHashTable->EntryCount(); valueIdx++, walker++) {
    if (*walker) {
      outStream->Write(*walker, strlen(*walker), &writeAmount);
      outStream->Write(NS_LINEBREAK, NS_LINEBREAK_LEN, &writeAmount);
      free(*walker);
    }
  }

  
  
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(outStream);
  NS_ASSERTION(safeStream, "expected a safe output stream!");
  if (safeStream) {
    rv = safeStream->Finish();
    if (NS_FAILED(rv)) {
      NS_WARNING("failed to save prefs file! possible data loss");
      return rv;
    }
  }

  gDirty = false;
  return NS_OK;
}

static nsresult openPrefFile(nsIFile* aFile)
{
  nsCOMPtr<nsIInputStream> inStr;

  nsresult rv = NS_NewLocalFileInputStream(getter_AddRefs(inStr), aFile);
  if (NS_FAILED(rv)) 
    return rv;        

  uint64_t fileSize64;
  rv = inStr->Available(&fileSize64);
  if (NS_FAILED(rv))
    return rv;
  NS_ENSURE_TRUE(fileSize64 <= UINT32_MAX, NS_ERROR_FILE_TOO_BIG);

  uint32_t fileSize = (uint32_t)fileSize64;
  nsAutoArrayPtr<char> fileBuffer(new char[fileSize]);
  if (fileBuffer == nullptr)
    return NS_ERROR_OUT_OF_MEMORY;

  PrefParseState ps;
  PREF_InitParseState(&ps, PREF_ReaderCallback, nullptr);

  
  
  nsresult rv2 = NS_OK;
  for (;;) {
    uint32_t amtRead = 0;
    rv = inStr->Read((char*)fileBuffer, fileSize, &amtRead);
    if (NS_FAILED(rv) || amtRead == 0)
      break;
    if (!PREF_ParseBuf(&ps, fileBuffer, amtRead))
      rv2 = NS_ERROR_FILE_CORRUPTED;
  }

  PREF_FinalizeParseState(&ps);

  return NS_FAILED(rv) ? rv : rv2;
}





static int
pref_CompareFileNames(nsIFile* aFile1, nsIFile* aFile2, void* )
{
  nsAutoCString filename1, filename2;
  aFile1->GetNativeLeafName(filename1);
  aFile2->GetNativeLeafName(filename2);

  return Compare(filename2, filename1);
}






static nsresult
pref_LoadPrefsInDir(nsIFile* aDir, char const *const *aSpecialFiles, uint32_t aSpecialFilesCount)
{
  nsresult rv, rv2;
  bool hasMoreElements;

  nsCOMPtr<nsISimpleEnumerator> dirIterator;

  
  rv = aDir->GetDirectoryEntries(getter_AddRefs(dirIterator));
  if (NS_FAILED(rv)) {
    
    
    if (rv == NS_ERROR_FILE_NOT_FOUND || rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)
      rv = NS_OK;
    return rv;
  }

  rv = dirIterator->HasMoreElements(&hasMoreElements);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMArray<nsIFile> prefFiles(INITIAL_PREF_FILES);
  nsCOMArray<nsIFile> specialFiles(aSpecialFilesCount);
  nsCOMPtr<nsIFile> prefFile;

  while (hasMoreElements && NS_SUCCEEDED(rv)) {
    nsAutoCString leafName;

    nsCOMPtr<nsISupports> supports;
    rv = dirIterator->GetNext(getter_AddRefs(supports));
    prefFile = do_QueryInterface(supports);
    if (NS_FAILED(rv)) {
      break;
    }

    prefFile->GetNativeLeafName(leafName);
    NS_ASSERTION(!leafName.IsEmpty(), "Failure in default prefs: directory enumerator returned empty file?");

    
    if (StringEndsWith(leafName, NS_LITERAL_CSTRING(".js"),
                       nsCaseInsensitiveCStringComparator())) {
      bool shouldParse = true;
      
      for (uint32_t i = 0; i < aSpecialFilesCount; ++i) {
        if (leafName.Equals(nsDependentCString(aSpecialFiles[i]))) {
          shouldParse = false;
          
          
          specialFiles.ReplaceObjectAt(prefFile, i);
        }
      }

      if (shouldParse) {
        prefFiles.AppendObject(prefFile);
      }
    }

    rv = dirIterator->HasMoreElements(&hasMoreElements);
  }

  if (prefFiles.Count() + specialFiles.Count() == 0) {
    NS_WARNING("No default pref files found.");
    if (NS_SUCCEEDED(rv)) {
      rv = NS_SUCCESS_FILE_DIRECTORY_EMPTY;
    }
    return rv;
  }

  prefFiles.Sort(pref_CompareFileNames, nullptr);
  
  uint32_t arrayCount = prefFiles.Count();
  uint32_t i;
  for (i = 0; i < arrayCount; ++i) {
    rv2 = openPrefFile(prefFiles[i]);
    if (NS_FAILED(rv2)) {
      NS_ERROR("Default pref file not parsed successfully.");
      rv = rv2;
    }
  }

  arrayCount = specialFiles.Count();
  for (i = 0; i < arrayCount; ++i) {
    
    nsIFile* file = specialFiles[i];
    if (file) {
      rv2 = openPrefFile(file);
      if (NS_FAILED(rv2)) {
        NS_ERROR("Special default pref file not parsed successfully.");
        rv = rv2;
      }
    }
  }

  return rv;
}

static nsresult pref_LoadPrefsInDirList(const char *listId)
{
  nsresult rv;
  nsCOMPtr<nsIProperties> dirSvc(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISimpleEnumerator> list;
  dirSvc->Get(listId,
              NS_GET_IID(nsISimpleEnumerator),
              getter_AddRefs(list));
  if (!list)
    return NS_OK;

  bool hasMore;
  while (NS_SUCCEEDED(list->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> elem;
    list->GetNext(getter_AddRefs(elem));
    if (!elem)
      continue;

    nsCOMPtr<nsIFile> path = do_QueryInterface(elem);
    if (!path)
      continue;

    nsAutoCString leaf;
    path->GetNativeLeafName(leaf);

    
    if (Substring(leaf, leaf.Length() - 4).EqualsLiteral(".xpi"))
      ReadExtensionPrefs(path);
    else
      pref_LoadPrefsInDir(path, nullptr, 0);
  }
  return NS_OK;
}

static nsresult pref_ReadPrefFromJar(nsZipArchive* jarReader, const char *name)
{
  nsZipItemPtr<char> manifest(jarReader, name, true);
  NS_ENSURE_TRUE(manifest.Buffer(), NS_ERROR_NOT_AVAILABLE);

  PrefParseState ps;
  PREF_InitParseState(&ps, PREF_ReaderCallback, nullptr);
  PREF_ParseBuf(&ps, manifest, manifest.Length());
  PREF_FinalizeParseState(&ps);

  return NS_OK;
}





static nsresult pref_InitInitialObjects()
{
  nsresult rv;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsZipFind *findPtr;
  nsAutoPtr<nsZipFind> find;
  nsTArray<nsCString> prefEntries;
  const char *entryName;
  uint16_t entryNameLen;

  nsRefPtr<nsZipArchive> jarReader = mozilla::Omnijar::GetReader(mozilla::Omnijar::GRE);
  if (jarReader) {
    
    rv = pref_ReadPrefFromJar(jarReader, "greprefs.js");
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = jarReader->FindInit("defaults/pref/*.js$", &findPtr);
    NS_ENSURE_SUCCESS(rv, rv);

    find = findPtr;
    while (NS_SUCCEEDED(find->FindNext(&entryName, &entryNameLen))) {
      prefEntries.AppendElement(Substring(entryName, entryNameLen));
    }

    prefEntries.Sort();
    for (uint32_t i = prefEntries.Length(); i--; ) {
      rv = pref_ReadPrefFromJar(jarReader, prefEntries[i].get());
      if (NS_FAILED(rv))
        NS_WARNING("Error parsing preferences.");
    }
  } else {
    
    nsCOMPtr<nsIFile> greprefsFile;
    rv = NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(greprefsFile));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = greprefsFile->AppendNative(NS_LITERAL_CSTRING("greprefs.js"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = openPrefFile(greprefsFile);
    if (NS_FAILED(rv))
      NS_WARNING("Error parsing GRE default preferences. Is this an old-style embedding app?");
  }

  
  nsCOMPtr<nsIFile> defaultPrefDir;

  rv = NS_GetSpecialDirectory(NS_APP_PREF_DEFAULTS_50_DIR, getter_AddRefs(defaultPrefDir));
  NS_ENSURE_SUCCESS(rv, rv);

  
  static const char* specialFiles[] = {
#if defined(XP_MACOSX)
    "macprefs.js"
#elif defined(XP_WIN)
    "winpref.js"
#elif defined(XP_UNIX)
    "unix.js"
#if defined(VMS)
    , "openvms.js"
#elif defined(_AIX)
    , "aix.js"
#endif
#elif defined(XP_BEOS)
    "beos.js"
#endif
  };

  rv = pref_LoadPrefsInDir(defaultPrefDir, specialFiles, ArrayLength(specialFiles));
  if (NS_FAILED(rv))
    NS_WARNING("Error parsing application default preferences.");

  
  
  nsRefPtr<nsZipArchive> appJarReader = mozilla::Omnijar::GetReader(mozilla::Omnijar::APP);
  
  
  if (!appJarReader)
    appJarReader = mozilla::Omnijar::GetReader(mozilla::Omnijar::GRE);
  if (appJarReader) {
    rv = appJarReader->FindInit("defaults/preferences/*.js$", &findPtr);
    NS_ENSURE_SUCCESS(rv, rv);
    find = findPtr;
    prefEntries.Clear();
    while (NS_SUCCEEDED(find->FindNext(&entryName, &entryNameLen))) {
      prefEntries.AppendElement(Substring(entryName, entryNameLen));
    }
    prefEntries.Sort();
    for (uint32_t i = prefEntries.Length(); i--; ) {
      rv = pref_ReadPrefFromJar(appJarReader, prefEntries[i].get());
      if (NS_FAILED(rv))
        NS_WARNING("Error parsing preferences.");
    }
  }

  rv = pref_LoadPrefsInDirList(NS_APP_PREFS_DEFAULTS_DIR_LIST);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  if (Preferences::GetDefaultType(kTelemetryPref) == PREF_INVALID) {
    bool prerelease = false;
#ifdef MOZ_TELEMETRY_ON_BY_DEFAULT
    prerelease = true;
#else
    if (Preferences::GetDefaultCString(kChannelPref).EqualsLiteral("beta")) {
      prerelease = true;
    }
#endif
    PREF_SetBoolPref(kTelemetryPref, prerelease, true);
  }

  NS_CreateServicesFromCategory(NS_PREFSERVICE_APPDEFAULTS_TOPIC_ID,
                                nullptr, NS_PREFSERVICE_APPDEFAULTS_TOPIC_ID);

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  observerService->NotifyObservers(nullptr, NS_PREFSERVICE_APPDEFAULTS_TOPIC_ID, nullptr);

  return pref_LoadPrefsInDirList(NS_EXT_PREFS_DEFAULTS_DIR_LIST);
}









nsresult
Preferences::GetBool(const char* aPref, bool* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_GetBoolPref(aPref, aResult, false);
}


nsresult
Preferences::GetInt(const char* aPref, int32_t* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_GetIntPref(aPref, aResult, false);
}


nsresult
Preferences::GetFloat(const char* aPref, float* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsAutoCString result;
  nsresult rv = PREF_CopyCharPref(aPref, getter_Copies(result), false);
  if (NS_SUCCEEDED(rv)) {
    *aResult = result.ToFloat(&rv);
  }

  return rv;
}


nsAdoptingCString
Preferences::GetCString(const char* aPref)
{
  nsAdoptingCString result;
  PREF_CopyCharPref(aPref, getter_Copies(result), false);
  return result;
}


nsAdoptingString
Preferences::GetString(const char* aPref)
{
  nsAdoptingString result;
  GetString(aPref, &result);
  return result;
}


nsresult
Preferences::GetCString(const char* aPref, nsACString* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsAutoCString result;
  nsresult rv = PREF_CopyCharPref(aPref, getter_Copies(result), false);
  if (NS_SUCCEEDED(rv)) {
    *aResult = result;
  }
  return rv;
}


nsresult
Preferences::GetString(const char* aPref, nsAString* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsAutoCString result;
  nsresult rv = PREF_CopyCharPref(aPref, getter_Copies(result), false);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(result, *aResult);
  }
  return rv;
}


nsAdoptingCString
Preferences::GetLocalizedCString(const char* aPref)
{
  nsAdoptingCString result;
  GetLocalizedCString(aPref, &result);
  return result;
}


nsAdoptingString
Preferences::GetLocalizedString(const char* aPref)
{
  nsAdoptingString result;
  GetLocalizedString(aPref, &result);
  return result;
}


nsresult
Preferences::GetLocalizedCString(const char* aPref, nsACString* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  nsAutoString result;
  nsresult rv = GetLocalizedString(aPref, &result);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF16toUTF8(result, *aResult);
  }
  return rv;
}


nsresult
Preferences::GetLocalizedString(const char* aPref, nsAString* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsCOMPtr<nsIPrefLocalizedString> prefLocalString;
  nsresult rv = sRootBranch->GetComplexValue(aPref,
                                             NS_GET_IID(nsIPrefLocalizedString),
                                             getter_AddRefs(prefLocalString));
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(prefLocalString, "Succeeded but the result is NULL");
    prefLocalString->GetData(getter_Copies(*aResult));
  }
  return rv;
}


nsresult
Preferences::GetComplex(const char* aPref, const nsIID &aType, void** aResult)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->GetComplexValue(aPref, aType, aResult);
}


nsresult
Preferences::SetCString(const char* aPref, const char* aValue)
{
  ENSURE_MAIN_PROCESS("Cannot SetCString from content process:", aPref);
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_SetCharPref(aPref, aValue, false);
}


nsresult
Preferences::SetCString(const char* aPref, const nsACString &aValue)
{
  ENSURE_MAIN_PROCESS("Cannot SetCString from content process:", aPref);
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_SetCharPref(aPref, PromiseFlatCString(aValue).get(), false);
}


nsresult
Preferences::SetString(const char* aPref, const char16_t* aValue)
{
  ENSURE_MAIN_PROCESS("Cannot SetString from content process:", aPref);
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_SetCharPref(aPref, NS_ConvertUTF16toUTF8(aValue).get(), false);
}


nsresult
Preferences::SetString(const char* aPref, const nsAString &aValue)
{
  ENSURE_MAIN_PROCESS("Cannot SetString from content process:", aPref);
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_SetCharPref(aPref, NS_ConvertUTF16toUTF8(aValue).get(), false);
}


nsresult
Preferences::SetBool(const char* aPref, bool aValue)
{
  ENSURE_MAIN_PROCESS("Cannot SetBool from content process:", aPref);
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_SetBoolPref(aPref, aValue, false);
}


nsresult
Preferences::SetInt(const char* aPref, int32_t aValue)
{
  ENSURE_MAIN_PROCESS("Cannot SetInt from content process:", aPref);
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_SetIntPref(aPref, aValue, false);
}


nsresult
Preferences::SetFloat(const char* aPref, float aValue)
{
  return SetCString(aPref, nsPrintfCString("%f", aValue).get());
}


nsresult
Preferences::SetComplex(const char* aPref, const nsIID &aType,
                        nsISupports* aValue)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->SetComplexValue(aPref, aType, aValue);
}


nsresult
Preferences::ClearUser(const char* aPref)
{
  ENSURE_MAIN_PROCESS("Cannot ClearUser from content process:", aPref);
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_ClearUserPref(aPref);
}


bool
Preferences::HasUserValue(const char* aPref)
{
  NS_ENSURE_TRUE(InitStaticMembers(), false);
  return PREF_HasUserPref(aPref);
}


int32_t
Preferences::GetType(const char* aPref)
{
  NS_ENSURE_TRUE(InitStaticMembers(), nsIPrefBranch::PREF_INVALID);
  int32_t result;
  return NS_SUCCEEDED(sRootBranch->GetPrefType(aPref, &result)) ?
    result : nsIPrefBranch::PREF_INVALID;
}


nsresult
Preferences::AddStrongObserver(nsIObserver* aObserver,
                               const char* aPref)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->AddObserver(aPref, aObserver, false);
}


nsresult
Preferences::AddWeakObserver(nsIObserver* aObserver,
                             const char* aPref)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->AddObserver(aPref, aObserver, true);
}


nsresult
Preferences::RemoveObserver(nsIObserver* aObserver,
                            const char* aPref)
{
  if (!sPreferences && sShutdown) {
    return NS_OK; 
  }
  NS_ENSURE_TRUE(sPreferences, NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->RemoveObserver(aPref, aObserver);
}


nsresult
Preferences::AddStrongObservers(nsIObserver* aObserver,
                                const char** aPrefs)
{
  for (uint32_t i = 0; aPrefs[i]; i++) {
    nsresult rv = AddStrongObserver(aObserver, aPrefs[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


nsresult
Preferences::AddWeakObservers(nsIObserver* aObserver,
                              const char** aPrefs)
{
  for (uint32_t i = 0; aPrefs[i]; i++) {
    nsresult rv = AddWeakObserver(aObserver, aPrefs[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


nsresult
Preferences::RemoveObservers(nsIObserver* aObserver,
                             const char** aPrefs)
{
  if (!sPreferences && sShutdown) {
    return NS_OK; 
  }
  NS_ENSURE_TRUE(sPreferences, NS_ERROR_NOT_AVAILABLE);

  for (uint32_t i = 0; aPrefs[i]; i++) {
    nsresult rv = RemoveObserver(aObserver, aPrefs[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


nsresult
Preferences::RegisterCallback(PrefChangedFunc aCallback,
                              const char* aPref,
                              void* aClosure)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);

  ValueObserverHashKey hashKey(aPref, aCallback);
  nsRefPtr<ValueObserver> observer;
  gObserverTable->Get(&hashKey, getter_AddRefs(observer));
  if (observer) {
    observer->AppendClosure(aClosure);
    return NS_OK;
  }

  observer = new ValueObserver(aPref, aCallback);
  observer->AppendClosure(aClosure);
  nsresult rv = AddStrongObserver(observer, aPref);
  NS_ENSURE_SUCCESS(rv, rv);
  gObserverTable->Put(observer, observer);
  return NS_OK;
}


nsresult
Preferences::RegisterCallbackAndCall(PrefChangedFunc aCallback,
                                     const char* aPref,
                                     void* aClosure)
{
  nsresult rv = RegisterCallback(aCallback, aPref, aClosure);
  if (NS_SUCCEEDED(rv)) {
    (*aCallback)(aPref, aClosure);
  }
  return rv;
}


nsresult
Preferences::UnregisterCallback(PrefChangedFunc aCallback,
                                const char* aPref,
                                void* aClosure)
{
  if (!sPreferences && sShutdown) {
    return NS_OK; 
  }
  NS_ENSURE_TRUE(sPreferences, NS_ERROR_NOT_AVAILABLE);

  ValueObserverHashKey hashKey(aPref, aCallback);
  nsRefPtr<ValueObserver> observer;
  gObserverTable->Get(&hashKey, getter_AddRefs(observer));
  if (!observer) {
    return NS_OK;
  }

  observer->RemoveClosure(aClosure);
  if (observer->HasNoClosures()) {
    
    gObserverTable->Remove(observer);
  }
  return NS_OK;
}

static void BoolVarChanged(const char* aPref, void* aClosure)
{
  CacheData* cache = static_cast<CacheData*>(aClosure);
  *((bool*)cache->cacheLocation) =
    Preferences::GetBool(aPref, cache->defaultValueBool);
}


nsresult
Preferences::AddBoolVarCache(bool* aCache,
                             const char* aPref,
                             bool aDefault)
{
  NS_ASSERTION(aCache, "aCache must not be NULL");
#ifdef DEBUG
  AssertNotAlreadyCached("bool", aPref, aCache);
#endif
  *aCache = GetBool(aPref, aDefault);
  CacheData* data = new CacheData();
  data->cacheLocation = aCache;
  data->defaultValueBool = aDefault;
  gCacheData->AppendElement(data);
  return RegisterCallback(BoolVarChanged, aPref, data);
}

static void IntVarChanged(const char* aPref, void* aClosure)
{
  CacheData* cache = static_cast<CacheData*>(aClosure);
  *((int32_t*)cache->cacheLocation) =
    Preferences::GetInt(aPref, cache->defaultValueInt);
}


nsresult
Preferences::AddIntVarCache(int32_t* aCache,
                            const char* aPref,
                            int32_t aDefault)
{
  NS_ASSERTION(aCache, "aCache must not be NULL");
#ifdef DEBUG
  AssertNotAlreadyCached("int", aPref, aCache);
#endif
  *aCache = Preferences::GetInt(aPref, aDefault);
  CacheData* data = new CacheData();
  data->cacheLocation = aCache;
  data->defaultValueInt = aDefault;
  gCacheData->AppendElement(data);
  return RegisterCallback(IntVarChanged, aPref, data);
}

static void UintVarChanged(const char* aPref, void* aClosure)
{
  CacheData* cache = static_cast<CacheData*>(aClosure);
  *((uint32_t*)cache->cacheLocation) =
    Preferences::GetUint(aPref, cache->defaultValueUint);
}


nsresult
Preferences::AddUintVarCache(uint32_t* aCache,
                             const char* aPref,
                             uint32_t aDefault)
{
  NS_ASSERTION(aCache, "aCache must not be NULL");
#ifdef DEBUG
  AssertNotAlreadyCached("uint", aPref, aCache);
#endif
  *aCache = Preferences::GetUint(aPref, aDefault);
  CacheData* data = new CacheData();
  data->cacheLocation = aCache;
  data->defaultValueUint = aDefault;
  gCacheData->AppendElement(data);
  return RegisterCallback(UintVarChanged, aPref, data);
}

static void FloatVarChanged(const char* aPref, void* aClosure)
{
  CacheData* cache = static_cast<CacheData*>(aClosure);
  *((float*)cache->cacheLocation) =
    Preferences::GetFloat(aPref, cache->defaultValueFloat);
}


nsresult
Preferences::AddFloatVarCache(float* aCache,
                             const char* aPref,
                             float aDefault)
{
  NS_ASSERTION(aCache, "aCache must not be NULL");
#ifdef DEBUG
  AssertNotAlreadyCached("float", aPref, aCache);
#endif
  *aCache = Preferences::GetFloat(aPref, aDefault);
  CacheData* data = new CacheData();
  data->cacheLocation = aCache;
  data->defaultValueFloat = aDefault;
  gCacheData->AppendElement(data);
  return RegisterCallback(FloatVarChanged, aPref, data);
}


nsresult
Preferences::GetDefaultBool(const char* aPref, bool* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_GetBoolPref(aPref, aResult, true);
}


nsresult
Preferences::GetDefaultInt(const char* aPref, int32_t* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return PREF_GetIntPref(aPref, aResult, true);
}


nsresult
Preferences::GetDefaultCString(const char* aPref, nsACString* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsAutoCString result;
  nsresult rv = PREF_CopyCharPref(aPref, getter_Copies(result), true);
  if (NS_SUCCEEDED(rv)) {
    *aResult = result;
  }
  return rv;
}


nsresult
Preferences::GetDefaultString(const char* aPref, nsAString* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsAutoCString result;
  nsresult rv = PREF_CopyCharPref(aPref, getter_Copies(result), true);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(result, *aResult);
  }
  return rv;
}


nsresult
Preferences::GetDefaultLocalizedCString(const char* aPref,
                                        nsACString* aResult)
{
  nsAutoString result;
  nsresult rv = GetDefaultLocalizedString(aPref, &result);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF16toUTF8(result, *aResult);
  }
  return rv;
}


nsresult
Preferences::GetDefaultLocalizedString(const char* aPref,
                                       nsAString* aResult)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsCOMPtr<nsIPrefLocalizedString> prefLocalString;
  nsresult rv =
    sDefaultRootBranch->GetComplexValue(aPref,
                                        NS_GET_IID(nsIPrefLocalizedString),
                                        getter_AddRefs(prefLocalString));
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(prefLocalString, "Succeeded but the result is NULL");
    prefLocalString->GetData(getter_Copies(*aResult));
  }
  return rv;
}


nsAdoptingString
Preferences::GetDefaultString(const char* aPref)
{
  nsAdoptingString result;
  GetDefaultString(aPref, &result);
  return result;
}


nsAdoptingCString
Preferences::GetDefaultCString(const char* aPref)
{
  nsAdoptingCString result;
  PREF_CopyCharPref(aPref, getter_Copies(result), true);
  return result;
}


nsAdoptingString
Preferences::GetDefaultLocalizedString(const char* aPref)
{
  nsAdoptingString result;
  GetDefaultLocalizedString(aPref, &result);
  return result;
}


nsAdoptingCString
Preferences::GetDefaultLocalizedCString(const char* aPref)
{
  nsAdoptingCString result;
  GetDefaultLocalizedCString(aPref, &result);
  return result;
}


nsresult
Preferences::GetDefaultComplex(const char* aPref, const nsIID &aType,
                               void** aResult)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sDefaultRootBranch->GetComplexValue(aPref, aType, aResult);
}


int32_t
Preferences::GetDefaultType(const char* aPref)
{
  NS_ENSURE_TRUE(InitStaticMembers(), nsIPrefBranch::PREF_INVALID);
  int32_t result;
  return NS_SUCCEEDED(sDefaultRootBranch->GetPrefType(aPref, &result)) ?
    result : nsIPrefBranch::PREF_INVALID;
}

} 

#undef ENSURE_MAIN_PROCESS
