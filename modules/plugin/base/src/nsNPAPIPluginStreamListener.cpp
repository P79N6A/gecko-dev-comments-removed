




































#include "nsNPAPIPluginStreamListener.h"
#include "plstr.h"
#include "prmem.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsILocalFile.h"
#include "nsNetUtil.h"
#include "nsPluginHost.h"
#include "nsNPAPIPlugin.h"
#include "nsPluginSafety.h"
#include "nsPluginLogging.h"

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
nsPluginStreamToFile::IsNonBlocking(PRBool *aNonBlocking)
{
  *aNonBlocking = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamToFile::Close(void)
{
  mOutputStream->Close();
  mOwner->GetURL(mFileURL.get(), mTarget, nsnull, nsnull, 0);
  return NS_OK;
}



NS_IMPL_ISUPPORTS3(nsNPAPIPluginStreamListener, nsIPluginStreamListener,
                   nsITimerCallback, nsIHTTPHeaderListener)

nsNPAPIPluginStreamListener::nsNPAPIPluginStreamListener(nsNPAPIPluginInstance* inst, 
                                                         void* notifyData,
                                                         const char* aURL)
: mNotifyData(notifyData),
mStreamBuffer(nsnull),
mNotifyURL(aURL ? PL_strdup(aURL) : nsnull),
mInst(inst),
mStreamBufferSize(0),
mStreamBufferByteCount(0),
mStreamType(NP_NORMAL),
mStreamStarted(PR_FALSE),
mStreamCleanedUp(PR_FALSE),
mCallNotify(PR_FALSE),
mIsSuspended(PR_FALSE),
mIsPluginInitJSStream(mInst->mInPluginInitCall &&
                      aURL && strncmp(aURL, "javascript:",
                                      sizeof("javascript:") - 1) == 0),
mResponseHeaderBuf(nsnull)
{
  memset(&mNPStream, 0, sizeof(mNPStream));
}

nsNPAPIPluginStreamListener::~nsNPAPIPluginStreamListener()
{
  
  nsTArray<nsNPAPIPluginStreamListener*> *pStreamListeners = mInst->PStreamListeners();
  pStreamListeners->RemoveElement(this);

  
  
  
  
  CallURLNotify(NPRES_NETWORK_ERR);
  
  
  if (mStreamBuffer) {
    PR_Free(mStreamBuffer);
    mStreamBuffer=nsnull;
  }
  
  if (mNotifyURL)
    PL_strfree(mNotifyURL);
  
  if (mResponseHeaderBuf)
    PL_strfree(mResponseHeaderBuf);
}

nsresult
nsNPAPIPluginStreamListener::CleanUpStream(NPReason reason)
{
  nsresult rv = NS_ERROR_FAILURE;
  
  if (mStreamCleanedUp)
    return NS_OK;
  
  mStreamCleanedUp = PR_TRUE;
  
  StopDataPump();
  
  
  
  if (NP_SEEK == mStreamType)
    NS_RELEASE_THIS();
  
  if (!mInst || !mInst->CanFireNotifications())
    return rv;
  
  mStreamInfo = NULL;
  
  PluginDestructionGuard guard(mInst);

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin)
    return rv;

  PluginLibrary* library = plugin->GetLibrary();
  if (!library)
    return rv;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  NPP npp;
  mInst->GetNPP(&npp);

  if (mStreamStarted && pluginFunctions->destroystream) {
    NPPAutoPusher nppPusher(npp);

    NPError error;
    NS_TRY_SAFE_CALL_RETURN(error, (*pluginFunctions->destroystream)(npp, &mNPStream, reason), library, mInst);
    
    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                   ("NPP DestroyStream called: this=%p, npp=%p, reason=%d, return=%d, url=%s\n",
                    this, npp, reason, error, mNPStream.url));
    
    if (error == NPERR_NO_ERROR)
      rv = NS_OK;
  }
  
  mStreamStarted = PR_FALSE;
  
  
  CallURLNotify(reason);
  
  return rv;
}

