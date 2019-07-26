












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
#include "nsIPermissionManager.h"
#include "nsPluginHost.h"
#include "nsIPresShell.h"
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
#include "nsIAppShell.h"

#include "nsError.h"


#include "prenv.h"
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
#include "sampler.h"
#include "nsObjectFrame.h"
#include "nsDOMClassInfo.h"

#include "nsWidgetsCID.h"
#include "nsContentCID.h"
static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#ifdef PR_LOGGING
static PRLogModuleInfo* gObjectLog = PR_NewLogModule("objlc");
#endif

#define LOG(args) PR_LOG(gObjectLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gObjectLog, PR_LOG_DEBUG)

static bool
InActiveDocument(nsIContent *aContent)
{
  if (!aContent->IsInDoc()) {
    return false;
  }
  nsIDocument *doc = aContent->OwnerDoc();
  return (doc && doc->IsActive());
}





class nsAsyncInstantiateEvent : public nsRunnable {
public:
  nsAsyncInstantiateEvent(nsObjectLoadingContent *aContent)
  : mContent(aContent) {}

  ~nsAsyncInstantiateEvent() {}

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIObjectLoadingContent> mContent;
};

NS_IMETHODIMP
nsAsyncInstantiateEvent::Run()
{
  nsObjectLoadingContent *objLC =
    static_cast<nsObjectLoadingContent *>(mContent.get());

  
  if (objLC->mPendingInstantiateEvent != this) {
    return NS_OK;
  }
  objLC->mPendingInstantiateEvent = nullptr;

  return objLC->SyncStartPluginInstance();
}



class InDocCheckEvent : public nsRunnable {
public:
  InDocCheckEvent(nsObjectLoadingContent *aContent)
  : mContent(aContent) {}

  ~InDocCheckEvent() {}

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIObjectLoadingContent> mContent;
};

NS_IMETHODIMP
InDocCheckEvent::Run()
{
  nsObjectLoadingContent *objLC =
    static_cast<nsObjectLoadingContent *>(mContent.get());

  nsCOMPtr<nsIContent> content =
    do_QueryInterface(static_cast<nsIImageLoadingContent *>(objLC));

  if (!InActiveDocument(content)) {
    nsObjectLoadingContent *objLC =
      static_cast<nsObjectLoadingContent *>(mContent.get());
    objLC->UnloadObject();
  }
  return NS_OK;
}




class nsPluginErrorEvent : public nsRunnable {
public:
  nsPluginErrorEvent(nsIContent* aContent,
                     nsObjectLoadingContent::FallbackType aFallbackType)
    : mContent(aContent),
      mFallbackType(aFallbackType) {}

  ~nsPluginErrorEvent() {}

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIContent> mContent;
  nsObjectLoadingContent::FallbackType mFallbackType;
};

NS_IMETHODIMP
nsPluginErrorEvent::Run()
{
  nsString type;
  switch (mFallbackType) {
    case nsObjectLoadingContent::eFallbackVulnerableUpdatable:
      type = NS_LITERAL_STRING("PluginVulnerableUpdatable");
      break;
    case nsObjectLoadingContent::eFallbackVulnerableNoUpdate:
      type = NS_LITERAL_STRING("PluginVulnerableNoUpdate");
      break;
    case nsObjectLoadingContent::eFallbackClickToPlay:
      type = NS_LITERAL_STRING("PluginClickToPlay");
      break;
    case nsObjectLoadingContent::eFallbackUnsupported:
      type = NS_LITERAL_STRING("PluginNotFound");
      break;
    case nsObjectLoadingContent::eFallbackDisabled:
      type = NS_LITERAL_STRING("PluginDisabled");
      break;
    case nsObjectLoadingContent::eFallbackBlocklisted:
      type = NS_LITERAL_STRING("PluginBlocklisted");
      break;
    case nsObjectLoadingContent::eFallbackOutdated:
      type = NS_LITERAL_STRING("PluginOutdated");
      break;
    default:
      return NS_OK;
  }
  LOG(("OBJLC [%p]: nsPluginErrorEvent firing '%s'",
       mContent.get(), NS_ConvertUTF16toUTF8(type).get()));
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
  LOG(("OBJLC [%p]: Firing plugin crashed event\n",
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
  nsCOMPtr<nsIDOMDataContainerEvent> containerEvent(do_QueryInterface(event));
  if (!containerEvent) {
    NS_WARNING("Couldn't QI event for PluginCrashed event!");
    return NS_OK;
  }

  event->InitEvent(NS_LITERAL_STRING("PluginCrashed"), true, true);
  event->SetTrusted(true);
  event->GetInternalNSEvent()->flags |= NS_EVENT_FLAG_ONLY_CHROME_DISPATCH;

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

  nsEventDispatcher::DispatchDOMEvent(mContent, nullptr, event, nullptr, nullptr);
  return NS_OK;
}

class nsStopPluginRunnable : public nsRunnable, public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  nsStopPluginRunnable(nsPluginInstanceOwner* aInstanceOwner,
                       nsObjectLoadingContent* aContent)
    : mInstanceOwner(aInstanceOwner)
    , mContent(aContent)
  {
    NS_ASSERTION(aInstanceOwner, "need an owner");
    NS_ASSERTION(aContent, "need a nsObjectLoadingContent");
  }

  
  NS_IMETHOD Run();

  
  NS_IMETHOD Notify(nsITimer *timer);

private:
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<nsPluginInstanceOwner> mInstanceOwner;
  nsCOMPtr<nsIObjectLoadingContent> mContent;
};

NS_IMPL_ISUPPORTS_INHERITED1(nsStopPluginRunnable, nsRunnable, nsITimerCallback)

NS_IMETHODIMP
nsStopPluginRunnable::Notify(nsITimer *aTimer)
{
  return Run();
}

NS_IMETHODIMP
nsStopPluginRunnable::Run()
{
  
  
  nsCOMPtr<nsITimerCallback> kungFuDeathGrip = this;
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    PRUint32 currentLevel = 0;
    appShell->GetEventloopNestingLevel(&currentLevel);
    if (currentLevel > mInstanceOwner->GetLastEventloopNestingLevel()) {
      if (!mTimer)
        mTimer = do_CreateInstance("@mozilla.org/timer;1");
      if (mTimer) {
        
        
        nsresult rv = mTimer->InitWithCallback(this, 100,
                                               nsITimer::TYPE_ONE_SHOT);
        if (NS_SUCCEEDED(rv)) {
          return rv;
        }
      }
      NS_ERROR("Failed to setup a timer to stop the plugin later (at a safe "
               "time). Stopping the plugin now, this might crash.");
    }
  }

  mTimer = nullptr;

  static_cast<nsObjectLoadingContent*>(mContent.get())->
    DoStopPlugin(mInstanceOwner, false, true);

  return NS_OK;
}





class AutoSetInstantiatingToFalse {
public:
  AutoSetInstantiatingToFalse(nsObjectLoadingContent *aContent)
    : mContent(aContent) {}
  ~AutoSetInstantiatingToFalse() { mContent->mInstantiating = false; }
private:
  nsObjectLoadingContent* mContent;
};


