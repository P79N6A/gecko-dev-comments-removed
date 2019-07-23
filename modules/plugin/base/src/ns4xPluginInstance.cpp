






































#include "prlog.h"
#include "prmem.h"
#include "nscore.h"
#include "prenv.h"

#include "ns4xPluginInstance.h"
#include "ns4xPlugin.h"
#include "ns4xPluginStreamListener.h"
#include "nsPluginHostImpl.h"
#include "nsPluginSafety.h"
#include "nsPluginLogging.h"

#include "nsPIPluginInstancePeer.h"
#include "nsPIDOMWindow.h"
#include "nsIDocument.h"

#include "nsJSNPRuntime.h"

#ifdef XP_OS2
#include "nsILegacyPluginWrapperOS2.h"
#endif

#ifdef MOZ_WIDGET_GTK
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include "gtkxtbin.h"
#endif

#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include "gtk2xtbin.h"
#endif

#ifdef MOZ_WIDGET_XLIB
#include "xlibxtbin.h"
#include "xlibrgb.h"
#endif




static NS_DEFINE_IID(kCPluginManagerCID, NS_PLUGINMANAGER_CID); 
static NS_DEFINE_IID(kIPluginStreamListenerIID, NS_IPLUGINSTREAMLISTENER_IID);




NS_IMPL_ISUPPORTS3(ns4xPluginStreamListener, nsIPluginStreamListener,
                   nsITimerCallback, nsIHTTPHeaderListener)



ns4xPluginStreamListener::ns4xPluginStreamListener(nsIPluginInstance* inst, 
                                                   void* notifyData,
                                                   const char* aURL)
  : mNotifyData(notifyData),
    mStreamBuffer(nsnull),
    mNotifyURL(aURL ? PL_strdup(aURL) : nsnull),
    mInst((ns4xPluginInstance *)inst),
    mStreamBufferSize(0),
    mStreamBufferByteCount(0),
    mStreamType(nsPluginStreamType_Normal),
    mStreamStarted(PR_FALSE),
    mStreamCleanedUp(PR_FALSE),
    mCallNotify(PR_FALSE),
    mIsSuspended(PR_FALSE),
    mResponseHeaderBuf(nsnull)
{
  
  memset(&mNPStream, 0, sizeof(mNPStream));

  NS_IF_ADDREF(mInst);
}



