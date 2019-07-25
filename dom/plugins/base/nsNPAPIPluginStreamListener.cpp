




#include "nsNPAPIPluginStreamListener.h"
#include "plstr.h"
#include "prmem.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIFile.h"
#include "nsNetUtil.h"
#include "nsPluginHost.h"
#include "nsNPAPIPlugin.h"
#include "nsPluginSafety.h"
#include "nsPluginLogging.h"
#include "nsPluginStreamListenerPeer.h"

#include "mozilla/StandardInteger.h"

nsNPAPIStreamWrapper::nsNPAPIStreamWrapper(nsIOutputStream *outputStream,
                                           nsNPAPIPluginStreamListener *streamListener)
{
  mOutputStream = outputStream;
  mStreamListener = streamListener;

  memset(&mNPStream, 0, sizeof(mNPStream));
  mNPStream.ndata = static_cast<void*>(this);
}

nsNPAPIStreamWrapper::~nsNPAPIStreamWrapper()
{
  if (mOutputStream) {
    mOutputStream->Close();
  }
}

NS_IMPL_ISUPPORTS1(nsPluginStreamToFile, nsIOutputStream)

nsPluginStreamToFile::nsPluginStreamToFile(const char* target,
                                           nsIPluginInstanceOwner* owner)
: mTarget(PL_strdup(target)),
mOwner(owner)
{
  nsresult rv;
  nsCOMPtr<nsIFile> pluginTmp;
  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(pluginTmp));
  if (NS_FAILED(rv)) return;
  
  mTempFile = do_QueryInterface(pluginTmp, &rv);
  if (NS_FAILED(rv)) return;
  
  
  rv = mTempFile->AppendNative(nsDependentCString(target));
  if (NS_FAILED(rv)) return;
  
  
  rv = mTempFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0700); 
  if (NS_FAILED(rv)) return;
  
  
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(mOutputStream), mTempFile, -1, 00600);
  if (NS_FAILED(rv))
    return;
	
  
  NS_GetURLSpecFromFile(mTempFile, mFileURL);
  
#ifdef NS_DEBUG
  printf("File URL = %s\n", mFileURL.get());
#endif
}

nsPluginStreamToFile::~nsPluginStreamToFile()
{
  
  if (nsnull != mTarget)
    PL_strfree(mTarget);
}

