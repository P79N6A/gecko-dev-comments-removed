









































#include "nsXULPrototypeCache.h"

#include "nsContentUtils.h"
#include "plstr.h"
#include "nsXULPrototypeDocument.h"
#include "nsICSSStyleSheet.h"
#include "nsIScriptRuntime.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsIXBLDocumentInfo.h"

#include "nsIChromeRegistry.h"
#include "nsIFastLoadService.h"
#include "nsIFastLoadFileControl.h"
#include "nsIFile.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIObserverService.h"

#include "nsNetUtil.h"
#include "nsAppDirectoryServiceDefs.h"

#include "jsxdrapi.h"

static NS_DEFINE_CID(kXULPrototypeCacheCID, NS_XULPROTOTYPECACHE_CID);

static PRBool gDisableXULCache = PR_FALSE; 
static const char kDisableXULCachePref[] = "nglayout.debug.disable_xul_cache";



static int
DisableXULCacheChangedCallback(const char* aPref, void* aClosure)
{
    gDisableXULCache =
        nsContentUtils::GetBoolPref(kDisableXULCachePref, gDisableXULCache);

    
    nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
    if (cache)
        cache->Flush();

    return 0;
}




nsIFastLoadService*   nsXULPrototypeCache::gFastLoadService = nsnull;
nsIFile*              nsXULPrototypeCache::gFastLoadFile = nsnull;
nsXULPrototypeCache*  nsXULPrototypeCache::sInstance = nsnull;


nsXULPrototypeCache::nsXULPrototypeCache()
{
}


nsXULPrototypeCache::~nsXULPrototypeCache()
{
    FlushScripts();

    NS_IF_RELEASE(gFastLoadService); 
    NS_IF_RELEASE(gFastLoadFile);
}


NS_IMPL_THREADSAFE_ISUPPORTS2(nsXULPrototypeCache,
                              nsIXULPrototypeCache,
                              nsIObserver)


NS_IMETHODIMP
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
          result->mXBLDocTable.Init() &&
          result->mFastLoadURITable.Init())) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    gDisableXULCache =
        nsContentUtils::GetBoolPref(kDisableXULCachePref, gDisableXULCache);
    nsContentUtils::RegisterPrefCallback(kDisableXULCachePref,
                                         DisableXULCacheChangedCallback,
                                         nsnull);

    nsresult rv = result->QueryInterface(aIID, aResult);

    nsCOMPtr<nsIObserverService> obsSvc(do_GetService("@mozilla.org/observer-service;1"));
    if (obsSvc && NS_SUCCEEDED(rv)) {
        nsXULPrototypeCache *p = result;
        obsSvc->AddObserver(p, "chrome-flush-skin-caches", PR_FALSE);
        obsSvc->AddObserver(p, "chrome-flush-caches", PR_FALSE);
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

 nsIFastLoadService*
nsXULPrototypeCache::GetFastLoadService()
{
    return gFastLoadService;
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
    else {
        NS_WARNING("Unexpected observer topic.");
    }
    return NS_OK;
}

nsXULPrototypeDocument*
nsXULPrototypeCache::GetPrototype(nsIURI* aURI)
{
    nsXULPrototypeDocument* protoDoc = mPrototypeTable.GetWeak(aURI);

    if (!protoDoc) {
        
        
        nsresult rv = StartFastLoad(aURI);
        if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIObjectInputStream> objectInput;
            gFastLoadService->GetInputStream(getter_AddRefs(objectInput));

            rv = StartFastLoadingURI(aURI, nsIFastLoadService::NS_FASTLOAD_READ);
            if (NS_SUCCEEDED(rv)) {
                nsCOMPtr<nsIURI> oldURI;
                gFastLoadService->SelectMuxedDocument(aURI, getter_AddRefs(oldURI));

                
                nsRefPtr<nsXULPrototypeDocument> newProto;
                rv = NS_NewXULPrototypeDocument(getter_AddRefs(newProto));
                if (NS_FAILED(rv)) return nsnull;

                rv = newProto->Read(objectInput);
                if (NS_SUCCEEDED(rv)) {
                    rv = PutPrototype(newProto);
                    if (NS_FAILED(rv))
                        newProto = nsnull;

                    gFastLoadService->EndMuxedDocument(aURI);
                } else {
                    newProto = nsnull;
                }

                RemoveFromFastLoadSet(aURI);
                protoDoc = newProto;
            }
        }
    }
    return protoDoc;
}

