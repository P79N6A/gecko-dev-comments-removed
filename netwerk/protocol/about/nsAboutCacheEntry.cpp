




#include "nsAboutCacheEntry.h"
#include "nsAboutCache.h"
#include "nsICacheStorage.h"
#include "CacheObserver.h"
#include "nsDOMString.h"
#include "nsNetUtil.h"
#include "prprf.h"
#include "nsEscape.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsAboutProtocolUtils.h"
#include "nsContentUtils.h"
#include "nsInputStreamPump.h"
#include "CacheFileUtils.h"
#include <algorithm>

using namespace mozilla::net;

#define HEXDUMP_MAX_ROWS 16

static void
HexDump(uint32_t *state, const char *buf, int32_t n, nsCString &result)
{
  char temp[16];

  const unsigned char *p;
  while (n) {
    PR_snprintf(temp, sizeof(temp), "%08x:  ", *state);
    result.Append(temp);
    *state += HEXDUMP_MAX_ROWS;

    p = (const unsigned char *) buf;

    int32_t i, row_max = std::min(HEXDUMP_MAX_ROWS, n);

    
    for (i = 0; i < row_max; ++i) {
      PR_snprintf(temp, sizeof(temp), "%02x  ", *p++);
      result.Append(temp);
    }
    for (i = row_max; i < HEXDUMP_MAX_ROWS; ++i) {
      result.AppendLiteral("    ");
    }

    
    p = (const unsigned char *) buf;
    for (i = 0; i < row_max; ++i, ++p) {
      switch (*p) {
      case '<':
        result.AppendLiteral("&lt;");
        break;
      case '>':
        result.AppendLiteral("&gt;");
        break;
      case '&':
        result.AppendLiteral("&amp;");
        break;
      default:
        if (*p < 0x7F && *p > 0x1F) {
          result.Append(*p);
        } else {
          result.Append('.');
        }
      }
    }

    result.Append('\n');

    buf += row_max;
    n -= row_max;
  }
}




NS_IMPL_ISUPPORTS(nsAboutCacheEntry,
                  nsIAboutModule,
                  nsICacheEntryOpenCallback,
                  nsICacheEntryMetaDataVisitor,
                  nsIStreamListener)




NS_IMETHODIMP
nsAboutCacheEntry::NewChannel(nsIURI* uri,
                              nsILoadInfo* aLoadInfo,
                              nsIChannel** result)
{
    NS_ENSURE_ARG_POINTER(uri);
    nsresult rv;

    nsCOMPtr<nsIInputStream> stream;
    rv = GetContentStream(uri, getter_AddRefs(stream));
    if (NS_FAILED(rv)) return rv;
    return NS_NewInputStreamChannelInternal(result,
                                            uri,
                                            stream,
                                            NS_LITERAL_CSTRING("text/html"),
                                            NS_LITERAL_CSTRING("utf-8"),
                                            aLoadInfo);
}

NS_IMETHODIMP
nsAboutCacheEntry::GetURIFlags(nsIURI *aURI, uint32_t *result)
{
    *result = nsIAboutModule::HIDE_FROM_ABOUTABOUT;
    return NS_OK;
}

NS_IMETHODIMP
nsAboutCacheEntry::GetIndexedDBOriginPostfix(nsIURI *aURI, nsAString &result)
{
    SetDOMStringToNull(result);
    return NS_ERROR_NOT_IMPLEMENTED;
}




nsresult
nsAboutCacheEntry::GetContentStream(nsIURI *uri, nsIInputStream **result)
{
    nsresult rv;

    
    nsCOMPtr<nsIAsyncInputStream> inputStream;
    rv = NS_NewPipe2(getter_AddRefs(inputStream),
                     getter_AddRefs(mOutputStream),
                     true, false,
                     256, UINT32_MAX);
    if (NS_FAILED(rv)) return rv;

    NS_NAMED_LITERAL_CSTRING(
      buffer,
      "<!DOCTYPE html>\n"
      "<html>\n"
      "<head>\n"
      "  <title>Cache entry information</title>\n"
      "  <link rel=\"stylesheet\" "
      "href=\"chrome://global/skin/about.css\" type=\"text/css\"/>\n"
      "  <link rel=\"stylesheet\" "
      "href=\"chrome://global/skin/aboutCacheEntry.css\" type=\"text/css\"/>\n"
      "</head>\n"
      "<body>\n"
      "<h1>Cache entry information</h1>\n");
    uint32_t n;
    rv = mOutputStream->Write(buffer.get(), buffer.Length(), &n);
    if (NS_FAILED(rv)) return rv;
    if (n != buffer.Length()) return NS_ERROR_UNEXPECTED;

    rv = OpenCacheEntry(uri);
    if (NS_FAILED(rv)) return rv;

    inputStream.forget(result);
    return NS_OK;
}

