






































#include "nsCOMPtr.h"
#include "nsDOMError.h"
#include "nsDOMClassInfo.h"
#include "nsUnicharUtils.h"
#include "nsIDocument.h"
#include "nsDOMStorage.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsReadableUtils.h"
#include "nsIObserverService.h"
#include "nsNetUtil.h"
#include "nsIPrefBranch.h"
#include "nsICookiePermission.h"
#include "nsIPermissionManager.h"

static const PRUint32 ASK_BEFORE_ACCEPT = 1;
static const PRUint32 ACCEPT_SESSION = 2;
static const PRUint32 BEHAVIOR_REJECT = 2;

static const PRUint32 DEFAULT_QUOTA = 5 * 1024;

static const char kPermissionType[] = "cookie";
static const char kStorageEnabled[] = "dom.storage.enabled";
static const char kDefaultQuota[] = "dom.storage.default_quota";
static const char kCookiesBehavior[] = "network.cookie.cookieBehavior";
static const char kCookiesLifetimePolicy[] = "network.cookie.lifetimePolicy";





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

static PRInt32
GetQuota(const nsAString &domain)
{
  
  return ((PRInt32)nsContentUtils::GetIntPref(kDefaultQuota, DEFAULT_QUOTA) * 1024);
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

NS_IMPL_ISUPPORTS1(nsDOMStorageManager, nsIObserver)


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
  if (os)
    os->AddObserver(gStorageManager, "cookie-changed", PR_FALSE);

  return NS_OK;
}


void
nsDOMStorageManager::Shutdown()
{
  NS_IF_RELEASE(gStorageManager);
  gStorageManager = nsnull;
}

PR_STATIC_CALLBACK(PLDHashOperator)
ClearStorage(nsDOMStorageEntry* aEntry, void* userArg)
{
  aEntry->mStorage->ClearAll();
  return PL_DHASH_REMOVE;
}

nsresult
nsDOMStorageManager::Observe(nsISupports *aSubject,
                             const char *aTopic,
                             const PRUnichar *aData)
{
  if (!nsCRT::strcmp(aData, NS_LITERAL_STRING("cleared").get())) {
    mStorages.EnumerateEntries(ClearStorage, nsnull);

#ifdef MOZ_STORAGE
    nsresult rv = nsDOMStorage::InitDB();
    NS_ENSURE_SUCCESS(rv, rv);
    return nsDOMStorage::gStorageDB->RemoveAll();
#endif
  }

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
nsDOMStorageDB* nsDOMStorage::gStorageDB = nsnull;
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

NS_INTERFACE_MAP_BEGIN(nsDOMStorage)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMStorage)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorage)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMStorage)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Storage)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMStorage)
NS_IMPL_RELEASE(nsDOMStorage)

NS_IMETHODIMP
NS_NewDOMStorage(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  nsDOMStorage* storage = new nsDOMStorage();
  if (!storage)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(storage);
  *aResult = storage;

  return NS_OK;
}

nsDOMStorage::nsDOMStorage()
  : mUseDB(PR_FALSE), mSessionOnly(PR_TRUE), mItemsCached(PR_FALSE)
{
  mItems.Init(8);
  if (nsDOMStorageManager::gStorageManager)
    nsDOMStorageManager::gStorageManager->AddToStoragesHash(this);
}

nsDOMStorage::nsDOMStorage(nsIURI* aURI, const nsAString& aDomain, PRBool aUseDB)
  : mUseDB(aUseDB),
    mSessionOnly(PR_TRUE),
    mItemsCached(PR_FALSE),
    mURI(aURI),
    mDomain(aDomain)
{
#ifndef MOZ_STORAGE
  mUseDB = PR_FALSE;
#endif

  mItems.Init(8);
  if (nsDOMStorageManager::gStorageManager)
    nsDOMStorageManager::gStorageManager->AddToStoragesHash(this);
}

nsDOMStorage::~nsDOMStorage()
{
  if (nsDOMStorageManager::gStorageManager)
    nsDOMStorageManager::gStorageManager->RemoveFromStoragesHash(this);
}

