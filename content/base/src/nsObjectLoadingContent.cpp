













































#include "imgILoader.h"
#include "nsEventDispatcher.h"
#include "nsIContent.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIDOMDataContainerEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsIExternalProtocolHandler.h"
#include "nsEventStates.h"
#include "nsIObjectFrame.h"
#include "nsIPluginDocument.h"
#include "nsPluginHost.h"
#include "nsIPresShell.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsIStreamConverterService.h"
#include "nsIURILoader.h"
#include "nsIURL.h"
#include "nsIWebNavigation.h"
#include "nsIWebNavigationInfo.h"
#include "nsIScriptChannel.h"
#include "nsIBlocklistService.h"
#include "nsIAsyncVerifyRedirectCallback.h"

#include "nsPluginError.h"


#include "prlog.h"

#include "nsAutoPtr.h"
#include "nsCURILoader.h"
#include "nsContentPolicyUtils.h"
#include "nsContentUtils.h"
#include "nsDocShellCID.h"
#include "nsGkAtoms.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsMimeTypes.h"
#include "nsStyleUtil.h"
#include "nsGUIEvent.h"
#include "nsUnicharUtils.h"


#include "nsFrameLoader.h"

#include "nsObjectLoadingContent.h"
#include "mozAutoDocUpdate.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIChannelPolicy.h"
#include "nsChannelPolicy.h"
#include "mozilla/dom/Element.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gObjectLog = PR_NewLogModule("objlc");
#endif

#define LOG(args) PR_LOG(gObjectLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gObjectLog, PR_LOG_DEBUG)

#ifdef ANDROID
#include "nsXULAppAPI.h"
#include "mozilla/Preferences.h"
#endif

class nsAsyncInstantiateEvent : public nsRunnable {
public:
  
  
  nsObjectLoadingContent *mContent;
  nsWeakFrame             mFrame;
  nsCString               mContentType;
  nsCOMPtr<nsIURI>        mURI;

  nsAsyncInstantiateEvent(nsObjectLoadingContent* aContent,
                          nsIFrame* aFrame,
                          const nsCString& aType,
                          nsIURI* aURI)
    : mContent(aContent), mFrame(aFrame), mContentType(aType), mURI(aURI)
  {
    static_cast<nsIObjectLoadingContent *>(mContent)->AddRef();
  }

  ~nsAsyncInstantiateEvent()
  {
    static_cast<nsIObjectLoadingContent *>(mContent)->Release();
  }

  NS_IMETHOD Run();
};

NS_IMETHODIMP
nsAsyncInstantiateEvent::Run()
{
  
  if (mContent->mPendingInstantiateEvent != this)
    return NS_OK;
  mContent->mPendingInstantiateEvent = nsnull;

  
  
  
  
  nsIObjectFrame* frame = mContent->
    GetExistingFrame(nsObjectLoadingContent::eFlushContent);

  nsIFrame* objectFrame = nsnull;
  if (frame) {
    objectFrame = do_QueryFrame(frame);
  }

  if (objectFrame &&
      mFrame.GetFrame() == objectFrame &&
      mContent->mURI == mURI &&
      mContent->mContentType.Equals(mContentType)) {
    if (LOG_ENABLED()) {
      nsCAutoString spec;
      if (mURI) {
        mURI->GetSpec(spec);
      }
      LOG(("OBJLC [%p]: Handling Instantiate event: Type=<%s> URI=%p<%s>\n",
           mContent, mContentType.get(), mURI.get(), spec.get()));
    }

    nsresult rv = mContent->Instantiate(frame, mContentType, mURI);
    if (NS_FAILED(rv)) {
      mContent->Fallback(true);
    }
  } else {
    LOG(("OBJLC [%p]: Discarding event, data changed\n", mContent));
  }

  return NS_OK;
}




class nsPluginErrorEvent : public nsRunnable {
public:
  nsCOMPtr<nsIContent> mContent;
  PluginSupportState mState;

  nsPluginErrorEvent(nsIContent* aContent, PluginSupportState aState)
    : mContent(aContent),
      mState(aState)
  {}

  ~nsPluginErrorEvent() {}

  NS_IMETHOD Run();
};

NS_IMETHODIMP
nsPluginErrorEvent::Run()
{
  LOG(("OBJLC []: Firing plugin not found event for content %p\n",
       mContent.get()));
  nsString type;
  switch (mState) {
    case ePluginClickToPlay:
      type = NS_LITERAL_STRING("PluginClickToPlay");
      break;
    case ePluginUnsupported:
      type = NS_LITERAL_STRING("PluginNotFound");
      break;
    case ePluginDisabled:
      type = NS_LITERAL_STRING("PluginDisabled");
      break;
    case ePluginBlocklisted:
      type = NS_LITERAL_STRING("PluginBlocklisted");
      break;
    case ePluginOutdated:
      type = NS_LITERAL_STRING("PluginOutdated");
      break;
    default:
      return NS_OK;
  }
  nsContentUtils::DispatchTrustedEvent(mContent->GetDocument(), mContent,
                                       type, true, true);

  return NS_OK;
}




class nsPluginCrashedEvent : public nsRunnable {
public:
  nsCOMPtr<nsIContent> mContent;
  nsString mPluginDumpID;
  nsString mBrowserDumpID;
  nsString mPluginName;
  nsString mPluginFilename;
  bool mSubmittedCrashReport;

  nsPluginCrashedEvent(nsIContent* aContent,
                       const nsAString& aPluginDumpID,
                       const nsAString& aBrowserDumpID,
                       const nsAString& aPluginName,
                       const nsAString& aPluginFilename,
                       bool submittedCrashReport)
    : mContent(aContent),
      mPluginDumpID(aPluginDumpID),
      mBrowserDumpID(aBrowserDumpID),
      mPluginName(aPluginName),
      mPluginFilename(aPluginFilename),
      mSubmittedCrashReport(submittedCrashReport)
  {}

  ~nsPluginCrashedEvent() {}

  NS_IMETHOD Run();
};

