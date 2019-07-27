




#include "nsPluginStreamListenerPeer.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsIDOMElement.h"
#include "nsIStreamConverterService.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIFileChannel.h"
#include "nsMimeTypes.h"
#include "nsISupportsPrimitives.h"
#include "nsNetCID.h"
#include "nsPluginInstanceOwner.h"
#include "nsPluginLogging.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsPluginHost.h"
#include "nsIByteRangeRequest.h"
#include "nsIMultiPartChannel.h"
#include "nsIInputStreamTee.h"
#include "nsPrintfCString.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocument.h"
#include "nsIWebNavigation.h"
#include "nsContentUtils.h"
#include "nsNetUtil.h"
#include "nsPluginNativeWindow.h"
#include "GeckoProfiler.h"
#include "nsPluginInstanceOwner.h"
#include "nsDataHashtable.h"

#define MAGIC_REQUEST_CONTEXT 0x01020304



class nsPluginByteRangeStreamListener
  : public nsIStreamListener
  , public nsIInterfaceRequestor
{
public:
  explicit nsPluginByteRangeStreamListener(nsIWeakReference* aWeakPtr);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR

private:
  virtual ~nsPluginByteRangeStreamListener();

  nsCOMPtr<nsIStreamListener> mStreamConverter;
  nsWeakPtr mWeakPtrPluginStreamListenerPeer;
  bool mRemoveMagicNumber;
};

NS_IMPL_ISUPPORTS(nsPluginByteRangeStreamListener,
                  nsIRequestObserver,
                  nsIStreamListener,
                  nsIInterfaceRequestor)

nsPluginByteRangeStreamListener::nsPluginByteRangeStreamListener(nsIWeakReference* aWeakPtr)
{
  mWeakPtrPluginStreamListenerPeer = aWeakPtr;
  mRemoveMagicNumber = false;
}

nsPluginByteRangeStreamListener::~nsPluginByteRangeStreamListener()
{
  mStreamConverter = 0;
  mWeakPtrPluginStreamListenerPeer = 0;
}





static nsCOMPtr<nsIRequest>
GetBaseRequest(nsIRequest* r)
{
  nsCOMPtr<nsIMultiPartChannel> mp = do_QueryInterface(r);
  if (!mp)
    return r;

  nsCOMPtr<nsIChannel> base;
  mp->GetBaseChannel(getter_AddRefs(base));
  return already_AddRefed<nsIRequest>(base.forget());
}

NS_IMETHODIMP
nsPluginByteRangeStreamListener::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
  nsresult rv;

  nsCOMPtr<nsIStreamListener> finalStreamListener = do_QueryReferent(mWeakPtrPluginStreamListenerPeer);
  if (!finalStreamListener)
    return NS_ERROR_FAILURE;

  nsPluginStreamListenerPeer *pslp =
    static_cast<nsPluginStreamListenerPeer*>(finalStreamListener.get());

  NS_ASSERTION(pslp->mRequests.IndexOfObject(GetBaseRequest(request)) != -1,
               "Untracked byte-range request?");

  nsCOMPtr<nsIStreamConverterService> serv = do_GetService(NS_STREAMCONVERTERSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = serv->AsyncConvertData(MULTIPART_BYTERANGES,
                                "*/*",
                                finalStreamListener,
                                nullptr,
                                getter_AddRefs(mStreamConverter));
    if (NS_SUCCEEDED(rv)) {
      rv = mStreamConverter->OnStartRequest(request, ctxt);
      if (NS_SUCCEEDED(rv))
        return rv;
    }
  }
  mStreamConverter = 0;

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(request));
  if (!httpChannel) {
    return NS_ERROR_FAILURE;
  }

  uint32_t responseCode = 0;
  rv = httpChannel->GetResponseStatus(&responseCode);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  if (responseCode != 200) {
    uint32_t wantsAllNetworkStreams = 0;
    rv = pslp->GetPluginInstance()->GetValueFromPlugin(NPPVpluginWantsAllNetworkStreams,
                                                       &wantsAllNetworkStreams);
    
    if (NS_FAILED(rv)) {
      wantsAllNetworkStreams = 0;
    }

    if (!wantsAllNetworkStreams){
      return NS_ERROR_FAILURE;
    }
  }

  
  
  mStreamConverter = finalStreamListener;
  mRemoveMagicNumber = true;

  rv = pslp->ServeStreamAsFile(request, ctxt);
  return rv;
}