nsresult
nsXULPrototypeCache::PutPrototype(nsXULPrototypeDocument* aDocument)
{
    nsCOMPtr<nsIURI> uri = aDocument->GetURI();
    
    NS_ENSURE_TRUE(mPrototypeTable.Put(uri, aDocument), NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
}

nsresult
nsXULPrototypeCache::PutStyleSheet(nsICSSStyleSheet* aStyleSheet)
{
    nsCOMPtr<nsIURI> uri;
    nsresult rv = aStyleSheet->GetSheetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv))
        return rv;

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
nsXULPrototypeCache::PutXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo)
{
    nsIURI* uri = aDocumentInfo->DocumentURI();

    nsCOMPtr<nsIXBLDocumentInfo> info;
    mXBLDocTable.Get(uri, getter_AddRefs(info));
    if (!info) {
        NS_ENSURE_TRUE(mXBLDocTable.Put(uri, aDocumentInfo),
                       NS_ERROR_OUT_OF_MEMORY);
    }
    return NS_OK;
}

static PLDHashOperator
FlushSkinXBL(nsIURI* aKey, nsCOMPtr<nsIXBLDocumentInfo>& aDocInfo, void* aClosure)
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
FlushSkinSheets(nsIURI* aKey, nsCOMPtr<nsICSSStyleSheet>& aSheet, void* aClosure)
{
  nsCOMPtr<nsIURI> uri;
  aSheet->GetSheetURI(getter_AddRefs(uri));
  nsCAutoString str;
  uri->GetPath(str);

  PLDHashOperator ret = PL_DHASH_NEXT;

  if (!strncmp(str.get(), "/skin", 5)) {
    
    ret = PL_DHASH_REMOVE;
  }
  return ret;
}

static PLDHashOperator
FlushScopedSkinStylesheets(nsIURI* aKey, nsCOMPtr<nsIXBLDocumentInfo> &aDocInfo, void* aClosure)
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

static PRBool gDisableXULFastLoad = PR_FALSE;           
static PRBool gChecksumXULFastLoadFile = PR_TRUE;       

void
nsXULPrototypeCache::AbortFastLoads()
{
#ifdef DEBUG_brendan
    NS_BREAK();
#endif

    
    
    nsCOMPtr<nsIFile> file = gFastLoadFile;

    
    
    Flush();

    
    mFastLoadURITable.Clear();

    if (! gFastLoadService)
        return;

    
    
    nsCOMPtr<nsIObjectInputStream> objectInput;
    nsCOMPtr<nsIObjectOutputStream> objectOutput;
    gFastLoadService->GetInputStream(getter_AddRefs(objectInput));
    gFastLoadService->GetOutputStream(getter_AddRefs(objectOutput));

    if (objectOutput) {
        gFastLoadService->SetOutputStream(nsnull);

        if (NS_SUCCEEDED(objectOutput->Close()) && gChecksumXULFastLoadFile)
            gFastLoadService->CacheChecksum(gFastLoadFile,
                                            objectOutput);
    }

    if (objectInput) {
        
        
        
        gFastLoadService->SetInputStream(nsnull);
        objectInput->Close();
    }

    
    if (file) {
#ifdef DEBUG
        
        nsCOMPtr<nsIFile> existingAbortedFile;
        file->Clone(getter_AddRefs(existingAbortedFile));
        if (existingAbortedFile) {
            existingAbortedFile->SetLeafName(NS_LITERAL_STRING("Aborted.mfasl"));
            PRBool fileExists = PR_FALSE;
            existingAbortedFile->Exists(&fileExists);
            if (fileExists)
                existingAbortedFile->Remove(PR_FALSE);
        }
        file->MoveToNative(nsnull, NS_LITERAL_CSTRING("Aborted.mfasl"));
#else
        file->Remove(PR_FALSE);
#endif
    }

    
    NS_RELEASE(gFastLoadService);
    NS_RELEASE(gFastLoadFile);
}