class AutoSetLoadingToFalse {
public:
  AutoSetLoadingToFalse(nsObjectLoadingContent *aContent)
    : mContent(aContent) {}
  ~AutoSetLoadingToFalse() { mContent->mIsLoading = false; }
private:
  nsObjectLoadingContent* mContent;
};





static bool
IsSuccessfulRequest(nsIRequest* aRequest)
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

static bool
CanHandleURI(nsIURI* aURI)
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
  
  return extHandler == nullptr;
}



static bool inline
URIEquals(nsIURI *a, nsIURI *b)
{
  bool equal;
  return (!a && !b) || (a && b && NS_SUCCEEDED(a->Equals(b, &equal)) && equal);
}

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





bool
IsPluginEnabledByExtension(nsIURI* uri, nsCString& mimeType)
{
  nsCAutoString ext;
  GetExtensionFromURI(uri, ext);

  if (ext.IsEmpty()) {
    return false;
  }

  nsRefPtr<nsPluginHost> pluginHost =
    already_AddRefed<nsPluginHost>(nsPluginHost::GetInst());

  if (!pluginHost) {
    NS_NOTREACHED("No pluginhost");
    return false;
  }

  const char* typeFromExt;
  nsresult rv = pluginHost->IsPluginEnabledForExtension(ext.get(), typeFromExt);
  if (NS_SUCCEEDED(rv)) {
    mimeType = typeFromExt;
    return true;
  }
  return false;
}

nsresult
IsPluginEnabledForType(const nsCString& aMIMEType)
{
  nsRefPtr<nsPluginHost> pluginHost =
    already_AddRefed<nsPluginHost>(nsPluginHost::GetInst());

  if (!pluginHost) {
    NS_NOTREACHED("No pluginhost");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = pluginHost->IsPluginEnabledForType(aMIMEType.get());

  
  
  
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}





bool
nsObjectLoadingContent::IsSupportedDocument(const nsCString& aMimeType)
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");

  nsCOMPtr<nsIWebNavigationInfo> info(
    do_GetService(NS_WEBNAVIGATION_INFO_CONTRACTID));
  if (!info) {
    return false;
  }

  nsCOMPtr<nsIWebNavigation> webNav;
  nsIDocument* currentDoc = thisContent->GetCurrentDoc();
  if (currentDoc) {
    webNav = do_GetInterface(currentDoc->GetScriptGlobalObject());
  }
  
  PRUint32 supported;
  nsresult rv = info->IsTypeSupported(aMimeType, webNav, &supported);

  if (NS_FAILED(rv)) {
    return false;
  }

  if (supported != nsIWebNavigationInfo::UNSUPPORTED) {
    
    return supported != nsIWebNavigationInfo::PLUGIN;
  }

  
  
  
  
  nsCOMPtr<nsIStreamConverterService> convServ =
    do_GetService("@mozilla.org/streamConverters;1");
  bool canConvert = false;
  if (convServ) {
    rv = convServ->CanConvert(aMimeType.get(), "*/*", &canConvert);
  }
  return NS_SUCCEEDED(rv) && canConvert;
}

nsresult
nsObjectLoadingContent::BindToTree(nsIDocument* aDocument,
                                   nsIContent* aParent,
                                   nsIContent* aBindingParent,
                                   bool aCompileEventHandlers)
{
  nsImageLoadingContent::BindToTree(aDocument, aParent, aBindingParent,
                                    aCompileEventHandlers);

  if (aDocument) {
    return aDocument->AddPlugin(this);
  }
  return NS_OK;
}

void
nsObjectLoadingContent::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsImageLoadingContent::UnbindFromTree(aDeep, aNullParent);

  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIObjectLoadingContent*>(this));
  MOZ_ASSERT(thisContent);
  nsIDocument* ownerDoc = thisContent->OwnerDoc();
  ownerDoc->RemovePlugin(this);

  if (mType == eType_Plugin && mInstanceOwner) {
    
    
    
    
    nsCOMPtr<nsIRunnable> event = new InDocCheckEvent(this);

    nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
    if (appShell) {
      appShell->RunInStableState(event);
    }
  } else if (mType != eType_Image) {
    
    
    
    
    UnloadObject();
  }

}

nsObjectLoadingContent::nsObjectLoadingContent()
  : mPendingInstantiateEvent(nullptr)
  , mChannel(nullptr)
  , mType(eType_Loading)
  , mFallbackType(eFallbackAlternate)
  , mChannelLoaded(false)
  , mInstantiating(false)
  , mNetworkCreated(true)
  , mActivated(false)
  , mIsStopping(false)
  , mIsLoading(false)
  , mSrcStreamLoading(false) {}

nsObjectLoadingContent::~nsObjectLoadingContent()
{
  
  
  if (mFrameLoader) {
    NS_NOTREACHED("Should not be tearing down frame loaders at this point");
    mFrameLoader->Destroy();
  }
  if (mInstanceOwner) {
    
    
    NS_NOTREACHED("Should not be tearing down a plugin at this point!");
    StopPluginInstance();
  }
  DestroyImageLoadingContent();
}

nsresult
nsObjectLoadingContent::InstantiatePluginInstance()
{
  if (mType != eType_Plugin) {
    LOG(("OBJLC [%p]: Refusing to instantiate non-plugin, "
         "type %u", this, mType));
    return NS_OK;
  }

  
  if (mInstanceOwner) {
    return NS_OK;
  }

  
  if (mInstantiating) {
    return NS_OK;
  }
  mInstantiating = true;
  AutoSetInstantiatingToFalse autoInstantiating(this);

  
  
  
  nsCOMPtr<nsIObjectLoadingContent> kungFuDeathGrip = this;

  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent *>(this));
  
  nsIDocument* doc = thisContent->GetCurrentDoc();
  if (!doc) {
    return NS_ERROR_FAILURE;
  }
  if (!InActiveDocument(thisContent)) {
    NS_ERROR("Shouldn't be calling "
             "InstantiatePluginInstance in an inactive document");
    return NS_ERROR_FAILURE;
  }
  doc->FlushPendingNotifications(Flush_Layout);

  nsresult rv = NS_ERROR_FAILURE;
  nsRefPtr<nsPluginHost> pluginHost =
    already_AddRefed<nsPluginHost>(nsPluginHost::GetInst());

  if (!pluginHost) {
    NS_NOTREACHED("No pluginhost");
    return NS_ERROR_FAILURE;
  }

  
  
  
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    appShell->SuspendNative();
  }

  nsCOMPtr<nsIPluginDocument> pDoc(do_QueryInterface(doc));
  bool fullPageMode = false;
  if (pDoc) {
    pDoc->GetWillHandleInstantiation(&fullPageMode);
  }

  if (fullPageMode) {
    nsCOMPtr<nsIStreamListener> stream;
    rv = pluginHost->InstantiateFullPagePluginInstance(mContentType.get(),
                                                       mURI.get(), this,
                                                       getter_AddRefs(mInstanceOwner),
                                                       getter_AddRefs(stream));
    if (NS_SUCCEEDED(rv)) {
      pDoc->SetStreamListener(stream);
    }
  } else {
    rv = pluginHost->InstantiateEmbeddedPluginInstance(mContentType.get(),
                                                       mURI.get(), this,
                                                       getter_AddRefs(mInstanceOwner));
  }

  if (appShell) {
    appShell->ResumeNative();
  }

  if (NS_FAILED(rv)) {
    return rv;
  }

  
  NotifyContentObjectWrapper();

  nsRefPtr<nsNPAPIPluginInstance> pluginInstance;
  GetPluginInstance(getter_AddRefs(pluginInstance));
  if (pluginInstance) {
    nsCOMPtr<nsIPluginTag> pluginTag;
    pluginHost->GetPluginTagForInstance(pluginInstance,
                                        getter_AddRefs(pluginTag));

    nsCOMPtr<nsIBlocklistService> blocklist =
      do_GetService("@mozilla.org/extensions/blocklist;1");
    if (blocklist) {
      PRUint32 blockState = nsIBlocklistService::STATE_NOT_BLOCKED;
      blocklist->GetPluginBlocklistState(pluginTag, EmptyString(),
                                         EmptyString(), &blockState);
      if (blockState == nsIBlocklistService::STATE_OUTDATED)
        FirePluginError(eFallbackOutdated);
    }
  }

  return NS_OK;
}

