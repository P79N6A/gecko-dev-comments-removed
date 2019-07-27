




#include "nsAboutCache.h"
#include "nsIInputStream.h"
#include "nsIStorageStream.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsEscape.h"
#include "nsAboutProtocolUtils.h"
#include "nsPrintfCString.h"
#include "nsDOMString.h"

#include "nsICacheStorageService.h"
#include "nsICacheStorage.h"
#include "CacheFileUtils.h"
#include "CacheObserver.h"

#include "nsThreadUtils.h"

using namespace mozilla::net;

NS_IMPL_ISUPPORTS(nsAboutCache, nsIAboutModule, nsICacheStorageVisitor)

NS_IMETHODIMP
nsAboutCache::NewChannel(nsIURI* aURI,
                         nsILoadInfo* aLoadInfo,
                         nsIChannel** result)
{
    NS_ENSURE_ARG_POINTER(aURI);

    nsresult rv;

    *result = nullptr;

    nsCOMPtr<nsIInputStream> inputStream;
    rv = NS_NewPipe(getter_AddRefs(inputStream), getter_AddRefs(mStream),
                    16384, (uint32_t)-1,
                    true, 
                    false 
    );
    if (NS_FAILED(rv)) return rv;

    nsAutoCString storageName;
    rv = ParseURI(aURI, storageName);
    if (NS_FAILED(rv)) return rv;

    mOverview = storageName.IsEmpty();
    if (mOverview) {
        
        mStorageList.AppendElement(NS_LITERAL_CSTRING("memory"));
        mStorageList.AppendElement(NS_LITERAL_CSTRING("disk"));
        mStorageList.AppendElement(NS_LITERAL_CSTRING("appcache"));
    } else {
        
        mStorageList.AppendElement(storageName);
    }

    
    mEntriesHeaderAdded = false;

    nsCOMPtr<nsIChannel> channel;
    rv = NS_NewInputStreamChannelInternal(getter_AddRefs(channel),
                                          aURI,
                                          inputStream,
                                          NS_LITERAL_CSTRING("text/html"),
                                          NS_LITERAL_CSTRING("utf-8"),
                                          aLoadInfo);
    if (NS_FAILED(rv)) return rv;

    mBuffer.AssignLiteral(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <title>Network Cache Storage Information</title>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <link rel=\"stylesheet\" href=\"chrome://global/skin/about.css\"/>\n"
        "  <link rel=\"stylesheet\" href=\"chrome://global/skin/aboutCache.css\"/>\n"
        "  <script src=\"chrome://global/content/aboutCache.js\"></script>"
        "</head>\n"
        "<body class=\"aboutPageWideContainer\">\n"
        "<h1>Information about the Network Cache Storage Service</h1>\n");

    
    mBuffer.AppendLiteral(
        "<label><input id='priv' type='checkbox'/> Private</label>\n"
        "<label><input id='anon' type='checkbox'/> Anonymous</label>\n"
    );

    if (CacheObserver::UseNewCache()) {
        
        
        
        
        mBuffer.AppendLiteral(
            "<label><input id='appid' type='text' size='6'/> AppID</label>\n"
            "<label><input id='inbrowser' type='checkbox'/> In Browser Element</label>\n"
        );
    }

    mBuffer.AppendLiteral(
        "<label><input id='submit' type='button' value='Update' onclick='navigate()'/></label>\n"
    );

    if (!mOverview) {
        mBuffer.AppendLiteral("<a href=\"about:cache?storage=&amp;context=");
        char* escapedContext = nsEscapeHTML(mContextString.get());
        mBuffer.Append(escapedContext);
        nsMemory::Free(escapedContext);
        mBuffer.AppendLiteral("\">Back to overview</a>");
    }

    FlushBuffer();

    
    rv = VisitNextStorage();
    if (NS_FAILED(rv)) return rv;

    channel.forget(result);
    return NS_OK;
}

