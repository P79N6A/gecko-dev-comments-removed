






































#include <limits.h>

#include "nsAboutCacheEntry.h"
#include "nsICacheService.h"
#include "nsICacheEntryDescriptor.h"
#include "nsIStorageStream.h"
#include "nsNetUtil.h"
#include "nsAutoPtr.h"
#include "prprf.h"
#include "prtime.h"
#include "nsEscape.h"

#define HEXDUMP_MAX_ROWS 16

static void
HexDump(PRUint32 *state, const char *buf, PRInt32 n, nsCString &result)
{
  char temp[16];

  const unsigned char *p;
  while (n) {
    PR_snprintf(temp, sizeof(temp), "%08x:  ", *state);
    result.Append(temp);
    *state += HEXDUMP_MAX_ROWS;

    p = (const unsigned char *) buf;

    PRInt32 i, row_max = PR_MIN(HEXDUMP_MAX_ROWS, n);

    
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




NS_IMPL_ISUPPORTS2(nsAboutCacheEntry,
                   nsIAboutModule,
                   nsICacheMetaDataVisitor)




NS_IMETHODIMP
nsAboutCacheEntry::NewChannel(nsIURI *uri, nsIChannel **result)
{
    NS_ENSURE_ARG_POINTER(uri);
    nsresult rv;

    nsCOMPtr<nsIInputStream> stream;
    rv = GetContentStream(uri, getter_AddRefs(stream));
    if (NS_FAILED(rv)) return rv;

    return NS_NewInputStreamChannel(result, uri, stream,
                                    NS_LITERAL_CSTRING("application/xhtml+xml"),
                                    NS_LITERAL_CSTRING("utf-8"));
}

NS_IMETHODIMP
nsAboutCacheEntry::GetURIFlags(nsIURI *aURI, PRUint32 *result)
{
    *result = 0;
    return NS_OK;
}




nsresult
nsAboutCacheEntry::GetContentStream(nsIURI *uri, nsIInputStream **result)
{
    nsCOMPtr<nsIStorageStream> storageStream;
    nsCOMPtr<nsIOutputStream> outputStream;
    PRUint32 n;
    nsCString buffer;
    nsresult rv;

    nsCOMPtr<nsICacheEntryDescriptor> descriptor;
    OpenCacheEntry(uri, getter_AddRefs(descriptor));

    
    rv = NS_NewStorageStream(256, PRUint32(-1), getter_AddRefs(storageStream));
    if (NS_FAILED(rv)) return rv;

    rv = storageStream->GetOutputStream(0, getter_AddRefs(outputStream));
    if (NS_FAILED(rv)) return rv;

    buffer.AssignLiteral(
                  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\n"
                  "    \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n"
                  "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
                  "<head>\n<title>Cache entry information</title>\n"
                  "<style type=\"text/css\">\npre {\n  margin: 0;\n}\n"
                  "td:first-child {\n  text-align: right;\n  vertical-align: top;\n"
                  "  line-height: 0.8em;\n}\n</style>\n</head>\n<body>\n");
    outputStream->Write(buffer.get(), buffer.Length(), &n);

    if (descriptor)
        rv = WriteCacheEntryDescription(outputStream, descriptor);
    else
        rv = WriteCacheEntryUnavailable(outputStream);
    if (NS_FAILED(rv)) return rv;

    buffer.AssignLiteral("</body>\n</html>\n");
    outputStream->Write(buffer.get(), buffer.Length(), &n);
        
    nsCOMPtr<nsIInputStream> inStr;
    PRUint32 size;

    rv = storageStream->GetLength(&size);
    if (NS_FAILED(rv)) return rv;

    return storageStream->NewInputStream(0, result);
}

nsresult
nsAboutCacheEntry::OpenCacheEntry(nsIURI *uri, nsICacheEntryDescriptor **result)
{
    nsresult rv;
    nsCAutoString clientID, key;
    PRBool streamBased = PR_TRUE;

    rv = ParseURI(uri, clientID, streamBased, key);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsICacheService> serv =
        do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsICacheSession> session;
    rv = serv->CreateSession(clientID.get(),
                             nsICache::STORE_ANYWHERE,
                             streamBased,
                             getter_AddRefs(session));
    if (NS_FAILED(rv)) return rv;

    rv = session->SetDoomEntriesIfExpired(PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    rv = session->OpenCacheEntry(key, nsICache::ACCESS_READ, PR_FALSE, result);
    return rv;
}






static PRTime SecondsToPRTime(PRUint32 t_sec)
{
    PRTime t_usec, usec_per_sec;
    LL_I2L(t_usec, t_sec);
    LL_I2L(usec_per_sec, PR_USEC_PER_SEC);
    LL_MUL(t_usec, t_usec, usec_per_sec);
    return t_usec;
}
static void PrintTimeString(char *buf, PRUint32 bufsize, PRUint32 t_sec)
{
    PRExplodedTime et;
    PRTime t_usec = SecondsToPRTime(t_sec);
    PR_ExplodeTime(t_usec, PR_LocalTimeParameters, &et);
    PR_FormatTime(buf, bufsize, "%Y-%m-%d %H:%M:%S", &et);
}

#define APPEND_ROW(label, value) \
    PR_BEGIN_MACRO \
    buffer.AppendLiteral("<tr><td><tt><b>"); \
    buffer.AppendLiteral(label); \
    buffer.AppendLiteral(":</b></tt></td>\n<td><pre>"); \
    buffer.Append(value); \
    buffer.AppendLiteral("</pre></td></tr>\n"); \
    PR_END_MACRO

nsresult
nsAboutCacheEntry::WriteCacheEntryDescription(nsIOutputStream *outputStream,
                                              nsICacheEntryDescriptor *descriptor)
{
    nsresult rv;
    nsCString buffer;
    PRUint32 n;

    nsCAutoString str;

    rv = descriptor->GetKey(str);
    if (NS_FAILED(rv)) return rv;

    buffer.SetCapacity(4096);
    buffer.AssignLiteral("<table>"
                         "<tr><td><tt><b>key:</b></tt></td><td>");

    
    nsCOMPtr<nsIURI> uri;
    PRBool isJS = PR_FALSE;
    PRBool isData = PR_FALSE;

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
    }
    else
        buffer.Append(escapedStr);
    nsMemory::Free(escapedStr);
    buffer.AppendLiteral("</td></tr>\n");


    
    char timeBuf[255];
    PRUint32 u = 0;
    PRInt32  i = 0;
    nsCAutoString s;

    
    s.Truncate();
    descriptor->GetFetchCount(&i);
    s.AppendInt(i);
    APPEND_ROW("fetch count", s);

    
    descriptor->GetLastFetched(&u);
    if (u) {
        PrintTimeString(timeBuf, sizeof(timeBuf), u);
        APPEND_ROW("last fetched", timeBuf);
    } else {
        APPEND_ROW("last fetched", "No last fetch time");
    }

    
    descriptor->GetLastModified(&u);
    if (u) {
        PrintTimeString(timeBuf, sizeof(timeBuf), u);
        APPEND_ROW("last modified", timeBuf);
    } else {
        APPEND_ROW("last modified", "No last modified time");
    }

    
    descriptor->GetExpirationTime(&u);
    if (u < 0xFFFFFFFF) {
        PrintTimeString(timeBuf, sizeof(timeBuf), u);
        APPEND_ROW("expires", timeBuf);
    } else {
        APPEND_ROW("expires", "No expiration time");
    }

    
    s.Truncate();
    PRUint32 dataSize;
    descriptor->GetDataSize(&dataSize);
    s.AppendInt((PRInt32)dataSize);     
    APPEND_ROW("Data size", s);

    

    

    
    
    nsCOMPtr<nsIFile> cacheFile;
    rv = descriptor->GetFile(getter_AddRefs(cacheFile));
    if (NS_SUCCEEDED(rv)) {
        nsAutoString filePath;
        cacheFile->GetPath(filePath);
        APPEND_ROW("file on disk", NS_ConvertUTF16toUTF8(filePath));
    }
    else
        APPEND_ROW("file on disk", "none");

    
    nsCOMPtr<nsISupports> securityInfo;
    descriptor->GetSecurityInfo(getter_AddRefs(securityInfo));
    if (securityInfo) {
        APPEND_ROW("Security", "This is a secure document.");
    } else {
        APPEND_ROW("Security",
                   "This document does not have any security info associated with it.");
    }

    buffer.AppendLiteral("</table>\n"
                         "<hr />\n<table>");
    
    

    
    nsXPIDLCString str2;
    descriptor->GetClientID(getter_Copies(str2));
    if (!str2.IsEmpty())  APPEND_ROW("Client", str2);


    mBuffer = &buffer;  
    descriptor->VisitMetaData(this);
    mBuffer = nsnull;

    buffer.AppendLiteral("</table>\n"
                         "<hr />\n<pre>");
    outputStream->Write(buffer.get(), buffer.Length(), &n);

    buffer.Truncate();

    
    nsCOMPtr<nsIInputStream> stream;
    descriptor->OpenInputStream(0, getter_AddRefs(stream));
    if (stream) {
        PRUint32 hexDumpState = 0;
        char chunk[4096];
        while (dataSize) {
            PRUint32 count = PR_MIN(dataSize, sizeof(chunk));
            if (NS_FAILED(stream->Read(chunk, count, &n)) || n == 0)
                break;
            HexDump(&hexDumpState, chunk, n, buffer);
            outputStream->Write(buffer.get(), buffer.Length(), &n);
            dataSize -= n;
        }
    }

    buffer.AssignLiteral("</pre>");
    outputStream->Write(buffer.get(), buffer.Length(), &n);
    return NS_OK;
}

nsresult
nsAboutCacheEntry::WriteCacheEntryUnavailable(nsIOutputStream *outputStream)
{
    PRUint32 n;
    NS_NAMED_LITERAL_CSTRING(buffer,
        "The cache entry you selected is not available.");
    outputStream->Write(buffer.get(), buffer.Length(), &n);
    return NS_OK;
}

nsresult
nsAboutCacheEntry::ParseURI(nsIURI *uri, nsCString &clientID,
                            PRBool &streamBased, nsCString &key)
{
    
    
    
    nsresult rv;

    nsCAutoString path;
    rv = uri->GetPath(path);
    if (NS_FAILED(rv)) return rv;

    nsACString::const_iterator i1, i2, i3, end;
    path.BeginReading(i1);
    path.EndReading(end);

    i2 = end;
    if (!FindInReadable(NS_LITERAL_CSTRING("?client="), i1, i2))
        return NS_ERROR_FAILURE;
    

    i1 = i2;
    i3 = end;
    if (!FindInReadable(NS_LITERAL_CSTRING("&sb="), i1, i3))
        return NS_ERROR_FAILURE;
    
    

    clientID.Assign(Substring(i2, i1));

    i1 = i3;
    i2 = end;
    if (!FindInReadable(NS_LITERAL_CSTRING("&key="), i1, i2))
        return NS_ERROR_FAILURE;
    
    

    streamBased = FindCharInReadable('1', i3, i1);
    key.Assign(Substring(i2, end));

    return NS_OK;
}






NS_IMETHODIMP
nsAboutCacheEntry::VisitMetaDataElement(const char * key,
                                        const char * value,
                                        PRBool *     keepGoing)
{
    mBuffer->AppendLiteral("<tr><td><tt><b>");
    mBuffer->Append(key);
    mBuffer->AppendLiteral(":</b></tt></td>\n<td><pre>");
    char* escapedValue = nsEscapeHTML(value);
    mBuffer->Append(escapedValue);
    nsMemory::Free(escapedValue);
    mBuffer->AppendLiteral("</pre></td></tr>\n");

    *keepGoing = PR_TRUE;
    return NS_OK;
}

