




































#include "nsUnknownDecoder.h"
#include "nsIServiceManager.h"
#include "nsIStreamConverterService.h"

#include "nsIPipe.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsMimeTypes.h"
#include "netCore.h"
#include "nsXPIDLString.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsICategoryManager.h"
#include "nsISupportsPrimitives.h"
#include "nsIContentSniffer.h"

#include "nsCRT.h"

#include "nsIMIMEService.h"

#include "nsIViewSourceChannel.h"
#include "nsIHttpChannel.h"
#include "nsNetCID.h"


#define MAX_BUFFER_SIZE 1024

nsUnknownDecoder::nsUnknownDecoder()
  : mBuffer(nsnull)
  , mBufferLen(0)
  , mRequireHTMLsuffix(PR_FALSE)
{
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    PRBool val;
    if (NS_SUCCEEDED(prefs->GetBoolPref("security.requireHTMLsuffix", &val)))
      mRequireHTMLsuffix = val;
  }
}

nsUnknownDecoder::~nsUnknownDecoder()
{
  if (mBuffer) {
    delete [] mBuffer;
    mBuffer = nsnull;
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
                                  PRUint32 aSourceOffset, 
                                  PRUint32 aCount)
{
  nsresult rv = NS_OK;

  if (!mNextListener) return NS_ERROR_FAILURE;

  if (mContentType.IsEmpty()) {
    PRUint32 count, len;

    
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

    rv = FireListenerNotifications(request, aCtxt);

    if (NS_FAILED(rv)) {
      aStatus = rv;
    }
  }

  rv = mNextListener->OnStopRequest(request, aCtxt, aStatus);
  mNextListener = 0;

  return rv;
}






NS_IMETHODIMP
nsUnknownDecoder::GetMIMETypeFromContent(nsIRequest* aRequest,
                                         const PRUint8* aData,
                                         PRUint32 aLength,
                                         nsACString& type)
{
  mBuffer = const_cast<char*>(reinterpret_cast<const char*>(aData));
  mBufferLen = aLength;
  DetermineContentType(aRequest);
  mBuffer = nsnull;
  mBufferLen = 0;
  type.Assign(mContentType);
  mContentType.Truncate();
  return NS_OK;
}




PRBool nsUnknownDecoder::AllowSniffing(nsIRequest* aRequest)
{
  if (!mRequireHTMLsuffix) {
    return PR_TRUE;
  }
  
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
  if (!channel) {
    NS_ERROR("QI failed");
    return PR_FALSE;
  }

  nsCOMPtr<nsIURI> uri;
  if (NS_FAILED(channel->GetURI(getter_AddRefs(uri))) || !uri) {
    return PR_FALSE;
  }
  
  PRBool isLocalFile = PR_FALSE;
  if (NS_FAILED(uri->SchemeIs("file", &isLocalFile)) || isLocalFile) {
    return PR_FALSE;
  }

  return PR_TRUE;
}









nsUnknownDecoder::nsSnifferEntry nsUnknownDecoder::sSnifferEntries[] = {
  SNIFFER_ENTRY("%PDF-", APPLICATION_PDF),

  SNIFFER_ENTRY("%!PS-Adobe-", APPLICATION_POSTSCRIPT),
  SNIFFER_ENTRY("%! PS-Adobe-", APPLICATION_POSTSCRIPT),

  
  
  SNIFFER_ENTRY("From", TEXT_PLAIN),
  SNIFFER_ENTRY(">From", TEXT_PLAIN),

  
  
  
  
  SNIFFER_ENTRY_WITH_FUNC("#!", &nsUnknownDecoder::LastDitchSniff),

  
  
  SNIFFER_ENTRY_WITH_FUNC("<?xml", &nsUnknownDecoder::SniffForXML)
};

PRUint32 nsUnknownDecoder::sSnifferEntryNum =
  sizeof(nsUnknownDecoder::sSnifferEntries) /
    sizeof(nsUnknownDecoder::nsSnifferEntry);

void nsUnknownDecoder::DetermineContentType(nsIRequest* aRequest)
{
  NS_ASSERTION(mContentType.IsEmpty(), "Content type is already known.");
  if (!mContentType.IsEmpty()) return;

  
  
  PRUint32 i;
  for (i = 0; i < sSnifferEntryNum; ++i) {
    if (mBufferLen >= sSnifferEntries[i].mByteLen &&  
        memcmp(mBuffer, sSnifferEntries[i].mBytes, sSnifferEntries[i].mByteLen) == 0) {  
      NS_ASSERTION(sSnifferEntries[i].mMimeType ||
                   sSnifferEntries[i].mContentTypeSniffer,
                   "Must have either a type string or a function to set the type");
      NS_ASSERTION(sSnifferEntries[i].mMimeType == nsnull ||
                   sSnifferEntries[i].mContentTypeSniffer == nsnull,
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

  if (TryContentSniffers(aRequest)) {
    NS_ASSERTION(!mContentType.IsEmpty(), 
                 "Content type should be known by now.");
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

PRBool nsUnknownDecoder::TryContentSniffers(nsIRequest* aRequest)
{
  
  nsCOMPtr<nsICategoryManager> catMan(do_GetService("@mozilla.org/categorymanager;1"));
  if (!catMan) {
    return PR_FALSE;
  }

  nsCOMPtr<nsISimpleEnumerator> sniffers;
  catMan->EnumerateCategory("content-sniffing-services", getter_AddRefs(sniffers));
  if (!sniffers) {
    return PR_FALSE;
  }

  PRBool hasMore;
  while (NS_SUCCEEDED(sniffers->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> elem;
    sniffers->GetNext(getter_AddRefs(elem));
    NS_ASSERTION(elem, "No element even though hasMore returned true!?");

    nsCOMPtr<nsISupportsCString> sniffer_id(do_QueryInterface(elem));
    NS_ASSERTION(sniffer_id, "element is no nsISupportsCString!?");
    nsCAutoString contractid;
    nsresult rv = sniffer_id->GetData(contractid);
    if (NS_FAILED(rv)) {
      continue;
    }

    nsCOMPtr<nsIContentSniffer> sniffer(do_GetService(contractid.get()));
    if (!sniffer) {
      continue;
    }

    rv = sniffer->GetMIMETypeFromContent(aRequest, (const PRUint8*)mBuffer,
                                         mBufferLen, mContentType);
    if (NS_SUCCEEDED(rv)) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

PRBool nsUnknownDecoder::SniffForHTML(nsIRequest* aRequest)
{
  




  if (!AllowSniffing(aRequest)) {
    return PR_FALSE;
  }
  
  
  const char* str = mBuffer;
  const char* end = mBuffer + mBufferLen;

  
  while (str != end && nsCRT::IsAsciiSpace(*str)) {
    ++str;
  }

  
  if (str == end || *str != '<' || ++str == end) {
    return PR_FALSE;
  }

  
  if (*str == '!' || *str == '?') {
    mContentType = TEXT_HTML;
    return PR_TRUE;
  }
  
  PRUint32 bufSize = end - str;
  
  
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
    return PR_TRUE;
  }

#undef MATCHES_TAG
  
  return PR_FALSE;
}

PRBool nsUnknownDecoder::SniffForXML(nsIRequest* aRequest)
{
  
  if (!AllowSniffing(aRequest)) {
    return PR_FALSE;
  }

  
  if (!SniffURI(aRequest)) {
    
    mContentType = TEXT_XML;
  }
  
  return PR_TRUE;
}

PRBool nsUnknownDecoder::SniffURI(nsIRequest* aRequest)
{
  nsCOMPtr<nsIMIMEService> mimeService(do_GetService("@mozilla.org/mime;1"));
  if (mimeService) {
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
    if (channel) {
      nsCOMPtr<nsIURI> uri;
      nsresult result = channel->GetURI(getter_AddRefs(uri));
      if (NS_SUCCEEDED(result) && uri) {
        nsCAutoString type;
        result = mimeService->GetTypeFromURI(uri, type);
        if (NS_SUCCEEDED(result)) {
          mContentType = type;
          return PR_TRUE;
        }
      }
    }
  }

  return PR_FALSE;
}




#define IS_TEXT_CHAR(ch)                                     \
  (((unsigned char)(ch)) > 31 || (9 <= (ch) && (ch) <= 13) || (ch) == 27)

PRBool nsUnknownDecoder::LastDitchSniff(nsIRequest* aRequest)
{
  
  

  
  
  
  
  if (mBufferLen >= 4) {
    const unsigned char* buf = (const unsigned char*)mBuffer;
    if ((buf[0] == 0xFE && buf[1] == 0xFF) || 
        (buf[0] == 0xFF && buf[1] == 0xFE) || 
        (buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) || 
        (buf[0] == 0 && buf[1] == 0 && buf[2] == 0xFE && buf[3] == 0xFF) || 
        (buf[0] == 0 && buf[1] == 0 && buf[2] == 0xFF && buf[3] == 0xFE)) { 
        
      mContentType = TEXT_PLAIN;
      return PR_TRUE;
    }
  }
  
  
  
  
  PRUint32 i;
  for (i=0; i<mBufferLen && IS_TEXT_CHAR(mBuffer[i]); i++);

  if (i == mBufferLen) {
    mContentType = TEXT_PLAIN;
  }
  else {
    mContentType = APPLICATION_OCTET_STREAM;
  }

  return PR_TRUE;    
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

  if (!mBuffer) return NS_ERROR_OUT_OF_MEMORY;

  
  
  if (NS_SUCCEEDED(rv))
    request->GetStatus(&rv);

  
  
  if (NS_SUCCEEDED(rv) && (mBufferLen > 0)) {
    PRUint32 len = 0;
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
  mBuffer = nsnull;
  mBufferLen = 0;

  return rv;
}

void
nsBinaryDetector::DetermineContentType(nsIRequest* aRequest)
{
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aRequest);
  if (!httpChannel) {
    return;
  }

  
  nsCAutoString contentTypeHdr;
  httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Type"),
                                 contentTypeHdr);
  nsCAutoString contentType;
  httpChannel->GetContentType(contentType);

  
  
  
  
  
  
  if (!contentType.EqualsLiteral("text/plain") ||
      (!contentTypeHdr.EqualsLiteral("text/plain") &&
       !contentTypeHdr.EqualsLiteral("text/plain; charset=ISO-8859-1") &&
       !contentTypeHdr.EqualsLiteral("text/plain; charset=iso-8859-1"))) {
    return;
  }

  
  
  
  
  nsCAutoString contentEncoding;
  httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Encoding"),
                                 contentEncoding);
  if (!contentEncoding.IsEmpty()) {
    return;
  }
  
  LastDitchSniff(aRequest);
  if (mContentType.Equals(APPLICATION_OCTET_STREAM)) {
    
    mContentType = APPLICATION_GUESS_FROM_EXT;
  }
}

NS_METHOD
nsBinaryDetector::Register(nsIComponentManager* compMgr, nsIFile* path, 
                           const char* registryLocation,
                           const char* componentType, 
                           const nsModuleComponentInfo *info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) 
     return rv;
 
  return catman->AddCategoryEntry(NS_CONTENT_SNIFFER_CATEGORY,
                                  "Binary Detector", 
                                  NS_BINARYDETECTOR_CONTRACTID,
                                  PR_TRUE, PR_TRUE, nsnull);
}