void
nsDOMStorage::Init(nsIURI* aURI, const nsAString& aDomain, PRBool aUseDB)
{
  mURI = aURI;
  mDomain.Assign(aDomain);
#ifdef MOZ_STORAGE
  mUseDB = aUseDB;
#else
  mUseDB = PR_FALSE;
#endif
}


PRBool
nsDOMStorage::CanUseStorage(nsIURI* aURI, PRPackedBool* aSessionOnly)
{
  
  
  NS_ASSERTION(aURI && aSessionOnly, "null URI or session flag");

  if (!nsContentUtils::GetBoolPref(kStorageEnabled))
    return PR_FALSE;

  nsCOMPtr<nsIPermissionManager> permissionManager =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  if (!permissionManager)
    return PR_FALSE;

  *aSessionOnly = PR_FALSE;

  PRUint32 perm;
  permissionManager->TestPermission(aURI, kPermissionType, &perm);

  if (perm == nsIPermissionManager::DENY_ACTION)
    return PR_FALSE;

  if (perm == nsICookiePermission::ACCESS_SESSION) {
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

PR_STATIC_CALLBACK(PLDHashOperator)
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

PR_STATIC_CALLBACK(PLDHashOperator)
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

NS_IMETHODIMP
nsDOMStorage::GetItem(const nsAString& aKey, nsIDOMStorageItem **aItem)
{
  *aItem = nsnull;

  if (!CacheStoragePermissions())
    return NS_ERROR_DOM_SECURITY_ERR;

  if (aKey.IsEmpty())
    return NS_OK;

  nsSessionStorageEntry *entry = mItems.GetEntry(aKey);
 
  if (entry) {
    if (!IsCallerSecure() && entry->mItem->IsSecure()) {
      return NS_OK;
    }
    NS_ADDREF(*aItem = entry->mItem);
  }
  else if (UseDB()) {
    PRBool secure;
    nsAutoString value;
    nsAutoString unused;
    nsresult rv = GetDBValue(aKey, value, &secure, unused);
    
    if (rv == NS_ERROR_DOM_SECURITY_ERR || rv == NS_ERROR_DOM_NOT_FOUND_ERR)
      return NS_OK;
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<nsDOMStorageItem> newitem =
      new nsDOMStorageItem(this, aKey, value, secure);
    if (!newitem)
      return NS_ERROR_OUT_OF_MEMORY;

    entry = mItems.PutEntry(aKey);
    NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);

    entry->mItem = newitem;
    NS_ADDREF(*aItem = newitem);
  }

  return NS_OK;
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
    if (UseDB())
      newitem = new nsDOMStorageItem(this, aKey, aData, PR_FALSE);
    else 
      newitem = new nsDOMStorageItem(this, aKey, aData, PR_FALSE);
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
    nsAutoString owner;
    rv = GetDBValue(aKey, value, &secureItem, owner);
    if (rv == NS_ERROR_DOM_NOT_FOUND_ERR)
      return NS_OK;
    NS_ENSURE_SUCCESS(rv, rv);

    rv = gStorageDB->RemoveKey(mDomain, aKey, owner,
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

nsresult
nsDOMStorage::InitDB()
{
#ifdef MOZ_STORAGE
  if (!gStorageDB) {
    gStorageDB = new nsDOMStorageDB();
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

    rv = gStorageDB->GetAllKeys(mDomain, this, &mItems);
    NS_ENSURE_SUCCESS(rv, rv);

    mItemsCached = PR_TRUE;
  }
#endif

  return NS_OK;
}

nsresult
nsDOMStorage::GetDBValue(const nsAString& aKey, nsAString& aValue,
                         PRBool* aSecure, nsAString& aOwner)
{
  aValue.Truncate();

#ifdef MOZ_STORAGE
  if (!UseDB())
    return NS_OK;

  nsresult rv = InitDB();
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString value;
  rv = gStorageDB->GetKeyValue(mDomain, aKey, value, aSecure, aOwner);
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

  
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsContentUtils::GetSecurityManager()->
    GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));

  nsAutoString currentDomain;

  if (subjectPrincipal) {
    nsCOMPtr<nsIURI> uri;
    rv = subjectPrincipal->GetURI(getter_AddRefs(uri));

    if (NS_SUCCEEDED(rv) && uri) {
        nsCAutoString currentDomainAscii;
        uri->GetAsciiHost(currentDomainAscii);
        currentDomain = NS_ConvertUTF8toUTF16(currentDomainAscii);
    }
    
    if (currentDomain.IsEmpty()) {
        return NS_ERROR_DOM_SECURITY_ERR;
    }
  } else {
      currentDomain = mDomain;
  }
  
  rv = gStorageDB->SetKey(mDomain, aKey, aValue, aSecure,
                          currentDomain, GetQuota(currentDomain));
  NS_ENSURE_SUCCESS(rv, rv);

  mItemsCached = PR_FALSE;

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

    return gStorageDB->SetSecure(mDomain, aKey, aSecure);
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

PR_STATIC_CALLBACK(PLDHashOperator)
ClearStorageItem(nsSessionStorageEntry* aEntry, void* userArg)
{
  aEntry->mItem->SetValueInternal(EmptyString());
  return PL_DHASH_NEXT;
}

void
nsDOMStorage::ClearAll()
{
  mItems.EnumerateEntries(ClearStorageItem, nsnull);
}

PR_STATIC_CALLBACK(PLDHashOperator)
CopyStorageItems(nsSessionStorageEntry* aEntry, void* userArg)
{
  nsDOMStorage* newstorage = NS_STATIC_CAST(nsDOMStorage*, userArg);

  newstorage->SetItem(aEntry->GetKey(), aEntry->mItem->GetValueInternal());

  if (aEntry->mItem->IsSecure()) {
    newstorage->SetSecure(aEntry->GetKey(), PR_TRUE);
  }

  return PL_DHASH_NEXT;
}

already_AddRefed<nsIDOMStorage>
nsDOMStorage::Clone(nsIURI* aURI)
{
  if (UseDB()) {
    NS_ERROR("Uh, don't clone a global storage object.");

    return nsnull;
  }

  nsDOMStorage* storage = new nsDOMStorage(aURI, mDomain, PR_FALSE);
  if (!storage)
    return nsnull;

  mItems.EnumerateEntries(CopyStorageItems, storage);

  NS_ADDREF(storage);

  return storage;
}

struct KeysArrayBuilderStruct
{
  PRBool callerIsSecure;
  nsTArray<nsString> *keys;
};

PR_STATIC_CALLBACK(PLDHashOperator)
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

void
nsDOMStorage::BroadcastChangeNotification()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  if (NS_FAILED(rv)) {
    return;
  }

  
  
  
  observerService->NotifyObservers((nsIDOMStorage *)this,
                                   "dom-storage-changed",
                                   UseDB() ? mDomain.get() : nsnull);
}





NS_INTERFACE_MAP_BEGIN(nsDOMStorageList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageList)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StorageList)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMStorageList)
NS_IMPL_RELEASE(nsDOMStorageList)