void
nsNPAPIPluginStreamListener::CallURLNotify(NPReason reason)
{
  if (!mCallNotify || !mInst || !mInst->CanFireNotifications())
    return;
  
  PluginDestructionGuard guard(mInst);
  
  mCallNotify = PR_FALSE; 

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin)
    return;

  PluginLibrary* library = plugin->GetLibrary();
  if (!library)
    return;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  if (pluginFunctions->urlnotify) {
    NPP npp;
    mInst->GetNPP(&npp);
    
    NS_TRY_SAFE_CALL_VOID((*pluginFunctions->urlnotify)(npp, mNotifyURL, reason, mNotifyData), library, mInst);
    
    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                   ("NPP URLNotify called: this=%p, npp=%p, notify=%p, reason=%d, url=%s\n",
                    this, npp, mNotifyData, reason, mNotifyURL));
  }
}

NS_IMETHODIMP
nsNPAPIPluginStreamListener::OnStartBinding(nsIPluginStreamInfo* pluginInfo)
{
  if (!mInst || !mInst->CanFireNotifications())
    return NS_ERROR_FAILURE;

  PluginDestructionGuard guard(mInst);

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin)
    return NS_ERROR_FAILURE;

  PluginLibrary* library = plugin->GetLibrary();
  if (!library)
    return NS_ERROR_FAILURE;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  if (!pluginFunctions->newstream)
    return NS_ERROR_FAILURE;

  NPP npp;
  mInst->GetNPP(&npp);

  PRBool seekable;
  char* contentType;
  PRUint16 streamType = NP_NORMAL;
  NPError error;
  
  mNPStream.ndata = (void*) this;
  pluginInfo->GetURL(&mNPStream.url);
  mNPStream.notifyData = mNotifyData;
  
  pluginInfo->GetLength((PRUint32*)&(mNPStream.end));
  pluginInfo->GetLastModified((PRUint32*)&(mNPStream.lastmodified));
  pluginInfo->IsSeekable(&seekable);
  pluginInfo->GetContentType(&contentType);
  
  if (!mResponseHeaders.IsEmpty()) {
    mResponseHeaderBuf = PL_strdup(mResponseHeaders.get());
    mNPStream.headers = mResponseHeaderBuf;
  }
  
  mStreamInfo = pluginInfo;
  
  NPPAutoPusher nppPusher(npp);
  
  NS_TRY_SAFE_CALL_RETURN(error, (*pluginFunctions->newstream)(npp, (char*)contentType, &mNPStream, seekable, &streamType), library, mInst);
  
  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                 ("NPP NewStream called: this=%p, npp=%p, mime=%s, seek=%d, type=%d, return=%d, url=%s\n",
                  this, npp, (char *)contentType, seekable, streamType, error, mNPStream.url));
  
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
  
  mStreamStarted = PR_TRUE;
  return NS_OK;
}

nsresult
nsNPAPIPluginStreamListener::SuspendRequest()
{
  NS_ASSERTION(!mIsSuspended,
               "Suspending a request that's already suspended!");
  
  nsCOMPtr<nsINPAPIPluginStreamInfo> pluginInfoNPAPI =
  do_QueryInterface(mStreamInfo);
  nsIRequest *request;
  
  if (!pluginInfoNPAPI || !(request = pluginInfoNPAPI->GetRequest())) {
    NS_ERROR("Trying to suspend a non-suspendable stream!");
    return NS_ERROR_FAILURE;
  }
  
  nsresult rv = StartDataPump();
  NS_ENSURE_SUCCESS(rv, rv);
  
  mIsSuspended = PR_TRUE;
  
  return request->Suspend();
}

