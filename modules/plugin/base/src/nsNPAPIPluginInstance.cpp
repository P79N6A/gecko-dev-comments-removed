






































#include "prlog.h"
#include "prmem.h"
#include "nscore.h"
#include "prenv.h"

#include "nsNPAPIPluginInstance.h"
#include "nsNPAPIPlugin.h"
#include "nsNPAPIPluginStreamListener.h"
#include "nsPluginHostImpl.h"
#include "nsPluginSafety.h"
#include "nsPluginLogging.h"
#include "nsIPrivateBrowsingService.h"

#include "nsPIPluginInstancePeer.h"
#include "nsIDocument.h"

#include "nsJSNPRuntime.h"

static NS_DEFINE_IID(kCPluginManagerCID, NS_PLUGINMANAGER_CID); 
static NS_DEFINE_IID(kIPluginStreamListenerIID, NS_IPLUGINSTREAMLISTENER_IID);



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
    mStreamType(nsPluginStreamType_Normal),
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
    lib = mInst->fLibrary;
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

    NS_TRY_SAFE_CALL_VOID((*callbacks->urlnotify)(npp, mNotifyURL, reason, mNotifyData), mInst->fLibrary, mInst);

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
  nsMIMEType contentType;
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

  NS_TRY_SAFE_CALL_RETURN(error, (*callbacks->newstream)(npp, (char*)contentType, &mNPStream, seekable, &streamType), mInst->fLibrary, mInst);

  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("NPP NewStream called: this=%p, npp=%p, mime=%s, seek=%d, type=%d, return=%d, url=%s\n",
  this, npp, (char *)contentType, seekable, streamType, error, mNPStream.url));

  if (error != NPERR_NO_ERROR)
    return NS_ERROR_FAILURE;

  switch(streamType)
  {
    case NP_NORMAL:
      mStreamType = nsPluginStreamType_Normal; 
      break;
    case NP_ASFILEONLY:
      mStreamType = nsPluginStreamType_AsFileOnly; 
      break;
    case NP_ASFILE:
      mStreamType = nsPluginStreamType_AsFile; 
      break;
    case NP_SEEK:
      mStreamType = nsPluginStreamType_Seek; 
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

    mStreamBufferSize = PR_MAX(length, contentLength);

    
    
    
    mStreamBufferSize = PR_MIN(mStreamBufferSize, MAX_PLUGIN_NECKO_BUFFER);

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
        PR_MIN(length, mStreamBufferSize - mStreamBufferByteCount);

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
        NS_TRY_SAFE_CALL_RETURN(numtowrite, (*callbacks->writeready)(npp, &mNPStream), mInst->fLibrary, mInst);
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

        numtowrite = PR_MIN(numtowrite, mStreamBufferByteCount);
      } else {
        
        
        numtowrite = mStreamBufferByteCount;
      }

      PRInt32 writeCount = 0; 
      NS_TRY_SAFE_CALL_RETURN(writeCount, (*callbacks->write)(npp, &mNPStream, streamPosition, numtowrite, ptrStreamBuffer), mInst->fLibrary, mInst);

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

        writeCount = PR_MIN(writeCount, mStreamBufferByteCount);
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
  lib = mInst->fLibrary;

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
  if (mStreamType != nsPluginStreamType_Seek) {
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
nsNPAPIPluginStreamListener::GetStreamType(nsPluginStreamType *result)
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

NS_IMPL_ISUPPORTS2(nsNPAPIPluginInstance, nsIPluginInstance,
                   nsIPluginInstanceInternal)

nsNPAPIPluginInstance::nsNPAPIPluginInstance(NPPluginFuncs* callbacks,
                                       PRLibrary* aLibrary)
  : fCallbacks(callbacks),
#ifdef XP_MACOSX
#ifdef NP_NO_QUICKDRAW
    mDrawingModel(NPDrawingModelCoreGraphics),
#else
    mDrawingModel(NPDrawingModelQuickDraw),
#endif
#endif
    mWindowless(PR_FALSE),
    mTransparent(PR_FALSE),
    mStarted(PR_FALSE),
    mCached(PR_FALSE),
    mIsJavaPlugin(PR_FALSE),
    mWantsAllNetworkStreams(PR_FALSE),
    mInPluginInitCall(PR_FALSE),
    fLibrary(aLibrary),
    mStreams(nsnull)
{
  NS_ASSERTION(fCallbacks != NULL, "null callbacks");

  

  fNPP.pdata = NULL;
  fNPP.ndata = this;

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
}

PRBool
nsNPAPIPluginInstance::IsStarted(void)
{
  return mStarted;
}

NS_IMETHODIMP nsNPAPIPluginInstance::Initialize(nsIPluginInstancePeer* peer)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsNPAPIPluginInstance::Initialize this=%p\n",this));

  return InitializePlugin(peer);
}

