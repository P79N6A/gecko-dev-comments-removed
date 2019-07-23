




































#include "nsAString.h"
#include "nsNetUtil.h"
#include "nsMediaDecoder.h"
#include "nsIScriptSecurityManager.h"
#include "nsChannelToPipeListener.h"
#include "nsICachingChannel.h"

#define HTTP_OK_CODE 200
#define HTTP_PARTIAL_RESPONSE_CODE 206

nsChannelToPipeListener::nsChannelToPipeListener(
    nsMediaDecoder* aDecoder,
    PRBool aSeeking,
    PRInt64 aOffset) :
  mDecoder(aDecoder),
  mIntervalStart(0),
  mIntervalEnd(0),
  mOffset(aOffset),
  mTotalBytes(0),
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

double nsChannelToPipeListener::BytesPerSecond() const
{
  return mOutput ? mTotalBytes / ((PR_IntervalToMilliseconds(mIntervalEnd-mIntervalStart)) / 1000.0) : NS_MEDIA_UNKNOWN_RATE;
}

nsresult nsChannelToPipeListener::GetInputStream(nsIInputStream** aStream)
{
  NS_IF_ADDREF(*aStream = mInput);
  return NS_OK;
}

nsresult nsChannelToPipeListener::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  mIntervalStart = PR_IntervalNow();
  mIntervalEnd = mIntervalStart;
  mTotalBytes = 0;
  mDecoder->UpdateBytesDownloaded(mOffset);
  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(aRequest);
  if (hc) {
    nsCAutoString ranges;
    nsresult rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("Accept-Ranges"),
                                        ranges);
    PRBool acceptsRanges = ranges.EqualsLiteral("bytes"); 

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
  if (aStatus != NS_BINDING_ABORTED && mDecoder) {
    if (NS_SUCCEEDED(aStatus)) {
      mDecoder->ResourceLoaded();
    } else if (aStatus != NS_BASE_STREAM_CLOSED) {
      mDecoder->NetworkError();
    }
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

  PRUint32 bytes = 0;
  
  do {
    nsresult rv = mOutput->WriteFrom(aStream, aCount, &bytes);
    if (NS_FAILED(rv))
      return rv;
    
    aCount -= bytes;
    mTotalBytes += bytes;
    mDecoder->UpdateBytesDownloaded(mOffset + aOffset + bytes);
  } while (aCount) ;
  
  nsresult rv = mOutput->Flush();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  mDecoder->Progress(PR_FALSE);

  mIntervalEnd = PR_IntervalNow();
  return NS_OK;
}

nsIPrincipal*
nsChannelToPipeListener::GetCurrentPrincipal()
{
  return mPrincipal;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsChannelToPipeListener, nsIRequestObserver, nsIStreamListener)