void
nsObjectLoadingContent::NotifyOwnerDocumentActivityChanged()
{
  if (!mInstanceOwner) {
    return;
  }

  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  nsIDocument* ownerDoc = thisContent->OwnerDoc();
  if (!ownerDoc->IsActive()) {
    StopPluginInstance();
  }
}


NS_IMETHODIMP
nsObjectLoadingContent::OnStartRequest(nsIRequest *aRequest,
                                       nsISupports *aContext)
{
  
  

  SAMPLE_LABEL("nsObjectLoadingContent", "OnStartRequest");

  LOG(("OBJLC [%p]: Channel OnStartRequest", this));

  if (aRequest != mChannel || !aRequest) {
    
    return NS_BINDING_ABORTED;
  }

  NS_ASSERTION(!mChannelLoaded, "mChannelLoaded set already?");
  NS_ASSERTION(!mFinalListener, "mFinalListener exists already?");

  mChannelLoaded = true;

  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  NS_ASSERTION(chan, "Why is our request not a channel?");

  nsCOMPtr<nsIURI> uri;

  if (IsSuccessfulRequest(aRequest)) {
    chan->GetURI(getter_AddRefs(uri));
  }

  if (!uri) {
    LOG(("OBJLC [%p]: OnStartRequest: Request failed\n", this));
    
    
    
    mChannel = nullptr;
    LoadObject(true, false);
    return NS_ERROR_FAILURE;
  }

  return LoadObject(true, false, aRequest);
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

  mChannel = nullptr;

  if (mFinalListener) {
    mFinalListener->OnStopRequest(aRequest, aContext, aStatusCode);
    mFinalListener = nullptr;
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
    return mFinalListener->OnDataAvailable(aRequest, aContext, aInputStream,
                                           aOffset, aCount);
  }

  
  NS_NOTREACHED("Got data for channel with no connected final listener");
  mChannel = nullptr;

  return NS_ERROR_UNEXPECTED;
}


NS_IMETHODIMP
nsObjectLoadingContent::GetFrameLoader(nsIFrameLoader** aFrameLoader)
{
  NS_IF_ADDREF(*aFrameLoader = mFrameLoader);
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
nsObjectLoadingContent::HasNewFrame(nsIObjectFrame* aFrame)
{
  if (mType == eType_Plugin) {
    if (!mInstanceOwner) {
      
      
      AsyncStartPluginInstance();
      return NS_OK;
    }

    
    DisconnectFrame();

    
    nsObjectFrame *objFrame = static_cast<nsObjectFrame*>(aFrame);
    mInstanceOwner->SetFrame(objFrame);

    
    objFrame->FixupWindow(objFrame->GetContentRectRelativeToSelf().Size());
    objFrame->Invalidate(objFrame->GetContentRectRelativeToSelf());
  }
  return NS_OK;
}

NS_IMETHODIMP
nsObjectLoadingContent::DisconnectFrame()
{
  if (mInstanceOwner) {
    mInstanceOwner->SetFrame(nullptr);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsObjectLoadingContent::GetPluginInstance(nsNPAPIPluginInstance** aInstance)
{
  *aInstance = nullptr;

  if (!mInstanceOwner) {
    return NS_OK;
  }

  return mInstanceOwner->GetInstance(aInstance);
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
      switch (mFallbackType) {
        case eFallbackSuppressed:
          return NS_EVENT_STATE_SUPPRESSED;
        case eFallbackUserDisabled:
          return NS_EVENT_STATE_USERDISABLED;
        case eFallbackClickToPlay:
          return NS_EVENT_STATE_TYPE_CLICK_TO_PLAY;
        case eFallbackDisabled:
          return NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_HANDLER_DISABLED;
        case eFallbackBlocklisted:
          return NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_HANDLER_BLOCKED;
        case eFallbackCrashed:
          return NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_HANDLER_CRASHED;
        case eFallbackUnsupported: {
          
          char* pluginsBlocked = PR_GetEnv("MOZ_PLUGINS_BLOCKED");
          if (pluginsBlocked && pluginsBlocked[0] == '1') {
            return NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_TYPE_UNSUPPORTED_PLATFORM;
          } else {
            return NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_TYPE_UNSUPPORTED;
          }
        }
        case eFallbackOutdated:
        case eFallbackAlternate:
          return NS_EVENT_STATE_BROKEN;
        case eFallbackVulnerableUpdatable:
          return NS_EVENT_STATE_VULNERABLE_UPDATABLE;
        case eFallbackVulnerableNoUpdate:
          return NS_EVENT_STATE_VULNERABLE_NO_UPDATE;
      }
  };
  NS_NOTREACHED("unknown type?");
  return NS_EVENT_STATE_LOADING;
}


bool nsObjectLoadingContent::CheckObjectURIs(PRInt16 *aContentPolicy,
                                             PRInt32 aContentPolicyType)
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "Must be an instance of content");

  nsCOMPtr<nsIURI> docBaseURI = thisContent->GetBaseURI();

  
  if (!aContentPolicy || !mBaseURI) {
    return false;
  }

  bool ret;
  if (!URIEquals(mBaseURI, docBaseURI)) {
    
    
    ret = CheckURILoad(mBaseURI, aContentPolicy, aContentPolicyType);
    if (!ret) {
      return false;
    }
  }

  if (mURI) {
    return CheckURILoad(mURI, aContentPolicy, aContentPolicyType);
  }

  return true;
}

