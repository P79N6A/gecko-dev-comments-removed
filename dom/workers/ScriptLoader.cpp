





































#include "ScriptLoader.h"

#include "nsIChannel.h"
#include "nsIChannelPolicy.h"
#include "nsIContentPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIHttpChannel.h"
#include "nsIIOService.h"
#include "nsIProtocolHandler.h"
#include "nsIScriptSecurityManager.h"
#include "nsIStreamLoader.h"
#include "nsIURI.h"

#include "jsapi.h"
#include "nsChannelPolicy.h"
#include "nsContentErrors.h"
#include "nsContentPolicyUtils.h"
#include "nsContentUtils.h"
#include "nsDocShellCID.h"
#include "nsISupportsPrimitives.h"
#include "nsNetError.h"
#include "nsNetUtil.h"
#include "nsScriptLoader.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"

#include "Principal.h"
#include "WorkerFeature.h"
#include "WorkerPrivate.h"

#define MAX_CONCURRENT_SCRIPTS 1000

USING_WORKERS_NAMESPACE

namespace {

class ScriptLoaderRunnable;

struct ScriptLoadInfo
{
  ScriptLoadInfo()
  : mLoadResult(NS_ERROR_NOT_INITIALIZED), mExecutionScheduled(false),
    mExecutionResult(false)
  { }

  bool
  ReadyToExecute()
  {
    return !mChannel && NS_SUCCEEDED(mLoadResult) && !mExecutionScheduled;
  }

  nsString mURL;
  nsCOMPtr<nsIChannel> mChannel;
  nsString mScriptText;

  nsresult mLoadResult;
  bool mExecutionScheduled;
  bool mExecutionResult;
};

class ScriptExecutorRunnable : public WorkerSyncRunnable
{
  ScriptLoaderRunnable& mScriptLoader;
  PRUint32 mFirstIndex;
  PRUint32 mLastIndex;

public:
  ScriptExecutorRunnable(ScriptLoaderRunnable& aScriptLoader,
                         PRUint32 aSyncQueueKey, PRUint32 aFirstIndex,
                         PRUint32 aLastIndex);

  bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    AssertIsOnMainThread();
    return true;
  }

  void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult)
  {
    AssertIsOnMainThread();
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate);

  void
  PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate, bool aRunResult);
};

