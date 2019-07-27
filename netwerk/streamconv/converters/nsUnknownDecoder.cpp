




#include "nsUnknownDecoder.h"
#include "nsIPipe.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsMimeTypes.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "nsCRT.h"

#include "nsIMIMEService.h"

#include "nsIViewSourceChannel.h"
#include "nsIHttpChannel.h"
#include "nsIForcePendingChannel.h"
#include "nsIEncodedChannel.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"

#include <algorithm>

#define MAX_BUFFER_SIZE 512u

NS_IMPL_ISUPPORTS(nsUnknownDecoder::ConvertedStreamListener,
                  nsIStreamListener,
                  nsIRequestObserver)

nsUnknownDecoder::ConvertedStreamListener::
                  ConvertedStreamListener(nsUnknownDecoder *aDecoder)
{
  mDecoder = aDecoder;
}

nsUnknownDecoder::ConvertedStreamListener::~ConvertedStreamListener()
{
}

NS_IMETHODIMP
nsUnknownDecoder::ConvertedStreamListener::
                  AppendDataToString(nsIInputStream* inputStream,
                                     void* closure,
                                     const char* rawSegment,
                                     uint32_t toOffset,
                                     uint32_t count,
                                     uint32_t* writeCount)
{
  nsCString* decodedData = static_cast<nsCString*>(closure);
  decodedData->Append(rawSegment, count);
  *writeCount = count;
  return NS_OK;
}

NS_IMETHODIMP
nsUnknownDecoder::ConvertedStreamListener::OnStartRequest(nsIRequest* request,
                                                          nsISupports* context)
{
  return NS_OK;
}

NS_IMETHODIMP
nsUnknownDecoder::ConvertedStreamListener::
                  OnDataAvailable(nsIRequest* request,
                                  nsISupports* context,
                                  nsIInputStream* stream,
                                  uint64_t offset,
                                  uint32_t count)
{
  uint32_t read;
  return stream->ReadSegments(AppendDataToString, &mDecoder->mDecodedData, count,
                              &read);
}

NS_IMETHODIMP
nsUnknownDecoder::ConvertedStreamListener::OnStopRequest(nsIRequest* request,
                                                         nsISupports* context,
                                                         nsresult status)
{
  return NS_OK;
}

nsUnknownDecoder::nsUnknownDecoder()
  : mBuffer(nullptr)
  , mBufferLen(0)
  , mRequireHTMLsuffix(false)
  , mDecodedData("")
{
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    bool val;
    if (NS_SUCCEEDED(prefs->GetBoolPref("security.requireHTMLsuffix", &val)))
      mRequireHTMLsuffix = val;
  }
}

nsUnknownDecoder::~nsUnknownDecoder()
{
  if (mBuffer) {
    delete [] mBuffer;
    mBuffer = nullptr;
  }
}







NS_IMPL_ADDREF(nsUnknownDecoder)
NS_IMPL_RELEASE(nsUnknownDecoder)

NS_INTERFACE_MAP_BEGIN(nsUnknownDecoder)
   NS_INTERFACE_MAP_ENTRY(nsIStreamConverter)
   NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
   NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
   NS_INTERFACE_MAP_ENTRY(nsIContentSniffer)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStreamListener)
NS_INTERFACE_MAP_END








NS_IMETHODIMP
nsUnknownDecoder::Convert(nsIInputStream *aFromStream,
                          const char *aFromType,
                          const char *aToType,
                          nsISupports *aCtxt, 
                          nsIInputStream **aResultStream) 
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsUnknownDecoder::AsyncConvertData(const char *aFromType, 
                                   const char *aToType,
                                   nsIStreamListener *aListener, 
                                   nsISupports *aCtxt)
{
  NS_ASSERTION(aListener && aFromType && aToType, 
               "null pointer passed into multi mixed converter");
  
  
  
  mNextListener = aListener;
  return (aListener) ? NS_OK : NS_ERROR_FAILURE;
}







