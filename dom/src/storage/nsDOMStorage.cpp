









































#include "StorageChild.h"
#include "StorageParent.h"
#include "nsXULAppAPI.h"
using mozilla::dom::StorageChild;

#include "prnetdb.h"
#include "nsCOMPtr.h"
#include "nsDOMError.h"
#include "nsDOMClassInfoID.h"
#include "nsDOMJSUtils.h"
#include "nsUnicharUtils.h"
#include "nsIDocument.h"
#include "nsDOMStorage.h"
#include "nsEscape.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsReadableUtils.h"
#include "nsIObserverService.h"
#include "nsNetUtil.h"
#include "nsIPrefBranch.h"
#include "nsICookiePermission.h"
#include "nsIPermission.h"
#include "nsIPermissionManager.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsIJSContextStack.h"
#include "nsIPrivateBrowsingService.h"
#include "nsDOMString.h"
#include "nsNetCID.h"
#include "mozilla/Preferences.h"
#include "nsThreadUtils.h"

using namespace mozilla;

static const PRUint32 ASK_BEFORE_ACCEPT = 1;
static const PRUint32 ACCEPT_SESSION = 2;
static const PRUint32 BEHAVIOR_REJECT = 2;

static const PRUint32 DEFAULT_QUOTA = 5 * 1024;

static const PRUint32 DEFAULT_OFFLINE_APP_QUOTA = 200 * 1024;

static const PRUint32 DEFAULT_OFFLINE_WARN_QUOTA = 50 * 1024;


#define NS_DOMSTORAGE_MAXIMUM_TEMPTABLE_INACTIVITY_TIME (5)
#define NS_DOMSTORAGE_MAXIMUM_TEMPTABLE_AGE (30)

static const char kPermissionType[] = "cookie";
static const char kStorageEnabled[] = "dom.storage.enabled";
static const char kDefaultQuota[] = "dom.storage.default_quota";
static const char kCookiesBehavior[] = "network.cookie.cookieBehavior";
static const char kCookiesLifetimePolicy[] = "network.cookie.lifetimePolicy";
static const char kOfflineAppWarnQuota[] = "offline-apps.quota.warn";
static const char kOfflineAppQuota[] = "offline-apps.quota.max";



static nsresult
GetPrincipalURIAndHost(nsIPrincipal* aPrincipal, nsIURI** aURI, nsCString& aHost)
{
  nsresult rv = aPrincipal->GetDomain(aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!*aURI) {
    rv = aPrincipal->GetURI(aURI);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!*aURI) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(*aURI);
  if (!innerURI) {
    return NS_ERROR_UNEXPECTED;
  }

  rv = innerURI->GetAsciiHost(aHost);
  if (NS_FAILED(rv)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }
  
  innerURI.swap(*aURI);

  return NS_OK;
}





static bool
IsCallerSecure()
{
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsresult rv = nsContentUtils::GetSecurityManager()->
                  GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  NS_ENSURE_SUCCESS(rv, false);

  if (!subjectPrincipal) {
    
    

    return false;
  }

  nsCOMPtr<nsIURI> codebase;
  subjectPrincipal->GetURI(getter_AddRefs(codebase));

  if (!codebase) {
    return false;
  }

  nsCOMPtr<nsIURI> innerUri = NS_GetInnermostURI(codebase);

  if (!innerUri) {
    return false;
  }

  bool isHttps = false;
  rv = innerUri->SchemeIs("https", &isHttps);

  return NS_SUCCEEDED(rv) && isHttps;
}

PRUint32
GetOfflinePermission(const nsACString &aDomain)
{
  
  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), NS_LITERAL_CSTRING("http://") + aDomain);

  PRUint32 perm;
  if (uri) {
    nsCOMPtr<nsIPermissionManager> permissionManager =
      do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);

    if (permissionManager &&
        NS_SUCCEEDED(permissionManager->TestPermission(uri, "offline-app", &perm)))
        return perm;
  }

  return nsIPermissionManager::UNKNOWN_ACTION;
}

bool
IsOfflineAllowed(const nsACString &aDomain)
{
  PRInt32 perm = GetOfflinePermission(aDomain);
  return IS_PERMISSION_ALLOWED(perm);
}






static PRUint32
GetQuota(const nsACString &aDomain, PRInt32 *aQuota, PRInt32 *aWarnQuota,
         bool aOverrideQuota)
{
  PRUint32 perm = GetOfflinePermission(aDomain);
  if (IS_PERMISSION_ALLOWED(perm) || aOverrideQuota) {
    
    *aQuota = Preferences::GetInt(kOfflineAppQuota,
                                  DEFAULT_OFFLINE_APP_QUOTA) * 1024;

    if (perm == nsIOfflineCacheUpdateService::ALLOW_NO_WARN ||
        aOverrideQuota) {
      *aWarnQuota = -1;
    } else {
      *aWarnQuota = Preferences::GetInt(kOfflineAppWarnQuota,
                                        DEFAULT_OFFLINE_WARN_QUOTA) * 1024;
    }
    return perm;
  }

  
  *aQuota = Preferences::GetInt(kDefaultQuota, DEFAULT_QUOTA) * 1024;
  *aWarnQuota = -1;

  return perm;
}

nsSessionStorageEntry::nsSessionStorageEntry(KeyTypePointer aStr)
  : nsStringHashKey(aStr), mItem(nsnull)
{
}

nsSessionStorageEntry::nsSessionStorageEntry(const nsSessionStorageEntry& aToCopy)
  : nsStringHashKey(aToCopy), mItem(nsnull)
{
  NS_ERROR("We're horked.");
}

nsSessionStorageEntry::~nsSessionStorageEntry()
{
}





nsDOMStorageManager* nsDOMStorageManager::gStorageManager;

nsDOMStorageManager::nsDOMStorageManager()
  : mInPrivateBrowsing(false)
{
}

NS_IMPL_ISUPPORTS2(nsDOMStorageManager,
                   nsIDOMStorageManager,
                   nsIObserver)