ns4xPluginStreamListener::~ns4xPluginStreamListener(void)
{
  
  ns4xPluginInstance *inst = mInst;
  if(inst) {
    nsInstanceStream * prev = nsnull;
    for(nsInstanceStream *is = inst->mStreams; is != nsnull; is = is->mNext) {
      if(is->mPluginStreamListener == this) {
        if(prev == nsnull)
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

  
  if (mStreamBuffer)
  {
    PR_Free(mStreamBuffer);
    mStreamBuffer=nsnull;
  }

  NS_IF_RELEASE(inst);

  if (mNotifyURL)
    PL_strfree(mNotifyURL);

  if (mResponseHeaderBuf)
    PL_strfree(mResponseHeaderBuf);
}


nsresult ns4xPluginStreamListener::CleanUpStream(NPReason reason)
{
  nsresult rv = NS_ERROR_FAILURE;

  if(mStreamCleanedUp)
    return NS_OK;

  if(!mInst || !mInst->IsStarted())
    return rv;

  const NPPluginFuncs *callbacks = nsnull;
  mInst->GetCallbacks(&callbacks);
  if(!callbacks)
    return rv;

  NPP npp;
  mInst->GetNPP(&npp);

  if (mStreamStarted && callbacks->destroystream != NULL)
  {
    PRLibrary* lib = nsnull;
    lib = mInst->fLibrary;
    NPError error;
    NS_TRY_SAFE_CALL_RETURN(error, CallNPP_DestroyStreamProc(callbacks->destroystream,
                                                               npp,
                                                               &mNPStream,
                                                               reason), lib, mInst);

    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
    ("NPP DestroyStream called: this=%p, npp=%p, reason=%d, return=%d, url=%s\n",
    this, npp, reason, error, mNPStream.url));

    if(error == NPERR_NO_ERROR)
      rv = NS_OK;
  }

  mStreamCleanedUp = PR_TRUE;
  mStreamStarted   = PR_FALSE;

  StopDataPump();

  
  CallURLNotify(reason);

  return rv;
}



void ns4xPluginStreamListener::CallURLNotify(NPReason reason)
{
  if(!mCallNotify || !mInst || !mInst->IsStarted())
    return;

  mCallNotify = PR_FALSE; 

  const NPPluginFuncs *callbacks = nsnull;
  mInst->GetCallbacks(&callbacks);
  if(!callbacks)
    return;
  
  if (callbacks->urlnotify) {

    NPP npp;
    mInst->GetNPP(&npp);

    NS_TRY_SAFE_CALL_VOID(CallNPP_URLNotifyProc(callbacks->urlnotify,
                                                npp,
                                                mNotifyURL,
                                                reason,
                                                mNotifyData), mInst->fLibrary, mInst);

    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
    ("NPP URLNotify called: this=%p, npp=%p, notify=%p, reason=%d, url=%s\n",
    this, npp, mNotifyData, reason, mNotifyURL));
  }
}



NS_IMETHODIMP
ns4xPluginStreamListener::OnStartBinding(nsIPluginStreamInfo* pluginInfo)
{
  if(!mInst)
    return NS_ERROR_FAILURE;

  NPP npp;
  const NPPluginFuncs *callbacks = nsnull;

  mInst->GetCallbacks(&callbacks);
  mInst->GetNPP(&npp);

  if(!callbacks || !mInst->IsStarted())
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

  NS_TRY_SAFE_CALL_RETURN(error, CallNPP_NewStreamProc(callbacks->newstream,
                                                       npp,
                                                       (char *)contentType,
                                                       &mNPStream,
                                                       seekable,
                                                       &streamType), mInst->fLibrary, mInst);

  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("NPP NewStream called: this=%p, npp=%p, mime=%s, seek=%d, type=%d, return=%d, url=%s\n",
  this, npp, (char *)contentType, seekable, streamType, error, mNPStream.url));

  if(error != NPERR_NO_ERROR)
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
ns4xPluginStreamListener::SuspendRequest()
{
  NS_ASSERTION(!mIsSuspended,
               "Suspending a request that's already suspended!");

  nsCOMPtr<nsI4xPluginStreamInfo> pluginInfo4x =
    do_QueryInterface(mStreamInfo);
  nsIRequest *request;

  if (!pluginInfo4x || !(request = pluginInfo4x->GetRequest())) {
    NS_ERROR("Trying to suspend a non-suspendable stream!");

    return NS_ERROR_FAILURE;
  }

  nsresult rv = StartDataPump();
  NS_ENSURE_SUCCESS(rv, rv);

  mIsSuspended = PR_TRUE;

  return request->Suspend();
}

void
ns4xPluginStreamListener::ResumeRequest()
{
  nsCOMPtr<nsI4xPluginStreamInfo> pluginInfo4x =
    do_QueryInterface(mStreamInfo);

  nsIRequest *request = pluginInfo4x->GetRequest();

  
  if (request) {
    request->Resume();
  }

  mIsSuspended = PR_FALSE;
}

nsresult
ns4xPluginStreamListener::StartDataPump()
{
  nsresult rv;
  mDataPumpTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  return mDataPumpTimer->InitWithCallback(this, 100,
                                          nsITimer::TYPE_REPEATING_SLACK);
}

void
ns4xPluginStreamListener::StopDataPump()
{
  if (mDataPumpTimer) {
    mDataPumpTimer->Cancel();

    mDataPumpTimer = nsnull;
  }
}










NS_IMETHODIMP
ns4xPluginStreamListener::OnDataAvailable(nsIPluginStreamInfo* pluginInfo,
                                          nsIInputStream* input,
                                          PRUint32 length)
{
  if (!mInst || !mInst->IsStarted())
    return NS_ERROR_FAILURE;

  
  mStreamInfo = pluginInfo;

  const NPPluginFuncs *callbacks = nsnull;
  mInst->GetCallbacks(&callbacks);
  
  if(!callbacks || !callbacks->write || !length)
    return NS_ERROR_FAILURE; 
  
  if (!mStreamBuffer)
  {
    
    
    
    
    

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
      if (mStreamBufferSize < mStreamBufferByteCount + length &&
          mIsSuspended) {
        
        
        
        
        
        
        mStreamBufferSize = mStreamBufferByteCount + length;
        char *buf = (char *)PR_Realloc(mStreamBuffer, mStreamBufferSize);
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
        NS_TRY_SAFE_CALL_RETURN(numtowrite, 
                                CallNPP_WriteReadyProc(callbacks->writeready,
                                                       npp, &mNPStream),
                                mInst->fLibrary, mInst);

        NPP_PLUGIN_LOG(PLUGIN_LOG_NOISY,
                       ("NPP WriteReady called: this=%p, npp=%p, "
                        "return(towrite)=%d, url=%s\n",
                        this, npp, numtowrite, mNPStream.url));

        if (!mStreamStarted) {
          
          

          return NS_BINDING_ABORTED;
        }

        
        
        
        if (numtowrite <= 0) {
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
      NS_TRY_SAFE_CALL_RETURN(writeCount, 
                              CallNPP_WriteProc(callbacks->write, npp,
                                                &mNPStream, streamPosition,
                                                numtowrite,
                                                ptrStreamBuffer),
                              mInst->fLibrary, mInst);

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
ns4xPluginStreamListener::OnFileAvailable(nsIPluginStreamInfo* pluginInfo, 
                                          const char* fileName)
{
  if(!mInst || !mInst->IsStarted())
    return NS_ERROR_FAILURE;

  const NPPluginFuncs *callbacks = nsnull;
  mInst->GetCallbacks(&callbacks);
  if(!callbacks || !callbacks->asfile)
    return NS_ERROR_FAILURE;
  
  NPP npp;
  mInst->GetNPP(&npp);

  PRLibrary* lib = nsnull;
  lib = mInst->fLibrary;

  NS_TRY_SAFE_CALL_VOID(CallNPP_StreamAsFileProc(callbacks->asfile,
                                                   npp,
                                                   &mNPStream,
                                                   fileName), lib, mInst);

  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("NPP StreamAsFile called: this=%p, npp=%p, url=%s, file=%s\n",
  this, npp, mNPStream.url, fileName));

  return NS_OK;
}



NS_IMETHODIMP
ns4xPluginStreamListener::OnStopBinding(nsIPluginStreamInfo* pluginInfo, 
                                        nsresult status)
{
  StopDataPump();

  if (NS_FAILED(status)) {
    
    
    nsCOMPtr<nsI4xPluginStreamInfo> pluginInfo4x =
      do_QueryInterface(mStreamInfo);

    nsIRequest *request;
    if (pluginInfo4x && (request = pluginInfo4x->GetRequest())) {
      request->Cancel(status);
    }
  }

  if(!mInst || !mInst->IsStarted())
    return NS_ERROR_FAILURE;

  
  
  nsresult rv = NS_OK;
  if(mStreamType != nsPluginStreamType_Seek) {
    NPReason reason = NPRES_DONE;

    if (NS_FAILED(status))
      reason = NPRES_NETWORK_ERR;   

    rv = CleanUpStream(reason);
  }

  if(rv != NPERR_NO_ERROR)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
ns4xPluginStreamListener::GetStreamType(nsPluginStreamType *result)
{
  *result = mStreamType;
  return NS_OK;
}

NS_IMETHODIMP
ns4xPluginStreamListener::Notify(nsITimer *aTimer)
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
ns4xPluginStreamListener::StatusLine(const char* line)
{
  mResponseHeaders.Append(line);
  mResponseHeaders.Append('\n');
  return NS_OK;
}


NS_IMETHODIMP
ns4xPluginStreamListener::NewResponseHeader(const char* headerName,
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




NS_IMPL_ISUPPORTS3(ns4xPluginInstance, nsIPluginInstance, nsIScriptablePlugin,
                   nsIPluginInstanceInternal)



ns4xPluginInstance::ns4xPluginInstance(NPPluginFuncs* callbacks,
                                       PRLibrary* aLibrary)
  : fCallbacks(callbacks)
{
  NS_ASSERTION(fCallbacks != NULL, "null callbacks");

  

  fNPP.pdata = NULL;
  fNPP.ndata = this;

  fLibrary = aLibrary;
  mWindowless = PR_FALSE;
  mTransparent = PR_FALSE;
  mStarted = PR_FALSE;
  mStreams = nsnull;
  mCached = PR_FALSE;

#ifdef XP_MACOSX
#ifdef NP_NO_QUICKDRAW
  mDrawingModel = NPDrawingModelCoreGraphics;
#else
  mDrawingModel = NPDrawingModelQuickDraw;
#endif
#endif

  PLUGIN_LOG(PLUGIN_LOG_BASIC, ("ns4xPluginInstance ctor: this=%p\n",this));
}



ns4xPluginInstance::~ns4xPluginInstance(void)
{
  PLUGIN_LOG(PLUGIN_LOG_BASIC, ("ns4xPluginInstance dtor: this=%p\n",this));

#if defined(MOZ_WIDGET_GTK) || defined (MOZ_WIDGET_GTK2)
  if (mXtBin)
    gtk_widget_destroy(mXtBin);
#elif defined(MOZ_WIDGET_XLIB)
  if (mXlibXtBin) {
    delete mXlibXtBin;
  }
#endif

  
  for(nsInstanceStream *is = mStreams; is != nsnull;) {
    nsInstanceStream * next = is->mNext;
    delete is;
    is = next;
  }
}



PRBool
ns4xPluginInstance::IsStarted(void)
{
  return mStarted;
}



NS_IMETHODIMP ns4xPluginInstance::Initialize(nsIPluginInstancePeer* peer)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("ns4xPluginInstance::Initialize this=%p\n",this));

#if defined (MOZ_WIDGET_GTK) || defined (MOZ_WIDGET_GTK2)
  mXtBin = nsnull;
#elif defined(MOZ_WIDGET_XLIB)
  mXlibXtBin = nsnull;
#endif
  return InitializePlugin(peer);
}


NS_IMETHODIMP ns4xPluginInstance::GetPeer(nsIPluginInstancePeer* *resultingPeer)
{
  *resultingPeer = mPeer;
  NS_IF_ADDREF(*resultingPeer);
  
  return NS_OK;
}


NS_IMETHODIMP ns4xPluginInstance::Start(void)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("ns4xPluginInstance::Start this=%p\n",this));

#ifdef MOZ_WIDGET_XLIB
  if (mXlibXtBin == nsnull)
    mXlibXtBin = new xtbin();

  if (mXlibXtBin == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mXlibXtBin->xtbin_initialized())
    mXlibXtBin->xtbin_init();

#ifdef NS_DEBUG
  printf("Made new XtBin: %p, %d\n", mXlibXtBin, mXlibXtBin->xtbin_initialized());
#endif
#endif

  if(mStarted)
    return NS_OK;
  else
    return InitializePlugin(mPeer); 
}



NS_IMETHODIMP ns4xPluginInstance::Stop(void)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("ns4xPluginInstance::Stop this=%p\n",this));

  NPError error;

  
  if (mPopupStates.Count() > 0) {
    nsCOMPtr<nsPIDOMWindow> window = GetDOMWindow();

    if (window) {
      window->PopPopupControlState(openAbused);
    }
  }

#if defined(MOZ_WIDGET_GTK) || defined (MOZ_WIDGET_GTK2)
  if (mXtBin) {
    gtk_widget_destroy(mXtBin);
    mXtBin = 0;
  }
#elif defined(MOZ_WIDGET_XLIB)
  if (mXlibXtBin) {
    mXlibXtBin->xtbin_destroy();
    mXlibXtBin = 0;
  }
#endif

  if(!mStarted)
    return NS_OK;

  if (fCallbacks->destroy == NULL)
    return NS_ERROR_FAILURE; 

  NPSavedData *sdata = 0;

  
  for(nsInstanceStream *is = mStreams; is != nsnull;) {
    ns4xPluginStreamListener * listener = is->mPluginStreamListener;

    nsInstanceStream *next = is->mNext;
    delete is;
    is = next;
    mStreams = is;

    
    
    if(listener)
      listener->CleanUpStream(NPRES_USER_BREAK);
  }

  NS_TRY_SAFE_CALL_RETURN(error, CallNPP_DestroyProc(fCallbacks->destroy, &fNPP, &sdata), fLibrary, this);

  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("NPP Destroy called: this=%p, npp=%p, return=%d\n", this, &fNPP, error));

  mStarted = PR_FALSE;

  nsJSNPRuntime::OnPluginDestroy(&fNPP);

  if(error != NPERR_NO_ERROR)
    return NS_ERROR_FAILURE;
  else
    return NS_OK;
}

