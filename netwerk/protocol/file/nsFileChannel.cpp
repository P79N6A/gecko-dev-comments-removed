






































#include "nsIOService.h"
#include "nsFileChannel.h"
#include "nsBaseContentStream.h"
#include "nsDirectoryIndexStream.h"
#include "nsThreadUtils.h"
#include "nsTransportUtils.h"
#include "nsStreamUtils.h"
#include "nsURLHelper.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsProxyRelease.h"
#include "nsAutoPtr.h"
#include "nsStandardURL.h"

#include "nsIFileURL.h"
#include "nsIMIMEService.h"



class nsFileCopyEvent : public nsRunnable {
public:
  nsFileCopyEvent(nsIOutputStream *dest, nsIInputStream *source, PRInt64 len)
    : mDest(dest)
    , mSource(source)
    , mLen(len)
    , mStatus(NS_OK)
    , mInterruptStatus(NS_OK) {
  }

  
  nsresult Status() { return mStatus; }
  
  
  void DoCopy();

  
  
  nsresult Dispatch(nsIRunnable *callback,
                    nsITransportEventSink *sink,
                    nsIEventTarget *target);

  
  
  
  void Interrupt(nsresult status) {
    NS_ASSERTION(NS_FAILED(status), "must be a failure code");
    mInterruptStatus = status;
  }

  NS_IMETHOD Run() {
    DoCopy();
    return NS_OK;
  }

private:
  nsCOMPtr<nsIEventTarget> mCallbackTarget;
  nsCOMPtr<nsIRunnable> mCallback;
  nsCOMPtr<nsITransportEventSink> mSink;
  nsCOMPtr<nsIOutputStream> mDest;
  nsCOMPtr<nsIInputStream> mSource;
  PRInt64 mLen;
  nsresult mStatus;           
  nsresult mInterruptStatus;  
};

void
nsFileCopyEvent::DoCopy()
{
  
  
  const PRInt32 chunk = nsIOService::gDefaultSegmentSize * nsIOService::gDefaultSegmentCount;

  nsresult rv = NS_OK;

  PRInt64 len = mLen, progress = 0;
  while (len) {
    
    rv = mInterruptStatus;
    if (NS_FAILED(rv))
      break;

    PRInt32 num = NS_MIN((PRInt32) len, chunk);

    PRUint32 result;
    rv = mSource->ReadSegments(NS_CopySegmentToStream, mDest, num, &result);
    if (NS_FAILED(rv))
      break;
    if (result != (PRUint32) num) {
      rv = NS_ERROR_FILE_DISK_FULL;  
      break;
    }

    
    if (mSink) {
      progress += num;
      mSink->OnTransportStatus(nsnull, nsITransport::STATUS_WRITING, progress,
                               mLen);
    }
                               
    len -= num;
  }

  if (NS_FAILED(rv))
    mStatus = rv;

  
  
  mDest->Close();

  
  if (mCallback) {
    mCallbackTarget->Dispatch(mCallback, NS_DISPATCH_NORMAL);

    
    
    nsIRunnable *doomed = nsnull;
    mCallback.swap(doomed);
    NS_ProxyRelease(mCallbackTarget, doomed);
  }
}

nsresult
nsFileCopyEvent::Dispatch(nsIRunnable *callback,
                          nsITransportEventSink *sink,
                          nsIEventTarget *target)
{
  

  mCallback = callback;
  mCallbackTarget = target;

  
  nsresult rv = net_NewTransportEventSinkProxy(getter_AddRefs(mSink), sink,
                                               target, PR_TRUE);
  if (NS_FAILED(rv))
    return rv;

  
  nsCOMPtr<nsIEventTarget> pool =
      do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  return pool->Dispatch(this, NS_DISPATCH_NORMAL);
}






class nsFileUploadContentStream : public nsBaseContentStream {
public:
  NS_DECL_ISUPPORTS_INHERITED

  nsFileUploadContentStream(PRBool nonBlocking,
                            nsIOutputStream *dest,
                            nsIInputStream *source,
                            PRInt64 len,
                            nsITransportEventSink *sink)
    : nsBaseContentStream(nonBlocking)
    , mCopyEvent(new nsFileCopyEvent(dest, source, len))
    , mSink(sink) {
  }

  PRBool IsInitialized() {
    return mCopyEvent != nsnull;
  }

  NS_IMETHODIMP ReadSegments(nsWriteSegmentFun fun, void *closure,
                             PRUint32 count, PRUint32 *result);
  NS_IMETHODIMP AsyncWait(nsIInputStreamCallback *callback, PRUint32 flags,
                          PRUint32 count, nsIEventTarget *target);

private:
  void OnCopyComplete();

  nsRefPtr<nsFileCopyEvent> mCopyEvent;
  nsCOMPtr<nsITransportEventSink> mSink;
};

NS_IMPL_ISUPPORTS_INHERITED0(nsFileUploadContentStream,
                             nsBaseContentStream)

NS_IMETHODIMP
nsFileUploadContentStream::ReadSegments(nsWriteSegmentFun fun, void *closure,
                                        PRUint32 count, PRUint32 *result)
{
  *result = 0;  

  if (IsClosed())
    return NS_OK;

  if (IsNonBlocking()) {
    
    
    
    return NS_BASE_STREAM_WOULD_BLOCK;  
  }

  
  mCopyEvent->DoCopy();
  nsresult status = mCopyEvent->Status();
  CloseWithStatus(NS_FAILED(status) ? status : NS_BASE_STREAM_CLOSED);
  return status;
}

