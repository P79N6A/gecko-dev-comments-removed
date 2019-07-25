




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

#include "jsapi.h"

#include "mozilla/Preferences.h"
#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"

using namespace mozilla;
using namespace mozilla::scache;

static bool gDisableXULCache = false; 
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



StartupCache*   nsXULPrototypeCache::gStartupCache = nullptr;
nsXULPrototypeCache*  nsXULPrototypeCache::sInstance = nullptr;


nsXULPrototypeCache::nsXULPrototypeCache()
{
}


nsXULPrototypeCache::~nsXULPrototypeCache()
{
    FlushScripts();
}


NS_IMPL_THREADSAFE_ISUPPORTS1(nsXULPrototypeCache, nsIObserver)

 nsXULPrototypeCache*
nsXULPrototypeCache::GetInstance()
{
    if (!sInstance) {
        NS_ADDREF(sInstance = new nsXULPrototypeCache());

        sInstance->mPrototypeTable.Init();
        sInstance->mStyleSheetTable.Init();
        sInstance->mScriptTable.Init();
        sInstance->mXBLDocTable.Init();

        sInstance->mCacheURITable.Init();
        sInstance->mInputStreamTable.Init();
        sInstance->mOutputStreamTable.Init();

        gDisableXULCache =
            Preferences::GetBool(kDisableXULCachePref, gDisableXULCache);
        Preferences::RegisterCallback(DisableXULCacheChangedCallback,
                                      kDisableXULCachePref);

        nsCOMPtr<nsIObserverService> obsSvc =
            mozilla::services::GetObserverService();
        if (obsSvc) {
            nsXULPrototypeCache *p = sInstance;
            obsSvc->AddObserver(p, "chrome-flush-skin-caches", false);
            obsSvc->AddObserver(p, "chrome-flush-caches", false);
            obsSvc->AddObserver(p, "startupcache-invalidate", false);
        }
		
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
        return nullptr;

    
    nsCOMPtr<nsIObjectInputStream> ois;
    rv = GetInputStream(aURI, getter_AddRefs(ois));
    if (NS_FAILED(rv))
        return nullptr;
    
    nsRefPtr<nsXULPrototypeDocument> newProto;
    rv = NS_NewXULPrototypeDocument(getter_AddRefs(newProto));
    if (NS_FAILED(rv))
        return nullptr;
    
    rv = newProto->Read(ois);
    if (NS_SUCCEEDED(rv)) {
        rv = PutPrototype(newProto);
    } else {
        newProto = nullptr;
    }
    
    mInputStreamTable.Remove(aURI);
    RemoveFromCacheSet(aURI);
    return newProto;
}

nsresult
nsXULPrototypeCache::PutPrototype(nsXULPrototypeDocument* aDocument)
{
    nsCOMPtr<nsIURI> uri = aDocument->GetURI();
    
    mPrototypeTable.Put(uri, aDocument);

    return NS_OK;
}

nsresult
nsXULPrototypeCache::PutStyleSheet(nsCSSStyleSheet* aStyleSheet)
{
    nsIURI* uri = aStyleSheet->GetSheetURI();

    mStyleSheetTable.Put(uri, aStyleSheet);

    return NS_OK;
}


JSScript*
nsXULPrototypeCache::GetScript(nsIURI* aURI)
{
    CacheScriptEntry entry;
    if (!mScriptTable.Get(aURI, &entry)) {
        return nullptr;
    }
    return entry.mScriptObject;
}



static PLDHashOperator
ReleaseScriptObjectCallback(nsIURI* aKey, CacheScriptEntry &aData, void* aClosure)
{
    nsCOMPtr<nsIScriptRuntime> rt;
    if (NS_SUCCEEDED(NS_GetJSRuntime(getter_AddRefs(rt))))
        rt->DropScriptObject(aData.mScriptObject);
    return PL_DHASH_REMOVE;
}

nsresult
nsXULPrototypeCache::PutScript(nsIURI* aURI, JSScript* aScriptObject)
{
    CacheScriptEntry existingEntry;
    if (mScriptTable.Get(aURI, &existingEntry)) {
#ifdef DEBUG
        nsCAutoString scriptName;
        aURI->GetSpec(scriptName);
        nsCAutoString message("Loaded script ");
        message += scriptName;
        message += " twice (bug 392650)";
        NS_WARNING(message.get());
#endif
        
        ReleaseScriptObjectCallback(aURI, existingEntry, nullptr);
    }

    CacheScriptEntry entry = {aScriptObject};

    mScriptTable.Put(aURI, entry);

    
    nsCOMPtr<nsIScriptRuntime> rt;
    nsresult rv = NS_GetJSRuntime(getter_AddRefs(rt));
    if (NS_SUCCEEDED(rv))
        rv = rt->HoldScriptObject(aScriptObject);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to GC lock the object");

    
    return rv;
}

void
nsXULPrototypeCache::FlushScripts()
{
    
    
    mScriptTable.Enumerate(ReleaseScriptObjectCallback, nullptr);
}


nsresult
nsXULPrototypeCache::PutXBLDocumentInfo(nsXBLDocumentInfo* aDocumentInfo)
{
    nsIURI* uri = aDocumentInfo->DocumentURI();

    nsRefPtr<nsXBLDocumentInfo> info;
    mXBLDocTable.Get(uri, getter_AddRefs(info));
    if (!info) {
        mXBLDocTable.Put(uri, aDocumentInfo);
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
  
  mXBLDocTable.Enumerate(FlushSkinXBL, nullptr);

  
  mStyleSheetTable.Enumerate(FlushSkinSheets, nullptr);

  
  
  
  mXBLDocTable.Enumerate(FlushScopedSkinStylesheets, nullptr);
}


void
nsXULPrototypeCache::Flush()
{
    mPrototypeTable.Clear();

    
    FlushScripts();

    mStyleSheetTable.Clear();
    mXBLDocTable.Clear();
}


bool
nsXULPrototypeCache::IsEnabled()
{
    return !gDisableXULCache;
}

static bool gDisableXULDiskCache = false;           

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
    nsresult rv = PathifyURI(uri, spec);
    if (NS_FAILED(rv)) 
        return NS_ERROR_NOT_AVAILABLE;
    
    nsAutoArrayPtr<char> buf;
    uint32_t len;
    nsCOMPtr<nsIObjectInputStream> ois;
    if (!gStartupCache)
        return NS_ERROR_NOT_AVAILABLE;
    
    rv = gStartupCache->GetBuffer(spec.get(), getter_Transfers(buf), &len);
    if (NS_FAILED(rv)) 
        return NS_ERROR_NOT_AVAILABLE;

    rv = NewObjectInputStreamFromBuffer(buf, len, getter_AddRefs(ois));
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
    bool found = mOutputStreamTable.Get(uri, getter_AddRefs(storageStream));
    if (found) {
        objectOutput = do_CreateInstance("mozilla.org/binaryoutputstream;1");
        if (!objectOutput) return NS_ERROR_OUT_OF_MEMORY;
        nsCOMPtr<nsIOutputStream> outputStream
            = do_QueryInterface(storageStream);
        objectOutput->SetOutputStream(outputStream);
    } else {
        rv = NewObjectOutputWrappedStorageStream(getter_AddRefs(objectOutput), 
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
    bool found = mOutputStreamTable.Get(uri, getter_AddRefs(storageStream));
    if (!found)
        return NS_ERROR_UNEXPECTED;
    nsCOMPtr<nsIOutputStream> outputStream
        = do_QueryInterface(storageStream);
    outputStream->Close();
    
    nsAutoArrayPtr<char> buf;
    uint32_t len;
    rv = NewBufferFromStorageStream(storageStream, getter_Transfers(buf), 
                                    &len);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString spec(kXULCachePrefix);
    rv = PathifyURI(uri, spec);
    if (NS_FAILED(rv))
        return NS_ERROR_NOT_AVAILABLE;
    rv = gStartupCache->PutBuffer(spec.get(), buf, len);
    if (NS_SUCCEEDED(rv))
        mOutputStreamTable.Remove(uri);
    
    return rv;
}



nsresult
nsXULPrototypeCache::HasData(nsIURI* uri, bool* exists)
{
    if (mOutputStreamTable.Get(uri, nullptr)) {
        *exists = true;
        return NS_OK;
    }
    nsCAutoString spec(kXULCachePrefix);
    nsresult rv = PathifyURI(uri, spec);
    if (NS_FAILED(rv)) {
        *exists = false;
        return NS_OK;
    }
    nsAutoArrayPtr<char> buf;
    uint32_t len;
    if (gStartupCache)
        rv = gStartupCache->GetBuffer(spec.get(), getter_Transfers(buf), 
                                      &len);
    else {
        
        
        
        StartupCache* sc = StartupCache::GetSingleton();
        if (!sc) {
            *exists = false;
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
    bool wasEnabled = !gDisableXULDiskCache;
    gDisableXULDiskCache =
        Preferences::GetBool(kDisableXULCachePref,
                             gDisableXULDiskCache);

    if (wasEnabled && gDisableXULDiskCache) {
        nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();

        if (cache)
            cache->AbortCaching();
    }
    return 0;
}

nsresult
nsXULPrototypeCache::BeginCaching(nsIURI* aURI)
{
    nsresult rv, tmp;

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
    uint32_t len, amtRead;
    nsCOMPtr<nsIObjectInputStream> objectInput;

    rv = startupCache->GetBuffer(kXULCacheInfoKey, getter_Transfers(buf), 
                                 &len);
    if (NS_SUCCEEDED(rv))
        rv = NewObjectInputStreamFromBuffer(buf, len, getter_AddRefs(objectInput));
    
    if (NS_SUCCEEDED(rv)) {
        buf.forget();
        rv = objectInput->ReadCString(fileLocale);
        tmp = objectInput->ReadCString(fileChromePath);
        if (NS_FAILED(tmp)) {
          rv = tmp;
        }
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
        rv = NewObjectOutputWrappedStorageStream(getter_AddRefs(objectOutput),
                                                 getter_AddRefs(storageStream),
                                                 false);
        if (NS_SUCCEEDED(rv)) {
            rv = objectOutput->WriteStringZ(locale.get());
            tmp = objectOutput->WriteStringZ(chromePath.get());
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }
            tmp = objectOutput->Close();
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }
            tmp = storageStream->NewInputStream(0, getter_AddRefs(inputStream));
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }
        }

        if (NS_SUCCEEDED(rv)) {
            uint64_t len64;
            rv = inputStream->Available(&len64);
            if (NS_SUCCEEDED(rv)) {
              if (len64 <= PR_UINT32_MAX)
                len = (uint32_t)len64;
              else
                rv = NS_ERROR_FILE_TOO_BIG;
            }
        }
        
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

static PLDHashOperator
MarkXBLInCCGeneration(nsIURI* aKey, nsRefPtr<nsXBLDocumentInfo> &aDocInfo,
                      void* aClosure)
{
    uint32_t* gen = static_cast<uint32_t*>(aClosure);
    aDocInfo->MarkInCCGeneration(*gen);
    return PL_DHASH_NEXT;
}

static PLDHashOperator
MarkXULInCCGeneration(nsIURI* aKey, nsRefPtr<nsXULPrototypeDocument> &aDoc,
                      void* aClosure)
{
    uint32_t* gen = static_cast<uint32_t*>(aClosure);
    aDoc->MarkInCCGeneration(*gen);
    return PL_DHASH_NEXT;
}

void
nsXULPrototypeCache::MarkInCCGeneration(uint32_t aGeneration)
{
    mXBLDocTable.Enumerate(MarkXBLInCCGeneration, &aGeneration);
    mPrototypeTable.Enumerate(MarkXULInCCGeneration, &aGeneration);
}
