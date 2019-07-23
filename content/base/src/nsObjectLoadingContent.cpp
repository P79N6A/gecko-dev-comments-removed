












































#include "imgILoader.h"
#include "nsIContent.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIExternalProtocolHandler.h"
#include "nsIEventStateManager.h"
#include "nsIObjectFrame.h"
#include "nsIPluginDocument.h"
#include "nsIPluginHost.h"
#include "nsIPresShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsIStreamConverterService.h"
#include "nsIURILoader.h"
#include "nsIURL.h"
#include "nsIWebNavigation.h"
#include "nsIWebNavigationInfo.h"

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
#include "nsPresShellIterator.h"


#include "nsFrameLoader.h"

#include "nsObjectLoadingContent.h"

static NS_DEFINE_CID(kCPluginManagerCID, NS_PLUGINMANAGER_CID);

#ifdef PR_LOGGING
static PRLogModuleInfo* gObjectLog = PR_NewLogModule("objlc");
#endif

#define LOG(args) PR_LOG(gObjectLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gObjectLog, PR_LOG_DEBUG)

class nsAsyncInstantiateEvent : public nsRunnable {
public:
  
  
  nsObjectLoadingContent *mContent;
  nsIObjectFrame*         mFrame;
  nsCString               mContentType;
  nsCOMPtr<nsIURI>        mURI;

  nsAsyncInstantiateEvent(nsObjectLoadingContent* aContent,
                          nsIObjectFrame* aFrame,
                          const nsCString& aType,
                          nsIURI* aURI)
    : mContent(aContent), mFrame(aFrame), mContentType(aType), mURI(aURI)
  {
    NS_STATIC_CAST(nsIObjectLoadingContent *, mContent)->AddRef();
  }

  ~nsAsyncInstantiateEvent()
  {
    NS_STATIC_CAST(nsIObjectLoadingContent *, mContent)->Release();
  }

  NS_IMETHOD Run();
};

NS_IMETHODIMP
nsAsyncInstantiateEvent::Run()
{
  
  if (mContent->mPendingInstantiateEvent != this)
    return NS_OK;
  mContent->mPendingInstantiateEvent = nsnull;

  
  
  
  
  if (mContent->GetFrame() == mFrame &&
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

    nsresult rv = mContent->Instantiate(mContentType, mURI);
    if (NS_FAILED(rv)) {
      mContent->Fallback(PR_TRUE);
    }
  } else {
    LOG(("OBJLC [%p]: Discarding event, data changed\n", mContent));
  }

  return NS_OK;
}




class nsPluginNotFoundEvent : public nsRunnable {
public:
  nsCOMPtr<nsIContent> mContent;

  nsPluginNotFoundEvent(nsIContent* aContent)
    : mContent(aContent)
  {}

  ~nsPluginNotFoundEvent() {}

  NS_IMETHOD Run();
};

NS_IMETHODIMP
nsPluginNotFoundEvent::Run()
{
  LOG(("OBJLC []: Firing plugin not found event for content %p\n",
       mContent.get()));
  nsContentUtils::DispatchTrustedEvent(mContent->GetDocument(), mContent,
                                       NS_LITERAL_STRING("PluginNotFound"),
                                       PR_TRUE, PR_TRUE);
  return NS_OK;
}

class AutoNotifier {
  public:
    AutoNotifier(nsObjectLoadingContent* aContent, PRBool aNotify) :
      mContent(aContent), mNotify(aNotify) {
        mOldType = aContent->Type();
        mOldState = aContent->ObjectState();
    }
    ~AutoNotifier() {
      if (mNotify) {
        mContent->NotifyStateChanged(mOldType, mOldState, PR_FALSE);
      }
    }

    




    void Notify() {
      NS_ASSERTION(mNotify, "Should not notify when notify=false");

      mContent->NotifyStateChanged(mOldType, mOldState, PR_TRUE);
      mOldType = mContent->Type();
      mOldState = mContent->ObjectState();
    }

  private:
    nsObjectLoadingContent*            mContent;
    PRBool                             mNotify;
    nsObjectLoadingContent::ObjectType mOldType;
    PRInt32                            mOldState;
};