NS_IMETHODIMP
nsPluginCrashedEvent::Run()
{
  LOG(("OBJLC []: Firing plugin crashed event for content %p\n",
       mContent.get()));

  nsCOMPtr<nsIDOMDocument> domDoc =
    do_QueryInterface(mContent->GetDocument());
  if (!domDoc) {
    NS_WARNING("Couldn't get document for PluginCrashed event!");
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEvent> event;
  domDoc->CreateEvent(NS_LITERAL_STRING("datacontainerevents"),
                      getter_AddRefs(event));
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
  nsCOMPtr<nsIDOMDataContainerEvent> containerEvent(do_QueryInterface(event));
  if (!privateEvent || !containerEvent) {
    NS_WARNING("Couldn't QI event for PluginCrashed event!");
    return NS_OK;
  }

  event->InitEvent(NS_LITERAL_STRING("PluginCrashed"), true, true);
  privateEvent->SetTrusted(true);
  privateEvent->GetInternalNSEvent()->flags |= NS_EVENT_FLAG_ONLY_CHROME_DISPATCH;
  
  nsCOMPtr<nsIWritableVariant> variant;

  
  variant = do_CreateInstance("@mozilla.org/variant;1");
  if (!variant) {
    NS_WARNING("Couldn't create pluginDumpID variant for PluginCrashed event!");
    return NS_OK;
  }
  variant->SetAsAString(mPluginDumpID);
  containerEvent->SetData(NS_LITERAL_STRING("pluginDumpID"), variant);

  
  variant = do_CreateInstance("@mozilla.org/variant;1");
  if (!variant) {
    NS_WARNING("Couldn't create browserDumpID variant for PluginCrashed event!");
    return NS_OK;
  }
  variant->SetAsAString(mBrowserDumpID);
  containerEvent->SetData(NS_LITERAL_STRING("browserDumpID"), variant);

  
  variant = do_CreateInstance("@mozilla.org/variant;1");
  if (!variant) {
    NS_WARNING("Couldn't create pluginName variant for PluginCrashed event!");
    return NS_OK;
  }
  variant->SetAsAString(mPluginName);
  containerEvent->SetData(NS_LITERAL_STRING("pluginName"), variant);

  
  variant = do_CreateInstance("@mozilla.org/variant;1");
  if (!variant) {
    NS_WARNING("Couldn't create pluginFilename variant for PluginCrashed event!");
    return NS_OK;
  }
  variant->SetAsAString(mPluginFilename);
  containerEvent->SetData(NS_LITERAL_STRING("pluginFilename"), variant);

  
  variant = do_CreateInstance("@mozilla.org/variant;1");
  if (!variant) {
    NS_WARNING("Couldn't create crashSubmit variant for PluginCrashed event!");
    return NS_OK;
  }
  variant->SetAsBool(mSubmittedCrashReport);
  containerEvent->SetData(NS_LITERAL_STRING("submittedCrashReport"), variant);

  nsEventDispatcher::DispatchDOMEvent(mContent, nsnull, event, nsnull, nsnull);
  return NS_OK;
}

class AutoNotifier {
  public:
    AutoNotifier(nsObjectLoadingContent* aContent, bool aNotify) :
      mContent(aContent), mNotify(aNotify) {
        mOldType = aContent->Type();
        mOldState = aContent->ObjectState();
    }
    ~AutoNotifier() {
      mContent->NotifyStateChanged(mOldType, mOldState, false, mNotify);
    }

    




    void Notify() {
      NS_ASSERTION(mNotify, "Should not notify when notify=false");

      mContent->NotifyStateChanged(mOldType, mOldState, true, true);
      mOldType = mContent->Type();
      mOldState = mContent->ObjectState();
    }

  private:
    nsObjectLoadingContent*            mContent;
    bool                               mNotify;
    nsObjectLoadingContent::ObjectType mOldType;
    nsEventStates                      mOldState;
};





class AutoFallback {
  public:
    AutoFallback(nsObjectLoadingContent* aContent, const nsresult* rv)
      : mContent(aContent), mResult(rv), mPluginState(ePluginOtherState) {}
    ~AutoFallback() {
      if (NS_FAILED(*mResult)) {
        LOG(("OBJLC [%p]: rv=%08x, falling back\n", mContent, *mResult));
        mContent->Fallback(false);
        if (mPluginState != ePluginOtherState) {
          mContent->mFallbackReason = mPluginState;
        }
      }
    }

    



     void SetPluginState(PluginSupportState aState) {
       NS_ASSERTION(aState != ePluginOtherState, "Should not be setting ePluginOtherState");
       mPluginState = aState;
     }
  private:
    nsObjectLoadingContent* mContent;
    const nsresult* mResult;
    PluginSupportState mPluginState;
};





class AutoSetInstantiatingToFalse {
  public:
    AutoSetInstantiatingToFalse(nsObjectLoadingContent* objlc) : mContent(objlc) {}
    ~AutoSetInstantiatingToFalse() { mContent->mInstantiating = false; }
  private:
    nsObjectLoadingContent* mContent;
};


static bool
IsSupportedImage(const nsCString& aMimeType)
{
  imgILoader* loader = nsContentUtils::GetImgLoader();
  if (!loader) {
    return false;
  }

  bool supported;
  nsresult rv = loader->SupportImageWithMimeType(aMimeType.get(), &supported);
  return NS_SUCCEEDED(rv) && supported;
}

static bool
IsSupportedPlugin(const nsCString& aMIMEType)
{
  nsCOMPtr<nsIPluginHost> pluginHostCOM(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID));
  nsPluginHost *pluginHost = static_cast<nsPluginHost*>(pluginHostCOM.get());
  if (!pluginHost) {
    return false;
  }
  nsresult rv = pluginHost->IsPluginEnabledForType(aMIMEType.get());
  return NS_SUCCEEDED(rv);
}

static void
GetExtensionFromURI(nsIURI* uri, nsCString& ext)
{
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (url) {
    url->GetFileExtension(ext);
  } else {
    nsCString spec;
    uri->GetSpec(spec);

    PRInt32 offset = spec.RFindChar('.');
    if (offset != kNotFound) {
      ext = Substring(spec, offset + 1, spec.Length());
    }
  }
}





static bool
IsPluginEnabledByExtension(nsIURI* uri, nsCString& mimeType)
{
  nsCAutoString ext;
  GetExtensionFromURI(uri, ext);

  if (ext.IsEmpty()) {
    return false;
  }

  nsCOMPtr<nsIPluginHost> pluginHostCOM(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID));
  nsPluginHost *pluginHost = static_cast<nsPluginHost*>(pluginHostCOM.get());
  if (!pluginHost) {
    return false;
  }

  const char* typeFromExt;
  if (NS_SUCCEEDED(pluginHost->IsPluginEnabledForExtension(ext.get(), typeFromExt))) {
    mimeType = typeFromExt;
    return true;
  }
  return false;
}

nsObjectLoadingContent::nsObjectLoadingContent()
  : mPendingInstantiateEvent(nsnull)
  , mChannel(nsnull)
  , mType(eType_Loading)
  , mInstantiating(false)
  , mUserDisabled(false)
  , mSuppressed(false)
  , mNetworkCreated(true)
  , mFallbackReason(ePluginOtherState)
{
}

nsObjectLoadingContent::~nsObjectLoadingContent()
{
  DestroyImageLoadingContent();
  if (mFrameLoader) {
    mFrameLoader->Destroy();
  }
}


