




#include "nsNPAPIPluginStreamListener.h"
#include "plstr.h"
#include "prmem.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIFile.h"
#include "nsNetUtil.h"
#include "nsPluginHost.h"
#include "nsNPAPIPlugin.h"
#include "nsPluginLogging.h"
#include "nsPluginStreamListenerPeer.h"

#include <stdint.h>
#include <algorithm>

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

NS_IMPL_ISUPPORTS(nsPluginStreamToFile, nsIOutputStream)

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
  
#ifdef DEBUG
  printf("File URL = %s\n", mFileURL.get());
#endif
}

nsPluginStreamToFile::~nsPluginStreamToFile()
{
  
  if (nullptr != mTarget)
    PL_strfree(mTarget);
}

NS_IMETHODIMP
nsPluginStreamToFile::Flush()
{
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamToFile::Write(const char* aBuf, uint32_t aCount,
                            uint32_t *aWriteCount)
{
  mOutputStream->Write(aBuf, aCount, aWriteCount);
  mOutputStream->Flush();
  mOwner->GetURL(mFileURL.get(), mTarget, nullptr, nullptr, 0);
  
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamToFile::WriteFrom(nsIInputStream *inStr, uint32_t count,
                                uint32_t *_retval)
{
  NS_NOTREACHED("WriteFrom");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPluginStreamToFile::WriteSegments(nsReadSegmentFun reader, void * closure,
                                    uint32_t count, uint32_t *_retval)
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
  mOwner->GetURL(mFileURL.get(), mTarget, nullptr, nullptr, 0);
  return NS_OK;
}



NS_IMPL_ISUPPORTS(nsNPAPIPluginStreamListener,
                  nsITimerCallback, nsIHTTPHeaderListener)

nsNPAPIPluginStreamListener::nsNPAPIPluginStreamListener(nsNPAPIPluginInstance* inst, 
                                                         void* notifyData,
                                                         const char* aURL)
  : mStreamBuffer(nullptr)
  , mNotifyURL(aURL ? PL_strdup(aURL) : nullptr)
  , mInst(inst)
  , mStreamBufferSize(0)
  , mStreamBufferByteCount(0)
  , mStreamType(NP_NORMAL)
  , mStreamState(eStreamStopped)
  , mStreamCleanedUp(false)
  , mCallNotify(notifyData ? true : false)
  , mIsSuspended(false)
  , mIsPluginInitJSStream(mInst->mInPluginInitCall &&
                          aURL && strncmp(aURL, "javascript:",
                                          sizeof("javascript:") - 1) == 0)
  , mRedirectDenied(false)
  , mResponseHeaderBuf(nullptr)
{
  mNPStreamWrapper = new nsNPAPIStreamWrapper(nullptr, this);
  mNPStreamWrapper->mNPStream.notifyData = notifyData;
}

nsNPAPIPluginStreamListener::~nsNPAPIPluginStreamListener()
{
  
  nsTArray<nsNPAPIPluginStreamListener*> *streamListeners = mInst->StreamListeners();
  streamListeners->RemoveElement(this);

  
  
  
  
  CallURLNotify(NPRES_NETWORK_ERR);
  
  
  if (mStreamBuffer) {
    PR_Free(mStreamBuffer);
    mStreamBuffer=nullptr;
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
    mHTTPRedirectCallback = nullptr;
  }

  
  
  if (NP_SEEK == mStreamType && mStreamState == eStreamTypeSet)
    NS_RELEASE_THIS();

  if (mStreamListenerPeer) {
    mStreamListenerPeer->CancelRequests(NS_BINDING_ABORTED);
    mStreamListenerPeer = nullptr;
  }

  if (!mInst || !mInst->CanFireNotifications())
    return rv;
  
  PluginDestructionGuard guard(mInst);

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin || !plugin->GetLibrary())
    return rv;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  NPP npp;
  mInst->GetNPP(&npp);

  if (mStreamState >= eNewStreamCalled && pluginFunctions->destroystream) {
    NPPAutoPusher nppPusher(npp);

    NPError error;
    NS_TRY_SAFE_CALL_RETURN(error, (*pluginFunctions->destroystream)(npp, &mNPStreamWrapper->mNPStream, reason), mInst,
                            NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO);
    
    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                   ("NPP DestroyStream called: this=%p, npp=%p, reason=%d, return=%d, url=%s\n",
                    this, npp, reason, error, mNPStreamWrapper->mNPStream.url));
    
    if (error == NPERR_NO_ERROR)
      rv = NS_OK;
  }
  
  mStreamState = eStreamStopped;
  
  
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
    
    NS_TRY_SAFE_CALL_VOID((*pluginFunctions->urlnotify)(npp, mNotifyURL, reason, mNPStreamWrapper->mNPStream.notifyData), mInst,
                          NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO);
    
    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                   ("NPP URLNotify called: this=%p, npp=%p, notify=%p, reason=%d, url=%s\n",
                    this, npp, mNPStreamWrapper->mNPStream.notifyData, reason, mNotifyURL));
  }
}

