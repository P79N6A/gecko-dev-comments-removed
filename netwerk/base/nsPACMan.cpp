





#include "nsPACMan.h"
#include "mozIApplication.h"
#include "nsIAppsService.h"
#include "nsThreadUtils.h"
#include "nsIAuthPrompt.h"
#include "nsIPromptFactory.h"
#include "nsIHttpChannel.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsNetUtil.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsISystemProxySettings.h"
#include "nsContentUtils.h"
#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif


using namespace mozilla;
using namespace mozilla::net;

#if defined(PR_LOGGING)
#endif
#undef LOG
#define LOG(args) PR_LOG(GetProxyLog(), PR_LOG_DEBUG, args)







static bool
HttpRequestSucceeded(nsIStreamLoader *loader)
{
  nsCOMPtr<nsIRequest> request;
  loader->GetRequest(getter_AddRefs(request));

  bool result = true;  

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
  if (httpChannel)
    httpChannel->GetRequestSucceeded(&result);

  return result;
}







class ExecuteCallback final : public nsRunnable
{
public:
  ExecuteCallback(nsPACManCallback *aCallback,
                  nsresult status)
    : mCallback(aCallback)
    , mStatus(status)
  {
  }

  void SetPACString(const nsCString &pacString)
  {
    mPACString = pacString;
  }

  void SetPACURL(const nsCString &pacURL)
  {
    mPACURL = pacURL;
  }

  NS_IMETHODIMP Run()
  {
    mCallback->OnQueryComplete(mStatus, mPACString, mPACURL);
    mCallback = nullptr;
    return NS_OK;
  }

private:
  nsRefPtr<nsPACManCallback> mCallback;
  nsresult                   mStatus;
  nsCString                  mPACString;
  nsCString                  mPACURL;
};







class ShutdownThread final : public nsRunnable
{
public:
  explicit ShutdownThread(nsIThread *thread)
    : mThread(thread)
  {
  }

  NS_IMETHODIMP Run()
  {
    MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
    mThread->Shutdown();
    return NS_OK;
  }

private:
  nsCOMPtr<nsIThread> mThread;
};



class WaitForThreadShutdown final : public nsRunnable
{
public:
  explicit WaitForThreadShutdown(nsPACMan *aPACMan)
    : mPACMan(aPACMan)
  {
  }

  NS_IMETHODIMP Run()
  {
    MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
    if (mPACMan->mPACThread) {
      mPACMan->mPACThread->Shutdown();
      mPACMan->mPACThread = nullptr;
    }
    return NS_OK;
  }

private:
  nsRefPtr<nsPACMan> mPACMan;
};







class PACLoadComplete final : public nsRunnable
{
public:
  explicit PACLoadComplete(nsPACMan *aPACMan)
    : mPACMan(aPACMan)
  {
  }

  NS_IMETHODIMP Run()
  {
    MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
    mPACMan->mLoader = nullptr;
    mPACMan->PostProcessPendingQ();
    return NS_OK;
  }

private:
    nsRefPtr<nsPACMan> mPACMan;
};







class ExecutePACThreadAction final : public nsRunnable
{
public:
  
  explicit ExecutePACThreadAction(nsPACMan *aPACMan)
    : mPACMan(aPACMan)
    , mCancel(false)
    , mSetupPAC(false)
  { }

  void CancelQueue (nsresult status)
  {
    mCancel = true;
    mCancelStatus = status;
  }

  void SetupPAC (const char *text, uint32_t datalen, nsCString &pacURI)
  {
    mSetupPAC = true;
    mSetupPACData.Assign(text, datalen);
    mSetupPACURI = pacURI;
  }

  NS_IMETHODIMP Run()
  {
    MOZ_ASSERT(!NS_IsMainThread(), "wrong thread");
    if (mCancel) {
      mPACMan->CancelPendingQ(mCancelStatus);
      mCancel = false;
      return NS_OK;
    }

    if (mSetupPAC) {
      mSetupPAC = false;

      mPACMan->mPAC.Init(mSetupPACURI,
                         mSetupPACData);

      nsRefPtr<PACLoadComplete> runnable = new PACLoadComplete(mPACMan);
      NS_DispatchToMainThread(runnable);
      return NS_OK;
    }

    mPACMan->ProcessPendingQ();
    return NS_OK;
  }

private:
  nsRefPtr<nsPACMan> mPACMan;