NS_IMETHODIMP
nsPluginStreamToFile::Flush()
{
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamToFile::Write(const char* aBuf, PRUint32 aCount,
                            PRUint32 *aWriteCount)
{
  mOutputStream->Write(aBuf, aCount, aWriteCount);
  mOutputStream->Flush();
  mOwner->GetURL(mFileURL.get(), mTarget, nsnull, nsnull, 0);
  
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamToFile::WriteFrom(nsIInputStream *inStr, PRUint32 count,
                                PRUint32 *_retval)
{
  NS_NOTREACHED("WriteFrom");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPluginStreamToFile::WriteSegments(nsReadSegmentFun reader, void * closure,
                                    PRUint32 count, PRUint32 *_retval)
{
  NS_NOTREACHED("WriteSegments");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPluginStreamToFile::IsNonBlocking(bool *aNonBlocking)
{
  *aNonBlocking = false;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamToFile::Close(void)
{
  mOutputStream->Close();
  mOwner->GetURL(mFileURL.get(), mTarget, nsnull, nsnull, 0);
  return NS_OK;
}



NS_IMPL_ISUPPORTS2(nsNPAPIPluginStreamListener,
                   nsITimerCallback, nsIHTTPHeaderListener)

nsNPAPIPluginStreamListener::nsNPAPIPluginStreamListener(nsNPAPIPluginInstance* inst, 
                                                         void* notifyData,
                                                         const char* aURL)
: mStreamBuffer(nsnull),
mNotifyURL(aURL ? PL_strdup(aURL) : nsnull),
mInst(inst),
mStreamListenerPeer(nsnull),
mStreamBufferSize(0),
mStreamBufferByteCount(0),
mStreamType(NP_NORMAL),
mStreamStarted(false),
mStreamCleanedUp(false),
mCallNotify(notifyData ? true : false),
mIsSuspended(false),
mIsPluginInitJSStream(mInst->mInPluginInitCall &&
                      aURL && strncmp(aURL, "javascript:",
                                      sizeof("javascript:") - 1) == 0),
mRedirectDenied(false),
mResponseHeaderBuf(nsnull)
{
  mNPStreamWrapper = new nsNPAPIStreamWrapper(nsnull, this);
  mNPStreamWrapper->mNPStream.notifyData = notifyData;
}

nsNPAPIPluginStreamListener::~nsNPAPIPluginStreamListener()
{
  
  nsTArray<nsNPAPIPluginStreamListener*> *streamListeners = mInst->StreamListeners();
  streamListeners->RemoveElement(this);

  
  
  
  
  CallURLNotify(NPRES_NETWORK_ERR);
  
  
  if (mStreamBuffer) {
    PR_Free(mStreamBuffer);
    mStreamBuffer=nsnull;
  }
  
  if (mNotifyURL)
    PL_strfree(mNotifyURL);
  
  if (mResponseHeaderBuf)
    PL_strfree(mResponseHeaderBuf);

  if (mNPStreamWrapper) {
    delete mNPStreamWrapper;
  }
}

nsresult
nsNPAPIPluginStreamListener::CleanUpStream(NPReason reason)
{
  nsresult rv = NS_ERROR_FAILURE;

  
  
  
  nsRefPtr<nsNPAPIPluginStreamListener> kungFuDeathGrip(this);

  if (mStreamCleanedUp)
    return NS_OK;
  
  mStreamCleanedUp = true;
  
  StopDataPump();

  
  if (mHTTPRedirectCallback) {
    mHTTPRedirectCallback->OnRedirectVerifyCallback(NS_ERROR_FAILURE);
    mHTTPRedirectCallback = nsnull;
  }

  
  
  if (NP_SEEK == mStreamType)
    NS_RELEASE_THIS();
  
  if (!mInst || !mInst->CanFireNotifications())
    return rv;
  
  mStreamInfo = NULL;
  
  PluginDestructionGuard guard(mInst);

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin || !plugin->GetLibrary())
    return rv;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  NPP npp;
  mInst->GetNPP(&npp);

  if (mStreamStarted && pluginFunctions->destroystream) {
    NPPAutoPusher nppPusher(npp);

    NPError error;
    NS_TRY_SAFE_CALL_RETURN(error, (*pluginFunctions->destroystream)(npp, &mNPStreamWrapper->mNPStream, reason), mInst);
    
    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                   ("NPP DestroyStream called: this=%p, npp=%p, reason=%d, return=%d, url=%s\n",
                    this, npp, reason, error, mNPStreamWrapper->mNPStream.url));
    
    if (error == NPERR_NO_ERROR)
      rv = NS_OK;
  }
  
  mStreamStarted = false;
  
  
  CallURLNotify(reason);
  
  return rv;
}

void
nsNPAPIPluginStreamListener::CallURLNotify(NPReason reason)
{
  if (!mCallNotify || !mInst || !mInst->CanFireNotifications())
    return;
  
  PluginDestructionGuard guard(mInst);
  
  mCallNotify = false; 

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin || !plugin->GetLibrary())
    return;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  if (pluginFunctions->urlnotify) {
    NPP npp;
    mInst->GetNPP(&npp);
    
    NS_TRY_SAFE_CALL_VOID((*pluginFunctions->urlnotify)(npp, mNotifyURL, reason, mNPStreamWrapper->mNPStream.notifyData), mInst);
    
    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                   ("NPP URLNotify called: this=%p, npp=%p, notify=%p, reason=%d, url=%s\n",
                    this, npp, mNPStreamWrapper->mNPStream.notifyData, reason, mNotifyURL));
  }
}

nsresult
nsNPAPIPluginStreamListener::OnStartBinding(nsIPluginStreamInfo* pluginInfo)
{
  if (!mInst || !mInst->CanFireNotifications())
    return NS_ERROR_FAILURE;

  PluginDestructionGuard guard(mInst);

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin || !plugin->GetLibrary())
    return NS_ERROR_FAILURE;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  if (!pluginFunctions->newstream)
    return NS_ERROR_FAILURE;

  NPP npp;
  mInst->GetNPP(&npp);

  bool seekable;
  char* contentType;
  PRUint16 streamType = NP_NORMAL;
  NPError error;

  pluginInfo->GetURL(&mNPStreamWrapper->mNPStream.url);
  pluginInfo->GetLength((PRUint32*)&(mNPStreamWrapper->mNPStream.end));
  pluginInfo->GetLastModified((PRUint32*)&(mNPStreamWrapper->mNPStream.lastmodified));
  pluginInfo->IsSeekable(&seekable);
  pluginInfo->GetContentType(&contentType);
  
  if (!mResponseHeaders.IsEmpty()) {
    mResponseHeaderBuf = PL_strdup(mResponseHeaders.get());
    mNPStreamWrapper->mNPStream.headers = mResponseHeaderBuf;
  }
  
  mStreamInfo = pluginInfo;
  
  NPPAutoPusher nppPusher(npp);
  
  NS_TRY_SAFE_CALL_RETURN(error, (*pluginFunctions->newstream)(npp, (char*)contentType, &mNPStreamWrapper->mNPStream, seekable, &streamType), mInst);
  
  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                 ("NPP NewStream called: this=%p, npp=%p, mime=%s, seek=%d, type=%d, return=%d, url=%s\n",
                  this, npp, (char *)contentType, seekable, streamType, error, mNPStreamWrapper->mNPStream.url));
  
  if (error != NPERR_NO_ERROR)
    return NS_ERROR_FAILURE;
  
  switch(streamType)
  {
    case NP_NORMAL:
      mStreamType = NP_NORMAL; 
      break;
    case NP_ASFILEONLY:
      mStreamType = NP_ASFILEONLY; 
      break;
    case NP_ASFILE:
      mStreamType = NP_ASFILE; 
      break;
    case NP_SEEK:
      mStreamType = NP_SEEK; 
      
      
      
      
      
      NS_ADDREF_THIS();
      break;
    default:
      return NS_ERROR_FAILURE;
  }
  
  mStreamStarted = true;
  return NS_OK;
}