nsresult
nsNPAPIPluginStreamListener::OnStartBinding(nsPluginStreamListenerPeer* streamPeer)
{
  if (!mInst || !mInst->CanFireNotifications() || mStreamCleanedUp)
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
  uint16_t streamType = NP_NORMAL;
  NPError error;

  streamPeer->GetURL(&mNPStreamWrapper->mNPStream.url);
  streamPeer->GetLength((uint32_t*)&(mNPStreamWrapper->mNPStream.end));
  streamPeer->GetLastModified((uint32_t*)&(mNPStreamWrapper->mNPStream.lastmodified));
  streamPeer->IsSeekable(&seekable);
  streamPeer->GetContentType(&contentType);
  
  if (!mResponseHeaders.IsEmpty()) {
    mResponseHeaderBuf = PL_strdup(mResponseHeaders.get());
    mNPStreamWrapper->mNPStream.headers = mResponseHeaderBuf;
  }
  
  mStreamListenerPeer = streamPeer;
  
  NPPAutoPusher nppPusher(npp);
  
  NS_TRY_SAFE_CALL_RETURN(error, (*pluginFunctions->newstream)(npp, (char*)contentType, &mNPStreamWrapper->mNPStream, seekable, &streamType), mInst,
                          NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO);
  
  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                 ("NPP NewStream called: this=%p, npp=%p, mime=%s, seek=%d, type=%d, return=%d, url=%s\n",
                  this, npp, (char *)contentType, seekable, streamType, error, mNPStreamWrapper->mNPStream.url));
  
  if (error != NPERR_NO_ERROR)
    return NS_ERROR_FAILURE;

  mStreamState = eNewStreamCalled;

  if (streamType == nsPluginStreamListenerPeer::STREAM_TYPE_UNKNOWN) {
    SuspendRequest();
  }
  if (!SetStreamType(streamType, false)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

bool
nsNPAPIPluginStreamListener::SetStreamType(uint16_t aType, bool aNeedsResume)
{
  switch(aType)
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
    case nsPluginStreamListenerPeer::STREAM_TYPE_UNKNOWN:
      MOZ_ASSERT(!aNeedsResume);
      mStreamType = nsPluginStreamListenerPeer::STREAM_TYPE_UNKNOWN;
      
      
      return true;
    default:
      return false;
  }
  mStreamState = eStreamTypeSet;
  if (aNeedsResume) {
    if (mStreamListenerPeer) {
      mStreamListenerPeer->OnStreamTypeSet(mStreamType);
    }
    ResumeRequest();
  }
  return true;
}

void
nsNPAPIPluginStreamListener::SuspendRequest()
{
  NS_ASSERTION(!mIsSuspended,
               "Suspending a request that's already suspended!");

  nsresult rv = StartDataPump();
  if (NS_FAILED(rv))
    return;

  mIsSuspended = true;

  if (mStreamListenerPeer) {
    mStreamListenerPeer->SuspendRequests();
  }
}

