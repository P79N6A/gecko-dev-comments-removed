




































#include "nsAString.h"
#include "nsNetUtil.h"
#include "nsMediaDecoder.h"
#include "nsIScriptSecurityManager.h"
#include "nsChannelToPipeListener.h"
#include "nsICachingChannel.h"
#include "nsDOMError.h"
#include "nsHTMLMediaElement.h"

#define HTTP_OK_CODE 200
#define HTTP_PARTIAL_RESPONSE_CODE 206

nsChannelToPipeListener::nsChannelToPipeListener(
    nsMediaDecoder* aDecoder,
    PRBool aSeeking) :
  mDecoder(aDecoder),
  mSeeking(aSeeking)
{
}

nsresult nsChannelToPipeListener::Init() 
{
  nsresult rv = NS_NewPipe(getter_AddRefs(mInput), 
                           getter_AddRefs(mOutput),
                           0, 
                           PR_UINT32_MAX);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void nsChannelToPipeListener::Stop()
{
  mDecoder = nsnull;
  mInput = nsnull;
  mOutput = nsnull;
}

void nsChannelToPipeListener::Cancel()
{
  if (mOutput)
    mOutput->Close();

  if (mInput)
    mInput->Close();
}

nsresult nsChannelToPipeListener::GetInputStream(nsIInputStream** aStream)
{
  NS_IF_ADDREF(*aStream = mInput);
  return NS_OK;
}

nsresult nsChannelToPipeListener::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  nsHTMLMediaElement* element = mDecoder->GetMediaElement();
  NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);
  if (element->ShouldCheckAllowOrigin()) {
    
    
    nsresult status;
    nsresult rv = aRequest->GetStatus(&status);
    if (NS_FAILED(rv) || status == NS_ERROR_DOM_BAD_URI) {
      mDecoder->NetworkError();
      return NS_ERROR_DOM_BAD_URI;
    }
  }

  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(aRequest);
  if (hc) {
    nsCAutoString ranges;
    hc->GetResponseHeader(NS_LITERAL_CSTRING("Accept-Ranges"),
                          ranges);
    PRBool acceptsRanges = ranges.EqualsLiteral("bytes"); 

    if (!mSeeking) {
      
      
      
      
      
      
      nsCAutoString durationText;
      PRInt32 ec = 0;
      nsresult rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("X-Content-Duration"), durationText);
      if (NS_FAILED(rv)) {
        rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("X-AMZ-Meta-Content-Duration"), durationText);
      }

      if (NS_SUCCEEDED(rv)) {
        float duration = durationText.ToFloat(&ec);
        if (ec == NS_OK && duration >= 0) {
          mDecoder->SetDuration(PRInt64(NS_round(duration*1000)));
        }
      }
    }
 
    PRUint32 responseStatus = 0; 
    hc->GetResponseStatus(&responseStatus);
    if (mSeeking && responseStatus == HTTP_OK_CODE) {
      
      
      
      
      mDecoder->SetSeekable(acceptsRanges);
    }
    else if (!mSeeking && 
             (responseStatus == HTTP_OK_CODE ||
              responseStatus == HTTP_PARTIAL_RESPONSE_CODE)) {
      
      
      PRInt32 cl = 0;
      hc->GetContentLength(&cl);
      mDecoder->SetTotalBytes(cl);

      
      
      
      mDecoder->SetSeekable(responseStatus == HTTP_PARTIAL_RESPONSE_CODE ||
                            acceptsRanges);
    }
  }

  nsCOMPtr<nsICachingChannel> cc = do_QueryInterface(aRequest);
  if (cc) {
    PRBool fromCache = PR_FALSE;
    nsresult rv = cc->IsFromCache(&fromCache);
    if (NS_SUCCEEDED(rv) && !fromCache) {
      cc->SetCacheAsFile(PR_TRUE);
    }
  }

  
  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  if (chan) {
    nsCOMPtr<nsIScriptSecurityManager> secMan =
      do_GetService("@mozilla.org/scriptsecuritymanager;1");
    if (secMan) {
      nsresult rv = secMan->GetChannelPrincipal(chan,
                                                getter_AddRefs(mPrincipal));
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }

  
  
  mDecoder->Progress(PR_FALSE);

  return NS_OK;
}

nsresult nsChannelToPipeListener::OnStopRequest(nsIRequest* aRequest, nsISupports* aContext, nsresult aStatus) 
{
  mOutput = nsnull;
  if (mDecoder) {
    mDecoder->NotifyDownloadEnded(aStatus);
  }
  return NS_OK;
}

nsresult nsChannelToPipeListener::OnDataAvailable(nsIRequest* aRequest, 
                                                nsISupports* aContext, 
                                                nsIInputStream* aStream,
                                                PRUint32 aOffset,
                                                PRUint32 aCount)
{
  if (!mOutput)
    return NS_ERROR_FAILURE;

  mDecoder->NotifyBytesDownloaded(aCount);

  do {
    PRUint32 bytes;
    nsresult rv = mOutput->WriteFrom(aStream, aCount, &bytes);
    if (NS_FAILED(rv))
      return rv;
    
    aCount -= bytes;
  } while (aCount);
  
  nsresult rv = mOutput->Flush();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  mDecoder->Progress(PR_FALSE);
  return NS_OK;
}

nsIPrincipal*
nsChannelToPipeListener::GetCurrentPrincipal()
{
  return mPrincipal;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsChannelToPipeListener, nsIRequestObserver, nsIStreamListener)

