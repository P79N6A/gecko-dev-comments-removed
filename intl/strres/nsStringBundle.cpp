




#include "nsStringBundle.h"
#include "nsID.h"
#include "nsString.h"
#include "nsIStringBundle.h"
#include "nsStringBundleService.h"
#include "nsStringBundleTextOverride.h"
#include "nsISupportsPrimitives.h"
#include "nsIMutableArray.h"
#include "nsArrayEnumerator.h"
#include "nscore.h"
#include "nsMemory.h"
#include "nsNetUtil.h"
#include "nsIObserverService.h"
#include "nsCOMArray.h"
#include "nsTextFormatter.h"
#include "nsIErrorService.h"
#include "nsICategoryManager.h"
#include "nsContentUtils.h"


#ifdef ASYNC_LOADING
#include "nsIBinaryInputStream.h"
#include "nsIStringStream.h"
#endif

using namespace mozilla;

static NS_DEFINE_CID(kErrorServiceCID, NS_ERRORSERVICE_CID);

nsStringBundle::~nsStringBundle()
{
}

nsStringBundle::nsStringBundle(const char* aURLSpec,
                               nsIStringBundleOverride* aOverrideStrings) :
  mPropertiesURL(aURLSpec),
  mOverrideStrings(aOverrideStrings),
  mReentrantMonitor("nsStringBundle.mReentrantMonitor"),
  mAttemptedLoad(false),
  mLoaded(false)
{
}

nsresult
nsStringBundle::LoadProperties()
{
  
  
  
  
  if (mAttemptedLoad) {
    if (mLoaded)
      return NS_OK;

    return NS_ERROR_UNEXPECTED;
  }

  mAttemptedLoad = true;

  nsresult rv;

  
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), mPropertiesURL);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     uri,
                     nsContentUtils::GetSystemPrincipal(),
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_OTHER);

  if (NS_FAILED(rv)) return rv;

  
  channel->SetContentType(NS_LITERAL_CSTRING("text/plain"));

  nsCOMPtr<nsIInputStream> in;
  rv = channel->Open(getter_AddRefs(in));
  if (NS_FAILED(rv)) return rv;

  NS_ASSERTION(NS_SUCCEEDED(rv) && in, "Error in OpenBlockingStream");
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && in, NS_ERROR_FAILURE);

  static NS_DEFINE_CID(kPersistentPropertiesCID, NS_IPERSISTENTPROPERTIES_CID);
  mProps = do_CreateInstance(kPersistentPropertiesCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mAttemptedLoad = mLoaded = true;
  rv = mProps->Load(in);

  mLoaded = NS_SUCCEEDED(rv);

  return rv;
}


nsresult
nsStringBundle::GetStringFromID(int32_t aID, nsAString& aResult)
{
  ReentrantMonitorAutoEnter automon(mReentrantMonitor);
  nsAutoCString name;
  name.AppendInt(aID, 10);

  nsresult rv;

  
  if (mOverrideStrings) {
    rv = mOverrideStrings->GetStringFromName(mPropertiesURL,
                                             name,
                                             aResult);
    if (NS_SUCCEEDED(rv)) return rv;
  }

  rv = mProps->GetStringProperty(name, aResult);

  return rv;
}

nsresult
nsStringBundle::GetStringFromName(const nsAString& aName,
                                  nsAString& aResult)
{
  nsresult rv;

  
  if (mOverrideStrings) {
    rv = mOverrideStrings->GetStringFromName(mPropertiesURL,
                                             NS_ConvertUTF16toUTF8(aName),
                                             aResult);
    if (NS_SUCCEEDED(rv)) return rv;
  }

  rv = mProps->GetStringProperty(NS_ConvertUTF16toUTF8(aName), aResult);
  return rv;
}

NS_IMETHODIMP
nsStringBundle::FormatStringFromID(int32_t aID,
                                   const char16_t **aParams,
                                   uint32_t aLength,
                                   char16_t ** aResult)
{
  nsAutoString idStr;
  idStr.AppendInt(aID, 10);

  return FormatStringFromName(idStr.get(), aParams, aLength, aResult);
}