void
nsNPAPIPluginStreamListener::ResumeRequest()
{
  if (mStreamListenerPeer) {
    mStreamListenerPeer->ResumeRequests();
  }
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
    mDataPumpTimer = nullptr;
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
nsNPAPIPluginStreamListener::OnDataAvailable(nsPluginStreamListenerPeer* streamPeer,
                                             nsIInputStream* input,
                                             uint32_t length)
{
  if (!length || !mInst || !mInst->CanFireNotifications())
    return NS_ERROR_FAILURE;
  
  PluginDestructionGuard guard(mInst);
  
  
  mStreamListenerPeer = streamPeer;

  nsNPAPIPlugin* plugin = mInst->GetPlugin();
  if (!plugin || !plugin->GetLibrary())
    return NS_ERROR_FAILURE;

  NPPluginFuncs* pluginFunctions = plugin->PluginFuncs();

  
  if (!pluginFunctions->write)
    return NS_ERROR_FAILURE; 
  
  if (!mStreamBuffer) {
    
    
    
    
    
    
    uint32_t contentLength;
    streamPeer->GetLength(&contentLength);
    
    mStreamBufferSize = std::max(length, contentLength);
    
    
    
    
    mStreamBufferSize = std::min(mStreamBufferSize,
                               uint32_t(MAX_PLUGIN_NECKO_BUFFER));
    
    mStreamBuffer = (char*) PR_Malloc(mStreamBufferSize);
    if (!mStreamBuffer)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  
  
  NPP npp;
  mInst->GetNPP(&npp);
  
  int32_t streamPosition;
  streamPeer->GetStreamOffset(&streamPosition);
  int32_t streamOffset = streamPosition;
  
  if (input) {
    streamOffset += length;
    
    
    
    
    
    
    
    
    
    
    streamPeer->SetStreamOffset(streamOffset);
    
    
    
    
    if ((int32_t)mNPStreamWrapper->mNPStream.end < streamOffset)
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
      
      uint32_t bytesToRead =
      std::min(length, mStreamBufferSize - mStreamBufferByteCount);
      
      uint32_t amountRead = 0;
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
    
    
    
    
    int32_t zeroBytesWriteCount = 0;
    
    
    
    
    while (mStreamBufferByteCount > 0) {
      int32_t numtowrite;
      if (pluginFunctions->writeready) {
        NPPAutoPusher nppPusher(npp);
        
        NS_TRY_SAFE_CALL_RETURN(numtowrite, (*pluginFunctions->writeready)(npp, &mNPStreamWrapper->mNPStream), mInst,
                                NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO);
        NPP_PLUGIN_LOG(PLUGIN_LOG_NOISY,
                       ("NPP WriteReady called: this=%p, npp=%p, "
                        "return(towrite)=%d, url=%s\n",
                        this, npp, numtowrite, mNPStreamWrapper->mNPStream.url));
        
        if (mStreamState == eStreamStopped) {
          
          
          
          return NS_BINDING_ABORTED;
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (numtowrite <= 0 ||
            (!mIsPluginInitJSStream && PluginInitJSLoadInProgress())) {
          if (!mIsSuspended) {
            SuspendRequest();
          }
          
          
          
          
          
          break;
        }
        
        numtowrite = std::min(numtowrite, mStreamBufferByteCount);
      } else {
        
        
        numtowrite = mStreamBufferByteCount;
      }
      
      NPPAutoPusher nppPusher(npp);
      
      int32_t writeCount = 0; 
      NS_TRY_SAFE_CALL_RETURN(writeCount, (*pluginFunctions->write)(npp, &mNPStreamWrapper->mNPStream, streamPosition, numtowrite, ptrStreamBuffer), mInst,
                              NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO);
      
      NPP_PLUGIN_LOG(PLUGIN_LOG_NOISY,
                     ("NPP Write called: this=%p, npp=%p, pos=%d, len=%d, "
                      "buf=%s, return(written)=%d,  url=%s\n",
                      this, npp, streamPosition, numtowrite,
                      ptrStreamBuffer, writeCount, mNPStreamWrapper->mNPStream.url));
      
      if (mStreamState == eStreamStopped) {
        
        
        return NS_BINDING_ABORTED;
      }
      
      if (writeCount > 0) {
        NS_ASSERTION(writeCount <= mStreamBufferByteCount,
                     "Plugin read past the end of the available data!");
        
        writeCount = std::min(writeCount, mStreamBufferByteCount);
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
    
    
    
    
    
    
    int32_t postWriteStreamPosition;
    streamPeer->GetStreamOffset(&postWriteStreamPosition);
    
    if (postWriteStreamPosition == streamOffset) {
      streamPeer->SetStreamOffset(streamPosition);
    }
  }
  
  return rv;
}

nsresult
nsNPAPIPluginStreamListener::OnFileAvailable(nsPluginStreamListenerPeer* streamPeer, 
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
  
  NS_TRY_SAFE_CALL_VOID((*pluginFunctions->asfile)(npp, &mNPStreamWrapper->mNPStream, fileName), mInst,
                        NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO);
  
  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
                 ("NPP StreamAsFile called: this=%p, npp=%p, url=%s, file=%s\n",
                  this, npp, mNPStreamWrapper->mNPStream.url, fileName));
  
  return NS_OK;
}

nsresult
nsNPAPIPluginStreamListener::OnStopBinding(nsPluginStreamListenerPeer* streamPeer, 
                                           nsresult status)
{
  StopDataPump();
  
  if (NS_FAILED(status)) {
    
    
    if (mStreamListenerPeer) {
      mStreamListenerPeer->CancelRequests(status);
    }
  }
  
  if (!mInst || !mInst->CanFireNotifications())
    return NS_ERROR_FAILURE;

  NPReason reason = NS_FAILED(status) ? NPRES_NETWORK_ERR : NPRES_DONE;
  if (mRedirectDenied || status == NS_BINDING_ABORTED) {
    reason = NPRES_USER_BREAK;
  }

  
  
  
  
  
  
  
  
  if (mStreamType != NP_SEEK ||
      (NP_SEEK == mStreamType && NS_BINDING_ABORTED == status)) {
    return CleanUpStream(reason);
  }

  return NS_OK;
}

nsresult
nsNPAPIPluginStreamListener::GetStreamType(int32_t *result)
{
  *result = mStreamType;
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginStreamListener::Notify(nsITimer *aTimer)
{
  NS_ASSERTION(aTimer == mDataPumpTimer, "Uh, wrong timer?");
  
  int32_t oldStreamBufferByteCount = mStreamBufferByteCount;
  
  nsresult rv = OnDataAvailable(mStreamListenerPeer, nullptr, mStreamBufferByteCount);
  
  if (NS_FAILED(rv)) {
    
    aTimer->Cancel();
    return NS_OK;
  }
  
  if (mStreamBufferByteCount != oldStreamBufferByteCount &&
      ((mStreamState == eStreamTypeSet && mStreamBufferByteCount < 1024) ||
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
  mResponseHeaders.AppendLiteral(": ");
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
    uint32_t status;
    if (NS_SUCCEEDED(oldHttpChannel->GetResponseStatus(&status))) {
      nsCOMPtr<nsIURI> uri;
      if (NS_SUCCEEDED(newHttpChannel->GetURI(getter_AddRefs(uri))) && uri) {
        nsAutoCString spec;
        if (NS_SUCCEEDED(uri->GetAsciiSpec(spec))) {
          
          
          mHTTPRedirectCallback = callback;

          NPP npp;
          mInst->GetNPP(&npp);
#if defined(XP_WIN)
          NS_TRY_SAFE_CALL_VOID((*pluginFunctions->urlredirectnotify)(npp, spec.get(), static_cast<int32_t>(status), mNPStreamWrapper->mNPStream.notifyData), mInst,
                                NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO);
#else
          MAIN_THREAD_JNI_REF_GUARD;
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
    mHTTPRedirectCallback = nullptr;
  }
}

void*
nsNPAPIPluginStreamListener::GetNotifyData()
{
  if (mNPStreamWrapper) {
    return mNPStreamWrapper->mNPStream.notifyData;
  }
  return nullptr;
}