NS_IMETHODIMP
nsObjectLoadingContent::OnStartRequest(nsIRequest *aRequest,
                                       nsISupports *aContext)
{
  if (aRequest != mChannel || !aRequest) {
    
    
    return NS_BINDING_ABORTED;
  }

  AutoNotifier notifier(this, true);

  if (!IsSuccessfulRequest(aRequest)) {
    LOG(("OBJLC [%p]: OnStartRequest: Request failed\n", this));
    Fallback(false);
    return NS_BINDING_ABORTED;
  }

  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  NS_ASSERTION(chan, "Why is our request not a channel?");

  nsresult rv = NS_ERROR_UNEXPECTED;
  
  
  AutoFallback fallback(this, &rv);

  nsCString channelType;
  rv = chan->GetContentType(channelType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (channelType.EqualsASCII(APPLICATION_GUESS_FROM_EXT)) {
    channelType = APPLICATION_OCTET_STREAM;
    chan->SetContentType(channelType);
  }

  
  
  
  
  
  

  if ((channelType.EqualsASCII(APPLICATION_OCTET_STREAM) && 
       !mContentType.IsEmpty() &&
       GetTypeOfContent(mContentType) != eType_Document) ||
      
      
      
      (IsSupportedPlugin(mContentType) && 
       GetTypeOfContent(mContentType) == eType_Plugin)) {
    
    
    

    nsCAutoString typeHint, dummy;
    NS_ParseContentType(mContentType, typeHint, dummy);
    if (!typeHint.IsEmpty()) {
      chan->SetContentType(typeHint);
    }
  } else {
    mContentType = channelType;
  }

  nsCOMPtr<nsIURI> uri;
  chan->GetURI(getter_AddRefs(uri));

  if (mContentType.EqualsASCII(APPLICATION_OCTET_STREAM)) {
    nsCAutoString extType;
    if (IsPluginEnabledByExtension(uri, extType)) {
      mContentType = extType;
      chan->SetContentType(extType);
    }
  }

  
  
  
  ObjectType newType = GetTypeOfContent(mContentType);
  LOG(("OBJLC [%p]: OnStartRequest: Content Type=<%s> Old type=%u New Type=%u\n",
       this, mContentType.get(), mType, newType));

  
  
  PRInt32 contentPolicyType;
  switch (newType) {
    case eType_Image:
      contentPolicyType = nsIContentPolicy::TYPE_IMAGE;
      break;
    case eType_Document:
      contentPolicyType = nsIContentPolicy::TYPE_SUBDOCUMENT;
      break;
    default:
      contentPolicyType = nsIContentPolicy::TYPE_OBJECT;
      break;
  }
  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");

  nsIDocument* doc = thisContent->OwnerDoc();

  PRInt16 shouldProcess = nsIContentPolicy::ACCEPT;
  rv =
    NS_CheckContentProcessPolicy(contentPolicyType,
                                 uri,
                                 doc->NodePrincipal(),
                                 static_cast<nsIImageLoadingContent*>(this),
                                 mContentType,
                                 nsnull, 
                                 &shouldProcess,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  if (NS_FAILED(rv) || NS_CP_REJECTED(shouldProcess)) {
    HandleBeingBlockedByContentPolicy(rv, shouldProcess);
    rv = NS_OK; 
    return NS_BINDING_ABORTED;
  }  
  
  if (mType != newType) {
    UnloadContent();
  }

  switch (newType) {
    case eType_Image:
      rv = LoadImageWithChannel(chan, getter_AddRefs(mFinalListener));
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      if (!mFinalListener) {
        mType = newType;
        return NS_BINDING_ABORTED;
      }
      break;
    case eType_Document: {
      if (!mFrameLoader) {
        mFrameLoader = nsFrameLoader::Create(thisContent->AsElement(),
                                             mNetworkCreated);
        if (!mFrameLoader) {
          Fallback(false);
          return NS_ERROR_UNEXPECTED;
        }
      }

      rv = mFrameLoader->CheckForRecursiveLoad(uri);
      if (NS_FAILED(rv)) {
        Fallback(false);
        return rv;
      }

      if (mType != newType) {
        
        
        mType = newType;
        notifier.Notify();

        if (!mFrameLoader) {
          
          
          
          return NS_BINDING_ABORTED;
        }
      }

      
      
      nsLoadFlags flags = 0;
      chan->GetLoadFlags(&flags);
      flags |= nsIChannel::LOAD_DOCUMENT_URI;
      chan->SetLoadFlags(flags);

      nsCOMPtr<nsIDocShell> docShell;
      rv = mFrameLoader->GetDocShell(getter_AddRefs(docShell));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIInterfaceRequestor> req(do_QueryInterface(docShell));
      NS_ASSERTION(req, "Docshell must be an ifreq");

      nsCOMPtr<nsIURILoader>
        uriLoader(do_GetService(NS_URI_LOADER_CONTRACTID, &rv));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = uriLoader->OpenChannel(chan, nsIURILoader::DONT_RETARGET, req,
                                  getter_AddRefs(mFinalListener));
      break;
    }
    case eType_Plugin:
      mInstantiating = true;
      if (mType != newType) {
        
        mType = newType;
        notifier.Notify();
      }
      nsIObjectFrame* frame;
      frame = GetExistingFrame(eFlushLayout);
      if (!frame) {
        
        
        
        mInstantiating = false;
        return NS_BINDING_ABORTED;
      }

      {
        nsIFrame *nsiframe = do_QueryFrame(frame);

        nsWeakFrame weakFrame(nsiframe);

        rv = frame->Instantiate(chan, getter_AddRefs(mFinalListener));

        mInstantiating = false;

        if (!weakFrame.IsAlive()) {
          
          return NS_BINDING_ABORTED;
        }
      }

      break;
    case eType_Loading:
      NS_NOTREACHED("Should not have a loading type here!");
    case eType_Null:
      LOG(("OBJLC [%p]: Unsupported type, falling back\n", this));
      
      
      
      Fallback(false);

      PluginSupportState pluginState = GetPluginSupportState(thisContent,
                                                             mContentType);
      
      if (pluginState != ePluginOtherState) {
        mFallbackReason = pluginState;
        FirePluginError(thisContent, pluginState);
      }
      return NS_BINDING_ABORTED;
  }

  if (mFinalListener) {
    mType = newType;
    rv = mFinalListener->OnStartRequest(aRequest, aContext);
    if (NS_FAILED(rv)) {
      LOG(("OBJLC [%p]: mFinalListener->OnStartRequest failed (%08x), falling back\n",
           this, rv));
#ifdef XP_MACOSX
      
      
      if (mContentType.EqualsLiteral("application/x-director")) {
        LOG(("OBJLC [%p]: (ignoring)\n", this));
        rv = NS_OK; 
        return NS_BINDING_ABORTED;
      }
#endif
      Fallback(false);
    } else if (mType == eType_Plugin) {
      nsIObjectFrame* frame = GetExistingFrame(eFlushContent);
      if (frame) {
        
        
        
        frame->TryNotifyContentObjectWrapper();
      }
    }
    return rv;
  }

  LOG(("OBJLC [%p]: Found no listener, falling back\n", this));
  Fallback(false);
  return NS_BINDING_ABORTED;
}

NS_IMETHODIMP
nsObjectLoadingContent::OnStopRequest(nsIRequest *aRequest,
                                      nsISupports *aContext,
                                      nsresult aStatusCode)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  if (aRequest != mChannel) {
    return NS_BINDING_ABORTED;
  }

  mChannel = nsnull;

  if (mFinalListener) {
    mFinalListener->OnStopRequest(aRequest, aContext, aStatusCode);
    mFinalListener = nsnull;
  }

  
  return NS_OK;
}



NS_IMETHODIMP
nsObjectLoadingContent::OnDataAvailable(nsIRequest *aRequest,
                                        nsISupports *aContext,
                                        nsIInputStream *aInputStream,
                                        PRUint32 aOffset, PRUint32 aCount)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  if (aRequest != mChannel) {
    return NS_BINDING_ABORTED;
  }

  if (mFinalListener) {
    return mFinalListener->OnDataAvailable(aRequest, aContext, aInputStream, aOffset, aCount);
  }

  
  return NS_ERROR_UNEXPECTED;
}