void
nsNPAPIPluginStreamListener::SuspendRequest()
{
  NS_ASSERTION(!mIsSuspended,
               "Suspending a request that's already suspended!");
  
  nsCOMPtr<nsINPAPIPluginStreamInfo> pluginInfoNPAPI =
  do_QueryInterface(mStreamInfo);
  
  if (!pluginInfoNPAPI) {
    return;
  }
  
  nsresult rv = StartDataPump();
  if (NS_FAILED(rv))
    return;
  
  mIsSuspended = true;
  
  pluginInfoNPAPI->SuspendRequests();
}

void
nsNPAPIPluginStreamListener::ResumeRequest()
{
  nsCOMPtr<nsINPAPIPluginStreamInfo> pluginInfoNPAPI =
  do_QueryInterface(mStreamInfo);
  
  pluginInfoNPAPI->ResumeRequests();
  
  mIsSuspended = false;
}

nsresult
nsNPAPIPluginStreamListener::StartDataPump()
{
  nsresult rv;
  mDataPumpTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  
  return mDataPumpTimer->InitWithCallback(this, 100,
                                          nsITimer::TYPE_REPEATING_SLACK);
}

void
nsNPAPIPluginStreamListener::StopDataPump()
{
  if (mDataPumpTimer) {
    mDataPumpTimer->Cancel();
    mDataPumpTimer = nsnull;
  }
}



bool
nsNPAPIPluginStreamListener::PluginInitJSLoadInProgress()
{
  if (!mInst)
    return false;

  nsTArray<nsNPAPIPluginStreamListener*> *streamListeners = mInst->StreamListeners();
  for (unsigned int i = 0; i < streamListeners->Length(); i++) {
    if (streamListeners->ElementAt(i)->mIsPluginInitJSStream) {
      return true;
    }
  }
  
  return false;
}