nsresult
nsDOMStorageManager::Initialize()
{
  gStorageManager = new nsDOMStorageManager();
  if (!gStorageManager)
    return NS_ERROR_OUT_OF_MEMORY;

  if (!gStorageManager->mStorages.Init()) {
    delete gStorageManager;
    gStorageManager = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(gStorageManager);

  
  if (XRE_GetProcessType() != GeckoProcessType_Default)
    return NS_OK;

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (!os)
    return NS_OK;

  os->AddObserver(gStorageManager, "cookie-changed", false);
  os->AddObserver(gStorageManager, "offline-app-removed", false);
  os->AddObserver(gStorageManager, NS_PRIVATE_BROWSING_SWITCH_TOPIC, false);
  os->AddObserver(gStorageManager, "profile-after-change", false);
  os->AddObserver(gStorageManager, "perm-changed", false);
  os->AddObserver(gStorageManager, "browser:purge-domain-data", false);
  
  os->AddObserver(gStorageManager, "profile-before-change", false);
  os->AddObserver(gStorageManager, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  os->AddObserver(gStorageManager, NS_DOMSTORAGE_FLUSH_TIMER_OBSERVER, false);

  return NS_OK;
}


nsDOMStorageManager*
nsDOMStorageManager::GetInstance()
{
  NS_ASSERTION(gStorageManager,
               "nsDOMStorageManager::GetInstance() called before Initialize()");
  NS_IF_ADDREF(gStorageManager);
  return gStorageManager;
}


void
nsDOMStorageManager::Shutdown()
{
  NS_IF_RELEASE(gStorageManager);
  gStorageManager = nsnull;

  delete DOMStorageImpl::gStorageDB;
  DOMStorageImpl::gStorageDB = nsnull;
}

static PLDHashOperator
ClearStorage(nsDOMStorageEntry* aEntry, void* userArg)
{
  aEntry->mStorage->ClearAll();
  return PL_DHASH_REMOVE;
}

static PLDHashOperator
ClearStorageIfDomainMatches(nsDOMStorageEntry* aEntry, void* userArg)
{
  nsCAutoString* aKey = static_cast<nsCAutoString*> (userArg);
  if (StringBeginsWith(aEntry->mStorage->GetScopeDBKey(), *aKey)) {
    aEntry->mStorage->ClearAll();
  }
  return PL_DHASH_REMOVE;
}

static nsresult
GetOfflineDomains(nsTArray<nsString>& aDomains)
{
  nsCOMPtr<nsIPermissionManager> permissionManager =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  if (permissionManager) {
    nsCOMPtr<nsISimpleEnumerator> enumerator;
    nsresult rv = permissionManager->GetEnumerator(getter_AddRefs(enumerator));
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore;
    while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> supp;
      rv = enumerator->GetNext(getter_AddRefs(supp));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIPermission> perm(do_QueryInterface(supp, &rv));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 capability;
      rv = perm->GetCapability(&capability);
      NS_ENSURE_SUCCESS(rv, rv);
      if (capability != nsIPermissionManager::DENY_ACTION) {
        nsCAutoString type;
        rv = perm->GetType(type);
        NS_ENSURE_SUCCESS(rv, rv);

        if (type.EqualsLiteral("offline-app")) {
          nsCAutoString host;
          rv = perm->GetHost(host);
          NS_ENSURE_SUCCESS(rv, rv);

          aDomains.AppendElement(NS_ConvertUTF8toUTF16(host));
        }
      }
    }
  }

  return NS_OK;
}

nsresult
nsDOMStorageManager::Observe(nsISupports *aSubject,
                             const char *aTopic,
                             const PRUnichar *aData)
{
  if (!strcmp(aTopic, "profile-after-change")) {
    nsCOMPtr<nsIPrivateBrowsingService> pbs =
      do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
    if (pbs)
      pbs->GetPrivateBrowsingEnabled(&gStorageManager->mInPrivateBrowsing);
  }
  else if (!strcmp(aTopic, "offline-app-removed")) {
    nsresult rv = DOMStorageImpl::InitDB();
    NS_ENSURE_SUCCESS(rv, rv);
    return DOMStorageImpl::gStorageDB->RemoveOwner(NS_ConvertUTF16toUTF8(aData),
                                                   true);
  } else if (!strcmp(aTopic, "cookie-changed") &&
             !nsCRT::strcmp(aData, NS_LITERAL_STRING("cleared").get())) {
    mStorages.EnumerateEntries(ClearStorage, nsnull);

    nsresult rv = DOMStorageImpl::InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsTArray<nsString> domains;
    rv = GetOfflineDomains(domains);
    NS_ENSURE_SUCCESS(rv, rv);
    return DOMStorageImpl::gStorageDB->RemoveOwners(domains, true, false);
  } else if (!strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC)) {
    mStorages.EnumerateEntries(ClearStorage, nsnull);
    if (!nsCRT::strcmp(aData, NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).get()))
      mInPrivateBrowsing = true;
    else if (!nsCRT::strcmp(aData, NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).get()))
      mInPrivateBrowsing = false;
    nsresult rv = DOMStorageImpl::InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    return DOMStorageImpl::gStorageDB->DropPrivateBrowsingStorages();
  } else if (!strcmp(aTopic, "perm-changed")) {
    
    nsCOMPtr<nsIPermission> perm(do_QueryInterface(aSubject));
    if (perm) {
      nsCAutoString type;
      perm->GetType(type);
      if (type != NS_LITERAL_CSTRING("cookie"))
        return NS_OK;

      PRUint32 cap = 0;
      perm->GetCapability(&cap);
      if (!(cap & nsICookiePermission::ACCESS_SESSION) ||
          nsDependentString(aData) != NS_LITERAL_STRING("deleted"))
        return NS_OK;

      nsCAutoString host;
      perm->GetHost(host);
      if (host.IsEmpty())
        return NS_OK;

      nsresult rv = DOMStorageImpl::InitDB();
      NS_ENSURE_SUCCESS(rv, rv);

      return DOMStorageImpl::gStorageDB->DropSessionOnlyStoragesForHost(host);
    }
  } else if (!strcmp(aTopic, "timer-callback")) {
    nsCOMPtr<nsIObserverService> obsserv = mozilla::services::GetObserverService();
    if (obsserv)
      obsserv->NotifyObservers(nsnull, NS_DOMSTORAGE_FLUSH_TIMER_OBSERVER, nsnull);
  } else if (!strcmp(aTopic, "browser:purge-domain-data")) {
    
    nsCAutoString aceDomain;
    nsresult rv;
    nsCOMPtr<nsIIDNService> converter = do_GetService(NS_IDNSERVICE_CONTRACTID);
    if (converter) {
      rv = converter->ConvertUTF8toACE(NS_ConvertUTF16toUTF8(aData), aceDomain);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      
      NS_EscapeURL(NS_ConvertUTF16toUTF8(aData),
                   esc_OnlyNonASCII | esc_AlwaysCopy,
                   aceDomain);
    }

    nsCAutoString key;
    rv = nsDOMStorageDBWrapper::CreateDomainScopeDBKey(aceDomain, key);
    NS_ENSURE_SUCCESS(rv, rv);

    
    mStorages.EnumerateEntries(ClearStorageIfDomainMatches, &key);

    rv = DOMStorageImpl::InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    DOMStorageImpl::gStorageDB->RemoveOwner(aceDomain, true);
  } else if (!strcmp(aTopic, "profile-before-change") || 
             !strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    if (DOMStorageImpl::gStorageDB) {
      DebugOnly<nsresult> rv =
        DOMStorageImpl::gStorageDB->FlushAndDeleteTemporaryTables(true);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                       "DOMStorage: temporary table commit failed");
    }
  } else if (!strcmp(aTopic, NS_DOMSTORAGE_FLUSH_TIMER_OBSERVER)) {
    if (DOMStorageImpl::gStorageDB) {
      DebugOnly<nsresult> rv =
        DOMStorageImpl::gStorageDB->FlushAndDeleteTemporaryTables(false);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                       "DOMStorage: temporary table commit failed");
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStorageManager::GetUsage(const nsAString& aDomain,
                              PRInt32 *aUsage)
{
  nsresult rv = DOMStorageImpl::InitDB();
  NS_ENSURE_SUCCESS(rv, rv);

  return DOMStorageImpl::gStorageDB->GetUsage(NS_ConvertUTF16toUTF8(aDomain),
                                              false, aUsage);
}

NS_IMETHODIMP
nsDOMStorageManager::ClearOfflineApps()
{
    nsresult rv = DOMStorageImpl::InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    nsTArray<nsString> domains;
    rv = GetOfflineDomains(domains);
    NS_ENSURE_SUCCESS(rv, rv);
    return DOMStorageImpl::gStorageDB->RemoveOwners(domains, true, true);
}

NS_IMETHODIMP
nsDOMStorageManager::GetLocalStorageForPrincipal(nsIPrincipal *aPrincipal,
                                                 const nsSubstring &aDocumentURI,
                                                 nsIDOMStorage **aResult)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);
  *aResult = nsnull;

  nsresult rv;

  nsRefPtr<nsDOMStorage2> storage = new nsDOMStorage2();
  if (!storage)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = storage->InitAsLocalStorage(aPrincipal, aDocumentURI);
  if (NS_FAILED(rv))
    return rv;

  *aResult = storage.get();
  storage.forget();

  return NS_OK;
}

void
nsDOMStorageManager::AddToStoragesHash(DOMStorageImpl* aStorage)
{
  nsDOMStorageEntry* entry = mStorages.PutEntry(aStorage);
  if (entry)
    entry->mStorage = aStorage;
}

void
nsDOMStorageManager::RemoveFromStoragesHash(DOMStorageImpl* aStorage)
{
  nsDOMStorageEntry* entry = mStorages.GetEntry(aStorage);
  if (entry)
    mStorages.RemoveEntry(aStorage);
}





nsDOMStorageDBWrapper* DOMStorageImpl::gStorageDB = nsnull;

nsDOMStorageEntry::nsDOMStorageEntry(KeyTypePointer aStr)
  : nsVoidPtrHashKey(aStr), mStorage(nsnull)
{
}