bool nsObjectLoadingContent::CheckURILoad(nsIURI *aURI,
                                          PRInt16 *aContentPolicy,
                                          PRInt32 aContentPolicyType)
{
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  NS_ASSERTION(secMan, "No security manager!?");

  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "Must be an instance of content");

  nsCOMPtr<nsIURI> docBaseURI = thisContent->GetBaseURI();

  nsIDocument* doc = thisContent->OwnerDoc();
  nsresult rv =
    secMan->CheckLoadURIWithPrincipal(thisContent->NodePrincipal(), aURI, 0);
  
  if (NS_FAILED(rv)) {
    nsCAutoString uri;
    nsCAutoString baseUri;
    aURI->GetSpec(uri);
    aURI->GetSpec(baseUri);
    LOG(("OBJLC [%p]: CheckLoadURIWithPrincipal denied load of %s (base %s)",
         this, uri.get(), baseUri.get()));
    return false;
  }

  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT; 
  rv = NS_CheckContentLoadPolicy(aContentPolicyType,
                                 aURI,
                                 doc->NodePrincipal(),
                                 static_cast<nsIImageLoadingContent*>(this),
                                 mContentType,
                                 nullptr, 
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 secMan);
  NS_ENSURE_SUCCESS(rv, false);
  if (aContentPolicy) {
    *aContentPolicy = shouldLoad;
  }
  if (NS_CP_REJECTED(shouldLoad)) {
    nsCAutoString uri;
    nsCAutoString baseUri;
    aURI->GetSpec(uri);
    aURI->GetSpec(baseUri);
    LOG(("OBJLC [%p]: Content policy denied load of %s (base %s)",
         this, uri.get(), baseUri.get()));
    return false;
  }
  return true;
}

nsObjectLoadingContent::ParameterUpdateFlags
nsObjectLoadingContent::UpdateObjectParameters()
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "Must be an instance of content");

  PRUint32 caps = GetCapabilities();
  LOG(("OBJLC [%p]: Updating object parameters", this));

  nsresult rv;
  nsCAutoString newMime;
  nsCOMPtr<nsIURI> newURI;
  nsCOMPtr<nsIURI> newBaseURI;
  ObjectType newType;
  
  bool stateInvalid = false;
  
  
  
  
  
  
  
  
  
  
  
  nsObjectLoadingContent::ParameterUpdateFlags retval = eParamNoChange;

  
  
  
  if (thisContent->NodeInfo()->Equals(nsGkAtoms::applet)) {
    newMime.AssignLiteral("application/x-java-vm");
  } else {
    nsAutoString typeAttr;
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type, typeAttr);
    if (!typeAttr.IsEmpty()) {
      CopyUTF16toUTF8(typeAttr, newMime);
    }
  }

  
  
  

  bool usingClassID = false;
  if (caps & eSupportClassID) {
    nsAutoString classIDAttr;
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::classid, classIDAttr);
    if (!classIDAttr.IsEmpty()) {
      usingClassID = true;
      if (NS_FAILED(TypeForClassID(classIDAttr, newMime))) {
        
        
        
        newMime.Assign("");
        stateInvalid = true;
      }
    }
  }

  
  
  

  nsAutoString codebaseStr;
  nsCOMPtr<nsIURI> docBaseURI = thisContent->GetBaseURI();
  thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::codebase, codebaseStr);
  if (codebaseStr.IsEmpty() && thisContent->NodeInfo()->Equals(nsGkAtoms::applet)) {
    
    
    
    
    
    codebaseStr.AssignLiteral("/"); 
    
    
    
  }

  if (!codebaseStr.IsEmpty()) {
    rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(newBaseURI),
                                                   codebaseStr,
                                                   thisContent->OwnerDoc(),
                                                   docBaseURI);
    if (NS_SUCCEEDED(rv)) {
      NS_TryToSetImmutable(newBaseURI);
    } else {
      
      LOG(("OBJLC [%p]: Could not parse plugin's codebase as a URI, "
           "will use document baseURI instead", this));
    }
  }

  
  if (!newBaseURI) {
    newBaseURI = docBaseURI;
  }

  
  
  

  nsAutoString uriStr;
  
  if (thisContent->NodeInfo()->Equals(nsGkAtoms::object)) {
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::data, uriStr);
  } else if (thisContent->NodeInfo()->Equals(nsGkAtoms::embed)) {
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::src, uriStr);
  } else if (thisContent->NodeInfo()->Equals(nsGkAtoms::applet)) {
    
  } else {
    NS_NOTREACHED("Unrecognized plugin-loading tag");
  }

  
  
  if (!uriStr.IsEmpty()) {
    rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(newURI),
                                                   uriStr,
                                                   thisContent->OwnerDoc(),
                                                   newBaseURI);
    if (NS_SUCCEEDED(rv)) {
      NS_TryToSetImmutable(newURI);
    } else {
      stateInvalid = true;
    }
  }

  
  
  
  

  if ((mOriginalContentType != newMime) || !URIEquals(mOriginalURI, newURI)) {
    
    
    
    
    
    retval = (ParameterUpdateFlags)(retval | eParamChannelChanged);
    LOG(("OBJLC [%p]: Channel parameters changed", this));
  }
  mOriginalContentType = newMime;
  mOriginalURI = newURI;

  
  
  
  

  
  
  bool useChannel = mChannelLoaded && !(retval & eParamChannelChanged);
  if (mChannel && useChannel) {
    nsCString channelType;
    rv = mChannel->GetContentType(channelType);
    if (NS_FAILED(rv)) {
      NS_NOTREACHED("GetContentType failed");
      stateInvalid = true;
      channelType.Assign("");
    }

    LOG(("OBJLC [%p]: Channel has a content type of %s", this, channelType.get()));

    bool binaryChannelType = false;
    if (channelType.EqualsASCII(APPLICATION_GUESS_FROM_EXT)) {
      channelType = APPLICATION_OCTET_STREAM;
      mChannel->SetContentType(channelType);
      binaryChannelType = true;
    } else if (channelType.EqualsASCII(APPLICATION_OCTET_STREAM)
               || channelType.EqualsASCII(BINARY_OCTET_STREAM)) {
      binaryChannelType = true;
    }

    
    rv = NS_GetFinalChannelURI(mChannel, getter_AddRefs(newURI));
    if (NS_FAILED(rv)) {
      NS_NOTREACHED("NS_GetFinalChannelURI failure");
      stateInvalid = true;
    }

    
    
    
    
    
    
    
    
    
    

    ObjectType typeHint = newMime.IsEmpty() ? eType_Null : GetTypeOfContent(newMime);

    bool caseOne = binaryChannelType
                   && typeHint != eType_Null
                   && typeHint != eType_Document;
    bool caseTwo = typeHint == eType_Plugin;
    if (caseOne || caseTwo) {
        
        
        
        nsCAutoString typeHint, dummy;
        NS_ParseContentType(newMime, typeHint, dummy);
        if (!typeHint.IsEmpty()) {
          mChannel->SetContentType(typeHint);
        }
    } else if (binaryChannelType
               && IsPluginEnabledByExtension(newURI, newMime)) {
      mChannel->SetContentType(newMime);
    } else {
      newMime = channelType;
    }
  }

  bool isJava = nsPluginHost::IsJavaMIMEType(newMime.get());
  if (useChannel && (!mChannel || isJava)) {
    
    
    
    
    
    
    
    
    
    stateInvalid = true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (stateInvalid) {
    newType = eType_Null;
  } else if (useChannel) {
      
      newType = GetTypeOfContent(newMime);
      LOG(("OBJLC [%p]: Using channel type", this));
  } else if (((caps & eAllowPluginSkipChannel) || !newURI || usingClassID) &&
             (GetTypeOfContent(newMime) == eType_Plugin)) {
    newType = eType_Plugin;
    LOG(("OBJLC [%p]: Skipping loading channel, type plugin", this));
  } else if (newURI) {
    
    
    newType = eType_Loading;
  } else {
    
    
    newType = eType_Null;
  }

  
  
  

  if (newType != mType) {
    retval = (ParameterUpdateFlags)(retval | eParamStateChanged);
    LOG(("OBJLC [%p]: Type changed from %u -> %u", this, mType, newType));
    mType = newType;
  }

  if (!URIEquals(mBaseURI, newBaseURI)) {
    if (isJava) {
      
      
      
      retval = (ParameterUpdateFlags)(retval | eParamStateChanged);
    }
    LOG(("OBJLC [%p]: Object effective baseURI changed", this));
    mBaseURI = newBaseURI;
  }

  if (!URIEquals(newURI, mURI)) {
    retval = (ParameterUpdateFlags)(retval | eParamStateChanged);
    LOG(("OBJLC [%p]: Object effective URI changed", this));
    mURI = newURI;
  }

  if (mContentType != newMime) {
    retval = (ParameterUpdateFlags)(retval | eParamStateChanged);
    LOG(("OBJLC [%p]: Object effective mime type changed (%s -> %s)", this, mContentType.get(), newMime.get()));
    mContentType = newMime;
  }

  return retval;
}