NS_IMETHODIMP
nsObjectLoadingContent::GetFrameLoader(nsIFrameLoader** aFrameLoader)
{
  *aFrameLoader = mFrameLoader;
  NS_IF_ADDREF(*aFrameLoader);
  return NS_OK;
}

NS_IMETHODIMP_(already_AddRefed<nsFrameLoader>)
nsObjectLoadingContent::GetFrameLoader()
{
  nsFrameLoader* loader = mFrameLoader;
  NS_IF_ADDREF(loader);
  return loader;
}

NS_IMETHODIMP
nsObjectLoadingContent::SwapFrameLoaders(nsIFrameLoaderOwner* aOtherLoader)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsObjectLoadingContent::GetActualType(nsACString& aType)
{
  aType = mContentType;
  return NS_OK;
}

NS_IMETHODIMP
nsObjectLoadingContent::GetDisplayedType(PRUint32* aType)
{
  *aType = mType;
  return NS_OK;
}


NS_IMETHODIMP
nsObjectLoadingContent::EnsureInstantiation(nsNPAPIPluginInstance** aInstance)
{
  
  
  *aInstance = nsnull;

  if (mType != eType_Plugin) {
    return NS_OK;
  }

  nsIObjectFrame* frame = GetExistingFrame(eFlushContent);
  if (frame) {
    
    
    if (mPendingInstantiateEvent) {
      LOG(("OBJLC [%p]: Revoking pending instantiate event\n", this));
      mPendingInstantiateEvent = nsnull;
    }
  } else {
    
    
    if (mInstantiating) {
      return NS_OK;
    }

    
    mInstantiating = true;

    nsCOMPtr<nsIContent> thisContent = 
      do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
    NS_ASSERTION(thisContent, "must be a content");

    nsIDocument* doc = thisContent->GetCurrentDoc();
    if (!doc) {
      
      mInstantiating = false;
      return NS_OK;
    }

    doc->FlushPendingNotifications(Flush_Frames);

    mInstantiating = false;

    frame = GetExistingFrame(eFlushContent);
    if (!frame) {
      return NS_OK;
    }
  }

  nsIFrame *nsiframe = do_QueryFrame(frame);

  if (nsiframe->GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    
    
    
    
    frame = GetExistingFrame(eFlushLayout);

    if (!frame) {
      return NS_OK;
    }

    nsiframe = do_QueryFrame(frame);
  }

  nsWeakFrame weakFrame(nsiframe);

  
  nsresult rv = frame->GetPluginInstance(aInstance);
  if (!*aInstance && weakFrame.IsAlive()) {
    rv = Instantiate(frame, mContentType, mURI);
    if (NS_SUCCEEDED(rv) && weakFrame.IsAlive()) {
      rv = frame->GetPluginInstance(aInstance);
    } else {
      Fallback(true);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsObjectLoadingContent::HasNewFrame(nsIObjectFrame* aFrame)
{
  LOG(("OBJLC [%p]: Got frame %p (mInstantiating=%i)\n", this, aFrame,
       mInstantiating));

  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");
  nsIDocument* doc = thisContent->OwnerDoc();
  if (doc->IsStaticDocument() || doc->IsBeingUsedAsImage()) {
    return NS_OK;
  }

  
  
  mPendingInstantiateEvent = nsnull;

  nsRefPtr<nsNPAPIPluginInstance> instance;
  aFrame->GetPluginInstance(getter_AddRefs(instance));

  if (instance) {
    
    

    return NS_OK;
  }

  if (!mInstantiating && mType == eType_Plugin) {
    
    
    
    

    
    
    nsCOMPtr<nsIPluginDocument> pDoc (do_QueryInterface(GetOurDocument()));
    if (pDoc) {
      bool willHandleInstantiation;
      pDoc->GetWillHandleInstantiation(&willHandleInstantiation);
      if (willHandleInstantiation) {
        return NS_OK;
      }
    }

    nsIFrame* frame = do_QueryFrame(aFrame);
    nsCOMPtr<nsIRunnable> event =
      new nsAsyncInstantiateEvent(this, frame, mContentType, mURI);
    if (!event) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    LOG(("                 dispatching event\n"));
    nsresult rv = NS_DispatchToCurrentThread(event);
    if (NS_FAILED(rv)) {
      NS_ERROR("failed to dispatch nsAsyncInstantiateEvent");
    } else {
      
      
      mPendingInstantiateEvent = event;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsObjectLoadingContent::GetPluginInstance(nsNPAPIPluginInstance** aInstance)
{
  *aInstance = nsnull;

  nsIObjectFrame* objFrame = GetExistingFrame(eDontFlush);
  if (!objFrame) {
    return NS_OK;
  }

  return objFrame->GetPluginInstance(aInstance);
}

NS_IMETHODIMP
nsObjectLoadingContent::GetContentTypeForMIMEType(const nsACString& aMIMEType,
                                                  PRUint32* aType)
{
  *aType = GetTypeOfContent(PromiseFlatCString(aMIMEType));
  return NS_OK;
}


NS_IMETHODIMP
nsObjectLoadingContent::GetInterface(const nsIID & aIID, void **aResult)
{
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    nsIChannelEventSink* sink = this;
    *aResult = sink;
    NS_ADDREF(sink);
    return NS_OK;
  }
  return NS_NOINTERFACE;
}


NS_IMETHODIMP
nsObjectLoadingContent::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                               nsIChannel *aNewChannel,
                                               PRUint32 aFlags,
                                               nsIAsyncVerifyRedirectCallback *cb)
{
  
  
  if (!mChannel || aOldChannel != mChannel) {
    return NS_BINDING_ABORTED;
  }

  mChannel = aNewChannel;
  cb->OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}


nsEventStates
nsObjectLoadingContent::ObjectState() const
{
  switch (mType) {
    case eType_Loading:
      return NS_EVENT_STATE_LOADING;
    case eType_Image:
      return ImageState();
    case eType_Plugin:
    case eType_Document:
      
      
      
      return nsEventStates();
    case eType_Null:
      if (mSuppressed)
        return NS_EVENT_STATE_SUPPRESSED;
      if (mUserDisabled)
        return NS_EVENT_STATE_USERDISABLED;

      
      nsEventStates state = NS_EVENT_STATE_BROKEN;
      switch (mFallbackReason) {
        case ePluginClickToPlay:
          return NS_EVENT_STATE_TYPE_CLICK_TO_PLAY;
        case ePluginDisabled:
          state |= NS_EVENT_STATE_HANDLER_DISABLED;
          break;
        case ePluginBlocklisted:
          state |= NS_EVENT_STATE_HANDLER_BLOCKED;
          break;
        case ePluginCrashed:
          state |= NS_EVENT_STATE_HANDLER_CRASHED;
          break;
        case ePluginUnsupported:
          state |= NS_EVENT_STATE_TYPE_UNSUPPORTED;
          break;
        case ePluginOutdated:
        case ePluginOtherState:
          
          break;
      }
      return state;
  };
  NS_NOTREACHED("unknown type?");
  
  return nsEventStates();
}


nsresult
nsObjectLoadingContent::LoadObject(const nsAString& aURI,
                                   bool aNotify,
                                   const nsCString& aTypeHint,
                                   bool aForceLoad)
{
  LOG(("OBJLC [%p]: Loading object: URI string=<%s> notify=%i type=<%s> forceload=%i\n",
       this, NS_ConvertUTF16toUTF8(aURI).get(), aNotify, aTypeHint.get(), aForceLoad));

  NS_ASSERTION(!mInstantiating, "LoadObject was reentered?");

  
  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");

  nsIDocument* doc = thisContent->OwnerDoc();
  nsCOMPtr<nsIURI> baseURI;
  GetObjectBaseURI(thisContent, getter_AddRefs(baseURI));

  nsCOMPtr<nsIURI> uri;
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(uri),
                                            aURI, doc,
                                            baseURI);
  
  
  if (!uri) {
    Fallback(aNotify);
    return NS_OK;
  }

  NS_TryToSetImmutable(uri);

  return LoadObject(uri, aNotify, aTypeHint, aForceLoad);
}

void
nsObjectLoadingContent::UpdateFallbackState(nsIContent* aContent,
                                            AutoFallback& fallback,
                                            const nsCString& aTypeHint)
{
  
  PluginSupportState state = GetPluginSupportState(aContent, aTypeHint);
  if (state != ePluginOtherState) {
    fallback.SetPluginState(state);
    FirePluginError(aContent, state);
  }
}

nsresult
nsObjectLoadingContent::LoadObject(nsIURI* aURI,
                                   bool aNotify,
                                   const nsCString& aTypeHint,
                                   bool aForceLoad)
{
  LOG(("OBJLC [%p]: Loading object: URI=<%p> notify=%i type=<%s> forceload=%i\n",
       this, aURI, aNotify, aTypeHint.get(), aForceLoad));

  if (mURI && aURI && !aForceLoad) {
    bool equal;
    nsresult rv = mURI->Equals(aURI, &equal);
    if (NS_SUCCEEDED(rv) && equal) {
      
      return NS_OK;
    }
  }

  
  if (mType == eType_Plugin && mPendingInstantiateEvent) {
    LOG(("OBJLC [%p]: Revoking pending instantiate event\n", this));
    mPendingInstantiateEvent = nsnull;
  }

  AutoNotifier notifier(this, aNotify);

  
  
  
  NS_ASSERTION(!mInstantiating, "LoadObject was reentered?");
  mInstantiating = true;
  AutoSetInstantiatingToFalse autoset(this);

  mUserDisabled = mSuppressed = false;

  mURI = aURI;
  mContentType = aTypeHint;

  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");

  nsIDocument* doc = thisContent->OwnerDoc();
  if (doc->IsBeingUsedAsImage()) {
    return NS_OK;
  }

  
  
  if (mChannel) {
    LOG(("OBJLC [%p]: Cancelling existing load\n", this));

    
    
    
    
    mChannel->Cancel(NS_BINDING_ABORTED);
    if (mFinalListener) {
      
      
      
      mFinalListener->OnStopRequest(mChannel, nsnull, NS_BINDING_ABORTED);
      mFinalListener = nsnull;
    }
    mChannel = nsnull;
  }

  
  if (doc->IsLoadedAsData()) {
    if (!doc->IsStaticDocument()) {
      Fallback(false);
    }
    return NS_OK;
  }

  
  
  
  
  if (aURI) {
    nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
    NS_ASSERTION(secMan, "No security manager!?");
    nsresult rv =
      secMan->CheckLoadURIWithPrincipal(thisContent->NodePrincipal(), aURI, 0);
    if (NS_FAILED(rv)) {
      Fallback(false);
      return NS_OK;
    }

    PRInt16 shouldLoad = nsIContentPolicy::ACCEPT; 
    rv =
      NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OBJECT,
                                aURI,
                                doc->NodePrincipal(),
                                static_cast<nsIImageLoadingContent*>(this),
                                aTypeHint,
                                nsnull, 
                                &shouldLoad,
                                nsContentUtils::GetContentPolicy(),
                                secMan);
    if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
      HandleBeingBlockedByContentPolicy(rv, shouldLoad);
      return NS_OK;
    }
  }

  nsresult rv = NS_ERROR_UNEXPECTED;
  
  
  AutoFallback fallback(this, &rv);

  PRUint32 caps = GetCapabilities();
  LOG(("OBJLC [%p]: Capabilities: %04x\n", this, caps));

  nsCAutoString overrideType;
  if ((caps & eOverrideServerType) &&
      ((!aTypeHint.IsEmpty() && IsSupportedPlugin(aTypeHint)) ||
       (aURI && IsPluginEnabledByExtension(aURI, overrideType)))) {
    ObjectType newType;
    if (overrideType.IsEmpty()) {
      newType = GetTypeOfContent(aTypeHint);
    } else {
      mContentType = overrideType;
      newType = eType_Plugin;
    }

    if (newType != mType) {
      LOG(("OBJLC [%p]: (eOverrideServerType) Changing type from %u to %u\n", this, mType, newType));

      UnloadContent();

      
      
      if (!mFrameLoader && newType == eType_Document) {
        mFrameLoader = nsFrameLoader::Create(thisContent->AsElement(),
                                             mNetworkCreated);
        if (!mFrameLoader) {
          mURI = nsnull;
          return NS_OK;
        }
      }

      
      
      
      
      mType = newType;
      if (aNotify)
        notifier.Notify();
    }
    switch (newType) {
      case eType_Image:
        
        if (aURI) {
          rv = LoadImage(aURI, aForceLoad, false);
        } else {
          rv = NS_ERROR_NOT_AVAILABLE;
        }
        break;
      case eType_Plugin:
        rv = TryInstantiate(mContentType, mURI);
        break;
      case eType_Document:
        if (aURI) {
          rv = mFrameLoader->LoadURI(aURI);
        } else {
          rv = NS_ERROR_NOT_AVAILABLE;
        }
        break;
      case eType_Loading:
        NS_NOTREACHED("Should not have a loading type here!");
      case eType_Null:
        
        UpdateFallbackState(thisContent, fallback, aTypeHint);
        break;
    };
    return NS_OK;
  }

  
  
  bool isSupportedClassID = false;
  nsCAutoString typeForID; 
  bool hasID = false;
  if (caps & eSupportClassID) {
    nsAutoString classid;
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::classid, classid);
    if (!classid.IsEmpty()) {
      hasID = true;
      isSupportedClassID = NS_SUCCEEDED(TypeForClassID(classid, typeForID));
    }
  }

  if (hasID && !isSupportedClassID) {
    
    LOG(("OBJLC [%p]: invalid classid\n", this));
    rv = NS_ERROR_NOT_AVAILABLE;
    return NS_OK;
  }

  if (isSupportedClassID ||
      (!aURI && !aTypeHint.IsEmpty() &&
       GetTypeOfContent(aTypeHint) == eType_Plugin)) {
    
    
    LOG(("OBJLC [%p]: (classid) Changing type from %u to eType_Plugin\n", this, mType));
    mType = eType_Plugin;

    
    
    
    
    
    NS_ASSERTION(mContentType.Equals(aTypeHint), "mContentType wrong!");
    NS_ASSERTION(mURI == aURI, "mURI wrong!");

    if (isSupportedClassID) {
      
      NS_ASSERTION(!typeForID.IsEmpty(), "Must have a real type!");
      mContentType = typeForID;
      
      
      
      GetObjectBaseURI(thisContent, getter_AddRefs(mURI));
      if (!mURI) {
        mURI = aURI;
      }
    }

    rv = TryInstantiate(mContentType, mURI);
    return NS_OK;
  }

  if (!aURI) {
    
    LOG(("OBJLC [%p]: no URI\n", this));
    rv = NS_ERROR_NOT_AVAILABLE;

    
    
    if (!aTypeHint.IsEmpty() && GetTypeOfContent(aTypeHint) == eType_Null) {
      UpdateFallbackState(thisContent, fallback, aTypeHint);
    }

    return NS_OK;
  }

  
  if (!CanHandleURI(aURI)) {
    LOG(("OBJLC [%p]: can't handle URI\n", this));
    if (aTypeHint.IsEmpty()) {
      rv = NS_ERROR_NOT_AVAILABLE;
      return NS_OK;
    }

    if (IsSupportedPlugin(aTypeHint)) {
      mType = eType_Plugin;

      rv = TryInstantiate(aTypeHint, aURI);
    } else {
      rv = NS_ERROR_NOT_AVAILABLE;
      
      UpdateFallbackState(thisContent, fallback, aTypeHint);
    }

    return NS_OK;
  }

  nsCOMPtr<nsILoadGroup> group = doc->GetDocumentLoadGroup();
  nsCOMPtr<nsIChannel> chan;
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = doc->NodePrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);
  if (csp) {
    channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_OBJECT);
  }
  rv = NS_NewChannel(getter_AddRefs(chan), aURI, nsnull, group, this,
                     nsIChannel::LOAD_CALL_CONTENT_SNIFFERS |
                     nsIChannel::LOAD_CLASSIFY_URI,
                     channelPolicy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIHttpChannel> httpChan(do_QueryInterface(chan));
  if (httpChan) {
    httpChan->SetReferrer(doc->GetDocumentURI());
  }

  
  if (!aTypeHint.IsEmpty()) {
    nsCAutoString typeHint, dummy;
    NS_ParseContentType(aTypeHint, typeHint, dummy);
    if (!typeHint.IsEmpty()) {
      chan->SetContentType(typeHint);
    }
  }

  
  nsContentUtils::SetUpChannelOwner(thisContent->NodePrincipal(),
                                    chan, aURI, true);

  nsCOMPtr<nsIScriptChannel> scriptChannel = do_QueryInterface(chan);
  if (scriptChannel) {
    
    scriptChannel->
      SetExecutionPolicy(nsIScriptChannel::EXECUTE_NORMAL);
  }

  
  
  rv = chan->AsyncOpen(this, nsnull);
  if (NS_SUCCEEDED(rv)) {
    LOG(("OBJLC [%p]: Channel opened.\n", this));

    mChannel = chan;
    mType = eType_Loading;
  }
  return NS_OK;
}