  bool      mCancel;
  nsresult  mCancelStatus;

  bool                 mSetupPAC;
  nsCString            mSetupPACData;
  nsCString            mSetupPACURI;
};



PendingPACQuery::PendingPACQuery(nsPACMan *pacMan, nsIURI *uri,
                                 uint32_t appId, bool isInBrowser,
                                 nsPACManCallback *callback,
                                 bool mainThreadResponse)
  : mPACMan(pacMan)
  , mAppId(appId)
  , mIsInBrowser(isInBrowser)
  , mCallback(callback)
  , mOnMainThreadOnly(mainThreadResponse)
{
  uri->GetAsciiSpec(mSpec);
  uri->GetAsciiHost(mHost);
  uri->GetScheme(mScheme);
  uri->GetPort(&mPort);

  nsCOMPtr<nsIAppsService> appsService =
      do_GetService(APPS_SERVICE_CONTRACTID);
  if (!appsService) {
    return;
  }
  nsCOMPtr<mozIApplication> mozApp;
  nsresult rv = appsService->GetAppByLocalId(appId, getter_AddRefs(mozApp));
  if (NS_FAILED(rv) || !mozApp) {
      return;
  }
  mozApp->GetOrigin(mAppOrigin);
}

void
PendingPACQuery::Complete(nsresult status, const nsCString &pacString)
{
  if (!mCallback)
    return;
  nsRefPtr<ExecuteCallback> runnable = new ExecuteCallback(mCallback, status);
  runnable->SetPACString(pacString);
  if (mOnMainThreadOnly)
    NS_DispatchToMainThread(runnable);
  else
    runnable->Run();
}

void
PendingPACQuery::UseAlternatePACFile(const nsCString &pacURL)
{
  if (!mCallback)
    return;

  nsRefPtr<ExecuteCallback> runnable = new ExecuteCallback(mCallback, NS_OK);
  runnable->SetPACURL(pacURL);
  if (mOnMainThreadOnly)
    NS_DispatchToMainThread(runnable);
  else
    runnable->Run();
}

NS_IMETHODIMP
PendingPACQuery::Run()
{
  MOZ_ASSERT(!NS_IsMainThread(), "wrong thread");
  mPACMan->PostQuery(this);
  return NS_OK;
}



static bool sThreadLocalSetup = false;
static uint32_t sThreadLocalIndex = 0xdeadbeef; 

nsPACMan::nsPACMan()
  : mLoadPending(false)
  , mShutdown(false)
  , mLoadFailureCount(0)
  , mInProgress(false)
{
  MOZ_ASSERT(NS_IsMainThread(), "pacman must be created on main thread");
  if (!sThreadLocalSetup){
    sThreadLocalSetup = true;
    PR_NewThreadPrivateIndex(&sThreadLocalIndex, nullptr);
  }
  mPAC.SetThreadLocalIndex(sThreadLocalIndex);
}

nsPACMan::~nsPACMan()
{
  if (mPACThread) {
    if (NS_IsMainThread()) {
      mPACThread->Shutdown();
    }
    else {
      nsRefPtr<ShutdownThread> runnable = new ShutdownThread(mPACThread);
      NS_DispatchToMainThread(runnable);
    }
  }

  NS_ASSERTION(mLoader == nullptr, "pac man not shutdown properly");
  NS_ASSERTION(mPendingQ.isEmpty(), "pac man not shutdown properly");
}

void
nsPACMan::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread(), "pacman must be shutdown on main thread");
  if (mShutdown) {
    return;
  }
  mShutdown = true;
  CancelExistingLoad();
  PostCancelPendingQ(NS_ERROR_ABORT);

  nsRefPtr<WaitForThreadShutdown> runnable = new WaitForThreadShutdown(this);
  NS_DispatchToMainThread(runnable);
}