nsresult
nsDOMStorageList::NamedItem(const nsAString& aDomain,
                            nsIDOMStorage** aStorage)
{
  *aStorage = nsnull;

  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  if (!ssm)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsresult rv = ssm->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  nsCAutoString currentDomain;
  if (subjectPrincipal) {
    rv = subjectPrincipal->GetURI(getter_AddRefs(uri));
    if (NS_SUCCEEDED(rv) && uri) {
      PRPackedBool sessionOnly;
      if (!nsDOMStorage::CanUseStorage(uri, &sessionOnly))
        return NS_ERROR_DOM_SECURITY_ERR;
      
      rv = uri->GetAsciiHost(currentDomain);
      NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  PRBool isSystem;
  rv = ssm->SubjectPrincipalIsSystem(&isSystem);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isSystem || !currentDomain.IsEmpty()) {
    return GetStorageForDomain(uri, aDomain, NS_ConvertUTF8toUTF16(currentDomain),
                               isSystem, aStorage);
  }

  return NS_ERROR_DOM_SECURITY_ERR;
}


PRBool
nsDOMStorageList::CanAccessDomain(const nsAString& aRequestedDomain,
                                  const nsAString& aCurrentDomain)
{
  nsStringArray requestedDomainArray, currentDomainArray;
  PRBool ok = ConvertDomainToArray(aRequestedDomain, &requestedDomainArray);
  if (!ok)
    return PR_FALSE;

  ok = ConvertDomainToArray(aCurrentDomain, &currentDomainArray);
  if (!ok)
    return PR_FALSE;

  if (currentDomainArray.Count() == 1)
    currentDomainArray.AppendString(NS_LITERAL_STRING("localdomain"));

  
  PRInt32 currentPos = 0;
  PRInt32 requestedPos = 0;
  PRInt32 length = requestedDomainArray.Count();
  if (currentDomainArray.Count() > length)
    currentPos = currentDomainArray.Count() - length;
  else if (currentDomainArray.Count() < length)
    requestedPos = length - currentDomainArray.Count();

  
  
  for (; requestedPos < length; requestedPos++, currentPos++) {
    if (*requestedDomainArray[requestedPos] != *currentDomainArray[currentPos])
      return PR_FALSE;
  }

  return PR_TRUE;
}

nsresult
nsDOMStorageList::GetStorageForDomain(nsIURI* aURI,
                                      const nsAString& aRequestedDomain,
                                      const nsAString& aCurrentDomain,
                                      PRBool aNoCurrentDomainCheck,
                                      nsIDOMStorage** aStorage)
{
  
  
  
  nsAutoString trimmedDomain(aRequestedDomain);
  trimmedDomain.Trim(".");
  if (trimmedDomain.FindChar('.') == kNotFound)
    return NS_ERROR_DOM_SECURITY_ERR;

  if (!aNoCurrentDomainCheck && !CanAccessDomain(aRequestedDomain,
                                                 aCurrentDomain)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsStringArray requestedDomainArray;
  PRBool ok = ConvertDomainToArray(aRequestedDomain, &requestedDomainArray);
  if (!ok)
    return NS_ERROR_DOM_SECURITY_ERR;
  
  
  nsAutoString usedDomain;
  PRInt32 requestedPos = 0;
  for (requestedPos = 0; requestedPos < requestedDomainArray.Count();
       requestedPos++) {
    if (!usedDomain.IsEmpty())
      usedDomain.AppendLiteral(".");
    usedDomain.Append(*requestedDomainArray[requestedPos]);
  }

  
  if (!mStorages.Get(usedDomain, aStorage)) {
    nsCOMPtr<nsIDOMStorage> newstorage = new nsDOMStorage(aURI, usedDomain, PR_TRUE);
    if (!newstorage)
      return NS_ERROR_OUT_OF_MEMORY;

    if (!mStorages.Put(usedDomain, newstorage))
      return NS_ERROR_OUT_OF_MEMORY;

    newstorage.swap(*aStorage);
  }

  return NS_OK;
}


PRBool
nsDOMStorageList::ConvertDomainToArray(const nsAString& aDomain,
                                       nsStringArray* aArray)
{
  PRInt32 length = aDomain.Length();
  PRInt32 n = 0;
  while (n < length) {
    PRInt32 dotpos = aDomain.FindChar('.', n);
    nsAutoString domain;

    if (dotpos == -1) 
      domain.Assign(Substring(aDomain, n));
    else if (dotpos - n == 0) 
      return false;
    else if (dotpos >= 0)
      domain.Assign(Substring(aDomain, n, dotpos - n));

    ToLowerCase(domain);
    aArray->AppendString(domain);

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





NS_INTERFACE_MAP_BEGIN(nsDOMStorageItem)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMStorageItem)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageItem)
  NS_INTERFACE_MAP_ENTRY(nsIDOMToString)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StorageItem)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMStorageItem)
NS_IMPL_RELEASE(nsDOMStorageItem)

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
    nsAutoString owner;
    return mStorage->GetDBValue(mKey, value, aSecure, owner);
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
    nsAutoString unused;
    nsresult rv = mStorage->GetDBValue(mKey, aValue, &secure, unused);
    return (rv == NS_ERROR_DOM_NOT_FOUND_ERR) ? NS_OK : rv;
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

NS_IMETHODIMP
nsDOMStorageEvent::InitStorageEventNS(const nsAString& aNamespaceURIArg,
                                      const nsAString& aTypeArg,
                                      PRBool aCanBubbleArg,
                                      PRBool aCancelableArg,
                                      const nsAString& aDomainArg)
{
  
  nsresult rv = InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mDomain = aDomainArg;

  return NS_OK;
}
