




#include "DOMStorageManager.h"
#include "DOMStorage.h"
#include "DOMStorageDBThread.h"

#include "nsIScriptSecurityManager.h"
#include "nsIEffectiveTLDService.h"

#include "nsNetUtil.h"
#include "nsPrintfCString.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"




#define DEFAULT_QUOTA_LIMIT (5 * 1024)

namespace mozilla {
namespace dom {

namespace { 

int32_t gQuotaLimit = DEFAULT_QUOTA_LIMIT;

} 

DOMLocalStorageManager*
DOMLocalStorageManager::sSelf = nullptr;


uint32_t
DOMStorageManager::GetQuota()
{
  static bool preferencesInitialized = false;
  if (!preferencesInitialized) {
    mozilla::Preferences::AddIntVarCache(&gQuotaLimit, "dom.storage.default_quota",
                                         DEFAULT_QUOTA_LIMIT);
    preferencesInitialized = true;
  }

  return gQuotaLimit * 1024; 
}

void
ReverseString(const nsCSubstring& aSource, nsCSubstring& aResult)
{
  nsACString::const_iterator sourceBegin, sourceEnd;
  aSource.BeginReading(sourceBegin);
  aSource.EndReading(sourceEnd);

  aResult.SetLength(aSource.Length());
  nsACString::iterator destEnd;
  aResult.EndWriting(destEnd);

  while (sourceBegin != sourceEnd) {
    *(--destEnd) = *sourceBegin;
    ++sourceBegin;
  }
}

nsresult
CreateReversedDomain(const nsACString& aAsciiDomain,
                     nsACString& aKey)
{
  if (aAsciiDomain.IsEmpty()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  ReverseString(aAsciiDomain, aKey);

  aKey.AppendLiteral(".");
  return NS_OK;
}

bool
PrincipalsEqual(nsIPrincipal* aObjectPrincipal, nsIPrincipal* aSubjectPrincipal)
{
  if (!aSubjectPrincipal) {
    return true;
  }

  if (!aObjectPrincipal) {
    return false;
  }

  bool equals;
  nsresult rv = aSubjectPrincipal->EqualsIgnoringDomain(aObjectPrincipal, &equals);

  NS_ASSERTION(NS_SUCCEEDED(rv) && equals,
               "Trying to get DOM storage for wrong principal!");

  if (NS_FAILED(rv) || !equals) {
    return false;
  }

  return true;
}

NS_IMPL_ISUPPORTS1(DOMStorageManager,
                   nsIDOMStorageManager)

DOMStorageManager::DOMStorageManager(nsPIDOMStorage::StorageType aType)
  : mType(aType)
  , mLowDiskSpace(false)
{
  mCaches.Init(10);
  DOMStorageObserver* observer = DOMStorageObserver::Self();
  NS_ASSERTION(observer, "No DOMStorageObserver, cannot observe private data delete notifications!");

  if (observer) {
    observer->AddSink(this);
  }
}

DOMStorageManager::~DOMStorageManager()
{
  DOMStorageObserver* observer = DOMStorageObserver::Self();
  if (observer) {
    observer->RemoveSink(this);
  }
}

namespace { 

nsresult
CreateScopeKey(nsIPrincipal* aPrincipal,
               nsACString& aKey)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!uri) {
    return NS_ERROR_UNEXPECTED;
  }

  nsAutoCString domainScope;
  rv = uri->GetAsciiHost(domainScope);
  NS_ENSURE_SUCCESS(rv, rv);

  if (domainScope.IsEmpty()) {
    
    bool isScheme = false;
    if (NS_SUCCEEDED(uri->SchemeIs("file", &isScheme)) && isScheme) {
      nsCOMPtr<nsIURL> url = do_QueryInterface(uri, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = url->GetDirectory(domainScope);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsAutoCString key;

  rv = CreateReversedDomain(domainScope, key);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsAutoCString scheme;
  rv = uri->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  key.Append(NS_LITERAL_CSTRING(":") + scheme);

  int32_t port = NS_GetRealPort(uri);
  if (port != -1) {
    key.Append(nsPrintfCString(":%d", port));
  }

  bool unknownAppId;
  rv = aPrincipal->GetUnknownAppId(&unknownAppId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!unknownAppId) {
    uint32_t appId;
    rv = aPrincipal->GetAppId(&appId);
    NS_ENSURE_SUCCESS(rv, rv);

    bool isInBrowserElement;
    rv = aPrincipal->GetIsInBrowserElement(&isInBrowserElement);
    NS_ENSURE_SUCCESS(rv, rv);

    if (appId == nsIScriptSecurityManager::NO_APP_ID && !isInBrowserElement) {
      aKey.Assign(key);
      return NS_OK;
    }

    aKey.Truncate();
    aKey.AppendInt(appId);
    aKey.Append(NS_LITERAL_CSTRING(":") + (isInBrowserElement ?
                NS_LITERAL_CSTRING("t") : NS_LITERAL_CSTRING("f")) +
                NS_LITERAL_CSTRING(":") + key);
  }

  return NS_OK;
}

nsresult
CreateQuotaDBKey(nsIPrincipal* aPrincipal,
                 nsACString& aKey)
{
  nsresult rv;

  nsAutoCString subdomainsDBKey;
  nsCOMPtr<nsIEffectiveTLDService> eTLDService(do_GetService(
    NS_EFFECTIVETLDSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(uri, NS_ERROR_UNEXPECTED);

  nsAutoCString eTLDplusOne;
  rv = eTLDService->GetBaseDomain(uri, 0, eTLDplusOne);
  if (NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS == rv) {
    
    rv = uri->GetAsciiHost(eTLDplusOne);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  CreateReversedDomain(eTLDplusOne, subdomainsDBKey);

  bool unknownAppId;
  rv = aPrincipal->GetUnknownAppId(&unknownAppId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!unknownAppId) {
    uint32_t appId;
    rv = aPrincipal->GetAppId(&appId);
    NS_ENSURE_SUCCESS(rv, rv);

    bool isInBrowserElement;
    rv = aPrincipal->GetIsInBrowserElement(&isInBrowserElement);
    NS_ENSURE_SUCCESS(rv, rv);

    if (appId == nsIScriptSecurityManager::NO_APP_ID && !isInBrowserElement) {
      aKey.Assign(subdomainsDBKey);
      return NS_OK;
    }

    aKey.Truncate();
    aKey.AppendInt(appId);
    aKey.Append(NS_LITERAL_CSTRING(":") + (isInBrowserElement ?
                NS_LITERAL_CSTRING("t") : NS_LITERAL_CSTRING("f")) +
                NS_LITERAL_CSTRING(":") + subdomainsDBKey);
  }

  return NS_OK;
}

} 

DOMStorageCache*
DOMStorageManager::GetCache(const nsACString& aScope) const
{
  DOMStorageCacheHashKey* entry = mCaches.GetEntry(aScope);
  if (!entry) {
    return nullptr;
  }

  return entry->cache();
}

already_AddRefed<DOMStorageCache>
DOMStorageManager::PutCache(const nsACString& aScope,
                            nsIPrincipal* aPrincipal)
{
  DOMStorageCacheHashKey* entry = mCaches.PutEntry(aScope);
  nsRefPtr<DOMStorageCache> cache = entry->cache();

  nsAutoCString quotaScope;
  CreateQuotaDBKey(aPrincipal, quotaScope);

  switch (mType) {
  case SessionStorage:
    
    entry->HardRef();
    cache->Init(nullptr, false, aPrincipal, quotaScope);
    break;

  case LocalStorage:
    
    cache->Init(this, true, aPrincipal, quotaScope);
    break;

  default:
    MOZ_ASSERT(false);
  }

  return cache.forget();
}

void
DOMStorageManager::DropCache(DOMStorageCache* aCache)
{
  if (!NS_IsMainThread()) {
    NS_WARNING("DOMStorageManager::DropCache called on a non-main thread, shutting down?");
  }

  mCaches.RemoveEntry(aCache->Scope());
}

nsresult
DOMStorageManager::GetStorageInternal(bool aCreate,
                                      nsIPrincipal* aPrincipal,
                                      const nsAString& aDocumentURI,
                                      bool aPrivate,
                                      nsIDOMStorage** aRetval)
{
  nsresult rv;

  nsAutoCString scope;
  rv = CreateScopeKey(aPrincipal, scope);
  if (NS_FAILED(rv)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<DOMStorageCache> cache = GetCache(scope);

  
  if (!cache) {
    if (!aCreate) {
      *aRetval = nullptr;
      return NS_OK;
    }

    if (!aRetval) {
      
      
      DOMStorageDBBridge* db = DOMStorageCache::GetDatabase();
      if (db) {
        if (!db->ShouldPreloadScope(scope)) {
          return NS_OK;
        }
      } else {
        if (scope.Equals(NS_LITERAL_CSTRING("knalb.:about"))) {
          return NS_OK;
        }
      }
    }

    
    
    cache = PutCache(scope, aPrincipal);
  } else if (mType == SessionStorage) {
    if (!cache->CheckPrincipal(aPrincipal)) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }
  }

  if (aRetval) {
    *aRetval = new DOMStorage(this, cache, aDocumentURI, aPrincipal, aPrivate);
    NS_ADDREF(*aRetval);
  }

  return NS_OK;
}

NS_IMETHODIMP
DOMStorageManager::PrecacheStorage(nsIPrincipal* aPrincipal)
{
  return GetStorageInternal(true, aPrincipal, EmptyString(), false, nullptr);
}

NS_IMETHODIMP
DOMStorageManager::CreateStorage(nsIPrincipal* aPrincipal,
                                 const nsAString& aDocumentURI,
                                 bool aPrivate,
                                 nsIDOMStorage** aRetval)
{
  return GetStorageInternal(true, aPrincipal, aDocumentURI, aPrivate, aRetval);
}

NS_IMETHODIMP
DOMStorageManager::GetStorage(nsIPrincipal* aPrincipal,
                              bool aPrivate,
                              nsIDOMStorage** aRetval)
{
  return GetStorageInternal(false, aPrincipal, EmptyString(), aPrivate, aRetval);
}

NS_IMETHODIMP
DOMStorageManager::CloneStorage(nsIDOMStorage* aStorage)
{
  if (mType != SessionStorage) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsCOMPtr<nsPIDOMStorage> pstorage = do_QueryInterface(aStorage);
  if (!pstorage) {
    return NS_ERROR_UNEXPECTED;
  }

  const DOMStorageCache* origCache = pstorage->GetCache();

  DOMStorageCache* existingCache = GetCache(origCache->Scope());
  if (existingCache) {
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  nsRefPtr<DOMStorageCache> newCache = PutCache(origCache->Scope(),
                                                origCache->Principal());

  newCache->CloneFrom(origCache);
  return NS_OK;
}

NS_IMETHODIMP
DOMStorageManager::CheckStorage(nsIPrincipal* aPrincipal,
                                nsIDOMStorage* aStorage,
                                bool* aRetval)
{
  nsCOMPtr<nsPIDOMStorage> pstorage = do_QueryInterface(aStorage);
  if (!pstorage) {
    return NS_ERROR_UNEXPECTED;
  }

  *aRetval = false;

  if (!aPrincipal) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsAutoCString scope;
  nsresult rv = CreateScopeKey(aPrincipal, scope);
  NS_ENSURE_SUCCESS(rv, rv);

  DOMStorageCache* cache = GetCache(scope);
  if (cache != pstorage->GetCache()) {
    return NS_OK;
  }

  if (!pstorage->PrincipalEquals(aPrincipal)) {
    return NS_OK;
  }

  *aRetval = true;
  return NS_OK;
}



NS_IMETHODIMP
DOMStorageManager::GetLocalStorageForPrincipal(nsIPrincipal* aPrincipal,
                                               const nsAString& aDocumentURI,
                                               bool aPrivate,
                                               nsIDOMStorage** aRetval)
{
  if (mType != LocalStorage) {
    return NS_ERROR_UNEXPECTED;
  }

  return CreateStorage(aPrincipal, aDocumentURI, aPrivate, aRetval);
}

namespace { 

class ClearCacheEnumeratorData
{
public:
  ClearCacheEnumeratorData(uint32_t aFlags)
    : mUnloadFlags(aFlags)
  {}

  uint32_t mUnloadFlags;
  nsCString mKeyPrefix;
};

} 

PLDHashOperator
DOMStorageManager::ClearCacheEnumerator(DOMStorageCacheHashKey* aEntry, void* aClosure)
{
  DOMStorageCache* cache = aEntry->cache();
  nsCString& key = const_cast<nsCString&>(cache->Scope());

  ClearCacheEnumeratorData* data = static_cast<ClearCacheEnumeratorData*>(aClosure);

  if (data->mKeyPrefix.IsEmpty() || StringBeginsWith(key, data->mKeyPrefix)) {
    cache->UnloadItems(data->mUnloadFlags);
  }

  return PL_DHASH_NEXT;
}

nsresult
DOMStorageManager::Observe(const char* aTopic, const nsACString& aScopePrefix)
{
  
  if (!strcmp(aTopic, "cookie-cleared")) {
    ClearCacheEnumeratorData data(DOMStorageCache::kUnloadComplete);
    mCaches.EnumerateEntries(ClearCacheEnumerator, &data);

    return NS_OK;
  }

  
  
  if (!strcmp(aTopic, "session-only-cleared")) {
    ClearCacheEnumeratorData data(DOMStorageCache::kUnloadSession);
    data.mKeyPrefix = aScopePrefix;
    mCaches.EnumerateEntries(ClearCacheEnumerator, &data);

    return NS_OK;
  }

  
  
  if (!strcmp(aTopic, "domain-data-cleared")) {
    ClearCacheEnumeratorData data(DOMStorageCache::kUnloadComplete);
    data.mKeyPrefix = aScopePrefix;
    mCaches.EnumerateEntries(ClearCacheEnumerator, &data);

    return NS_OK;
  }

  
  if (!strcmp(aTopic, "private-browsing-data-cleared")) {
    ClearCacheEnumeratorData data(DOMStorageCache::kUnloadPrivate);
    mCaches.EnumerateEntries(ClearCacheEnumerator, &data);

    return NS_OK;
  }

  
  if (!strcmp(aTopic, "app-data-cleared")) {

    
    if (mType == SessionStorage) {
      return NS_OK;
    }

    ClearCacheEnumeratorData data(DOMStorageCache::kUnloadComplete);
    data.mKeyPrefix = aScopePrefix;
    mCaches.EnumerateEntries(ClearCacheEnumerator, &data);

    return NS_OK;
  }

  if (!strcmp(aTopic, "profile-change")) {
    
    ClearCacheEnumeratorData data(DOMStorageCache::kUnloadComplete);
    mCaches.EnumerateEntries(ClearCacheEnumerator, &data);

    mCaches.Clear();
    return NS_OK;
  }

  if (!strcmp(aTopic, "low-disk-space")) {
    if (mType == LocalStorage) {
      mLowDiskSpace = true;
    }

    return NS_OK;
  }

  if (!strcmp(aTopic, "no-low-disk-space")) {
    if (mType == LocalStorage) {
      mLowDiskSpace = false;
    }

    return NS_OK;
  }

#ifdef DOM_STORAGE_TESTS
  if (!strcmp(aTopic, "test-reload")) {
    if (mType != LocalStorage) {
      return NS_OK;
    }

    
    ClearCacheEnumeratorData data(DOMStorageCache::kTestReload);
    mCaches.EnumerateEntries(ClearCacheEnumerator, &data);
    return NS_OK;
  }

  if (!strcmp(aTopic, "test-flushed")) {
    if (XRE_GetProcessType() != GeckoProcessType_Default) {
      nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
      if (obs) {
        obs->NotifyObservers(nullptr, "domstorage-test-flushed", nullptr);
      }
    }

    return NS_OK;
  }
#endif

  NS_ERROR("Unexpected topic");
  return NS_ERROR_UNEXPECTED;
}



DOMLocalStorageManager::DOMLocalStorageManager()
  : DOMStorageManager(LocalStorage)
{
  NS_ASSERTION(!sSelf, "Somebody is trying to do_CreateInstance(\"@mozilla/dom/localStorage-manager;1\"");
  sSelf = this;

  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    
    
    
    DOMStorageCache::StartDatabase();
  }
}

DOMLocalStorageManager::~DOMLocalStorageManager()
{
  sSelf = nullptr;
}



DOMSessionStorageManager::DOMSessionStorageManager()
  : DOMStorageManager(SessionStorage)
{
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    
    
    DOMStorageCache::StartDatabase();
  }
}

} 
} 
