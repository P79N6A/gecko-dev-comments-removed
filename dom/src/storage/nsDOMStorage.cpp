








































#include "prnetdb.h"
#include "nsCOMPtr.h"
#include "nsDOMError.h"
#include "nsDOMClassInfo.h"
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

static const PRUint32 ASK_BEFORE_ACCEPT = 1;
static const PRUint32 ACCEPT_SESSION = 2;
static const PRUint32 BEHAVIOR_REJECT = 2;

static const PRUint32 DEFAULT_QUOTA = 5 * 1024;

static const PRUint32 DEFAULT_OFFLINE_APP_QUOTA = 200 * 1024;

static const PRUint32 DEFAULT_OFFLINE_WARN_QUOTA = 50 * 1024;

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





static PRBool
IsCallerSecure()
{
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsContentUtils::GetSecurityManager()->
    GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));

  if (!subjectPrincipal) {
    
    

    return PR_FALSE;
  }

  nsCOMPtr<nsIURI> codebase;
  subjectPrincipal->GetURI(getter_AddRefs(codebase));

  if (!codebase) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIURI> innerUri = NS_GetInnermostURI(codebase);

  if (!innerUri) {
    return PR_FALSE;
  }

  PRBool isHttps = PR_FALSE;
  nsresult rv = innerUri->SchemeIs("https", &isHttps);

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

PRBool
IsOfflineAllowed(const nsACString &aDomain)
{
  PRInt32 perm = GetOfflinePermission(aDomain);
  return IS_PERMISSION_ALLOWED(perm);
}




static PRUint32
GetQuota(const nsACString &aDomain, PRInt32 *aQuota, PRInt32 *aWarnQuota)
{
  PRUint32 perm = GetOfflinePermission(aDomain);
  if (IS_PERMISSION_ALLOWED(perm)) {
    
    *aQuota = ((PRInt32)nsContentUtils::GetIntPref(kOfflineAppQuota,
                                                   DEFAULT_OFFLINE_APP_QUOTA) * 1024);

    if (perm == nsIOfflineCacheUpdateService::ALLOW_NO_WARN) {
      *aWarnQuota = -1;
    } else {
      *aWarnQuota = ((PRInt32)nsContentUtils::GetIntPref(kOfflineAppWarnQuota,
                                                         DEFAULT_OFFLINE_WARN_QUOTA) * 1024);
    }
    return perm;
  }

  
  *aQuota = ((PRInt32)nsContentUtils::GetIntPref(kDefaultQuota,
                                                 DEFAULT_QUOTA) * 1024);
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
  : mInPrivateBrowsing(PR_FALSE)
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

  nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");
  if (os) {
    os->AddObserver(gStorageManager, "cookie-changed", PR_FALSE);
    os->AddObserver(gStorageManager, "offline-app-removed", PR_FALSE);
    os->AddObserver(gStorageManager, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_FALSE);
    os->AddObserver(gStorageManager, "perm-changed", PR_FALSE);

    nsCOMPtr<nsIPrivateBrowsingService> pbs =
      do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
    if (pbs)
      pbs->GetPrivateBrowsingEnabled(&gStorageManager->mInPrivateBrowsing);
  }

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

#ifdef MOZ_STORAGE
  delete nsDOMStorage::gStorageDB;
  nsDOMStorage::gStorageDB = nsnull;
#endif
}

static PLDHashOperator
ClearStorage(nsDOMStorageEntry* aEntry, void* userArg)
{
  aEntry->mStorage->ClearAll();
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

    PRBool hasMore;
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
  if (!strcmp(aTopic, "offline-app-removed")) {
#ifdef MOZ_STORAGE
    nsresult rv = nsDOMStorage::InitDB();
    NS_ENSURE_SUCCESS(rv, rv);
    return nsDOMStorage::gStorageDB->RemoveOwner(NS_ConvertUTF16toUTF8(aData),
                                                 PR_TRUE);
#endif
  } else if (!strcmp(aTopic, "cookie-changed") &&
             !nsCRT::strcmp(aData, NS_LITERAL_STRING("cleared").get())) {
    mStorages.EnumerateEntries(ClearStorage, nsnull);

#ifdef MOZ_STORAGE
    nsresult rv = nsDOMStorage::InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsTArray<nsString> domains;
    rv = GetOfflineDomains(domains);
    NS_ENSURE_SUCCESS(rv, rv);
    return nsDOMStorage::gStorageDB->RemoveOwners(domains, PR_TRUE, PR_FALSE);
#endif
  } else if (!strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC)) {
    mStorages.EnumerateEntries(ClearStorage, nsnull);
    if (!nsCRT::strcmp(aData, NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).get()))
      mInPrivateBrowsing = PR_TRUE;
    else if (!nsCRT::strcmp(aData, NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).get()))
      mInPrivateBrowsing = PR_FALSE;