nsresult
nsObjectLoadingContent::LoadObject(bool aNotify,
                                   bool aForceLoad)
{
  return LoadObject(aNotify, aForceLoad, nullptr);
}

nsresult
nsObjectLoadingContent::LoadObject(bool aNotify,
                                   bool aForceLoad,
                                   nsIRequest *aLoadingChannel)
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");
  nsIDocument* doc = thisContent->OwnerDoc();
  nsresult rv = NS_OK;

  
  if (!InActiveDocument(thisContent)) {
    NS_NOTREACHED("LoadObject called while not bound to an active document");
    return NS_ERROR_UNEXPECTED;
  }

  
  
  
  if (doc->IsBeingUsedAsImage() || doc->IsLoadedAsData()) {
    return NS_OK;
  }

  LOG(("OBJLC [%p]: LoadObject called, notify %u, forceload %u, channel %p",
       this, aNotify, aForceLoad, aLoadingChannel));

  
  
  if (aForceLoad && mChannelLoaded) {
    CloseChannel();
    mChannelLoaded = false;
  }

  
  nsEventStates oldState = ObjectState();
  ObjectType oldType = mType;

  ParameterUpdateFlags stateChange = UpdateObjectParameters();

  
  
  if ((!stateChange && !aForceLoad) || (mType == eType_Loading && mChannel)) {
    return NS_OK;
  }

  
  
  
  LOG(("OBJLC [%p]: LoadObject - plugin state changed (%u)",
       this, stateChange));

  
  
  
  FallbackType fallbackType = eFallbackAlternate;

  if (mType == eType_Null) {
    nsresult pluginsupport = IsPluginEnabledForType(mContentType);
    if (pluginsupport == NS_ERROR_PLUGIN_DISABLED) {
      fallbackType = eFallbackDisabled;
    } else if (pluginsupport == NS_ERROR_PLUGIN_BLOCKLISTED) {
      fallbackType = eFallbackBlocklisted;
    } else {
      fallbackType = eFallbackUnsupported;
    }
  }

  
  
  
  
  
  mIsLoading = true;
  AutoSetLoadingToFalse reentryCheck(this);

  
  
  UnloadObject(false); 
  if (!mIsLoading) {
    
    
    return NS_OK;
  }

  if (stateChange & eParamChannelChanged) {
    
    
    
    CloseChannel();
    mChannelLoaded = false;
  } else if (mType == eType_Null && mChannel) {
    
    
    
    CloseChannel();
  } else if (mChannelLoaded && mChannel != aLoadingChannel) {
    
    
    
    NS_NOTREACHED("Loading with a channel, but state doesn't make sense");
    return NS_OK;
  }

  
  
  

  
  
  
  
  
  PRInt32 policyType;
  if (mType != eType_Null) {
    bool allowLoad = false;
    PRInt16 contentPolicy = nsIContentPolicy::ACCEPT;
    PRUint32 caps = GetCapabilities();
    bool supportImage = caps & eSupportImages;
    bool supportDoc = (caps & eSupportDocuments) || (caps & eSupportSVG);
    bool supportPlugin = caps & eSupportPlugins;
    if (mType == eType_Image || (mType == eType_Loading && supportImage)) {
      policyType = nsIContentPolicy::TYPE_IMAGE;
      allowLoad = CheckObjectURIs(&contentPolicy, policyType);
    }
    if (!allowLoad &&
        (mType == eType_Document || (mType == eType_Loading && supportDoc))) {
      contentPolicy = nsIContentPolicy::ACCEPT;
      policyType = nsIContentPolicy::TYPE_SUBDOCUMENT;
      allowLoad = CheckObjectURIs(&contentPolicy, policyType);
    }
    if (!allowLoad &&
        (mType == eType_Plugin || (mType == eType_Loading && supportPlugin))) {
      contentPolicy = nsIContentPolicy::ACCEPT;
      policyType = nsIContentPolicy::TYPE_OBJECT;
      allowLoad = CheckObjectURIs(&contentPolicy, policyType);
    }

    
    if (!allowLoad) {
      mType = eType_Null;
      if (contentPolicy == nsIContentPolicy::REJECT_TYPE) {
        fallbackType = eFallbackUserDisabled;
      } else {
        fallbackType = eFallbackSuppressed;
      }
    }
  }

  
  
  FallbackType clickToPlayReason;
  if (mType == eType_Plugin && !ShouldPlay(clickToPlayReason)) {
    LOG(("OBJLC [%p]: Marking plugin as click-to-play", this));
    mType = eType_Null;
    fallbackType = clickToPlayReason;
  }

  
  
  if (mFrameLoader || mPendingInstantiateEvent || mInstanceOwner ||
      mFinalListener)
  {
    NS_NOTREACHED("Trying to load new plugin with existing content");
    rv = NS_ERROR_UNEXPECTED;
    return NS_OK;
  }

  
  
  if (mType != eType_Null && !!mChannel != mChannelLoaded) {
    NS_NOTREACHED("Trying to load with bad channel state");
    rv = NS_ERROR_UNEXPECTED;
    return NS_OK;
  }

  
  
  
  switch (mType) {
    case eType_Image:
      if (!mChannel) {
        
        
        NS_NOTREACHED("Attempting to load image without a channel?");
        rv = NS_ERROR_UNEXPECTED;
        break;
      }
      rv = LoadImageWithChannel(mChannel, getter_AddRefs(mFinalListener));
      if (mFinalListener) {
        
        
        mSrcStreamLoading = true;
        rv = mFinalListener->OnStartRequest(mChannel, nullptr);
        mSrcStreamLoading = false;
      }
    break;
    case eType_Plugin:
    {
      if (mChannel) {
        nsRefPtr<nsPluginHost> pluginHost =
          already_AddRefed<nsPluginHost>(nsPluginHost::GetInst());
        if (!pluginHost) {
          NS_NOTREACHED("No pluginHost");
          rv = NS_ERROR_UNEXPECTED;
          break;
        }

        
        NotifyStateChanged(oldType, oldState, true, aNotify);
        oldType = mType;
        oldState = ObjectState();

        rv = pluginHost->NewEmbeddedPluginStreamListener(mURI, this, nullptr,
                                                         getter_AddRefs(mFinalListener));
        if (NS_SUCCEEDED(rv)) {
          
          

          mSrcStreamLoading = true;
          rv = mFinalListener->OnStartRequest(mChannel, nullptr);
          mSrcStreamLoading = false;
          if (NS_SUCCEEDED(rv)) {
            NotifyContentObjectWrapper();
          }
        }
      } else {
        rv = AsyncStartPluginInstance();
      }
    }
    break;
    case eType_Document:
    {
      if (!mChannel) {
        
        
        NS_NOTREACHED("Attempting to load a document without a channel");
        mType = eType_Null;
        break;
      }
      
      mFrameLoader = nsFrameLoader::Create(thisContent->AsElement(),
                                           mNetworkCreated);
      if (!mFrameLoader) {
        NS_NOTREACHED("nsFrameLoader::Create failed");
        mType = eType_Null;
        break;
      }
      
      rv = mFrameLoader->CheckForRecursiveLoad(mURI);
      if (NS_FAILED(rv)) {
        mType = eType_Null;
        break;
      }

      
      
      nsLoadFlags flags = 0;
      mChannel->GetLoadFlags(&flags);
      flags |= nsIChannel::LOAD_DOCUMENT_URI;
      mChannel->SetLoadFlags(flags);

      nsCOMPtr<nsIDocShell> docShell;
      rv = mFrameLoader->GetDocShell(getter_AddRefs(docShell));
      if (NS_FAILED(rv)) {
        NS_NOTREACHED("Could not get DocShell from mFrameLoader?");
        mType = eType_Null;
        break;
      }

      nsCOMPtr<nsIInterfaceRequestor> req(do_QueryInterface(docShell));
      NS_ASSERTION(req, "Docshell must be an ifreq");

      nsCOMPtr<nsIURILoader>
        uriLoader(do_GetService(NS_URI_LOADER_CONTRACTID, &rv));
      if (NS_FAILED(rv)) {
        NS_NOTREACHED("Failed to get uriLoader service");
        mType = eType_Null;
        break;
      }
      rv = uriLoader->OpenChannel(mChannel, nsIURILoader::DONT_RETARGET, req,
                                  getter_AddRefs(mFinalListener));
      if (NS_SUCCEEDED(rv)) {
        
        
        mSrcStreamLoading = true;
        rv = mFinalListener->OnStartRequest(mChannel, nullptr);
        mSrcStreamLoading = false;
      }
    }
    break;
    case eType_Loading:
      
      rv = OpenChannel(policyType);
      if (NS_FAILED(rv)) {
        LOG(("OBJLC [%p]: OpenChannel returned failure (%u)", this, rv));
      }
    break;
    case eType_Null:
      
    break;
  };

  if (NS_FAILED(rv)) {
    
    LOG(("OBJLC [%p]: Loading failed, switching to fallback", this));
    mType = eType_Null;
  }

  
  if (mType == eType_Null) {
    LOG(("OBJLC [%p]: Loading fallback, type %u", this, fallbackType));
    NS_ASSERTION(!mFrameLoader && !mInstanceOwner,
                 "switched to type null but also loaded something");

    if (mChannel) {
      
      
      CloseChannel();
    }

    
    
    LoadFallback(fallbackType, false);
  }

  
  NotifyStateChanged(oldType, oldState, false, aNotify);

  if (mType == eType_Null && !mContentType.IsEmpty() &&
      mFallbackType != eFallbackAlternate) {
    
    
    
    FirePluginError(mFallbackType);
  }

  return NS_OK;
}

