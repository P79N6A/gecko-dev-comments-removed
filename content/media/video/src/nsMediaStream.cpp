




































#include "nsDebug.h"
#include "nsMediaStream.h"
#include "nsMediaDecoder.h"
#include "nsNetUtil.h"
#include "nsAutoLock.h"
#include "nsThreadUtils.h"
#include "nsIFile.h"
#include "nsIFileChannel.h"
#include "nsIHttpChannel.h"
#include "nsISeekableStream.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsIScriptSecurityManager.h"
#include "nsChannelToPipeListener.h"




#define SEEK_VS_READ_THRESHOLD (32*1024)

class nsDefaultStreamStrategy : public nsStreamStrategy
{
public:
  nsDefaultStreamStrategy(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    nsStreamStrategy(aDecoder, aChannel, aURI),
    mPosition(0)
  {
  }
  
  
  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult Close();
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();
  virtual PRUint32 Available();
  virtual float    DownloadRate();
  virtual void     Cancel();
  virtual nsIPrincipal* GetCurrentPrincipal();

private:
  
  
  
  
  nsCOMPtr<nsChannelToPipeListener> mListener;

  
  
  nsCOMPtr<nsIInputStream>  mPipeInput;

  
  
  PRInt64 mPosition;
};

nsresult nsDefaultStreamStrategy::Open(nsIStreamListener** aStreamListener)
{
  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  mListener = new nsChannelToPipeListener(mDecoder);
  NS_ENSURE_TRUE(mListener, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mListener->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  if (aStreamListener) {
    *aStreamListener = mListener;
    NS_ADDREF(mListener);
  } else {
    rv = mChannel->AsyncOpen(mListener, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mListener->GetInputStream(getter_AddRefs(mPipeInput));
  NS_ENSURE_SUCCESS(rv, rv);

  mPosition = 0;

  return NS_OK;
}

nsresult nsDefaultStreamStrategy::Close()
{
  nsAutoLock lock(mLock);
  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
  }
  if (mPipeInput) {
    mPipeInput->Close();
    mPipeInput = nsnull;
  }
  mListener = nsnull;
  return NS_OK;
}

nsresult nsDefaultStreamStrategy::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
{
  
  
  
  nsAutoLock lock(mLock);
  if (!mPipeInput)
    return NS_ERROR_FAILURE;

  nsresult rv = mPipeInput->Read(aBuffer, aCount, aBytes);
  NS_ENSURE_SUCCESS(rv, rv);
  mPosition += *aBytes;

  return NS_OK;
}

nsresult nsDefaultStreamStrategy::Seek(PRInt32 aWhence, PRInt64 aOffset) 
{
  
  return NS_ERROR_FAILURE;
}

PRInt64 nsDefaultStreamStrategy::Tell()
{
  return mPosition;
}

PRUint32 nsDefaultStreamStrategy::Available()
{
  
  
  
  nsAutoLock lock(mLock);
  if (!mPipeInput)
    return 0;

  PRUint32 count = 0;
  mPipeInput->Available(&count);
  return count;
}

float nsDefaultStreamStrategy::DownloadRate()
{
  nsAutoLock lock(mLock);
  return mListener ? mListener->BytesPerSecond() : NS_MEDIA_UNKNOWN_RATE;
}

void nsDefaultStreamStrategy::Cancel()
{
  if (mListener)
    mListener->Cancel();
}

nsIPrincipal* nsDefaultStreamStrategy::GetCurrentPrincipal()
{
  if (!mListener)
    return nsnull;

  return mListener->GetCurrentPrincipal();
}

class nsFileStreamStrategy : public nsStreamStrategy
{
public:
  nsFileStreamStrategy(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    nsStreamStrategy(aDecoder, aChannel, aURI)
  {
  }
  
  
  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult Close();
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();
  virtual PRUint32 Available();
  virtual float    DownloadRate();
  virtual nsIPrincipal* GetCurrentPrincipal();

private:
  
  
  nsCOMPtr<nsISeekableStream> mSeekable;

  
  
  nsCOMPtr<nsIInputStream>  mInput;

  
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

nsresult nsFileStreamStrategy::Open(nsIStreamListener** aStreamListener)
{
  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  nsresult rv;
  if (aStreamListener) {
    
    
    
    nsCOMPtr<nsIFileChannel> fc(do_QueryInterface(mChannel));
    if (!fc)
      return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIFile> file; 
    rv = fc->GetFile(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = NS_NewLocalFileInputStream(getter_AddRefs(mInput), file);
  } else {
    rv = mChannel->Open(getter_AddRefs(mInput));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  mSeekable = do_QueryInterface(mInput);
  if (!mSeekable) {
    
    
    
    
    return NS_ERROR_FAILURE;
  }

  
  
  PRUint32 size;
  rv = mInput->Available(&size);
  if (NS_SUCCEEDED(rv)) {
    mDecoder->SetTotalBytes(size);
    mDecoder->UpdateBytesDownloaded(size);
  }

  
  nsCOMPtr<nsIScriptSecurityManager> secMan =
    do_GetService("@mozilla.org/scriptsecuritymanager;1");
  if (secMan) {
    rv = secMan->GetChannelPrincipal(mChannel,
                                     getter_AddRefs(mPrincipal));
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  
  
  nsCOMPtr<nsIRunnable> event = 
    NS_NEW_RUNNABLE_METHOD(nsMediaDecoder, mDecoder, ResourceLoaded); 
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  
  return NS_OK;
}

nsresult nsFileStreamStrategy::Close()
{
  nsAutoLock lock(mLock);
  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
    mInput = nsnull;
    mSeekable = nsnull;
  }

  return NS_OK;
}

nsresult nsFileStreamStrategy::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
{
  nsAutoLock lock(mLock);
  return mInput ? mInput->Read(aBuffer, aCount, aBytes) : NS_ERROR_FAILURE;
}

nsresult nsFileStreamStrategy::Seek(PRInt32 aWhence, PRInt64 aOffset) 
{  
  nsAutoLock lock(mLock);
  return mSeekable ? mSeekable->Seek(aWhence, aOffset) : NS_ERROR_FAILURE;
}

PRInt64 nsFileStreamStrategy::Tell()
{
  nsAutoLock lock(mLock);
  if (!mSeekable)
    return 0;

  PRInt64 offset = 0;
  mSeekable->Tell(&offset);
  return offset;
}

PRUint32 nsFileStreamStrategy::Available()
{
  nsAutoLock lock(mLock);
  if (!mInput)
    return 0;

  PRUint32 count = 0;
  mInput->Available(&count);
  return count;
}

float nsFileStreamStrategy::DownloadRate()
{
  return NS_MEDIA_UNKNOWN_RATE;
}

nsIPrincipal* nsFileStreamStrategy::GetCurrentPrincipal()
{
  return mPrincipal;
}

class nsHttpStreamStrategy : public nsStreamStrategy
{
public:
  nsHttpStreamStrategy(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    nsStreamStrategy(aDecoder, aChannel, aURI),
    mPosition(0),
    mAtEOF(PR_FALSE),
    mCancelled(PR_FALSE)
  {
  }
  
  
  
  virtual nsresult Open(nsIStreamListener** aListener);
  virtual nsresult Close();
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();
  virtual PRUint32 Available();
  virtual float    DownloadRate();
  virtual void     Cancel();
  virtual nsIPrincipal* GetCurrentPrincipal();

  
  PRBool IsCancelled() const;

  
  
  
  
  void Reset(nsIChannel* aChannel, 
             nsChannelToPipeListener* aListener, 
             nsIInputStream* aStream);

private:
  
  
  
  
  nsCOMPtr<nsChannelToPipeListener> mListener;

  
  
  nsCOMPtr<nsIInputStream>  mPipeInput;

  
  
  
  
  
  PRInt64 mPosition;

  
  
  
  PRPackedBool mAtEOF;

  
  
  PRPackedBool mCancelled;
};

void nsHttpStreamStrategy::Reset(nsIChannel* aChannel, 
                                 nsChannelToPipeListener* aListener, 
                                 nsIInputStream* aStream)
{
  nsAutoLock lock(mLock);
  mChannel = aChannel;
  mListener = aListener;
  mPipeInput = aStream;
}

nsresult nsHttpStreamStrategy::Open(nsIStreamListener **aStreamListener)
{
  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  mListener = new nsChannelToPipeListener(mDecoder);
  NS_ENSURE_TRUE(mListener, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mListener->Init();
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (aStreamListener) {
    *aStreamListener = mListener;
    NS_ADDREF(*aStreamListener);
  } else {
    
    
    
    nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(mChannel);
    if (hc) {
      hc->SetRequestHeader(NS_LITERAL_CSTRING("Range"),
          NS_LITERAL_CSTRING("bytes=0-"),
          PR_FALSE);
    }
 
    rv = mChannel->AsyncOpen(mListener, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  rv = mListener->GetInputStream(getter_AddRefs(mPipeInput));
  NS_ENSURE_SUCCESS(rv, rv);

  mPosition = 0;

  return NS_OK;
}

nsresult nsHttpStreamStrategy::Close()
{
  nsAutoLock lock(mLock);
  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
  }
  if (mPipeInput) {
    mPipeInput->Close();
    mPipeInput = nsnull;
  }
  mListener = nsnull;
  return NS_OK;
}

nsresult nsHttpStreamStrategy::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
{
  
  
  
  nsAutoLock lock(mLock);
  if (!mPipeInput)
    return NS_ERROR_FAILURE;

  
  
  nsresult rv = mPipeInput->Read(aBuffer, aCount, aBytes);
  NS_ENSURE_SUCCESS(rv, rv);
  mPosition += *aBytes;

  return rv;
}

class nsByteRangeEvent : public nsRunnable 
{
public:
  nsByteRangeEvent(nsHttpStreamStrategy* aStrategy, 
                   nsMediaDecoder* aDecoder, 
                   nsIURI* aURI, 
                   PRInt64 aOffset) :
    mStrategy(aStrategy),
    mDecoder(aDecoder),
    mURI(aURI),
    mOffset(aOffset),
    mResult(NS_OK)
  {
    MOZ_COUNT_CTOR(nsByteRangeEvent);
  }

  ~nsByteRangeEvent()
  {
    MOZ_COUNT_DTOR(nsByteRangeEvent);
  }

  nsresult GetResult()
  {
    return mResult;
  }

  NS_IMETHOD Run() {
    
    
    
    
    
    
    if (mStrategy->IsCancelled()) {
      mResult = NS_ERROR_FAILURE;
      return NS_OK;
    }

    mStrategy->Close();
    mResult = NS_NewChannel(getter_AddRefs(mChannel),
                            mURI,
                            nsnull,
                            nsnull,
                            nsnull,
                            nsIRequest::LOAD_NORMAL);
    NS_ENSURE_SUCCESS(mResult, mResult);
    nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(mChannel);
    if (hc) {
      nsCAutoString rangeString("bytes=");
      rangeString.AppendInt(mOffset);
      rangeString.Append("-");
      hc->SetRequestHeader(NS_LITERAL_CSTRING("Range"), rangeString, PR_FALSE);
    }

    mListener = new nsChannelToPipeListener(mDecoder, PR_TRUE, mOffset);
    NS_ENSURE_TRUE(mListener, NS_ERROR_OUT_OF_MEMORY);

    mResult = mListener->Init();
    NS_ENSURE_SUCCESS(mResult, mResult);

    mResult = mChannel->AsyncOpen(mListener, nsnull);
    NS_ENSURE_SUCCESS(mResult, mResult);

    mResult = mListener->GetInputStream(getter_AddRefs(mStream));
    NS_ENSURE_SUCCESS(mResult, mResult);

    mStrategy->Reset(mChannel, mListener, mStream);
    return NS_OK;
  }

private:
  nsHttpStreamStrategy* mStrategy;
  nsMediaDecoder* mDecoder;
  nsIURI* mURI;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsChannelToPipeListener> mListener;
  nsCOMPtr<nsIInputStream> mStream;
  PRInt64 mOffset;
  nsresult mResult;
};

nsresult nsHttpStreamStrategy::Seek(PRInt32 aWhence, PRInt64 aOffset) 
{
  {
    nsAutoLock lock(mLock);
    if (!mChannel || !mPipeInput) 
      return NS_ERROR_FAILURE;

    
    
    
    
    
    if(aWhence == nsISeekableStream::NS_SEEK_END && aOffset == 0) {
      if (mDecoder->GetTotalBytes() == -1)
        return NS_ERROR_FAILURE;
      
      mAtEOF = PR_TRUE;
      return NS_OK;
    }
    else {
      mAtEOF = PR_FALSE;
    }

    
    
    switch (aWhence) {
    case nsISeekableStream::NS_SEEK_END: {
      PRInt32 length;
      mChannel->GetContentLength(&length);
      if (length == -1)
        return NS_ERROR_FAILURE;
      
      aOffset -= length; 
      aWhence = nsISeekableStream::NS_SEEK_SET;
      break;
    }
    case nsISeekableStream::NS_SEEK_CUR: {
      aOffset += mPosition;
      aWhence = nsISeekableStream::NS_SEEK_SET;
      break;
    }
    default:
      
      break;
    };
    
    
    if (aOffset == mPosition) {
      return NS_OK;
    }

    
    
    
    PRInt32 bytesAhead = aOffset - mPosition;
    PRUint32 available = 0;
    nsresult rv = mPipeInput->Available(&available);
    PRInt32 diff = available - PRUint32(bytesAhead);

    
    
    
    
    
    
    if (NS_SUCCEEDED(rv) && bytesAhead > 0 && diff > -SEEK_VS_READ_THRESHOLD) {
      nsAutoArrayPtr<char> data(new char[bytesAhead]);
      if (!data)
        return NS_ERROR_OUT_OF_MEMORY;
    
      
      
      
      PRUint32 bytesRead = 0;
      PRUint32 bytes = 0;
      do {
        nsresult rv = mPipeInput->Read(data.get(),
                                       (bytesAhead-bytesRead),
                                       &bytes);
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_TRUE(bytes != 0, NS_ERROR_FAILURE); 
        mPosition += bytes;
        bytesRead += bytes;
      } while (bytesRead != bytesAhead);
      return rv;
    }
  }

  
  
  nsCOMPtr<nsByteRangeEvent> event = new nsByteRangeEvent(this, mDecoder, mURI, aOffset);
  NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);

  
  
  nsresult rv = event->GetResult();
  if (NS_SUCCEEDED(rv)) {
    mPosition = aOffset;
  }

  return rv;
}

PRInt64 nsHttpStreamStrategy::Tell()
{
  
  
  return mAtEOF ? mDecoder->GetTotalBytes() : mPosition;
}

PRUint32 nsHttpStreamStrategy::Available()
{
  
  
  
  nsAutoLock lock(mLock);
  if (!mPipeInput)
    return 0;

  PRUint32 count = 0;
  mPipeInput->Available(&count);
  return count;
}

float nsHttpStreamStrategy::DownloadRate()
{
  nsAutoLock lock(mLock);
  if (!mListener)
    return NS_MEDIA_UNKNOWN_RATE;
  return mListener->BytesPerSecond();
}

void nsHttpStreamStrategy::Cancel()
{
  mCancelled = PR_TRUE;
  if (mListener)
    mListener->Cancel();
}

PRBool nsHttpStreamStrategy::IsCancelled() const
{
  return mCancelled;
}

nsIPrincipal* nsHttpStreamStrategy::GetCurrentPrincipal()
{
  if (!mListener)
    return nsnull;

  return mListener->GetCurrentPrincipal();
}

nsMediaStream::nsMediaStream()  :
  mPlaybackRateCount(0)
{
  NS_ASSERTION(NS_IsMainThread(), 
	       "nsMediaStream created on non-main thread");
  MOZ_COUNT_CTOR(nsMediaStream);
}

nsMediaStream::~nsMediaStream()
{
  MOZ_COUNT_DTOR(nsMediaStream);
}

nsresult nsMediaStream::Open(nsMediaDecoder* aDecoder, nsIURI* aURI,
                             nsIChannel* aChannel, nsIStreamListener** aListener)
{
  NS_ASSERTION(NS_IsMainThread(), 
	       "nsMediaStream::Open called on non-main thread");

  nsCOMPtr<nsIChannel> channel;
  if (aChannel) {
    channel = aChannel;
  } else {
    nsresult rv = NS_NewChannel(getter_AddRefs(channel), 
                                aURI, 
                                nsnull,
                                nsnull,
                                nsnull,
                                nsIRequest::LOAD_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIFileChannel> fc = do_QueryInterface(channel);
  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(channel);
  if (hc) 
    mStreamStrategy = new nsHttpStreamStrategy(aDecoder, channel, aURI);
  else if (fc) 
    mStreamStrategy = new nsFileStreamStrategy(aDecoder, channel, aURI);
  else
    mStreamStrategy = new nsDefaultStreamStrategy(aDecoder, channel, aURI);

  mPlaybackRateCount = 0;
  mPlaybackRateStart = PR_IntervalNow();

  return mStreamStrategy->Open(aListener);
}

nsresult nsMediaStream::Close()
{
  NS_ASSERTION(NS_IsMainThread(), 
	       "nsMediaStream::Close called on non-main thread");

  return mStreamStrategy->Close();
}

nsresult nsMediaStream::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
{
  nsresult rv = mStreamStrategy->Read(aBuffer, aCount, aBytes);
  mPlaybackRateCount += *aBytes;    
  return rv;
}

nsresult nsMediaStream::Seek(PRInt32 aWhence, PRInt64 aOffset) 
{
  return mStreamStrategy->Seek(aWhence, aOffset);
}

PRInt64 nsMediaStream::Tell()
{
  return mStreamStrategy->Tell();
}

PRUint32 nsMediaStream::Available()
{
  return mStreamStrategy->Available();
}

float nsMediaStream::DownloadRate()
{
  return mStreamStrategy->DownloadRate();
}

float nsMediaStream::PlaybackRate()
{
  PRIntervalTime now = PR_IntervalNow();
  PRUint32 interval = PR_IntervalToMilliseconds(now - mPlaybackRateStart);
  return static_cast<float>(mPlaybackRateCount) * 1000 / interval;
}

void nsMediaStream::Cancel()
{
  NS_ASSERTION(NS_IsMainThread(), 
	       "nsMediaStream::Cancel called on non-main thread");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  mStreamStrategy->Cancel();
}

nsIPrincipal* nsMediaStream::GetCurrentPrincipal()
{
  return mStreamStrategy->GetCurrentPrincipal();
}