#ifdef MOZ_STORAGE
    nsresult rv = nsDOMStorage::InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    return nsDOMStorage::gStorageDB->DropPrivateBrowsingStorages();
#endif
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

#ifdef MOZ_STORAGE
      nsresult rv = nsDOMStorage::InitDB();
      NS_ENSURE_SUCCESS(rv, rv);

      return nsDOMStorage::gStorageDB->DropSessionOnlyStoragesForHost(host);
#endif
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStorageManager::GetUsage(const nsAString& aDomain,
                              PRInt32 *aUsage)
{
  nsresult rv = nsDOMStorage::InitDB();
  NS_ENSURE_SUCCESS(rv, rv);

  return nsDOMStorage::gStorageDB->GetUsage(NS_ConvertUTF16toUTF8(aDomain),
                                            PR_FALSE, aUsage);
}

NS_IMETHODIMP
nsDOMStorageManager::ClearOfflineApps()
{
    nsresult rv = nsDOMStorage::InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    nsTArray<nsString> domains;
    rv = GetOfflineDomains(domains);
    NS_ENSURE_SUCCESS(rv, rv);
    return nsDOMStorage::gStorageDB->RemoveOwners(domains, PR_TRUE, PR_TRUE);
}

NS_IMETHODIMP
nsDOMStorageManager::GetLocalStorageForPrincipal(nsIPrincipal *aPrincipal,
                                                 nsIDOMStorage **aResult)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);
  *aResult = nsnull;

  nsresult rv;

  nsRefPtr<nsDOMStorage2> storage = new nsDOMStorage2();
  if (!storage)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = storage->InitAsLocalStorage(aPrincipal);
  if (NS_FAILED(rv))
    return rv;

  *aResult = storage.get();
  storage.forget();

  return NS_OK;
}

void
nsDOMStorageManager::AddToStoragesHash(nsDOMStorage* aStorage)
{
  nsDOMStorageEntry* entry = mStorages.PutEntry(aStorage);
  if (entry)
    entry->mStorage = aStorage;
}

void
nsDOMStorageManager::RemoveFromStoragesHash(nsDOMStorage* aStorage)
{
  nsDOMStorageEntry* entry = mStorages.GetEntry(aStorage);
  if (entry)
    mStorages.RemoveEntry(aStorage);
}





#ifdef MOZ_STORAGE
nsDOMStorageDBWrapper* nsDOMStorage::gStorageDB = nsnull;
#endif

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