void
nsNPAPIPluginStreamListener::ResumeRequest()
{
  nsCOMPtr<nsINPAPIPluginStreamInfo> pluginInfoNPAPI =
  do_QueryInterface(mStreamInfo);
  
  nsIRequest *request = pluginInfoNPAPI->GetRequest();
  
  
  if (request)
    request->Resume();
  
  mIsSuspended = PR_FALSE;
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



PRBool
nsNPAPIPluginStreamListener::PluginInitJSLoadInProgress()
{
  if (!mInst)
    return PR_FALSE;

  nsTArray<nsNPAPIPluginStreamListener*> *pStreamListeners = mInst->PStreamListeners();
  for (unsigned int i = 0; i < pStreamListeners->Length(); i++) {
    if (pStreamListeners->ElementAt(i)->mIsPluginInitJSStream) {
      return PR_TRUE;
    }
  }
  
  return PR_FALSE;
}








NS_IMETHODIMP
nsNPAPIPluginStreamListener::OnDataAvailable(nsIPluginStreamInfo* pluginInfo,
                                             nsIInputStream* input,
                                             PRUint32 length)
{
  if (!length || !mInst || !mInst->CanFireNotifications())
    return NS_ERROR_FAILURE;
  
  PluginDestructionGuard guard(mInst);
  
  
  mStreamInfo = pluginInfo;

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin)
    return NS_ERROR_FAILURE;

  PluginLibrary* library = plugin->GetLibrary();
  if (!library)
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
    
    
    
    
    if ((PRInt32)mNPStream.end < streamOffset)
      mNPStream.end = streamOffset;
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
        
        NS_TRY_SAFE_CALL_RETURN(numtowrite, (*pluginFunctions->writeready)(npp, &mNPStream), library, mInst);
        NPP_PLUGIN_LOG(PLUGIN_LOG_NOISY,
                       ("NPP WriteReady called: this=%p, npp=%p, "
                        "return(towrite)=%d, url=%s\n",
                        this, npp, numtowrite, mNPStream.url));
        
        if (!mStreamStarted) {
          
          
          
          return NS_BINDING_ABORTED;
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (numtowrite <= 0 ||
            (!mIsPluginInitJSStream && PluginInitJSLoadInProgress())) {
          if (!mIsSuspended) {
            rv = SuspendRequest();
          }
          
          
          
          
          
          break;
        }
        
        numtowrite = NS_MIN(numtowrite, mStreamBufferByteCount);
      } else {
        
        
        numtowrite = mStreamBufferByteCount;
      }
      
      NPPAutoPusher nppPusher(npp);
      
      PRInt32 writeCount = 0; 
      NS_TRY_SAFE_CALL_RETURN(writeCount, (*pluginFunctions->write)(npp, &mNPStream, streamPosition, numtowrite, ptrStreamBuffer), library, mInst);
      
      NPP_PLUGIN_LOG(PLUGIN_LOG_NOISY,
                     ("NPP Write called: this=%p, npp=%p, pos=%d, len=%d, "
                      "buf=%s, return(written)=%d,  url=%s\n",
                      this, npp, streamPosition, numtowrite,
                      ptrStreamBuffer, writeCount, mNPStream.url));
      
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
          
          
          
          if (writeCount % sizeof(PRWord)) {
            
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
            rv = SuspendRequest();
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

NS_IMETHODIMP
nsNPAPIPluginStreamListener::OnFileAvailable(nsIPluginStreamInfo* pluginInfo, 
                                             const char* fileName)
{
  if (!mInst || !mInst->CanFireNotifications())
    return NS_ERROR_FAILURE;
  
  PluginDestructionGuard guard(mInst);

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin)
    return NS_ERROR_FAILURE;

  PluginLibrary* library = plugin->GetLibrary();
  if (!library)
    return NS_ERROR_FAILURE;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  if (!pluginFunctions->asfile)
    return NS_ERROR_FAILURE;

  NPP npp;
  mInst->GetNPP(&npp);
  
  NS_TRY_SAFE_CALL_VOID((*pluginFunctions->asfile)(npp, &mNPStream, fileName), library, mInst);
  
  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                 ("NPP StreamAsFile called: this=%p, npp=%p, url=%s, file=%s\n",
                  this, npp, mNPStream.url, fileName));
  
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginStreamListener::OnStopBinding(nsIPluginStreamInfo* pluginInfo, 
                                           nsresult status)
{
  StopDataPump();
  
  if (NS_FAILED(status)) {
    
    
    nsCOMPtr<nsINPAPIPluginStreamInfo> pluginInfoNPAPI =
    do_QueryInterface(mStreamInfo);
    
    nsIRequest *request;
    if (pluginInfoNPAPI && (request = pluginInfoNPAPI->GetRequest())) {
      request->Cancel(status);
    }
  }
  
  if (!mInst || !mInst->CanFireNotifications())
    return NS_ERROR_FAILURE;
  
  
  
  nsresult rv = NS_OK;
  NPReason reason = NS_FAILED(status) ? NPRES_NETWORK_ERR : NPRES_DONE;
  if (mStreamType != NP_SEEK ||
      (NP_SEEK == mStreamType && NS_BINDING_ABORTED == status)) {
    rv = CleanUpStream(reason);
  }
  
  return rv;
}

NS_IMETHODIMP
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