nsresult
nsAboutCache::ParseURI(nsIURI * uri, nsACString & storage)
{
    
    
    
    nsresult rv;

    nsAutoCString path;
    rv = uri->GetPath(path);
    if (NS_FAILED(rv)) return rv;

    mContextString.Truncate();
    mLoadInfo = CacheFileUtils::ParseKey(NS_LITERAL_CSTRING(""));
    storage.Truncate();

    nsACString::const_iterator start, valueStart, end;
    path.BeginReading(start);
    path.EndReading(end);

    valueStart = end;
    if (!FindInReadable(NS_LITERAL_CSTRING("?storage="), start, valueStart)) {
        return NS_OK;
    }

    nsACString::const_iterator storageNameBegin = valueStart;

    start = valueStart;
    valueStart = end;
    if (!FindInReadable(NS_LITERAL_CSTRING("&context="), start, valueStart))
        start = end;

    nsACString::const_iterator storageNameEnd = start;

    mContextString = Substring(valueStart, end);
    mLoadInfo = CacheFileUtils::ParseKey(mContextString);
    storage.Assign(Substring(storageNameBegin, storageNameEnd));

    return NS_OK;
}

nsresult
nsAboutCache::VisitNextStorage()
{
    if (!mStorageList.Length())
        return NS_ERROR_NOT_AVAILABLE;

    mStorageName = mStorageList[0];
    mStorageList.RemoveElementAt(0);

    
    
    
    
    nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(this, &nsAboutCache::FireVisitStorage);
    return NS_DispatchToMainThread(event);
}

void
nsAboutCache::FireVisitStorage()
{
    nsresult rv;

    rv = VisitStorage(mStorageName);
    if (NS_FAILED(rv)) {
        if (mLoadInfo) {
            char* escaped = nsEscapeHTML(mStorageName.get());
            mBuffer.Append(
                nsPrintfCString("<p>Unrecognized storage name '%s' in about:cache URL</p>",
                                escaped));
            nsMemory::Free(escaped);
        } else {
            char* escaped = nsEscapeHTML(mContextString.get());
            mBuffer.Append(
                nsPrintfCString("<p>Unrecognized context key '%s' in about:cache URL</p>",
                                escaped));
            nsMemory::Free(escaped);
        }

        FlushBuffer();

        
        
        OnCacheEntryVisitCompleted();
    }
}