NS_IMETHODIMP
nsUnknownDecoder::OnDataAvailable(nsIRequest* request, 
                                  nsISupports *aCtxt,
                                  nsIInputStream *aStream, 
                                  uint64_t aSourceOffset, 
                                  uint32_t aCount)
{
  nsresult rv = NS_OK;

  if (!mNextListener) return NS_ERROR_FAILURE;

  if (mContentType.IsEmpty()) {
    uint32_t count, len;

    
    if (!mBuffer) return NS_ERROR_OUT_OF_MEMORY;

    
    
    
    
    if (mBufferLen + aCount >= MAX_BUFFER_SIZE) {
      count = MAX_BUFFER_SIZE-mBufferLen;
    } else {
      count = aCount;
    }

    
    rv = aStream->Read((mBuffer+mBufferLen), count, &len);
    if (NS_FAILED(rv)) return rv;

    mBufferLen += len;
    aCount     -= len;

    if (aCount) {
      
      
      
      
      
      aSourceOffset += mBufferLen;

      DetermineContentType(request);

      rv = FireListenerNotifications(request, aCtxt);
    }
  }

  
  if (aCount && NS_SUCCEEDED(rv)) {
    NS_ASSERTION(!mContentType.IsEmpty(), 
                 "Content type should be known by now.");

    rv = mNextListener->OnDataAvailable(request, aCtxt, aStream, 
                                        aSourceOffset, aCount);
  }

  return rv;
}