class ScriptLoaderRunnable : public WorkerFeature,
                             public nsIRunnable,
                             public nsIStreamLoaderObserver
{
  friend class ScriptExecutorRunnable;

  WorkerPrivate* mWorkerPrivate;
  PRUint32 mSyncQueueKey;
  nsTArray<ScriptLoadInfo> mLoadInfos;
  bool mIsWorkerScript;
  bool mCanceled;
  bool mCanceledMainThread;

public:
  NS_DECL_ISUPPORTS

  ScriptLoaderRunnable(WorkerPrivate* aWorkerPrivate,
                       PRUint32 aSyncQueueKey,
                       nsTArray<ScriptLoadInfo>& aLoadInfos,
                       bool aIsWorkerScript)
  : mWorkerPrivate(aWorkerPrivate), mSyncQueueKey(aSyncQueueKey),
    mIsWorkerScript(aIsWorkerScript), mCanceled(false),
    mCanceledMainThread(false)
  {
    aWorkerPrivate->AssertIsOnWorkerThread();
    NS_ASSERTION(!aIsWorkerScript || aLoadInfos.Length() == 1, "Bad args!");

    if (!mLoadInfos.SwapElements(aLoadInfos)) {
      NS_ERROR("This should never fail!");
    }
  }

  NS_IMETHOD
  Run()
  {
    AssertIsOnMainThread();

    if (NS_FAILED(RunInternal())) {
      CancelMainThread();
    }

    return NS_OK;
  }

  NS_IMETHOD
  OnStreamComplete(nsIStreamLoader* aLoader, nsISupports* aContext,
                   nsresult aStatus, PRUint32 aStringLen,
                   const PRUint8* aString)
  {
    AssertIsOnMainThread();

    nsCOMPtr<nsISupportsPRUint32> indexSupports(do_QueryInterface(aContext));
    NS_ASSERTION(indexSupports, "This should never fail!");

    PRUint32 index = PR_UINT32_MAX;
    if (NS_FAILED(indexSupports->GetData(&index)) ||
        index >= mLoadInfos.Length()) {
      NS_ERROR("Bad index!");
    }

    ScriptLoadInfo& loadInfo = mLoadInfos[index];

    loadInfo.mLoadResult = OnStreamCompleteInternal(aLoader, aContext, aStatus,
                                                    aStringLen, aString,
                                                    loadInfo);

    ExecuteFinishedScripts();

    return NS_OK;
  }

  bool
  Notify(JSContext* aCx, Status aStatus)
  {
    mWorkerPrivate->AssertIsOnWorkerThread();

    if (aStatus >= Terminating && !mCanceled) {
      mCanceled = true;

      nsCOMPtr<nsIRunnable> runnable =
        NS_NewRunnableMethod(this, &ScriptLoaderRunnable::CancelMainThread);
      NS_ASSERTION(runnable, "This should never fail!");

      if (NS_FAILED(NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL))) {
        JS_ReportError(aCx, "Failed to cancel script loader!");
        return false;
      }
    }

    return true;
  }

  void
  CancelMainThread()
  {
    AssertIsOnMainThread();

    if (mCanceledMainThread) {
      return;
    }

    mCanceledMainThread = true;

    
    for (PRUint32 index = 0; index < mLoadInfos.Length(); index++) {
      ScriptLoadInfo& loadInfo = mLoadInfos[index];

      if (loadInfo.mChannel &&
          NS_FAILED(loadInfo.mChannel->Cancel(NS_BINDING_ABORTED))) {
        NS_WARNING("Failed to cancel channel!");
        loadInfo.mChannel = nsnull;
        loadInfo.mLoadResult = NS_BINDING_ABORTED;
      }
    }

    ExecuteFinishedScripts();
  }

  nsresult
  RunInternal()
  {
    AssertIsOnMainThread();

    WorkerPrivate* parentWorker = mWorkerPrivate->GetParent();

    
    nsIPrincipal* principal = mWorkerPrivate->GetPrincipal();
    if (!principal) {
      NS_ASSERTION(parentWorker, "Must have a principal!");
      NS_ASSERTION(mIsWorkerScript, "Must have a principal for importScripts!");

      principal = parentWorker->GetPrincipal();
    }
    NS_ASSERTION(principal, "This should never be null here!");

    
    nsCOMPtr<nsIURI> baseURI;
    if (mIsWorkerScript) {
      if (parentWorker) {
        baseURI = parentWorker->GetBaseURI();
        NS_ASSERTION(baseURI, "Should have been set already!");
      }
      else {
        
        baseURI = mWorkerPrivate->GetBaseURI();
      }
    }
    else {
      baseURI = mWorkerPrivate->GetBaseURI();
      NS_ASSERTION(baseURI, "Should have been set already!");
    }

    
    nsCOMPtr<nsIDocument> parentDoc = mWorkerPrivate->GetDocument();

    
    
    nsCOMPtr<nsILoadGroup> loadGroup;
    if (parentDoc) {
      loadGroup = parentDoc->GetDocumentLoadGroup();
    }

    nsCOMPtr<nsIIOService> ios(do_GetIOService());

    nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
    NS_ASSERTION(secMan, "This should never be null!");

    for (PRUint32 index = 0; index < mLoadInfos.Length(); index++) {
      ScriptLoadInfo& loadInfo = mLoadInfos[index];
      nsresult& rv = loadInfo.mLoadResult;

      nsCOMPtr<nsIURI> uri;
      rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(uri),
                                                     loadInfo.mURL, parentDoc,
                                                     baseURI);
      if (NS_FAILED(rv)) {
        return rv;
      }

      
      if (parentDoc) {
        PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
        rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_SCRIPT, uri,
                                       principal, parentDoc,
                                       NS_LITERAL_CSTRING("text/javascript"),
                                       nsnull, &shouldLoad,
                                       nsContentUtils::GetContentPolicy(),
                                       secMan);
        if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
          if (NS_FAILED(rv) || shouldLoad != nsIContentPolicy::REJECT_TYPE) {
            return rv = NS_ERROR_CONTENT_BLOCKED;
          }
          return rv = NS_ERROR_CONTENT_BLOCKED_SHOW_ALT;
        }
      }

      
      
      
      rv = mIsWorkerScript ?
           principal->CheckMayLoad(uri, PR_FALSE):
           secMan->CheckLoadURIWithPrincipal(principal, uri, 0);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      nsCOMPtr<nsISupportsPRUint32> indexSupports =
        do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = indexSupports->SetData(index);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      nsCOMPtr<nsIStreamLoader> loader;
      rv = NS_NewStreamLoader(getter_AddRefs(loader), this);
      NS_ENSURE_SUCCESS(rv, rv);

      
      nsCOMPtr<nsIContentSecurityPolicy> csp;
      rv = principal->GetCsp(getter_AddRefs(csp));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIChannelPolicy> channelPolicy;
      if (csp) {
        channelPolicy = do_CreateInstance(NSCHANNELPOLICY_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = channelPolicy->SetContentSecurityPolicy(csp);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = channelPolicy->SetLoadType(nsIContentPolicy::TYPE_SCRIPT);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      PRUint32 flags = nsIRequest::LOAD_NORMAL | nsIChannel::LOAD_CLASSIFY_URI;

      nsCOMPtr<nsIChannel> channel;
      rv = NS_NewChannel(getter_AddRefs(channel), uri, ios, loadGroup, nsnull,
                         flags, channelPolicy);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = channel->AsyncOpen(loader, indexSupports);
      NS_ENSURE_SUCCESS(rv, rv);

      loadInfo.mChannel.swap(channel);
    }

    return NS_OK;
  }

  nsresult
  OnStreamCompleteInternal(nsIStreamLoader* aLoader, nsISupports* aContext,
                           nsresult aStatus, PRUint32 aStringLen,
                           const PRUint8* aString, ScriptLoadInfo& aLoadInfo)
  {
    AssertIsOnMainThread();

    if (!aLoadInfo.mChannel) {
      return NS_BINDING_ABORTED;
    }

    aLoadInfo.mChannel = nsnull;

    if (NS_FAILED(aStatus)) {
      return aStatus;
    }

    if (!aStringLen) {
      return NS_OK;
    }

    NS_ASSERTION(aString, "This should never be null!");

    
    
    nsCOMPtr<nsIRequest> request;
    nsresult rv = aLoader->GetRequest(getter_AddRefs(request));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
    if (httpChannel) {
      bool requestSucceeded;
      rv = httpChannel->GetRequestSucceeded(&requestSucceeded);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!requestSucceeded) {
        return NS_ERROR_NOT_AVAILABLE;
      }
    }

    
    nsIDocument* parentDoc = mWorkerPrivate->GetDocument();

    
    
    rv = nsScriptLoader::ConvertToUTF16(aLoadInfo.mChannel, aString, aStringLen,
                                        EmptyString(), parentDoc,
                                        aLoadInfo.mScriptText);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (aLoadInfo.mScriptText.IsEmpty()) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
    NS_ASSERTION(channel, "This should never fail!");

    
    nsCOMPtr<nsIURI> finalURI;
    rv = NS_GetFinalChannelURI(channel, getter_AddRefs(finalURI));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString filename;
    rv = finalURI->GetSpec(filename);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!filename.IsEmpty()) {
      
      
      aLoadInfo.mURL.Assign(NS_ConvertUTF8toUTF16(filename));
    }

    
    
    if (mIsWorkerScript) {
      
      rv = mWorkerPrivate->SetBaseURI(finalURI);
      NS_ENSURE_SUCCESS(rv, rv);

      
      WorkerPrivate* parent = mWorkerPrivate->GetParent();

      NS_ASSERTION(mWorkerPrivate->GetPrincipal() || parent,
                   "Must have one of these!");

      nsCOMPtr<nsIPrincipal> loadPrincipal = mWorkerPrivate->GetPrincipal() ?
                                             mWorkerPrivate->GetPrincipal() :
                                             parent->GetPrincipal();

      nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
      NS_ASSERTION(ssm, "Should never be null!");

      nsCOMPtr<nsIPrincipal> channelPrincipal;
      rv = ssm->GetChannelPrincipal(channel, getter_AddRefs(channelPrincipal));
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      if (!nsContentUtils::IsSystemPrincipal(channelPrincipal)) {
        bool isResource;
        rv = NS_URIChainHasFlags(finalURI,
                                 nsIProtocolHandler::URI_IS_UI_RESOURCE,
                                 &isResource);
        NS_ENSURE_SUCCESS(rv, rv);

        if (isResource) {
          rv = ssm->GetSystemPrincipal(getter_AddRefs(channelPrincipal));
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }

      
      
      
      
      
      if (nsContentUtils::IsSystemPrincipal(loadPrincipal)) {
        if (!nsContentUtils::IsSystemPrincipal(channelPrincipal)) {
          return NS_ERROR_DOM_BAD_URI;
        }
      }
      else if (NS_FAILED(loadPrincipal->CheckMayLoad(finalURI, PR_FALSE))) {
        return NS_ERROR_DOM_BAD_URI;
      }

      mWorkerPrivate->SetPrincipal(channelPrincipal);
    }

    return NS_OK;
  }

  void
  ExecuteFinishedScripts()
  {
    PRUint32 firstIndex = PR_UINT32_MAX;
    PRUint32 lastIndex = PR_UINT32_MAX;

    
    for (PRUint32 index = 0; index < mLoadInfos.Length(); index++) {
      if (!mLoadInfos[index].mExecutionScheduled) {
        firstIndex = index;
        break;
      }
    }

    
    
    if (firstIndex != PR_UINT32_MAX) {
      for (PRUint32 index = firstIndex; index < mLoadInfos.Length(); index++) {
        ScriptLoadInfo& loadInfo = mLoadInfos[index];

        
        if (loadInfo.mChannel) {
          break;
        }

        
        loadInfo.mExecutionScheduled = true;

        lastIndex = index;
      }
    }

    if (firstIndex != PR_UINT32_MAX && lastIndex != PR_UINT32_MAX) {
      nsRefPtr<ScriptExecutorRunnable> runnable =
        new ScriptExecutorRunnable(*this, mSyncQueueKey, firstIndex, lastIndex);
      if (!runnable->Dispatch(nsnull)) {
        NS_ERROR("This should never fail!");
      }
    }
  }
};

