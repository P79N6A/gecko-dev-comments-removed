




































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
#include "nsCrossSiteListenerProxy.h"
#include "nsHTMLMediaElement.h"
#include "nsIDocument.h"

class nsMediaChannelStream : public nsMediaStream
{
public:
  nsMediaChannelStream(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    nsMediaStream(aDecoder, aChannel, aURI),
    mPosition(0), mCancelled(PR_FALSE)
  {
  }
  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult Close();
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();
  virtual void     Cancel();
  virtual nsIPrincipal* GetCurrentPrincipal();
  virtual void     Suspend();
  virtual void     Resume();

  
  PRBool IsCancelled() const { return mCancelled; }

  
  
  nsresult OpenAtOffset(nsIStreamListener** aStreamListener, PRInt64 aOffset);

protected:
  
  
  
  
  nsCOMPtr<nsChannelToPipeListener> mListener;

  
  
  nsCOMPtr<nsIInputStream>  mPipeInput;

  
  
  
  
  
  PRInt64 mPosition;

  
  
  PRPackedBool mCancelled;
};

nsresult nsMediaChannelStream::Open(nsIStreamListener **aStreamListener)
{
  return OpenAtOffset(aStreamListener, 0);
}

nsresult nsMediaChannelStream::OpenAtOffset(nsIStreamListener** aStreamListener,
                                               PRInt64 aOffset)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  NS_ENSURE_TRUE(mChannel, NS_ERROR_NULL_POINTER);

  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  mListener = new nsChannelToPipeListener(mDecoder, aOffset != 0);
  NS_ENSURE_TRUE(mListener, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mListener->Init();
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIStreamListener> listener = mListener.get();

  if (aStreamListener) {
    *aStreamListener = mListener;
    NS_ADDREF(*aStreamListener);
  } else {
    
    
    nsHTMLMediaElement* element = mDecoder->GetMediaElement();
    NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);
    if (element->ShouldCheckAllowOrigin()) {
      listener = new nsCrossSiteListenerProxy(mListener,
                                              element->NodePrincipal(),
                                              mChannel, 
                                              PR_FALSE,
                                              &rv);
      NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      rv = nsContentUtils::GetSecurityManager()->
             CheckLoadURIWithPrincipal(element->NodePrincipal(),
                                       mURI,
                                       nsIScriptSecurityManager::STANDARD);
      NS_ENSURE_SUCCESS(rv, rv);

    }
    
    
    
    nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(mChannel);
    if (hc) {
      nsCAutoString rangeString("bytes=");
      rangeString.AppendInt(aOffset);
      rangeString.Append("-");
      hc->SetRequestHeader(NS_LITERAL_CSTRING("Range"), rangeString, PR_FALSE);
    } else {
      NS_ASSERTION(aOffset == 0, "Don't know how to seek on this channel type");
    }
 
    rv = mChannel->AsyncOpen(listener, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mListener->GetInputStream(getter_AddRefs(mPipeInput));
  NS_ENSURE_SUCCESS(rv, rv);

  mDecoder->NotifyDownloadSeeked(aOffset);

  return NS_OK;
}

nsresult nsMediaChannelStream::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
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

nsresult nsMediaChannelStream::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
{
  
  
  
  nsAutoLock lock(mLock);
  if (!mPipeInput)
    return NS_ERROR_FAILURE;

  
  
  nsresult rv = mPipeInput->Read(aBuffer, aCount, aBytes);
  NS_ENSURE_SUCCESS(rv, rv);
  mPosition += *aBytes;

  return rv;
}

nsresult nsMediaChannelStream::Seek(PRInt32 aWhence, PRInt64 aOffset) 
{
  
  return NS_ERROR_FAILURE;
}

PRInt64 nsMediaChannelStream::Tell()
{
  return mPosition;
}

void nsMediaChannelStream::Cancel()
{
  mCancelled = PR_TRUE;
  if (mListener)
    mListener->Cancel();
}

nsIPrincipal* nsMediaChannelStream::GetCurrentPrincipal()
{
  if (!mListener)
    return nsnull;

  return mListener->GetCurrentPrincipal();
}

void nsMediaChannelStream::Suspend()
{
  mChannel->Suspend();
}

void nsMediaChannelStream::Resume()
{
  mChannel->Resume();
}

class nsMediaFileStream : public nsMediaStream
{
public:
  nsMediaFileStream(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    nsMediaStream(aDecoder, aChannel, aURI)
  {
  }
  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult Close();
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();
  virtual nsIPrincipal* GetCurrentPrincipal();
  virtual void     Suspend();
  virtual void     Resume();

private:
  
  
  nsCOMPtr<nsISeekableStream> mSeekable;

  
  
  nsCOMPtr<nsIInputStream>  mInput;

  
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

class LoadedEvent : public nsRunnable 
{
public:
  LoadedEvent(nsMediaDecoder* aDecoder, PRInt64 aOffset, PRInt64 aSize) :
    mOffset(aOffset), mSize(aSize), mDecoder(aDecoder)
  {
    MOZ_COUNT_CTOR(LoadedEvent);
  }
  ~LoadedEvent()
  {
    MOZ_COUNT_DTOR(LoadedEvent);
  }

  NS_IMETHOD Run() {
    if (mOffset >= 0) {
      mDecoder->NotifyDownloadSeeked(mOffset);
    }
    if (mSize > 0) {
      mDecoder->NotifyBytesDownloaded(mSize);
    }
    mDecoder->NotifyDownloadEnded(NS_OK);
    return NS_OK;
  }

private:
  PRInt64                  mOffset;
  PRInt64                  mSize;
  nsRefPtr<nsMediaDecoder> mDecoder;
};

nsresult nsMediaFileStream::Open(nsIStreamListener** aStreamListener)
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
    
    
    nsHTMLMediaElement* element = mDecoder->GetMediaElement();
    NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);

    rv = nsContentUtils::GetSecurityManager()->
           CheckLoadURIWithPrincipal(element->NodePrincipal(),
                                     mURI,
                                     nsIScriptSecurityManager::STANDARD);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mChannel->Open(getter_AddRefs(mInput));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  mSeekable = do_QueryInterface(mInput);
  if (!mSeekable) {
    
    
    
    
    return NS_ERROR_FAILURE;
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

  
  
  PRUint32 size;
  rv = mInput->Available(&size);
  if (NS_SUCCEEDED(rv)) {
    mDecoder->SetTotalBytes(size);
  }

  
  
  
  mDecoder->NotifyBytesDownloaded(size);

  nsCOMPtr<nsIRunnable> event = new LoadedEvent(mDecoder, -1, 0);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  return NS_OK;
}

nsresult nsMediaFileStream::Close()
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

nsresult nsMediaFileStream::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
{
  nsAutoLock lock(mLock);
  if (!mInput)
    return NS_ERROR_FAILURE;
  return mInput->Read(aBuffer, aCount, aBytes);
}

nsresult nsMediaFileStream::Seek(PRInt32 aWhence, PRInt64 aOffset) 
{  
  PRUint32 size = 0;
  PRInt64 absoluteOffset = 0;
  nsresult rv;
  {
    nsAutoLock lock(mLock);
    if (!mSeekable)
      return NS_ERROR_FAILURE;
    rv = mSeekable->Seek(aWhence, aOffset);
    if (NS_SUCCEEDED(rv)) {
      mSeekable->Tell(&absoluteOffset);
    }
    mInput->Available(&size);
  }

  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIRunnable> event = new LoadedEvent(mDecoder, absoluteOffset, size);
    
    
    NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
  }

  return rv;
}

PRInt64 nsMediaFileStream::Tell()
{
  nsAutoLock lock(mLock);
  if (!mSeekable)
    return 0;

  PRInt64 offset = 0;
  mSeekable->Tell(&offset);
  return offset;
}

nsIPrincipal* nsMediaFileStream::GetCurrentPrincipal()
{
  return mPrincipal;
}

void nsMediaFileStream::Suspend()
{
  mChannel->Suspend();
}

void nsMediaFileStream::Resume()
{
  mChannel->Resume();
}

class nsMediaHttpStream : public nsMediaChannelStream
{
public:
  nsMediaHttpStream(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    nsMediaChannelStream(aDecoder, aChannel, aURI),
    mAtEOF(PR_FALSE)
  {
  }

  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();

  
  
  
  
  nsresult OpenInternal(nsIChannel* aChannel, PRInt64 aOffset);

private:
  
  
  
  PRPackedBool mAtEOF;
};

nsresult nsMediaHttpStream::OpenInternal(nsIChannel* aChannel,
                                            PRInt64 aOffset)
{
  nsAutoLock lock(mLock);
  mChannel = aChannel;
  return OpenAtOffset(static_cast<nsIStreamListener**>(nsnull), aOffset);
}

class nsByteRangeEvent : public nsRunnable 
{
public:
  nsByteRangeEvent(nsMediaHttpStream* aStrategy, 
                   nsIURI* aURI, 
                   PRInt64 aOffset) :
    mStrategy(aStrategy),
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

    nsCOMPtr<nsIChannel> channel;
    mStrategy->Close();
    mResult = NS_NewChannel(getter_AddRefs(channel),
                            mURI,
                            nsnull,
                            nsnull,
                            nsnull,
                            nsIRequest::LOAD_NORMAL);
    NS_ENSURE_SUCCESS(mResult, mResult);
    mResult = mStrategy->OpenInternal(channel, mOffset);
    return NS_OK;
  }

private:
  nsMediaHttpStream* mStrategy;
  nsMediaDecoder* mDecoder;
  nsIURI* mURI;
  PRInt64 mOffset;
  nsresult mResult;
};

nsresult nsMediaHttpStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
  PRInt64 totalBytes = mDecoder->GetStatistics().mTotalBytes;
  {
    nsAutoLock lock(mLock);
    if (!mChannel || !mPipeInput) 
      return NS_ERROR_FAILURE;

    
    
    
    
    
    if(aWhence == nsISeekableStream::NS_SEEK_END && aOffset == 0) {
      if (totalBytes == -1)
        return NS_ERROR_FAILURE;
      
      mAtEOF = PR_TRUE;
      return NS_OK;
    }
    else {
      mAtEOF = PR_FALSE;
    }

    
    
    switch (aWhence) {
    case nsISeekableStream::NS_SEEK_END: {
      if (totalBytes == -1)
        return NS_ERROR_FAILURE;
      
      aOffset += totalBytes; 
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
      
      
      
      PRInt32 bytesRead = 0;
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

      
      
      
      
      
      
      mDecoder->NotifyBytesConsumed(bytesRead);
      return rv;
    }
  }

  
  
  nsCOMPtr<nsByteRangeEvent> event = new nsByteRangeEvent(this, mURI, aOffset);
  NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);

  
  
  nsresult rv = event->GetResult();
  if (NS_SUCCEEDED(rv)) {
    mPosition = aOffset;
  }

  return rv;
}

PRInt64 nsMediaHttpStream::Tell()
{
  
  
  return mAtEOF ? mDecoder->GetStatistics().mTotalBytes : mPosition;
}

nsresult
nsMediaStream::Open(nsMediaDecoder* aDecoder, nsIURI* aURI,
                    nsIChannel* aChannel, nsMediaStream** aStream,
                    nsIStreamListener** aListener)
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

  nsMediaStream* stream;
  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(channel);
  if (hc) {
    stream = new nsMediaHttpStream(aDecoder, channel, aURI);
  } else {
    nsCOMPtr<nsIFileChannel> fc = do_QueryInterface(channel);
    if (fc) {
      stream = new nsMediaFileStream(aDecoder, channel, aURI);
    } else {
      stream = new nsMediaChannelStream(aDecoder, channel, aURI);
    }
  }
  if (!stream)
    return NS_ERROR_OUT_OF_MEMORY;

  *aStream = stream;
  return stream->Open(aListener);
}
