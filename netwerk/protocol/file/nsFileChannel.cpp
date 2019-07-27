





#include "nsIOService.h"
#include "nsFileChannel.h"
#include "nsBaseContentStream.h"
#include "nsDirectoryIndexStream.h"
#include "nsThreadUtils.h"
#include "nsTransportUtils.h"
#include "nsStreamUtils.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsProxyRelease.h"
#include "nsAutoPtr.h"
#include "nsIContentPolicy.h"
#include "nsContentUtils.h"

#include "nsIFileURL.h"
#include "nsIMIMEService.h"
#include <algorithm>



class nsFileCopyEvent : public nsRunnable {
public:
  nsFileCopyEvent(nsIOutputStream *dest, nsIInputStream *source, int64_t len)
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
  int64_t mLen;
  nsresult mStatus;           
  nsresult mInterruptStatus;  
};

void
nsFileCopyEvent::DoCopy()
{
  
  
  const int32_t chunk = nsIOService::gDefaultSegmentSize * nsIOService::gDefaultSegmentCount;

  nsresult rv = NS_OK;

  int64_t len = mLen, progress = 0;
  while (len) {
    
    rv = mInterruptStatus;
    if (NS_FAILED(rv))
      break;

    int32_t num = std::min((int32_t) len, chunk);

    uint32_t result;
    rv = mSource->ReadSegments(NS_CopySegmentToStream, mDest, num, &result);
    if (NS_FAILED(rv))
      break;
    if (result != (uint32_t) num) {
      rv = NS_ERROR_FILE_DISK_FULL;  
      break;
    }

    
    if (mSink) {
      progress += num;
      mSink->OnTransportStatus(nullptr, NS_NET_STATUS_WRITING, progress,
                               mLen);
    }
                               
    len -= num;
  }

  if (NS_FAILED(rv))
    mStatus = rv;

  
  
  mDest->Close();

  
  if (mCallback) {
    mCallbackTarget->Dispatch(mCallback, NS_DISPATCH_NORMAL);

    
    
    nsIRunnable *doomed = nullptr;
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
                                               target, true);
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

  nsFileUploadContentStream(bool nonBlocking,
                            nsIOutputStream *dest,
                            nsIInputStream *source,
                            int64_t len,
                            nsITransportEventSink *sink)
    : nsBaseContentStream(nonBlocking)
    , mCopyEvent(new nsFileCopyEvent(dest, source, len))
    , mSink(sink) {
  }

  bool IsInitialized() {
    return mCopyEvent != nullptr;
  }

  NS_IMETHODIMP ReadSegments(nsWriteSegmentFun fun, void *closure,
                             uint32_t count, uint32_t *result);
  NS_IMETHODIMP AsyncWait(nsIInputStreamCallback *callback, uint32_t flags,
                          uint32_t count, nsIEventTarget *target);

private:
  virtual ~nsFileUploadContentStream() {}

  void OnCopyComplete();

  nsRefPtr<nsFileCopyEvent> mCopyEvent;
  nsCOMPtr<nsITransportEventSink> mSink;
};

NS_IMPL_ISUPPORTS_INHERITED0(nsFileUploadContentStream,
                             nsBaseContentStream)

NS_IMETHODIMP
nsFileUploadContentStream::ReadSegments(nsWriteSegmentFun fun, void *closure,
                                        uint32_t count, uint32_t *result)
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
                                     uint32_t flags, uint32_t count,
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