already_AddRefed<nsPIDOMWindow>
ns4xPluginInstance::GetDOMWindow()
{
  nsCOMPtr<nsPIPluginInstancePeer> pp (do_QueryInterface(mPeer));
  if (!pp) {
    return nsnull;
  }

  nsCOMPtr<nsIPluginInstanceOwner> owner;
  pp->GetOwner(getter_AddRefs(owner));

  if (!owner) {
    return nsnull;
  }

  nsCOMPtr<nsIDocument> doc;
  owner->GetDocument(getter_AddRefs(doc));

  if (!doc) {
    return nsnull;
  }

  nsPIDOMWindow *window = doc->GetWindow();
  NS_IF_ADDREF(window);

  return window;
}


nsresult ns4xPluginInstance::InitializePlugin(nsIPluginInstancePeer* peer)
{
  NS_ENSURE_ARG_POINTER(peer);
 
  nsCOMPtr<nsIPluginTagInfo2> taginfo = do_QueryInterface(peer);
  NS_ENSURE_TRUE(taginfo, NS_ERROR_NO_INTERFACE);
  
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
        NS_ASSERTION(nsnull == values[count], "attribute/parameter array not setup correctly for 4.x plugins");
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

  
  
  
  
  mPeer = peer;
  mStarted = PR_TRUE;

  NS_TRY_SAFE_CALL_RETURN(error, CallNPP_NewProc(fCallbacks->newp,
                                          (char *)mimetype,
                                          &fNPP,
                                          (PRUint16)mode,
                                          count,
                                          (char**)names,
                                          (char**)values,
                                          NULL), fLibrary,this);

  NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("NPP New called: this=%p, npp=%p, mime=%s, mode=%d, argc=%d, return=%d\n",
  this, &fNPP, mimetype, mode, count, error));

  if(error != NPERR_NO_ERROR) {
    
    mPeer = nsnull;
    mStarted = PR_FALSE;

    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}