NS_IMETHODIMP
nsFileUploadContentStream::AsyncWait(nsIInputStreamCallback *callback,
                                     PRUint32 flags, PRUint32 count,
                                     nsIEventTarget *target)
{
  nsresult rv = nsBaseContentStream::AsyncWait(callback, flags, count, target);
  if (NS_FAILED(rv) || IsClosed())
    return rv;

  if (IsNonBlocking()) {
    nsCOMPtr<nsIRunnable> callback =
      NS_NewRunnableMethod(this, &nsFileUploadContentStream::OnCopyComplete);
    mCopyEvent->Dispatch(callback, mSink, target);
  }

  return NS_OK;
}

void
nsFileUploadContentStream::OnCopyComplete()
{
  
  nsresult status = mCopyEvent->Status();

  CloseWithStatus(NS_FAILED(status) ? status : NS_BASE_STREAM_CLOSED);
}



nsresult
nsFileChannel::MakeFileInputStream(nsIFile *file,
                                   nsCOMPtr<nsIInputStream> &stream,
                                   nsCString &contentType)
{
  
  PRBool isDir;
  nsresult rv = file->IsDirectory(&isDir);
  if (NS_FAILED(rv)) {
    
    if (rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)
      rv = NS_ERROR_FILE_NOT_FOUND;
    return rv;
  }

  if (isDir) {
    rv = nsDirectoryIndexStream::Create(file, getter_AddRefs(stream));
    if (NS_SUCCEEDED(rv) && !HasContentTypeHint())
      contentType.AssignLiteral(APPLICATION_HTTP_INDEX_FORMAT);
  } else {
    rv = NS_NewLocalFileInputStream(getter_AddRefs(stream), file);
    if (NS_SUCCEEDED(rv) && !HasContentTypeHint()) {
      
      nsCOMPtr<nsIMIMEService> mime = do_GetService("@mozilla.org/mime;1", &rv);
      if (NS_SUCCEEDED(rv)) {
        mime->GetTypeFromFile(file, contentType);
      }
    }
  }
  return rv;
}

nsresult
nsFileChannel::OpenContentStream(PRBool async, nsIInputStream **result,
                                 nsIChannel** channel)
{
  
  
  nsCOMPtr<nsIFile> file;
  nsresult rv = GetFile(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFileProtocolHandler> fileHandler;
  rv = NS_GetFileProtocolHandler(getter_AddRefs(fileHandler));
  if (NS_FAILED(rv))
    return rv;
    
  nsCOMPtr<nsIURI> newURI;
  rv = fileHandler->ReadURLFile(file, getter_AddRefs(newURI));
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIChannel> newChannel;
    rv = NS_NewChannel(getter_AddRefs(newChannel), newURI);
    if (NS_FAILED(rv))
      return rv;

    *result = nsnull;
    newChannel.forget(channel);
    return NS_OK;
  }

  nsCOMPtr<nsIInputStream> stream;

  if (mUploadStream) {
    
    
    

    nsCOMPtr<nsIOutputStream> fileStream;
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(fileStream), file,
                                     PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE,
                                     PR_IRUSR | PR_IWUSR);
    if (NS_FAILED(rv))
      return rv;

    nsFileUploadContentStream *uploadStream =
        new nsFileUploadContentStream(async, fileStream, mUploadStream,
                                      mUploadLength, this);
    if (!uploadStream || !uploadStream->IsInitialized()) {
      delete uploadStream;
      return NS_ERROR_OUT_OF_MEMORY;
    }
    stream = uploadStream;

    SetContentLength64(0);

    
    
    
    
    if (!HasContentTypeHint())
      SetContentType(NS_LITERAL_CSTRING(APPLICATION_OCTET_STREAM));
  } else {
    nsCAutoString contentType;
    nsresult rv = MakeFileInputStream(file, stream, contentType);
    if (NS_FAILED(rv))
      return rv;

    EnableSynthesizedProgressEvents(PR_TRUE);

    
    if (ContentLength64() < 0) {
      PRInt64 size;
      rv = file->GetFileSize(&size);
      if (NS_FAILED(rv))
        return rv;
      SetContentLength64(size);
    }
    if (!contentType.IsEmpty())
      SetContentType(contentType);
  }

  *result = nsnull;
  stream.swap(*result);
  return NS_OK;
}




NS_IMPL_ISUPPORTS_INHERITED2(nsFileChannel,
                             nsBaseChannel,
                             nsIUploadChannel,
                             nsIFileChannel)




NS_IMETHODIMP
nsFileChannel::GetFile(nsIFile **file)
{
    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(URI());
    NS_ENSURE_STATE(fileURL);

    
    return fileURL->GetFile(file);
}




NS_IMETHODIMP
nsFileChannel::SetUploadStream(nsIInputStream *stream,
                               const nsACString &contentType,
                               PRInt32 contentLength)
{
  NS_ENSURE_TRUE(!IsPending(), NS_ERROR_IN_PROGRESS);

  if ((mUploadStream = stream)) {
    mUploadLength = contentLength;
    if (mUploadLength < 0) {
      
      PRUint32 avail;
      nsresult rv = mUploadStream->Available(&avail);
      if (NS_FAILED(rv))
        return rv;
      mUploadLength = avail;
    }
  } else {
    mUploadLength = -1;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFileChannel::GetUploadStream(nsIInputStream **result)
{
    NS_IF_ADDREF(*result = mUploadStream);
    return NS_OK;
}