PLDHashOperator
SessionStorageTraverser(nsSessionStorageEntry* aEntry, void* userArg) {
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(userArg);

  cb->NoteXPCOMChild((nsIDOMStorageItem *) aEntry->mItem);

  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMStorage)
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsDOMStorage)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMStorage)
  {
    if (tmp->mItems.IsInitialized()) {
      tmp->mItems.EnumerateEntries(SessionStorageTraverser, &cb);
    }
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsDOMStorage, nsIDOMStorageObsolete)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsDOMStorage, nsIDOMStorageObsolete)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMStorage)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMStorageObsolete)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageObsolete)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMStorage)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StorageObsolete)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
NS_NewDOMStorage(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  nsDOMStorage* storage = new nsDOMStorage();
  if (!storage)
    return NS_ERROR_OUT_OF_MEMORY;

  return storage->QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
NS_NewDOMStorage2(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  nsDOMStorage2* storage = new nsDOMStorage2();
  if (!storage)
    return NS_ERROR_OUT_OF_MEMORY;

  return storage->QueryInterface(aIID, aResult);
}

nsDOMStorage::nsDOMStorage()
  : mUseDB(PR_FALSE)
  , mSessionOnly(PR_TRUE)
  , mLocalStorage(PR_FALSE)
  , mItemsCached(PR_FALSE)
{
  mSecurityChecker = this;
  mItems.Init(8);
  if (nsDOMStorageManager::gStorageManager)
    nsDOMStorageManager::gStorageManager->AddToStoragesHash(this);
}

nsDOMStorage::nsDOMStorage(nsDOMStorage& aThat)
  : mUseDB(PR_FALSE) 
  , mSessionOnly(PR_TRUE)
  , mLocalStorage(PR_FALSE) 
  , mItemsCached(PR_FALSE)
  , mDomain(aThat.mDomain)
#ifdef MOZ_STORAGE
  , mScopeDBKey(aThat.mScopeDBKey)
#endif
{
  mSecurityChecker = this;
  mItems.Init(8);

  if (nsDOMStorageManager::gStorageManager)
    nsDOMStorageManager::gStorageManager->AddToStoragesHash(this);
}

nsDOMStorage::~nsDOMStorage()
{
  if (nsDOMStorageManager::gStorageManager)
    nsDOMStorageManager::gStorageManager->RemoveFromStoragesHash(this);
}

static
nsresult
GetDomainURI(nsIPrincipal *aPrincipal, PRBool aIncludeDomain, nsIURI **_domain)
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
nsDOMStorage::InitAsSessionStorage(nsIPrincipal *aPrincipal)
{
  nsCOMPtr<nsIURI> domainURI;
  nsresult rv = GetDomainURI(aPrincipal, PR_TRUE, getter_AddRefs(domainURI));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  domainURI->GetAsciiHost(mDomain);

#ifdef MOZ_STORAGE
  mUseDB = PR_FALSE;
  mScopeDBKey.Truncate();
  mQuotaDomainDBKey.Truncate();
#endif
  return NS_OK;
}

nsresult
nsDOMStorage::InitAsLocalStorage(nsIPrincipal *aPrincipal)
{
  nsCOMPtr<nsIURI> domainURI;
  nsresult rv = GetDomainURI(aPrincipal, PR_FALSE, getter_AddRefs(domainURI));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  domainURI->GetAsciiHost(mDomain);

#ifdef MOZ_STORAGE
  nsDOMStorageDBWrapper::CreateOriginScopeDBKey(domainURI, mScopeDBKey);

  
  
  
  
  mUseDB = !mScopeDBKey.IsEmpty();

  nsDOMStorageDBWrapper::CreateQuotaDomainDBKey(mDomain,
      PR_TRUE, PR_FALSE, mQuotaDomainDBKey);
  nsDOMStorageDBWrapper::CreateQuotaDomainDBKey(mDomain,
      PR_TRUE, PR_TRUE, mQuotaETLDplus1DomainDBKey);
#endif

  mLocalStorage = PR_TRUE;
  return NS_OK;
}

nsresult
nsDOMStorage::InitAsGlobalStorage(const nsACString &aDomainDemanded)
{
  mDomain = aDomainDemanded;
#ifdef MOZ_STORAGE
  nsDOMStorageDBWrapper::CreateDomainScopeDBKey(aDomainDemanded, mScopeDBKey);

  
  
  
  
  if (!(mUseDB = !mScopeDBKey.IsEmpty()))
    mScopeDBKey.AppendLiteral(":");

  nsDOMStorageDBWrapper::CreateQuotaDomainDBKey(aDomainDemanded,
      PR_TRUE, PR_FALSE, mQuotaDomainDBKey);
  nsDOMStorageDBWrapper::CreateQuotaDomainDBKey(aDomainDemanded,
      PR_TRUE, PR_TRUE, mQuotaETLDplus1DomainDBKey);
#endif
  return NS_OK;
}

static PLDHashOperator
CopyStorageItems(nsSessionStorageEntry* aEntry, void* userArg)
{
  nsDOMStorage* newstorage = static_cast<nsDOMStorage*>(userArg);

  newstorage->SetItem(aEntry->GetKey(), aEntry->mItem->GetValueInternal());

  if (aEntry->mItem->IsSecure()) {
    newstorage->SetSecure(aEntry->GetKey(), PR_TRUE);
  }

  return PL_DHASH_NEXT;
}

nsresult
nsDOMStorage::CloneFrom(nsDOMStorage* aThat)
{
  aThat->mItems.EnumerateEntries(CopyStorageItems, this);
  return NS_OK;
}


PRBool
nsDOMStorage::CanUseStorage(PRPackedBool* aSessionOnly)
{
  
  
  NS_ASSERTION(aSessionOnly, "null session flag");
  *aSessionOnly = PR_FALSE;

  if (!nsContentUtils::GetBoolPref(kStorageEnabled))
    return PR_FALSE;

  
  if (nsContentUtils::IsCallerChrome())
    return PR_TRUE;

  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsContentUtils::GetSecurityManager()->
    GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));

  
  

  nsCOMPtr<nsIURI> subjectURI;
  nsCAutoString unused;
  if (NS_FAILED(GetPrincipalURIAndHost(subjectPrincipal,
                                       getter_AddRefs(subjectURI),
                                       unused))) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIPermissionManager> permissionManager =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  if (!permissionManager)
    return PR_FALSE;

  PRUint32 perm;
  permissionManager->TestPermission(subjectURI, kPermissionType, &perm);

  if (perm == nsIPermissionManager::DENY_ACTION)
    return PR_FALSE;

  
  
  
  if (perm == nsICookiePermission::ACCESS_SESSION ||
      nsDOMStorageManager::gStorageManager->InPrivateBrowsingMode()) {
    *aSessionOnly = PR_TRUE;
  }
  else if (perm != nsIPermissionManager::ALLOW_ACTION) {
    PRUint32 cookieBehavior = nsContentUtils::GetIntPref(kCookiesBehavior);
    PRUint32 lifetimePolicy = nsContentUtils::GetIntPref(kCookiesLifetimePolicy);

    
    if (cookieBehavior == BEHAVIOR_REJECT || lifetimePolicy == ASK_BEFORE_ACCEPT)
      return PR_FALSE;

    if (lifetimePolicy == ACCEPT_SESSION)
      *aSessionOnly = PR_TRUE;
  }

  return PR_TRUE;
}