nsresult
nsNPAPIPluginStreamListener::OnDataAvailable(nsIPluginStreamInfo* pluginInfo,
                                             nsIInputStream* input,
                                             PRUint32 length)
{
  if (!length || !mInst || !mInst->CanFireNotifications())
    return NS_ERROR_FAILURE;
  
  PluginDestructionGuard guard(mInst);
  
  
  mStreamInfo = pluginInfo;

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin || !plugin->GetLibrary())
    return NS_ERROR_FAILURE;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  
  if (!pluginFunctions->write)
    return NS_ERROR_FAILURE; 
  
  if (!mStreamBuffer) {
    
    
    
    
    
    
    PRUint32 contentLength;
    pluginInfo->GetLength(&contentLength);
    
    mStreamBufferSize = NS_MAX(length, contentLength);
    
    
    
    
    mStreamBufferSize = NS_MIN(mStreamBufferSize,
                               PRUint32(MAX_PLUGIN_NECKO_BUFFER));
    
    mStreamBuffer = (char*) PR_Malloc(mStreamBufferSize);
    if (!mStreamBuffer)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  
  
  NPP npp;
  mInst->GetNPP(&npp);
  
  PRInt32 streamPosition;
  pluginInfo->GetStreamOffset(&streamPosition);
  PRInt32 streamOffset = streamPosition;
  
  if (input) {
    streamOffset += length;
    
    
    
    
    
    
    
    
    
    
    pluginInfo->SetStreamOffset(streamOffset);
    
    
    
    
    if ((PRInt32)mNPStreamWrapper->mNPStream.end < streamOffset)
      mNPStreamWrapper->mNPStream.end = streamOffset;
  }
  
  nsresult rv = NS_OK;
  while (NS_SUCCEEDED(rv) && length > 0) {
    if (input && length) {
      if (mStreamBufferSize < mStreamBufferByteCount + length && mIsSuspended) {
        
        
        
        
        
        
        mStreamBufferSize = mStreamBufferByteCount + length;
        char *buf = (char*)PR_Realloc(mStreamBuffer, mStreamBufferSize);
        if (!buf)
          return NS_ERROR_OUT_OF_MEMORY;
        
        mStreamBuffer = buf;
      }
      
      PRUint32 bytesToRead =
      NS_MIN(length, mStreamBufferSize - mStreamBufferByteCount);
      
      PRUint32 amountRead = 0;
      rv = input->Read(mStreamBuffer + mStreamBufferByteCount, bytesToRead,
                       &amountRead);
      NS_ENSURE_SUCCESS(rv, rv);
      
      if (amountRead == 0) {
        NS_NOTREACHED("input->Read() returns no data, it's almost impossible "
                      "to get here");
        
        break;
      }
      
      mStreamBufferByteCount += amountRead;
      length -= amountRead;
    } else {
      
      
      
      length = 0;
    }
    
    
    
    char *ptrStreamBuffer = mStreamBuffer;
    
    
    
    
    PRInt32 zeroBytesWriteCount = 0;
    
    
    
    
    while (mStreamBufferByteCount > 0) {
      PRInt32 numtowrite;
      if (pluginFunctions->writeready) {
        NPPAutoPusher nppPusher(npp);
        
        NS_TRY_SAFE_CALL_RETURN(numtowrite, (*pluginFunctions->writeready)(npp, &mNPStreamWrapper->mNPStream), mInst);
        NPP_PLUGIN_LOG(PLUGIN_LOG_NOISY,
                       ("NPP WriteReady called: this=%p, npp=%p, "
                        "return(towrite)=%d, url=%s\n",
                        this, npp, numtowrite, mNPStreamWrapper->mNPStream.url));
        
        if (!mStreamStarted) {
          
          
          
          return NS_BINDING_ABORTED;
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (numtowrite <= 0 ||
            (!mIsPluginInitJSStream && PluginInitJSLoadInProgress())) {
          if (!mIsSuspended) {
            SuspendRequest();
          }
          
          
          
          
          
          break;
        }
        
        numtowrite = NS_MIN(numtowrite, mStreamBufferByteCount);
      } else {
        
        
        numtowrite = mStreamBufferByteCount;
      }
      
      NPPAutoPusher nppPusher(npp);
      
      PRInt32 writeCount = 0; 
      NS_TRY_SAFE_CALL_RETURN(writeCount, (*pluginFunctions->write)(npp, &mNPStreamWrapper->mNPStream, streamPosition, numtowrite, ptrStreamBuffer), mInst);
      
      NPP_PLUGIN_LOG(PLUGIN_LOG_NOISY,
                     ("NPP Write called: this=%p, npp=%p, pos=%d, len=%d, "
                      "buf=%s, return(written)=%d,  url=%s\n",
                      this, npp, streamPosition, numtowrite,
                      ptrStreamBuffer, writeCount, mNPStreamWrapper->mNPStream.url));
      
      if (!mStreamStarted) {
        
        
        return NS_BINDING_ABORTED;
      }
      
      if (writeCount > 0) {
        NS_ASSERTION(writeCount <= mStreamBufferByteCount,
                     "Plugin read past the end of the available data!");
        
        writeCount = NS_MIN(writeCount, mStreamBufferByteCount);
        mStreamBufferByteCount -= writeCount;
        
        streamPosition += writeCount;
        
        zeroBytesWriteCount = 0;
        
        if (mStreamBufferByteCount > 0) {
          
          
          
          if (writeCount % sizeof(intptr_t)) {
            
            memmove(mStreamBuffer, ptrStreamBuffer + writeCount,
                    mStreamBufferByteCount);
            ptrStreamBuffer = mStreamBuffer;
          } else {
            
            
            ptrStreamBuffer += writeCount;
          }
        }
      } else if (writeCount == 0) {
        
        
        
        
        if (mIsSuspended || ++zeroBytesWriteCount == 3) {
          if (!mIsSuspended) {
            SuspendRequest();
          }
          
          
          
          
          
          break;
        }
      } else {
        
        rv = NS_ERROR_FAILURE;
        
        break;
      }  
    } 
    
    if (mStreamBufferByteCount && mStreamBuffer != ptrStreamBuffer) {
      memmove(mStreamBuffer, ptrStreamBuffer, mStreamBufferByteCount);
    }
  }
  
  if (streamPosition != streamOffset) {
    
    
    
    
    
    
    PRInt32 postWriteStreamPosition;
    pluginInfo->GetStreamOffset(&postWriteStreamPosition);
    
    if (postWriteStreamPosition == streamOffset) {
      pluginInfo->SetStreamOffset(streamPosition);
    }
  }
  
  return rv;
}

nsresult
nsNPAPIPluginStreamListener::OnFileAvailable(nsIPluginStreamInfo* pluginInfo, 
                                             const char* fileName)
{
  if (!mInst || !mInst->CanFireNotifications())
    return NS_ERROR_FAILURE;
  
  PluginDestructionGuard guard(mInst);

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin || !plugin->GetLibrary())
    return NS_ERROR_FAILURE;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  if (!pluginFunctions->asfile)
    return NS_ERROR_FAILURE;

  NPP npp;
  mInst->GetNPP(&npp);
  
  NS_TRY_SAFE_CALL_VOID((*pluginFunctions->asfile)(npp, &mNPStreamWrapper->mNPStream, fileName), mInst);
  
  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                 ("NPP StreamAsFile called: this=%p, npp=%p, url=%s, file=%s\n",
                  this, npp, mNPStreamWrapper->mNPStream.url, fileName));
  
  return NS_OK;
}