class AutoFallback {
  public:
    AutoFallback(nsObjectLoadingContent* aContent, const nsresult* rv)
      : mContent(aContent), mResult(rv), mTypeUnsupported(PR_FALSE) {}
    ~AutoFallback() {
      if (NS_FAILED(*mResult)) {
        LOG(("OBJLC [%p]: rv=%08x, falling back\n", mContent, *mResult));
        mContent->Fallback(PR_FALSE);
        if (mTypeUnsupported) {
          mContent->mTypeUnsupported = PR_TRUE;
        }
      }
    }

    



    void TypeUnsupported() {
      mTypeUnsupported = PR_TRUE;
    }
  private:
    nsObjectLoadingContent* mContent;
    const nsresult* mResult;
    PRBool mTypeUnsupported;
};





class AutoSetInstantiatingToFalse {
  public:
    AutoSetInstantiatingToFalse(nsObjectLoadingContent* objlc) : mContent(objlc) {}
    ~AutoSetInstantiatingToFalse() { mContent->mInstantiating = PR_FALSE; }
  private:
    nsObjectLoadingContent* mContent;
};


static PRBool
IsSupportedImage(const nsCString& aMimeType)
{
  imgILoader* loader = nsContentUtils::GetImgLoader();
  if (!loader) {
    return PR_FALSE;
  }

  PRBool supported;
  nsresult rv = loader->SupportImageWithMimeType(aMimeType.get(), &supported);
  return NS_SUCCEEDED(rv) && supported;
}

static PRBool
IsSupportedPlugin(const nsCString& aMIMEType)
{
  nsCOMPtr<nsIPluginHost> host(do_GetService("@mozilla.org/plugin/host;1"));
  if (!host) {
    return PR_FALSE;
  }
  nsresult rv = host->IsPluginEnabledForType(aMIMEType.get());
  
  return NS_SUCCEEDED(rv);
}

nsObjectLoadingContent::nsObjectLoadingContent()
  : mPendingInstantiateEvent(nsnull)
  , mChannel(nsnull)
  , mType(eType_Loading)
  , mInstantiating(PR_FALSE)
  , mUserDisabled(PR_FALSE)
  , mSuppressed(PR_FALSE)
  , mTypeUnsupported(PR_FALSE)
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
nsObjectLoadingContent::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  if (aRequest != mChannel) {
    
    
    return NS_BINDING_ABORTED;
  }

  AutoNotifier notifier(this, PR_TRUE);

  if (!IsSuccessfulRequest(aRequest)) {
    LOG(("OBJLC [%p]: OnStartRequest: Request failed\n", this));
    Fallback(PR_FALSE);
    return NS_BINDING_ABORTED;
  }

  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  NS_ASSERTION(chan, "Why is our request not a channel?");

  nsresult rv = NS_ERROR_UNEXPECTED;
  
  
  AutoFallback fallback(this, &rv);

  rv = chan->GetContentType(mContentType);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  ObjectType newType = GetTypeOfContent(mContentType);
  LOG(("OBJLC [%p]: OnStartRequest: Content Type=<%s> Old type=%u New Type=%u\n",
       this, mContentType.get(), mType, newType));
  if (mType != newType) {
    UnloadContent();
  }

  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(NS_STATIC_CAST(nsIImageLoadingContent*, this));
  NS_ASSERTION(thisContent, "must be a content");
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
        if (!thisContent->IsInDoc()) {
          
          Fallback(PR_FALSE);
          return NS_ERROR_UNEXPECTED;
        }
        mFrameLoader = new nsFrameLoader(thisContent);
        if (!mFrameLoader) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }

      if (mType != newType) {
        
        
        mType = newType;
        notifier.Notify();
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
      mInstantiating = PR_TRUE;
      if (mType != newType) {
        
        mType = newType;
        notifier.Notify();
      }
      nsIObjectFrame* frame;
      frame = GetFrame();
      if (!frame) {
        
        
        
        mInstantiating = PR_FALSE;
        return NS_BINDING_ABORTED;
      }
      rv = frame->Instantiate(chan, getter_AddRefs(mFinalListener));
      mInstantiating = PR_FALSE;
      break;
    case eType_Loading:
      NS_NOTREACHED("Should not have a loading type here!");
    case eType_Null:
      LOG(("OBJLC [%p]: Unsupported type, falling back\n", this));
      
      
      
      Fallback(PR_FALSE);

      PluginSupportState pluginState = GetPluginSupportState(thisContent,
                                                             mContentType);
      
      if (pluginState == ePluginUnsupported) {
        FirePluginNotFound(thisContent);
      }
      if (pluginState != ePluginDisabled) {
        mTypeUnsupported = PR_TRUE;
      }
      return NS_BINDING_ABORTED;
  }

  if (mFinalListener) {
    mType = newType;
    rv = mFinalListener->OnStartRequest(aRequest, aContext);
    if (NS_FAILED(rv)) {
      LOG(("OBJLC [%p]: mFinalListener->OnStartRequest failed (%08x), falling back\n",
           this, rv));
      Fallback(PR_FALSE);
    }
    return rv;
  }

  LOG(("OBJLC [%p]: Found no listener, falling back\n", this));
  Fallback(PR_FALSE);
  return NS_BINDING_ABORTED;
}

