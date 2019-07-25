





































#include "nsFeedSniffer.h"

#include "prmem.h"

#include "nsNetCID.h"
#include "nsXPCOM.h"
#include "nsCOMPtr.h"
#include "nsStringStream.h"

#include "nsBrowserCompsCID.h"

#include "nsICategoryManager.h"
#include "nsIServiceManager.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"

#include "nsIStreamConverterService.h"
#include "nsIStreamConverter.h"

#include "nsIStreamListener.h"

#include "nsIHttpChannel.h"
#include "nsIMIMEHeaderParam.h"

#include "nsMimeTypes.h"

#define TYPE_ATOM "application/atom+xml"
#define TYPE_RSS "application/rss+xml"
#define TYPE_MAYBE_FEED "application/vnd.mozilla.maybe.feed"

#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RSS "http://purl.org/rss/1.0/"

#define MAX_BYTES 512

NS_IMPL_ISUPPORTS3(nsFeedSniffer,
                   nsIContentSniffer,
                   nsIStreamListener,
                   nsIRequestObserver)

nsresult
nsFeedSniffer::ConvertEncodedData(nsIRequest* request,
                                  const PRUint8* data,
                                  PRUint32 length)
{
  nsresult rv = NS_OK;

 mDecodedData = "";
 nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(request));
  if (!httpChannel)
    return NS_ERROR_NO_INTERFACE;

  nsCAutoString contentEncoding;
  httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Encoding"), 
                                 contentEncoding);
  if (!contentEncoding.IsEmpty()) {
    nsCOMPtr<nsIStreamConverterService> converterService(do_GetService(NS_STREAMCONVERTERSERVICE_CONTRACTID));
    if (converterService) {
      ToLowerCase(contentEncoding);

      nsCOMPtr<nsIStreamListener> converter;
      rv = converterService->AsyncConvertData(contentEncoding.get(), 
                                              "uncompressed", this, nsnull, 
                                              getter_AddRefs(converter));
      NS_ENSURE_SUCCESS(rv, rv);

      converter->OnStartRequest(request, nsnull);

      nsCOMPtr<nsIStringInputStream> rawStream =
        do_CreateInstance(NS_STRINGINPUTSTREAM_CONTRACTID);
      if (!rawStream)
        return NS_ERROR_FAILURE;

      rv = rawStream->SetData((const char*)data, length);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = converter->OnDataAvailable(request, nsnull, rawStream, 0, length);
      NS_ENSURE_SUCCESS(rv, rv);

      converter->OnStopRequest(request, nsnull, NS_OK);
    }
  }
  return rv;
}

template<int N>
static bool
StringBeginsWithLowercaseLiteral(nsAString& aString,
                                 const char (&aSubstring)[N])
{
  return StringHead(aString, N).LowerCaseEqualsLiteral(aSubstring);
}

bool
HasAttachmentDisposition(nsIHttpChannel* httpChannel)
{
  if (!httpChannel)
    return PR_FALSE;

  PRUint32 disp;
  nsresult rv = httpChannel->GetContentDisposition(&disp);

  if (NS_SUCCEEDED(rv) && disp == nsIChannel::DISPOSITION_ATTACHMENT)
    return PR_TRUE;

  return PR_FALSE;
}





static const char*
FindChar(char c, const char *begin, const char *end)
{
  for (; begin < end; ++begin) {
    if (*begin == c)
      return begin;
  }
  return nsnull;
}



















static bool
IsDocumentElement(const char *start, const char* end)
{
  
  
  while ( (start = FindChar('<', start, end)) ) {
    ++start;
    if (start >= end)
      return PR_FALSE;

    
    
    
    if (*start != '?' && *start != '!')
      return PR_FALSE;
    
    
    
    
    start = FindChar('>', start, end);
    if (!start)
      return PR_FALSE;

    ++start;
  }
  return PR_TRUE;
}











static bool
ContainsTopLevelSubstring(nsACString& dataString, const char *substring) 
{
  PRInt32 offset = dataString.Find(substring);
  if (offset == -1)
    return PR_FALSE;

  const char *begin = dataString.BeginReading();

  
  return IsDocumentElement(begin, begin + offset);
}