NS_IMETHODIMP nsNPAPIPluginInstance::GetPeer(nsIPluginInstancePeer* *resultingPeer)
{
  *resultingPeer = mPeer;
  NS_IF_ADDREF(*resultingPeer);
  
  return NS_OK;
}

NS_IMETHODIMP nsNPAPIPluginInstance::Start(void)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsNPAPIPluginInstance::Start this=%p\n",this));

  if (mStarted)
    return NS_OK;

  return InitializePlugin(mPeer); 
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

  
  
  if (PluginDestructionGuard::DelayDestroy(this)) {
    return NS_OK;
  }

  
  
  EnterAsyncPluginThreadCallLock();
  mStarted = PR_FALSE;
  ExitAsyncPluginThreadCallLock();

  OnPluginDestroy(&fNPP);

  if (fCallbacks->destroy == NULL) {
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

  NS_TRY_SAFE_CALL_RETURN(error, (*fCallbacks->destroy)(&fNPP, &sdata), fLibrary, this);

  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("NPP Destroy called: this=%p, npp=%p, return=%d\n", this, &fNPP, error));

  nsJSNPRuntime::OnPluginDestroy(&fNPP);

  if (error != NPERR_NO_ERROR)
    return NS_ERROR_FAILURE;
  else
    return NS_OK;
}