nsresult
nsPACMan::AsyncGetProxyForURI(nsIURI *uri, uint32_t appId,
                              bool isInBrowser, nsPACManCallback *callback,
                              bool mainThreadResponse)
{
  MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
  if (mShutdown)
    return NS_ERROR_NOT_AVAILABLE;

  
  if (!mPACURISpec.IsEmpty() && !mScheduledReload.IsNull() &&
      TimeStamp::Now() > mScheduledReload)
    LoadPACFromURI(EmptyCString());

  nsRefPtr<PendingPACQuery> query =
    new PendingPACQuery(this, uri, appId, isInBrowser, callback,
                        mainThreadResponse);

  if (IsPACURI(uri)) {
    
    query->Complete(NS_OK, EmptyCString());
    return NS_OK;
  }

  return mPACThread->Dispatch(query, nsIEventTarget::DISPATCH_NORMAL);
}

nsresult
nsPACMan::PostQuery(PendingPACQuery *query)
{
  MOZ_ASSERT(!NS_IsMainThread(), "wrong thread");

  if (mShutdown) {
    query->Complete(NS_ERROR_NOT_AVAILABLE, EmptyCString());
    return NS_OK;
  }

  
  nsRefPtr<PendingPACQuery> addref(query);
  mPendingQ.insertBack(addref.forget().take());
  ProcessPendingQ();
  return NS_OK;
}

nsresult
nsPACMan::LoadPACFromURI(const nsCString &spec)
{
  NS_ENSURE_STATE(!mShutdown);
  NS_ENSURE_ARG(!spec.IsEmpty() || !mPACURISpec.IsEmpty());

  nsCOMPtr<nsIStreamLoader> loader =
      do_CreateInstance(NS_STREAMLOADER_CONTRACTID);
  NS_ENSURE_STATE(loader);

  
  
  
  
  

  if (!mLoadPending) {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &nsPACMan::StartLoading);
    nsresult rv;
    if (NS_FAILED(rv = NS_DispatchToCurrentThread(event)))
      return rv;
    mLoadPending = true;
  }

  CancelExistingLoad();

  mLoader = loader;
  if (!spec.IsEmpty()) {
    mPACURISpec = spec;
    mPACURIRedirectSpec.Truncate();
    mNormalPACURISpec.Truncate(); 
    mLoadFailureCount = 0;  
  }

  
  mScheduledReload = TimeStamp();
  return NS_OK;
}

void
nsPACMan::StartLoading()
{
  MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
  mLoadPending = false;

  
  if (!mLoader) {
    PostCancelPendingQ(NS_ERROR_ABORT);
    return;
  }

  if (NS_SUCCEEDED(mLoader->Init(this))) {
    
    nsCOMPtr<nsIIOService> ios = do_GetIOService();
    if (ios) {
      nsCOMPtr<nsIChannel> channel;
      nsCOMPtr<nsIURI> pacURI;
      NS_NewURI(getter_AddRefs(pacURI), mPACURISpec);

      
      if (pacURI) {
        pacURI->GetSpec(mNormalPACURISpec);
        NS_NewChannel(getter_AddRefs(channel),
                      pacURI,
                      nsContentUtils::GetSystemPrincipal(),
                      nsILoadInfo::SEC_NORMAL,
                      nsIContentPolicy::TYPE_OTHER,
                      nullptr, 
                      nullptr, 
                      nsIRequest::LOAD_NORMAL,
                      ios);
      }
      else {
        LOG(("nsPACMan::StartLoading Failed pacspec uri conversion %s\n",
             mPACURISpec.get()));
      }

      if (channel) {
        channel->SetLoadFlags(nsIRequest::LOAD_BYPASS_CACHE);
        channel->SetNotificationCallbacks(this);
        if (NS_SUCCEEDED(channel->AsyncOpen(mLoader, nullptr)))
          return;
      }
    }
  }

  CancelExistingLoad();
  PostCancelPendingQ(NS_ERROR_UNEXPECTED);
}