nsDOMStorageEntry::nsDOMStorageEntry(const nsDOMStorageEntry& aToCopy)
  : nsVoidPtrHashKey(aToCopy), mStorage(nsnull)
{
  NS_ERROR("DOMStorage horked.");
}

nsDOMStorageEntry::~nsDOMStorageEntry()
{
}

NS_IMPL_CYCLE_COLLECTION_1(nsDOMStorage, mStorageImpl)

DOMCI_DATA(StorageObsolete, nsDOMStorage)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMStorage)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMStorage)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMStorage)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMStorageObsolete)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageObsolete)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMStorage)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StorageObsolete)
NS_INTERFACE_MAP_END

nsresult
NS_NewDOMStorage(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  nsDOMStorage* storage = new nsDOMStorage();
  if (!storage)
    return NS_ERROR_OUT_OF_MEMORY;

  return storage->QueryInterface(aIID, aResult);
}

nsresult
NS_NewDOMStorage2(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  nsDOMStorage2* storage = new nsDOMStorage2();
  if (!storage)
    return NS_ERROR_OUT_OF_MEMORY;

  return storage->QueryInterface(aIID, aResult);
}

DOMStorageBase::DOMStorageBase()
  : mStorageType(nsPIDOMStorage::Unknown)
  , mUseDB(false)
  , mSessionOnly(true)
  , mCanUseChromePersist(false)
{
}

DOMStorageBase::DOMStorageBase(DOMStorageBase& aThat)
  : mStorageType(aThat.mStorageType)
  , mUseDB(false) 
  , mSessionOnly(true)
  , mDomain(aThat.mDomain)
  , mScopeDBKey(aThat.mScopeDBKey)
  , mQuotaETLDplus1DomainDBKey(aThat.mQuotaETLDplus1DomainDBKey)
  , mQuotaDomainDBKey(aThat.mQuotaDomainDBKey)
  , mCanUseChromePersist(aThat.mCanUseChromePersist)
{
}

void
DOMStorageBase::InitAsSessionStorage(nsIURI* aDomainURI)
{
  
  
  
  
  
  aDomainURI->GetAsciiHost(mDomain);

  mUseDB = false;
  mScopeDBKey.Truncate();
  mQuotaDomainDBKey.Truncate();
  mStorageType = nsPIDOMStorage::SessionStorage;
}

void
DOMStorageBase::InitAsLocalStorage(nsIURI* aDomainURI,
                                   bool aCanUseChromePersist)
{
  
  
  
  
  
  
  aDomainURI->GetAsciiHost(mDomain);

  nsDOMStorageDBWrapper::CreateOriginScopeDBKey(aDomainURI, mScopeDBKey);

  
  
  
  
  mUseDB = !mScopeDBKey.IsEmpty();

  nsDOMStorageDBWrapper::CreateQuotaDomainDBKey(mDomain,
                                                true, false, mQuotaDomainDBKey);
  nsDOMStorageDBWrapper::CreateQuotaDomainDBKey(mDomain,
                                                true, true, mQuotaETLDplus1DomainDBKey);
  mCanUseChromePersist = aCanUseChromePersist;
  mStorageType = nsPIDOMStorage::LocalStorage;
}

void
DOMStorageBase::InitAsGlobalStorage(const nsACString& aDomainDemanded)
{
  mDomain = aDomainDemanded;

  nsDOMStorageDBWrapper::CreateDomainScopeDBKey(aDomainDemanded, mScopeDBKey);

  
  
  
  
  if (!(mUseDB = !mScopeDBKey.IsEmpty()))
    mScopeDBKey.AppendLiteral(":");

  nsDOMStorageDBWrapper::CreateQuotaDomainDBKey(aDomainDemanded,
                                                true, false, mQuotaDomainDBKey);
  nsDOMStorageDBWrapper::CreateQuotaDomainDBKey(aDomainDemanded,
                                                true, true, mQuotaETLDplus1DomainDBKey);
  mStorageType = nsPIDOMStorage::GlobalStorage;
}

PLDHashOperator
SessionStorageTraverser(nsSessionStorageEntry* aEntry, void* userArg) {
  nsCycleCollectionTraversalCallback *cb = 
      static_cast<nsCycleCollectionTraversalCallback*>(userArg);

  cb->NoteXPCOMChild((nsIDOMStorageItem *) aEntry->mItem);

  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(DOMStorageImpl)
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(DOMStorageImpl)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMStorageImpl)
{
  if (tmp->mItems.IsInitialized()) {
    tmp->mItems.EnumerateEntries(SessionStorageTraverser, &cb);
  }
}
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMStorageImpl)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMStorageImpl)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMStorageImpl)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

DOMStorageImpl::DOMStorageImpl(nsDOMStorage* aStorage)
{
  Init(aStorage);
}

DOMStorageImpl::DOMStorageImpl(nsDOMStorage* aStorage, DOMStorageImpl& aThat)
  : DOMStorageBase(aThat)
{
  Init(aStorage);
}

void
DOMStorageImpl::Init(nsDOMStorage* aStorage)
{
  mItemsCachedVersion = 0;
  mItems.Init(8);
  mOwner = aStorage;
  if (nsDOMStorageManager::gStorageManager)
    nsDOMStorageManager::gStorageManager->AddToStoragesHash(this);
}

DOMStorageImpl::~DOMStorageImpl()
{
  if (nsDOMStorageManager::gStorageManager)
    nsDOMStorageManager::gStorageManager->RemoveFromStoragesHash(this);
}

nsresult
DOMStorageImpl::InitDB()
{
  if (!gStorageDB) {
    gStorageDB = new nsDOMStorageDBWrapper();
    if (!gStorageDB)
      return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = gStorageDB->Init();
    if (NS_FAILED(rv)) {
      
      
      

      delete gStorageDB;
      gStorageDB = nsnull;

      return rv;
    }
  }

  return NS_OK;
}

void
DOMStorageImpl::InitFromChild(bool aUseDB, bool aCanUseChromePersist,
                              bool aSessionOnly, const nsACString& aDomain,
                              const nsACString& aScopeDBKey,
                              const nsACString& aQuotaDomainDBKey,
                              const nsACString& aQuotaETLDplus1DomainDBKey,
                              PRUint32 aStorageType)
{
  mUseDB = aUseDB;
  mCanUseChromePersist = aCanUseChromePersist;
  mSessionOnly = aSessionOnly;
  mDomain = aDomain;
  mScopeDBKey = aScopeDBKey;
  mQuotaDomainDBKey = aQuotaDomainDBKey;
  mQuotaETLDplus1DomainDBKey = aQuotaETLDplus1DomainDBKey;
  mStorageType = static_cast<nsPIDOMStorage::nsDOMStorageType>(aStorageType);
}

void
DOMStorageImpl::SetSessionOnly(bool aSessionOnly)
{
  mSessionOnly = aSessionOnly;
}

void
DOMStorageImpl::InitAsSessionStorage(nsIURI* aDomainURI)
{
  DOMStorageBase::InitAsSessionStorage(aDomainURI);
}

void
DOMStorageImpl::InitAsLocalStorage(nsIURI* aDomainURI,
                                   bool aCanUseChromePersist)
{
  DOMStorageBase::InitAsLocalStorage(aDomainURI, aCanUseChromePersist);
}

void
DOMStorageImpl::InitAsGlobalStorage(const nsACString& aDomainDemanded)
{
  DOMStorageBase::InitAsGlobalStorage(aDomainDemanded);
}

bool
DOMStorageImpl::CacheStoragePermissions()
{
  
  
  
  if (!mOwner)
    return nsDOMStorage::CanUseStorage(&mSessionOnly);
  
  return mOwner->CacheStoragePermissions();
}

bool
DOMStorageImpl::CanUseChromePersist()
{
  return mCanUseChromePersist;
}