NS_IMETHODIMP
nsStringBundle::FormatStringFromName(const char16_t *aName,
                                     const char16_t **aParams,
                                     uint32_t aLength,
                                     char16_t **aResult)
{
  NS_ENSURE_ARG_POINTER(aName);
  NS_ASSERTION(aParams && aLength, "FormatStringFromName() without format parameters: use GetStringFromName() instead");
  NS_ENSURE_ARG_POINTER(aResult);

  nsresult rv;
  rv = LoadProperties();
  if (NS_FAILED(rv)) return rv;

  nsAutoString formatStr;
  rv = GetStringFromName(nsDependentString(aName), formatStr);
  if (NS_FAILED(rv)) return rv;

  return FormatString(formatStr.get(), aParams, aLength, aResult);
}


NS_IMPL_ISUPPORTS(nsStringBundle, nsIStringBundle)


NS_IMETHODIMP
nsStringBundle::GetStringFromID(int32_t aID, char16_t **aResult)
{
  nsresult rv;
  rv = LoadProperties();
  if (NS_FAILED(rv)) return rv;

  *aResult = nullptr;
  nsAutoString tmpstr;

  rv = GetStringFromID(aID, tmpstr);
  NS_ENSURE_SUCCESS(rv, rv);

  *aResult = ToNewUnicode(tmpstr);
  NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}