nsresult
nsAboutCacheEntry::OpenCacheEntry(nsIURI *uri)
{
    nsresult rv;

    rv = ParseURI(uri, mStorageName, getter_AddRefs(mLoadInfo),
                       mEnhanceId, getter_AddRefs(mCacheURI));
    if (NS_FAILED(rv)) return rv;

    if (!CacheObserver::UseNewCache() &&
        mLoadInfo->IsPrivate() &&
        mStorageName.EqualsLiteral("disk")) {
        
        
        mStorageName = NS_LITERAL_CSTRING("memory");
    }

    return OpenCacheEntry();
}

nsresult
nsAboutCacheEntry::OpenCacheEntry()
{
    nsresult rv;

    nsCOMPtr<nsICacheStorage> storage;
    rv = nsAboutCache::GetStorage(mStorageName, mLoadInfo, getter_AddRefs(storage));
    if (NS_FAILED(rv)) return rv;

    
    rv = storage->AsyncOpenURI(mCacheURI, mEnhanceId,
                               nsICacheStorage::OPEN_READONLY, this);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsAboutCacheEntry::ParseURI(nsIURI *uri,
                            nsACString &storageName,
                            nsILoadContextInfo **loadInfo,
                            nsCString &enahnceID,
                            nsIURI **cacheUri)
{
    
    
    
    nsresult rv;

    nsAutoCString path;
    rv = uri->GetPath(path);
    if (NS_FAILED(rv))
        return rv;

    nsACString::const_iterator keyBegin, keyEnd, valBegin, begin, end;
    path.BeginReading(begin);
    path.EndReading(end);

    keyBegin = begin; keyEnd = end;
    if (!FindInReadable(NS_LITERAL_CSTRING("?storage="), keyBegin, keyEnd))
        return NS_ERROR_FAILURE;

    valBegin = keyEnd; 

    keyBegin = keyEnd; keyEnd = end;
    if (!FindInReadable(NS_LITERAL_CSTRING("&context="), keyBegin, keyEnd))
        return NS_ERROR_FAILURE;

    storageName.Assign(Substring(valBegin, keyBegin));
    valBegin = keyEnd; 

    keyBegin = keyEnd; keyEnd = end;
    if (!FindInReadable(NS_LITERAL_CSTRING("&eid="), keyBegin, keyEnd))
        return NS_ERROR_FAILURE;

    nsAutoCString contextKey(Substring(valBegin, keyBegin));
    valBegin = keyEnd; 

    keyBegin = keyEnd; keyEnd = end;
    if (!FindInReadable(NS_LITERAL_CSTRING("&uri="), keyBegin, keyEnd))
        return NS_ERROR_FAILURE;

    enahnceID.Assign(Substring(valBegin, keyBegin));

    valBegin = keyEnd; 
    nsAutoCString uriSpec(Substring(valBegin, end)); 

    

    nsCOMPtr<nsILoadContextInfo> info =
      CacheFileUtils::ParseKey(contextKey);
    if (!info)
        return NS_ERROR_FAILURE;
    info.forget(loadInfo);

    rv = NS_NewURI(cacheUri, uriSpec);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}





NS_IMETHODIMP
nsAboutCacheEntry::OnCacheEntryCheck(nsICacheEntry *aEntry,
                                     nsIApplicationCache *aApplicationCache,
                                     uint32_t *result)
{
    *result = nsICacheEntryOpenCallback::ENTRY_WANTED;
    return NS_OK;
}

NS_IMETHODIMP
nsAboutCacheEntry::OnCacheEntryAvailable(nsICacheEntry *entry,
                                         bool isNew,
                                         nsIApplicationCache *aApplicationCache,
                                         nsresult status)
{
    nsresult rv;

    mWaitingForData = false;
    if (entry) {
        rv = WriteCacheEntryDescription(entry);
    } else if (!CacheObserver::UseNewCache() &&
               !mLoadInfo->IsPrivate() &&
               mStorageName.EqualsLiteral("memory")) {
        
        
        
        
        
        
        mStorageName = NS_LITERAL_CSTRING("disk");
        rv = OpenCacheEntry();
        if (NS_SUCCEEDED(rv)) {
            return NS_OK;
        }
    } else {
        rv = WriteCacheEntryUnavailable();
    }
    if (NS_FAILED(rv)) return rv;


    if (!mWaitingForData) {
        
        CloseContent();
    }

    return NS_OK;
}





#define APPEND_ROW(label, value) \
    PR_BEGIN_MACRO \
    buffer.AppendLiteral("  <tr>\n" \
                         "    <th>"); \
    buffer.AppendLiteral(label); \
    buffer.AppendLiteral(":</th>\n" \
                         "    <td>"); \
    buffer.Append(value); \
    buffer.AppendLiteral("</td>\n" \
                         "  </tr>\n"); \
    PR_END_MACRO

nsresult
nsAboutCacheEntry::WriteCacheEntryDescription(nsICacheEntry *entry)
{
    nsresult rv;
    nsCString buffer;
    uint32_t n;

    nsAutoCString str;

    rv = entry->GetKey(str);
    if (NS_FAILED(rv)) return rv;

    buffer.SetCapacity(4096);
    buffer.AssignLiteral("<table>\n"
                         "  <tr>\n"
                         "    <th>key:</th>\n"
                         "    <td id=\"td-key\">");

    
    nsCOMPtr<nsIURI> uri;
    bool isJS = false;
    bool isData = false;

    rv = NS_NewURI(getter_AddRefs(uri), str);
    
    
    if (NS_SUCCEEDED(rv)) {
        uri->SchemeIs("javascript", &isJS);
        uri->SchemeIs("data", &isData);
    }
    char* escapedStr = nsEscapeHTML(str.get());
    if (NS_SUCCEEDED(rv) && !(isJS || isData)) {
        buffer.AppendLiteral("<a href=\"");
        buffer.Append(escapedStr);
        buffer.AppendLiteral("\">");
        buffer.Append(escapedStr);
        buffer.AppendLiteral("</a>");
        uri = 0;
    } else {
        buffer.Append(escapedStr);
    }
    nsMemory::Free(escapedStr);
    buffer.AppendLiteral("</td>\n"
                         "  </tr>\n");

    
    char timeBuf[255];
    uint32_t u = 0;
    int32_t  i = 0;
    nsAutoCString s;

    
    s.Truncate();
    entry->GetFetchCount(&i);
    s.AppendInt(i);
    APPEND_ROW("fetch count", s);

    
    entry->GetLastFetched(&u);
    if (u) {
        PrintTimeString(timeBuf, sizeof(timeBuf), u);
        APPEND_ROW("last fetched", timeBuf);
    } else {
        APPEND_ROW("last fetched", "No last fetch time (bug 1000338)");
    }

    
    entry->GetLastModified(&u);
    if (u) {
        PrintTimeString(timeBuf, sizeof(timeBuf), u);
        APPEND_ROW("last modified", timeBuf);
    } else {
        APPEND_ROW("last modified", "No last modified time (bug 1000338)");
    }

    
    entry->GetExpirationTime(&u);
    if (u < 0xFFFFFFFF) {
        PrintTimeString(timeBuf, sizeof(timeBuf), u);
        APPEND_ROW("expires", timeBuf);
    } else {
        APPEND_ROW("expires", "No expiration time");
    }

    
    s.Truncate();
    uint32_t dataSize;
    if (NS_FAILED(entry->GetStorageDataSize(&dataSize)))
        dataSize = 0;
    s.AppendInt((int32_t)dataSize);     
    s.AppendLiteral(" B");
    APPEND_ROW("Data size", s);

    
    
    
    
    
    
    
    

    
    nsCOMPtr<nsISupports> securityInfo;
    entry->GetSecurityInfo(getter_AddRefs(securityInfo));
    if (securityInfo) {
        APPEND_ROW("Security", "This is a secure document.");
    } else {
        APPEND_ROW("Security",
                   "This document does not have any security info associated with it.");
    }

    buffer.AppendLiteral("</table>\n"
                         "<hr/>\n"
                         "<table>\n");

    mBuffer = &buffer;  
    entry->VisitMetaData(this);
    mBuffer = nullptr;

    buffer.AppendLiteral("</table>\n");
    mOutputStream->Write(buffer.get(), buffer.Length(), &n);
    buffer.Truncate();

    
    if (!dataSize) {
        return NS_OK;
    }

    nsCOMPtr<nsIInputStream> stream;
    entry->OpenInputStream(0, getter_AddRefs(stream));
    if (!stream) {
        return NS_OK;
    }

    nsRefPtr<nsInputStreamPump> pump;
    rv = nsInputStreamPump::Create(getter_AddRefs(pump), stream);
    if (NS_FAILED(rv)) {
        return NS_OK; 
    }

    rv = pump->AsyncRead(this, nullptr);
    if (NS_FAILED(rv)) {
        return NS_OK; 
    }

    mWaitingForData = true;
    return NS_OK;
}

nsresult
nsAboutCacheEntry::WriteCacheEntryUnavailable()
{
    uint32_t n;
    NS_NAMED_LITERAL_CSTRING(buffer,
        "The cache entry you selected is not available.");
    mOutputStream->Write(buffer.get(), buffer.Length(), &n);
    return NS_OK;
}





NS_IMETHODIMP
nsAboutCacheEntry::OnMetaDataElement(char const * key, char const * value)
{
    mBuffer->AppendLiteral("  <tr>\n"
                           "    <th>");
    mBuffer->Append(key);
    mBuffer->AppendLiteral(":</th>\n"
                           "    <td>");
    char* escapedValue = nsEscapeHTML(value);
    mBuffer->Append(escapedValue);
    nsMemory::Free(escapedValue);
    mBuffer->AppendLiteral("</td>\n"
                           "  </tr>\n");

    return NS_OK;
}





NS_IMETHODIMP
nsAboutCacheEntry::OnStartRequest(nsIRequest *request, nsISupports *ctx)
{
    mHexDumpState = 0;

    NS_NAMED_LITERAL_CSTRING(buffer, "<hr/>\n<pre>");
    uint32_t n;
    return mOutputStream->Write(buffer.get(), buffer.Length(), &n);
}

NS_IMETHODIMP
nsAboutCacheEntry::OnDataAvailable(nsIRequest *request, nsISupports *ctx,
                                   nsIInputStream *aInputStream,
                                   uint64_t aOffset,
                                   uint32_t aCount)
{
    uint32_t n;
    return aInputStream->ReadSegments(
        &nsAboutCacheEntry::PrintCacheData, this, aCount, &n);
}


NS_METHOD
nsAboutCacheEntry::PrintCacheData(nsIInputStream *aInStream,
                                  void *aClosure,
                                  const char *aFromSegment,
                                  uint32_t aToOffset,
                                  uint32_t aCount,
                                  uint32_t *aWriteCount)
{
    nsAboutCacheEntry *a = static_cast<nsAboutCacheEntry*>(aClosure);

    nsCString buffer;
    HexDump(&a->mHexDumpState, aFromSegment, aCount, buffer);

    uint32_t n;
    a->mOutputStream->Write(buffer.get(), buffer.Length(), &n);

    *aWriteCount = aCount;

    return NS_OK;
}

NS_IMETHODIMP
nsAboutCacheEntry::OnStopRequest(nsIRequest *request, nsISupports *ctx,
                                 nsresult result)
{
    NS_NAMED_LITERAL_CSTRING(buffer, "</pre>\n");
    uint32_t n;
    mOutputStream->Write(buffer.get(), buffer.Length(), &n);

    CloseContent();

    return NS_OK;
}

void
nsAboutCacheEntry::CloseContent()
{
    NS_NAMED_LITERAL_CSTRING(buffer, "</body>\n</html>\n");
    uint32_t n;
    mOutputStream->Write(buffer.get(), buffer.Length(), &n);

    mOutputStream->Close();
    mOutputStream = nullptr;
}