NS_IMETHODIMP
nsPluginByteRangeStreamListener::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                                               nsresult status)
{
  if (!mStreamConverter)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStreamListener> finalStreamListener = do_QueryReferent(mWeakPtrPluginStreamListenerPeer);
  if (!finalStreamListener)
    return NS_ERROR_FAILURE;

  nsPluginStreamListenerPeer *pslp =
    static_cast<nsPluginStreamListenerPeer*>(finalStreamListener.get());
  bool found = pslp->mRequests.RemoveObject(request);
  if (!found) {
    NS_ERROR("OnStopRequest received for untracked byte-range request!");
  }

  if (mRemoveMagicNumber) {
    
    nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(ctxt);
    if (container) {
      uint32_t magicNumber = 0;
      container->GetData(&magicNumber);
      if (magicNumber == MAGIC_REQUEST_CONTEXT) {
        
        
        container->SetData(0);
      }
    } else {
      NS_WARNING("Bad state of nsPluginByteRangeStreamListener");
    }
  }

  return mStreamConverter->OnStopRequest(request, ctxt, status);
}



CachedFileHolder::CachedFileHolder(nsIFile* cacheFile)
: mFile(cacheFile)
{
  NS_ASSERTION(mFile, "Empty CachedFileHolder");
}

CachedFileHolder::~CachedFileHolder()
{
  mFile->Remove(false);
}

void
CachedFileHolder::AddRef()
{
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "CachedFileHolder", sizeof(*this));
}

void
CachedFileHolder::Release()
{
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "CachedFileHolder");
  if (0 == mRefCnt)
    delete this;
}


NS_IMETHODIMP
nsPluginByteRangeStreamListener::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
                                                 nsIInputStream *inStr,
                                                 uint64_t sourceOffset,
                                                 uint32_t count)
{
  if (!mStreamConverter)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStreamListener> finalStreamListener = do_QueryReferent(mWeakPtrPluginStreamListenerPeer);
  if (!finalStreamListener)
    return NS_ERROR_FAILURE;

  return mStreamConverter->OnDataAvailable(request, ctxt, inStr, sourceOffset, count);
}

NS_IMETHODIMP
nsPluginByteRangeStreamListener::GetInterface(const nsIID& aIID, void** result)
{
  
  nsCOMPtr<nsIInterfaceRequestor> finalStreamListener = do_QueryReferent(mWeakPtrPluginStreamListenerPeer);
  if (!finalStreamListener)
    return NS_ERROR_FAILURE;

  return finalStreamListener->GetInterface(aIID, result);
}



NS_IMPL_ISUPPORTS(nsPluginStreamListenerPeer,
                  nsIStreamListener,
                  nsIRequestObserver,
                  nsIHttpHeaderVisitor,
                  nsISupportsWeakReference,
                  nsIInterfaceRequestor,
                  nsIChannelEventSink)

nsPluginStreamListenerPeer::nsPluginStreamListenerPeer()
{
  mStreamType = NP_NORMAL;
  mStartBinding = false;
  mAbort = false;
  mRequestFailed = false;

  mPendingRequests = 0;
  mHaveFiredOnStartRequest = false;
  mDataForwardToRequest = nullptr;

  mUseLocalCache = false;
  mSeekable = false;
  mModified = 0;
  mStreamOffset = 0;
  mStreamComplete = 0;
}

nsPluginStreamListenerPeer::~nsPluginStreamListenerPeer()
{
#ifdef PLUGIN_LOGGING
  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
         ("nsPluginStreamListenerPeer::dtor this=%p, url=%s\n",this, mURLSpec.get()));
#endif

  if (mPStreamListener) {
    mPStreamListener->SetStreamListenerPeer(nullptr);
  }

  
  
  if (mFileCacheOutputStream)
    mFileCacheOutputStream = nullptr;

  delete mDataForwardToRequest;

  if (mPluginInstance)
    mPluginInstance->FileCachedStreamListeners()->RemoveElement(this);
}



nsresult nsPluginStreamListenerPeer::Initialize(nsIURI *aURL,
                                                nsNPAPIPluginInstance *aInstance,
                                                nsNPAPIPluginStreamListener* aListener)
{
#ifdef PLUGIN_LOGGING
  nsAutoCString urlSpec;
  if (aURL != nullptr) aURL->GetSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
         ("nsPluginStreamListenerPeer::Initialize instance=%p, url=%s\n", aInstance, urlSpec.get()));

  PR_LogFlush();
#endif

  
  if (!aInstance) {
    return NS_ERROR_FAILURE;
  }

  mURL = aURL;

  NS_ASSERTION(mPluginInstance == nullptr,
               "nsPluginStreamListenerPeer::Initialize mPluginInstance != nullptr");
  mPluginInstance = aInstance;

  
  
  
  if (aListener) {
    mPStreamListener = aListener;
    mPStreamListener->SetStreamListenerPeer(this);
  }

  mPendingRequests = 1;

  mDataForwardToRequest = new nsDataHashtable<nsUint32HashKey, uint32_t>();

  return NS_OK;
}