PRUint32
nsObjectLoadingContent::GetCapabilities() const
{
  return eSupportImages |
         eSupportPlugins |
         eSupportDocuments |
         eSupportSVG;
}

void
nsObjectLoadingContent::Fallback(bool aNotify)
{
  LOG(("OBJLC [%p]: Falling back (Notify=%i)\n", this, aNotify));

  AutoNotifier notifier(this, aNotify);

  UnloadContent();
}

void
nsObjectLoadingContent::RemovedFromDocument()
{
  LOG(("OBJLC [%p]: Removed from doc\n", this));
  if (mFrameLoader) {
    
    mFrameLoader->Destroy();
    mFrameLoader = nsnull;

    
    
    mURI = nsnull;
  }
}


void
nsObjectLoadingContent::Traverse(nsObjectLoadingContent *tmp,
                                 nsCycleCollectionTraversalCallback &cb)
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mFrameLoader");
  cb.NoteXPCOMChild(static_cast<nsIFrameLoader*>(tmp->mFrameLoader));
}


 bool
nsObjectLoadingContent::IsSuccessfulRequest(nsIRequest* aRequest)
{
  nsresult status;
  nsresult rv = aRequest->GetStatus(&status);
  if (NS_FAILED(rv) || NS_FAILED(status)) {
    return false;
  }

  
  nsCOMPtr<nsIHttpChannel> httpChan(do_QueryInterface(aRequest));
  if (httpChan) {
    bool success;
    rv = httpChan->GetRequestSucceeded(&success);
    if (NS_FAILED(rv) || !success) {
      return false;
    }
  }

  
  return true;
}

 bool