nsresult
nsObjectLoadingContent::CloseChannel()
{
  if (mChannel) {
    LOG(("OBJLC [%p]: Closing channel\n", this));
    
    
    
    
    mChannel->Cancel(NS_BINDING_ABORTED);
    if (mFinalListener) {
      
      
      mFinalListener->OnStopRequest(mChannel, nullptr, NS_BINDING_ABORTED);
      mFinalListener = nullptr;
    }
    mChannel = nullptr;
  }
  return NS_OK;
}

nsresult
nsObjectLoadingContent::OpenChannel(PRInt32 aPolicyType)
{
  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");
  nsIDocument* doc = thisContent->OwnerDoc();
  NS_ASSERTION(doc, "No owner document?");
  NS_ASSERTION(!mInstanceOwner && !mInstantiating,
               "opening a new channel with already loaded content");

  nsresult rv;
  mChannel = nullptr;

  
  if (!mURI || !CanHandleURI(mURI)) {
    return NS_ERROR_NOT_AVAILABLE;
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
    channelPolicy->SetLoadType(aPolicyType);
  }
  rv = NS_NewChannel(getter_AddRefs(chan), mURI, nullptr, group, this,
                     nsIChannel::LOAD_CALL_CONTENT_SNIFFERS |
                     nsIChannel::LOAD_CLASSIFY_URI,
                     channelPolicy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIHttpChannel> httpChan(do_QueryInterface(chan));
  if (httpChan) {
    httpChan->SetReferrer(doc->GetDocumentURI());
  }

  
  nsContentUtils::SetUpChannelOwner(thisContent->NodePrincipal(),
                                    chan, mURI, true);

  nsCOMPtr<nsIScriptChannel> scriptChannel = do_QueryInterface(chan);
  if (scriptChannel) {
    
    scriptChannel->SetExecutionPolicy(nsIScriptChannel::EXECUTE_NORMAL);
  }

  
  rv = chan->AsyncOpen(this, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);
  LOG(("OBJLC [%p]: Channel opened", this));
  mChannel = chan;
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
nsObjectLoadingContent::DestroyContent()
{
  if (mFrameLoader) {
    mFrameLoader->Destroy();
    mFrameLoader = nullptr;
  }

  StopPluginInstance();
}


void
nsObjectLoadingContent::Traverse(nsObjectLoadingContent *tmp,
                                 nsCycleCollectionTraversalCallback &cb)
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mFrameLoader");
  cb.NoteXPCOMChild(static_cast<nsIFrameLoader*>(tmp->mFrameLoader));
}

void
nsObjectLoadingContent::UnloadObject(bool aResetState)
{
  
  
  CancelImageRequests(false);
  if (mFrameLoader) {
    mFrameLoader->Destroy();
    mFrameLoader = nullptr;
  }

  if (aResetState) {
    CloseChannel();
    mType = eType_Loading;
  }

  
  StopPluginInstance();
}

void
nsObjectLoadingContent::NotifyStateChanged(ObjectType aOldType,
                                           nsEventStates aOldState,
                                           bool aSync,
                                           bool aNotify)
{
  LOG(("OBJLC [%p]: Notifying about state change: (%u, %llx) -> (%u, %llx)"
       " (sync %i, notify %i)", this, aOldType, aOldState.GetInternalValue(),
       mType, ObjectState().GetInternalValue(), aSync, aNotify));

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
    
    NS_ASSERTION(InActiveDocument(thisContent), "Something is confused");
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
nsObjectLoadingContent::FirePluginError(FallbackType aFallbackType)
{
  nsCOMPtr<nsIContent> thisContent = 
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");

  LOG(("OBJLC [%p]: Dispatching nsPluginErrorEvent for content %p\n",
       this));

  nsCOMPtr<nsIRunnable> ev = new nsPluginErrorEvent(thisContent, aFallbackType);
  nsresult rv = NS_DispatchToCurrentThread(ev);
  if (NS_FAILED(rv)) {
    NS_WARNING("failed to dispatch nsPluginErrorEvent");
  }
}

nsObjectLoadingContent::ObjectType
nsObjectLoadingContent::GetTypeOfContent(const nsCString& aMIMEType)
{
  if (aMIMEType.IsEmpty()) {
    return eType_Null;
  }

  PRUint32 caps = GetCapabilities();

  if ((caps & eSupportImages) && IsSupportedImage(aMIMEType)) {
    return eType_Image;
  }

  
  bool isSVG = aMIMEType.LowerCaseEqualsLiteral("image/svg+xml");
  bool supportType = isSVG ? eSupportSVG : eSupportDocuments;
  if ((caps & supportType) && IsSupportedDocument(aMIMEType)) {
    return eType_Document;
  }

  if ((caps & eSupportPlugins) && NS_SUCCEEDED(IsPluginEnabledForType(aMIMEType))) {
    return eType_Plugin;
  }

  return eType_Null;
}

nsresult
nsObjectLoadingContent::TypeForClassID(const nsAString& aClassID,
                                       nsACString& aType)
{
  if (StringBeginsWith(aClassID, NS_LITERAL_STRING("java:"))) {
    
    aType.AssignLiteral("application/x-java-vm");
    nsresult rv = IsPluginEnabledForType(NS_LITERAL_CSTRING("application/x-java-vm"));
    return NS_SUCCEEDED(rv) ? NS_OK : NS_ERROR_NOT_AVAILABLE;
  }

  
  if (StringBeginsWith(aClassID, NS_LITERAL_STRING("clsid:"), nsCaseInsensitiveStringComparator())) {
    

    if (NS_SUCCEEDED(IsPluginEnabledForType(NS_LITERAL_CSTRING("application/x-oleobject")))) {
      aType.AssignLiteral("application/x-oleobject");
      return NS_OK;
    }
    if (NS_SUCCEEDED(IsPluginEnabledForType(NS_LITERAL_CSTRING("application/oleobject")))) {
      aType.AssignLiteral("application/oleobject");
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

nsObjectFrame*
nsObjectLoadingContent::GetExistingFrame()
{
  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  nsIFrame* frame = thisContent->GetPrimaryFrame();
  nsIObjectFrame* objFrame = do_QueryFrame(frame);
  return static_cast<nsObjectFrame*>(objFrame);
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
    aDest->mPrintFrame = const_cast<nsObjectLoadingContent*>(this)->GetExistingFrame();
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
  LOG(("OBJLC [%p]: Plugin Crashed, queuing crash event", this));
  NS_ASSERTION(mType == eType_Plugin, "PluginCrashed at non-plugin type");

  
  mInstanceOwner = nullptr;
  CloseChannel();

  
  LoadFallback(eFallbackCrashed, true);

  
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));

  
  
  nsCAutoString pluginName;
  aPluginTag->GetName(pluginName);
  nsCAutoString pluginFilename;
  aPluginTag->GetFilename(pluginFilename);

  nsCOMPtr<nsIRunnable> ev =
    new nsPluginCrashedEvent(thisContent,
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

NS_IMETHODIMP
nsObjectLoadingContent::SyncStartPluginInstance()
{
  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(),
               "Must be able to run script in order to instantiate a plugin instance!");

  
  
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  if (!InActiveDocument(thisContent)) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIURI> kungFuURIGrip(mURI);
  nsCString contentType(mContentType);
  return InstantiatePluginInstance();
}

NS_IMETHODIMP
nsObjectLoadingContent::AsyncStartPluginInstance()
{
  
  if (mInstanceOwner) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  nsIDocument* doc = thisContent->OwnerDoc();
  if (doc->IsStaticDocument() || doc->IsBeingUsedAsImage()) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIRunnable> event = new nsAsyncInstantiateEvent(this);
  if (!event) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult rv = NS_DispatchToCurrentThread(event);
  if (NS_SUCCEEDED(rv)) {
    
    
    mPendingInstantiateEvent = event;
  }

  return rv;
}

NS_IMETHODIMP
nsObjectLoadingContent::GetSrcURI(nsIURI** aURI)
{
  NS_IF_ADDREF(*aURI = mURI);
  return NS_OK;
}

static bool
DoDelayedStop(nsPluginInstanceOwner* aInstanceOwner,
              nsObjectLoadingContent* aContent,
              bool aDelayedStop)
{
#if (MOZ_PLATFORM_MAEMO==5)
  
  if (aDelayedStop && aInstanceOwner->MatchPluginName("Shockwave Flash"))
    return false;
#endif

  
  
  if (aDelayedStop
#if !(defined XP_WIN || defined MOZ_X11)
      && !aInstanceOwner->MatchPluginName("QuickTime")
      && !aInstanceOwner->MatchPluginName("Flip4Mac")
      && !aInstanceOwner->MatchPluginName("XStandard plugin")
      && !aInstanceOwner->MatchPluginName("CMISS Zinc Plugin")
#endif
      ) {
    nsCOMPtr<nsIRunnable> evt =
      new nsStopPluginRunnable(aInstanceOwner, aContent);
    NS_DispatchToCurrentThread(evt);
    return true;
  }
  return false;
}

void
nsObjectLoadingContent::LoadFallback(FallbackType aType, bool aNotify) {
  nsEventStates oldState = ObjectState();
  ObjectType oldType = mType;

  NS_ASSERTION(!mInstanceOwner && !mFrameLoader && !mChannel,
               "LoadFallback called with loaded content");

  
  
  
  nsCOMPtr<nsIContent> thisContent =
  do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "must be a content");

  if (!thisContent->IsHTML()) {
    
    LOG(("OBJLC [%p]: Non-HTML content, forcing eFallbackAlternate", this));
    aType = eFallbackAlternate;
  }

  
  
  
  bool hasAlternateContent = false;
  bool hasPluginUrl = false;
  if (thisContent->Tag() == nsGkAtoms::object &&
      (aType == eFallbackUnsupported ||
       aType == eFallbackDisabled ||
       aType == eFallbackBlocklisted))
  {
    for (nsIContent* child = thisContent->GetFirstChild();
         child; child = child->GetNextSibling())
    {
      if (child->IsHTML(nsGkAtoms::param)) {
        if (child->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
          NS_LITERAL_STRING("pluginurl"), eIgnoreCase)) {
          hasPluginUrl = true;
        }
      } else if (nsStyleUtil::IsSignificantChild(child, true, false)) {
        hasAlternateContent = true;
      }
    }

    
    
    
    if (hasAlternateContent && !hasPluginUrl) {
      LOG(("OBJLC [%p]: Unsupported/disabled/blocked plugin has alternate "
      "content, showing instead of custom handler", this));
      aType = eFallbackAlternate;
    }
  }

  mType = eType_Null;
  mFallbackType = aType;

  
  
  
  if (!aNotify) {
    return; 
  }

  NotifyStateChanged(oldType, oldState, false, true);

  if (mFallbackType != eFallbackCrashed &&
      mFallbackType != eFallbackAlternate)
  {
    
    
    FirePluginError(mFallbackType);
  }
}

void
nsObjectLoadingContent::DoStopPlugin(nsPluginInstanceOwner* aInstanceOwner,
                                     bool aDelayedStop,
                                     bool aForcedReentry)
{
  
  
  
  
  if (mIsStopping && !aForcedReentry) {
    return;
  }
  mIsStopping = true;

  nsRefPtr<nsPluginInstanceOwner> kungFuDeathGrip(aInstanceOwner);
  nsRefPtr<nsNPAPIPluginInstance> inst;
  aInstanceOwner->GetInstance(getter_AddRefs(inst));
  if (inst) {
    if (DoDelayedStop(aInstanceOwner, this, aDelayedStop)) {
      return;
    }

#if defined(XP_MACOSX)
    aInstanceOwner->HidePluginWindow();
#endif

    nsRefPtr<nsPluginHost> pluginHost =
      already_AddRefed<nsPluginHost>(nsPluginHost::GetInst());
    NS_ASSERTION(pluginHost, "No plugin host?");
    pluginHost->StopPluginInstance(inst);
  }

  aInstanceOwner->Destroy();
}

NS_IMETHODIMP
nsObjectLoadingContent::StopPluginInstance()
{
  
  mPendingInstantiateEvent = nullptr;

  if (!mInstanceOwner) {
    return NS_OK;
  }

  if (mChannel) {
    
    
    
    
    
    LOG(("OBJLC [%p]: StopPluginInstance - Closing used channel", this));
    CloseChannel();
  }

  DisconnectFrame();

  bool delayedStop = false;
#ifdef XP_WIN
  
  nsRefPtr<nsNPAPIPluginInstance> inst;
  mInstanceOwner->GetInstance(getter_AddRefs(inst));
  if (inst) {
    const char* mime = nullptr;
    if (NS_SUCCEEDED(inst->GetMIMEType(&mime)) && mime) {
      if (strcmp(mime, "audio/x-pn-realaudio-plugin") == 0) {
        delayedStop = true;
      }
    }
  }
#endif

  DoStopPlugin(mInstanceOwner, delayedStop);

  mInstanceOwner = nullptr;

  return NS_OK;
}

void
nsObjectLoadingContent::NotifyContentObjectWrapper()
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));

  nsCOMPtr<nsIDocument> doc = thisContent->GetDocument();
  if (!doc)
    return;

  nsIScriptGlobalObject *sgo = doc->GetScopeObject();
  if (!sgo)
    return;

  nsIScriptContext *scx = sgo->GetContext();
  if (!scx)
    return;

  JSContext *cx = scx->GetNativeContext();

  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  nsContentUtils::XPConnect()->
  GetWrappedNativeOfNativeObject(cx, sgo->GetGlobalJSObject(), thisContent,
                                 NS_GET_IID(nsISupports),
                                 getter_AddRefs(wrapper));

  if (!wrapper) {
    
    
    return;
  }

  JSObject *obj = nullptr;
  nsresult rv = wrapper->GetJSObject(&obj);
  if (NS_FAILED(rv))
    return;

  nsHTMLPluginObjElementSH::SetupProtoChain(wrapper, cx, obj);
}