nsresult
nsPluginStreamListenerPeer::SetupPluginCacheFile(nsIChannel* channel)
{
  nsresult rv = NS_OK;

  bool useExistingCacheFile = false;
  nsRefPtr<nsPluginHost> pluginHost = nsPluginHost::GetInst();

  
  nsTArray< nsRefPtr<nsNPAPIPluginInstance> > *instances = pluginHost->InstanceArray();
  for (uint32_t i = 0; i < instances->Length(); i++) {
    
    nsTArray<nsPluginStreamListenerPeer*> *streamListeners = instances->ElementAt(i)->FileCachedStreamListeners();
    for (int32_t i = streamListeners->Length() - 1; i >= 0; --i) {
      nsPluginStreamListenerPeer *lp = streamListeners->ElementAt(i);
      if (lp && lp->mLocalCachedFileHolder) {
        useExistingCacheFile = lp->UseExistingPluginCacheFile(this);
        if (useExistingCacheFile) {
          mLocalCachedFileHolder = lp->mLocalCachedFileHolder;
          break;
        }
      }
      if (useExistingCacheFile)
        break;
    }
  }

  
  if (!useExistingCacheFile) {
    nsCOMPtr<nsIFile> pluginTmp;
    rv = nsPluginHost::GetPluginTempDir(getter_AddRefs(pluginTmp));
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    nsCOMPtr<nsIURI> uri;
    rv = channel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
    if (!url)
      return NS_ERROR_FAILURE;

    nsAutoCString filename;
    url->GetFileName(filename);
    if (NS_FAILED(rv))
      return rv;

    
    filename.Insert(NS_LITERAL_CSTRING("plugin-"), 0);
    rv = pluginTmp->AppendNative(filename);
    if (NS_FAILED(rv))
      return rv;

    
    rv = pluginTmp->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
    if (NS_FAILED(rv))
      return rv;

    
    nsCOMPtr<nsIOutputStream> outstream;
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(mFileCacheOutputStream), pluginTmp, -1, 00600);
    if (NS_FAILED(rv))
      return rv;

    
    mLocalCachedFileHolder = new CachedFileHolder(pluginTmp);
  }

  
  mPluginInstance->FileCachedStreamListeners()->AppendElement(this);

  return rv;
}

NS_IMETHODIMP
nsPluginStreamListenerPeer::OnStartRequest(nsIRequest *request,
                                           nsISupports* aContext)
{
  nsresult rv = NS_OK;
  PROFILER_LABEL("nsPluginStreamListenerPeer", "OnStartRequest",
    js::ProfileEntry::Category::OTHER);

  if (mRequests.IndexOfObject(GetBaseRequest(request)) == -1) {
    NS_ASSERTION(mRequests.Count() == 0,
                 "Only our initial stream should be unknown!");
    TrackRequest(request);
  }

  if (mHaveFiredOnStartRequest) {
    return NS_OK;
  }

  mHaveFiredOnStartRequest = true;

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  NS_ENSURE_TRUE(channel, NS_ERROR_FAILURE);

  
  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel) {
    uint32_t responseCode = 0;
    rv = httpChannel->GetResponseStatus(&responseCode);
    if (NS_FAILED(rv)) {
      
      
      
      
      mRequestFailed = true;
      return NS_ERROR_FAILURE;
    }

    if (responseCode > 206) { 
      uint32_t wantsAllNetworkStreams = 0;

      
      
      if (mPluginInstance) {
        rv = mPluginInstance->GetValueFromPlugin(NPPVpluginWantsAllNetworkStreams,
                                                 &wantsAllNetworkStreams);
        
        if (NS_FAILED(rv)) {
          wantsAllNetworkStreams = 0;
        }
      }

      if (!wantsAllNetworkStreams) {
        mRequestFailed = true;
        return NS_ERROR_FAILURE;
      }
    }
  }

  nsAutoCString contentType;
  rv = channel->GetContentType(contentType);
  if (NS_FAILED(rv))
    return rv;

  
  nsRefPtr<nsPluginInstanceOwner> owner;
  if (mPluginInstance) {
    owner = mPluginInstance->GetOwner();
  }
  nsCOMPtr<nsIDOMElement> element;
  nsCOMPtr<nsIDocument> doc;
  if (owner) {
    owner->GetDOMElement(getter_AddRefs(element));
    owner->GetDocument(getter_AddRefs(doc));
  }
  nsCOMPtr<nsIPrincipal> principal = doc ? doc->NodePrincipal() : nullptr;

  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentProcessPolicy(nsIContentPolicy::TYPE_OBJECT_SUBREQUEST,
                                    mURL,
                                    principal,
                                    element,
                                    contentType,
                                    nullptr,
                                    &shouldLoad);
  if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
    mRequestFailed = true;
    return NS_ERROR_CONTENT_BLOCKED;
  }

  
  
  
  nsCOMPtr<nsIInterfaceRequestor> callbacks;
  channel->GetNotificationCallbacks(getter_AddRefs(callbacks));
  if (callbacks)
    mWeakPtrChannelCallbacks = do_GetWeakReference(callbacks);

  nsCOMPtr<nsILoadGroup> loadGroup;
  channel->GetLoadGroup(getter_AddRefs(loadGroup));
  if (loadGroup)
    mWeakPtrChannelLoadGroup = do_GetWeakReference(loadGroup);

  int64_t length;
  rv = channel->GetContentLength(&length);

  
  
  if (NS_FAILED(rv) || length < 0 || length > UINT32_MAX) {
    
    nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(channel);
    if (fileChannel) {
      
      mRequestFailed = true;
      return NS_ERROR_FAILURE;
    }
    mLength = 0;
  }
  else {
    mLength = uint32_t(length);
  }

  nsCOMPtr<nsIURI> aURL;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv))
    return rv;

  aURL->GetSpec(mURLSpec);

  if (!contentType.IsEmpty())
    mContentType = contentType;