void
nsXULPrototypeCache::RemoveFromFastLoadSet(nsIURI* aURI)
{
    mFastLoadURITable.Remove(aURI);
}

static const char kDisableXULFastLoadPref[] = "nglayout.debug.disable_xul_fastload";
static const char kChecksumXULFastLoadFilePref[] = "nglayout.debug.checksum_xul_fastload_file";

nsresult
nsXULPrototypeCache::WritePrototype(nsXULPrototypeDocument* aPrototypeDocument)
{
    nsresult rv = NS_OK, rv2 = NS_OK;

    
    
    if (! gFastLoadService)
        return NS_OK;

    
    
    nsCOMPtr<nsIObjectInputStream> objectInput;
    nsCOMPtr<nsIObjectOutputStream> objectOutput;
    gFastLoadService->GetInputStream(getter_AddRefs(objectInput));
    gFastLoadService->GetOutputStream(getter_AddRefs(objectOutput));

    nsCOMPtr<nsIURI> protoURI = aPrototypeDocument->GetURI();

    
    
    
    
    RemoveFromFastLoadSet(protoURI);

    PRInt32 count = mFastLoadURITable.Count();

    if (objectOutput) {
        rv = StartFastLoadingURI(protoURI, nsIFastLoadService::NS_FASTLOAD_WRITE);
        if (NS_SUCCEEDED(rv)) {
            
            
            nsCOMPtr<nsIURI> oldURI;
            gFastLoadService->SelectMuxedDocument(protoURI, getter_AddRefs(oldURI));

            aPrototypeDocument->Write(objectOutput);

            gFastLoadService->EndMuxedDocument(protoURI);
        }

        
        
        
        
        
        
        
        
        
        if (count == 0) {
            gFastLoadService->SetOutputStream(nsnull);
            rv = objectOutput->Close();

            if (NS_SUCCEEDED(rv) && gChecksumXULFastLoadFile) {
                rv = gFastLoadService->CacheChecksum(gFastLoadFile,
                                                     objectOutput);
            }
        }
    }

    if (objectInput) {
        
        
        
        if (count == 0) {
            gFastLoadService->SetInputStream(nsnull);
            rv2 = objectInput->Close();
        }
    }

    
    if (count == 0) {
        NS_RELEASE(gFastLoadService);
        NS_RELEASE(gFastLoadFile);
    }

    return NS_FAILED(rv) ? rv : rv2;
}


nsresult
nsXULPrototypeCache::StartFastLoadingURI(nsIURI* aURI, PRInt32 aDirectionFlags)
{
    nsresult rv;

    nsCAutoString urlspec;
    rv = aURI->GetAsciiSpec(urlspec);
    if (NS_FAILED(rv)) return rv;

    
    
    
    
    
    
    return gFastLoadService->StartMuxedDocument(aURI, urlspec.get(), aDirectionFlags);
}

static int
FastLoadPrefChangedCallback(const char* aPref, void* aClosure)
{
    PRBool wasEnabled = !gDisableXULFastLoad;
    gDisableXULFastLoad =
        nsContentUtils::GetBoolPref(kDisableXULFastLoadPref,
                                    gDisableXULFastLoad);

    if (wasEnabled && gDisableXULFastLoad) {
        static NS_DEFINE_CID(kXULPrototypeCacheCID, NS_XULPROTOTYPECACHE_CID);
        nsCOMPtr<nsIXULPrototypeCache> cache =
            do_GetService(kXULPrototypeCacheCID);

        if (cache)
            cache->AbortFastLoads();
    }

    gChecksumXULFastLoadFile =
        nsContentUtils::GetBoolPref(kChecksumXULFastLoadFilePref,
                                    gChecksumXULFastLoadFile);

    return 0;
}


class nsXULFastLoadFileIO : public nsIFastLoadFileIO
{
  public:
    nsXULFastLoadFileIO(nsIFile* aFile)
      : mFile(aFile), mTruncateOutputFile(true) {
    }