NS_IMETHODIMP ns4xPluginInstance::Destroy(void)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("ns4xPluginInstance::Destroy this=%p\n",this));

  
  return NS_OK;
}



NS_IMETHODIMP ns4xPluginInstance::SetWindow(nsPluginWindow* window)
{
#if defined(MOZ_WIDGET_GTK) || defined (MOZ_WIDGET_GTK2) || defined(MOZ_WIDGET_XLIB)
  NPSetWindowCallbackStruct *ws;
#endif

  
  if (!window || !mStarted)
    return NS_OK;
  
  NPError error;
  
#if defined (MOZ_WIDGET_GTK) || defined (MOZ_WIDGET_GTK2)
  PRBool isXembed = PR_FALSE;
  
  if ((PRInt32) window->width <= 0 || (PRInt32) window->height <= 0)
    return NS_OK;

  
  
  
  GdkWindow *win = gdk_window_lookup((XID)window->window);
  if (!win)
    return NS_ERROR_FAILURE;

  gpointer user_data = nsnull;
  gdk_window_get_user_data(win, &user_data);
  if (user_data && GTK_IS_WIDGET(user_data)) {
    GtkWidget* widget = GTK_WIDGET(user_data);

    if (GTK_IS_SOCKET(widget))
      isXembed = PR_TRUE;
  }

  
  if (!window->ws_info || !mXtBin) {
    if (!window->ws_info) {
#ifdef NS_DEBUG
      printf("About to create new ws_info...\n");
#endif    

      
      window->ws_info = (NPSetWindowCallbackStruct *)PR_MALLOC(sizeof(NPSetWindowCallbackStruct));

      if (!window->ws_info)
        return NS_ERROR_OUT_OF_MEMORY;
    }

    ws = (NPSetWindowCallbackStruct *)window->ws_info;

    if (!isXembed)
    {  
#ifdef NS_DEBUG      
      printf("About to create new xtbin of %i X %i from %p...\n",
             window->width, window->height, win);
#endif

#if 0
      
      
      
      
      
      
      
      
      
      

      if (mXtBin) {
        gtk_widget_destroy(mXtBin);
        mXtBin = NULL;
      }
#endif


      if (!mXtBin) {
        mXtBin = gtk_xtbin_new(win, 0);
        
        
        if (!mXtBin)
          return NS_ERROR_FAILURE;
      } 

      gtk_widget_set_usize(mXtBin, window->width, window->height);

#ifdef NS_DEBUG
      printf("About to show xtbin(%p)...\n", mXtBin); fflush(NULL);
#endif
      gtk_widget_show(mXtBin);
#ifdef NS_DEBUG
      printf("completed gtk_widget_show(%p)\n", mXtBin); fflush(NULL);
#endif
    }

    
    ws->type = 0; 
#ifdef MOZ_X11
    ws->depth = gdk_window_get_visual(win)->depth;
    if (!isXembed)
      ws->display = GTK_XTBIN(mXtBin)->xtdisplay;
    else
      ws->display = GDK_WINDOW_XDISPLAY(win);
    ws->visual = GDK_VISUAL_XVISUAL(gdk_window_get_visual(win));
    ws->colormap = GDK_COLORMAP_XCOLORMAP(gdk_window_get_colormap(win));

    XFlush(ws->display);
#endif
  } 

  if (!mXtBin && !isXembed)
    return NS_ERROR_FAILURE;

  if (!isXembed) {
    
    
    window->window = (nsPluginPort *)GTK_XTBIN(mXtBin)->xtwindow;
    
    gtk_xtbin_resize(mXtBin, window->width, window->height);
  }
  
#elif defined(MOZ_WIDGET_XLIB)


  
  if (!window->ws_info) {
#ifdef NS_DEBUG
    printf("About to create new ws_info...\n");
#endif

    
    window->ws_info = (NPSetWindowCallbackStruct *)PR_MALLOC(sizeof(NPSetWindowCallbackStruct));

    if (!window->ws_info)
      return NS_ERROR_OUT_OF_MEMORY;

    ws = (NPSetWindowCallbackStruct *)window->ws_info;

#if 1
     
     if (mXlibXtBin) {
       delete mXlibXtBin;
       mXlibXtBin = nsnull;
     }
#endif

      if (!mXlibXtBin) {
        mXlibXtBin = new xtbin();
        
        
        if (!mXlibXtBin)
          return NS_ERROR_FAILURE;
      } 
      
    if (window->window) {
#ifdef NS_DEBUG
      printf("About to create new xtbin of %i X %i from %08x...\n",
             window->width, window->height, window->window);
#endif

      mXlibXtBin->xtbin_new((Window)window->window);
      mXlibXtBin->xtbin_resize(0, 0, window->width, window->height);
#ifdef NS_DEBUG
      printf("About to show xtbin(%p)...\n", mXlibXtBin); fflush(NULL);
#endif
      mXlibXtBin->xtbin_realize();
    }
    
    
    XlibRgbHandle *xlibRgbHandle = xxlib_find_handle(XXLIBRGB_DEFAULT_HANDLE);
    Display *xdisplay = xxlib_rgb_get_display(xlibRgbHandle);

    
    ws->type     = 0;
    ws->depth    = xxlib_rgb_get_depth(xlibRgbHandle);
    ws->display  = xdisplay;
    ws->visual   = xxlib_rgb_get_visual(xlibRgbHandle);
    ws->colormap = xxlib_rgb_get_cmap(xlibRgbHandle);
    XFlush(ws->display);
  } 

  
  
  window->window = (nsPluginPort *)mXlibXtBin->xtbin_xtwindow();
#endif 

  if (fCallbacks->setwindow) {
    
    

    PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("ns4xPluginInstance::SetWindow (about to call it) this=%p\n",this));

    NS_TRY_SAFE_CALL_RETURN(error, CallNPP_SetWindowProc(fCallbacks->setwindow,
                                  &fNPP,
                                  (NPWindow*) window), fLibrary, this);


    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
    ("NPP SetWindow called: this=%p, [x=%d,y=%d,w=%d,h=%d], clip[t=%d,b=%d,l=%d,r=%d], return=%d\n",
    this, window->x, window->y, window->width, window->height,
    window->clipRect.top, window->clipRect.bottom, window->clipRect.left, window->clipRect.right, error));
      
    
    
    

  }
  return NS_OK;
}