#ifdef PLUGIN_LOGGING
  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NOISY,
         ("nsPluginStreamListenerPeer::OnStartRequest this=%p request=%p mime=%s, url=%s\n",
          this, request, contentType.get(), mURLSpec.get()));

  PR_LogFlush();
#endif

  
  rv = SetUpStreamListener(request, aURL);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return rv;
}

NS_IMETHODIMP nsPluginStreamListenerPeer::OnProgress(nsIRequest *request,
                                                     nsISupports* aContext,
                                                     int64_t aProgress,
                                                     int64_t aProgressMax)
{
  nsresult rv = NS_OK;
  return rv;
}

NS_IMETHODIMP nsPluginStreamListenerPeer::OnStatus(nsIRequest *request,
                                                   nsISupports* aContext,
                                                   nsresult aStatus,
                                                   const char16_t* aStatusArg)
{
  return NS_OK;
}

nsresult
nsPluginStreamListenerPeer::GetContentType(char** result)
{
  *result = const_cast<char*>(mContentType.get());
  return NS_OK;
}


nsresult
nsPluginStreamListenerPeer::IsSeekable(bool* result)
{
  *result = mSeekable;
  return NS_OK;
}

nsresult
nsPluginStreamListenerPeer::GetLength(uint32_t* result)
{
  *result = mLength;
  return NS_OK;
}

nsresult
nsPluginStreamListenerPeer::GetLastModified(uint32_t* result)
{
  *result = mModified;
  return NS_OK;
}

nsresult
nsPluginStreamListenerPeer::GetURL(const char** result)
{
  *result = mURLSpec.get();
  return NS_OK;
}

void
nsPluginStreamListenerPeer::MakeByteRangeString(NPByteRange* aRangeList, nsACString &rangeRequest,
                                                int32_t *numRequests)
{
  rangeRequest.Truncate();
  *numRequests  = 0;
  
  if (!aRangeList)
    return;

  int32_t requestCnt = 0;
  nsAutoCString string("bytes=");

  for (NPByteRange * range = aRangeList; range != nullptr; range = range->next) {
    
    if (!range->length)
      continue;

    
    string.AppendInt(range->offset);
    string.Append('-');
    string.AppendInt(range->offset + range->length - 1);
    if (range->next)
      string.Append(',');

    requestCnt++;
  }

  
  string.Trim(",", false);

  rangeRequest = string;
  *numRequests  = requestCnt;
  return;
}