nsObjectLoadingContent::CanHandleURI(nsIURI* aURI)
{
  nsCAutoString scheme;
  if (NS_FAILED(aURI->GetScheme(scheme))) {
    return false;
  }

  nsIIOService* ios = nsContentUtils::GetIOService();
  if (!ios)
    return false;
  
  nsCOMPtr<nsIProtocolHandler> handler;
  ios->GetProtocolHandler(scheme.get(), getter_AddRefs(handler));
  if (!handler) {
    return false;
  }
  
  nsCOMPtr<nsIExternalProtocolHandler> extHandler =
    do_QueryInterface(handler);
  
  return extHandler == nsnull;
}

bool
nsObjectLoadingContent::IsSupportedDocument(const nsCString& aMimeType)
{
  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");

  nsresult rv;
  nsCOMPtr<nsIWebNavigationInfo> info(
    do_GetService(NS_WEBNAVIGATION_INFO_CONTRACTID, &rv));
  PRUint32 supported;
  if (info) {
    nsCOMPtr<nsIWebNavigation> webNav;
    nsIDocument* currentDoc = thisContent->GetCurrentDoc();
    if (currentDoc) {
      webNav = do_GetInterface(currentDoc->GetScriptGlobalObject());
    }
    rv = info->IsTypeSupported(aMimeType, webNav, &supported);
  }

  if (NS_SUCCEEDED(rv)) {
    if (supported == nsIWebNavigationInfo::UNSUPPORTED) {
      
      
      
      
      nsCOMPtr<nsIStreamConverterService> convServ =
        do_GetService("@mozilla.org/streamConverters;1");
      bool canConvert = false;
      if (convServ) {
        rv = convServ->CanConvert(aMimeType.get(), "*/*", &canConvert);
      }

      return NS_SUCCEEDED(rv) && canConvert;
    }

    
    return supported != nsIWebNavigationInfo::PLUGIN;
  }

  return false;
}