NS_IMPL_THREADSAFE_ISUPPORTS2(ScriptLoaderRunnable, nsIRunnable,
                                                    nsIStreamLoaderObserver)

ScriptExecutorRunnable::ScriptExecutorRunnable(
                                            ScriptLoaderRunnable& aScriptLoader,
                                            PRUint32 aSyncQueueKey,
                                            PRUint32 aFirstIndex,
                                            PRUint32 aLastIndex)
: WorkerSyncRunnable(aScriptLoader.mWorkerPrivate, aSyncQueueKey),
  mScriptLoader(aScriptLoader), mFirstIndex(aFirstIndex), mLastIndex(aLastIndex)
{
  NS_ASSERTION(aFirstIndex <= aLastIndex, "Bad first index!");
  NS_ASSERTION(aLastIndex < aScriptLoader.mLoadInfos.Length(),
               "Bad last index!");
}

bool
ScriptExecutorRunnable::WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
{
  nsTArray<ScriptLoadInfo>& loadInfos = mScriptLoader.mLoadInfos;

  
  for (PRUint32 index = 0; index < mFirstIndex; index++) {
    ScriptLoadInfo& loadInfo = loadInfos.ElementAt(index);

    NS_ASSERTION(!loadInfo.mChannel, "Should no longer have a channel!");
    NS_ASSERTION(loadInfo.mExecutionScheduled, "Should be scheduled!");

    if (!loadInfo.mExecutionResult) {
      return true;
    }
  }

  JSObject* global = JS_GetGlobalObject(aCx);
  NS_ASSERTION(global, "Must have a global by now!");

  JSPrincipals* principal = GetWorkerPrincipal();
  NS_ASSERTION(principal, "This should never be null!");

  for (PRUint32 index = mFirstIndex; index <= mLastIndex; index++) {
    ScriptLoadInfo& loadInfo = loadInfos.ElementAt(index);

    NS_ASSERTION(!loadInfo.mChannel, "Should no longer have a channel!");
    NS_ASSERTION(loadInfo.mExecutionScheduled, "Should be scheduled!");
    NS_ASSERTION(!loadInfo.mExecutionResult, "Should not have executed yet!");

    if (NS_FAILED(loadInfo.mLoadResult)) {
      NS_ConvertUTF16toUTF8 url(loadInfo.mURL);

      switch (loadInfo.mLoadResult) {
        case NS_BINDING_ABORTED:
          
          break;

        case NS_ERROR_MALFORMED_URI:
          JS_ReportError(aCx, "Malformed script URI: %s", url.get());
          break;

        case NS_ERROR_FILE_NOT_FOUND:
        case NS_ERROR_NOT_AVAILABLE:
          JS_ReportError(aCx, "Script file not found: %s", url.get());
          break;

        default:
          JS_ReportError(aCx, "Failed to load script: %s (nsresult = 0x%x)",
                         url.get(), loadInfo.mLoadResult);
      }
      return true;
    }

    NS_ConvertUTF16toUTF8 filename(loadInfo.mURL);

    if (!JS_EvaluateUCScriptForPrincipals(aCx, global, principal,
                                          loadInfo.mScriptText.get(),
                                          loadInfo.mScriptText.Length(),
                                          filename.get(), 1, nsnull)) {
      return true;
    }

    loadInfo.mExecutionResult = true;
  }

  return true;
}