nsresult
nsNPAPIPluginStreamListener::OnStopBinding(nsIPluginStreamInfo* pluginInfo, 
                                           nsresult status)
{
  StopDataPump();
  
  if (NS_FAILED(status)) {
    
    
    nsCOMPtr<nsINPAPIPluginStreamInfo> pluginInfoNPAPI = do_QueryInterface(mStreamInfo);
    if (pluginInfoNPAPI) {
      pluginInfoNPAPI->CancelRequests(status);
    }
  }
  
  if (!mInst || !mInst->CanFireNotifications())
    return NS_ERROR_FAILURE;

  NPReason reason = NS_FAILED(status) ? NPRES_NETWORK_ERR : NPRES_DONE;
  if (mRedirectDenied) {
    reason = NPRES_USER_BREAK;
  }

  
  
  
  
  
  
  
  
  if (mStreamType != NP_SEEK ||
      (NP_SEEK == mStreamType && NS_BINDING_ABORTED == status)) {
    return CleanUpStream(reason);
  }

  return NS_OK;
}

nsresult
nsNPAPIPluginStreamListener::GetStreamType(PRInt32 *result)
{
  *result = mStreamType;
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginStreamListener::Notify(nsITimer *aTimer)
{
  NS_ASSERTION(aTimer == mDataPumpTimer, "Uh, wrong timer?");
  
  PRInt32 oldStreamBufferByteCount = mStreamBufferByteCount;
  
  nsresult rv = OnDataAvailable(mStreamInfo, nsnull, mStreamBufferByteCount);
  
  if (NS_FAILED(rv)) {
    
    aTimer->Cancel();
    return NS_OK;
  }
  
  if (mStreamBufferByteCount != oldStreamBufferByteCount &&
      ((mStreamStarted && mStreamBufferByteCount < 1024) ||
       mStreamBufferByteCount == 0)) {
        
        
        
        
        ResumeRequest();
        
        StopDataPump();
      }
  
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginStreamListener::StatusLine(const char* line)
{
  mResponseHeaders.Append(line);
  mResponseHeaders.Append('\n');
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginStreamListener::NewResponseHeader(const char* headerName,
                                               const char* headerValue)
{
  mResponseHeaders.Append(headerName);
  mResponseHeaders.Append(": ");
  mResponseHeaders.Append(headerValue);
  mResponseHeaders.Append('\n');
  return NS_OK;
}

bool
nsNPAPIPluginStreamListener::HandleRedirectNotification(nsIChannel *oldChannel, nsIChannel *newChannel,
                                                        nsIAsyncVerifyRedirectCallback* callback)
{
  nsCOMPtr<nsIHttpChannel> oldHttpChannel = do_QueryInterface(oldChannel);
  nsCOMPtr<nsIHttpChannel> newHttpChannel = do_QueryInterface(newChannel);
  if (!oldHttpChannel || !newHttpChannel) {
    return false;
  }

  if (!mInst || !mInst->CanFireNotifications()) {
    return false;
  }

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin || !plugin->GetLibrary()) {
    return false;
  }

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();
  if (!pluginFunctions->urlredirectnotify) {
    return false;
  }

  
  if (mNPStreamWrapper->mNPStream.notifyData) {
    PRUint32 status;
    if (NS_SUCCEEDED(oldHttpChannel->GetResponseStatus(&status))) {
      nsCOMPtr<nsIURI> uri;
      if (NS_SUCCEEDED(newHttpChannel->GetURI(getter_AddRefs(uri))) && uri) {
        nsCAutoString spec;
        if (NS_SUCCEEDED(uri->GetAsciiSpec(spec))) {
          
          
          mHTTPRedirectCallback = callback;

          NPP npp;
          mInst->GetNPP(&npp);
#if defined(XP_WIN) || defined(XP_OS2)
          NS_TRY_SAFE_CALL_VOID((*pluginFunctions->urlredirectnotify)(npp, spec.get(), static_cast<int32_t>(status), mNPStreamWrapper->mNPStream.notifyData), mInst);
#else
          (*pluginFunctions->urlredirectnotify)(npp, spec.get(), static_cast<int32_t>(status), mNPStreamWrapper->mNPStream.notifyData);
#endif
          return true;
        }
      }
    }
  }

  callback->OnRedirectVerifyCallback(NS_ERROR_FAILURE);
  return true;
}

void
nsNPAPIPluginStreamListener::URLRedirectResponse(NPBool allow)
{
  if (mHTTPRedirectCallback) {
    mHTTPRedirectCallback->OnRedirectVerifyCallback(allow ? NS_OK : NS_ERROR_FAILURE);
    mRedirectDenied = allow ? false : true;
    mHTTPRedirectCallback = nsnull;
  }
}

void*
nsNPAPIPluginStreamListener::GetNotifyData()
{
  if (mNPStreamWrapper) {
    return mNPStreamWrapper->mNPStream.notifyData;
  }
  return nsnull;
}