PRBool
nsDOMStorage::CacheStoragePermissions()
{
  
  
  
  if (!CanUseStorage(&mSessionOnly))
    return PR_FALSE;

  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  if (!ssm)
    return PR_FALSE;

  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  ssm->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));

  NS_ASSERTION(mSecurityChecker, "Has non-null mSecurityChecker");
  return mSecurityChecker->CanAccess(subjectPrincipal);
}


class ItemCounterState
{
public:
  ItemCounterState(PRBool aIsCallerSecure)
    : mIsCallerSecure(aIsCallerSecure), mCount(0)
  {
  }

  PRBool mIsCallerSecure;
  PRBool mCount;
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

NS_IMETHODIMP
nsDOMStorage::GetLength(PRUint32 *aLength)
{
  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;

  
  
  mItemsCached = PR_FALSE;
  if (UseDB())
    CacheKeysFromDB();

  ItemCounterState state(IsCallerSecure());

  mItems.EnumerateEntries(ItemCounter, &state);

  *aLength = state.mCount;

  return NS_OK;
}

class IndexFinderData
{
public:
  IndexFinderData(PRBool aIsCallerSecure, PRUint32 aWantedIndex)
    : mIsCallerSecure(aIsCallerSecure), mIndex(0), mWantedIndex(aWantedIndex),
      mItem(nsnull)
  {
  }

