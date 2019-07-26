












#include "imgLoader.h"
#include "nsEventDispatcher.h"
#include "nsIContent.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIDOMDataContainerEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsIExternalProtocolHandler.h"
#include "nsEventStates.h"
#include "nsIObjectFrame.h"
#include "nsIPermissionManager.h"
#include "nsPluginHost.h"
#include "nsPluginInstanceOwner.h"
#include "nsJSNPRuntime.h"
#include "nsIPresShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsScriptSecurityManager.h"
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
#include "nsCxPusher.h"
#include "nsDocShellCID.h"
#include "nsGkAtoms.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsMimeTypes.h"
#include "nsStyleUtil.h"
#include "nsUnicharUtils.h"
#include "mozilla/Preferences.h"
#include "nsSandboxFlags.h"


#include "nsFrameLoader.h"

#include "nsObjectLoadingContent.h"
#include "mozAutoDocUpdate.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIChannelPolicy.h"
#include "nsChannelPolicy.h"
#include "mozilla/dom/Element.h"
#include "GeckoProfiler.h"
#include "nsObjectFrame.h"
#include "nsDOMClassInfo.h"
#include "nsWrapperCacheInlines.h"
#include "nsDOMJSUtils.h"

#include "nsWidgetsCID.h"
#include "nsContentCID.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/Telemetry.h"

#ifdef XP_WIN

#ifdef CreateEvent
#undef CreateEvent
#endif
#endif 

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

using namespace mozilla;
using namespace mozilla::dom;

#ifdef PR_LOGGING
static PRLogModuleInfo*
GetObjectLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("objlc");
  return sLog;
}
#endif

#define LOG(args) PR_LOG(GetObjectLog(), PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(GetObjectLog(), PR_LOG_DEBUG)

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





class CheckPluginStopEvent : public nsRunnable {
public:
  CheckPluginStopEvent(nsObjectLoadingContent *aContent)
  : mContent(aContent) {}

  ~CheckPluginStopEvent() {}

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIObjectLoadingContent> mContent;
};

NS_IMETHODIMP
CheckPluginStopEvent::Run()
{
  nsObjectLoadingContent *objLC =
    static_cast<nsObjectLoadingContent *>(mContent.get());

  
  
  
  if (objLC->mPendingCheckPluginStopEvent != this) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content =
    do_QueryInterface(static_cast<nsIImageLoadingContent *>(objLC));
  if (!InActiveDocument(content)) {
    
    LOG(("OBJLC [%p]: Unloading plugin outside of document", this));
    objLC->UnloadObject();
    return NS_OK;
  }

  if (!content->GetPrimaryFrame()) {
    LOG(("OBJLC [%p]: CheckPluginStopEvent - No frame, flushing layout", this));
    nsIDocument* currentDoc = content->GetCurrentDoc();
    if (currentDoc) {
      currentDoc->FlushPendingNotifications(Flush_Layout);
      if (objLC->mPendingCheckPluginStopEvent != this) {
        LOG(("OBJLC [%p]: CheckPluginStopEvent - superseded in layout flush",
             this));
        return NS_OK;
      } else if (content->GetPrimaryFrame()) {
        LOG(("OBJLC [%p]: CheckPluginStopEvent - frame gained in layout flush",
             this));
        objLC->mPendingCheckPluginStopEvent = nullptr;
        return NS_OK;
      }
    }
    
    
    LOG(("OBJLC [%p]: Stopping plugin that lost frame", this));
    
    objLC->StopPluginInstance();
  }

  objLC->mPendingCheckPluginStopEvent = nullptr;
  return NS_OK;
}




class nsSimplePluginEvent : public nsRunnable {
public:
  nsSimplePluginEvent(nsIContent* aTarget, const nsAString &aEvent)
    : mTarget(aTarget)
    , mDocument(aTarget->GetCurrentDoc())
    , mEvent(aEvent)
  {
  }

  nsSimplePluginEvent(nsIDocument* aTarget, const nsAString& aEvent)
    : mTarget(aTarget)
    , mDocument(aTarget)
    , mEvent(aEvent)
  {
  }

  ~nsSimplePluginEvent() {}

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsISupports> mTarget;
  nsCOMPtr<nsIDocument> mDocument;
  nsString mEvent;
};

NS_IMETHODIMP
nsSimplePluginEvent::Run()
{
  LOG(("OBJLC [%p]: nsSimplePluginEvent firing event \"%s\"", mTarget.get(),
       mEvent.get()));
  nsContentUtils::DispatchTrustedEvent(mDocument, mTarget,
                                       mEvent, true, true);
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
  event->GetInternalNSEvent()->mFlags.mOnlyChromeDispatch = true;

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
    uint32_t currentLevel = 0;
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
  nsAutoCString scheme;
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
  return imgLoader::SupportImageWithMimeType(aMimeType.get());
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

    int32_t offset = spec.RFindChar('.');
    if (offset != kNotFound) {
      ext = Substring(spec, offset + 1, spec.Length());
    }
  }
}





bool
IsPluginEnabledByExtension(nsIURI* uri, nsCString& mimeType)
{
  nsAutoCString ext;
  GetExtensionFromURI(uri, ext);

  if (ext.IsEmpty()) {
    return false;
  }

  nsRefPtr<nsPluginHost> pluginHost = nsPluginHost::GetInst();

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

bool
PluginExistsForType(const char* aMIMEType)
{
  nsRefPtr<nsPluginHost> pluginHost = nsPluginHost::GetInst();

  if (!pluginHost) {
    NS_NOTREACHED("No pluginhost");
    return false;
  }

  return pluginHost->PluginExistsForType(aMIMEType);
}






void
nsObjectLoadingContent::QueueCheckPluginStopEvent()
{
  nsCOMPtr<nsIRunnable> event = new CheckPluginStopEvent(this);
  mPendingCheckPluginStopEvent = event;

  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    appShell->RunInStableState(event);
  }
}



bool
nsObjectLoadingContent::MakePluginListener()
{
  if (!mInstanceOwner) {
    NS_NOTREACHED("expecting a spawned plugin");
    return false;
  }
  nsRefPtr<nsPluginHost> pluginHost = nsPluginHost::GetInst();
  if (!pluginHost) {
    NS_NOTREACHED("No pluginHost");
    return false;
  }
  NS_ASSERTION(!mFinalListener, "overwriting a final listener");
  nsresult rv;
  nsRefPtr<nsNPAPIPluginInstance> inst;
  nsCOMPtr<nsIStreamListener> finalListener;
  rv = mInstanceOwner->GetInstance(getter_AddRefs(inst));
  NS_ENSURE_SUCCESS(rv, false);
  rv = pluginHost->NewPluginStreamListener(mURI, inst,
                                           getter_AddRefs(finalListener));
  NS_ENSURE_SUCCESS(rv, false);
  mFinalListener = finalListener;
  return true;
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
    webNav = do_GetInterface(currentDoc->GetWindow());
  }

  uint32_t supported;
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
    
    
    
    
    QueueCheckPluginStopEvent();
  } else if (mType != eType_Image) {
    
    
    
    
    UnloadObject();
  }
  nsCOMPtr<nsIRunnable> ev = new nsSimplePluginEvent(thisContent->GetCurrentDoc(),
                                           NS_LITERAL_STRING("PluginRemoved"));
  NS_DispatchToCurrentThread(ev);
}

