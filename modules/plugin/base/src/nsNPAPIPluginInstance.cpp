






































#include "prlog.h"
#include "prmem.h"
#include "nscore.h"
#include "prenv.h"

#include "nsNPAPIPluginInstance.h"
#include "nsNPAPIPlugin.h"
#include "nsNPAPIPluginStreamListener.h"
#include "nsPluginHost.h"
#include "nsPluginSafety.h"
#include "nsPluginLogging.h"
#include "nsIPrivateBrowsingService.h"

#include "nsIDocument.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsDirectoryServiceDefs.h"

#include "nsJSNPRuntime.h"

static NS_DEFINE_IID(kIPluginStreamListenerIID, NS_IPLUGINSTREAMLISTENER_IID);







static NS_DEFINE_IID(kIOutputStreamIID, NS_IOUTPUTSTREAM_IID);

class nsPluginStreamToFile : public nsIOutputStream
{
public:
  nsPluginStreamToFile(const char* target, nsIPluginInstanceOwner* owner);
  virtual ~nsPluginStreamToFile();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM
protected:
  char* mTarget;
  nsCString mFileURL;
  nsCOMPtr<nsILocalFile> mTempFile;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsIPluginInstanceOwner* mOwner;
};

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
  mOwner->GetURL(mFileURL.get(), mTarget, nsnull, 0, nsnull, 0);
  
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
  mOwner->GetURL(mFileURL.get(), mTarget, nsnull, 0, nsnull, 0);
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

  NS_IF_ADDREF(mInst);
}

nsNPAPIPluginStreamListener::~nsNPAPIPluginStreamListener(void)
{
  
  nsNPAPIPluginInstance *inst = mInst;
  if (inst) {
    nsInstanceStream * prev = nsnull;
    for (nsInstanceStream *is = inst->mStreams; is != nsnull; is = is->mNext) {
      if (is->mPluginStreamListener == this) {
        if (!prev)
          inst->mStreams = is->mNext;
        else
          prev->mNext = is->mNext;

        delete is;
        break;
      }
      prev = is;
    }
  }

  
  
  
  
  CallURLNotify(NPRES_NETWORK_ERR);

  
  if (mStreamBuffer) {
    PR_Free(mStreamBuffer);
    mStreamBuffer=nsnull;
  }

  NS_IF_RELEASE(inst);

  if (mNotifyURL)
    PL_strfree(mNotifyURL);

  if (mResponseHeaderBuf)
    PL_strfree(mResponseHeaderBuf);
}

nsresult nsNPAPIPluginStreamListener::CleanUpStream(NPReason reason)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (mStreamCleanedUp)
    return NS_OK;

  if (!mInst || !mInst->IsStarted())
    return rv;

  PluginDestructionGuard guard(mInst);

  const NPPluginFuncs *callbacks = nsnull;
  mInst->GetCallbacks(&callbacks);
  if (!callbacks)
    return rv;

  NPP npp;
  mInst->GetNPP(&npp);

  if (mStreamStarted && callbacks->destroystream) {
    PRLibrary* lib = nsnull;
    lib = mInst->mLibrary;
    NPError error;
    NS_TRY_SAFE_CALL_RETURN(error, (*callbacks->destroystream)(npp, &mNPStream, reason), lib, mInst);

    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
    ("NPP DestroyStream called: this=%p, npp=%p, reason=%d, return=%d, url=%s\n",
    this, npp, reason, error, mNPStream.url));

    if (error == NPERR_NO_ERROR)
      rv = NS_OK;
  }

  mStreamCleanedUp = PR_TRUE;
  mStreamStarted   = PR_FALSE;

  StopDataPump();

  
  CallURLNotify(reason);

  return rv;
}

void nsNPAPIPluginStreamListener::CallURLNotify(NPReason reason)
{
  if (!mCallNotify || !mInst || !mInst->IsStarted())
    return;

  PluginDestructionGuard guard(mInst);

  mCallNotify = PR_FALSE; 

  const NPPluginFuncs *callbacks = nsnull;
  mInst->GetCallbacks(&callbacks);
  if (!callbacks)
    return;
  
  if (callbacks->urlnotify) {

    NPP npp;
    mInst->GetNPP(&npp);

    NS_TRY_SAFE_CALL_VOID((*callbacks->urlnotify)(npp, mNotifyURL, reason, mNotifyData), mInst->mLibrary, mInst);

    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
    ("NPP URLNotify called: this=%p, npp=%p, notify=%p, reason=%d, url=%s\n",
    this, npp, mNotifyData, reason, mNotifyURL));
  }
}