NS_IMETHODIMP ns4xPluginInstance::NewStream(nsIPluginStreamListener** listener)
{
  return NewNotifyStream(listener, nsnull, PR_FALSE, nsnull);
}




nsresult ns4xPluginInstance::NewNotifyStream(nsIPluginStreamListener** listener, 
                                             void* notifyData,
                                             PRBool aCallNotify,
                                             const char* aURL)
{
  ns4xPluginStreamListener* stream = new ns4xPluginStreamListener(this, notifyData, aURL);
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

NS_IMETHODIMP ns4xPluginInstance::Print(nsPluginPrint* platformPrint)
{
  NS_ENSURE_TRUE(platformPrint, NS_ERROR_NULL_POINTER);

  NPPrint* thePrint = (NPPrint *)platformPrint;

  
  
  
  if(fCallbacks) {
    PRUint16 sdkmajorversion = (fCallbacks->version & 0xff00)>>8;
    PRUint16 sdkminorversion = fCallbacks->version & 0x00ff;
    if((sdkmajorversion == 0) && (sdkminorversion < 11)) { 
      
      
      
      if(sizeof(NPWindowType) >= sizeof(void *)) {
        void* source = thePrint->print.embedPrint.platformPrint; 
        void** destination = (void **)&(thePrint->print.embedPrint.window.type); 
        *destination = source;
      } 
      else 
        NS_ASSERTION(PR_FALSE, "Incompatible OS for assignment");
    }
  }

  if(fCallbacks->print) {
      NS_TRY_SAFE_CALL_VOID(CallNPP_PrintProc(fCallbacks->print,
                                              &fNPP,
                                              thePrint), fLibrary, this);
  }

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

NS_IMETHODIMP ns4xPluginInstance::HandleEvent(nsPluginEvent* event, PRBool* handled)
{
  if(!mStarted)
    return NS_OK;

  if (event == nsnull)
    return NS_ERROR_FAILURE;

  PRInt16 result = 0;
  
  if (fCallbacks->event) {
#ifdef XP_MACOSX
    result = CallNPP_HandleEventProc(fCallbacks->event,
                                     &fNPP,
                                     (void*) event->event);
#endif

#if defined(XP_WIN) || defined(XP_OS2)
      NPEvent npEvent;
      npEvent.event = event->event;
      npEvent.wParam = event->wParam;
      npEvent.lParam = event->lParam;

      NS_TRY_SAFE_CALL_RETURN(result, CallNPP_HandleEventProc(fCallbacks->event,
                                    &fNPP,
                                    (void*)&npEvent), fLibrary, this);
#endif

      NPP_PLUGIN_LOG(PLUGIN_LOG_NOISY,
      ("NPP HandleEvent called: this=%p, npp=%p, event=%d, return=%d\n", 
      this, &fNPP, event->event, result));

      *handled = result;
    }

  return NS_OK;
}

nsresult ns4xPluginInstance::GetValueInternal(NPPVariable variable, void* value)
{
  nsresult  res = NS_OK;
  if(fCallbacks->getvalue && mStarted) {

    NS_TRY_SAFE_CALL_RETURN(res, 
                            CallNPP_GetValueProc(fCallbacks->getvalue, 
                                                 &fNPP, 
                                                 variable, 
                                                 value), 
                                                 fLibrary, this);
    NPP_PLUGIN_LOG(PLUGIN_LOG_NORMAL,
    ("NPP GetValue called: this=%p, npp=%p, var=%d, value=%d, return=%d\n", 
    this, &fNPP, variable, value, res));

#ifdef XP_OS2
    
    if (res == NS_OK && variable == NPPVpluginScriptableInstance)
    {
      nsCOMPtr<nsILegacyPluginWrapperOS2> wrapper =
               do_GetService(NS_LEGACY_PLUGIN_WRAPPER_CONTRACTID, &res);
      if (res == NS_OK)
      {
        nsIID *iid = nsnull; 
        res = CallNPP_GetValueProc(fCallbacks->getvalue, &fNPP, 
                                   NPPVpluginScriptableIID, (void *)&iid);
        if (res == NS_OK)
          res = wrapper->MaybeWrap(*iid, *(nsISupports**)value,
                                   (nsISupports**)value);
      }
    }
#endif
  }

  return res;
}



NS_IMETHODIMP ns4xPluginInstance::GetValue(nsPluginInstanceVariable variable,
                                           void *value)
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



nsresult ns4xPluginInstance::GetNPP(NPP* aNPP) 
{
  if(aNPP != nsnull)
    *aNPP = &fNPP;
  else
    return NS_ERROR_NULL_POINTER;

  return NS_OK;
}



nsresult ns4xPluginInstance::GetCallbacks(const NPPluginFuncs ** aCallbacks)
{
  if(aCallbacks != nsnull)
    *aCallbacks = fCallbacks;
  else
    return NS_ERROR_NULL_POINTER;

  return NS_OK;
}



NPError ns4xPluginInstance::SetWindowless(PRBool aWindowless)
{
  mWindowless = aWindowless;
  return NPERR_NO_ERROR;
}



NPError ns4xPluginInstance::SetTransparent(PRBool aTransparent)
{
  mTransparent = aTransparent;
  return NPERR_NO_ERROR;
}

#ifdef XP_MACOSX

void ns4xPluginInstance::SetDrawingModel(NPDrawingModel aModel)
{
  mDrawingModel = aModel;
}



NPDrawingModel ns4xPluginInstance::GetDrawingModel()
{
  return mDrawingModel;
}
#endif



NS_IMETHODIMP ns4xPluginInstance::GetScriptablePeer(void * *aScriptablePeer)
{
  if (!aScriptablePeer)
    return NS_ERROR_NULL_POINTER;

  *aScriptablePeer = nsnull;
  return GetValueInternal(NPPVpluginScriptableInstance, aScriptablePeer);
}



NS_IMETHODIMP ns4xPluginInstance::GetScriptableInterface(nsIID * *aScriptableInterface)
{
  if (!aScriptableInterface)
    return NS_ERROR_NULL_POINTER;

  *aScriptableInterface = nsnull;
  return GetValueInternal(NPPVpluginScriptableIID, (void*)aScriptableInterface);
}

JSObject *
ns4xPluginInstance::GetJSObject(JSContext *cx)
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

nsresult
ns4xPluginInstance::GetFormValue(nsAString& aValue)
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
ns4xPluginInstance::PushPopupsEnabledState(PRBool aEnabled)
{
  nsCOMPtr<nsPIDOMWindow> window = GetDOMWindow();
  if (!window)
    return;

  PopupControlState oldState =
    window->PushPopupControlState(aEnabled ? openAllowed : openAbused,
                                  PR_TRUE);

  if (!mPopupStates.AppendElement(NS_INT32_TO_PTR(oldState))) {
    

    window->PopPopupControlState(oldState);
  }
}

void
ns4xPluginInstance::PopPopupsEnabledState()
{
  PRInt32 last = mPopupStates.Count() - 1;

  if (last < 0) {
    

    return;
  }

  nsCOMPtr<nsPIDOMWindow> window = GetDOMWindow();
  if (!window)
    return;

  PopupControlState oldState =
    (PopupControlState)NS_PTR_TO_INT32(mPopupStates[last]);

  window->PopPopupControlState(oldState);

  mPopupStates.RemoveElementAt(last);
}

PRUint16
ns4xPluginInstance::GetPluginAPIVersion()
{
  return fCallbacks->version;
}