nsObjectLoadingContent::nsObjectLoadingContent()
  : mType(eType_Loading)
  , mFallbackType(eFallbackAlternate)
  , mChannelLoaded(false)
  , mInstantiating(false)
  , mNetworkCreated(true)
  , mActivated(false)
  , mPlayPreviewCanceled(false)
  , mIsStopping(false)
  , mIsLoading(false)
  , mScriptRequested(false) {}

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
nsObjectLoadingContent::InstantiatePluginInstance(bool aIsLoading)
{
  if (mInstanceOwner || mType != eType_Plugin || (mIsLoading != aIsLoading) ||
      mInstantiating) {
    
    
    
    NS_ASSERTION(mIsLoading || !aIsLoading,
                 "aIsLoading should only be true inside LoadObject");
    return NS_OK;
  }

  mInstantiating = true;
  AutoSetInstantiatingToFalse autoInstantiating(this);

  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent *>(this));

  nsIDocument* doc = thisContent->GetCurrentDoc();
  if (!doc || !InActiveDocument(thisContent)) {
    NS_ERROR("Shouldn't be calling "
             "InstantiatePluginInstance without an active document");
    return NS_ERROR_FAILURE;
  }

  
  
  
  nsCOMPtr<nsIObjectLoadingContent> kungFuDeathGrip = this;

  
  
  doc->FlushPendingNotifications(Flush_Layout);
  
  NS_ENSURE_TRUE(mInstantiating, NS_OK);

  if (!thisContent->GetPrimaryFrame()) {
    LOG(("OBJLC [%p]: Not instantiating plugin with no frame", this));
    return NS_OK;
  }

  nsresult rv = NS_ERROR_FAILURE;
  nsRefPtr<nsPluginHost> pluginHost = nsPluginHost::GetInst();

  if (!pluginHost) {
    NS_NOTREACHED("No pluginhost");
    return NS_ERROR_FAILURE;
  }

  
  
  
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    appShell->SuspendNative();
  }

  nsRefPtr<nsPluginInstanceOwner> newOwner;
  rv = pluginHost->InstantiatePluginInstance(mContentType.get(),
                                             mURI.get(), this,
                                             getter_AddRefs(newOwner));

  
  if (appShell) {
    appShell->ResumeNative();
  }

  if (!mInstantiating || NS_FAILED(rv)) {
    LOG(("OBJLC [%p]: Plugin instantiation failed or re-entered, "
         "killing old instance", this));
    
    
    
    if (newOwner) {
      nsRefPtr<nsNPAPIPluginInstance> inst;
      newOwner->GetInstance(getter_AddRefs(inst));
      newOwner->SetFrame(nullptr);
      if (inst) {
        pluginHost->StopPluginInstance(inst);
      }
      newOwner->Destroy();
    }
    return NS_OK;
  }

  mInstanceOwner = newOwner;

  
  
  
  nsIFrame* frame = thisContent->GetPrimaryFrame();
  if (frame && mInstanceOwner) {
    mInstanceOwner->SetFrame(static_cast<nsObjectFrame*>(frame));

    
    
    mInstanceOwner->CallSetWindow();
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
      uint32_t blockState = nsIBlocklistService::STATE_NOT_BLOCKED;
      blocklist->GetPluginBlocklistState(pluginTag, EmptyString(),
                                         EmptyString(), &blockState);
      if (blockState == nsIBlocklistService::STATE_OUTDATED) {
        
        LOG(("OBJLC [%p]: Dispatching plugin outdated event for content %p\n",
             this));
        nsCOMPtr<nsIRunnable> ev = new nsSimplePluginEvent(thisContent,
                                                     NS_LITERAL_STRING("PluginOutdated"));
        nsresult rv = NS_DispatchToCurrentThread(ev);
        if (NS_FAILED(rv)) {
          NS_WARNING("failed to dispatch nsSimplePluginEvent");
        }
      }
    }

    
    
    
    
    if ((mURI && !mChannelLoaded) || (mChannelLoaded && !aIsLoading)) {
      NS_ASSERTION(!mChannel, "should not have an existing channel here");
      
      
      OpenChannel();
    }
  }

  nsCOMPtr<nsIRunnable> ev = new nsSimplePluginEvent(thisContent,
    NS_LITERAL_STRING("PluginInstantiated"));
  NS_DispatchToCurrentThread(ev);

  return NS_OK;
}

void
nsObjectLoadingContent::NotifyOwnerDocumentActivityChanged()
{

  
  
  if (mInstanceOwner)
    QueueCheckPluginStopEvent();
}


NS_IMETHODIMP
nsObjectLoadingContent::OnStartRequest(nsIRequest *aRequest,
                                       nsISupports *aContext)
{
  PROFILER_LABEL("nsObjectLoadingContent", "OnStartRequest");

  LOG(("OBJLC [%p]: Channel OnStartRequest", this));

  if (aRequest != mChannel || !aRequest) {
    
    return NS_BINDING_ABORTED;
  }

  
  
  if (mType == eType_Plugin) {
    if (!mInstanceOwner) {
      
      NS_NOTREACHED("Opened a channel in plugin mode, but don't have a plugin");
      return NS_BINDING_ABORTED;
    }
    if (MakePluginListener()) {
      return mFinalListener->OnStartRequest(aRequest, nullptr);
    } else {
      NS_NOTREACHED("Failed to create PluginStreamListener, aborting channel");
      return NS_BINDING_ABORTED;
    }
  }

  
  if (mType != eType_Loading) {
    NS_NOTREACHED("Should be type loading at this point");
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
    
    nsCOMPtr<nsIStreamListener> listenerGrip(mFinalListener);
    mFinalListener = nullptr;
    listenerGrip->OnStopRequest(aRequest, aContext, aStatusCode);
  }

  
  return NS_OK;
}



NS_IMETHODIMP
nsObjectLoadingContent::OnDataAvailable(nsIRequest *aRequest,
                                        nsISupports *aContext,
                                        nsIInputStream *aInputStream,
                                        uint64_t aOffset, uint32_t aCount)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  if (aRequest != mChannel) {
    return NS_BINDING_ABORTED;
  }

  if (mFinalListener) {
    
    nsCOMPtr<nsIStreamListener> listenerGrip(mFinalListener);
    return listenerGrip->OnDataAvailable(aRequest, aContext, aInputStream,
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
  nsRefPtr<nsFrameLoader> loader = mFrameLoader;
  return loader.forget();
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
nsObjectLoadingContent::GetDisplayedType(uint32_t* aType)
{
  *aType = DisplayedType();
  return NS_OK;
}

NS_IMETHODIMP
nsObjectLoadingContent::HasNewFrame(nsIObjectFrame* aFrame)
{
  if (mType != eType_Plugin) {
    return NS_OK;
  }

  if (!aFrame) {
    
    
    
    if (mInstanceOwner) {
      mInstanceOwner->SetFrame(nullptr);
      QueueCheckPluginStopEvent();
    }
    return NS_OK;
  }

  

  if (!mInstanceOwner) {
    
    
    AsyncStartPluginInstance();
    return NS_OK;
  }

  
  
  nsObjectFrame *objFrame = static_cast<nsObjectFrame*>(aFrame);
  mInstanceOwner->SetFrame(objFrame);

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
                                                  uint32_t* aType)
{
  *aType = GetTypeOfContent(PromiseFlatCString(aMIMEType));
  return NS_OK;
}

NS_IMETHODIMP
nsObjectLoadingContent::GetBaseURI(nsIURI **aResult)
{
  NS_IF_ADDREF(*aResult = mBaseURI);
  return NS_OK;
}





class ObjectInterfaceRequestorShim MOZ_FINAL : public nsIInterfaceRequestor,
                                               public nsIChannelEventSink,
                                               public nsIStreamListener
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(ObjectInterfaceRequestorShim,
                                           nsIInterfaceRequestor)
  NS_DECL_NSIINTERFACEREQUESTOR
  
  
  NS_FORWARD_NSICHANNELEVENTSINK(static_cast<nsObjectLoadingContent *>
                                 (mContent.get())->)
  NS_FORWARD_NSISTREAMLISTENER  (static_cast<nsObjectLoadingContent *>
                                 (mContent.get())->)
  NS_FORWARD_NSIREQUESTOBSERVER (static_cast<nsObjectLoadingContent *>
                                 (mContent.get())->)

  ObjectInterfaceRequestorShim(nsIObjectLoadingContent* aContent)
    : mContent(aContent)
  {}

protected:
  nsCOMPtr<nsIObjectLoadingContent> mContent;
};

NS_IMPL_CYCLE_COLLECTION_1(ObjectInterfaceRequestorShim, mContent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ObjectInterfaceRequestorShim)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIInterfaceRequestor)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(ObjectInterfaceRequestorShim)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ObjectInterfaceRequestorShim)