NS_IMETHODIMP
nsNPAPIPluginStreamListener::OnStartBinding(nsIPluginStreamInfo* pluginInfo)
{
  if (!mInst)
    return NS_ERROR_FAILURE;

  PluginDestructionGuard guard(mInst);

  NPP npp;
  const NPPluginFuncs *callbacks = nsnull;

  mInst->GetCallbacks(&callbacks);
  mInst->GetNPP(&npp);

  if (!callbacks || !mInst->IsStarted())
    return NS_ERROR_FAILURE;

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

  NS_TRY_SAFE_CALL_RETURN(error, (*callbacks->newstream)(npp, (char*)contentType, &mNPStream, seekable, &streamType), mInst->mLibrary, mInst);

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
  for (nsInstanceStream *is = mInst->mStreams; is; is = is->mNext) {
    if (is->mPluginStreamListener->mIsPluginInitJSStream) {
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
  if (!mInst || !mInst->IsStarted())
    return NS_ERROR_FAILURE;

  PluginDestructionGuard guard(mInst);

  
  mStreamInfo = pluginInfo;

  const NPPluginFuncs *callbacks = nsnull;
  mInst->GetCallbacks(&callbacks);
  
  if (!callbacks || !callbacks->write || !length)
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
      if (callbacks->writeready) {
        NS_TRY_SAFE_CALL_RETURN(numtowrite, (*callbacks->writeready)(npp, &mNPStream), mInst->mLibrary, mInst);
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

      PRInt32 writeCount = 0; 
      NS_TRY_SAFE_CALL_RETURN(writeCount, (*callbacks->write)(npp, &mNPStream, streamPosition, numtowrite, ptrStreamBuffer), mInst->mLibrary, mInst);

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
  if (!mInst || !mInst->IsStarted())
    return NS_ERROR_FAILURE;

  PluginDestructionGuard guard(mInst);

  const NPPluginFuncs *callbacks = nsnull;
  mInst->GetCallbacks(&callbacks);
  if (!callbacks || !callbacks->asfile)
    return NS_ERROR_FAILURE;
  
  NPP npp;
  mInst->GetNPP(&npp);

  PRLibrary* lib = nsnull;
  lib = mInst->mLibrary;

  NS_TRY_SAFE_CALL_VOID((*callbacks->asfile)(npp, &mNPStream, fileName), lib, mInst);

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

  if (!mInst || !mInst->IsStarted())
    return NS_ERROR_FAILURE;

  
  
  nsresult rv = NS_OK;
  if (mStreamType != NP_SEEK) {
    NPReason reason = NPRES_DONE;

    if (NS_FAILED(status))
      reason = NPRES_NETWORK_ERR;   

    rv = CleanUpStream(reason);
  }

  if (rv != NPERR_NO_ERROR)
    return NS_ERROR_FAILURE;

  return NS_OK;
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

nsInstanceStream::nsInstanceStream()
{
  mNext = nsnull;
  mPluginStreamListener = nsnull;
}

nsInstanceStream::~nsInstanceStream()
{
}

NS_IMPL_ISUPPORTS1(nsNPAPIPluginInstance, nsIPluginInstance)

nsNPAPIPluginInstance::nsNPAPIPluginInstance(NPPluginFuncs* callbacks,
                                       PRLibrary* aLibrary)
  : mCallbacks(callbacks),
#ifdef XP_MACOSX
#ifdef NP_NO_QUICKDRAW
    mDrawingModel(NPDrawingModelCoreGraphics),
#else
    mDrawingModel(NPDrawingModelQuickDraw),
#endif
#endif
    mWindowless(PR_FALSE),
    mWindowlessLocal(PR_FALSE),
    mTransparent(PR_FALSE),
    mStarted(PR_FALSE),
    mCached(PR_FALSE),
    mWantsAllNetworkStreams(PR_FALSE),
    mInPluginInitCall(PR_FALSE),
    mLibrary(aLibrary),
    mStreams(nsnull),
    mMIMEType(nsnull),
    mOwner(nsnull),
    mCurrentPluginEvent(nsnull)
{
  NS_ASSERTION(mCallbacks != NULL, "null callbacks");

  

  mNPP.pdata = NULL;
  mNPP.ndata = this;

  PLUGIN_LOG(PLUGIN_LOG_BASIC, ("nsNPAPIPluginInstance ctor: this=%p\n",this));
}

nsNPAPIPluginInstance::~nsNPAPIPluginInstance(void)
{
  PLUGIN_LOG(PLUGIN_LOG_BASIC, ("nsNPAPIPluginInstance dtor: this=%p\n",this));

  
  for (nsInstanceStream *is = mStreams; is != nsnull;) {
    nsInstanceStream * next = is->mNext;
    delete is;
    is = next;
  }

  if (mMIMEType) {
    PR_Free((void *)mMIMEType);
    mMIMEType = nsnull;
  }
}

PRBool
nsNPAPIPluginInstance::IsStarted(void)
{
  return mStarted;
}

NS_IMETHODIMP nsNPAPIPluginInstance::Initialize(nsIPluginInstanceOwner* aOwner, const char* aMIMEType)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsNPAPIPluginInstance::Initialize this=%p\n",this));

  mOwner = aOwner;

  if (aMIMEType) {
    mMIMEType = (char*)PR_Malloc(PL_strlen(aMIMEType) + 1);

    if (mMIMEType)
      PL_strcpy(mMIMEType, aMIMEType);
  }

  return InitializePlugin();
}

NS_IMETHODIMP nsNPAPIPluginInstance::Start(void)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsNPAPIPluginInstance::Start this=%p\n",this));

  if (mStarted)
    return NS_OK;

  return InitializePlugin();
}

NS_IMETHODIMP nsNPAPIPluginInstance::Stop(void)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsNPAPIPluginInstance::Stop this=%p\n",this));

  NPError error;

  
  if (mPopupStates.Length() > 0) {
    nsCOMPtr<nsPIDOMWindow> window = GetDOMWindow();

    if (window) {
      window->PopPopupControlState(openAbused);
    }
  }

  if (!mStarted) {
    return NS_OK;
  }

  
  for (PRUint32 i = mTimers.Length(); i > 0; i--)
    UnscheduleTimer(mTimers[i - 1]->id);

  
  
  if (PluginDestructionGuard::DelayDestroy(this)) {
    return NS_OK;
  }

  
  
  EnterAsyncPluginThreadCallLock();
  mStarted = PR_FALSE;
  ExitAsyncPluginThreadCallLock();

  OnPluginDestroy(&mNPP);

  if (mCallbacks->destroy == NULL) {
    return NS_ERROR_FAILURE;
  }

  NPSavedData *sdata = 0;

  
  for (nsInstanceStream *is = mStreams; is != nsnull;) {
    nsNPAPIPluginStreamListener * listener = is->mPluginStreamListener;

    nsInstanceStream *next = is->mNext;
    delete is;
    is = next;
    mStreams = is;

    
    
    if (listener)
      listener->CleanUpStream(NPRES_USER_BREAK);
  }

  NS_TRY_SAFE_CALL_RETURN(error, (*mCallbacks->destroy)(&mNPP, &sdata), mLibrary, this);

  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("NPP Destroy called: this=%p, npp=%p, return=%d\n", this, &mNPP, error));

  nsJSNPRuntime::OnPluginDestroy(&mNPP);

  if (error != NPERR_NO_ERROR)
    return NS_ERROR_FAILURE;
  else
    return NS_OK;
}