void
nsObjectLoadingContent::UnloadContent()
{
  
  CancelImageRequests(false);
  if (mFrameLoader) {
    mFrameLoader->Destroy();
    mFrameLoader = nsnull;
  }
  mType = eType_Null;
  mUserDisabled = mSuppressed = false;
  mFallbackReason = ePluginOtherState;
}

void
nsObjectLoadingContent::NotifyStateChanged(ObjectType aOldType,
                                           nsEventStates aOldState,
                                           bool aSync,
                                           bool aNotify)
{
  LOG(("OBJLC [%p]: Notifying about state change: (%u, %llx) -> (%u, %llx) (sync=%i)\n",
       this, aOldType, aOldState.GetInternalValue(), mType,
       ObjectState().GetInternalValue(), aSync));

  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");

  NS_ASSERTION(thisContent->IsElement(), "Not an element?");

  
  
  
  thisContent->AsElement()->UpdateState(false);

  if (!aNotify) {
    
    return;
  }

  nsIDocument* doc = thisContent->GetCurrentDoc();
  if (!doc) {
    return; 
  }

  nsEventStates newState = ObjectState();

  if (newState != aOldState) {
    
    NS_ASSERTION(thisContent->IsInDoc(), "Something is confused");
    nsEventStates changedBits = aOldState ^ newState;

    {
      nsAutoScriptBlocker scriptBlocker;
      doc->ContentStateChanged(thisContent, changedBits);
    }
    if (aSync) {
      
      doc->FlushPendingNotifications(Flush_Frames);
    }
  } else if (aOldType != mType) {
    
    
    nsCOMPtr<nsIPresShell> shell = doc->GetShell();
    if (shell) {
      shell->RecreateFramesFor(thisContent);
    }
  }
}

 void
nsObjectLoadingContent::FirePluginError(nsIContent* thisContent,
                                        PluginSupportState state)
{
  LOG(("OBJLC []: Dispatching nsPluginErrorEvent for content %p\n",
       thisContent));

  nsCOMPtr<nsIRunnable> ev = new nsPluginErrorEvent(thisContent, state);
  nsresult rv = NS_DispatchToCurrentThread(ev);
  if (NS_FAILED(rv)) {
    NS_WARNING("failed to dispatch nsPluginErrorEvent");
  }
}

nsObjectLoadingContent::ObjectType
nsObjectLoadingContent::GetTypeOfContent(const nsCString& aMIMEType)
{
  PRUint32 caps = GetCapabilities();

  if ((caps & eSupportImages) && IsSupportedImage(aMIMEType)) {
    return eType_Image;
  }

  bool isSVG = aMIMEType.LowerCaseEqualsLiteral("image/svg+xml");
  bool supportedSVG = isSVG && (caps & eSupportSVG);
  if (((caps & eSupportDocuments) || supportedSVG) &&
      IsSupportedDocument(aMIMEType)) {
    return eType_Document;
  }

  if ((caps & eSupportPlugins) && IsSupportedPlugin(aMIMEType)) {
    return eType_Plugin;
  }

  return eType_Null;
}

nsresult
nsObjectLoadingContent::TypeForClassID(const nsAString& aClassID,
                                       nsACString& aType)
{
  nsCOMPtr<nsIPluginHost> pluginHostCOM(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID));
  nsPluginHost *pluginHost = static_cast<nsPluginHost*>(pluginHostCOM.get());
  if (!pluginHost) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (StringBeginsWith(aClassID, NS_LITERAL_STRING("java:"))) {
    
    aType.AssignLiteral("application/x-java-vm");
    nsresult rv = pluginHost->IsPluginEnabledForType("application/x-java-vm");
    return NS_SUCCEEDED(rv) ? NS_OK : NS_ERROR_NOT_AVAILABLE;
  }

  
  if (StringBeginsWith(aClassID, NS_LITERAL_STRING("clsid:"), nsCaseInsensitiveStringComparator())) {
    

    if (NS_SUCCEEDED(pluginHost->IsPluginEnabledForType("application/x-oleobject"))) {
      aType.AssignLiteral("application/x-oleobject");
      return NS_OK;
    }
    if (NS_SUCCEEDED(pluginHost->IsPluginEnabledForType("application/oleobject"))) {
      aType.AssignLiteral("application/oleobject");
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

void
nsObjectLoadingContent::GetObjectBaseURI(nsIContent* thisContent, nsIURI** aURI)
{
  
  
  NS_PRECONDITION(*aURI == nsnull, "URI must be inited to zero");

  
  nsCOMPtr<nsIURI> baseURI = thisContent->GetBaseURI();
  nsAutoString codebase;
  thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::codebase,
                       codebase);
  if (!codebase.IsEmpty()) {
    nsContentUtils::NewURIWithDocumentCharset(aURI, codebase,
                                              thisContent->OwnerDoc(),
                                              baseURI);
  } else {
    baseURI.swap(*aURI);
  }
}

nsIObjectFrame*
nsObjectLoadingContent::GetExistingFrame(FlushType aFlushType)
{
  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");

  nsIFrame* frame;
  do {
    frame = thisContent->GetPrimaryFrame();
    if (!frame) {
      return nsnull;
    }

    if (aFlushType == eDontFlush) {
      break;
    }
    
    
    
    nsIDocument* doc = thisContent->GetCurrentDoc();
    NS_ASSERTION(doc, "Frame but no document?");
    mozFlushType flushType =
      aFlushType == eFlushLayout ? Flush_Layout : Flush_ContentAndNotify;
    doc->FlushPendingNotifications(flushType);

    aFlushType = eDontFlush;
  } while (1);

  nsIObjectFrame* objFrame = do_QueryFrame(frame);
  return objFrame;
}

void
nsObjectLoadingContent::HandleBeingBlockedByContentPolicy(nsresult aStatus,
                                                          PRInt16 aRetval)
{
  
  
  
  UnloadContent();
  if (NS_SUCCEEDED(aStatus)) {
    if (aRetval == nsIContentPolicy::REJECT_TYPE) {
      mUserDisabled = true;
    } else if (aRetval == nsIContentPolicy::REJECT_SERVER) {
      mSuppressed = true;
    }
  }
}

nsresult
nsObjectLoadingContent::TryInstantiate(const nsACString& aMIMEType,
                                       nsIURI* aURI)
{
  nsIObjectFrame* frame = GetExistingFrame(eFlushContent);
  if (!frame) {
    LOG(("OBJLC [%p]: No frame yet\n", this));
    return NS_OK; 
  }

  nsRefPtr<nsNPAPIPluginInstance> instance;
  frame->GetPluginInstance(getter_AddRefs(instance));

  if (!instance) {
    
    
    
    
    
    
    
    nsIFrame* iframe = do_QueryFrame(frame);
    if (iframe->GetStateBits() & NS_FRAME_FIRST_REFLOW) {
      LOG(("OBJLC [%p]: Frame hasn't been reflowed yet\n", this));
      return NS_OK; 
    }
  }

  return Instantiate(frame, aMIMEType, aURI);
}

nsresult
nsObjectLoadingContent::Instantiate(nsIObjectFrame* aFrame,
                                    const nsACString& aMIMEType,
                                    nsIURI* aURI)
{
  NS_ASSERTION(aFrame, "Must have a frame here");

  
  
  mPendingInstantiateEvent = nsnull;

  
  
  bool oldInstantiatingValue = mInstantiating;
  mInstantiating = true;

  nsCString typeToUse(aMIMEType);
  if (typeToUse.IsEmpty() && aURI) {
    IsPluginEnabledByExtension(aURI, typeToUse);
  }

  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");
  
  nsCOMPtr<nsIURI> baseURI;
  if (!aURI) {
    
    
    GetObjectBaseURI(thisContent, getter_AddRefs(baseURI));
    aURI = baseURI;
  }

  nsIFrame *nsiframe = do_QueryFrame(aFrame);
  nsWeakFrame weakFrame(nsiframe);

  
  NS_ASSERTION(aURI || !typeToUse.IsEmpty(), "Need a URI or a type");
  LOG(("OBJLC [%p]: Calling [%p]->Instantiate(<%s>, %p)\n", this, aFrame,
       typeToUse.get(), aURI));
  nsresult rv = aFrame->Instantiate(typeToUse.get(), aURI);

  mInstantiating = oldInstantiatingValue;

  nsRefPtr<nsNPAPIPluginInstance> pluginInstance;
  if (weakFrame.IsAlive()) {
    aFrame->GetPluginInstance(getter_AddRefs(pluginInstance));
  }
  if (pluginInstance) {
    nsCOMPtr<nsIPluginTag> pluginTag;
    nsCOMPtr<nsIPluginHost> host(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID));
    static_cast<nsPluginHost*>(host.get())->
      GetPluginTagForInstance(pluginInstance, getter_AddRefs(pluginTag));

    nsCOMPtr<nsIBlocklistService> blocklist =
      do_GetService("@mozilla.org/extensions/blocklist;1");
    if (blocklist) {
      PRUint32 blockState = nsIBlocklistService::STATE_NOT_BLOCKED;
      blocklist->GetPluginBlocklistState(pluginTag, EmptyString(),
                                         EmptyString(), &blockState);
      if (blockState == nsIBlocklistService::STATE_OUTDATED)
        FirePluginError(thisContent, ePluginOutdated);
    }
  }

  return rv;
}

 PluginSupportState