NS_IMETHODIMP
ObjectInterfaceRequestorShim::GetInterface(const nsIID & aIID, void **aResult)
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
                                               uint32_t aFlags,
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
        case eFallbackPlayPreview:
          return NS_EVENT_STATE_TYPE_PLAY_PREVIEW;
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


bool
nsObjectLoadingContent::CheckJavaCodebase()
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  nsCOMPtr<nsIScriptSecurityManager> secMan =
    nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsINetUtil> netutil = do_GetNetUtil();
  NS_ASSERTION(thisContent && secMan && netutil, "expected interfaces");


  
  
  nsresult rv = secMan->CheckLoadURIWithPrincipal(thisContent->NodePrincipal(),
                                                  mBaseURI, 0);
  if (NS_FAILED(rv)) {
    LOG(("OBJLC [%p]: Java codebase check failed", this));
    return false;
  }

  nsCOMPtr<nsIURI> principalBaseURI;
  rv = thisContent->NodePrincipal()->GetURI(getter_AddRefs(principalBaseURI));
  if (NS_FAILED(rv)) {
    NS_NOTREACHED("Failed to URI from node principal?");
    return false;
  }
  
  
  if (NS_URIIsLocalFile(mBaseURI) &&
      nsScriptSecurityManager::GetStrictFileOriginPolicy() &&
      !NS_RelaxStrictFileOriginPolicy(mBaseURI, principalBaseURI, true)) {
    LOG(("OBJLC [%p]: Java failed RelaxStrictFileOriginPolicy for file URI",
         this));
    return false;
  }

  return true;
}

bool
nsObjectLoadingContent::CheckLoadPolicy(int16_t *aContentPolicy)
{
  if (!aContentPolicy || !mURI) {
    NS_NOTREACHED("Doing it wrong");
    return false;
  }

  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "Must be an instance of content");

  nsIDocument* doc = thisContent->OwnerDoc();

  *aContentPolicy = nsIContentPolicy::ACCEPT;
  nsresult rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OBJECT,
                                          mURI,
                                          doc->NodePrincipal(),
                                          thisContent,
                                          mContentType,
                                          nullptr, 
                                          aContentPolicy,
                                          nsContentUtils::GetContentPolicy(),
                                          nsContentUtils::GetSecurityManager());
  NS_ENSURE_SUCCESS(rv, false);
  if (NS_CP_REJECTED(*aContentPolicy)) {
    nsAutoCString uri;
    nsAutoCString baseUri;
    mURI->GetSpec(uri);
    mURI->GetSpec(baseUri);
    LOG(("OBJLC [%p]: Content policy denied load of %s (base %s)",
         this, uri.get(), baseUri.get()));
    return false;
  }

  return true;
}

bool
nsObjectLoadingContent::CheckProcessPolicy(int16_t *aContentPolicy)
{
  if (!aContentPolicy) {
    NS_NOTREACHED("Null out variable");
    return false;
  }

  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "Must be an instance of content");

  nsIDocument* doc = thisContent->OwnerDoc();
  
  int32_t objectType;
  switch (mType) {
    case eType_Image:
      objectType = nsIContentPolicy::TYPE_IMAGE;
      break;
    case eType_Document:
      objectType = nsIContentPolicy::TYPE_DOCUMENT;
      break;
    case eType_Plugin:
      objectType = nsIContentPolicy::TYPE_OBJECT;
      break;
    default:
      NS_NOTREACHED("Calling checkProcessPolicy with a unloadable type");
      return false;
  }

  *aContentPolicy = nsIContentPolicy::ACCEPT;
  nsresult rv =
    NS_CheckContentProcessPolicy(objectType,
                                 mURI ? mURI : mBaseURI,
                                 doc->NodePrincipal(),
                                 static_cast<nsIImageLoadingContent*>(this),
                                 mContentType,
                                 nullptr, 
                                 aContentPolicy,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  NS_ENSURE_SUCCESS(rv, false);

  if (NS_CP_REJECTED(*aContentPolicy)) {
    LOG(("OBJLC [%p]: CheckContentProcessPolicy rejected load", this));
    return false;
  }

  return true;
}