nsresult
DOMStorageImpl::GetCachedValue(const nsAString& aKey, nsAString& aValue,
                               bool* aSecure)
{
  aValue.Truncate();
  *aSecure = false;

  nsSessionStorageEntry *entry = mItems.GetEntry(aKey);
  if (!entry)
    return NS_ERROR_NOT_AVAILABLE;

  aValue = entry->mItem->GetValueInternal();
  *aSecure = entry->mItem->IsSecure();

  return NS_OK;
}

nsresult
DOMStorageImpl::GetDBValue(const nsAString& aKey, nsAString& aValue,
                           bool* aSecure)
{
  aValue.Truncate();

  if (!UseDB())
    return NS_OK;

  nsresult rv = InitDB();
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString value;
  rv = gStorageDB->GetKeyValue(this, aKey, value, aSecure);

  if (rv == NS_ERROR_DOM_NOT_FOUND_ERR &&
      mStorageType != nsPIDOMStorage::GlobalStorage) {
    SetDOMStringToNull(aValue);
  }

  if (NS_FAILED(rv))
    return rv;

  aValue.Assign(value);

  return NS_OK;
}

nsresult
DOMStorageImpl::SetDBValue(const nsAString& aKey,
                           const nsAString& aValue,
                           bool aSecure)
{
  if (!UseDB())
    return NS_OK;

  nsresult rv = InitDB();
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 offlineAppPermission;
  PRInt32 quota;
  PRInt32 warnQuota;
  offlineAppPermission = GetQuota(mDomain, &quota, &warnQuota,
                                  CanUseChromePersist());

  PRInt32 usage;
  rv = gStorageDB->SetKey(this, aKey, aValue, aSecure, quota,
                         !IS_PERMISSION_ALLOWED(offlineAppPermission),
                         &usage);
  NS_ENSURE_SUCCESS(rv, rv);

  if (warnQuota >= 0 && usage > warnQuota) {
    
    nsCOMPtr<nsIDOMWindow> window;
    JSContext *cx;
    nsCOMPtr<nsIJSContextStack> stack =
        do_GetService("@mozilla.org/js/xpc/ContextStack;1");
    if (stack && NS_SUCCEEDED(stack->Peek(&cx)) && cx) {
      nsCOMPtr<nsIScriptContext> scriptContext;
      scriptContext = GetScriptContextFromJSContext(cx);
      if (scriptContext) {
        window = do_QueryInterface(scriptContext->GetGlobalObject());
      }
    }

    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    os->NotifyObservers(window, "dom-storage-warn-quota-exceeded",
                       NS_ConvertUTF8toUTF16(mDomain).get());
  }

  return NS_OK;
}

nsresult
DOMStorageImpl::SetSecure(const nsAString& aKey, bool aSecure)
{
  if (UseDB()) {
    nsresult rv = InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    return gStorageDB->SetSecure(this, aKey, aSecure);
  }

  nsSessionStorageEntry *entry = mItems.GetEntry(aKey);
  NS_ASSERTION(entry, "Don't use SetSecure() with nonexistent keys!");

  if (entry) {
    entry->mItem->SetSecureInternal(aSecure);
  }  

  return NS_OK;
}

static PLDHashOperator
ClearStorageItem(nsSessionStorageEntry* aEntry, void* userArg)
{
  aEntry->mItem->SetValueInternal(EmptyString());
  return PL_DHASH_NEXT;
}

void
DOMStorageImpl::ClearAll()
{
  mItems.EnumerateEntries(ClearStorageItem, nsnull);
  mItemsCachedVersion = 0;
}

struct CopyArgs {
  DOMStorageImpl* storage;
  bool callerSecure;
};

static PLDHashOperator
CopyStorageItems(nsSessionStorageEntry* aEntry, void* userArg)
{
  
  
  
  
  CopyArgs* args = static_cast<CopyArgs*>(userArg);

  nsAutoString unused;
  nsresult rv = args->storage->SetValue(args->callerSecure, aEntry->GetKey(),
                                        aEntry->mItem->GetValueInternal(), unused);
  if (NS_FAILED(rv))
    return PL_DHASH_NEXT;

  if (aEntry->mItem->IsSecure()) {
    args->storage->SetSecure(aEntry->GetKey(), true);
  }

  return PL_DHASH_NEXT;
}

nsresult
DOMStorageImpl::CloneFrom(bool aCallerSecure, DOMStorageBase* aThat)
{
  
  
  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;
  
  DOMStorageImpl* that = static_cast<DOMStorageImpl*>(aThat);
  CopyArgs args = { this, aCallerSecure };
  that->mItems.EnumerateEntries(CopyStorageItems, &args);
  return NS_OK;
}

nsresult
DOMStorageImpl::CacheKeysFromDB()
{
  
  
  
  if (gStorageDB->IsScopeDirty(this)) {
    nsresult rv = InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    mItems.Clear();

    rv = gStorageDB->GetAllKeys(this, &mItems);
    NS_ENSURE_SUCCESS(rv, rv);

    gStorageDB->MarkScopeCached(this);
  }

  return NS_OK;
}

struct KeysArrayBuilderStruct
{
  bool callerIsSecure;
  nsTArray<nsString> *keys;
};

static PLDHashOperator
KeysArrayBuilder(nsSessionStorageEntry* aEntry, void* userArg)
{
  KeysArrayBuilderStruct *keystruct = (KeysArrayBuilderStruct *)userArg;
  
  if (keystruct->callerIsSecure || !aEntry->mItem->IsSecure())
    keystruct->keys->AppendElement(aEntry->GetKey());

  return PL_DHASH_NEXT;
}

nsTArray<nsString>*
DOMStorageImpl::GetKeys(bool aCallerSecure)
{
  if (UseDB())
    CacheKeysFromDB();

  KeysArrayBuilderStruct keystruct;
  keystruct.callerIsSecure = aCallerSecure;
  keystruct.keys = new nsTArray<nsString>();
  if (keystruct.keys)
    mItems.EnumerateEntries(KeysArrayBuilder, &keystruct);
 
  return keystruct.keys;
}

class ItemCounterState
{
 public:
  ItemCounterState(bool aIsCallerSecure)
  : mIsCallerSecure(aIsCallerSecure), mCount(0)
  {
  }

  bool mIsCallerSecure;
  PRUint32 mCount;
 private:
  ItemCounterState(); 
};

static PLDHashOperator
ItemCounter(nsSessionStorageEntry* aEntry, void* userArg)
{
  ItemCounterState *state = (ItemCounterState *)userArg;

  if (state->mIsCallerSecure || !aEntry->mItem->IsSecure()) {
    ++state->mCount;
  }

  return PL_DHASH_NEXT;
}

nsresult
DOMStorageImpl::GetLength(bool aCallerSecure, PRUint32* aLength)
{
  
  
  if (UseDB())
    CacheKeysFromDB();

  ItemCounterState state(aCallerSecure);

  mItems.EnumerateEntries(ItemCounter, &state);

  *aLength = state.mCount;
  return NS_OK;
}

class IndexFinderData
{
 public:
  IndexFinderData(bool aIsCallerSecure, PRUint32 aWantedIndex)
  : mIsCallerSecure(aIsCallerSecure), mIndex(0), mWantedIndex(aWantedIndex),
    mItem(nsnull)
  {
  }

  bool mIsCallerSecure;
  PRUint32 mIndex;
  PRUint32 mWantedIndex;
  nsSessionStorageEntry *mItem;

 private:
  IndexFinderData(); 
};

static PLDHashOperator
IndexFinder(nsSessionStorageEntry* aEntry, void* userArg)
{
  IndexFinderData *data = (IndexFinderData *)userArg;

  if (data->mIndex == data->mWantedIndex &&
      (data->mIsCallerSecure || !aEntry->mItem->IsSecure())) {
    data->mItem = aEntry;

    return PL_DHASH_STOP;
  }

  ++data->mIndex;

  return PL_DHASH_NEXT;
}