NS_IMETHODIMP
nsObjectLoadingContent::PlayPlugin()
{
  if (!nsContentUtils::IsCallerChrome())
    return NS_OK;

  mActivated = true;
  return LoadObject(true, true);
}

NS_IMETHODIMP
nsObjectLoadingContent::GetActivated(bool *aActivated)
{
  FallbackType reason;
  *aActivated = ShouldPlay(reason);
  return NS_OK;
}

bool
nsObjectLoadingContent::ShouldPlay(FallbackType &aReason)
{
  
  
  

  nsRefPtr<nsPluginHost> pluginHost =
    already_AddRefed<nsPluginHost>(nsPluginHost::GetInst());

  bool isCTP = pluginHost->IsPluginClickToPlayForType(mContentType.get());

  if (!isCTP || mActivated) {
    return true;
  }

  aReason = eFallbackClickToPlay;

  
  
  
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(static_cast<nsIObjectLoadingContent*>(this));
  MOZ_ASSERT(thisContent);
  nsIDocument* ownerDoc = thisContent->OwnerDoc();

  nsCOMPtr<nsIDOMWindow> window = ownerDoc->GetWindow();
  if (!window) {
    return false;
  }
  nsCOMPtr<nsIDOMWindow> topWindow;
  rv = window->GetTop(getter_AddRefs(topWindow));
  NS_ENSURE_SUCCESS(rv, false);
  nsCOMPtr<nsIDOMDocument> topDocument;
  rv = topWindow->GetDocument(getter_AddRefs(topDocument));
  NS_ENSURE_SUCCESS(rv, false);
  nsCOMPtr<nsIDocument> topDoc = do_QueryInterface(topDocument);
  nsIURI* topUri = topDoc->GetDocumentURI();

  nsCOMPtr<nsIPermissionManager> permissionManager = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, false);

  bool allowPerm = false;
  
  
  
  
  
  if (!nsContentUtils::IsSystemPrincipal(topDoc->NodePrincipal())) {
    PRUint32 permission;
    rv = permissionManager->TestPermissionFromPrincipal(topDoc->NodePrincipal(),
                                                        "plugins",
                                                        &permission);
    NS_ENSURE_SUCCESS(rv, false);
    allowPerm = permission == nsIPermissionManager::ALLOW_ACTION;
  }

  PRUint32 state;
  rv = pluginHost->GetBlocklistStateForType(mContentType.get(), &state);
  NS_ENSURE_SUCCESS(rv, false);

  
  if (state == nsIBlocklistService::STATE_VULNERABLE_UPDATE_AVAILABLE) {
    aReason = eFallbackVulnerableUpdatable;
    return false;
  }
  if (state == nsIBlocklistService::STATE_VULNERABLE_NO_UPDATE) {
    aReason = eFallbackVulnerableNoUpdate;
    return false;
  }

  return allowPerm;
}

