




































#include "nsAString.h"
#include "nsNetUtil.h"
#include "nsMediaDecoder.h"
#include "nsIScriptSecurityManager.h"
#include "nsChannelToPipeListener.h"

nsChannelToPipeListener::nsChannelToPipeListener(nsMediaDecoder* aDecoder) :
  mDecoder(aDecoder),
  mIntervalStart(0),
  mIntervalEnd(0),
  mTotalBytes(0)
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
  mDecoder->UpdateBytesDownloaded(mTotalBytes);

  
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

  return NS_OK;
}

nsresult nsChannelToPipeListener::OnStopRequest(nsIRequest* aRequest, nsISupports* aContext, nsresult aStatus) 
{
  mOutput = nsnull;
  if (mDecoder) {
    mDecoder->ResourceLoaded();
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
    NS_ENSURE_SUCCESS(rv, rv);
    
    aCount -= bytes;
    mTotalBytes += bytes;
    mDecoder->UpdateBytesDownloaded(mTotalBytes);
  } while (aCount) ;
  
  nsresult rv = mOutput->Flush();
  NS_ENSURE_SUCCESS(rv, rv);
  mIntervalEnd = PR_IntervalNow();
  return NS_OK;
}

nsIPrincipal*
nsChannelToPipeListener::GetCurrentPrincipal()
{
  return mPrincipal;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsChannelToPipeListener, nsIRequestObserver, nsIStreamListener)