nsresult
DOMStorageImpl::GetKey(bool aCallerSecure, PRUint32 aIndex, nsAString& aKey)
{
  
  
  

  
  
  
  

  if (UseDB()) {
    CacheKeysFromDB();
  }

  IndexFinderData data(aCallerSecure, aIndex);
  mItems.EnumerateEntries(IndexFinder, &data);

  if (!data.mItem) {
    
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  aKey = data.mItem->GetKey();
  return NS_OK;
}



nsIDOMStorageItem*
DOMStorageImpl::GetValue(bool aCallerSecure, const nsAString& aKey,
                         nsresult* aResult)
{
  nsSessionStorageEntry *entry = mItems.GetEntry(aKey);
  nsIDOMStorageItem* item = nsnull;
  if (entry) {
    if (aCallerSecure || !entry->mItem->IsSecure()) {
      item = entry->mItem;
    }
  }
  else if (UseDB()) {
    bool secure;
    nsAutoString value;
    nsresult rv = GetDBValue(aKey, value, &secure);
    
    if (rv == NS_ERROR_DOM_SECURITY_ERR || rv == NS_ERROR_DOM_NOT_FOUND_ERR ||
        (!aCallerSecure && secure))
      return nsnull;

    *aResult = rv;
    NS_ENSURE_SUCCESS(rv, nsnull);

    nsRefPtr<nsDOMStorageItem> newitem =
        new nsDOMStorageItem(this, aKey, value, secure);
    if (newitem && (entry = mItems.PutEntry(aKey))) {
      item = entry->mItem = newitem;
    }
    else {
      *aResult = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return item;
}

nsresult
DOMStorageImpl::SetValue(bool aIsCallerSecure, const nsAString& aKey,
                         const nsAString& aData, nsAString& aOldValue)
{
  if (aKey.IsEmpty())
    return NS_OK;

  nsresult rv;
  nsString oldValue;
  SetDOMStringToNull(oldValue);

  
  
  
  if (UseDB()) {
    rv = SetDBValue(aKey, aData, aIsCallerSecure);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsSessionStorageEntry *entry = mItems.GetEntry(aKey);
  if (entry) {
    if (entry->mItem->IsSecure() && !aIsCallerSecure) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }
    oldValue = entry->mItem->GetValueInternal();
    entry->mItem->SetValueInternal(aData);
  }
  else {
    nsRefPtr<nsDOMStorageItem> newitem =
        new nsDOMStorageItem(this, aKey, aData, aIsCallerSecure);
    if (!newitem)
      return NS_ERROR_OUT_OF_MEMORY;
    entry = mItems.PutEntry(aKey);
    NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);
    entry->mItem = newitem;
  }
  aOldValue = oldValue;
  return NS_OK;
}

nsresult
DOMStorageImpl::RemoveValue(bool aCallerSecure, const nsAString& aKey,
                            nsAString& aOldValue)
{
  nsString oldValue;
  nsSessionStorageEntry *entry = mItems.GetEntry(aKey);

  if (entry && entry->mItem->IsSecure() && !aCallerSecure) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (UseDB()) {
    nsresult rv = InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString value;
    bool secureItem;
    rv = GetDBValue(aKey, value, &secureItem);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!aCallerSecure && secureItem)
      return NS_ERROR_DOM_SECURITY_ERR;

    oldValue = value;

    rv = gStorageDB->RemoveKey(this, aKey, !IsOfflineAllowed(mDomain),
                               aKey.Length() + value.Length());
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (entry) {
    
    oldValue = entry->mItem->GetValueInternal();
    entry->mItem->ClearValue();
  }

  if (entry) {
    mItems.RawRemoveEntry(entry);
  }
  aOldValue = oldValue;
  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
CheckSecure(nsSessionStorageEntry* aEntry, void* userArg)
{
  bool* secure = (bool*)userArg;
  if (aEntry->mItem->IsSecure()) {
    *secure = true;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

nsresult
DOMStorageImpl::Clear(bool aCallerSecure, PRInt32* aOldCount)
{
  if (UseDB())
    CacheKeysFromDB();

  PRInt32 oldCount = mItems.Count();

  bool foundSecureItem = false;
  mItems.EnumerateEntries(CheckSecure, &foundSecureItem);

  if (foundSecureItem && !aCallerSecure) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (UseDB()) {
    nsresult rv = InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = gStorageDB->ClearStorage(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aOldCount = oldCount;
  mItems.Clear();
  return NS_OK;
}

nsDOMStorage::nsDOMStorage()
  : mStorageType(nsPIDOMStorage::Unknown)
  , mEventBroadcaster(nsnull)
{
  mSecurityChecker = this;

  if (XRE_GetProcessType() != GeckoProcessType_Default)
    mStorageImpl = new StorageChild(this);
  else
    mStorageImpl = new DOMStorageImpl(this);
}

nsDOMStorage::nsDOMStorage(nsDOMStorage& aThat)
  : mStorageType(aThat.mStorageType)
  , mEventBroadcaster(nsnull)
{
  mSecurityChecker = this;

  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    StorageChild* other = static_cast<StorageChild*>(aThat.mStorageImpl.get());
    mStorageImpl = new StorageChild(this, *other);
  } else {
    DOMStorageImpl* other = static_cast<DOMStorageImpl*>(aThat.mStorageImpl.get());
    mStorageImpl = new DOMStorageImpl(this, *other);
  }
}

nsDOMStorage::~nsDOMStorage()
{
}

static
nsresult
GetDomainURI(nsIPrincipal *aPrincipal, bool aIncludeDomain, nsIURI **_domain)
{
  nsCOMPtr<nsIURI> uri;

  if (aIncludeDomain) {
    nsresult rv = aPrincipal->GetDomain(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!uri) {
    nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if (!uri)
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(uri);
  if (!innerURI)
    return NS_ERROR_UNEXPECTED;
  innerURI.forget(_domain);

  return NS_OK;
}

nsresult
nsDOMStorage::InitAsSessionStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI)
{
  nsCOMPtr<nsIURI> domainURI;
  nsresult rv = GetDomainURI(aPrincipal, true, getter_AddRefs(domainURI));
  NS_ENSURE_SUCCESS(rv, rv);

  mDocumentURI = aDocumentURI;

  mStorageType = SessionStorage;

  mStorageImpl->InitAsSessionStorage(domainURI);
  return NS_OK;
}

nsresult
nsDOMStorage::InitAsLocalStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI)
{
  nsCOMPtr<nsIURI> domainURI;
  nsresult rv = GetDomainURI(aPrincipal, false, getter_AddRefs(domainURI));
  NS_ENSURE_SUCCESS(rv, rv);

  mDocumentURI = aDocumentURI;

  mStorageType = LocalStorage;

  bool canUseChromePersist = false;
  nsCOMPtr<nsIURI> URI;
  if (NS_SUCCEEDED(aPrincipal->GetURI(getter_AddRefs(URI))) && URI) {
    canUseChromePersist = URICanUseChromePersist(URI);
  }
  
  mStorageImpl->InitAsLocalStorage(domainURI, canUseChromePersist);
  return NS_OK;
}

nsresult
nsDOMStorage::InitAsGlobalStorage(const nsACString &aDomainDemanded)
{
  mStorageType = GlobalStorage;
  mEventBroadcaster = this;
  mStorageImpl->InitAsGlobalStorage(aDomainDemanded);
  return NS_OK;
}


bool
nsDOMStorage::CanUseStorage(bool* aSessionOnly)
{
  
  
  NS_ASSERTION(aSessionOnly, "null session flag");
  *aSessionOnly = false;

  if (!Preferences::GetBool(kStorageEnabled)) {
    return false;
  }

  
  if (nsContentUtils::IsCallerChrome())
    return true;

  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsresult rv = nsContentUtils::GetSecurityManager()->
                  GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  NS_ENSURE_SUCCESS(rv, false);

  
  

  nsCOMPtr<nsIURI> subjectURI;
  nsCAutoString unused;
  if (NS_FAILED(GetPrincipalURIAndHost(subjectPrincipal,
                                       getter_AddRefs(subjectURI),
                                       unused))) {
    return false;
  }

  nsCOMPtr<nsIPermissionManager> permissionManager =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  if (!permissionManager)
    return false;

  PRUint32 perm;
  permissionManager->TestPermission(subjectURI, kPermissionType, &perm);

  if (perm == nsIPermissionManager::DENY_ACTION)
    return false;

  
  
  
  if (perm == nsICookiePermission::ACCESS_SESSION ||
      nsDOMStorageManager::gStorageManager->InPrivateBrowsingMode()) {
    *aSessionOnly = true;
  }
  else if (perm != nsIPermissionManager::ALLOW_ACTION) {
    PRUint32 cookieBehavior = Preferences::GetUint(kCookiesBehavior);
    PRUint32 lifetimePolicy = Preferences::GetUint(kCookiesLifetimePolicy);

    
    
    if ((cookieBehavior == BEHAVIOR_REJECT || lifetimePolicy == ASK_BEFORE_ACCEPT) &&
        !URICanUseChromePersist(subjectURI))
      return false;

    if (lifetimePolicy == ACCEPT_SESSION)
      *aSessionOnly = true;
  }

  return true;
}

bool
nsDOMStorage::CacheStoragePermissions()
{
  
  
  
  if (!CanUseStorage(&mStorageImpl->mSessionOnly))
    return false;

  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  if (!ssm)
    return false;

  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsresult rv = ssm->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  NS_ENSURE_SUCCESS(rv, false);

  NS_ASSERTION(mSecurityChecker, "Has non-null mSecurityChecker");
  return mSecurityChecker->CanAccess(subjectPrincipal);
}


bool
nsDOMStorage::URICanUseChromePersist(nsIURI* aURI) {
  bool isAbout;
  return
    (NS_SUCCEEDED(aURI->SchemeIs("moz-safe-about", &isAbout)) && isAbout) ||
    (NS_SUCCEEDED(aURI->SchemeIs("about", &isAbout)) && isAbout);
}

NS_IMETHODIMP
nsDOMStorage::GetLength(PRUint32 *aLength)
{
  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;
  
  return mStorageImpl->GetLength(IsCallerSecure(), aLength);
}

NS_IMETHODIMP
nsDOMStorage::Key(PRUint32 aIndex, nsAString& aKey)
{
  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;

  return mStorageImpl->GetKey(IsCallerSecure(), aIndex, aKey);
}

nsIDOMStorageItem*
nsDOMStorage::GetNamedItem(const nsAString& aKey, nsresult* aResult)
{
  if (!CacheStoragePermissions()) {
    *aResult = NS_ERROR_DOM_SECURITY_ERR;
    return nsnull;
  }

  *aResult = NS_OK;
  if (aKey.IsEmpty())
    return nsnull;
  
  return mStorageImpl->GetValue(IsCallerSecure(), aKey, aResult);
}

nsresult
nsDOMStorage::GetItem(const nsAString& aKey, nsAString &aData)
{
  nsresult rv;

  
  
  
  
  
  

  nsCOMPtr<nsIDOMStorageItem> item;
  rv = GetItem(aKey, getter_AddRefs(item));
  if (NS_FAILED(rv))
    return rv;

  if (item) {
    rv = item->GetValue(aData);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
    SetDOMStringToNull(aData);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStorage::GetItem(const nsAString& aKey, nsIDOMStorageItem **aItem)
{
  nsresult rv;

  NS_IF_ADDREF(*aItem = GetNamedItem(aKey, &rv));

  return rv;
}

NS_IMETHODIMP
nsDOMStorage::SetItem(const nsAString& aKey, const nsAString& aData)
{
  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;

  nsString oldValue;
  nsresult rv = mStorageImpl->SetValue(IsCallerSecure(), aKey, aData, oldValue);
  if (NS_FAILED(rv))
    return rv;

  if ((oldValue != aData || mStorageType == GlobalStorage) && mEventBroadcaster)
    mEventBroadcaster->BroadcastChangeNotification(aKey, oldValue, aData);

  return NS_OK;
}

NS_IMETHODIMP nsDOMStorage::RemoveItem(const nsAString& aKey)
{
  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;

  if (aKey.IsEmpty())
    return NS_OK;

  nsString oldValue;
  nsresult rv = mStorageImpl->RemoveValue(IsCallerSecure(), aKey, oldValue);
  if (rv == NS_ERROR_DOM_NOT_FOUND_ERR)
    return NS_OK;
  if (NS_FAILED(rv))
    return rv;

  if ((!oldValue.IsEmpty() && mStorageType != GlobalStorage) && mEventBroadcaster) {
    nsAutoString nullString;
    SetDOMStringToNull(nullString);
    mEventBroadcaster->BroadcastChangeNotification(aKey, oldValue, nullString);
  }

  return NS_OK;
}

nsresult
nsDOMStorage::Clear()
{
  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;

  PRInt32 oldCount;
  nsresult rv = mStorageImpl->Clear(IsCallerSecure(), &oldCount);
  if (NS_FAILED(rv))
    return rv;
  
  if (oldCount && mEventBroadcaster) {
    nsAutoString nullString;
    SetDOMStringToNull(nullString);
    mEventBroadcaster->BroadcastChangeNotification(nullString, nullString, nullString);
  }

  return NS_OK;
}

already_AddRefed<nsIDOMStorage>
nsDOMStorage::Clone()
{
  NS_ASSERTION(false, "Old DOMStorage doesn't implement cloning");
  return nsnull;
}

already_AddRefed<nsIDOMStorage>
nsDOMStorage::Fork(const nsSubstring &aDocumentURI)
{
  NS_ASSERTION(false, "Old DOMStorage doesn't implement forking");
  return nsnull;
}

bool nsDOMStorage::IsForkOf(nsIDOMStorage* aThat)
{
  NS_ASSERTION(false, "Old DOMStorage doesn't implement forking");
  return false;
}

nsresult
nsDOMStorage::CloneFrom(nsDOMStorage* aThat)
{
  return mStorageImpl->CloneFrom(IsCallerSecure(), aThat->mStorageImpl);
}

nsTArray<nsString> *
nsDOMStorage::GetKeys()
{
  return mStorageImpl->GetKeys(IsCallerSecure());
}

nsIPrincipal*
nsDOMStorage::Principal()
{
  return nsnull;
}

bool
nsDOMStorage::CanAccessSystem(nsIPrincipal *aPrincipal)
{
  if (!aPrincipal)
    return true;

  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  if (!ssm)
    return false;

  bool isSystem;
  nsresult rv = ssm->IsSystemPrincipal(aPrincipal, &isSystem);

  return NS_SUCCEEDED(rv) && isSystem;
}

bool
nsDOMStorage::CanAccess(nsIPrincipal *aPrincipal)
{
  
  if (CanAccessSystem(aPrincipal))
    return true;

  nsCAutoString domain;
  nsCOMPtr<nsIURI> unused;
  nsresult rv = GetPrincipalURIAndHost(aPrincipal,
                                       getter_AddRefs(unused), domain);
  NS_ENSURE_SUCCESS(rv, false);

  return domain.Equals(mStorageImpl->mDomain);
}

nsPIDOMStorage::nsDOMStorageType
nsDOMStorage::StorageType()
{
  return mStorageType;
}

void
nsDOMStorage::BroadcastChangeNotification(const nsSubstring &aKey,
                                          const nsSubstring &aOldValue,
                                          const nsSubstring &aNewValue)
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService) {
    return;
  }

  
  
  
  observerService->NotifyObservers((nsIDOMStorageObsolete *)this,
                                   "dom-storage-changed",
                                   NS_ConvertUTF8toUTF16(mStorageImpl->mDomain).get());
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMStorage2)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMStorage2)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mStorage)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMStorage2)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mStorage, nsIDOMStorageObsolete)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(Storage, nsDOMStorage2)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMStorage2)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMStorage2)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMStorage2)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMStorage)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorage)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMStorage)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Storage)
NS_INTERFACE_MAP_END