    virtual ~nsXULFastLoadFileIO() {
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIFASTLOADFILEIO

    nsCOMPtr<nsIFile>         mFile;
    nsCOMPtr<nsIInputStream>  mInputStream;
    nsCOMPtr<nsIOutputStream> mOutputStream;
    bool mTruncateOutputFile;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(nsXULFastLoadFileIO, nsIFastLoadFileIO)


NS_IMETHODIMP
nsXULFastLoadFileIO::GetInputStream(nsIInputStream** aResult)
{
    if (! mInputStream) {
        nsresult rv;
        nsCOMPtr<nsIInputStream> fileInput;
        rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInput), mFile);
        if (NS_FAILED(rv)) return rv;

        rv = NS_NewBufferedInputStream(getter_AddRefs(mInputStream),
                                       fileInput,
                                       XUL_DESERIALIZATION_BUFFER_SIZE);
        if (NS_FAILED(rv)) return rv;
    }

    NS_ADDREF(*aResult = mInputStream);
    return NS_OK;
}


NS_IMETHODIMP
nsXULFastLoadFileIO::GetOutputStream(nsIOutputStream** aResult)
{
    if (! mOutputStream) {
        PRInt32 ioFlags = PR_WRONLY;
        if (mTruncateOutputFile)
            ioFlags |= PR_CREATE_FILE | PR_TRUNCATE;

        nsresult rv;
        nsCOMPtr<nsIOutputStream> fileOutput;
        rv = NS_NewLocalFileOutputStream(getter_AddRefs(fileOutput), mFile,
                                         ioFlags, 0644);
        if (NS_FAILED(rv)) return rv;

        rv = NS_NewBufferedOutputStream(getter_AddRefs(mOutputStream),
                                        fileOutput,
                                        XUL_SERIALIZATION_BUFFER_SIZE);
        if (NS_FAILED(rv)) return rv;
    }

    NS_ADDREF(*aResult = mOutputStream);
    return NS_OK;
}

NS_IMETHODIMP
nsXULFastLoadFileIO::DisableTruncate()
{
    mTruncateOutputFile = false;
    return NS_OK;
}