nsObjectLoadingContent::ParameterUpdateFlags
nsObjectLoadingContent::UpdateObjectParameters(bool aJavaURI)
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "Must be an instance of content");

  uint32_t caps = GetCapabilities();
  LOG(("OBJLC [%p]: Updating object parameters", this));

  nsresult rv;
  nsAutoCString newMime;
  nsCOMPtr<nsIURI> newURI;
  nsCOMPtr<nsIURI> newBaseURI;
  ObjectType newType;
  bool isJava = false;
  
  bool stateInvalid = false;
  
  
  
  
  
  
  
  
  
  
  
  nsObjectLoadingContent::ParameterUpdateFlags retval = eParamNoChange;

  
  
  
  if (aJavaURI || thisContent->NodeInfo()->Equals(nsGkAtoms::applet)) {
    newMime.AssignLiteral("application/x-java-vm");
    isJava = true;
  } else {
    nsAutoString typeAttr;
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type, typeAttr);
    if (!typeAttr.IsEmpty()) {
      CopyUTF16toUTF8(typeAttr, newMime);
      isJava = nsPluginHost::IsJavaMIMEType(newMime.get());
    }
  }

  
  
  

  if (caps & eSupportClassID) {
    nsAutoString classIDAttr;
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::classid, classIDAttr);
    if (!classIDAttr.IsEmpty()) {
      
      if (StringBeginsWith(classIDAttr, NS_LITERAL_STRING("java:")) &&
          PluginExistsForType("application/x-java-vm")) {
        newMime.Assign("application/x-java-vm");
        isJava = true;
      } else {
        
        
        
        newMime.Truncate();
        stateInvalid = true;
      }
    }
  }

  
  
  

  nsAutoString codebaseStr;
  nsCOMPtr<nsIURI> docBaseURI = thisContent->GetBaseURI();
  bool hasCodebase = thisContent->HasAttr(kNameSpaceID_None, nsGkAtoms::codebase);
  if (hasCodebase)
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::codebase, codebaseStr);


  
  
  
  if (isJava) {
    
    
    nsCOMArray<nsIDOMElement> ourParams;
    nsCOMPtr<nsIDOMElement> mydomElement = do_QueryInterface(thisContent);

    nsCOMPtr<nsIDOMHTMLCollection> allParams;
    NS_NAMED_LITERAL_STRING(xhtml_ns, "http://www.w3.org/1999/xhtml");
    mydomElement->GetElementsByTagNameNS(xhtml_ns, NS_LITERAL_STRING("param"),
                                         getter_AddRefs(allParams));
    if (allParams) {
      uint32_t numAllParams;
      allParams->GetLength(&numAllParams);
      for (uint32_t i = 0; i < numAllParams; i++) {
        nsCOMPtr<nsIDOMNode> pnode;
        allParams->Item(i, getter_AddRefs(pnode));
        nsCOMPtr<nsIDOMElement> domelement = do_QueryInterface(pnode);
        if (domelement) {
          nsAutoString name;
          domelement->GetAttribute(NS_LITERAL_STRING("name"), name);
          name.Trim(" \n\r\t\b", true, true, false);
          if (name.EqualsIgnoreCase("codebase")) {
            
            nsCOMPtr<nsIDOMNode> parent;
            nsCOMPtr<nsIDOMHTMLObjectElement> domobject;
            nsCOMPtr<nsIDOMHTMLAppletElement> domapplet;
            pnode->GetParentNode(getter_AddRefs(parent));
            while (!(domobject || domapplet) && parent) {
              domobject = do_QueryInterface(parent);
              domapplet = do_QueryInterface(parent);
              nsCOMPtr<nsIDOMNode> temp;
              parent->GetParentNode(getter_AddRefs(temp));
              parent = temp;
            }
            if (domapplet || domobject) {
              if (domapplet) {
                parent = do_QueryInterface(domapplet);
              }
              else {
                parent = do_QueryInterface(domobject);
              }
              nsCOMPtr<nsIDOMNode> mydomNode = do_QueryInterface(mydomElement);
              if (parent == mydomNode) {
                hasCodebase = true;
                domelement->GetAttribute(NS_LITERAL_STRING("value"),
                                         codebaseStr);
                codebaseStr.Trim(" \n\r\t\b", true, true, false);
              }
            }
          }
        }
      }
    }
  }

  if (isJava && hasCodebase && codebaseStr.IsEmpty()) {
    
    codebaseStr.AssignLiteral("/");
    
    
    
  } else if (isJava && !hasCodebase) {
    
    
    codebaseStr.AssignLiteral(".");
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
  
  if (isJava) {
    
    
    
    
    
  } else if (thisContent->NodeInfo()->Equals(nsGkAtoms::object)) {
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::data, uriStr);
  } else if (thisContent->NodeInfo()->Equals(nsGkAtoms::embed)) {
    thisContent->GetAttr(kNameSpaceID_None, nsGkAtoms::src, uriStr);
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

  
  
  if (GetTypeOfContent(newMime) != eType_Plugin && newURI &&
      (caps & eAllowPluginSkipChannel) &&
      IsPluginEnabledByExtension(newURI, newMime)) {
    LOG(("OBJLC [%p]: Using extension as type hint (%s)", this, newMime.get()));
    if (!isJava && nsPluginHost::IsJavaMIMEType(newMime.get())) {
      return UpdateObjectParameters(true);
    }
  }

  
  
  
  

  if ((mOriginalContentType != newMime) || !URIEquals(mOriginalURI, newURI)) {
    
    
    
    
    
    retval = (ParameterUpdateFlags)(retval | eParamChannelChanged);
    LOG(("OBJLC [%p]: Channel parameters changed", this));
  }
  mOriginalContentType = newMime;
  mOriginalURI = newURI;

  
  
  
  

  
  
  bool useChannel = mChannelLoaded && !(retval & eParamChannelChanged);
  
  
  bool newChannel = useChannel && mType == eType_Loading;

  if (newChannel && mChannel) {
    nsCString channelType;
    rv = mChannel->GetContentType(channelType);
    if (NS_FAILED(rv)) {
      NS_NOTREACHED("GetContentType failed");
      stateInvalid = true;
      channelType.Truncate();
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

    ObjectType typeHint = newMime.IsEmpty() ?
                          eType_Null : GetTypeOfContent(newMime);

    
    
    
    
    
    
    
    
    
    
    
    
    

    bool overrideChannelType = false;
    if (typeHint == eType_Plugin) {
      LOG(("OBJLC [%p]: Using plugin type hint in favor of any channel type",
           this));
      overrideChannelType = true;
    } else if ((caps & eAllowPluginSkipChannel) &&
               IsPluginEnabledByExtension(newURI, newMime)) {
      LOG(("OBJLC [%p]: Using extension as type hint for "
           "eAllowPluginSkipChannel tag (%s)", this, newMime.get()));
      overrideChannelType = true;
    } else if (binaryChannelType &&
               typeHint != eType_Null && typeHint != eType_Document) {
      LOG(("OBJLC [%p]: Using type hint in favor of binary channel type",
           this));
      overrideChannelType = true;
    } else if (binaryChannelType &&
               IsPluginEnabledByExtension(newURI, newMime)) {
      LOG(("OBJLC [%p]: Using extension as type hint for binary channel (%s)",
           this, newMime.get()));
      overrideChannelType = true;
    }

    if (overrideChannelType) {
      
      
      
      nsAutoCString parsedMime, dummy;
      NS_ParseContentType(newMime, parsedMime, dummy);
      if (!parsedMime.IsEmpty()) {
        mChannel->SetContentType(parsedMime);
      }
    } else {
      newMime = channelType;
      if (nsPluginHost::IsJavaMIMEType(newMime.get())) {
        
        
        
        
        LOG(("OBJLC [%p]: Refusing to load with channel with java MIME",
             this));
        stateInvalid = true;
      }
    }
  } else if (newChannel) {
    LOG(("OBJLC [%p]: We failed to open a channel, marking invalid", this));
    stateInvalid = true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (stateInvalid) {
    newType = eType_Null;
    newMime.Truncate();
  } else if (newChannel) {
      
      newType = GetTypeOfContent(newMime);
      LOG(("OBJLC [%p]: Using channel type", this));
  } else if (((caps & eAllowPluginSkipChannel) || !newURI) &&
             GetTypeOfContent(newMime) == eType_Plugin) {
    newType = eType_Plugin;
    LOG(("OBJLC [%p]: Plugin type with no URI, skipping channel load", this));
  } else if (newURI) {
    
    
    newType = eType_Loading;
  } else {
    
    
    newType = eType_Null;
  }

  
  
  

  if (useChannel && newType == eType_Loading) {
    
    
    newType = mType;
    newMime = mContentType;
    newURI = mURI;
  } else if (useChannel && !newChannel) {
    
    retval = (ParameterUpdateFlags)(retval | eParamChannelChanged);
    useChannel = false;
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

  
  
  
  
  if (mType != eType_Loading && mContentType != newMime) {
    retval = (ParameterUpdateFlags)(retval | eParamStateChanged);
    retval = (ParameterUpdateFlags)(retval | eParamContentTypeChanged);
    LOG(("OBJLC [%p]: Object effective mime type changed (%s -> %s)",
         this, mContentType.get(), newMime.get()));
    mContentType = newMime;
  }

  
  
  if (useChannel && !newChannel && (retval & eParamStateChanged)) {
    mType = eType_Loading;
    retval = (ParameterUpdateFlags)(retval | eParamChannelChanged);
  }

  return retval;
}



NS_IMETHODIMP
nsObjectLoadingContent::InitializeFromChannel(nsIRequest *aChannel)
{
  LOG(("OBJLC [%p] InitializeFromChannel: %p", this, aChannel));
  if (mType != eType_Loading || mChannel) {
    
    
    NS_NOTREACHED("Should not have begun loading at this point");
    return NS_ERROR_UNEXPECTED;
  }

  
  
  
  UpdateObjectParameters();
  
  mType = eType_Loading;
  mChannel = do_QueryInterface(aChannel);
  NS_ASSERTION(mChannel, "passed a request that is not a channel");

  
  
  
  return NS_OK;
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

  if (!stateChange && !aForceLoad) {
    return NS_OK;
  }

  
  
  
  LOG(("OBJLC [%p]: LoadObject - plugin state changed (%u)",
       this, stateChange));

  
  
  
  FallbackType fallbackType = eFallbackAlternate;

  
  
  
  if (mType == eType_Null && GetTypeOfContent(mContentType) == eType_Null) {
    fallbackType = eFallbackUnsupported;
  }

  
  if (mActivated && (stateChange & eParamContentTypeChanged)) {
    LOG(("OBJLC [%p]: Content type changed, clearing activation state", this));
    mActivated = false;
  }

  
  
  
  
  
  if (mIsLoading) {
    LOG(("OBJLC [%p]: Re-entering into LoadObject", this));
  }
  mIsLoading = true;
  AutoSetLoadingToFalse reentryCheck(this);

  
  
  UnloadObject(false); 
  if (!mIsLoading) {
    
    
    LOG(("OBJLC [%p]: Re-entered into LoadObject, aborting outer load", this));
    return NS_OK;
  }

  
  if (stateChange & eParamChannelChanged) {
    
    
    
    CloseChannel();
    mChannelLoaded = false;
  } else if (mType == eType_Null && mChannel) {
    
    
    
    CloseChannel();
  } else if (mType == eType_Loading && mChannel) {
    
    
    return NS_OK;
  } else if (mChannelLoaded && mChannel != aLoadingChannel) {
    
    
    
    NS_NOTREACHED("Loading with a channel, but state doesn't make sense");
    return NS_OK;
  }

  
  
  

  if (mType != eType_Null) {
    bool allowLoad = true;
    if (nsPluginHost::IsJavaMIMEType(mContentType.get())) {
      allowLoad = CheckJavaCodebase();
    }
    int16_t contentPolicy = nsIContentPolicy::ACCEPT;
    
    if (allowLoad && mURI && !mChannelLoaded) {
      allowLoad = CheckLoadPolicy(&contentPolicy);
    }
    
    
    
    if (allowLoad && mType != eType_Loading) {
      allowLoad = CheckProcessPolicy(&contentPolicy);
    }

    
    if (!mIsLoading) {
      LOG(("OBJLC [%p]: We re-entered in content policy, leaving original load",
           this));
      return NS_OK;
    }

    
    if (!allowLoad) {
      LOG(("OBJLC [%p]: Load denied by policy", this));
      mType = eType_Null;
      if (contentPolicy == nsIContentPolicy::REJECT_TYPE) {
        
        
        
        fallbackType = eFallbackUserDisabled;
      } else {
        fallbackType = eFallbackSuppressed;
      }
    }
  }

  
  
  
  
  FallbackType clickToPlayReason;
  if (!mActivated && (mType == eType_Null || mType == eType_Plugin) &&
      !ShouldPlay(clickToPlayReason, false)) {
    LOG(("OBJLC [%p]: Marking plugin as click-to-play", this));
    mType = eType_Null;
    fallbackType = clickToPlayReason;
  }

  if (!mActivated && mType == eType_Plugin) {
    
    
    LOG(("OBJLC [%p]: Object implicitly activated", this));
    mActivated = true;
  }

  
  
  if (mFrameLoader || mPendingInstantiateEvent || mInstanceOwner ||
      mPendingCheckPluginStopEvent || mFinalListener)
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

  
  
  

  
  
  nsCOMPtr<nsIStreamListener> finalListener;
  
  
  bool doSpawnPlugin = false;
  switch (mType) {
    case eType_Image:
      if (!mChannel) {
        
        
        NS_NOTREACHED("Attempting to load image without a channel?");
        rv = NS_ERROR_UNEXPECTED;
        break;
      }
      rv = LoadImageWithChannel(mChannel, getter_AddRefs(finalListener));
      
    break;
    case eType_Plugin:
    {
      if (mChannel) {
        
        NotifyStateChanged(oldType, oldState, true, aNotify);
        oldType = mType;
        oldState = ObjectState();

        if (!thisContent->GetPrimaryFrame()) {
          
          
          LOG(("OBJLC [%p]: Aborting load - plugin-type, but no frame", this));
          CloseChannel();
          break;
        }

        
        doSpawnPlugin = true;
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
        LOG(("OBJLC [%p]: Aborting recursive load", this));
        mFrameLoader->Destroy();
        mFrameLoader = nullptr;
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
                                  getter_AddRefs(finalListener));
      
    }
    break;
    case eType_Loading:
      
      rv = OpenChannel();
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

    
    doSpawnPlugin = false;
    finalListener = nullptr;

    
    
    LoadFallback(fallbackType, false);
  }

  
  NotifyStateChanged(oldType, oldState, false, aNotify);
  NS_ENSURE_TRUE(mIsLoading, NS_OK);


  
  
  
  
  
  
  
  

  rv = NS_OK;
  if (doSpawnPlugin) {
    rv = InstantiatePluginInstance(true);
    NS_ENSURE_TRUE(mIsLoading, NS_OK);
    
    
    if (aLoadingChannel && NS_SUCCEEDED(rv)) {
      if (NS_SUCCEEDED(rv) && MakePluginListener()) {
        rv = mFinalListener->OnStartRequest(mChannel, nullptr);
        if (NS_FAILED(rv)) {
          
          CloseChannel();
          NS_ENSURE_TRUE(mIsLoading, NS_OK);
          rv = NS_OK;
        }
      }
    }
  } else if (finalListener) {
    NS_ASSERTION(mType != eType_Null && mType != eType_Loading,
                 "We should not have a final listener with a non-loaded type");
    mFinalListener = finalListener;
    rv = finalListener->OnStartRequest(mChannel, nullptr);
  }

  if (NS_FAILED(rv) && mIsLoading) {
    
    
    mType = eType_Null;
    UnloadObject(false);
    NS_ENSURE_TRUE(mIsLoading, NS_OK);
    CloseChannel();
    LoadFallback(fallbackType, true);
  }

  return NS_OK;
}


nsresult
nsObjectLoadingContent::CloseChannel()
{
  if (mChannel) {
    LOG(("OBJLC [%p]: Closing channel\n", this));
    
    
    nsCOMPtr<nsIChannel> channelGrip(mChannel);
    nsCOMPtr<nsIStreamListener> listenerGrip(mFinalListener);
    mChannel = nullptr;
    mFinalListener = nullptr;
    channelGrip->Cancel(NS_BINDING_ABORTED);
    if (listenerGrip) {
      
      
      listenerGrip->OnStopRequest(channelGrip, nullptr, NS_BINDING_ABORTED);
    }
  }
  return NS_OK;
}

nsresult
nsObjectLoadingContent::OpenChannel()
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  nsCOMPtr<nsIScriptSecurityManager> secMan =
    nsContentUtils::GetSecurityManager();
  NS_ASSERTION(thisContent, "must be a content");
  nsIDocument* doc = thisContent->OwnerDoc();
  NS_ASSERTION(doc, "No owner document?");

  nsresult rv;
  mChannel = nullptr;

  
  if (!mURI || !CanHandleURI(mURI)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  rv = secMan->CheckLoadURIWithPrincipal(thisContent->NodePrincipal(), mURI, 0);
  NS_ENSURE_SUCCESS(rv, rv);

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
  nsRefPtr<ObjectInterfaceRequestorShim> shim =
    new ObjectInterfaceRequestorShim(this);
  rv = NS_NewChannel(getter_AddRefs(chan), mURI, nullptr, group, shim,
                     nsIChannel::LOAD_CALL_CONTENT_SNIFFERS |
                     nsIChannel::LOAD_CLASSIFY_URI,
                     channelPolicy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIHttpChannel> httpChan(do_QueryInterface(chan));
  if (httpChan) {
    httpChan->SetReferrer(doc->GetDocumentURI());
  }

  
  
  
  
  nsCOMPtr<nsIPrincipal> ownerPrincipal;
  uint32_t sandboxFlags = doc->GetSandboxFlags();
  if (sandboxFlags & SANDBOXED_ORIGIN) {
    ownerPrincipal = do_CreateInstance("@mozilla.org/nullprincipal;1");
  } else {
    
    ownerPrincipal = thisContent->NodePrincipal();
  }
  nsContentUtils::SetUpChannelOwner(ownerPrincipal, chan, mURI, true,
                                    sandboxFlags & SANDBOXED_ORIGIN);

  nsCOMPtr<nsIScriptChannel> scriptChannel = do_QueryInterface(chan);
  if (scriptChannel) {
    
    scriptChannel->SetExecutionPolicy(nsIScriptChannel::EXECUTE_NORMAL);
  }

  
  rv = chan->AsyncOpen(shim, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);
  LOG(("OBJLC [%p]: Channel opened", this));
  mChannel = chan;
  return NS_OK;
}

uint32_t
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

  QueueCheckPluginStopEvent();
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
    if (mType != eType_Plugin) {
      
      
      CloseChannel();
    }
    mChannelLoaded = false;
    mType = eType_Loading;
    mURI = mOriginalURI = mBaseURI = nullptr;
    mContentType.Truncate();
    mOriginalContentType.Truncate();
  }

  
  
  mInstantiating = false;

  mScriptRequested = false;

  
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

nsObjectLoadingContent::ObjectType
nsObjectLoadingContent::GetTypeOfContent(const nsCString& aMIMEType)
{
  if (aMIMEType.IsEmpty()) {
    return eType_Null;
  }

  uint32_t caps = GetCapabilities();

  if ((caps & eSupportImages) && IsSupportedImage(aMIMEType)) {
    return eType_Image;
  }

  
  bool isSVG = aMIMEType.LowerCaseEqualsLiteral("image/svg+xml");
  Capabilities supportType = isSVG ? eSupportSVG : eSupportDocuments;
  if ((caps & supportType) && IsSupportedDocument(aMIMEType)) {
    return eType_Document;
  }

  if (caps & eSupportPlugins && PluginExistsForType(aMIMEType.get())) {
    
    return eType_Plugin;
  }

  return eType_Null;
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
nsObjectLoadingContent::PluginDestroyed()
{
  
  
  
  TeardownProtoChain();
  mInstanceOwner->Destroy();
  mInstanceOwner = nullptr;
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

  PluginDestroyed();

  
  LoadFallback(eFallbackCrashed, true);

  
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));

  
  
  nsAutoCString pluginName;
  aPluginTag->GetName(pluginName);
  nsAutoCString pluginFilename;
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

nsresult
nsObjectLoadingContent::ScriptRequestPluginInstance(JSContext* aCx,
                                                    nsNPAPIPluginInstance **aResult)
{
  
  
  
  
  
  MOZ_ASSERT_IF(nsContentUtils::GetCurrentJSContext(),
                aCx == nsContentUtils::GetCurrentJSContext());
  bool callerIsContentJS = (!nsContentUtils::IsCallerChrome() &&
                            !nsContentUtils::IsCallerXBL() &&
                            js::IsContextRunningJS(aCx));

  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));

  *aResult = nullptr;

  
  
  
  if (callerIsContentJS && !mScriptRequested &&
      InActiveDocument(thisContent) && mType == eType_Null &&
      mFallbackType >= eFallbackClickToPlay) {
    nsCOMPtr<nsIRunnable> ev =
      new nsSimplePluginEvent(thisContent,
                              NS_LITERAL_STRING("PluginScripted"));
    nsresult rv = NS_DispatchToCurrentThread(ev);
    if (NS_FAILED(rv)) {
      NS_NOTREACHED("failed to dispatch PluginScripted event");
    }
    mScriptRequested = true;
  } else if (mType == eType_Plugin && !mInstanceOwner &&
             nsContentUtils::IsSafeToRunScript() &&
             InActiveDocument(thisContent)) {
    
    
    SyncStartPluginInstance();
  }

  if (mInstanceOwner) {
    return mInstanceOwner->GetInstance(aResult);
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
  
  if (mInstanceOwner || mPendingInstantiateEvent) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
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
  NS_IF_ADDREF(*aURI = GetSrcURI());
  return NS_OK;
}

static bool
DoDelayedStop(nsPluginInstanceOwner* aInstanceOwner,
              nsObjectLoadingContent* aContent,
              bool aDelayedStop)
{
  
  
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

  if (!thisContent->IsHTML() || mContentType.IsEmpty()) {
    
    
    aType = eFallbackAlternate;
  }

  if (thisContent->Tag() == nsGkAtoms::object &&
      (aType == eFallbackUnsupported ||
       aType == eFallbackDisabled ||
       aType == eFallbackBlocklisted))
  {
    
    for (nsIContent* child = thisContent->GetFirstChild();
         child; child = child->GetNextSibling()) {
      if (!child->IsHTML(nsGkAtoms::param) &&
          nsStyleUtil::IsSignificantChild(child, true, false)) {
        aType = eFallbackAlternate;
        break;
      }
    }
  }

  mType = eType_Null;
  mFallbackType = aType;

  
  if (!aNotify) {
    return; 
  }

  NotifyStateChanged(oldType, oldState, false, true);
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

    nsRefPtr<nsPluginHost> pluginHost = nsPluginHost::GetInst();
    NS_ASSERTION(pluginHost, "No plugin host?");
    pluginHost->StopPluginInstance(inst);
  }
  TeardownProtoChain();
  aInstanceOwner->Destroy();

  mIsStopping = false;
}

NS_IMETHODIMP
nsObjectLoadingContent::StopPluginInstance()
{
  
  mPendingInstantiateEvent = nullptr;
  mPendingCheckPluginStopEvent = nullptr;

  if (!mInstanceOwner) {
    return NS_OK;
  }

  if (mChannel) {
    
    
    
    
    
    LOG(("OBJLC [%p]: StopPluginInstance - Closing used channel", this));
    CloseChannel();
  }

  
  
  mInstanceOwner->SetFrame(nullptr);

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

  nsRefPtr<nsPluginInstanceOwner> ownerGrip(mInstanceOwner);
  mInstanceOwner = nullptr;

  
  DoStopPlugin(ownerGrip, delayedStop);

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

  nsCOMPtr<nsIScriptGlobalObject> sgo =  do_QueryInterface(doc->GetScopeObject());
  if (!sgo)
    return;

  nsIScriptContext *scx = sgo->GetContext();
  if (!scx)
    return;

  JSContext *cx = scx->GetNativeContext();
  nsCxPusher pusher;
  pusher.Push(cx);

  JS::Rooted<JSObject*> obj(cx, thisContent->GetWrapper());
  if (!obj) {
    
    
    return;
  }

  SetupProtoChain(cx, obj);
}

NS_IMETHODIMP
nsObjectLoadingContent::PlayPlugin()
{
  if (!nsContentUtils::IsCallerChrome())
    return NS_OK;

  if (!mActivated) {
    mActivated = true;
    LOG(("OBJLC [%p]: Activated by user", this));
  }

  
  
  
  if (mType == eType_Null && mFallbackType >= eFallbackClickToPlay) {
    return LoadObject(true, true);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsObjectLoadingContent::GetActivated(bool *aActivated)
{
  *aActivated = Activated();
  return NS_OK;
}

NS_IMETHODIMP
nsObjectLoadingContent::GetPluginFallbackType(uint32_t* aPluginFallbackType)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);
  *aPluginFallbackType = mFallbackType;
  return NS_OK;
}

uint32_t
nsObjectLoadingContent::DefaultFallbackType()
{
  FallbackType reason;
  bool go = ShouldPlay(reason, true);
  if (go) {
    return PLUGIN_ACTIVE;
  }
  return reason;
}

NS_IMETHODIMP
nsObjectLoadingContent::GetHasRunningPlugin(bool *aHasPlugin)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);
  *aHasPlugin = HasRunningPlugin();
  return NS_OK;
}