void
nsPACMan::OnLoadFailure()
{
  int32_t minInterval = 5;    
  int32_t maxInterval = 300;  

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    prefs->GetIntPref("network.proxy.autoconfig_retry_interval_min",
                      &minInterval);
    prefs->GetIntPref("network.proxy.autoconfig_retry_interval_max",
                      &maxInterval);
  }

  int32_t interval = minInterval << mLoadFailureCount++;  
  if (!interval || interval > maxInterval)
    interval = maxInterval;

  mScheduledReload = TimeStamp::Now() + TimeDuration::FromSeconds(interval);

  
  
  PostCancelPendingQ(NS_ERROR_NOT_AVAILABLE);
}

void
nsPACMan::CancelExistingLoad()
{
  if (mLoader) {
    nsCOMPtr<nsIRequest> request;
    mLoader->GetRequest(getter_AddRefs(request));
    if (request)
      request->Cancel(NS_ERROR_ABORT);
    mLoader = nullptr;
  }
}

void
nsPACMan::PostProcessPendingQ()
{
  MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
  nsRefPtr<ExecutePACThreadAction> pending =
    new ExecutePACThreadAction(this);
  if (mPACThread)
    mPACThread->Dispatch(pending, nsIEventTarget::DISPATCH_NORMAL);
}

void
nsPACMan::PostCancelPendingQ(nsresult status)
{
  MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
  nsRefPtr<ExecutePACThreadAction> pending =
    new ExecutePACThreadAction(this);
  pending->CancelQueue(status);
  if (mPACThread)
    mPACThread->Dispatch(pending, nsIEventTarget::DISPATCH_NORMAL);
}

void
nsPACMan::CancelPendingQ(nsresult status)
{
  MOZ_ASSERT(!NS_IsMainThread(), "wrong thread");
  nsRefPtr<PendingPACQuery> query;

  while (!mPendingQ.isEmpty()) {
    query = dont_AddRef(mPendingQ.popLast());
    query->Complete(status, EmptyCString());
  }

  if (mShutdown)
    mPAC.Shutdown();
}

void
nsPACMan::ProcessPendingQ()
{
  MOZ_ASSERT(!NS_IsMainThread(), "wrong thread");
  while (ProcessPending());

  if (mShutdown) {
    mPAC.Shutdown();
  } else {
    
    mPAC.GC();
  }
}


bool
nsPACMan::ProcessPending()
{
  if (mPendingQ.isEmpty())
    return false;

  
  
  if (mInProgress || (IsLoading() && !mLoadFailureCount))
    return false;

  nsRefPtr<PendingPACQuery> query(dont_AddRef(mPendingQ.popFirst()));

  if (mShutdown || IsLoading()) {
    query->Complete(NS_ERROR_NOT_AVAILABLE, EmptyCString());
    return true;
  }

  nsAutoCString pacString;
  bool completed = false;
  mInProgress = true;
  nsAutoCString PACURI;

  
  if (mSystemProxySettings &&
      NS_SUCCEEDED(mSystemProxySettings->GetPACURI(PACURI)) &&
      !PACURI.IsEmpty() &&
      !PACURI.Equals(mPACURISpec)) {
    query->UseAlternatePACFile(PACURI);
    completed = true;
  }

  
  
  if (!completed && mSystemProxySettings && PACURI.IsEmpty() &&
      NS_SUCCEEDED(mSystemProxySettings->
                   GetProxyForURI(query->mSpec, query->mScheme,
                                  query->mHost, query->mPort,
                                  pacString))) {
    query->Complete(NS_OK, pacString);
    completed = true;
  }

  
  if (!completed) {
    nsresult status = mPAC.GetProxyForURI(query->mSpec, query->mHost,
                                          query->mAppId, query->mAppOrigin,
                                          query->mIsInBrowser,
                                          pacString);
    query->Complete(status, pacString);
  }

  mInProgress = false;
  return true;
}