  PRBool mIsCallerSecure;
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

NS_IMETHODIMP
nsDOMStorage::Key(PRUint32 aIndex, nsAString& aKey)
{
  
  
  

  
  
  
  

  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;

  if (UseDB())
    CacheKeysFromDB();

  IndexFinderData data(IsCallerSecure(), aIndex);
  mItems.EnumerateEntries(IndexFinder, &data);

  if (!data.mItem) {
    
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  aKey = data.mItem->GetKey();

  return NS_OK;
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

  nsSessionStorageEntry *entry = mItems.GetEntry(aKey);
  nsIDOMStorageItem* item = nsnull;
  if (entry) {
    if (IsCallerSecure() || !entry->mItem->IsSecure()) {
      item = entry->mItem;
    }
  }
  else if (UseDB()) {
    PRBool secure;
    nsAutoString value;
    nsresult rv = GetDBValue(aKey, value, &secure);
    
    if (rv == NS_ERROR_DOM_SECURITY_ERR || rv == NS_ERROR_DOM_NOT_FOUND_ERR)
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

  if (aKey.IsEmpty())
    return NS_OK;

  nsresult rv;
  nsRefPtr<nsDOMStorageItem> newitem = nsnull;
  nsSessionStorageEntry *entry = mItems.GetEntry(aKey);
  if (entry) {
    if (entry->mItem->IsSecure() && !IsCallerSecure()) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }
    if (!UseDB()) {
      entry->mItem->SetValueInternal(aData);
    }
  }
  else {
    newitem = new nsDOMStorageItem(this, aKey, aData, IsCallerSecure());
    if (!newitem)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (UseDB()) {
    rv = SetDBValue(aKey, aData, IsCallerSecure());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (newitem) {
    entry = mItems.PutEntry(aKey);
    NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);
    entry->mItem = newitem;
  }

  
  if (!UseDB())
    BroadcastChangeNotification();

  return NS_OK;
}

NS_IMETHODIMP nsDOMStorage::RemoveItem(const nsAString& aKey)
{
  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;

  if (aKey.IsEmpty())
    return NS_OK;

  nsSessionStorageEntry *entry = mItems.GetEntry(aKey);

  if (entry && entry->mItem->IsSecure() && !IsCallerSecure()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (UseDB()) {
#ifdef MOZ_STORAGE
    nsresult rv = InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString value;
    PRBool secureItem;
    rv = GetDBValue(aKey, value, &secureItem);
    if (rv == NS_ERROR_DOM_NOT_FOUND_ERR)
      return NS_OK;
    NS_ENSURE_SUCCESS(rv, rv);

    rv = gStorageDB->RemoveKey(this, aKey, !IsOfflineAllowed(mDomain),
                               aKey.Length() + value.Length());
    NS_ENSURE_SUCCESS(rv, rv);

    mItemsCached = PR_FALSE;

    BroadcastChangeNotification();
#endif
  }
  else if (entry) {
    
    entry->mItem->ClearValue();

    BroadcastChangeNotification();
  }

  if (entry) {
    mItems.RawRemoveEntry(entry);
  }

  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
CheckSecure(nsSessionStorageEntry* aEntry, void* userArg)
{
  PRBool* secure = (PRBool*)userArg;
  if (aEntry->mItem->IsSecure()) {
    *secure = PR_TRUE;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

nsresult
nsDOMStorage::Clear()
{
  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;

  if (UseDB())
    CacheKeysFromDB();

  PRBool foundSecureItem = PR_FALSE;
  mItems.EnumerateEntries(CheckSecure, &foundSecureItem);

  if (foundSecureItem && !IsCallerSecure()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

#ifdef MOZ_STORAGE
  if (UseDB()) {
    nsresult rv = InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = gStorageDB->ClearStorage(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }
#endif

  mItems.Clear();
  BroadcastChangeNotification();

  return NS_OK;
}

nsresult
nsDOMStorage::InitDB()
{
#ifdef MOZ_STORAGE
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
#endif

  return NS_OK;
}

nsresult
nsDOMStorage::CacheKeysFromDB()
{
#ifdef MOZ_STORAGE
  
  
  
  if (!mItemsCached) {
    nsresult rv = InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    mItems.Clear();

    rv = gStorageDB->GetAllKeys(this, &mItems);
    NS_ENSURE_SUCCESS(rv, rv);

    mItemsCached = PR_TRUE;
  }
#endif

  return NS_OK;
}

nsresult
nsDOMStorage::GetDBValue(const nsAString& aKey, nsAString& aValue,
                         PRBool* aSecure)
{
  aValue.Truncate();

#ifdef MOZ_STORAGE
  if (!UseDB())
    return NS_OK;

  nsresult rv = InitDB();
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString value;
  rv = gStorageDB->GetKeyValue(this, aKey, value, aSecure);

  if (rv == NS_ERROR_DOM_NOT_FOUND_ERR && mLocalStorage) {
    SetDOMStringToNull(aValue);
  }

  if (NS_FAILED(rv))
    return rv;

  if (!IsCallerSecure() && *aSecure) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  aValue.Assign(value);
#endif

  return NS_OK;
}

nsresult
nsDOMStorage::SetDBValue(const nsAString& aKey,
                         const nsAString& aValue,
                         PRBool aSecure)
{
#ifdef MOZ_STORAGE
  if (!UseDB())
    return NS_OK;

  nsresult rv = InitDB();
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 offlineAppPermission;
  PRInt32 quota;
  PRInt32 warnQuota;
  offlineAppPermission = GetQuota(mDomain, &quota, &warnQuota);

  PRInt32 usage;
  rv = gStorageDB->SetKey(this, aKey, aValue, aSecure, quota,
                          !IS_PERMISSION_ALLOWED(offlineAppPermission),
                          &usage);
  NS_ENSURE_SUCCESS(rv, rv);

  mItemsCached = PR_FALSE;

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

    nsCOMPtr<nsIObserverService> os =
      do_GetService("@mozilla.org/observer-service;1");
    os->NotifyObservers(window, "dom-storage-warn-quota-exceeded",
                        NS_ConvertUTF8toUTF16(mDomain).get());
  }

  BroadcastChangeNotification();
#endif

  return NS_OK;
}

nsresult
nsDOMStorage::SetSecure(const nsAString& aKey, PRBool aSecure)
{
#ifdef MOZ_STORAGE
  if (UseDB()) {
    nsresult rv = InitDB();
    NS_ENSURE_SUCCESS(rv, rv);

    return gStorageDB->SetSecure(this, aKey, aSecure);
  }
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif

  nsSessionStorageEntry *entry = mItems.GetEntry(aKey);
  NS_ASSERTION(entry, "Don't use SetSecure() with non-existing keys!");

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
nsDOMStorage::ClearAll()
{
  mItems.EnumerateEntries(ClearStorageItem, nsnull);
  mItemsCached = PR_FALSE;
}

already_AddRefed<nsIDOMStorage>
nsDOMStorage::Clone()
{
  NS_ASSERTION(PR_FALSE, "Old DOMStorage doesn't implement cloning");
  return nsnull;
}

struct KeysArrayBuilderStruct
{
  PRBool callerIsSecure;
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

nsTArray<nsString> *
nsDOMStorage::GetKeys()
{
  if (UseDB())
    CacheKeysFromDB();

  KeysArrayBuilderStruct keystruct;
  keystruct.callerIsSecure = IsCallerSecure();
  keystruct.keys = new nsTArray<nsString>();
  if (keystruct.keys)
    mItems.EnumerateEntries(KeysArrayBuilder, &keystruct);
 
  return keystruct.keys;
}

nsIPrincipal*
nsDOMStorage::Principal()
{
  return nsnull;
}

PRBool
nsDOMStorage::CanAccessSystem(nsIPrincipal *aPrincipal)
{
  if (!aPrincipal)
    return PR_TRUE;

  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  if (!ssm)
    return PR_FALSE;

  PRBool isSystem;
  nsresult rv = ssm->IsSystemPrincipal(aPrincipal, &isSystem);

  return NS_SUCCEEDED(rv) && isSystem;
}

PRBool
nsDOMStorage::CanAccess(nsIPrincipal *aPrincipal)
{
  
  if (CanAccessSystem(aPrincipal))
    return PR_TRUE;

  nsCAutoString domain;
  nsCOMPtr<nsIURI> unused;
  nsresult rv = GetPrincipalURIAndHost(aPrincipal,
                                       getter_AddRefs(unused), domain);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return domain.Equals(mDomain);
}

void
nsDOMStorage::BroadcastChangeNotification()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  if (NS_FAILED(rv)) {
    return;
  }

  
  
  
  observerService->NotifyObservers((nsIDOMStorageObsolete *)this,
                                   "dom-storage-changed",
                                   UseDB() ? NS_ConvertUTF8toUTF16(mDomain).get() : nsnull);
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMStorage2)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMStorage2)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mStorage)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMStorage2)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mStorage, nsIDOMStorageObsolete)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsDOMStorage2, nsIDOMStorage)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsDOMStorage2, nsIDOMStorage)
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
nsDOMStorage2::InitAsSessionStorage(nsIPrincipal *aPrincipal)
{
  mStorage = new nsDOMStorage();
  if (!mStorage)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mStorage->mSecurityChecker = mStorage;
  mPrincipal = aPrincipal;
  return mStorage->InitAsSessionStorage(aPrincipal);
}

nsresult
nsDOMStorage2::InitAsLocalStorage(nsIPrincipal *aPrincipal)
{
  mStorage = new nsDOMStorage();
  if (!mStorage)
    return NS_ERROR_OUT_OF_MEMORY;

  mStorage->mSecurityChecker = this;
  mPrincipal = aPrincipal;
  return mStorage->InitAsLocalStorage(aPrincipal);
}

nsresult
nsDOMStorage2::InitAsGlobalStorage(const nsACString &aDomainDemanded)
{
  NS_ASSERTION(PR_FALSE, "Should not initialize nsDOMStorage2 as global storage.");
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

PRBool
nsDOMStorage2::CanAccess(nsIPrincipal *aPrincipal)
{
  if (mStorage->mSecurityChecker != this)
    return mStorage->mSecurityChecker->CanAccess(aPrincipal);

  
  if (!aPrincipal)
    return PR_TRUE;

  
  PRBool subsumes;
  nsresult rv = aPrincipal->Subsumes(mPrincipal, &subsumes);
  if (NS_FAILED(rv))
    return PR_FALSE;

  return subsumes;
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
  return mStorage->SetItem(aKey, aData);
}

NS_IMETHODIMP
nsDOMStorage2::RemoveItem(const nsAString& aKey)
{
  return mStorage->RemoveItem(aKey);
}

NS_IMETHODIMP
nsDOMStorage2::Clear()
{
  return mStorage->Clear();
}





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

    PRPackedBool sessionOnly;
    if (!nsDOMStorage::CanUseStorage(&sessionOnly)) {
      *aResult = NS_ERROR_DOM_SECURITY_ERR;
      return nsnull;
    }
  }

  PRBool isSystem = nsContentUtils::IsCallerTrustedForRead();
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


PRBool
nsDOMStorageList::CanAccessDomain(const nsACString& aRequestedDomain,
                                  const nsACString& aCurrentDomain)
{
  return aRequestedDomain.Equals(aCurrentDomain);
}

nsIDOMStorageObsolete*
nsDOMStorageList::GetStorageForDomain(const nsACString& aRequestedDomain,
                                      const nsACString& aCurrentDomain,
                                      PRBool aNoCurrentDomainCheck,
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


PRBool
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
    cb.NoteXPCOMChild((nsIDOMStorageObsolete*) tmp->mStorage);
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsDOMStorageItem, nsIDOMStorageItem)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsDOMStorageItem, nsIDOMStorageItem)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMStorageItem)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMStorageItem)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageItem)
  NS_INTERFACE_MAP_ENTRY(nsIDOMToString)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StorageItem)