NS_IMETHODIMP
nsObjectLoadingContent::CancelPlayPreview()
{
  if (!nsContentUtils::IsCallerChrome())
    return NS_ERROR_NOT_AVAILABLE;

  mPlayPreviewCanceled = true;

  
  if (mType == eType_Null && mFallbackType == eFallbackPlayPreview) {
    return LoadObject(true, true);
  }

  return NS_OK;
}

static bool sPrefsInitialized;
static uint32_t sSessionTimeoutMinutes;
static uint32_t sPersistentTimeoutDays;

bool
nsObjectLoadingContent::ShouldPlay(FallbackType &aReason, bool aIgnoreCurrentType)
{
  nsresult rv;

  if (!sPrefsInitialized) {
    Preferences::AddUintVarCache(&sSessionTimeoutMinutes,
                                 "plugin.sessionPermissionNow.intervalInMinutes", 60);
    Preferences::AddUintVarCache(&sPersistentTimeoutDays,
                                 "plugin.persistentPermissionAlways.intervalInDays", 90);
    sPrefsInitialized = true;
  }

  nsRefPtr<nsPluginHost> pluginHost = nsPluginHost::GetInst();

  nsCOMPtr<nsIPluginPlayPreviewInfo> playPreviewInfo;
  bool isPlayPreviewSpecified = NS_SUCCEEDED(pluginHost->GetPlayPreviewInfo(
    mContentType, getter_AddRefs(playPreviewInfo)));
  bool ignoreCTP = false;
  if (isPlayPreviewSpecified) {
    playPreviewInfo->GetIgnoreCTP(&ignoreCTP);
  }
  if (isPlayPreviewSpecified && !mPlayPreviewCanceled &&
      ignoreCTP) {
    
    
    aReason = eFallbackPlayPreview;
    return false;
  }
  
  if (!aIgnoreCurrentType && mType != eType_Plugin) {
    return true;
  }

  
  
  
  
  
  
  
  
  
  

  aReason = eFallbackClickToPlay;

  uint32_t enabledState = nsIPluginTag::STATE_DISABLED;
  pluginHost->GetStateForType(mContentType, &enabledState);
  if (nsIPluginTag::STATE_DISABLED == enabledState) {
    aReason = eFallbackDisabled;
    return false;
  }

  
  
  uint32_t blocklistState = nsIBlocklistService::STATE_NOT_BLOCKED;
  pluginHost->GetBlocklistStateForType(mContentType.get(), &blocklistState);
  if (blocklistState == nsIBlocklistService::STATE_BLOCKED) {
    
    aReason = eFallbackBlocklisted;
    return false;
  }

  if (blocklistState == nsIBlocklistService::STATE_VULNERABLE_UPDATE_AVAILABLE) {
    aReason = eFallbackVulnerableUpdatable;
  }
  else if (blocklistState == nsIBlocklistService::STATE_VULNERABLE_NO_UPDATE) {
    aReason = eFallbackVulnerableNoUpdate;
  }

  if (aReason == eFallbackClickToPlay && isPlayPreviewSpecified &&
      !mPlayPreviewCanceled && !ignoreCTP) {
    
    aReason = eFallbackPlayPreview;
  }

  
  

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

  nsCOMPtr<nsIPermissionManager> permissionManager = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, false);

  
  
  
  
  
  if (!nsContentUtils::IsSystemPrincipal(topDoc->NodePrincipal())) {
    nsAutoCString permissionString;
    rv = pluginHost->GetPermissionStringForType(mContentType, permissionString);
    NS_ENSURE_SUCCESS(rv, false);
    uint32_t permission;
    rv = permissionManager->TestPermissionFromPrincipal(topDoc->NodePrincipal(),
                                                        permissionString.Data(),
                                                        &permission);
    NS_ENSURE_SUCCESS(rv, false);
    if (permission != nsIPermissionManager::UNKNOWN_ACTION) {
      uint64_t nowms = PR_Now() / 1000;
      permissionManager->UpdateExpireTime(
        topDoc->NodePrincipal(), permissionString.Data(), false,
        nowms + sSessionTimeoutMinutes * 60 * 1000,
        nowms / 1000 + uint64_t(sPersistentTimeoutDays) * 24 * 60 * 60 * 1000);
    }
    switch (permission) {
    case nsIPermissionManager::ALLOW_ACTION:
      return true;
    case nsIPermissionManager::DENY_ACTION:
      aReason = eFallbackDisabled;
      return false;
    case nsIPermissionManager::PROMPT_ACTION:
      return false;
    case nsIPermissionManager::UNKNOWN_ACTION:
      break;
    default:
      MOZ_ASSERT(false);
      return false;
    }
  }

  
  if (blocklistState == nsIBlocklistService::STATE_VULNERABLE_UPDATE_AVAILABLE ||
      blocklistState == nsIBlocklistService::STATE_VULNERABLE_NO_UPDATE) {
    return false;
  }

  switch (enabledState) {
  case nsIPluginTag::STATE_ENABLED:
    return true;
  case nsIPluginTag::STATE_CLICKTOPLAY:
    return false;
  }
  MOZ_CRASH("Unexpected enabledState");
}