NS_IMPL_ISUPPORTS(nsPACMan, nsIStreamLoaderObserver,
                  nsIInterfaceRequestor, nsIChannelEventSink)

NS_IMETHODIMP
nsPACMan::OnStreamComplete(nsIStreamLoader *loader,
                           nsISupports *context,
                           nsresult status,
                           uint32_t dataLen,
                           const uint8_t *data)
{
  MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
  if (mLoader != loader) {
    
    
    
    
    if (status == NS_ERROR_ABORT)
      return NS_OK;
  }

  if (NS_SUCCEEDED(status) && HttpRequestSucceeded(loader)) {
    
    nsAutoCString pacURI;
    {
      nsCOMPtr<nsIRequest> request;
      loader->GetRequest(getter_AddRefs(request));
      nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
      if (channel) {
        nsCOMPtr<nsIURI> uri;
        channel->GetURI(getter_AddRefs(uri));
        if (uri)
          uri->GetAsciiSpec(pacURI);
      }
    }

    
    
    
    const char *text = (const char *) data;

    
    
    
    

    nsRefPtr<ExecutePACThreadAction> pending =
      new ExecutePACThreadAction(this);
    pending->SetupPAC(text, dataLen, pacURI);
    if (mPACThread)
      mPACThread->Dispatch(pending, nsIEventTarget::DISPATCH_NORMAL);

    
    
    mLoadFailureCount = 0;
  } else {
    
    
    OnLoadFailure();
  }

  if (NS_SUCCEEDED(status))
    PostProcessPendingQ();
  else
    PostCancelPendingQ(status);

  return NS_OK;
}

NS_IMETHODIMP
nsPACMan::GetInterface(const nsIID &iid, void **result)
{
  
  if (iid.Equals(NS_GET_IID(nsIAuthPrompt))) {
    nsCOMPtr<nsIPromptFactory> promptFac = do_GetService("@mozilla.org/prompter;1");
    NS_ENSURE_TRUE(promptFac, NS_ERROR_FAILURE);
    return promptFac->GetPrompt(nullptr, iid, reinterpret_cast<void**>(result));
  }

  
  if (iid.Equals(NS_GET_IID(nsIChannelEventSink))) {
    NS_ADDREF_THIS();
    *result = static_cast<nsIChannelEventSink *>(this);
    return NS_OK;
  }

  return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP
nsPACMan::AsyncOnChannelRedirect(nsIChannel *oldChannel, nsIChannel *newChannel,
                                 uint32_t flags,
                                 nsIAsyncVerifyRedirectCallback *callback)
{
  MOZ_ASSERT(NS_IsMainThread(), "wrong thread");
  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIURI> pacURI;
  if (NS_FAILED((rv = newChannel->GetURI(getter_AddRefs(pacURI)))))
      return rv;

  rv = pacURI->GetSpec(mPACURIRedirectSpec);
  if (NS_FAILED(rv))
      return rv;

  LOG(("nsPACMan redirect from original %s to redirected %s\n",
       mPACURISpec.get(), mPACURIRedirectSpec.get()));

  
  
  
  
  

  callback->OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

void
nsPACMan::NamePACThread()
{
  MOZ_ASSERT(!NS_IsMainThread(), "wrong thread");
  PR_SetCurrentThreadName("Proxy Resolution");
#ifdef MOZ_NUWA_PROCESS
  if (IsNuwaProcess()) {
    NuwaMarkCurrentThread(nullptr, nullptr);
  }
#endif
}

nsresult
nsPACMan::Init(nsISystemProxySettings *systemProxySettings)
{
  mSystemProxySettings = systemProxySettings;

  nsresult rv = NS_NewThread(getter_AddRefs(mPACThread), nullptr);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(this, &nsPACMan::NamePACThread);
  
  mPACThread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);

  return NS_OK;
}

namespace mozilla {
namespace net {

PRLogModuleInfo*
GetProxyLog()
{
    static PRLogModuleInfo *sLog;
    if (!sLog)
        sLog = PR_NewLogModule("proxy");
    return sLog;
}

}
}