nsFileChannel::nsFileChannel(nsIURI *uri) 
{
  
  
  
  nsCOMPtr<nsIFile> file;
  nsCOMPtr <nsIURI> targetURI;
  nsAutoCString fileTarget;
  nsCOMPtr<nsIFile> resolvedFile;
  bool symLink;
  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(uri);
  if (fileURL && 
      NS_SUCCEEDED(fileURL->GetFile(getter_AddRefs(file))) &&
      NS_SUCCEEDED(file->IsSymlink(&symLink)) && 
      symLink &&
      NS_SUCCEEDED(file->GetNativeTarget(fileTarget)) &&
      NS_SUCCEEDED(NS_NewNativeLocalFile(fileTarget, PR_TRUE, 
                                         getter_AddRefs(resolvedFile))) &&
      NS_SUCCEEDED(NS_NewFileURI(getter_AddRefs(targetURI), 
                   resolvedFile, nullptr))) {
    SetURI(targetURI);
    SetOriginalURI(uri);
    nsLoadFlags loadFlags = 0;
    GetLoadFlags(&loadFlags);
    SetLoadFlags(loadFlags | nsIChannel::LOAD_REPLACE);
  } else {
    SetURI(uri);
  }
}

nsFileChannel::~nsFileChannel()
{
}

nsresult
nsFileChannel::MakeFileInputStream(nsIFile *file,
                                   nsCOMPtr<nsIInputStream> &stream,
                                   nsCString &contentType,
                                   bool async)
{
  
  bool isDir;
  nsresult rv = file->IsDirectory(&isDir);
  if (NS_FAILED(rv)) {
    
    if (rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)
      rv = NS_ERROR_FILE_NOT_FOUND;

    if (async && (NS_ERROR_FILE_NOT_FOUND == rv)) {
      
      
      isDir = false;
    } else {
      return rv;
    }
  }

  if (isDir) {
    rv = nsDirectoryIndexStream::Create(file, getter_AddRefs(stream));
    if (NS_SUCCEEDED(rv) && !HasContentTypeHint())
      contentType.AssignLiteral(APPLICATION_HTTP_INDEX_FORMAT);
  } else {
    rv = NS_NewLocalFileInputStream(getter_AddRefs(stream), file, -1, -1,
                                    async? nsIFileInputStream::DEFER_OPEN : 0);
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
nsFileChannel::OpenContentStream(bool async, nsIInputStream **result,
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
    rv = NS_NewChannel(getter_AddRefs(newChannel),
                       newURI,
                       nsContentUtils::GetSystemPrincipal(),
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER);

    if (NS_FAILED(rv))
      return rv;

    *result = nullptr;
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

    nsRefPtr<nsFileUploadContentStream> uploadStream =
        new nsFileUploadContentStream(async, fileStream, mUploadStream,
                                      mUploadLength, this);
    if (!uploadStream || !uploadStream->IsInitialized()) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    stream = uploadStream.forget();

    mContentLength = 0;

    
    
    
    
    if (!HasContentTypeHint())
      SetContentType(NS_LITERAL_CSTRING(APPLICATION_OCTET_STREAM));
  } else {
    nsAutoCString contentType;
    rv = MakeFileInputStream(file, stream, contentType, async);
    if (NS_FAILED(rv))
      return rv;

    EnableSynthesizedProgressEvents(true);

    
    if (mContentLength < 0) {
      int64_t size;
      rv = file->GetFileSize(&size);
      if (NS_FAILED(rv)) {
        if (async && 
            (NS_ERROR_FILE_NOT_FOUND == rv ||
             NS_ERROR_FILE_TARGET_DOES_NOT_EXIST == rv)) {
          size = 0;
        } else {
          return rv;
        }
      }
      mContentLength = size;
    }
    if (!contentType.IsEmpty())
      SetContentType(contentType);
  }

  *result = nullptr;
  stream.swap(*result);
  return NS_OK;
}




NS_IMPL_ISUPPORTS_INHERITED(nsFileChannel,
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
                               int64_t contentLength)
{
  NS_ENSURE_TRUE(!Pending(), NS_ERROR_IN_PROGRESS);

  if ((mUploadStream = stream)) {
    mUploadLength = contentLength;
    if (mUploadLength < 0) {
      
      uint64_t avail;
      nsresult rv = mUploadStream->Available(&avail);
      if (NS_FAILED(rv))
        return rv;
      if (avail < INT64_MAX)
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