already_AddRefed<nsPIDOMWindow>
nsNPAPIPluginInstance::GetDOMWindow()
{
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  GetOwner(getter_AddRefs(owner));
  if (!owner)
    return nsnull;

  nsCOMPtr<nsIDocument> doc;
  owner->GetDocument(getter_AddRefs(doc));
  if (!doc)
    return nsnull;

  nsPIDOMWindow *window = doc->GetWindow();
  NS_IF_ADDREF(window);

  return window;
}

nsresult
nsNPAPIPluginInstance::GetTagType(nsPluginTagType *result)
{
  if (mOwner) {
    nsCOMPtr<nsIPluginTagInfo> tinfo(do_QueryInterface(mOwner));
    if (tinfo)
      return tinfo->GetTagType(result);
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsNPAPIPluginInstance::GetAttributes(PRUint16& n, const char*const*& names,
                                     const char*const*& values)
{
  if (mOwner) {
    nsCOMPtr<nsIPluginTagInfo> tinfo(do_QueryInterface(mOwner));
    if (tinfo)
      return tinfo->GetAttributes(n, names, values);
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsNPAPIPluginInstance::GetParameters(PRUint16& n, const char*const*& names,
                                     const char*const*& values)
{
  if (mOwner) {
    nsCOMPtr<nsIPluginTagInfo> tinfo(do_QueryInterface(mOwner));
    if (tinfo)
      return tinfo->GetParameters(n, names, values);
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsNPAPIPluginInstance::GetMode(PRInt32 *result)
{
  if (mOwner)
    return mOwner->GetMode(result);
  else
    return NS_ERROR_FAILURE;
}

nsresult
nsNPAPIPluginInstance::InitializePlugin()
{ 
  PluginDestructionGuard guard(this);

  PRUint16 count = 0;
  const char* const* names = nsnull;
  const char* const* values = nsnull;
  nsPluginTagType tagtype;
  nsresult rv = GetTagType(&tagtype);
  if (NS_SUCCEEDED(rv)) {
    
    rv = GetAttributes(count, names, values);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    
    
    
    
    if (tagtype != nsPluginTagType_Embed) {
      PRUint16 pcount = 0;
      const char* const* pnames = nsnull;
      const char* const* pvalues = nsnull;    
      if (NS_SUCCEEDED(GetParameters(pcount, pnames, pvalues))) {
        NS_ASSERTION(!values[count], "attribute/parameter array not setup correctly for NPAPI plugins");
        if (pcount)
          count += ++pcount; 
                             
      }
    }
  }

  NS_ENSURE_TRUE(mCallbacks->newp, NS_ERROR_FAILURE);
  
  
  
  
  PRInt32       mode;
  const char*   mimetype;
  NPError       error;

  GetMode(&mode);
  GetMIMEType(&mimetype);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  static const char flashMimeType[] = "application/x-shockwave-flash";
  static const char blockedParam[] = "swliveconnect";
  if (count && !PL_strcasecmp(mimetype, flashMimeType)) {
    static int cachedDisableHack = 0;
    if (!cachedDisableHack) {
       if (PR_GetEnv("MOZILLA_PLUGIN_DISABLE_FLASH_SWLIVECONNECT_HACK"))
         cachedDisableHack = -1;
       else
         cachedDisableHack = 1;
    }
    if (cachedDisableHack > 0) {
      for (PRUint16 i=0; i<count; i++) {
        if (!PL_strcasecmp(names[i], blockedParam)) {
          
          
          
          
          
          char *val = (char*) values[i];
          if (val && *val) {
            
            val[0] = '0';
            val[1] = 0;
          }
          break;
        }
      }
    }
  }

  
  
  
  mStarted = PR_TRUE;

  PRBool oldVal = mInPluginInitCall;
  mInPluginInitCall = PR_TRUE;

  NS_TRY_SAFE_CALL_RETURN(error, (*mCallbacks->newp)((char*)mimetype, &mNPP, (PRUint16)mode, count, (char**)names, (char**)values, NULL), mLibrary,this);

  mInPluginInitCall = oldVal;

  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("NPP New called: this=%p, npp=%p, mime=%s, mode=%d, argc=%d, return=%d\n",
  this, &mNPP, mimetype, mode, count, error));

  if (error != NPERR_NO_ERROR) {
    mStarted = PR_FALSE;
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsNPAPIPluginInstance::SetWindow(NPWindow* window)
{
  
  if (!window || !mStarted)
    return NS_OK;

#if defined(MOZ_WIDGET_GTK2)
  
  
  if (!nsPluginHost::IsJavaMIMEType(mMIMEType) && window->type == NPWindowTypeWindow &&
      (window->width <= 0 || window->height <= 0)) {
    return NS_OK;
  }
#endif

  if (mCallbacks->setwindow) {
    PluginDestructionGuard guard(this);

    
    

    PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsNPAPIPluginInstance::SetWindow (about to call it) this=%p\n",this));

    PRBool oldVal = mInPluginInitCall;
    mInPluginInitCall = PR_TRUE;

    NPError error;
    NS_TRY_SAFE_CALL_RETURN(error, (*mCallbacks->setwindow)(&mNPP, (NPWindow*)window), mLibrary, this);

    mInPluginInitCall = oldVal;

    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
    ("NPP SetWindow called: this=%p, [x=%d,y=%d,w=%d,h=%d], clip[t=%d,b=%d,l=%d,r=%d], return=%d\n",
    this, window->x, window->y, window->width, window->height,
    window->clipRect.top, window->clipRect.bottom, window->clipRect.left, window->clipRect.right, error));
  }
  return NS_OK;
}



NS_IMETHODIMP
nsNPAPIPluginInstance::NewStreamToPlugin(nsIPluginStreamListener** listener)
{
  return NewNotifyStream(listener, nsnull, PR_FALSE, nsnull);
}

NS_IMETHODIMP
nsNPAPIPluginInstance::NewStreamFromPlugin(const char* type, const char* target,
                                           nsIOutputStream* *result)
{
  nsPluginStreamToFile* stream = new nsPluginStreamToFile(target, mOwner);
  if (!stream)
    return NS_ERROR_OUT_OF_MEMORY;

  return stream->QueryInterface(kIOutputStreamIID, (void**)result);
}


nsresult nsNPAPIPluginInstance::NewNotifyStream(nsIPluginStreamListener** listener, 
                                                void* notifyData,
                                                PRBool aCallNotify,
                                                const char* aURL)
{
  nsNPAPIPluginStreamListener* stream = new nsNPAPIPluginStreamListener(this, notifyData, aURL);
  NS_ENSURE_TRUE(stream, NS_ERROR_OUT_OF_MEMORY);

  
  nsInstanceStream * is = new nsInstanceStream();
  NS_ENSURE_TRUE(is, NS_ERROR_OUT_OF_MEMORY);

  is->mNext = mStreams;
  is->mPluginStreamListener = stream;
  mStreams = is;
  stream->SetCallNotify(aCallNotify); 

  NS_ADDREF(stream); 
    
  nsresult res = stream->QueryInterface(kIPluginStreamListenerIID, (void**)listener);

  
  NS_RELEASE(stream);

  return res;
}

NS_IMETHODIMP nsNPAPIPluginInstance::Print(NPPrint* platformPrint)
{
  NS_ENSURE_TRUE(platformPrint, NS_ERROR_NULL_POINTER);

  PluginDestructionGuard guard(this);

  NPPrint* thePrint = (NPPrint *)platformPrint;

  
  
  
  PRUint16 sdkmajorversion = (mCallbacks->version & 0xff00)>>8;
  PRUint16 sdkminorversion = mCallbacks->version & 0x00ff;
  if ((sdkmajorversion == 0) && (sdkminorversion < 11)) {
    
    
    
    if (sizeof(NPWindowType) >= sizeof(void *)) {
      void* source = thePrint->print.embedPrint.platformPrint;
      void** destination = (void **)&(thePrint->print.embedPrint.window.type);
      *destination = source;
    } else {
      NS_ERROR("Incompatible OS for assignment");
    }
  }

  if (mCallbacks->print)
      NS_TRY_SAFE_CALL_VOID((*mCallbacks->print)(&mNPP, thePrint), mLibrary, this);

  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("NPP PrintProc called: this=%p, pDC=%p, [x=%d,y=%d,w=%d,h=%d], clip[t=%d,b=%d,l=%d,r=%d]\n",
  this,
  platformPrint->print.embedPrint.platformPrint,
  platformPrint->print.embedPrint.window.x,
  platformPrint->print.embedPrint.window.y,
  platformPrint->print.embedPrint.window.width,
  platformPrint->print.embedPrint.window.height,
  platformPrint->print.embedPrint.window.clipRect.top,
  platformPrint->print.embedPrint.window.clipRect.bottom,
  platformPrint->print.embedPrint.window.clipRect.left,
  platformPrint->print.embedPrint.window.clipRect.right));

  return NS_OK;
}

NS_IMETHODIMP nsNPAPIPluginInstance::HandleEvent(void* event, PRBool* handled)
{
  if (!mStarted)
    return NS_OK;

  if (!event)
    return NS_ERROR_FAILURE;

  PluginDestructionGuard guard(this);

  PRInt16 result = 0;
  
  if (mCallbacks->event) {
    mCurrentPluginEvent = event;
#if defined(XP_WIN) || defined(XP_OS2)
    NS_TRY_SAFE_CALL_RETURN(result, (*mCallbacks->event)(&mNPP, event), mLibrary, this);
#else
    result = (*mCallbacks->event)(&mNPP, event);
#endif
    NPP_PLUGIN_LOG(PLUGIN_LOG_NOISY,
      ("NPP HandleEvent called: this=%p, npp=%p, event=%p, return=%d\n", 
      this, &mNPP, event, result));

    *handled = result;
    mCurrentPluginEvent = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP nsNPAPIPluginInstance::GetValueFromPlugin(NPPVariable variable, void* value)
{
#ifdef MOZ_PLATFORM_HILDON
  
  
  if (variable == NPPVpluginWindowlessLocalBool) {
    *(NPBool*)value = mWindowlessLocal;
    return NS_OK;
  }
#endif
  nsresult  res = NS_OK;
  if (mCallbacks->getvalue && mStarted) {
    PluginDestructionGuard guard(this);

    NS_TRY_SAFE_CALL_RETURN(res, (*mCallbacks->getvalue)(&mNPP, variable, value), mLibrary, this);
    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
    ("NPP GetValue called: this=%p, npp=%p, var=%d, value=%d, return=%d\n", 
    this, &mNPP, variable, value, res));
  }

  return res;
}

nsresult nsNPAPIPluginInstance::GetNPP(NPP* aNPP) 
{
  if (aNPP)
    *aNPP = &mNPP;
  else
    return NS_ERROR_NULL_POINTER;

  return NS_OK;
}

nsresult nsNPAPIPluginInstance::GetCallbacks(const NPPluginFuncs ** aCallbacks)
{
  if (aCallbacks)
    *aCallbacks = mCallbacks;
  else
    return NS_ERROR_NULL_POINTER;

  return NS_OK;
}

NPError nsNPAPIPluginInstance::SetWindowless(PRBool aWindowless)
{
  mWindowless = aWindowless;
  return NPERR_NO_ERROR;
}

NPError nsNPAPIPluginInstance::SetWindowlessLocal(PRBool aWindowlessLocal)
{
  mWindowlessLocal = aWindowlessLocal;
  return NPERR_NO_ERROR;
}

NPError nsNPAPIPluginInstance::SetTransparent(PRBool aTransparent)
{
  mTransparent = aTransparent;
  return NPERR_NO_ERROR;
}

NPError nsNPAPIPluginInstance::SetWantsAllNetworkStreams(PRBool aWantsAllNetworkStreams)
{
  mWantsAllNetworkStreams = aWantsAllNetworkStreams;
  return NPERR_NO_ERROR;
}

#ifdef XP_MACOSX
void nsNPAPIPluginInstance::SetDrawingModel(NPDrawingModel aModel)
{
  mDrawingModel = aModel;
}

void nsNPAPIPluginInstance::SetEventModel(NPEventModel aModel)
{
  
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  GetOwner(getter_AddRefs(owner));
  if (!owner) {
    NS_WARNING("Trying to set event model without a plugin instance owner!");
    return;
  }

  owner->SetEventModel(aModel);
}

#endif

NS_IMETHODIMP nsNPAPIPluginInstance::GetDrawingModel(PRInt32* aModel)
{
#ifdef XP_MACOSX
  *aModel = (PRInt32)mDrawingModel;
  return NS_OK;
#else
  return NS_ERROR_FAILURE;
#endif
}

NS_IMETHODIMP
nsNPAPIPluginInstance::GetJSObject(JSContext *cx, JSObject** outObject)
{
  NPObject *npobj = nsnull;
  nsresult rv = GetValueFromPlugin(NPPVpluginScriptableNPObject, &npobj);
  if (NS_FAILED(rv) || !npobj)
    return NS_ERROR_FAILURE;

  *outObject = nsNPObjWrapper::GetNewOrUsed(&mNPP, cx, npobj);

  _releaseobject(npobj);

  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::DefineJavaProperties()
{
  NPObject *plugin_obj = nsnull;

  
  
  

  
  nsresult rv = GetValueFromPlugin(NPPVpluginScriptableNPObject, &plugin_obj);

  if (NS_FAILED(rv) || !plugin_obj) {
    return NS_ERROR_FAILURE;
  }

  
  NPObject *window_obj = _getwindowobject(&mNPP);

  if (!window_obj) {
    _releaseobject(plugin_obj);

    return NS_ERROR_FAILURE;
  }

  NPIdentifier java_id = _getstringidentifier("java");
  NPIdentifier packages_id = _getstringidentifier("Packages");

  NPObject *java_obj = nsnull;
  NPVariant v;
  OBJECT_TO_NPVARIANT(plugin_obj, v);

  

  bool ok = _setproperty(&mNPP, window_obj, packages_id, &v);
  if (ok) {
    ok = _getproperty(&mNPP, plugin_obj, java_id, &v);

    if (ok && NPVARIANT_IS_OBJECT(v)) {
      
      
      java_obj = NPVARIANT_TO_OBJECT(v);

      ok = _setproperty(&mNPP, window_obj, java_id, &v);
    }
  }

  _releaseobject(window_obj);
  _releaseobject(plugin_obj);
  _releaseobject(java_obj);

  if (!ok)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult
nsNPAPIPluginInstance::SetCached(PRBool aCache)
{
  mCached = aCache;
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::ShouldCache(PRBool* shouldCache)
{
  *shouldCache = mCached;
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::IsWindowless(PRBool* isWindowless)
{
  *isWindowless = mWindowless;
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::IsTransparent(PRBool* isTransparent)
{
  *isTransparent = mTransparent;
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::GetFormValue(nsAString& aValue)
{
  aValue.Truncate();

  char *value = nsnull;
  nsresult rv = GetValueFromPlugin(NPPVformValue, &value);
  if (NS_FAILED(rv) || !value)
    return NS_ERROR_FAILURE;

  CopyUTF8toUTF16(value, aValue);

  
  
  nsMemory::Free(value);

  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::PushPopupsEnabledState(PRBool aEnabled)
{
  nsCOMPtr<nsPIDOMWindow> window = GetDOMWindow();
  if (!window)
    return NS_ERROR_FAILURE;

  PopupControlState oldState =
    window->PushPopupControlState(aEnabled ? openAllowed : openAbused,
                                  PR_TRUE);

  if (!mPopupStates.AppendElement(oldState)) {
    
    window->PopPopupControlState(oldState);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::PopPopupsEnabledState()
{
  PRInt32 last = mPopupStates.Length() - 1;

  if (last < 0) {
    
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> window = GetDOMWindow();
  if (!window)
    return NS_ERROR_FAILURE;

  PopupControlState &oldState = mPopupStates[last];

  window->PopPopupControlState(oldState);

  mPopupStates.RemoveElementAt(last);
  
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::GetPluginAPIVersion(PRUint16* version)
{
  NS_ENSURE_ARG_POINTER(version);
  *version = mCallbacks->version;
  return NS_OK;
}

nsresult
nsNPAPIPluginInstance::PrivateModeStateChanged()
{
  if (!mStarted)
    return NS_OK;
  
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsNPAPIPluginInstance informing plugin of private mode state change this=%p\n",this));
  
  if (mCallbacks->setvalue) {
    PluginDestructionGuard guard(this);
    
    nsCOMPtr<nsIPrivateBrowsingService> pbs = do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
    if (pbs) {
      PRBool pme = PR_FALSE;
      nsresult rv = pbs->GetPrivateBrowsingEnabled(&pme);
      if (NS_FAILED(rv))
        return rv;

      NPError error;
      NS_TRY_SAFE_CALL_RETURN(error, (*mCallbacks->setvalue)(&mNPP, NPNVprivateModeBool, &pme), mLibrary, this);
      return (error == NPERR_NO_ERROR) ? NS_OK : NS_ERROR_FAILURE;
    }
  }
  return NS_ERROR_FAILURE;
}

static void
PluginTimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsNPAPITimer* t = (nsNPAPITimer*)aClosure;
  NPP npp = t->npp;
  uint32_t id = t->id;

  (*(t->callback))(npp, id);

  
  
  nsNPAPIPluginInstance *inst = (nsNPAPIPluginInstance*)npp->ndata;
  if (!inst || !inst->TimerWithID(id, NULL))
    return;

  
  PRUint32 timerType;
  t->timer->GetType(&timerType);
  if (timerType == nsITimer::TYPE_ONE_SHOT)
      inst->UnscheduleTimer(id);
}

nsNPAPITimer*
nsNPAPIPluginInstance::TimerWithID(uint32_t id, PRUint32* index)
{
  PRUint32 len = mTimers.Length();
  for (PRUint32 i = 0; i < len; i++) {
    if (mTimers[i]->id == id) {
      if (index)
        *index = i;
      return mTimers[i];
    }
  }
  return nsnull;
}

uint32_t
nsNPAPIPluginInstance::ScheduleTimer(uint32_t interval, NPBool repeat, void (*timerFunc)(NPP npp, uint32_t timerID))
{
  nsNPAPITimer *newTimer = new nsNPAPITimer();

  newTimer->npp = &mNPP;

  
  uint32_t uniqueID = mTimers.Length();
  while ((uniqueID == 0) || TimerWithID(uniqueID, NULL))
    uniqueID++;
  newTimer->id = uniqueID;

  
  nsresult rv;
  nsCOMPtr<nsITimer> xpcomTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return 0;
  const short timerType = (repeat ? (short)nsITimer::TYPE_REPEATING_SLACK : (short)nsITimer::TYPE_ONE_SHOT);
  xpcomTimer->InitWithFuncCallback(PluginTimerCallback, newTimer, interval, timerType);
  newTimer->timer = xpcomTimer;

  
  newTimer->callback = timerFunc;

  
  mTimers.AppendElement(newTimer);

  return newTimer->id;
}

void
nsNPAPIPluginInstance::UnscheduleTimer(uint32_t timerID)
{
  
  PRUint32 index;
  nsNPAPITimer* t = TimerWithID(timerID, &index);
  if (!t)
    return;

  
  t->timer->Cancel();

  
  mTimers.RemoveElementAt(index);

  
  delete t;
}



NPError
nsNPAPIPluginInstance::PopUpContextMenu(NPMenu* menu)
{
  if (mOwner && mCurrentPluginEvent)
    return mOwner->ShowNativeContextMenu(menu, mCurrentPluginEvent);

  return NPERR_GENERIC_ERROR;
}

NPBool
nsNPAPIPluginInstance::ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                                    double *destX, double *destY, NPCoordinateSpace destSpace)
{
  if (mOwner)
    return mOwner->ConvertPoint(sourceX, sourceY, sourceSpace, destX, destY, destSpace);

  return PR_FALSE;
}

nsresult
nsNPAPIPluginInstance::GetDOMElement(nsIDOMElement* *result)
{
  if (!mOwner) {
    *result = nsnull;
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIPluginTagInfo> tinfo(do_QueryInterface(mOwner));
  if (tinfo)
    return tinfo->GetDOMElement(result);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::InvalidateRect(NPRect *invalidRect)
{
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  GetOwner(getter_AddRefs(owner));
  if (!owner)
    return NS_ERROR_FAILURE;

  return owner->InvalidateRect(invalidRect);
}

NS_IMETHODIMP
nsNPAPIPluginInstance::InvalidateRegion(NPRegion invalidRegion)
{
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  GetOwner(getter_AddRefs(owner));
  if (!owner)
    return NS_ERROR_FAILURE;

  return owner->InvalidateRegion(invalidRegion);
}

NS_IMETHODIMP
nsNPAPIPluginInstance::ForceRedraw()
{
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  GetOwner(getter_AddRefs(owner));
  if (!owner)
    return NS_ERROR_FAILURE;

  return owner->ForceRedraw();
}

NS_IMETHODIMP
nsNPAPIPluginInstance::GetMIMEType(const char* *result)
{
  if (!mMIMEType)
    *result = "";
  else
    *result = mMIMEType;

  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::GetJSContext(JSContext* *outContext)
{
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  GetOwner(getter_AddRefs(owner));
  if (!owner)
    return NS_ERROR_FAILURE;

  *outContext = NULL;
  nsCOMPtr<nsIDocument> document;

  nsresult rv = owner->GetDocument(getter_AddRefs(document));

  if (NS_SUCCEEDED(rv) && document) {
    nsIScriptGlobalObject *global = document->GetScriptGlobalObject();

    if (global) {
      nsIScriptContext *context = global->GetContext();

      if (context) {
        *outContext = (JSContext*) context->GetNativeContext();
      }
    }
  }

  return rv;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::GetOwner(nsIPluginInstanceOwner **aOwner)
{
  NS_ENSURE_ARG_POINTER(aOwner);
  *aOwner = mOwner;
  NS_IF_ADDREF(mOwner);
  return (mOwner ? NS_OK : NS_ERROR_FAILURE);
}

NS_IMETHODIMP
nsNPAPIPluginInstance::SetOwner(nsIPluginInstanceOwner *aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::ShowStatus(const char* message)
{
  if (mOwner)
    return mOwner->ShowStatus(message);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNPAPIPluginInstance::InvalidateOwner()
{
  mOwner = nsnull;

  return NS_OK;
}