nsresult
nsPluginStreamListenerPeer::RequestRead(NPByteRange* rangeList)
{
  nsAutoCString rangeString;
  int32_t numRequests;

  MakeByteRangeString(rangeList, rangeString, &numRequests);

  if (numRequests == 0)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;

  nsRefPtr<nsPluginInstanceOwner> owner = mPluginInstance->GetOwner();
  nsCOMPtr<nsIDOMElement> element;
  nsCOMPtr<nsIDocument> doc;
  if (owner) {
    rv = owner->GetDOMElement(getter_AddRefs(element));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = owner->GetDocument(getter_AddRefs(doc));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIInterfaceRequestor> callbacks = do_QueryReferent(mWeakPtrChannelCallbacks);
  nsCOMPtr<nsILoadGroup> loadGroup = do_QueryReferent(mWeakPtrChannelLoadGroup);

  nsCOMPtr<nsIChannel> channel;
  nsCOMPtr<nsINode> requestingNode(do_QueryInterface(element));
  if (requestingNode) {
    rv = NS_NewChannel(getter_AddRefs(channel),
                       mURL,
                       requestingNode,
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER,
                       loadGroup,
                       callbacks);
  }
  else {
    
    
    
    nsCOMPtr<nsIPrincipal> principal =
      do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = NS_NewChannel(getter_AddRefs(channel),
                       mURL,
                       principal,
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER,
                       loadGroup,
                       callbacks);
  }

  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (!httpChannel)
    return NS_ERROR_FAILURE;

  httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Range"), rangeString, false);

  mAbort = true; 
  

  nsCOMPtr<nsIStreamListener> converter;

  if (numRequests == 1) {
    converter = this;
    
    
    
    SetStreamOffset(rangeList->offset);
  } else {
    nsWeakPtr weakpeer =
    do_GetWeakReference(static_cast<nsISupportsWeakReference*>(this));
    nsPluginByteRangeStreamListener *brrListener =
    new nsPluginByteRangeStreamListener(weakpeer);
    if (brrListener)
      converter = brrListener;
    else
      return NS_ERROR_OUT_OF_MEMORY;
  }

  mPendingRequests += numRequests;

  nsCOMPtr<nsISupportsPRUint32> container = do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;
  rv = container->SetData(MAGIC_REQUEST_CONTEXT);
  if (NS_FAILED(rv))
    return rv;

  rv = channel->AsyncOpen(converter, container);
  if (NS_SUCCEEDED(rv))
    TrackRequest(channel);
  return rv;
}

nsresult
nsPluginStreamListenerPeer::GetStreamOffset(int32_t* result)
{
  *result = mStreamOffset;
  return NS_OK;
}

nsresult
nsPluginStreamListenerPeer::SetStreamOffset(int32_t value)
{
  mStreamOffset = value;
  return NS_OK;
}

nsresult nsPluginStreamListenerPeer::ServeStreamAsFile(nsIRequest *request,
                                                       nsISupports* aContext)
{
  if (!mPluginInstance)
    return NS_ERROR_FAILURE;

  
  mPluginInstance->Stop();
  mPluginInstance->Start();
  nsRefPtr<nsPluginInstanceOwner> owner = mPluginInstance->GetOwner();
  if (owner) {
    NPWindow* window = nullptr;
    owner->GetWindow(window);
#if (MOZ_WIDGET_GTK == 2) || defined(MOZ_WIDGET_QT)
    
    
    nsCOMPtr<nsIWidget> widget;
    ((nsPluginNativeWindow*)window)->GetPluginWidget(getter_AddRefs(widget));
    if (widget) {
      window->window = widget->GetNativeData(NS_NATIVE_PLUGIN_PORT);
    }
#endif
    owner->CallSetWindow();
  }

  mSeekable = false;
  mPStreamListener->OnStartBinding(this);
  mStreamOffset = 0;

  
  mStreamType = NP_ASFILE;

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (channel) {
    SetupPluginCacheFile(channel);
  }

  
  mPendingRequests = 0;

  return NS_OK;
}

bool
nsPluginStreamListenerPeer::UseExistingPluginCacheFile(nsPluginStreamListenerPeer* psi)
{
  NS_ENSURE_TRUE(psi, false);

  if (psi->mLength == mLength &&
      psi->mModified == mModified &&
      mStreamComplete &&
      mURLSpec.Equals(psi->mURLSpec))
  {
    return true;
  }
  return false;
}

NS_IMETHODIMP nsPluginStreamListenerPeer::OnDataAvailable(nsIRequest *request,
                                                          nsISupports* aContext,
                                                          nsIInputStream *aIStream,
                                                          uint64_t sourceOffset,
                                                          uint32_t aLength)
{
  if (mRequests.IndexOfObject(GetBaseRequest(request)) == -1) {
    MOZ_ASSERT(false, "Received OnDataAvailable for untracked request.");
    return NS_ERROR_UNEXPECTED;
  }

  if (mRequestFailed)
    return NS_ERROR_FAILURE;

  if (mAbort) {
    uint32_t magicNumber = 0;  
    nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(aContext);
    if (container)
      container->GetData(&magicNumber);

    if (magicNumber != MAGIC_REQUEST_CONTEXT) {
      
      mAbort = false;
      return NS_BINDING_ABORTED;
    }
  }

  nsresult rv = NS_OK;

  if (!mPStreamListener)
    return NS_ERROR_FAILURE;

  const char * url = nullptr;
  GetURL(&url);

  PLUGIN_LOG(PLUGIN_LOG_NOISY,
             ("nsPluginStreamListenerPeer::OnDataAvailable this=%p request=%p, offset=%llu, length=%u, url=%s\n",
              this, request, sourceOffset, aLength, url ? url : "no url set"));

  
  
  if (mStreamType != NP_ASFILEONLY) {
    
    nsCOMPtr<nsIByteRangeRequest> brr = do_QueryInterface(request);
    if (brr) {
      if (!mDataForwardToRequest)
        return NS_ERROR_FAILURE;

      int64_t absoluteOffset64 = 0;
      brr->GetStartRange(&absoluteOffset64);

      
      int32_t absoluteOffset = (int32_t)int64_t(absoluteOffset64);

      
      

      
      
      
      
      int32_t amtForwardToPlugin = mDataForwardToRequest->Get(absoluteOffset);
      mDataForwardToRequest->Put(absoluteOffset, (amtForwardToPlugin + aLength));

      SetStreamOffset(absoluteOffset + amtForwardToPlugin);
    }

    nsCOMPtr<nsIInputStream> stream = aIStream;

    
    
    

    if (mFileCacheOutputStream) {
      rv = NS_NewInputStreamTee(getter_AddRefs(stream), aIStream, mFileCacheOutputStream);
      if (NS_FAILED(rv))
        return rv;
    }

    rv =  mPStreamListener->OnDataAvailable(this,
                                            stream,
                                            aLength);

    
    
    if (NS_FAILED(rv))
      request->Cancel(rv);
  }
  else
  {
    
    char* buffer = new char[aLength];
    uint32_t amountRead, amountWrote = 0;
    rv = aIStream->Read(buffer, aLength, &amountRead);

    
    if (mFileCacheOutputStream) {
      while (amountWrote < amountRead && NS_SUCCEEDED(rv)) {
        rv = mFileCacheOutputStream->Write(buffer, amountRead, &amountWrote);
      }
    }
    delete [] buffer;
  }
  return rv;
}

NS_IMETHODIMP nsPluginStreamListenerPeer::OnStopRequest(nsIRequest *request,
                                                        nsISupports* aContext,
                                                        nsresult aStatus)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIMultiPartChannel> mp = do_QueryInterface(request);
  if (!mp) {
    bool found = mRequests.RemoveObject(request);
    if (!found) {
      NS_ERROR("Received OnStopRequest for untracked request.");
    }
  }

  PLUGIN_LOG(PLUGIN_LOG_NOISY,
             ("nsPluginStreamListenerPeer::OnStopRequest this=%p aStatus=%d request=%p\n",
              this, aStatus, request));

  
  nsCOMPtr<nsIByteRangeRequest> brr = do_QueryInterface(request);
  if (brr) {
    int64_t absoluteOffset64 = 0;
    brr->GetStartRange(&absoluteOffset64);
    
    int32_t absoluteOffset = (int32_t)int64_t(absoluteOffset64);

    
    mDataForwardToRequest->Remove(absoluteOffset);


    PLUGIN_LOG(PLUGIN_LOG_NOISY,
               ("                          ::OnStopRequest for ByteRangeRequest Started=%d\n",
                absoluteOffset));
  } else {
    
    
    
    mFileCacheOutputStream = nullptr;
  }

  
  if (--mPendingRequests > 0)
    return NS_OK;

  
  nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(aContext);
  if (container) {
    uint32_t magicNumber = 0;  
    container->GetData(&magicNumber);
    if (magicNumber == MAGIC_REQUEST_CONTEXT) {
      
      return NS_OK;
    }
  }

  if (!mPStreamListener)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (!channel)
    return NS_ERROR_FAILURE;
  
  nsAutoCString aContentType;
  rv = channel->GetContentType(aContentType);
  if (NS_FAILED(rv) && !mRequestFailed)
    return rv;

  if (!aContentType.IsEmpty())
    mContentType = aContentType;

  
  if (mRequestFailed)
    aStatus = NS_ERROR_FAILURE;

  if (NS_FAILED(aStatus)) {
    
    
    mPStreamListener->OnStopBinding(this, aStatus);
    return NS_OK;
  }

  
  if (mStreamType >= NP_ASFILE) {
    nsCOMPtr<nsIFile> localFile;
    if (mLocalCachedFileHolder)
      localFile = mLocalCachedFileHolder->file();
    else {
      
      nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(request);
      if (fileChannel) {
        fileChannel->GetFile(getter_AddRefs(localFile));
      }
    }

    if (localFile) {
      OnFileAvailable(localFile);
    }
  }

  if (mStartBinding) {
    
    mPStreamListener->OnStopBinding(this, aStatus);
  } else {
    
    mPStreamListener->OnStartBinding(this);
    mPStreamListener->OnStopBinding(this, aStatus);
  }

  if (NS_SUCCEEDED(aStatus)) {
    mStreamComplete = true;
  }

  return NS_OK;
}