nsDOMStorage2::nsDOMStorage2()
{
}

nsDOMStorage2::nsDOMStorage2(nsDOMStorage2& aThat)
{
  mStorage = new nsDOMStorage(*aThat.mStorage.get());
  mStorage->mSecurityChecker = mStorage;
  mPrincipal = aThat.mPrincipal;
}

nsresult
nsDOMStorage2::InitAsSessionStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI)
{
  mStorage = new nsDOMStorage();
  if (!mStorage)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mStorage->mSecurityChecker = mStorage;
  mPrincipal = aPrincipal;
  mDocumentURI = aDocumentURI;

  return mStorage->InitAsSessionStorage(aPrincipal, aDocumentURI);
}

nsresult
nsDOMStorage2::InitAsLocalStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI)
{
  mStorage = new nsDOMStorage();
  if (!mStorage)
    return NS_ERROR_OUT_OF_MEMORY;

  mStorage->mSecurityChecker = this;
  mPrincipal = aPrincipal;
  mDocumentURI = aDocumentURI;

  return mStorage->InitAsLocalStorage(aPrincipal, aDocumentURI);
}

nsresult
nsDOMStorage2::InitAsGlobalStorage(const nsACString &aDomainDemanded)
{
  NS_ASSERTION(false, "Should not initialize nsDOMStorage2 as global storage.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

already_AddRefed<nsIDOMStorage>
nsDOMStorage2::Clone()
{
  nsDOMStorage2* storage = new nsDOMStorage2(*this);
  if (!storage)
    return nsnull;

  storage->mStorage->CloneFrom(mStorage);
  NS_ADDREF(storage);

  return storage;
}

already_AddRefed<nsIDOMStorage>
nsDOMStorage2::Fork(const nsSubstring &aDocumentURI)
{
  nsRefPtr<nsDOMStorage2> storage = new nsDOMStorage2();
  if (!storage)
    return nsnull;

  nsresult rv = storage->InitAsSessionStorageFork(mPrincipal, aDocumentURI, mStorage);
  if (NS_FAILED(rv))
    return nsnull;

  nsIDOMStorage* result = static_cast<nsIDOMStorage*>(storage.get());
  storage.forget();
  return result;
}

bool nsDOMStorage2::IsForkOf(nsIDOMStorage* aThat)
{
  if (!aThat)
    return false;

  nsDOMStorage2* storage = static_cast<nsDOMStorage2*>(aThat);
  return mStorage == storage->mStorage;
}

nsresult
nsDOMStorage2::InitAsSessionStorageFork(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI, nsIDOMStorageObsolete* aStorage)
{
  mPrincipal = aPrincipal;
  mDocumentURI = aDocumentURI;
  mStorage = static_cast<nsDOMStorage*>(aStorage);

  return NS_OK;
}

nsTArray<nsString> *
nsDOMStorage2::GetKeys()
{
  return mStorage->GetKeys();
}

nsIPrincipal*
nsDOMStorage2::Principal()
{
  return mPrincipal;
}

bool
nsDOMStorage2::CanAccess(nsIPrincipal *aPrincipal)
{
  if (mStorage->mSecurityChecker != this)
    return mStorage->mSecurityChecker->CanAccess(aPrincipal);

  
  if (!aPrincipal)
    return true;

  
  bool subsumes;
  nsresult rv = aPrincipal->Subsumes(mPrincipal, &subsumes);
  if (NS_FAILED(rv))
    return false;

  return subsumes;
}

nsPIDOMStorage::nsDOMStorageType
nsDOMStorage2::StorageType()
{
  if (mStorage)
    return mStorage->StorageType();

  return nsPIDOMStorage::Unknown;
}

namespace {

class StorageNotifierRunnable : public nsRunnable
{
public:
  StorageNotifierRunnable(nsISupports* aSubject)
    : mSubject(aSubject)
  { }

  NS_DECL_NSIRUNNABLE

private:
  nsCOMPtr<nsISupports> mSubject;
};

NS_IMETHODIMP
StorageNotifierRunnable::Run()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->NotifyObservers(mSubject, "dom-storage2-changed", nsnull);
  }
  return NS_OK;
}

} 