nsIDocument*
nsObjectLoadingContent::GetContentDocument()
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));

  if (!thisContent->IsInDoc()) {
    return nullptr;
  }

  
  nsIDocument *sub_doc = thisContent->OwnerDoc()->GetSubDocumentFor(thisContent);
  if (!sub_doc) {
    return nullptr;
  }

  
  if (!nsContentUtils::GetSubjectPrincipal()->Subsumes(sub_doc->NodePrincipal())) {
    return nullptr;
  }

  return sub_doc;
}

JS::Value
nsObjectLoadingContent::LegacyCall(JSContext* aCx,
                                   JS::Handle<JS::Value> aThisVal,
                                   const Sequence<JS::Value>& aArguments,
                                   ErrorResult& aRv)
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  JS::Rooted<JSObject*> obj(aCx, thisContent->GetWrapper());
  MOZ_ASSERT(obj, "How did we get called?");

  
  
  
  
  if (!JS_WrapObject(aCx, obj.address())) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return JS::UndefinedValue();
  }

  if (nsDOMClassInfo::ObjectIsNativeWrapper(aCx, obj)) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);
    return JS::UndefinedValue();
  }

  obj = thisContent->GetWrapper();
  
  JSAutoCompartment ac(aCx, obj);
  nsTArray<JS::Value> args(aArguments);
  JS::AutoArrayRooter rooter(aCx, args.Length(), args.Elements());
  for (JS::Value *arg = args.Elements(), *arg_end = arg + args.Length();
       arg != arg_end;
       ++arg) {
    if (!JS_WrapValue(aCx, arg)) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return JS::UndefinedValue();
    }
  }

  JS::Rooted<JS::Value> thisVal(aCx, aThisVal);
  if (!JS_WrapValue(aCx, thisVal.address())) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return JS::UndefinedValue();
  }

  nsRefPtr<nsNPAPIPluginInstance> pi;
  nsresult rv = ScriptRequestPluginInstance(aCx, getter_AddRefs(pi));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return JS::UndefinedValue();
  }

  
  if (!pi) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);
    return JS::UndefinedValue();
  }

  JS::Rooted<JSObject*> pi_obj(aCx);
  JS::Rooted<JSObject*> pi_proto(aCx);

  rv = GetPluginJSObject(aCx, obj, pi, &pi_obj, &pi_proto);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return JS::UndefinedValue();
  }

  if (!pi_obj) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);
    return JS::UndefinedValue();
  }

  JS::Rooted<JS::Value> retval(aCx);
  bool ok = JS::Call(aCx, thisVal, pi_obj, args.Length(), args.Elements(),
                     &retval);
  if (!ok) {
    aRv.Throw(NS_ERROR_FAILURE);
    return JS::UndefinedValue();
  }

  Telemetry::Accumulate(Telemetry::PLUGIN_CALLED_DIRECTLY, true);
  return retval;
}