void
ScriptExecutorRunnable::PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
                                bool aRunResult)
{
  nsTArray<ScriptLoadInfo>& loadInfos = mScriptLoader.mLoadInfos;

  if (mLastIndex == loadInfos.Length() - 1) {
    
    bool result = true;
    for (PRUint32 index = 0; index < loadInfos.Length(); index++) {
      if (!loadInfos[index].mExecutionResult) {
        result = false;
        break;
      }
    }

    aWorkerPrivate->RemoveFeature(aCx, &mScriptLoader);
    aWorkerPrivate->StopSyncLoop(mSyncQueueKey, result);
  }
}

bool
LoadAllScripts(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               nsTArray<ScriptLoadInfo>& aLoadInfos, bool aIsWorkerScript)
{
  aWorkerPrivate->AssertIsOnWorkerThread();
  NS_ASSERTION(!aLoadInfos.IsEmpty(), "Bad arguments!");

  PRUint32 syncQueueKey = aWorkerPrivate->CreateNewSyncLoop();

  nsRefPtr<ScriptLoaderRunnable> loader =
    new ScriptLoaderRunnable(aWorkerPrivate, syncQueueKey, aLoadInfos,
                             aIsWorkerScript);

  NS_ASSERTION(aLoadInfos.IsEmpty(), "Should have swapped!");

  if (!aWorkerPrivate->AddFeature(aCx, loader)) {
    return false;
  }

  if (NS_FAILED(NS_DispatchToMainThread(loader, NS_DISPATCH_NORMAL))) {
    NS_ERROR("Failed to dispatch!");

    aWorkerPrivate->RemoveFeature(aCx, loader);
    return false;
  }

  return aWorkerPrivate->RunSyncLoop(aCx, syncQueueKey);
}

} 

