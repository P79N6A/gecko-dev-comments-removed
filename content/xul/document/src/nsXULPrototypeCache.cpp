









































#include "nsXULPrototypeCache.h"

#include "plstr.h"
#include "nsXULPrototypeDocument.h"
#include "nsCSSStyleSheet.h"
#include "nsIScriptRuntime.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"

#include "nsIChromeRegistry.h"
#include "nsIFile.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIObserverService.h"
#include "nsIStringStream.h"
#include "nsIStorageStream.h"

#include "nsNetUtil.h"
#include "nsAppDirectoryServiceDefs.h"

#include "jsxdrapi.h"

#include "mozilla/Preferences.h"
#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"

using namespace mozilla;
using namespace mozilla::scache;

static NS_DEFINE_CID(kXULPrototypeCacheCID, NS_XULPROTOTYPECACHE_CID);

static PRBool gDisableXULCache = PR_FALSE; 
static const char kDisableXULCachePref[] = "nglayout.debug.disable_xul_cache";
static const char kXULCacheInfoKey[] = "nsXULPrototypeCache.startupCache";
static const char kXULCachePrefix[] = "xulcache";



static int
DisableXULCacheChangedCallback(const char* aPref, void* aClosure)
{
    gDisableXULCache =
        Preferences::GetBool(kDisableXULCachePref, gDisableXULCache);

    
    nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
    if (cache)
        cache->Flush();

    return 0;
}



StartupCache*   nsXULPrototypeCache::gStartupCache = nsnull;
nsXULPrototypeCache*  nsXULPrototypeCache::sInstance = nsnull;


nsXULPrototypeCache::nsXULPrototypeCache()
{
}


nsXULPrototypeCache::~nsXULPrototypeCache()
{
    FlushScripts();
}


NS_IMPL_THREADSAFE_ISUPPORTS2(nsXULPrototypeCache,
                              nsIXULPrototypeCache,
                              nsIObserver)


nsresult
NS_NewXULPrototypeCache(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(! aOuter, "no aggregation");
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsRefPtr<nsXULPrototypeCache> result = new nsXULPrototypeCache();
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    if (!(result->mPrototypeTable.Init() &&
          result->mStyleSheetTable.Init() &&
          result->mScriptTable.Init() &&
          result->mXBLDocTable.Init())) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!(result->mCacheURITable.Init() &&
          result->mInputStreamTable.Init() &&
          result->mOutputStreamTable.Init())) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    gDisableXULCache =
        Preferences::GetBool(kDisableXULCachePref, gDisableXULCache);
    Preferences::RegisterCallback(DisableXULCacheChangedCallback,
                                  kDisableXULCachePref);

    nsresult rv = result->QueryInterface(aIID, aResult);

    nsCOMPtr<nsIObserverService> obsSvc =
        mozilla::services::GetObserverService();
    if (obsSvc && NS_SUCCEEDED(rv)) {
        nsXULPrototypeCache *p = result;
        obsSvc->AddObserver(p, "chrome-flush-skin-caches", PR_FALSE);
        obsSvc->AddObserver(p, "chrome-flush-caches", PR_FALSE);
        obsSvc->AddObserver(p, "startupcache-invalidate", PR_FALSE);
    }

    return rv;
}

 nsXULPrototypeCache*
nsXULPrototypeCache::GetInstance()
{
    
    if (!sInstance) {
        nsIXULPrototypeCache* cache;

        CallGetService(kXULPrototypeCacheCID, &cache);

        sInstance = static_cast<nsXULPrototypeCache*>(cache);
    }
    return sInstance;
}

 StartupCache*
nsXULPrototypeCache::GetStartupCache()
{
    return gStartupCache;
}



NS_IMETHODIMP
nsXULPrototypeCache::Observe(nsISupports* aSubject,
                             const char *aTopic,
                             const PRUnichar *aData)
{
    if (!strcmp(aTopic, "chrome-flush-skin-caches")) {
        FlushSkinFiles();
    }
    else if (!strcmp(aTopic, "chrome-flush-caches")) {
        Flush();
    }
    else if (!strcmp(aTopic, "startupcache-invalidate")) {
        AbortCaching();
    }
    else {
        NS_WARNING("Unexpected observer topic.");
    }
    return NS_OK;
}