void
nsObjectLoadingContent::SetupProtoChain(JSContext* aCx,
                                        JS::Handle<JSObject*> aObject)
{
  MOZ_ASSERT(nsCOMPtr<nsIContent>(do_QueryInterface(
    static_cast<nsIObjectLoadingContent*>(this)))->IsDOMBinding());

  if (mType != eType_Plugin) {
    return;
  }

  if (!nsContentUtils::IsSafeToRunScript()) {
    
    
    nsCOMPtr<nsIScriptContext> scriptContext = GetScriptContextFromJSContext(aCx);

    nsRefPtr<SetupProtoChainRunner> runner =
      new SetupProtoChainRunner(scriptContext, this);
    nsContentUtils::AddScriptRunner(runner);
    return;
  }

  
  
  
  MOZ_ASSERT(aCx == nsContentUtils::GetCurrentJSContext());

  JSAutoCompartment ac(aCx, aObject);

  nsRefPtr<nsNPAPIPluginInstance> pi;
  nsresult rv = ScriptRequestPluginInstance(aCx, getter_AddRefs(pi));
  if (NS_FAILED(rv)) {
    return;
  }

  if (!pi) {
    
    return;
  }

  JS::Rooted<JSObject*> pi_obj(aCx); 
  JS::Rooted<JSObject*> pi_proto(aCx); 

  rv = GetPluginJSObject(aCx, aObject, pi, &pi_obj, &pi_proto);
  if (NS_FAILED(rv)) {
    return;
  }

  if (!pi_obj) {
    
    return;
  }

  
  

  JS::Rooted<JSObject*> global(aCx, JS_GetGlobalForObject(aCx, aObject));
  JS::Handle<JSObject*> my_proto = GetDOMClass(aObject)->mGetProto(aCx, global);
  MOZ_ASSERT(my_proto);

  
  if (!::JS_SetPrototype(aCx, aObject, pi_obj)) {
    return;
  }

  if (pi_proto && js::GetObjectClass(pi_proto) != js::ObjectClassPtr) {
    
    
    if (pi_proto != my_proto && !::JS_SetPrototype(aCx, pi_proto, my_proto)) {
      return;
    }
  } else {
    
    
    
    if (!::JS_SetPrototype(aCx, pi_obj, my_proto)) {
      return;
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
}


nsresult
nsObjectLoadingContent::GetPluginJSObject(JSContext *cx,
                                          JS::Handle<JSObject*> obj,
                                          nsNPAPIPluginInstance *plugin_inst,
                                          JS::MutableHandle<JSObject*> plugin_obj,
                                          JS::MutableHandle<JSObject*> plugin_proto)
{
  
  
  
  JSAutoCompartment ac(cx, obj);

  if (plugin_inst) {
    plugin_inst->GetJSObject(cx, plugin_obj.address());
    if (plugin_obj) {
      if (!::JS_GetPrototype(cx, plugin_obj, plugin_proto)) {
        return NS_ERROR_UNEXPECTED;
      }
    }
  }

  return NS_OK;
}

void
nsObjectLoadingContent::TeardownProtoChain()
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));

  
  
  AutoSafeJSContext cx;
  JS::Rooted<JSObject*> obj(cx, thisContent->GetWrapper());
  NS_ENSURE_TRUE(obj, );

  JS::Rooted<JSObject*> proto(cx);
  JSAutoCompartment ac(cx, obj);

  
  
  DebugOnly<bool> removed = false;
  while (obj) {
    if (!::JS_GetPrototype(cx, obj, &proto)) {
      return;
    }
    if (!proto) {
      break;
    }
    
    
    if (JS_GetClass(js::UncheckedUnwrap(proto)) == &sNPObjectJSWrapperClass) {
      
      if (!::JS_GetPrototype(cx, proto, &proto)) {
        return;
      }

      MOZ_ASSERT(!removed, "more than one NPObject in prototype chain");
      removed = true;

      
      ::JS_SetPrototype(cx, obj, proto);
    }

    obj = proto;
  }
}