BEGIN_WORKERS_NAMESPACE

namespace scriptloader {

bool
LoadWorkerScript(JSContext* aCx)
{
  WorkerPrivate* worker = GetWorkerPrivateFromContext(aCx);
  NS_ASSERTION(worker, "This should never be null!");

  nsTArray<ScriptLoadInfo> loadInfos;

  ScriptLoadInfo* info = loadInfos.AppendElement();
  info->mURL = worker->ScriptURL();

  return LoadAllScripts(aCx, worker, loadInfos, true);
}

bool
Load(JSContext* aCx, uintN aURLCount, jsval* aURLs)
{
  WorkerPrivate* worker = GetWorkerPrivateFromContext(aCx);
  NS_ASSERTION(worker, "This should never be null!");

  if (!aURLCount) {
    return true;
  }

  if (aURLCount > MAX_CONCURRENT_SCRIPTS) {
    JS_ReportError(aCx, "Cannot load more than %d scripts at one time!",
                   MAX_CONCURRENT_SCRIPTS);
    return false;
  }

  nsTArray<ScriptLoadInfo> loadInfos;
  loadInfos.SetLength(PRUint32(aURLCount));

  for (uintN index = 0; index < aURLCount; index++) {
    JSString* str = JS_ValueToString(aCx, aURLs[index]);
    if (!str) {
      return false;
    }

    size_t length;
    const jschar* buffer = JS_GetStringCharsAndLength(aCx, str, &length);
    if (!buffer) {
      return false;
    }

    loadInfos[index].mURL.Assign(buffer, length);
  }

  return LoadAllScripts(aCx, worker, loadInfos, false);
}

} 

END_WORKERS_NAMESPACE