nsObjectLoadingContent::GetPluginSupportState(nsIContent* aContent,
                                              const nsCString& aContentType)
{
  if (!aContent->IsHTML()) {
    return ePluginOtherState;
  }

  if (aContent->Tag() == nsGkAtoms::embed ||
      aContent->Tag() == nsGkAtoms::applet) {
    return GetPluginDisabledState(aContentType);
  }

  bool hasAlternateContent = false;

  
  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsHTML(nsGkAtoms::param)) {
      if (child->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                             NS_LITERAL_STRING("pluginurl"), eIgnoreCase)) {
        return GetPluginDisabledState(aContentType);
      }
    } else if (!hasAlternateContent) {
      hasAlternateContent =
        nsStyleUtil::IsSignificantChild(child, true, false);
    }
  }

  return hasAlternateContent ? ePluginOtherState :
    GetPluginDisabledState(aContentType);
}

 PluginSupportState
nsObjectLoadingContent::GetPluginDisabledState(const nsCString& aContentType)
{
#ifdef ANDROID
  
  if (!mozilla::Preferences::GetBool("plugin.disable", false) && 
      XRE_GetProcessType() == GeckoProcessType_Content)
    return ePluginClickToPlay;
#endif
  nsCOMPtr<nsIPluginHost> pluginHostCOM(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID));
  nsPluginHost *pluginHost = static_cast<nsPluginHost*>(pluginHostCOM.get());
  if (!pluginHost) {
    return ePluginUnsupported;
  }

  nsresult rv = pluginHost->IsPluginEnabledForType(aContentType.get());
  if (rv == NS_ERROR_PLUGIN_DISABLED)
    return ePluginDisabled;
  if (rv == NS_ERROR_PLUGIN_BLOCKLISTED)
    return ePluginBlocklisted;
  return ePluginUnsupported;
}

void
nsObjectLoadingContent::CreateStaticClone(nsObjectLoadingContent* aDest) const
{
  nsImageLoadingContent::CreateStaticImageClone(aDest);

  aDest->mType = mType;
  nsObjectLoadingContent* thisObj = const_cast<nsObjectLoadingContent*>(this);
  if (thisObj->mPrintFrame.IsAlive()) {
    aDest->mPrintFrame = thisObj->mPrintFrame;
  } else {
    nsIObjectFrame* frame =
      const_cast<nsObjectLoadingContent*>(this)->GetExistingFrame(eDontFlush);
    nsIFrame* f = do_QueryFrame(frame);
    aDest->mPrintFrame = f;
  }

  if (mFrameLoader) {
    nsCOMPtr<nsIContent> content =
      do_QueryInterface(static_cast<nsIImageLoadingContent*>(aDest));
    nsFrameLoader* fl = nsFrameLoader::Create(content->AsElement(), false);
    if (fl) {
      aDest->mFrameLoader = fl;
      mFrameLoader->CreateStaticClone(fl);
    }
  }
}

NS_IMETHODIMP
nsObjectLoadingContent::GetPrintFrame(nsIFrame** aFrame)
{
  *aFrame = mPrintFrame.GetFrame();
  return NS_OK;
}

NS_IMETHODIMP
nsObjectLoadingContent::PluginCrashed(nsIPluginTag* aPluginTag,
                                      const nsAString& pluginDumpID,
                                      const nsAString& browserDumpID,
                                      bool submittedCrashReport)
{
  AutoNotifier notifier(this, true);
  UnloadContent();
  mFallbackReason = ePluginCrashed;
  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));

  
  
  nsCAutoString pluginName;
  aPluginTag->GetName(pluginName);
  nsCAutoString pluginFilename;
  aPluginTag->GetFilename(pluginFilename);

  nsCOMPtr<nsIRunnable> ev = new nsPluginCrashedEvent(thisContent,
                                                      pluginDumpID,
                                                      browserDumpID,
                                                      NS_ConvertUTF8toUTF16(pluginName),
                                                      NS_ConvertUTF8toUTF16(pluginFilename),
                                                      submittedCrashReport);
  nsresult rv = NS_DispatchToCurrentThread(ev);
  if (NS_FAILED(rv)) {
    NS_WARNING("failed to dispatch nsPluginCrashedEvent");
  }
  return NS_OK;
}