bool
nsObjectLoadingContent::DoNewResolve(JSContext* aCx, JS::Handle<JSObject*> aObject,
                                     JS::Handle<jsid> aId,
                                     JS::MutableHandle<JS::Value> aValue)
{
  

  nsRefPtr<nsNPAPIPluginInstance> pi;
  nsresult rv = ScriptRequestPluginInstance(aCx, getter_AddRefs(pi));
  if (NS_FAILED(rv)) {
    return mozilla::dom::Throw(aCx, rv);
  }
  return true;
}

void
nsObjectLoadingContent::GetOwnPropertyNames(JSContext* aCx,
                                            nsTArray<nsString>& ,
                                            ErrorResult& aRv)
{
  
  
  
  nsRefPtr<nsNPAPIPluginInstance> pi;
  aRv = ScriptRequestPluginInstance(aCx, getter_AddRefs(pi));
}



nsObjectLoadingContent::SetupProtoChainRunner::SetupProtoChainRunner(
    nsIScriptContext* scriptContext,
    nsObjectLoadingContent* aContent)
  : mContext(scriptContext)
  , mContent(aContent)
{
}

NS_IMETHODIMP
nsObjectLoadingContent::SetupProtoChainRunner::Run()
{
  
  
  nsCxPusher pusher;
  JSContext* cx = mContext ? mContext->GetNativeContext()
                           : nsContentUtils::GetSafeJSContext();
  pusher.Push(cx);

  nsCOMPtr<nsIContent> content;
  CallQueryInterface(mContent.get(), getter_AddRefs(content));
  JS::Rooted<JSObject*> obj(cx, content->GetWrapper());
  if (!obj) {
    
    return NS_OK;
  }
  nsObjectLoadingContent* objectLoadingContent =
    static_cast<nsObjectLoadingContent*>(mContent.get());
  objectLoadingContent->SetupProtoChain(cx, obj);
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsObjectLoadingContent::SetupProtoChainRunner, nsIRunnable)