nsresult
nsXULPrototypeCache::StartFastLoad(nsIURI* aURI)
{
    nsresult rv;

    nsCAutoString path;
    aURI->GetPath(path);
    if (!StringEndsWith(path, NS_LITERAL_CSTRING(".xul")))
        return NS_ERROR_NOT_AVAILABLE;

    
    
    
    
    
    
    
    
    
    
    
    if (gFastLoadService && gFastLoadFile) {
        mFastLoadURITable.Put(aURI, 1);

        return NS_OK;
    }

    
    
    
    nsCOMPtr<nsIFastLoadService> fastLoadService(do_GetFastLoadService());
    if (! fastLoadService)
        return NS_ERROR_FAILURE;

    gDisableXULFastLoad =
        nsContentUtils::GetBoolPref(kDisableXULFastLoadPref,
                                    gDisableXULFastLoad);
    gChecksumXULFastLoadFile =
        nsContentUtils::GetBoolPref(kChecksumXULFastLoadFilePref,
                                    gChecksumXULFastLoadFile);
    nsContentUtils::RegisterPrefCallback(kDisableXULFastLoadPref,
                                         FastLoadPrefChangedCallback,
                                         nsnull);
    nsContentUtils::RegisterPrefCallback(kChecksumXULFastLoadFilePref,
                                         FastLoadPrefChangedCallback,
                                         nsnull);

    if (gDisableXULFastLoad)
        return NS_ERROR_NOT_AVAILABLE;

    
    
    nsCOMPtr<nsIFile> chromeDir;
    rv = NS_GetSpecialDirectory(NS_APP_CHROME_DIR, getter_AddRefs(chromeDir));
    if (NS_FAILED(rv))
        return rv;
    nsCAutoString chromePath;
    rv = chromeDir->GetNativePath(chromePath);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIFile> file;
    rv = fastLoadService->NewFastLoadFile(XUL_FASTLOAD_FILE_BASENAME,
                                          getter_AddRefs(file));
    if (NS_FAILED(rv))
        return rv;

    
    
    nsXULFastLoadFileIO* xio = new nsXULFastLoadFileIO(file);
    nsCOMPtr<nsIFastLoadFileIO> io = static_cast<nsIFastLoadFileIO*>(xio);
    if (! io)
        return NS_ERROR_OUT_OF_MEMORY;
    fastLoadService->SetFileIO(io);

    nsCOMPtr<nsIXULChromeRegistry> chromeReg(do_GetService(NS_CHROMEREGISTRY_CONTRACTID, &rv));
    if (NS_FAILED(rv))
        return rv;

    
    
    nsCAutoString package;
    rv = aURI->GetHost(package);
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString locale;
    rv = chromeReg->GetSelectedLocale(package, locale);
    if (NS_FAILED(rv))
        return rv;

    
    PRBool exists = PR_FALSE;
    if (NS_SUCCEEDED(file->Exists(&exists)) && exists) {
        nsCOMPtr<nsIObjectInputStream> objectInput;
        rv = fastLoadService->NewInputStream(file, getter_AddRefs(objectInput));

        if (NS_SUCCEEDED(rv)) {
            if (NS_SUCCEEDED(rv)) {
                
                
                
                PRUint32 xulFastLoadVersion, jsByteCodeVersion;
                rv = objectInput->Read32(&xulFastLoadVersion);
                rv |= objectInput->Read32(&jsByteCodeVersion);
                if (NS_SUCCEEDED(rv)) {
                    if (xulFastLoadVersion != XUL_FASTLOAD_FILE_VERSION ||
                        jsByteCodeVersion != JSXDR_BYTECODE_VERSION) {
#ifdef DEBUG
                        printf((xulFastLoadVersion != XUL_FASTLOAD_FILE_VERSION)
                               ? "bad FastLoad file version\n"
                               : "bad JS bytecode version\n");
#endif
                        rv = NS_ERROR_UNEXPECTED;
                    } else {
                        nsCAutoString fileChromePath, fileLocale;

                        rv = objectInput->ReadCString(fileChromePath);
                        rv |= objectInput->ReadCString(fileLocale);
                        if (NS_SUCCEEDED(rv) &&
                            (!fileChromePath.Equals(chromePath) ||
                             !fileLocale.Equals(locale))) {
                            rv = NS_ERROR_UNEXPECTED;
                        }
                    }
                }
            }
        }

        if (NS_SUCCEEDED(rv)) {
            fastLoadService->SetInputStream(objectInput);
        } else {
            
            
            if (objectInput)
                objectInput->Close();
            xio->mInputStream = nsnull;

#ifdef DEBUG
            file->MoveToNative(nsnull, NS_LITERAL_CSTRING("Invalid.mfasl"));
#else
            file->Remove(PR_FALSE);
#endif
            exists = PR_FALSE;
        }
    }

    
    if (! exists) {
        nsCOMPtr<nsIOutputStream> output;
        rv = io->GetOutputStream(getter_AddRefs(output));
        if (NS_FAILED(rv))
            return rv;

        nsCOMPtr<nsIObjectOutputStream> objectOutput;
        rv = fastLoadService->NewOutputStream(output,
                                              getter_AddRefs(objectOutput));
        if (NS_SUCCEEDED(rv)) {
            rv = objectOutput->Write32(XUL_FASTLOAD_FILE_VERSION);
            rv |= objectOutput->Write32(JSXDR_BYTECODE_VERSION);
            rv |= objectOutput->WriteStringZ(chromePath.get());
            rv |= objectOutput->WriteStringZ(locale.get());
        }

        
        
        
        
        if (NS_FAILED(rv)) {
            if (objectOutput)
                objectOutput->Close();
            else
                output->Close();
            xio->mOutputStream = nsnull;

            file->Remove(PR_FALSE);
            return NS_ERROR_FAILURE;
        }

        fastLoadService->SetOutputStream(objectOutput);
    }

    
    
    mFastLoadURITable.Put(aURI, 1);

    NS_ADDREF(gFastLoadService = fastLoadService);
    NS_ADDREF(gFastLoadFile = file);
    return NS_OK;
}