void
nsDOMStorage2::BroadcastChangeNotification(const nsSubstring &aKey,
                                          const nsSubstring &aOldValue,
                                          const nsSubstring &aNewValue)
{
  nsresult rv;
  nsCOMPtr<nsIDOMStorageEvent> event = new nsDOMStorageEvent();
  rv = event->InitStorageEvent(NS_LITERAL_STRING("storage"),
                               false,
                               false,
                               aKey,
                               aOldValue,
                               aNewValue,
                               mDocumentURI,
                               static_cast<nsIDOMStorage*>(this));
  if (NS_FAILED(rv)) {
    return;
  }

  nsRefPtr<StorageNotifierRunnable> r = new StorageNotifierRunnable(event);
  NS_DispatchToMainThread(r);
}

NS_IMETHODIMP
nsDOMStorage2::GetLength(PRUint32 *aLength)
{
  return mStorage->GetLength(aLength);
}

NS_IMETHODIMP
nsDOMStorage2::Key(PRUint32 aIndex, nsAString& aKey)
{
  return mStorage->Key(aIndex, aKey);
}

NS_IMETHODIMP
nsDOMStorage2::GetItem(const nsAString& aKey, nsAString &aData)
{
  return mStorage->GetItem(aKey, aData);
}

NS_IMETHODIMP
nsDOMStorage2::SetItem(const nsAString& aKey, const nsAString& aData)
{
  mStorage->mEventBroadcaster = this;
  return mStorage->SetItem(aKey, aData);
}

NS_IMETHODIMP
nsDOMStorage2::RemoveItem(const nsAString& aKey)
{
  mStorage->mEventBroadcaster = this;
  return mStorage->RemoveItem(aKey);
}

NS_IMETHODIMP
nsDOMStorage2::Clear()
{
  mStorage->mEventBroadcaster = this;
  return mStorage->Clear();
}





DOMCI_DATA(StorageList, nsDOMStorageList)

NS_INTERFACE_MAP_BEGIN(nsDOMStorageList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageList)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StorageList)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMStorageList)
NS_IMPL_RELEASE(nsDOMStorageList)

nsIDOMStorageObsolete*
nsDOMStorageList::GetNamedItem(const nsAString& aDomain, nsresult* aResult)
{
  nsCAutoString requestedDomain;

  
  nsCOMPtr<nsIIDNService> idn = do_GetService(NS_IDNSERVICE_CONTRACTID);
  if (idn) {
    *aResult = idn->ConvertUTF8toACE(NS_ConvertUTF16toUTF8(aDomain),
                                     requestedDomain);
    NS_ENSURE_SUCCESS(*aResult, nsnull);
  } else {
    
    NS_EscapeURL(NS_ConvertUTF16toUTF8(aDomain),
                 esc_OnlyNonASCII | esc_AlwaysCopy,
                 requestedDomain);
  }
  ToLowerCase(requestedDomain);

  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  if (!ssm) {
    *aResult = NS_ERROR_FAILURE;
    return nsnull;
  }

  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  *aResult = ssm->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  NS_ENSURE_SUCCESS(*aResult, nsnull);

  nsCAutoString currentDomain;
  if (subjectPrincipal) {
    nsCOMPtr<nsIURI> unused;
    *aResult = GetPrincipalURIAndHost(subjectPrincipal, getter_AddRefs(unused),
                                      currentDomain);
    NS_ENSURE_SUCCESS(*aResult, nsnull);

    bool sessionOnly;
    if (!nsDOMStorage::CanUseStorage(&sessionOnly)) {
      *aResult = NS_ERROR_DOM_SECURITY_ERR;
      return nsnull;
    }
  }

  bool isSystem = nsContentUtils::IsCallerTrustedForRead();
  if (currentDomain.IsEmpty() && !isSystem) {
    *aResult = NS_ERROR_DOM_SECURITY_ERR;
    return nsnull;
  }

  return GetStorageForDomain(requestedDomain,
                             currentDomain, isSystem, aResult);
}

NS_IMETHODIMP
nsDOMStorageList::NamedItem(const nsAString& aDomain,
                            nsIDOMStorageObsolete** aStorage)
{
  nsresult rv;
  NS_IF_ADDREF(*aStorage = GetNamedItem(aDomain, &rv));
  return rv;
}