nsresult nsPluginStreamListenerPeer::SetUpStreamListener(nsIRequest *request,
                                                         nsIURI* aURL)
{
  nsresult rv = NS_OK;

  
  
  
  
  
  if (!mPStreamListener) {
    if (!mPluginInstance) {
      return NS_ERROR_FAILURE;
    }

    nsRefPtr<nsNPAPIPluginStreamListener> streamListener;
    rv = mPluginInstance->NewStreamListener(nullptr, nullptr,
                                            getter_AddRefs(streamListener));
    if (NS_FAILED(rv) || !streamListener) {
      return NS_ERROR_FAILURE;
    }

    mPStreamListener = static_cast<nsNPAPIPluginStreamListener*>(streamListener.get());
  }

  mPStreamListener->SetStreamListenerPeer(this);

  
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel);

  




  if (httpChannel) {
    
    
    
    
    uint32_t statusNum;
    if (NS_SUCCEEDED(httpChannel->GetResponseStatus(&statusNum)) &&
        statusNum < 1000) {
      
      nsCString ver;
      nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal =
      do_QueryInterface(channel);
      if (httpChannelInternal) {
        uint32_t major, minor;
        if (NS_SUCCEEDED(httpChannelInternal->GetResponseVersion(&major,
                                                                 &minor))) {
          ver = nsPrintfCString("/%lu.%lu", major, minor);
        }
      }

      
      nsCString statusText;
      if (NS_FAILED(httpChannel->GetResponseStatusText(statusText))) {
        statusText = "OK";
      }

      
      nsPrintfCString status("HTTP%s %lu %s", ver.get(), statusNum,
                             statusText.get());
      static_cast<nsIHTTPHeaderListener*>(mPStreamListener)->StatusLine(status.get());
    }

    
    httpChannel->VisitResponseHeaders(this);

    mSeekable = false;
    
    
    
    
    
    
    
    nsAutoCString contentEncoding;
    if (NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Encoding"),
                                                    contentEncoding))) {
      mUseLocalCache = true;
    } else {
      
      
      uint32_t length;
      GetLength(&length);
      if (length) {
        nsAutoCString range;
        if (NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("accept-ranges"), range)) &&
            range.Equals(NS_LITERAL_CSTRING("bytes"), nsCaseInsensitiveCStringComparator())) {
          mSeekable = true;
        }
      }
    }

    
    
    nsAutoCString lastModified;
    if (NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("last-modified"), lastModified)) &&
        !lastModified.IsEmpty()) {
      PRTime time64;
      PR_ParseTimeString(lastModified.get(), true, &time64);  

      
      double fpTime = double(time64);
      mModified = (uint32_t)(fpTime * 1e-6 + 0.5);
    }
  }

  MOZ_ASSERT(!mRequest);
  mRequest = request;

  rv = mPStreamListener->OnStartBinding(this);

  mStartBinding = true;

  if (NS_FAILED(rv))
    return rv;

  int32_t streamType = NP_NORMAL;
  mPStreamListener->GetStreamType(&streamType);

  if (streamType != STREAM_TYPE_UNKNOWN) {
    OnStreamTypeSet(streamType);
  }

  return NS_OK;
}