NS_IMETHODIMP
nsObjectLoadingContent::OnStopRequest(nsIRequest *aRequest,
                                      nsISupports *aContext,
                                      nsresult aStatusCode)
{
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
nsObjectLoadingContent::OnDataAvailable(nsIRequest *aRequest, nsISupports *aContext, nsIInputStream *aInputStream, PRUint32 aOffset, PRUint32 aCount)
{
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
nsObjectLoadingContent::EnsureInstantiation(nsIPluginInstance** aInstance)
{
  
  
  *aInstance = nsnull;

  if (mType != eType_Plugin) {
    return NS_OK;
  }

  nsIObjectFrame* frame = GetFrame();
  if (frame) {
    
    
    if (mPendingInstantiateEvent) {
      LOG(("OBJLC [%p]: Revoking pending instantiate event\n", this));
      mPendingInstantiateEvent = nsnull;
    }
  } else {
    
    
    if (mInstantiating) {
      return NS_OK;
    }

    
    mInstantiating = PR_TRUE;

    nsCOMPtr<nsIContent> thisContent = 
      do_QueryInterface(NS_STATIC_CAST(nsIImageLoadingContent*, this));
    NS_ASSERTION(thisContent, "must be a content");

    nsIDocument* doc = thisContent->GetCurrentDoc();
    if (!doc) {
      
      return NS_OK;
    }

    nsPresShellIterator iter(doc);
    nsCOMPtr<nsIPresShell> shell;
    while ((shell = iter.GetNextShell())) {
      shell->RecreateFramesFor(thisContent);
    }

    mInstantiating = PR_FALSE;

    frame = GetFrame();
    if (!frame) {
      return NS_OK;
    }
  }

  
  nsresult rv = frame->GetPluginInstance(*aInstance);
  if (!*aInstance) {
    rv = Instantiate(mContentType, mURI);
    if (NS_SUCCEEDED(rv)) {
      rv = frame->GetPluginInstance(*aInstance);
    } else {
      Fallback(PR_TRUE);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsObjectLoadingContent::HasNewFrame(nsIObjectFrame* aFrame)
{
  LOG(("OBJLC [%p]: Got frame %p (mInstantiating=%i)\n", this, aFrame,
       mInstantiating));
  if (!mInstantiating && aFrame && mType == eType_Plugin) {
    
    
    
    

    
    mPendingInstantiateEvent = nsnull;

    
    
    nsCOMPtr<nsIPluginDocument> pDoc (do_QueryInterface(GetOurDocument()));
    if (pDoc) {
      return NS_OK;
    }

    nsCOMPtr<nsIRunnable> event =
        new nsAsyncInstantiateEvent(this, aFrame, mContentType, mURI);
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
nsObjectLoadingContent::OnChannelRedirect(nsIChannel *aOldChannel,
                                          nsIChannel *aNewChannel,
                                          PRUint32    aFlags)
{
  
  if (aOldChannel != mChannel) {
    return NS_BINDING_ABORTED;
  }

  mChannel = aNewChannel;
  return NS_OK;
}


PRInt32
nsObjectLoadingContent::ObjectState() const
{
  switch (mType) {
    case eType_Loading:
      return NS_EVENT_STATE_LOADING;
    case eType_Image:
      return ImageState();
    case eType_Plugin:
    case eType_Document:
      
      
      
      return 0;
    case eType_Null:
      if (mSuppressed)
        return NS_EVENT_STATE_SUPPRESSED;
      if (mUserDisabled)
        return NS_EVENT_STATE_USERDISABLED;

      
      PRInt32 state = NS_EVENT_STATE_BROKEN;
      if (mTypeUnsupported) {
        state |= NS_EVENT_STATE_TYPE_UNSUPPORTED;
      }
      return state;
  };
  NS_NOTREACHED("unknown type?");
  
  return 0;
}


nsresult
nsObjectLoadingContent::LoadObject(const nsAString& aURI,
                                   PRBool aNotify,
                                   const nsCString& aTypeHint,
                                   PRBool aForceLoad)
{
  LOG(("OBJLC [%p]: Loading object: URI string=<%s> notify=%i type=<%s> forceload=%i\n",
       this, NS_ConvertUTF16toUTF8(aURI).get(), aNotify, aTypeHint.get(), aForceLoad));

  NS_ASSERTION(!mInstantiating, "LoadObject was reentered?");

  
  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(NS_STATIC_CAST(nsIImageLoadingContent*, this));
  NS_ASSERTION(thisContent, "must be a content");

  nsIDocument* doc = thisContent->GetOwnerDoc();
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

nsresult
nsObjectLoadingContent::LoadObject(nsIURI* aURI,
                                   PRBool aNotify,
                                   const nsCString& aTypeHint,
                                   PRBool aForceLoad)
{
  LOG(("OBJLC [%p]: Loading object: URI=<%p> notify=%i type=<%s> forceload=%i\n",
       this, aURI, aNotify, aTypeHint.get(), aForceLoad));

  if (mURI && aURI && !aForceLoad) {
    PRBool equal;
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
  mInstantiating = PR_TRUE;
  AutoSetInstantiatingToFalse autoset(this);

  mUserDisabled = mSuppressed = PR_FALSE;

  mURI = aURI;
  mContentType = aTypeHint;

  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(NS_STATIC_CAST(nsIImageLoadingContent*, this));
  NS_ASSERTION(thisContent, "must be a content");

  nsIDocument* doc = thisContent->GetOwnerDoc();
  if (!doc) {
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

  
  
  
  
  
  if (aURI) {
    nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
    NS_ASSERTION(secMan, "No security manager!?");
    nsresult rv =
      secMan->CheckLoadURIWithPrincipal(thisContent->NodePrincipal(), aURI, 0);
    if (NS_FAILED(rv)) {
      Fallback(PR_FALSE);
      return NS_OK;
    }

    PRInt16 shouldLoad = nsIContentPolicy::ACCEPT; 
    rv =
      NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OBJECT,
                                aURI,
                                doc->GetDocumentURI(),
                                NS_STATIC_CAST(nsIImageLoadingContent*, this),
                                aTypeHint,
                                nsnull, 
                                &shouldLoad,
                                nsContentUtils::GetContentPolicy());
    if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
      
      
      
      UnloadContent();
      if (NS_SUCCEEDED(rv)) {
        if (shouldLoad == nsIContentPolicy::REJECT_TYPE) {
          mUserDisabled = PR_TRUE;
        } else if (shouldLoad == nsIContentPolicy::REJECT_SERVER) {
          mSuppressed = PR_TRUE;
        }
      }
      return NS_OK;
    }
  }

  nsresult rv = NS_ERROR_UNEXPECTED;
  
  
  AutoFallback fallback(this, &rv);

  PRUint32 caps = GetCapabilities();
  LOG(("OBJLC [%p]: Capabilities: %04x\n", this, caps));

  if ((caps & eOverrideServerType) && !aTypeHint.IsEmpty()) {
    ObjectType newType = GetTypeOfContent(aTypeHint);
    if (newType != mType) {
      LOG(("OBJLC [%p]: (eOverrideServerType) Changing type from %u to %u\n", this, mType, newType));

      UnloadContent();

      
      
      if (!mFrameLoader && newType == eType_Document) {
        if (!thisContent->IsInDoc()) {
          
          mURI = nsnull;
          return NS_OK;
        }

        mFrameLoader = new nsFrameLoader(thisContent);
        if (!mFrameLoader) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }

      
      
      
      mType = newType;
      if (aNotify)
        notifier.Notify();
    }
    switch (newType) {
      case eType_Image:
        
        rv = LoadImage(aURI, aForceLoad, PR_FALSE);
        break;
      case eType_Plugin:
        rv = Instantiate(aTypeHint, aURI);
        break;
      case eType_Document:
        rv = mFrameLoader->LoadURI(aURI);
        break;
      case eType_Loading:
        NS_NOTREACHED("Should not have a loading type here!");
      case eType_Null:
        
        PluginSupportState pluginState = GetPluginSupportState(thisContent,
                                                               aTypeHint);
        if (pluginState == ePluginUnsupported) {
          FirePluginNotFound(thisContent);
        }
        if (pluginState != ePluginDisabled) {
          fallback.TypeUnsupported();
        }

        break;
    };
    return NS_OK;
  }

  
  
  PRBool isSupportedClassID = PR_FALSE;
  nsCAutoString typeForID; 
  PRBool hasID = PR_FALSE;
  if (caps & eSupportClassID) {
    nsAutoString classid;
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::classid, classid);
    if (!classid.IsEmpty()) {
      hasID = PR_TRUE;
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
    if (aNotify)
      notifier.Notify();

    
    
    
    
    
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

    rv = Instantiate(mContentType, mURI);
    return NS_OK;
  }

  if (!aURI) {
    
    LOG(("OBJLC [%p]: no URI\n", this));
    rv = NS_ERROR_NOT_AVAILABLE;
    return NS_OK;
  }

  if (!CanHandleURI(aURI)) {
    LOG(("OBJLC [%p]: can't handle URI\n", this));
    
    mType = eType_Plugin;
    if (aNotify)
      notifier.Notify();

    rv = Instantiate(aTypeHint, aURI);
    return NS_OK;
  }

  nsCOMPtr<nsILoadGroup> group = doc->GetDocumentLoadGroup();
  nsCOMPtr<nsIChannel> chan;
  rv = NS_NewChannel(getter_AddRefs(chan), aURI, nsnull, group, this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIHttpChannel> httpChan(do_QueryInterface(chan));
  if (httpChan) {
    httpChan->SetReferrer(doc->GetDocumentURI());
  }

  
  if (!aTypeHint.IsEmpty()) {
    chan->SetContentType(aTypeHint);
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
         eSupportDocuments
#ifdef MOZ_SVG
         | eSupportSVG
#endif
         ;
}

void
nsObjectLoadingContent::Fallback(PRBool aNotify)
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
nsObjectLoadingContent::Traverse(nsCycleCollectionTraversalCallback &cb)
{
  cb.NoteXPCOMChild(mFrameLoader);
}


 PRBool
nsObjectLoadingContent::IsSuccessfulRequest(nsIRequest* aRequest)
{
  nsresult status;
  nsresult rv = aRequest->GetStatus(&status);
  if (NS_FAILED(rv) || NS_FAILED(status)) {
    return PR_FALSE;
  }

  
  nsCOMPtr<nsIHttpChannel> httpChan(do_QueryInterface(aRequest));
  if (httpChan) {
    PRBool success;
    rv = httpChan->GetRequestSucceeded(&success);
    if (NS_FAILED(rv) || !success) {
      return PR_FALSE;
    }
  }

  
  return PR_TRUE;
}

 PRBool
nsObjectLoadingContent::CanHandleURI(nsIURI* aURI)
{
  nsCAutoString scheme;
  if (NS_FAILED(aURI->GetScheme(scheme))) {
    return PR_FALSE;
  }

  nsIIOService* ios = nsContentUtils::GetIOService();
  if (!ios)
    return PR_FALSE;
  
  nsCOMPtr<nsIProtocolHandler> handler;
  ios->GetProtocolHandler(scheme.get(), getter_AddRefs(handler));
  if (!handler) {
    return PR_FALSE;
  }
  
  nsCOMPtr<nsIExternalProtocolHandler> extHandler =
    do_QueryInterface(handler);
  
  return extHandler == nsnull;
}

PRBool
nsObjectLoadingContent::IsSupportedDocument(const nsCString& aMimeType)
{
  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(NS_STATIC_CAST(nsIImageLoadingContent*, this));
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
      PRBool canConvert = PR_FALSE;
      if (convServ) {
        rv = convServ->CanConvert(aMimeType.get(), "*/*", &canConvert);
      }

      return NS_SUCCEEDED(rv) && canConvert;
    }

    
    return supported != nsIWebNavigationInfo::PLUGIN;
  }

  return PR_FALSE;
}

void
nsObjectLoadingContent::UnloadContent()
{
  
  CancelImageRequests(PR_FALSE);
  if (mFrameLoader) {
    mFrameLoader->Destroy();
    mFrameLoader = nsnull;
  }
  mType = eType_Null;
  mUserDisabled = mSuppressed = mTypeUnsupported = PR_FALSE;
}

void
nsObjectLoadingContent::NotifyStateChanged(ObjectType aOldType,
                                          PRInt32 aOldState,
                                          PRBool aSync)
{
  LOG(("OBJLC [%p]: Notifying about state change: (%u, %x) -> (%u, %x) (sync=%i)\n",
       this, aOldType, aOldState, mType, ObjectState(), aSync));

  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(NS_STATIC_CAST(nsIImageLoadingContent*, this));
  NS_ASSERTION(thisContent, "must be a content");

  nsIDocument* doc = thisContent->GetCurrentDoc();
  if (!doc) {
    return; 
  }

  PRInt32 newState = ObjectState();

  if (newState != aOldState) {
    
    NS_ASSERTION(thisContent->IsInDoc(), "Something is confused");
    PRInt32 changedBits = aOldState ^ newState;

    {
      mozAutoDocUpdate upd(doc, UPDATE_CONTENT_STATE, PR_TRUE);
      doc->ContentStatesChanged(thisContent, nsnull, changedBits);
    }
    if (aSync) {
      
      
      doc->FlushPendingNotifications(Flush_Frames);
    }
  } else if (aOldType != mType) {
    
    

    nsPresShellIterator iter(doc);
    nsCOMPtr<nsIPresShell> shell;
    while ((shell = iter.GetNextShell())) {
      shell->RecreateFramesFor(thisContent);
    }
  }
}

 void
nsObjectLoadingContent::FirePluginNotFound(nsIContent* thisContent)
{
  LOG(("OBJLC []: Dispatching PluginNotFound event for content %p\n",
       thisContent));

  nsCOMPtr<nsIRunnable> ev = new nsPluginNotFoundEvent(thisContent);
  nsresult rv = NS_DispatchToCurrentThread(ev);
  if (NS_FAILED(rv)) {
    NS_WARNING("failed to dispatch nsPluginNotFoundEvent");
  }
}

nsObjectLoadingContent::ObjectType
nsObjectLoadingContent::GetTypeOfContent(const nsCString& aMIMEType)
{
  PRUint32 caps = GetCapabilities();

  if ((caps & eSupportImages) && IsSupportedImage(aMIMEType)) {
    return eType_Image;
  }

#ifdef MOZ_SVG
  PRBool isSVG = aMIMEType.LowerCaseEqualsLiteral("image/svg+xml");
  PRBool supportedSVG = isSVG && (caps & eSupportSVG);
#else
  PRBool supportedSVG = PR_FALSE;
#endif
  if (((caps & eSupportDocuments) || supportedSVG) &&
      IsSupportedDocument(aMIMEType)) {
    return eType_Document;
  }

  if ((caps & eSupportPlugins) && IsSupportedPlugin(aMIMEType)) {
    return eType_Plugin;
  }

  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(NS_STATIC_CAST(nsIImageLoadingContent*, this));
  NS_ASSERTION(thisContent, "must be a content");

  if (ShouldShowDefaultPlugin(thisContent, aMIMEType)) {
    return eType_Plugin;
  }

  return eType_Null;
}

nsresult
nsObjectLoadingContent::TypeForClassID(const nsAString& aClassID,
                                       nsACString& aType)
{
  
  nsCOMPtr<nsIPluginHost> pluginHost(do_GetService(kCPluginManagerCID));
  if (!pluginHost) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (StringBeginsWith(aClassID, NS_LITERAL_STRING("java:"))) {
    
    aType.AssignLiteral("application/x-java-vm");
    nsresult rv = pluginHost->IsPluginEnabledForType("application/x-java-vm");
    return NS_SUCCEEDED(rv) ? NS_OK : NS_ERROR_NOT_AVAILABLE;
  }

  
  if (StringBeginsWith(aClassID, NS_LITERAL_STRING("clsid:"))) {
    

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
                                              thisContent->GetOwnerDoc(),
                                              baseURI);
  } else {
    baseURI.swap(*aURI);
  }
}

nsIObjectFrame*
nsObjectLoadingContent::GetFrame()
{
  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(NS_STATIC_CAST(nsIImageLoadingContent*, this));
  NS_ASSERTION(thisContent, "must be a content");

  PRBool flushed = PR_FALSE;
  nsIFrame* frame;
  do {
    nsIDocument* doc = thisContent->GetCurrentDoc();
    if (!doc) {
      return nsnull; 
    }

    nsIPresShell* shell = doc->GetPrimaryShell();
    if (!shell) {
      return nsnull; 
    }

    frame = shell->GetPrimaryFrameFor(thisContent);
    if (!frame) {
      return nsnull;
    }

    if (flushed) {
      break;
    }
    
    
    
    doc->FlushPendingNotifications(Flush_ContentAndNotify);

    flushed = PR_TRUE;
  } while (1);

  nsIObjectFrame* objFrame;
  CallQueryInterface(frame, &objFrame);
  return objFrame;
}

nsresult
nsObjectLoadingContent::Instantiate(const nsACString& aMIMEType, nsIURI* aURI)
{
  nsIObjectFrame* frame = GetFrame();
  if (!frame) {
    LOG(("OBJLC [%p]: Attempted to instantiate, but have no frame\n", this));
    return NS_OK; 
  }

  nsCString typeToUse(aMIMEType);
  if (typeToUse.IsEmpty() && aURI) {
    nsCAutoString ext;
    
    nsCOMPtr<nsIURL> url(do_QueryInterface(aURI));
    if (url) {
      url->GetFileExtension(ext);
    } else {
      nsCString spec;
      aURI->GetSpec(spec);

      PRInt32 offset = spec.RFindChar('.');
      if (offset != kNotFound) {
        ext = Substring(spec, offset + 1, spec.Length());
      }
    }

    nsCOMPtr<nsIPluginHost> host(do_GetService("@mozilla.org/plugin/host;1"));
    const char* typeFromExt;
    if (host &&
        NS_SUCCEEDED(host->IsPluginEnabledForExtension(ext.get(), typeFromExt))) {
      typeToUse = typeFromExt;
    }
  }

  nsCOMPtr<nsIURI> baseURI;
  if (!aURI) {
    
    
    nsCOMPtr<nsIContent> thisContent = 
      do_QueryInterface(NS_STATIC_CAST(nsIImageLoadingContent*, this));
    NS_ASSERTION(thisContent, "must be a content");

    GetObjectBaseURI(thisContent, getter_AddRefs(baseURI));
    aURI = baseURI;
  }

  
  NS_ASSERTION(aURI || !typeToUse.IsEmpty(), "Need a URI or a type");
  LOG(("OBJLC [%p]: Calling [%p]->Instantiate(<%s>, %p)\n", this, frame,
       typeToUse.get(), aURI));
  return frame->Instantiate(typeToUse.get(), aURI);
}

 PRBool
nsObjectLoadingContent::ShouldShowDefaultPlugin(nsIContent* aContent,
                                                const nsCString& aContentType)
{
  if (nsContentUtils::GetBoolPref("plugin.default_plugin_disabled", PR_FALSE)) {
    return PR_FALSE;
  }

  return GetPluginSupportState(aContent, aContentType) == ePluginUnsupported;
}

 nsObjectLoadingContent::PluginSupportState
nsObjectLoadingContent::GetPluginSupportState(nsIContent* aContent,
                                              const nsCString& aContentType)
{
  if (!aContent->IsNodeOfType(nsINode::eHTML)) {
    return ePluginOtherState;
  }

  if (aContent->Tag() == nsGkAtoms::embed ||
      aContent->Tag() == nsGkAtoms::applet) {
    return GetPluginDisabledState(aContentType);
  }

  
  PRUint32 count = aContent->GetChildCount();
  for (PRUint32 i = 0; i < count; ++i) {
    nsIContent* child = aContent->GetChildAt(i);
    NS_ASSERTION(child, "GetChildCount lied!");

    if (child->IsNodeOfType(nsINode::eHTML) &&
        child->Tag() == nsGkAtoms::param &&
        child->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                           NS_LITERAL_STRING("pluginurl"), eIgnoreCase)) {
      return GetPluginDisabledState(aContentType);
    }
  }
  return ePluginOtherState;
}

 nsObjectLoadingContent::PluginSupportState
nsObjectLoadingContent::GetPluginDisabledState(const nsCString& aContentType)
{
  nsCOMPtr<nsIPluginHost> host(do_GetService("@mozilla.org/plugin/host;1"));
  if (!host) {
    return ePluginUnsupported;
  }
  nsresult rv = host->IsPluginEnabledForType(aContentType.get());
  return rv == NS_ERROR_PLUGIN_DISABLED ? ePluginDisabled : ePluginUnsupported;
}