nsresult
nsAboutCache::VisitStorage(nsACString const & storageName)
{
    nsresult rv;

    rv = GetStorage(storageName, mLoadInfo, getter_AddRefs(mStorage));
    if (NS_FAILED(rv)) return rv;

    rv = mStorage->AsyncVisitStorage(this, !mOverview);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


nsresult
nsAboutCache::GetStorage(nsACString const & storageName,
                         nsILoadContextInfo* loadInfo,
                         nsICacheStorage **storage)
{
    nsresult rv;

    nsCOMPtr<nsICacheStorageService> cacheService =
             do_GetService("@mozilla.org/netwerk/cache-storage-service;1", &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsICacheStorage> cacheStorage;
    if (storageName == "disk") {
        rv = cacheService->DiskCacheStorage(
            loadInfo, false, getter_AddRefs(cacheStorage));
    } else if (storageName == "memory") {
        rv = cacheService->MemoryCacheStorage(
            loadInfo, getter_AddRefs(cacheStorage));
    } else if (storageName == "appcache") {
        rv = cacheService->AppCacheStorage(
            loadInfo, nullptr, getter_AddRefs(cacheStorage));
    } else {
        rv = NS_ERROR_UNEXPECTED;
    }
    if (NS_FAILED(rv)) return rv;

    cacheStorage.forget(storage);
    return NS_OK;
}

NS_IMETHODIMP
nsAboutCache::OnCacheStorageInfo(uint32_t aEntryCount, uint64_t aConsumption,
                                 uint64_t aCapacity, nsIFile * aDirectory)
{
    
    if (!mStream) {
        return NS_ERROR_FAILURE;
    }

    mBuffer.AssignLiteral("<h2>");
    mBuffer.Append(mStorageName);
    mBuffer.AppendLiteral("</h2>\n"
                          "<table id=\"");
    mBuffer.AppendLiteral("\">\n");

    
    
    mBuffer.AppendLiteral("  <tr>\n"
                          "    <th>Number of entries:</th>\n"
                          "    <td>");
    mBuffer.AppendInt(aEntryCount);
    mBuffer.AppendLiteral("</td>\n"
                          "  </tr>\n");

    
    mBuffer.AppendLiteral("  <tr>\n"
                          "    <th>Maximum storage size:</th>\n"
                          "    <td>");
    mBuffer.AppendInt(aCapacity / 1024);
    mBuffer.AppendLiteral(" KiB</td>\n"
                          "  </tr>\n");

    
    mBuffer.AppendLiteral("  <tr>\n"
                          "    <th>Storage in use:</th>\n"
                          "    <td>");
    mBuffer.AppendInt(aConsumption / 1024);
    mBuffer.AppendLiteral(" KiB</td>\n"
                          "  </tr>\n");

    
    mBuffer.AppendLiteral("  <tr>\n"
                          "    <th>Storage disk location:</th>\n"
                          "    <td>");
    if (aDirectory) {
        nsAutoString path;
        aDirectory->GetPath(path);
        mBuffer.Append(NS_ConvertUTF16toUTF8(path));
    } else {
        mBuffer.AppendLiteral("none, only stored in memory");
    }
    mBuffer.AppendLiteral("    </td>\n"
                          "  </tr>\n");

    if (mOverview) { 
        if (aEntryCount != 0) { 
            mBuffer.AppendLiteral("  <tr>\n"
                                  "    <th><a href=\"about:cache?storage=");
            mBuffer.Append(mStorageName);
            mBuffer.AppendLiteral("&amp;context=");
            char* escapedContext = nsEscapeHTML(mContextString.get());
            mBuffer.Append(escapedContext);
            nsMemory::Free(escapedContext);
            mBuffer.AppendLiteral("\">List Cache Entries</a></th>\n"
                                  "  </tr>\n");
        }
    }

    mBuffer.AppendLiteral("</table>\n");

    
    mEntriesHeaderAdded = false;

    FlushBuffer();

    if (mOverview) {
        
        
        
        OnCacheEntryVisitCompleted();
    }

    return NS_OK;
}

NS_IMETHODIMP
nsAboutCache::OnCacheEntryInfo(nsIURI *aURI, const nsACString & aIdEnhance,
                               int64_t aDataSize, int32_t aFetchCount,
                               uint32_t aLastModified, uint32_t aExpirationTime)
{
    
    if (!mStream) {
        return NS_ERROR_FAILURE;
    }

    if (!mEntriesHeaderAdded) {
        mBuffer.AppendLiteral("<hr/>\n"
                              "<table id=\"entries\">\n"
                              "  <colgroup>\n"
                              "   <col id=\"col-key\">\n"
                              "   <col id=\"col-dataSize\">\n"
                              "   <col id=\"col-fetchCount\">\n"
                              "   <col id=\"col-lastModified\">\n"
                              "   <col id=\"col-expires\">\n"
                              "  </colgroup>\n"
                              "  <thead>\n"
                              "    <tr>\n"
                              "      <th>Key</th>\n"
                              "      <th>Data size</th>\n"
                              "      <th>Fetch count</th>\n"
                              "      <th>Last Modifed</th>\n"
                              "      <th>Expires</th>\n"
                              "    </tr>\n"
                              "  </thead>\n");
        mEntriesHeaderAdded = true;
    }

    

    nsAutoCString url;
    url.AssignLiteral("about:cache-entry?storage=");
    url.Append(mStorageName);

    url.AppendLiteral("&amp;context=");
    char* escapedContext = nsEscapeHTML(mContextString.get());
    url += escapedContext;
    nsMemory::Free(escapedContext);

    url.AppendLiteral("&amp;eid=");
    char* escapedEID = nsEscapeHTML(aIdEnhance.BeginReading());
    url += escapedEID;
    nsMemory::Free(escapedEID);

    nsAutoCString cacheUriSpec;
    aURI->GetAsciiSpec(cacheUriSpec);
    char* escapedCacheURI = nsEscapeHTML(cacheUriSpec.get());
    url.AppendLiteral("&amp;uri=");
    url += escapedCacheURI;

    
    mBuffer.AppendLiteral("  <tr>\n");

    
    mBuffer.AppendLiteral("    <td><a href=\"");
    mBuffer.Append(url);
    mBuffer.AppendLiteral("\">");
    if (!aIdEnhance.IsEmpty()) {
        mBuffer.Append(aIdEnhance);
        mBuffer.Append(':');
    }
    mBuffer.Append(escapedCacheURI);
    mBuffer.AppendLiteral("</a></td>\n");

    nsMemory::Free(escapedCacheURI);

    
    mBuffer.AppendLiteral("    <td>");
    mBuffer.AppendInt(aDataSize);
    mBuffer.AppendLiteral(" bytes</td>\n");

    
    mBuffer.AppendLiteral("    <td>");
    mBuffer.AppendInt(aFetchCount);
    mBuffer.AppendLiteral("</td>\n");

    
    char buf[255];

    
    mBuffer.AppendLiteral("    <td>");
    if (aLastModified) {
        PrintTimeString(buf, sizeof(buf), aLastModified);
        mBuffer.Append(buf);
    } else {
        mBuffer.AppendLiteral("No last modified time (bug 1000338)");
    }
    mBuffer.AppendLiteral("</td>\n");

    
    mBuffer.AppendLiteral("    <td>");
    if (aExpirationTime < 0xFFFFFFFF) {
        PrintTimeString(buf, sizeof(buf), aExpirationTime);
        mBuffer.Append(buf);
    } else {
        mBuffer.AppendLiteral("No expiration time");
    }
    mBuffer.AppendLiteral("</td>\n");

    
    mBuffer.AppendLiteral("  </tr>\n");

    FlushBuffer();
    return NS_OK;
}

NS_IMETHODIMP
nsAboutCache::OnCacheEntryVisitCompleted()
{
    if (!mStream) {
        return NS_ERROR_FAILURE;
    }

    if (mEntriesHeaderAdded) {
        mBuffer.AppendLiteral("</table>\n");
    }

    
    while (mStorageList.Length()) {
        nsresult rv = VisitNextStorage();
        if (NS_SUCCEEDED(rv)) {
            
            return NS_OK;
        }
    }

    
    mBuffer.AppendLiteral("</body>\n"
                          "</html>\n");
    FlushBuffer();
    mStream->Close();

    return NS_OK;
}

void
nsAboutCache::FlushBuffer()
{
    uint32_t bytesWritten;
    mStream->Write(mBuffer.get(), mBuffer.Length(), &bytesWritten);
    mBuffer.Truncate();
}

NS_IMETHODIMP
nsAboutCache::GetURIFlags(nsIURI *aURI, uint32_t *result)
{
    *result = 0;
    return NS_OK;
}

NS_IMETHODIMP
nsAboutCache::GetIndexedDBOriginPostfix(nsIURI *aURI, nsAString &result)
{
    SetDOMStringToNull(result);
    return NS_ERROR_NOT_IMPLEMENTED;
}


nsresult
nsAboutCache::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsAboutCache* about = new nsAboutCache();
    if (about == nullptr)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(about);
    nsresult rv = about->QueryInterface(aIID, aResult);
    NS_RELEASE(about);
    return rv;
}