void
nsPluginStreamListenerPeer::OnStreamTypeSet(const int32_t aStreamType)
{
  MOZ_ASSERT(aStreamType != STREAM_TYPE_UNKNOWN);
  MOZ_ASSERT(mRequest);
  mStreamType = aStreamType;
  if (!mUseLocalCache && mStreamType >= NP_ASFILE) {
    
    nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(mRequest);
    if (!fileChannel) {
      mUseLocalCache = true;
    }
  }

  if (mUseLocalCache) {
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(mRequest);
    SetupPluginCacheFile(channel);
  }
}

nsresult
nsPluginStreamListenerPeer::OnFileAvailable(nsIFile* aFile)
{
  nsresult rv;
  if (!mPStreamListener)
    return NS_ERROR_FAILURE;

  nsAutoCString path;
  rv = aFile->GetNativePath(path);
  if (NS_FAILED(rv)) return rv;

  if (path.IsEmpty()) {
    NS_WARNING("empty path");
    return NS_OK;
  }

  rv = mPStreamListener->OnFileAvailable(this, path.get());
  return rv;
}

NS_IMETHODIMP
nsPluginStreamListenerPeer::VisitHeader(const nsACString &header, const nsACString &value)
{
  return mPStreamListener->NewResponseHeader(PromiseFlatCString(header).get(),
                                             PromiseFlatCString(value).get());
}