already_AddRefed<nsPIDOMWindow>
nsNPAPIPluginInstance::GetDOMWindow()
{
  nsCOMPtr<nsPIPluginInstancePeer> pp (do_QueryInterface(mPeer));
  if (!pp)
    return nsnull;

  nsCOMPtr<nsIPluginInstanceOwner> owner;
  pp->GetOwner(getter_AddRefs(owner));
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

nsresult nsNPAPIPluginInstance::InitializePlugin(nsIPluginInstancePeer* peer)
{
  NS_ENSURE_ARG_POINTER(peer);
 
  nsCOMPtr<nsIPluginTagInfo2> taginfo = do_QueryInterface(peer);
  NS_ENSURE_TRUE(taginfo, NS_ERROR_NO_INTERFACE);
  
  PluginDestructionGuard guard(this);

  PRUint16 count = 0;
  const char* const* names = nsnull;
  const char* const* values = nsnull;
  nsPluginTagType tagtype;
  nsresult rv = taginfo->GetTagType(&tagtype);
  if (NS_SUCCEEDED(rv)) {
    
    rv = taginfo->GetAttributes(count, names, values);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    
    
    
    
    if (tagtype != nsPluginTagType_Embed) {
      PRUint16 pcount = 0;
      const char* const* pnames = nsnull;
      const char* const* pvalues = nsnull;    
      if (NS_SUCCEEDED(taginfo->GetParameters(pcount, pnames, pvalues))) {
        NS_ASSERTION(!values[count], "attribute/parameter array not setup correctly for NPAPI plugins");
        if (pcount)
          count += ++pcount; 
                             
      }
    }
  }

  NS_ENSURE_TRUE(fCallbacks->newp, NS_ERROR_FAILURE);
  
  
  
  
  nsPluginMode  mode;
  nsMIMEType    mimetype;
  NPError       error;

  peer->GetMode(&mode);
  peer->GetMIMEType(&mimetype);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

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

  mIsJavaPlugin = nsPluginHostImpl::IsJavaMIMEType(mimetype);

  
  
  
  
  mPeer = peer;
  mStarted = PR_TRUE;

  PRBool oldVal = mInPluginInitCall;
  mInPluginInitCall = PR_TRUE;

  NS_TRY_SAFE_CALL_RETURN(error, (*fCallbacks->newp)((char*)mimetype, &fNPP, (PRUint16)mode, count, (char**)names, (char**)values, NULL), fLibrary,this);

  mInPluginInitCall = oldVal;

  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("NPP New called: this=%p, npp=%p, mime=%s, mode=%d, argc=%d, return=%d\n",
  this, &fNPP, mimetype, mode, count, error));

  if (error != NPERR_NO_ERROR) {
    
    mPeer = nsnull;
    mStarted = PR_FALSE;

    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsNPAPIPluginInstance::Destroy(void)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsNPAPIPluginInstance::Destroy this=%p\n",this));

  
  return NS_OK;
}

NS_IMETHODIMP nsNPAPIPluginInstance::SetWindow(nsPluginWindow* window)
{
  
  if (!window || !mStarted)
    return NS_OK;

  NPError error;

#if defined (MOZ_WIDGET_GTK2)
  
  
  if (!mIsJavaPlugin && window->type == nsPluginWindowType_Window &&
      (window->width <= 0 || window->height <= 0)) {
    return NS_OK;
  }
#endif 

  if (fCallbacks->setwindow) {
    PluginDestructionGuard guard(this);

    
    

    PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsNPAPIPluginInstance::SetWindow (about to call it) this=%p\n",this));

    PRBool oldVal = mInPluginInitCall;
    mInPluginInitCall = PR_TRUE;

    NS_TRY_SAFE_CALL_RETURN(error, (*fCallbacks->setwindow)(&fNPP, (NPWindow*)window), fLibrary, this);

    mInPluginInitCall = oldVal;

    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
    ("NPP SetWindow called: this=%p, [x=%d,y=%d,w=%d,h=%d], clip[t=%d,b=%d,l=%d,r=%d], return=%d\n",
    this, window->x, window->y, window->width, window->height,
    window->clipRect.top, window->clipRect.bottom, window->clipRect.left, window->clipRect.right, error));
      
    
    
    
  }
  return NS_OK;
}