NS_IMETHODIMP
nsUnknownDecoder::OnStartRequest(nsIRequest* request, nsISupports *aCtxt) 
{
  nsresult rv = NS_OK;

  if (!mNextListener) return NS_ERROR_FAILURE;

  
  if (NS_SUCCEEDED(rv) && !mBuffer) {
    mBuffer = new char[MAX_BUFFER_SIZE];

    if (!mBuffer) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  return rv;
}

NS_IMETHODIMP
nsUnknownDecoder::OnStopRequest(nsIRequest* request, nsISupports *aCtxt,
                                nsresult aStatus)
{
  nsresult rv = NS_OK;

  if (!mNextListener) return NS_ERROR_FAILURE;

  
  
  
  
  if (mContentType.IsEmpty()) {
    DetermineContentType(request);

    
    
    
    nsCOMPtr<nsIForcePendingChannel> forcePendingChannel = do_QueryInterface(request);
    if (forcePendingChannel) {
      forcePendingChannel->ForcePending(true);
    }

    rv = FireListenerNotifications(request, aCtxt);

    if (NS_FAILED(rv)) {
      aStatus = rv;
    }

    
    if (forcePendingChannel) {
      forcePendingChannel->ForcePending(false);
    }
  }

  rv = mNextListener->OnStopRequest(request, aCtxt, aStatus);
  mNextListener = 0;

  return rv;
}






NS_IMETHODIMP
nsUnknownDecoder::GetMIMETypeFromContent(nsIRequest* aRequest,
                                         const uint8_t* aData,
                                         uint32_t aLength,
                                         nsACString& type)
{
  mBuffer = const_cast<char*>(reinterpret_cast<const char*>(aData));
  mBufferLen = aLength;
  DetermineContentType(aRequest);
  mBuffer = nullptr;
  mBufferLen = 0;
  type.Assign(mContentType);
  mContentType.Truncate();
  return type.IsEmpty() ? NS_ERROR_NOT_AVAILABLE : NS_OK;
}




bool nsUnknownDecoder::AllowSniffing(nsIRequest* aRequest)
{
  if (!mRequireHTMLsuffix) {
    return true;
  }
  
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
  if (!channel) {
    NS_ERROR("QI failed");
    return false;
  }

  nsCOMPtr<nsIURI> uri;
  if (NS_FAILED(channel->GetURI(getter_AddRefs(uri))) || !uri) {
    return false;
  }
  
  bool isLocalFile = false;
  if (NS_FAILED(uri->SchemeIs("file", &isLocalFile)) || isLocalFile) {
    return false;
  }

  return true;
}









nsUnknownDecoder::nsSnifferEntry nsUnknownDecoder::sSnifferEntries[] = {
  SNIFFER_ENTRY("%PDF-", APPLICATION_PDF),

  SNIFFER_ENTRY("%!PS-Adobe-", APPLICATION_POSTSCRIPT),

  
  
  SNIFFER_ENTRY("From", TEXT_PLAIN),
  SNIFFER_ENTRY(">From", TEXT_PLAIN),

  
  
  
  
  SNIFFER_ENTRY_WITH_FUNC("#!", &nsUnknownDecoder::LastDitchSniff),

  
  
  SNIFFER_ENTRY_WITH_FUNC("<?xml", &nsUnknownDecoder::SniffForXML)
};

uint32_t nsUnknownDecoder::sSnifferEntryNum =
  sizeof(nsUnknownDecoder::sSnifferEntries) /
    sizeof(nsUnknownDecoder::nsSnifferEntry);

void nsUnknownDecoder::DetermineContentType(nsIRequest* aRequest)
{
  NS_ASSERTION(mContentType.IsEmpty(), "Content type is already known.");
  if (!mContentType.IsEmpty()) return;

  const char* testData = mBuffer;
  uint32_t testDataLen = mBufferLen;
  
  nsCOMPtr<nsIHttpChannel> channel(do_QueryInterface(aRequest));
  if (channel) {
    nsresult rv = ConvertEncodedData(aRequest, mBuffer, mBufferLen);
    if (NS_SUCCEEDED(rv)) {
      if (!mDecodedData.IsEmpty()) {
        testData = mDecodedData.get();
        testDataLen = std::min(mDecodedData.Length(), MAX_BUFFER_SIZE);
      }
    }
  }

  
  
  uint32_t i;
  for (i = 0; i < sSnifferEntryNum; ++i) {
    if (testDataLen >= sSnifferEntries[i].mByteLen &&  
        memcmp(testData, sSnifferEntries[i].mBytes, sSnifferEntries[i].mByteLen) == 0) {  
      NS_ASSERTION(sSnifferEntries[i].mMimeType ||
                   sSnifferEntries[i].mContentTypeSniffer,
                   "Must have either a type string or a function to set the type");
      NS_ASSERTION(!sSnifferEntries[i].mMimeType ||
                   !sSnifferEntries[i].mContentTypeSniffer,
                   "Both a type string and a type sniffing function set;"
                   " using type string");
      if (sSnifferEntries[i].mMimeType) {
        mContentType = sSnifferEntries[i].mMimeType;
        NS_ASSERTION(!mContentType.IsEmpty(), 
                     "Content type should be known by now.");
        return;
      }
      if ((this->*(sSnifferEntries[i].mContentTypeSniffer))(aRequest)) {
        NS_ASSERTION(!mContentType.IsEmpty(), 
                     "Content type should be known by now.");
        return;
      }        
    }
  }

  NS_SniffContent(NS_DATA_SNIFFER_CATEGORY, aRequest,
                  (const uint8_t*)testData, testDataLen, mContentType);
  if (!mContentType.IsEmpty()) {
    return;
  }

  if (SniffForHTML(aRequest)) {
    NS_ASSERTION(!mContentType.IsEmpty(), 
                 "Content type should be known by now.");
    return;
  }
  
  
  
  if (SniffURI(aRequest)) {
    NS_ASSERTION(!mContentType.IsEmpty(), 
                 "Content type should be known by now.");
    return;
  }
  
  LastDitchSniff(aRequest);
  NS_ASSERTION(!mContentType.IsEmpty(), 
               "Content type should be known by now.");
}

bool nsUnknownDecoder::SniffForHTML(nsIRequest* aRequest)
{
  




  if (!AllowSniffing(aRequest)) {
    return false;
  }

  
  const char* str;
  const char* end;
  if (mDecodedData.IsEmpty()) {
    str = mBuffer;
    end = mBuffer + mBufferLen;
  } else {
    str = mDecodedData.get();
    end = mDecodedData.get() + std::min(mDecodedData.Length(),
                                        MAX_BUFFER_SIZE);
  }

  
  while (str != end && nsCRT::IsAsciiSpace(*str)) {
    ++str;
  }

  
  if (str == end || *str != '<' || ++str == end) {
    return false;
  }

  
  if (*str == '!' || *str == '?') {
    mContentType = TEXT_HTML;
    return true;
  }
  
  uint32_t bufSize = end - str;
  
  
#define MATCHES_TAG(_tagstr)                                              \
  (bufSize >= sizeof(_tagstr) &&                                          \
   (PL_strncasecmp(str, _tagstr " ", sizeof(_tagstr)) == 0 ||             \
    PL_strncasecmp(str, _tagstr ">", sizeof(_tagstr)) == 0))
    
  if (MATCHES_TAG("html")     ||
      MATCHES_TAG("frameset") ||
      MATCHES_TAG("body")     ||
      MATCHES_TAG("head")     ||
      MATCHES_TAG("script")   ||
      MATCHES_TAG("iframe")   ||
      MATCHES_TAG("a")        ||
      MATCHES_TAG("img")      ||
      MATCHES_TAG("table")    ||
      MATCHES_TAG("title")    ||
      MATCHES_TAG("link")     ||
      MATCHES_TAG("base")     ||
      MATCHES_TAG("style")    ||
      MATCHES_TAG("div")      ||
      MATCHES_TAG("p")        ||
      MATCHES_TAG("font")     ||
      MATCHES_TAG("applet")   ||
      MATCHES_TAG("meta")     ||
      MATCHES_TAG("center")   ||
      MATCHES_TAG("form")     ||
      MATCHES_TAG("isindex")  ||
      MATCHES_TAG("h1")       ||
      MATCHES_TAG("h2")       ||
      MATCHES_TAG("h3")       ||
      MATCHES_TAG("h4")       ||
      MATCHES_TAG("h5")       ||
      MATCHES_TAG("h6")       ||
      MATCHES_TAG("b")        ||
      MATCHES_TAG("pre")) {
  
    mContentType = TEXT_HTML;
    return true;
  }

#undef MATCHES_TAG
  
  return false;
}

bool nsUnknownDecoder::SniffForXML(nsIRequest* aRequest)
{
  
  if (!AllowSniffing(aRequest)) {
    return false;
  }

  
  if (!SniffURI(aRequest)) {
    
    mContentType = TEXT_XML;
  }
  
  return true;
}

bool nsUnknownDecoder::SniffURI(nsIRequest* aRequest)
{
  nsCOMPtr<nsIMIMEService> mimeService(do_GetService("@mozilla.org/mime;1"));
  if (mimeService) {
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
    if (channel) {
      nsCOMPtr<nsIURI> uri;
      nsresult result = channel->GetURI(getter_AddRefs(uri));
      if (NS_SUCCEEDED(result) && uri) {
        nsAutoCString type;
        result = mimeService->GetTypeFromURI(uri, type);
        if (NS_SUCCEEDED(result)) {
          mContentType = type;
          return true;
        }
      }
    }
  }

  return false;
}




#define IS_TEXT_CHAR(ch)                                     \
  (((unsigned char)(ch)) > 31 || (9 <= (ch) && (ch) <= 13) || (ch) == 27)

bool nsUnknownDecoder::LastDitchSniff(nsIRequest* aRequest)
{
  
  

  const char* testData;
  uint32_t testDataLen;
  if (mDecodedData.IsEmpty()) {
    testData = mBuffer;
    testDataLen = mBufferLen;
  } else {
    testData = mDecodedData.get();
    testDataLen = std::min(mDecodedData.Length(), MAX_BUFFER_SIZE);
  }

  
  
  
  
  if (testDataLen >= 4) {
    const unsigned char* buf = (const unsigned char*)testData;
    if ((buf[0] == 0xFE && buf[1] == 0xFF) || 
        (buf[0] == 0xFF && buf[1] == 0xFE) || 
        (buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) || 
        (buf[0] == 0 && buf[1] == 0 && buf[2] == 0xFE && buf[3] == 0xFF)) { 
       
      mContentType = TEXT_PLAIN;
      return true;
    }
  }

  
  
  
  uint32_t i;
  for (i = 0; i < testDataLen && IS_TEXT_CHAR(testData[i]); i++) {
    continue;
  }

  if (i == testDataLen) {
    mContentType = TEXT_PLAIN;
  }
  else {
    mContentType = APPLICATION_OCTET_STREAM;
  }

  return true;
}


nsresult nsUnknownDecoder::FireListenerNotifications(nsIRequest* request,
                                                     nsISupports *aCtxt)
{
  nsresult rv = NS_OK;

  if (!mNextListener) return NS_ERROR_FAILURE;

  if (!mContentType.IsEmpty()) {
    nsCOMPtr<nsIViewSourceChannel> viewSourceChannel =
      do_QueryInterface(request);
    if (viewSourceChannel) {
      rv = viewSourceChannel->SetOriginalContentType(mContentType);
    } else {
      nsCOMPtr<nsIChannel> channel = do_QueryInterface(request, &rv);
      if (NS_SUCCEEDED(rv)) {
        
        rv = channel->SetContentType(mContentType);
      }
    }

    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to set content type on channel!");

    if (NS_FAILED(rv)) {
      
      
      request->Cancel(rv);
      mNextListener->OnStartRequest(request, aCtxt);
      return rv;
    }
  }

  
  rv = mNextListener->OnStartRequest(request, aCtxt);

  if (NS_SUCCEEDED(rv)) {
    
    nsCOMPtr<nsIEncodedChannel> encodedChannel = do_QueryInterface(request);
    if (encodedChannel) {
      nsCOMPtr<nsIStreamListener> listener;
      rv = encodedChannel->DoApplyContentConversions(mNextListener, getter_AddRefs(listener), aCtxt);
      if (NS_SUCCEEDED(rv) && listener) {
        mNextListener = listener;
      }
    }
  }

  if (!mBuffer) return NS_ERROR_OUT_OF_MEMORY;

  
  
  if (NS_SUCCEEDED(rv))
    request->GetStatus(&rv);

  
  
  if (NS_SUCCEEDED(rv) && (mBufferLen > 0)) {
    uint32_t len = 0;
    nsCOMPtr<nsIInputStream> in;
    nsCOMPtr<nsIOutputStream> out;

    
    rv = NS_NewPipe(getter_AddRefs(in), getter_AddRefs(out),
                    MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);

    if (NS_SUCCEEDED(rv)) {
      rv = out->Write(mBuffer, mBufferLen, &len);
      if (NS_SUCCEEDED(rv)) {
        if (len == mBufferLen) {
          rv = mNextListener->OnDataAvailable(request, aCtxt, in, 0, len);
        } else {
          NS_ERROR("Unable to write all the data into the pipe.");
          rv = NS_ERROR_FAILURE;
        }
      }
    }
  }

  delete [] mBuffer;
  mBuffer = nullptr;
  mBufferLen = 0;

  return rv;
}


nsresult
nsUnknownDecoder::ConvertEncodedData(nsIRequest* request,
                                     const char* data,
                                     uint32_t length)
{
  nsresult rv = NS_OK;

  mDecodedData = "";
  nsCOMPtr<nsIEncodedChannel> encodedChannel(do_QueryInterface(request));
  if (encodedChannel) {

    nsRefPtr<ConvertedStreamListener> strListener =
      new ConvertedStreamListener(this);

    nsCOMPtr<nsIStreamListener> listener;
    rv = encodedChannel->DoApplyContentConversions(strListener,
                                                   getter_AddRefs(listener),
                                                   nullptr);

    if (NS_FAILED(rv)) {
      return rv;
    }

    if (listener) {
      listener->OnStartRequest(request, nullptr);

      nsCOMPtr<nsIStringInputStream> rawStream =
        do_CreateInstance(NS_STRINGINPUTSTREAM_CONTRACTID);
      if (!rawStream)
        return NS_ERROR_FAILURE;

      rv = rawStream->SetData((const char*)data, length);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = listener->OnDataAvailable(request, nullptr, rawStream, 0,
                                     length);
      NS_ENSURE_SUCCESS(rv, rv);

      listener->OnStopRequest(request, nullptr, NS_OK);
    }
  }
  return rv;
}

void
nsBinaryDetector::DetermineContentType(nsIRequest* aRequest)
{
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aRequest);
  if (!httpChannel) {
    return;
  }

  
  nsAutoCString contentTypeHdr;
  httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Type"),
                                 contentTypeHdr);
  nsAutoCString contentType;
  httpChannel->GetContentType(contentType);

  
  
  
  
  
  
  
  if (!contentType.EqualsLiteral("text/plain") ||
      (!contentTypeHdr.EqualsLiteral("text/plain") &&
       !contentTypeHdr.EqualsLiteral("text/plain; charset=ISO-8859-1") &&
       !contentTypeHdr.EqualsLiteral("text/plain; charset=iso-8859-1") &&
       !contentTypeHdr.EqualsLiteral("text/plain; charset=UTF-8"))) {
    return;
  }

  
  
  
  
  nsAutoCString contentEncoding;
  httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Encoding"),
                                 contentEncoding);
  if (!contentEncoding.IsEmpty()) {
    return;
  }
  
  LastDitchSniff(aRequest);
  if (mContentType.Equals(APPLICATION_OCTET_STREAM)) {
    
    mContentType = APPLICATION_GUESS_FROM_EXT;
  } else {
    
    
    mContentType.Truncate();
  }
}