nsXULPrototypeDocument*
nsXULPrototypeCache::GetPrototype(nsIURI* aURI)
{
    nsXULPrototypeDocument* protoDoc = mPrototypeTable.GetWeak(aURI);
    if (protoDoc)
        return protoDoc;

    nsresult rv = BeginCaching(aURI);
    if (NS_FAILED(rv))
        return nsnull;

    
    nsCOMPtr<nsIObjectInputStream> ois;
    rv = GetInputStream(aURI, getter_AddRefs(ois));
    if (NS_FAILED(rv))
        return nsnull;
    
    nsRefPtr<nsXULPrototypeDocument> newProto;
    rv = NS_NewXULPrototypeDocument(getter_AddRefs(newProto));
    if (NS_FAILED(rv))
        return nsnull;
    
    rv = newProto->Read(ois);
    if (NS_SUCCEEDED(rv)) {
        rv = PutPrototype(newProto);
    } else {
        newProto = nsnull;
    }
    
    mInputStreamTable.Remove(aURI);
    RemoveFromCacheSet(aURI);
    return newProto;
}

nsresult
nsXULPrototypeCache::PutPrototype(nsXULPrototypeDocument* aDocument)
{
    nsCOMPtr<nsIURI> uri = aDocument->GetURI();
    
    NS_ENSURE_TRUE(mPrototypeTable.Put(uri, aDocument), NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
}

nsresult
nsXULPrototypeCache::PutStyleSheet(nsCSSStyleSheet* aStyleSheet)
{
    nsIURI* uri = aStyleSheet->GetSheetURI();

    NS_ENSURE_TRUE(mStyleSheetTable.Put(uri, aStyleSheet),
                   NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
}


void*
nsXULPrototypeCache::GetScript(nsIURI* aURI, PRUint32 *aLangID)
{
    CacheScriptEntry entry;
    if (!mScriptTable.Get(aURI, &entry)) {
        *aLangID = nsIProgrammingLanguage::UNKNOWN;
        return nsnull;
    }
    *aLangID = entry.mScriptTypeID;
    return entry.mScriptObject;
}



static PLDHashOperator
ReleaseScriptObjectCallback(nsIURI* aKey, CacheScriptEntry &aData, void* aClosure)
{
    nsCOMPtr<nsIScriptRuntime> rt;
    if (NS_SUCCEEDED(NS_GetScriptRuntimeByID(aData.mScriptTypeID, getter_AddRefs(rt))))
        rt->DropScriptObject(aData.mScriptObject);
    return PL_DHASH_REMOVE;
}

nsresult
nsXULPrototypeCache::PutScript(nsIURI* aURI, PRUint32 aLangID, void* aScriptObject)
{
    CacheScriptEntry existingEntry;
    if (mScriptTable.Get(aURI, &existingEntry)) {
        NS_WARNING("loaded the same script twice (bug 392650)");

        
        ReleaseScriptObjectCallback(aURI, existingEntry, nsnull);
    }

    CacheScriptEntry entry = {aLangID, aScriptObject};

    NS_ENSURE_TRUE(mScriptTable.Put(aURI, entry), NS_ERROR_OUT_OF_MEMORY);

    
    nsCOMPtr<nsIScriptRuntime> rt;
    nsresult rv = NS_GetScriptRuntimeByID(aLangID, getter_AddRefs(rt));
    if (NS_SUCCEEDED(rv))
        rv = rt->HoldScriptObject(aScriptObject);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to GC lock the object");

    
    return rv;
}

void
nsXULPrototypeCache::FlushScripts()
{
    
    
    mScriptTable.Enumerate(ReleaseScriptObjectCallback, nsnull);
}


nsresult
nsXULPrototypeCache::PutXBLDocumentInfo(nsXBLDocumentInfo* aDocumentInfo)
{
    nsIURI* uri = aDocumentInfo->DocumentURI();

    nsRefPtr<nsXBLDocumentInfo> info;
    mXBLDocTable.Get(uri, getter_AddRefs(info));
    if (!info) {
        NS_ENSURE_TRUE(mXBLDocTable.Put(uri, aDocumentInfo),
                       NS_ERROR_OUT_OF_MEMORY);
    }
    return NS_OK;
}

static PLDHashOperator
FlushSkinXBL(nsIURI* aKey, nsRefPtr<nsXBLDocumentInfo>& aDocInfo, void* aClosure)
{
  nsCAutoString str;
  aKey->GetPath(str);

  PLDHashOperator ret = PL_DHASH_NEXT;

  if (!strncmp(str.get(), "/skin", 5)) {
    ret = PL_DHASH_REMOVE;
  }

  return ret;
}

static PLDHashOperator
FlushSkinSheets(nsIURI* aKey, nsRefPtr<nsCSSStyleSheet>& aSheet, void* aClosure)
{
  nsCAutoString str;
  aSheet->GetSheetURI()->GetPath(str);

  PLDHashOperator ret = PL_DHASH_NEXT;

  if (!strncmp(str.get(), "/skin", 5)) {
    
    ret = PL_DHASH_REMOVE;
  }
  return ret;
}

static PLDHashOperator
FlushScopedSkinStylesheets(nsIURI* aKey, nsRefPtr<nsXBLDocumentInfo> &aDocInfo, void* aClosure)
{
  aDocInfo->FlushSkinStylesheets();
  return PL_DHASH_NEXT;
}

void
nsXULPrototypeCache::FlushSkinFiles()
{
  
  mXBLDocTable.Enumerate(FlushSkinXBL, nsnull);

  
  mStyleSheetTable.Enumerate(FlushSkinSheets, nsnull);

  
  
  
  mXBLDocTable.Enumerate(FlushScopedSkinStylesheets, nsnull);
}


void
nsXULPrototypeCache::Flush()
{
    mPrototypeTable.Clear();

    
    FlushScripts();

    mStyleSheetTable.Clear();
    mXBLDocTable.Clear();
}


PRBool
nsXULPrototypeCache::IsEnabled()
{
    return !gDisableXULCache;
}

static PRBool gDisableXULDiskCache = PR_FALSE;           

void
nsXULPrototypeCache::AbortCaching()
{
#ifdef DEBUG_brendan
    NS_BREAK();
#endif

    
    
    Flush();

    
    mCacheURITable.Clear();
}


static const char kDisableXULDiskCachePref[] = "nglayout.debug.disable_xul_fastload";

void
nsXULPrototypeCache::RemoveFromCacheSet(nsIURI* aURI)
{
    mCacheURITable.Remove(aURI);
}

nsresult
nsXULPrototypeCache::WritePrototype(nsXULPrototypeDocument* aPrototypeDocument)
{
    nsresult rv = NS_OK, rv2 = NS_OK;

    
    
    if (!gStartupCache)
        return NS_OK;

    nsCOMPtr<nsIURI> protoURI = aPrototypeDocument->GetURI();

    
    
    
    RemoveFromCacheSet(protoURI);

    PRInt32 count = mCacheURITable.Count();
    nsCOMPtr<nsIObjectOutputStream> oos;
    rv = GetOutputStream(protoURI, getter_AddRefs(oos));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aPrototypeDocument->Write(oos);
    NS_ENSURE_SUCCESS(rv, rv);
    FinishOutputStream(protoURI);
    return NS_FAILED(rv) ? rv : rv2;
}

nsresult
nsXULPrototypeCache::GetInputStream(nsIURI* uri, nsIObjectInputStream** stream) 
{
    nsCAutoString spec(kXULCachePrefix);
    nsresult rv = NS_PathifyURI(uri, spec);
    if (NS_FAILED(rv)) 
        return NS_ERROR_NOT_AVAILABLE;
    
    nsAutoArrayPtr<char> buf;
    PRUint32 len;
    nsCOMPtr<nsIObjectInputStream> ois;
    if (!gStartupCache)
        return NS_ERROR_NOT_AVAILABLE;
    
    rv = gStartupCache->GetBuffer(spec.get(), getter_Transfers(buf), &len);
    if (NS_FAILED(rv)) 
        return NS_ERROR_NOT_AVAILABLE;

    rv = NS_NewObjectInputStreamFromBuffer(buf, len, getter_AddRefs(ois));
    NS_ENSURE_SUCCESS(rv, rv);
    buf.forget();

    mInputStreamTable.Put(uri, ois);
    
    NS_ADDREF(*stream = ois);
    return NS_OK;
}

nsresult
nsXULPrototypeCache::FinishInputStream(nsIURI* uri) {
    mInputStreamTable.Remove(uri);
    return NS_OK;
}

nsresult
nsXULPrototypeCache::GetOutputStream(nsIURI* uri, nsIObjectOutputStream** stream)
{
    nsresult rv;
    nsCOMPtr<nsIObjectOutputStream> objectOutput;
    nsCOMPtr<nsIStorageStream> storageStream;
    PRBool found = mOutputStreamTable.Get(uri, getter_AddRefs(storageStream));
    if (found) {
        objectOutput = do_CreateInstance("mozilla.org/binaryoutputstream;1");
        if (!objectOutput) return NS_ERROR_OUT_OF_MEMORY;
        nsCOMPtr<nsIOutputStream> outputStream
            = do_QueryInterface(storageStream);
        objectOutput->SetOutputStream(outputStream);
    } else {
        rv = NS_NewObjectOutputWrappedStorageStream(getter_AddRefs(objectOutput), 
                                                    getter_AddRefs(storageStream),
                                                    false);
        NS_ENSURE_SUCCESS(rv, rv);
        mOutputStreamTable.Put(uri, storageStream);
    }
    NS_ADDREF(*stream = objectOutput);
    return NS_OK;
}

nsresult
nsXULPrototypeCache::FinishOutputStream(nsIURI* uri) 
{
    nsresult rv;
    if (!gStartupCache)
        return NS_ERROR_NOT_AVAILABLE;
    
    nsCOMPtr<nsIStorageStream> storageStream;
    PRBool found = mOutputStreamTable.Get(uri, getter_AddRefs(storageStream));
    if (!found)
        return NS_ERROR_UNEXPECTED;
    nsCOMPtr<nsIOutputStream> outputStream
        = do_QueryInterface(storageStream);
    outputStream->Close();
    
    nsAutoArrayPtr<char> buf;
    PRUint32 len;
    rv = NS_NewBufferFromStorageStream(storageStream, getter_Transfers(buf), 
                                       &len);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString spec(kXULCachePrefix);
    rv = NS_PathifyURI(uri, spec);
    if (NS_FAILED(rv))
        return NS_ERROR_NOT_AVAILABLE;
    rv = gStartupCache->PutBuffer(spec.get(), buf, len);
    if (NS_SUCCEEDED(rv))
        mOutputStreamTable.Remove(uri);
    
    return rv;
}



nsresult
nsXULPrototypeCache::HasData(nsIURI* uri, PRBool* exists)
{
    if (mOutputStreamTable.Get(uri, nsnull)) {
        *exists = PR_TRUE;
        return NS_OK;
    }
    nsCAutoString spec(kXULCachePrefix);
    nsresult rv = NS_PathifyURI(uri, spec);
    if (NS_FAILED(rv)) {
        *exists = PR_FALSE;
        return NS_OK;
    }
    nsAutoArrayPtr<char> buf;
    PRUint32 len;
    if (gStartupCache)
        rv = gStartupCache->GetBuffer(spec.get(), getter_Transfers(buf), 
                                      &len);
    else {
        
        
        
        StartupCache* sc = StartupCache::GetSingleton();
        if (!sc) {
            *exists = PR_FALSE;
            return NS_OK;
        }
        rv = sc->GetBuffer(spec.get(), getter_Transfers(buf), &len);
    }
    *exists = NS_SUCCEEDED(rv);
    return NS_OK;
}

static int
CachePrefChangedCallback(const char* aPref, void* aClosure)
{
    PRBool wasEnabled = !gDisableXULDiskCache;
    gDisableXULDiskCache =
        Preferences::GetBool(kDisableXULCachePref,
                             gDisableXULDiskCache);

    if (wasEnabled && gDisableXULDiskCache) {
        static NS_DEFINE_CID(kXULPrototypeCacheCID, NS_XULPROTOTYPECACHE_CID);
        nsCOMPtr<nsIXULPrototypeCache> cache =
            do_GetService(kXULPrototypeCacheCID);

        if (cache)
            cache->AbortCaching();
    }
    return 0;
}

nsresult
nsXULPrototypeCache::BeginCaching(nsIURI* aURI)
{
    nsresult rv;

    nsCAutoString path;
    aURI->GetPath(path);
    if (!StringEndsWith(path, NS_LITERAL_CSTRING(".xul")))
        return NS_ERROR_NOT_AVAILABLE;

    
    
    
    
    
    
    
    if (gStartupCache) {
        mCacheURITable.Put(aURI, 1);

        return NS_OK;
    }

    
    
    StartupCache* startupCache = StartupCache::GetSingleton();
    if (!startupCache)
        return NS_ERROR_FAILURE;

    gDisableXULDiskCache =
        Preferences::GetBool(kDisableXULCachePref, gDisableXULDiskCache);

    Preferences::RegisterCallback(CachePrefChangedCallback,
                                  kDisableXULCachePref);

    if (gDisableXULDiskCache)
        return NS_ERROR_NOT_AVAILABLE;

    
    
    nsCOMPtr<nsIFile> chromeDir;
    rv = NS_GetSpecialDirectory(NS_APP_CHROME_DIR, getter_AddRefs(chromeDir));
    if (NS_FAILED(rv))
        return rv;
    nsCAutoString chromePath;
    rv = chromeDir->GetNativePath(chromePath);
    if (NS_FAILED(rv))
        return rv;

    
    
    nsCAutoString package;
    rv = aURI->GetHost(package);
    if (NS_FAILED(rv))
        return rv;
    nsCOMPtr<nsIXULChromeRegistry> chromeReg
        = do_GetService(NS_CHROMEREGISTRY_CONTRACTID, &rv);
    nsCAutoString locale;
    rv = chromeReg->GetSelectedLocale(package, locale);
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString fileChromePath, fileLocale;
    
    nsAutoArrayPtr<char> buf;
    PRUint32 len, amtRead;
    nsCOMPtr<nsIObjectInputStream> objectInput;

    rv = startupCache->GetBuffer(kXULCacheInfoKey, getter_Transfers(buf), 
                                 &len);
    if (NS_SUCCEEDED(rv))
        rv = NS_NewObjectInputStreamFromBuffer(buf, len, getter_AddRefs(objectInput));
    
    if (NS_SUCCEEDED(rv)) {
        buf.forget();
        rv = objectInput->ReadCString(fileLocale);
        rv |= objectInput->ReadCString(fileChromePath);
        if (NS_FAILED(rv) ||
            (!fileChromePath.Equals(chromePath) ||
             !fileLocale.Equals(locale))) {
            
            
            
            startupCache->InvalidateCache();
            rv = NS_ERROR_UNEXPECTED;
        }
    } else if (rv != NS_ERROR_NOT_AVAILABLE)
        
        return rv;

    if (NS_FAILED(rv)) {
        
        nsCOMPtr<nsIObjectOutputStream> objectOutput;
        nsCOMPtr<nsIInputStream> inputStream;
        nsCOMPtr<nsIStorageStream> storageStream;
        rv = NS_NewObjectOutputWrappedStorageStream(getter_AddRefs(objectOutput),
                                                    getter_AddRefs(storageStream),
                                                    false);
        if (NS_SUCCEEDED(rv)) {
            rv = objectOutput->WriteStringZ(locale.get());
            rv |= objectOutput->WriteStringZ(chromePath.get());
            rv |= objectOutput->Close();
            rv |= storageStream->NewInputStream(0, getter_AddRefs(inputStream));
        }
        if (NS_SUCCEEDED(rv))
            rv = inputStream->Available(&len);
        
        if (NS_SUCCEEDED(rv)) {
            buf = new char[len];
            rv = inputStream->Read(buf, len, &amtRead);
            if (NS_SUCCEEDED(rv) && len == amtRead)
                rv = startupCache->PutBuffer(kXULCacheInfoKey, buf, len);
            else {
                rv = NS_ERROR_UNEXPECTED;
            }
        }

        
        if (NS_FAILED(rv)) {
            startupCache->InvalidateCache();
            return NS_ERROR_FAILURE;
        }
    }

    
    
    mCacheURITable.Put(aURI, 1);

    gStartupCache = startupCache;
    return NS_OK;
}