NS_IMETHODIMP nsNPAPIPluginInstance::NewStream(nsIPluginStreamListener** listener)
{
  return NewNotifyStream(listener, nsnull, PR_FALSE, nsnull);
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

NS_IMETHODIMP nsNPAPIPluginInstance::Print(nsPluginPrint* platformPrint)
{
  NS_ENSURE_TRUE(platformPrint, NS_ERROR_NULL_POINTER);

  PluginDestructionGuard guard(this);

  NPPrint* thePrint = (NPPrint *)platformPrint;

  
  
  
  if (fCallbacks) {
    PRUint16 sdkmajorversion = (fCallbacks->version & 0xff00)>>8;
    PRUint16 sdkminorversion = fCallbacks->version & 0x00ff;
    if ((sdkmajorversion == 0) && (sdkminorversion < 11)) { 
      
      
      
      if (sizeof(NPWindowType) >= sizeof(void *)) {
        void* source = thePrint->print.embedPrint.platformPrint; 
        void** destination = (void **)&(thePrint->print.embedPrint.window.type); 
        *destination = source;
      } 
      else 
        NS_ASSERTION(PR_FALSE, "Incompatible OS for assignment");
    }
  }

  if (fCallbacks->print)
      NS_TRY_SAFE_CALL_VOID((*fCallbacks->print)(&fNPP, thePrint), fLibrary, this);

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

NS_IMETHODIMP nsNPAPIPluginInstance::HandleEvent(nsPluginEvent* event, PRBool* handled)
{
  if (!mStarted)
    return NS_OK;

  if (!event)
    return NS_ERROR_FAILURE;

  PluginDestructionGuard guard(this);

  PRInt16 result = 0;
  
  if (fCallbacks->event) {
#ifdef XP_MACOSX
    result = (*fCallbacks->event)(&fNPP, (void*)event->event);

#elif defined(XP_WIN) || defined(XP_OS2)
      NPEvent npEvent;
      npEvent.event = event->event;
      npEvent.wParam = event->wParam;
      npEvent.lParam = event->lParam;

      NS_TRY_SAFE_CALL_RETURN(result, (*fCallbacks->event)(&fNPP, (void*)&npEvent), fLibrary, this);

#else 
      result = (*fCallbacks->event)(&fNPP, (void*)&event->event);
#endif

      NPP_PLUGIN_LOG(PLUGIN_LOG_NOISY,
      ("NPP HandleEvent called: this=%p, npp=%p, event=%d, return=%d\n", 
      this, &fNPP, event->event, result));

      *handled = result;
    }

  return NS_OK;
}

nsresult nsNPAPIPluginInstance::GetValueInternal(NPPVariable variable, void* value)
{
  nsresult  res = NS_OK;
  if (fCallbacks->getvalue && mStarted) {
    PluginDestructionGuard guard(this);

    NS_TRY_SAFE_CALL_RETURN(res, (*fCallbacks->getvalue)(&fNPP, variable, value), fLibrary, this);
    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
    ("NPP GetValue called: this=%p, npp=%p, var=%d, value=%d, return=%d\n", 
    this, &fNPP, variable, value, res));
  }

  return res;
}

NS_IMETHODIMP nsNPAPIPluginInstance::GetValue(nsPluginInstanceVariable variable, void *value)
{
  nsresult  res = NS_OK;

  switch (variable) {
    case nsPluginInstanceVariable_WindowlessBool:
      *(PRBool *)value = mWindowless;
      break;

    case nsPluginInstanceVariable_TransparentBool:
      *(PRBool *)value = mTransparent;
      break;

    case nsPluginInstanceVariable_DoCacheBool:
      *(PRBool *)value = mCached;
      break;

    case nsPluginInstanceVariable_CallSetWindowAfterDestroyBool:
      *(PRBool *)value = 0;  
      break;

#ifdef XP_MACOSX
    case nsPluginInstanceVariable_DrawingModel:
      *(NPDrawingModel*)value = mDrawingModel;
      break;
#endif

    default:
      res = GetValueInternal((NPPVariable)variable, value);
  }

  return res;
}

nsresult nsNPAPIPluginInstance::GetNPP(NPP* aNPP) 
{
  if (aNPP)
    *aNPP = &fNPP;
  else
    return NS_ERROR_NULL_POINTER;

  return NS_OK;
}

nsresult nsNPAPIPluginInstance::GetCallbacks(const NPPluginFuncs ** aCallbacks)
{
  if (aCallbacks)
    *aCallbacks = fCallbacks;
  else
    return NS_ERROR_NULL_POINTER;

  return NS_OK;
}

NPError nsNPAPIPluginInstance::SetWindowless(PRBool aWindowless)
{
  mWindowless = aWindowless;
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

NPDrawingModel nsNPAPIPluginInstance::GetDrawingModel()
{
  return mDrawingModel;
}
#endif

JSObject *
nsNPAPIPluginInstance::GetJSObject(JSContext *cx)
{
  JSObject *obj = nsnull;
  NPObject *npobj = nsnull;

  nsresult rv = GetValueInternal(NPPVpluginScriptableNPObject, &npobj);

  if (NS_SUCCEEDED(rv) && npobj) {
    obj = nsNPObjWrapper::GetNewOrUsed(&fNPP, cx, npobj);

    _releaseobject(npobj);
  }

  return obj;
}

void
nsNPAPIPluginInstance::DefineJavaProperties()
{
  NPObject *plugin_obj = nsnull;

  
  
  

  
  nsresult rv = GetValueInternal(NPPVpluginScriptableNPObject, &plugin_obj);

  if (NS_FAILED(rv) || !plugin_obj) {
    return;
  }

  
  NPObject *window_obj = _getwindowobject(&fNPP);

  if (!window_obj) {
    _releaseobject(plugin_obj);

    return;
  }

  NPIdentifier java_id = _getstringidentifier("java");
  NPIdentifier packages_id = _getstringidentifier("Packages");

  NPObject *java_obj = nsnull;
  NPVariant v;
  OBJECT_TO_NPVARIANT(plugin_obj, v);

  

  bool ok = _setproperty(&fNPP, window_obj, packages_id, &v);
  if (ok) {
    ok = _getproperty(&fNPP, plugin_obj, java_id, &v);

    if (ok && NPVARIANT_IS_OBJECT(v)) {
      
      
      java_obj = NPVARIANT_TO_OBJECT(v);

      ok = _setproperty(&fNPP, window_obj, java_id, &v);
    }
  }

  _releaseobject(window_obj);
  _releaseobject(plugin_obj);
  _releaseobject(java_obj);
}

nsresult
nsNPAPIPluginInstance::GetFormValue(nsAString& aValue)
{
  aValue.Truncate();

  char *value = nsnull;
  nsresult rv = GetValueInternal(NPPVformValue, &value);

  if (NS_SUCCEEDED(rv) && value) {
    CopyUTF8toUTF16(value, aValue);

    
    
    nsMemory::Free(value);
  }

  return NS_OK;
}

void
nsNPAPIPluginInstance::PushPopupsEnabledState(PRBool aEnabled)
{
  nsCOMPtr<nsPIDOMWindow> window = GetDOMWindow();
  if (!window)
    return;

  PopupControlState oldState =
    window->PushPopupControlState(aEnabled ? openAllowed : openAbused,
                                  PR_TRUE);

  if (!mPopupStates.AppendElement(oldState)) {
    
    window->PopPopupControlState(oldState);
  }
}

void
nsNPAPIPluginInstance::PopPopupsEnabledState()
{
  PRInt32 last = mPopupStates.Length() - 1;

  if (last < 0) {
    
    return;
  }

  nsCOMPtr<nsPIDOMWindow> window = GetDOMWindow();
  if (!window)
    return;

  PopupControlState &oldState = mPopupStates[last];

  window->PopPopupControlState(oldState);

  mPopupStates.RemoveElementAt(last);
}

PRUint16
nsNPAPIPluginInstance::GetPluginAPIVersion()
{
  return fCallbacks->version;
}

nsresult nsNPAPIPluginInstance::PrivateModeStateChanged()
{
  if (!mStarted)
    return NS_OK;
  
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsNPAPIPluginInstance informing plugin of private mode state change this=%p\n",this));
  
  if (fCallbacks->setvalue) {
    PluginDestructionGuard guard(this);
    
    nsCOMPtr<nsIPrivateBrowsingService> pbs = do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
    if (pbs) {
      PRBool pme = PR_FALSE;
      nsresult rv = pbs->GetPrivateBrowsingEnabled(&pme);
      if (NS_FAILED(rv))
        return rv;

      NPError error;
      NS_TRY_SAFE_CALL_RETURN(error, (*fCallbacks->setvalue)(&fNPP, NPNVprivateModeBool, &pme), fLibrary, this);
      return (error == NPERR_NO_ERROR) ? NS_OK : NS_ERROR_FAILURE;
    }
  }
  return NS_ERROR_FAILURE;
}