NS_IMETHODIMP
nsStringBundle::GetStringFromName(const char16_t *aName, char16_t **aResult)
{
  NS_ENSURE_ARG_POINTER(aName);
  NS_ENSURE_ARG_POINTER(aResult);

  nsresult rv;
  rv = LoadProperties();
  if (NS_FAILED(rv)) return rv;

  ReentrantMonitorAutoEnter automon(mReentrantMonitor);
  *aResult = nullptr;
  nsAutoString tmpstr;
  rv = GetStringFromName(nsDependentString(aName), tmpstr);
  if (NS_FAILED(rv))
  {
#if 0
    
    
    NS_WARNING("String missing from string bundle");
    printf("  '%s' missing from bundle %s\n", NS_ConvertUTF16toUTF8(aName).get(), mPropertiesURL.get());
#endif
    return rv;
  }

  *aResult = ToNewUnicode(tmpstr);
  NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
nsStringBundle::GetCombinedEnumeration(nsIStringBundleOverride* aOverrideStrings,
                                       nsISimpleEnumerator** aResult)
{
  nsCOMPtr<nsISupports> supports;
  nsCOMPtr<nsIPropertyElement> propElement;

  nsresult rv;

  nsCOMPtr<nsIMutableArray> resultArray =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsISimpleEnumerator> overrideEnumerator;
  rv = aOverrideStrings->EnumerateKeysInBundle(mPropertiesURL,
                                               getter_AddRefs(overrideEnumerator));

  bool hasMore;
  rv = overrideEnumerator->HasMoreElements(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  while (hasMore) {

    rv = overrideEnumerator->GetNext(getter_AddRefs(supports));
    if (NS_SUCCEEDED(rv))
      resultArray->AppendElement(supports, false);

    rv = overrideEnumerator->HasMoreElements(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsISimpleEnumerator> propEnumerator;
  rv = mProps->Enumerate(getter_AddRefs(propEnumerator));
  if (NS_FAILED(rv)) {
    
    return NS_NewArrayEnumerator(aResult, resultArray);
  }

  
  do {
    rv = propEnumerator->GetNext(getter_AddRefs(supports));
    if (NS_SUCCEEDED(rv) &&
        (propElement = do_QueryInterface(supports, &rv))) {

      
      nsAutoCString key;
      propElement->GetKey(key);

      nsAutoString value;
      rv = aOverrideStrings->GetStringFromName(mPropertiesURL, key, value);

      
      if (NS_FAILED(rv))
        resultArray->AppendElement(propElement, false);
    }

    rv = propEnumerator->HasMoreElements(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
  } while (hasMore);

  return resultArray->Enumerate(aResult);
}


NS_IMETHODIMP
nsStringBundle::GetSimpleEnumeration(nsISimpleEnumerator** elements)
{
  if (!elements)
    return NS_ERROR_INVALID_POINTER;

  nsresult rv;
  rv = LoadProperties();
  if (NS_FAILED(rv)) return rv;

  if (mOverrideStrings)
      return GetCombinedEnumeration(mOverrideStrings, elements);

  return mProps->Enumerate(elements);
}

nsresult
nsStringBundle::FormatString(const char16_t *aFormatStr,
                             const char16_t **aParams, uint32_t aLength,
                             char16_t **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_ARG(aLength <= 10); 

  
  
  
  
  
  
  char16_t *text =
    nsTextFormatter::smprintf(aFormatStr,
                              aLength >= 1 ? aParams[0] : nullptr,
                              aLength >= 2 ? aParams[1] : nullptr,
                              aLength >= 3 ? aParams[2] : nullptr,
                              aLength >= 4 ? aParams[3] : nullptr,
                              aLength >= 5 ? aParams[4] : nullptr,
                              aLength >= 6 ? aParams[5] : nullptr,
                              aLength >= 7 ? aParams[6] : nullptr,
                              aLength >= 8 ? aParams[7] : nullptr,
                              aLength >= 9 ? aParams[8] : nullptr,
                              aLength >= 10 ? aParams[9] : nullptr);

  if (!text) {
    *aResult = nullptr;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  
  
  *aResult = NS_strdup(text);
  nsTextFormatter::smprintf_free(text);

  return *aResult ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMPL_ISUPPORTS(nsExtensibleStringBundle, nsIStringBundle)

nsExtensibleStringBundle::nsExtensibleStringBundle()
{
  mLoaded = false;
}

nsresult
nsExtensibleStringBundle::Init(const char * aCategory,
                               nsIStringBundleService* aBundleService)
{

  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = catman->EnumerateCategory(aCategory, getter_AddRefs(enumerator));
  if (NS_FAILED(rv)) return rv;

  bool hasMore;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = enumerator->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv))
      continue;

    nsCOMPtr<nsISupportsCString> supStr = do_QueryInterface(supports, &rv);
    if (NS_FAILED(rv))
      continue;

    nsAutoCString name;
    rv = supStr->GetData(name);
    if (NS_FAILED(rv))
      continue;

    nsCOMPtr<nsIStringBundle> bundle;
    rv = aBundleService->CreateBundle(name.get(), getter_AddRefs(bundle));
    if (NS_FAILED(rv))
      continue;

    mBundles.AppendObject(bundle);
  }

  return rv;
}

nsExtensibleStringBundle::~nsExtensibleStringBundle()
{
}

nsresult nsExtensibleStringBundle::GetStringFromID(int32_t aID, char16_t ** aResult)
{
  nsresult rv;
  const uint32_t size = mBundles.Count();
  for (uint32_t i = 0; i < size; ++i) {
    nsIStringBundle *bundle = mBundles[i];
    if (bundle) {
      rv = bundle->GetStringFromID(aID, aResult);
      if (NS_SUCCEEDED(rv))
        return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult nsExtensibleStringBundle::GetStringFromName(const char16_t *aName,
                                                     char16_t ** aResult)
{
  nsresult rv;
  const uint32_t size = mBundles.Count();
  for (uint32_t i = 0; i < size; ++i) {
    nsIStringBundle* bundle = mBundles[i];
    if (bundle) {
      rv = bundle->GetStringFromName(aName, aResult);
      if (NS_SUCCEEDED(rv))
        return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsExtensibleStringBundle::FormatStringFromID(int32_t aID,
                                             const char16_t ** aParams,
                                             uint32_t aLength,
                                             char16_t ** aResult)
{
  nsAutoString idStr;
  idStr.AppendInt(aID, 10);
  return FormatStringFromName(idStr.get(), aParams, aLength, aResult);
}

NS_IMETHODIMP
nsExtensibleStringBundle::FormatStringFromName(const char16_t *aName,
                                               const char16_t ** aParams,
                                               uint32_t aLength,
                                               char16_t ** aResult)
{
  nsXPIDLString formatStr;
  nsresult rv;
  rv = GetStringFromName(aName, getter_Copies(formatStr));
  if (NS_FAILED(rv))
    return rv;

  return nsStringBundle::FormatString(formatStr, aParams, aLength, aResult);
}

nsresult nsExtensibleStringBundle::GetSimpleEnumeration(nsISimpleEnumerator ** aResult)
{
  
  *aResult = nullptr;
  return NS_ERROR_NOT_IMPLEMENTED;
}



#define MAX_CACHED_BUNDLES 16

struct bundleCacheEntry_t MOZ_FINAL : public LinkedListElement<bundleCacheEntry_t> {
  nsCString mHashKey;
  nsCOMPtr<nsIStringBundle> mBundle;

  bundleCacheEntry_t()
  {
    MOZ_COUNT_CTOR(bundleCacheEntry_t);
  }

  ~bundleCacheEntry_t()
  {
    MOZ_COUNT_DTOR(bundleCacheEntry_t);
  }
};


nsStringBundleService::nsStringBundleService() :
  mBundleMap(MAX_CACHED_BUNDLES)
{
  mErrorService = do_GetService(kErrorServiceCID);
  NS_ASSERTION(mErrorService, "Couldn't get error service");
}

NS_IMPL_ISUPPORTS(nsStringBundleService,
                  nsIStringBundleService,
                  nsIObserver,
                  nsISupportsWeakReference)

nsStringBundleService::~nsStringBundleService()
{
  flushBundleCache();
}

nsresult
nsStringBundleService::Init()
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    os->AddObserver(this, "memory-pressure", true);
    os->AddObserver(this, "profile-do-change", true);
    os->AddObserver(this, "chrome-flush-caches", true);
    os->AddObserver(this, "xpcom-category-entry-added", true);
  }

  
  
  
  mOverrideStrings = do_GetService(NS_STRINGBUNDLETEXTOVERRIDE_CONTRACTID);

  return NS_OK;
}

NS_IMETHODIMP
nsStringBundleService::Observe(nsISupports* aSubject,
                               const char* aTopic,
                               const char16_t* aSomeData)
{
  if (strcmp("memory-pressure", aTopic) == 0 ||
      strcmp("profile-do-change", aTopic) == 0 ||
      strcmp("chrome-flush-caches", aTopic) == 0)
  {
    flushBundleCache();
  }
  else if (strcmp("xpcom-category-entry-added", aTopic) == 0 &&
           NS_LITERAL_STRING("xpcom-autoregistration").Equals(aSomeData))
  {
    mOverrideStrings = do_GetService(NS_STRINGBUNDLETEXTOVERRIDE_CONTRACTID);
  }

  return NS_OK;
}

void
nsStringBundleService::flushBundleCache()
{
  
  mBundleMap.Clear();

  while (!mBundleCache.isEmpty()) {
    delete mBundleCache.popFirst();
  }
}

NS_IMETHODIMP
nsStringBundleService::FlushBundles()
{
  flushBundleCache();
  return NS_OK;
}

nsresult
nsStringBundleService::getStringBundle(const char *aURLSpec,
                                       nsIStringBundle **aResult)
{
  nsDependentCString key(aURLSpec);
  bundleCacheEntry_t* cacheEntry = mBundleMap.Get(key);

  if (cacheEntry) {
    
    
    
    cacheEntry->remove();

  } else {

    
    nsRefPtr<nsStringBundle> bundle = new nsStringBundle(aURLSpec, mOverrideStrings);
    cacheEntry = insertIntoCache(bundle.forget(), key);
  }

  
  
  
  mBundleCache.insertFront(cacheEntry);

  
  *aResult = cacheEntry->mBundle;
  NS_ADDREF(*aResult);

  return NS_OK;
}

bundleCacheEntry_t *
nsStringBundleService::insertIntoCache(already_AddRefed<nsIStringBundle> aBundle,
                                       nsCString &aHashKey)
{
  bundleCacheEntry_t *cacheEntry;

  if (mBundleMap.Count() < MAX_CACHED_BUNDLES) {
    
    cacheEntry = new bundleCacheEntry_t();
  } else {
    
    
    cacheEntry = mBundleCache.getLast();

    
    NS_ASSERTION(mBundleMap.Contains(cacheEntry->mHashKey),
                 "Element will not be removed!");
    mBundleMap.Remove(cacheEntry->mHashKey);
    cacheEntry->remove();
  }

  
  
  cacheEntry->mHashKey = aHashKey;
  cacheEntry->mBundle = aBundle;

  
  mBundleMap.Put(cacheEntry->mHashKey, cacheEntry);

  return cacheEntry;
}

NS_IMETHODIMP
nsStringBundleService::CreateBundle(const char* aURLSpec,
                                    nsIStringBundle** aResult)
{
  return getStringBundle(aURLSpec,aResult);
}

NS_IMETHODIMP
nsStringBundleService::CreateExtensibleBundle(const char* aCategory,
                                              nsIStringBundle** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nullptr;

  nsRefPtr<nsExtensibleStringBundle> bundle = new nsExtensibleStringBundle();

  nsresult res = bundle->Init(aCategory, this);
  if (NS_FAILED(res)) {
    return res;
  }

  res = bundle->QueryInterface(NS_GET_IID(nsIStringBundle), (void**) aResult);

  return res;
}

#define GLOBAL_PROPERTIES "chrome://global/locale/global-strres.properties"

nsresult
nsStringBundleService::FormatWithBundle(nsIStringBundle* bundle, nsresult aStatus,
                                        uint32_t argCount, char16_t** argArray,
                                        char16_t* *result)
{
  nsresult rv;
  nsXPIDLCString key;

  
  uint16_t code = NS_ERROR_GET_CODE(aStatus);
  rv = bundle->FormatStringFromID(code, (const char16_t**)argArray, argCount, result);

  
  
  if (NS_FAILED(rv)) {
    nsAutoString statusStr;
    statusStr.AppendInt(static_cast<uint32_t>(aStatus), 16);
    const char16_t* otherArgArray[1];
    otherArgArray[0] = statusStr.get();
    uint16_t code = NS_ERROR_GET_CODE(NS_ERROR_FAILURE);
    rv = bundle->FormatStringFromID(code, otherArgArray, 1, result);
  }

  return rv;
}

NS_IMETHODIMP
nsStringBundleService::FormatStatusMessage(nsresult aStatus,
                                           const char16_t* aStatusArg,
                                           char16_t* *result)
{
  nsresult rv;
  uint32_t i, argCount = 0;
  nsCOMPtr<nsIStringBundle> bundle;
  nsXPIDLCString stringBundleURL;

  
  if (aStatus == NS_OK && aStatusArg) {
    *result = NS_strdup(aStatusArg);
    NS_ENSURE_TRUE(*result, NS_ERROR_OUT_OF_MEMORY);
    return NS_OK;
  }

  if (aStatus == NS_OK) {
    return NS_ERROR_FAILURE;       
  }

  
  const nsDependentString args(aStatusArg);
  argCount = args.CountChar(char16_t('\n')) + 1;
  NS_ENSURE_ARG(argCount <= 10); 
  char16_t* argArray[10];

  
  if (argCount == 1) {
    
    argArray[0] = (char16_t*)aStatusArg;
  }
  else if (argCount > 1) {
    int32_t offset = 0;
    for (i = 0; i < argCount; i++) {
      int32_t pos = args.FindChar('\n', offset);
      if (pos == -1)
        pos = args.Length();
      argArray[i] = ToNewUnicode(Substring(args, offset, pos - offset));
      if (argArray[i] == nullptr) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        argCount = i - 1; 
        goto done;
      }
      offset = pos + 1;
    }
  }

  
  rv = mErrorService->GetErrorStringBundle(NS_ERROR_GET_MODULE(aStatus),
                                           getter_Copies(stringBundleURL));
  if (NS_SUCCEEDED(rv)) {
    rv = getStringBundle(stringBundleURL, getter_AddRefs(bundle));
    if (NS_SUCCEEDED(rv)) {
      rv = FormatWithBundle(bundle, aStatus, argCount, argArray, result);
    }
  }
  if (NS_FAILED(rv)) {
    rv = getStringBundle(GLOBAL_PROPERTIES, getter_AddRefs(bundle));
    if (NS_SUCCEEDED(rv)) {
      rv = FormatWithBundle(bundle, aStatus, argCount, argArray, result);
    }
  }

done:
  if (argCount > 1) {
    for (i = 0; i < argCount; i++) {
      if (argArray[i])
        nsMemory::Free(argArray[i]);
    }
  }
  return rv;
}