bool
nsDOMStorageList::CanAccessDomain(const nsACString& aRequestedDomain,
                                  const nsACString& aCurrentDomain)
{
  return aRequestedDomain.Equals(aCurrentDomain);
}

nsIDOMStorageObsolete*
nsDOMStorageList::GetStorageForDomain(const nsACString& aRequestedDomain,
                                      const nsACString& aCurrentDomain,
                                      bool aNoCurrentDomainCheck,
                                      nsresult* aResult)
{
  nsTArray<nsCString> requestedDomainArray;
  if ((!aNoCurrentDomainCheck &&
       !CanAccessDomain(aRequestedDomain, aCurrentDomain)) ||
    !ConvertDomainToArray(aRequestedDomain, &requestedDomainArray)) {
    *aResult = NS_ERROR_DOM_SECURITY_ERR;

    return nsnull;
  }

  
  nsCAutoString usedDomain;
  PRUint32 requestedPos = 0;
  for (requestedPos = 0; requestedPos < requestedDomainArray.Length();
       requestedPos++) {
    if (!usedDomain.IsEmpty())
      usedDomain.Append('.');
    usedDomain.Append(requestedDomainArray[requestedPos]);
  }

  *aResult = NS_OK;

  
  nsIDOMStorageObsolete* storage = mStorages.GetWeak(usedDomain);
  if (!storage) {
    nsRefPtr<nsDOMStorage> newstorage;
    newstorage = new nsDOMStorage();
    if (newstorage && mStorages.Put(usedDomain, newstorage)) {
      *aResult = newstorage->InitAsGlobalStorage(usedDomain);
      if (NS_FAILED(*aResult)) {
        mStorages.Remove(usedDomain);
        return nsnull;
      }
      storage = newstorage;
    }
    else {
      *aResult = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return storage;
}


bool
nsDOMStorageList::ConvertDomainToArray(const nsACString& aDomain,
                                       nsTArray<nsCString> *aArray)
{
  PRInt32 length = aDomain.Length();
  PRInt32 n = 0;
  while (n < length) {
    PRInt32 dotpos = aDomain.FindChar('.', n);
    nsCAutoString domain;

    if (dotpos == -1) 
      domain.Assign(Substring(aDomain, n));
    else if (dotpos - n == 0) 
      return false;
    else if (dotpos >= 0)
      domain.Assign(Substring(aDomain, n, dotpos - n));

    ToLowerCase(domain);
    aArray->AppendElement(domain);

    if (dotpos == -1)
      break;

    n = dotpos + 1;
  }

  
  return (n != length);
}

nsresult
NS_NewDOMStorageList(nsIDOMStorageList** aResult)
{
  *aResult = new nsDOMStorageList();
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMStorageItem)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMStorageItem)
  {
    tmp->mStorage = nsnull;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMStorageItem)
  {
    cb.NoteXPCOMChild((nsISupports*) tmp->mStorage);
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMStorageItem)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMStorageItem)

DOMCI_DATA(StorageItem, nsDOMStorageItem)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMStorageItem)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMStorageItem)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageItem)
  NS_INTERFACE_MAP_ENTRY(nsIDOMToString)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StorageItem)
NS_INTERFACE_MAP_END

nsDOMStorageItem::nsDOMStorageItem(DOMStorageBase* aStorage,
                                   const nsAString& aKey,
                                   const nsAString& aValue,
                                   bool aSecure)
  : mSecure(aSecure),
    mKey(aKey),
    mValue(aValue),
    mStorage(aStorage)
{
}

nsDOMStorageItem::~nsDOMStorageItem()
{
}

NS_IMETHODIMP
nsDOMStorageItem::GetSecure(bool* aSecure)
{
  if (!mStorage->CacheStoragePermissions() || !IsCallerSecure()) {
    return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }

  if (mStorage->UseDB()) {
    nsAutoString value;
    return mStorage->GetDBValue(mKey, value, aSecure);
  }

  *aSecure = IsSecure();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMStorageItem::SetSecure(bool aSecure)
{
  if (!mStorage->CacheStoragePermissions() || !IsCallerSecure()) {
    return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }

  if (mStorage->UseDB()) {
    nsresult rv = mStorage->SetSecure(mKey, aSecure);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mSecure = aSecure;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMStorageItem::GetValue(nsAString& aValue)
{
  if (!mStorage->CacheStoragePermissions())
    return NS_ERROR_DOM_INVALID_ACCESS_ERR;

  if (mStorage->UseDB()) {
    bool secure;
    nsresult rv = mStorage->GetDBValue(mKey, aValue, &secure);
    if (rv == NS_ERROR_DOM_NOT_FOUND_ERR)
      return NS_OK;
    if (NS_SUCCEEDED(rv) && !IsCallerSecure() && secure)
      return NS_ERROR_DOM_SECURITY_ERR;
    return rv;
  }

  if (IsSecure() && !IsCallerSecure()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  aValue = mValue;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMStorageItem::SetValue(const nsAString& aValue)
{
  if (!mStorage->CacheStoragePermissions())
    return NS_ERROR_DOM_INVALID_ACCESS_ERR;

  bool secureCaller = IsCallerSecure();

  if (mStorage->UseDB()) {
    
    return mStorage->SetDBValue(mKey, aValue, secureCaller);
  }

  bool secureItem = IsSecure();

  if (!secureCaller && secureItem) {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  mValue = aValue;
  mSecure = secureCaller;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMStorageItem::ToString(nsAString& aStr)
{
  return GetValue(aStr);
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMStorageEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMStorageEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mStorageArea)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMStorageEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mStorageArea)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsDOMStorageEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMStorageEvent, nsDOMEvent)

DOMCI_DATA(StorageEvent, nsDOMStorageEvent)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMStorageEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StorageEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)



NS_IMETHODIMP nsDOMStorageEvent::GetKey(nsAString & aKey)
{
  aKey = mKey;
  return NS_OK;
}


NS_IMETHODIMP nsDOMStorageEvent::GetOldValue(nsAString & aOldValue)
{
  aOldValue = mOldValue;
  return NS_OK;
}


NS_IMETHODIMP nsDOMStorageEvent::GetNewValue(nsAString & aNewValue)
{
  aNewValue = mNewValue;
  return NS_OK;
}


NS_IMETHODIMP nsDOMStorageEvent::GetUrl(nsAString & aUrl)
{
  aUrl = mUrl;
  return NS_OK;
}


NS_IMETHODIMP nsDOMStorageEvent::GetStorageArea(nsIDOMStorage * *aStorageArea)
{
  NS_ENSURE_ARG_POINTER(aStorageArea);

  NS_ADDREF(*aStorageArea = mStorageArea);
  return NS_OK;
}


NS_IMETHODIMP nsDOMStorageEvent::InitStorageEvent(const nsAString & typeArg,
                                                  bool canBubbleArg,
                                                  bool cancelableArg,
                                                  const nsAString & keyArg,
                                                  const nsAString & oldValueArg,
                                                  const nsAString & newValueArg,
                                                  const nsAString & urlArg,
                                                  nsIDOMStorage *storageAreaArg)
{
  nsresult rv;

  rv = InitEvent(typeArg, canBubbleArg, cancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mKey = keyArg;
  mOldValue = oldValueArg;
  mNewValue = newValueArg;
  mUrl = urlArg;
  mStorageArea = storageAreaArg;

  return NS_OK;
}



DOMCI_DATA(StorageEventObsolete, nsDOMStorageEventObsolete)


NS_INTERFACE_MAP_BEGIN(nsDOMStorageEventObsolete)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageEventObsolete)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StorageEventObsolete)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMStorageEventObsolete, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMStorageEventObsolete, nsDOMEvent)


NS_IMETHODIMP
nsDOMStorageEventObsolete::GetDomain(nsAString& aDomain)
{
  
  
  aDomain = mDomain;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStorageEventObsolete::InitStorageEvent(const nsAString& aTypeArg,
                                    bool aCanBubbleArg,
                                    bool aCancelableArg,
                                    const nsAString& aDomainArg)
{
  nsresult rv = InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mDomain = aDomainArg;

  return NS_OK;
}