NS_IMETHODIMP
nsFeedSniffer::GetMIMETypeFromContent(nsIRequest* request, 
                                      const PRUint8* data, 
                                      PRUint32 length, 
                                      nsACString& sniffedType)
{
  nsCOMPtr<nsIHttpChannel> channel(do_QueryInterface(request));
  if (!channel)
    return NS_ERROR_NO_INTERFACE;

  
  nsCAutoString method;
  channel->GetRequestMethod(method);
  if (!method.Equals("GET")) {
    sniffedType.Truncate();
    return NS_OK;
  }

  
  
  
  
  
  
  
  nsCOMPtr<nsIURI> originalURI;
  channel->GetOriginalURI(getter_AddRefs(originalURI));

  nsCAutoString scheme;
  originalURI->GetScheme(scheme);
  if (scheme.EqualsLiteral("view-source")) {
    sniffedType.Truncate();
    return NS_OK;
  }

  
  
  
  
  nsCAutoString contentType;
  channel->GetContentType(contentType);
  bool noSniff = contentType.EqualsLiteral(TYPE_RSS) ||
                   contentType.EqualsLiteral(TYPE_ATOM);

  
  
  
  if (!noSniff) {
    nsCAutoString sniffHeader;
    nsresult foundHeader =
      channel->GetRequestHeader(NS_LITERAL_CSTRING("X-Moz-Is-Feed"),
                                sniffHeader);
    noSniff = NS_SUCCEEDED(foundHeader);
  }

  if (noSniff) {
    
    if(HasAttachmentDisposition(channel)) {
      sniffedType.Truncate();
      return NS_OK;
    }

    
    
    channel->SetResponseHeader(NS_LITERAL_CSTRING("X-Moz-Is-Feed"),
                               NS_LITERAL_CSTRING("1"), PR_FALSE);
    sniffedType.AssignLiteral(TYPE_MAYBE_FEED);
    return NS_OK;
  }

  
  
  if (!contentType.EqualsLiteral(TEXT_HTML) &&
      !contentType.EqualsLiteral(APPLICATION_OCTET_STREAM) &&
      
      
      contentType.Find("xml") == -1) {
    sniffedType.Truncate();
    return NS_OK;
  }

  
  
  nsresult rv = ConvertEncodedData(request, data, length);
  if (NS_FAILED(rv))
    return rv;
  
  const char* testData = 
    mDecodedData.IsEmpty() ? (const char*)data : mDecodedData.get();

  
  
  

  
  
  
  if (length > MAX_BYTES)
    length = MAX_BYTES;

  
  nsDependentCSubstring dataString((const char*)testData, length);

  bool isFeed = false;

  
  isFeed = ContainsTopLevelSubstring(dataString, "<rss");

  
  if (!isFeed)
    isFeed = ContainsTopLevelSubstring(dataString, "<feed");

  
  if (!isFeed) {
    isFeed = ContainsTopLevelSubstring(dataString, "<rdf:RDF") &&
      dataString.Find(NS_RDF) != -1 &&
      dataString.Find(NS_RSS) != -1;
  }

  
  if (isFeed && !HasAttachmentDisposition(channel))
    sniffedType.AssignLiteral(TYPE_MAYBE_FEED);
  else
    sniffedType.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsFeedSniffer::OnStartRequest(nsIRequest* request, nsISupports* context)
{
  return NS_OK;
}

NS_METHOD
nsFeedSniffer::AppendSegmentToString(nsIInputStream* inputStream,
                                     void* closure,
                                     const char* rawSegment,
                                     PRUint32 toOffset,
                                     PRUint32 count,
                                     PRUint32* writeCount)
{
  nsCString* decodedData = static_cast<nsCString*>(closure);
  decodedData->Append(rawSegment, count);
  *writeCount = count;
  return NS_OK;
}

NS_IMETHODIMP
nsFeedSniffer::OnDataAvailable(nsIRequest* request, nsISupports* context,
                               nsIInputStream* stream, PRUint32 offset, 
                               PRUint32 count)
{
  PRUint32 read;
  return stream->ReadSegments(AppendSegmentToString, &mDecodedData, count, 
                              &read);
}

NS_IMETHODIMP
nsFeedSniffer::OnStopRequest(nsIRequest* request, nsISupports* context, 
                             nsresult status)
{
  return NS_OK; 
}