nsresult
nsPluginStreamListenerPeer::GetInterfaceGlobal(const nsIID& aIID, void** result)
{
  if (!mPluginInstance) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsPluginInstanceOwner> owner = mPluginInstance->GetOwner();
  if (owner) {
    nsCOMPtr<nsIDocument> doc;
    nsresult rv = owner->GetDocument(getter_AddRefs(doc));
    if (NS_SUCCEEDED(rv) && doc) {
      nsPIDOMWindow *window = doc->GetWindow();
      if (window) {
        nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(window);
        nsCOMPtr<nsIInterfaceRequestor> ir = do_QueryInterface(webNav);
        return ir->GetInterface(aIID, result);
      }
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsPluginStreamListenerPeer::GetInterface(const nsIID& aIID, void** result)
{
  
  
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    return QueryInterface(aIID, result);
  }

  return GetInterfaceGlobal(aIID, result);
}






class ChannelRedirectProxyCallback : public nsIAsyncVerifyRedirectCallback
{
public:
  ChannelRedirectProxyCallback(nsPluginStreamListenerPeer* listener,
                               nsIAsyncVerifyRedirectCallback* parent,
                               nsIChannel* oldChannel,
                               nsIChannel* newChannel)
    : mWeakListener(do_GetWeakReference(static_cast<nsIStreamListener*>(listener)))
    , mParent(parent)
    , mOldChannel(oldChannel)
    , mNewChannel(newChannel)
  {
  }

  ChannelRedirectProxyCallback() {}

  NS_DECL_ISUPPORTS

  NS_IMETHODIMP OnRedirectVerifyCallback(nsresult aResult) override
  {
    if (NS_SUCCEEDED(aResult)) {
      nsCOMPtr<nsIStreamListener> listener = do_QueryReferent(mWeakListener);
      if (listener)
        static_cast<nsPluginStreamListenerPeer*>(listener.get())->ReplaceRequest(mOldChannel, mNewChannel);
    }
    return mParent->OnRedirectVerifyCallback(aResult);
  }

private:
  virtual ~ChannelRedirectProxyCallback() {}

  nsWeakPtr mWeakListener;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mParent;
  nsCOMPtr<nsIChannel> mOldChannel;
  nsCOMPtr<nsIChannel> mNewChannel;
};

NS_IMPL_ISUPPORTS(ChannelRedirectProxyCallback, nsIAsyncVerifyRedirectCallback)


NS_IMETHODIMP
nsPluginStreamListenerPeer::AsyncOnChannelRedirect(nsIChannel *oldChannel, nsIChannel *newChannel,
                                                   uint32_t flags, nsIAsyncVerifyRedirectCallback* callback)
{
  
  if (!mPStreamListener) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAsyncVerifyRedirectCallback> proxyCallback =
    new ChannelRedirectProxyCallback(this, callback, oldChannel, newChannel);

  
  bool notificationHandled = mPStreamListener->HandleRedirectNotification(oldChannel, newChannel, proxyCallback);
  if (notificationHandled) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIHttpChannel> oldHttpChannel(do_QueryInterface(oldChannel));
  if (oldHttpChannel) {
    uint32_t responseStatus;
    nsresult rv = oldHttpChannel->GetResponseStatus(&responseStatus);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (responseStatus == 307) {
      nsAutoCString method;
      rv = oldHttpChannel->GetRequestMethod(method);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (method.EqualsLiteral("POST")) {
        rv = nsContentUtils::CheckSameOrigin(oldChannel, newChannel);
        if (NS_FAILED(rv)) {
          return rv;
        }
      }
    }
  }

  
  nsCOMPtr<nsIChannelEventSink> channelEventSink;
  nsresult rv = GetInterfaceGlobal(NS_GET_IID(nsIChannelEventSink), getter_AddRefs(channelEventSink));
  if (NS_FAILED(rv)) {
    return rv;
  }

  return channelEventSink->AsyncOnChannelRedirect(oldChannel, newChannel, flags, proxyCallback);
}