NS_INTERFACE_MAP_END

nsDOMStorageItem::nsDOMStorageItem(nsDOMStorage* aStorage,
                                   const nsAString& aKey,
                                   const nsAString& aValue,
                                   PRBool aSecure)
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
nsDOMStorageItem::GetSecure(PRBool* aSecure)
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
nsDOMStorageItem::SetSecure(PRBool aSecure)
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
    
    PRBool secure;
    nsresult rv = mStorage->GetDBValue(mKey, aValue, &secure);
    if (rv == NS_ERROR_DOM_NOT_FOUND_ERR)
      return NS_OK;
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

  PRBool secureCaller = IsCallerSecure();

  if (mStorage->UseDB()) {
    
    return mStorage->SetDBValue(mKey, aValue, secureCaller);
  }

  PRBool secureItem = IsSecure();

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


NS_INTERFACE_MAP_BEGIN(nsDOMStorageEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StorageEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMStorageEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMStorageEvent, nsDOMEvent)


NS_IMETHODIMP
nsDOMStorageEvent::GetDomain(nsAString& aDomain)
{
  
  
  aDomain = mDomain;

  return NS_OK;
}

nsresult
nsDOMStorageEvent::Init()
{
  nsresult rv = InitEvent(NS_LITERAL_STRING("storage"), PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  SetTrusted(PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStorageEvent::InitStorageEvent(const nsAString& aTypeArg,
                                    PRBool aCanBubbleArg,
                                    PRBool aCancelableArg,
                                    const nsAString& aDomainArg)
{
  nsresult rv = InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mDomain = aDomainArg;

  return NS_OK;
}

