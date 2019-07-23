





































#include "nsDOMWorkerScriptLoader.h"


#include "nsIChannel.h"
#include "nsIContentPolicy.h"
#include "nsIHttpChannel.h"
#include "nsIIOService.h"
#include "nsIRequest.h"
#include "nsIScriptSecurityManager.h"
#include "nsIStreamLoader.h"
#include "nsIChannelClassifier.h"


#include "nsAutoLock.h"
#include "nsContentErrors.h"
#include "nsContentPolicyUtils.h"
#include "nsContentUtils.h"
#include "nsISupportsPrimitives.h"
#include "nsNetError.h"
#include "nsNetUtil.h"
#include "nsScriptLoader.h"
#include "nsThreadUtils.h"
#include "pratom.h"
#include "nsDocShellCID.h"


#include "nsDOMWorkerPool.h"
#include "nsDOMWorkerSecurityManager.h"
#include "nsDOMThreadService.h"
#include "nsDOMWorkerTimeout.h"

#define LOG(_args) PR_LOG(gDOMThreadsLog, PR_LOG_DEBUG, _args)

nsDOMWorkerScriptLoader::nsDOMWorkerScriptLoader(nsDOMWorker* aWorker)
: nsDOMWorkerFeature(aWorker),
  mTarget(nsnull),
  mScriptCount(0),
  mCanceled(PR_FALSE),
  mForWorker(PR_FALSE)
{
  
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aWorker, "Null worker!");
}

NS_IMPL_ISUPPORTS_INHERITED2(nsDOMWorkerScriptLoader, nsDOMWorkerFeature,
                                                      nsIRunnable,
                                                      nsIStreamLoaderObserver)

nsresult
nsDOMWorkerScriptLoader::LoadScripts(JSContext* aCx,
                                     const nsTArray<nsString>& aURLs,
                                     PRBool aForWorker)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aCx, "Null context!");

  mTarget = NS_GetCurrentThread();
  NS_ASSERTION(mTarget, "This should never be null!");

  if (mCanceled) {
    return NS_ERROR_ABORT;
  }

  mForWorker = aForWorker;

  mScriptCount = aURLs.Length();
  if (!mScriptCount) {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  PRBool success = mLoadInfos.SetCapacity(mScriptCount);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  
  
  success = mPendingRunnables.SetCapacity(mScriptCount + 1);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 index = 0; index < mScriptCount; index++) {
    ScriptLoadInfo* newInfo = mLoadInfos.AppendElement();
    NS_ASSERTION(newInfo, "Shouldn't fail if SetCapacity succeeded above!");

    newInfo->url.Assign(aURLs[index]);
    if (newInfo->url.IsEmpty()) {
      return NS_ERROR_INVALID_ARG;
    }

    success = newInfo->scriptObj.Hold(aCx);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  }

  
  
  AutoSuspendWorkerEvents aswe(this);

  nsresult rv = DoRunLoop(aCx);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  rv = VerifyScripts(aCx);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = ExecuteScripts(aCx);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

nsresult
nsDOMWorkerScriptLoader::LoadScript(JSContext* aCx,
                                    const nsString& aURL,
                                    PRBool aForWorker)
{
  nsAutoTArray<nsString, 1> url;
  url.AppendElement(aURL);

  return LoadScripts(aCx, url, aForWorker);
}

nsresult
nsDOMWorkerScriptLoader::DoRunLoop(JSContext* aCx)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  volatile PRBool done = PR_FALSE;
  mDoneRunnable = new ScriptLoaderDone(this, &done);
  NS_ENSURE_TRUE(mDoneRunnable, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = NS_DispatchToMainThread(this);
  NS_ENSURE_SUCCESS(rv, rv);

  while (!(done || mCanceled)) {
    JSAutoSuspendRequest asr(aCx);
    NS_ProcessNextEvent(mTarget);
  }

  return mCanceled ? NS_ERROR_ABORT : NS_OK;
}

nsresult
nsDOMWorkerScriptLoader::VerifyScripts(JSContext* aCx)
{
  NS_ASSERTION(aCx, "Shouldn't be null!");

  nsresult rv = NS_OK;

  for (PRUint32 index = 0; index < mScriptCount; index++) {
    ScriptLoadInfo& loadInfo = mLoadInfos[index];
    NS_ASSERTION(loadInfo.done, "Inconsistent state!");

    if (NS_SUCCEEDED(loadInfo.result) && loadInfo.scriptObj.ToJSObject()) {
      continue;
    }

    NS_ASSERTION(!loadInfo.scriptObj.ToJSObject(), "Inconsistent state!");

    
    rv = NS_FAILED(loadInfo.result) ? loadInfo.result : NS_ERROR_FAILURE;

    
    
    
    
    if (NS_SUCCEEDED(loadInfo.result) || loadInfo.result == NS_BINDING_ABORTED) {
      continue;
    }

    

    JSAutoRequest ar(aCx);

    
    if (!JS_IsExceptionPending(aCx)) {
      const char* message;
      switch (loadInfo.result) {
        case NS_ERROR_MALFORMED_URI:
          message = "Malformed script URI: %s";
          break;
        case NS_ERROR_FILE_NOT_FOUND:
        case NS_ERROR_NOT_AVAILABLE:
          message = "Script file not found: %s";
          break;
        default:
          message = "Failed to load script: %s (nsresult = 0x%x)";
          break;
      }
      NS_ConvertUTF16toUTF8 url(loadInfo.url);
      JS_ReportError(aCx, message, url.get(), loadInfo.result);
    }
    break;
  }

  return rv;
}

nsresult
nsDOMWorkerScriptLoader::ExecuteScripts(JSContext* aCx)
{
  NS_ASSERTION(aCx, "Shouldn't be null!");

  
  for (PRUint32 index = 0; index < mScriptCount; index++) {
    ScriptLoadInfo& loadInfo = mLoadInfos[index];

    JSAutoRequest ar(aCx);

    JSScript* script =
      static_cast<JSScript*>(JS_GetPrivate(aCx, loadInfo.scriptObj.ToJSObject()));
    NS_ASSERTION(script, "This shouldn't ever be null!");

    JSObject* global = mWorker->mGlobal ?
                       mWorker->mGlobal :
                       JS_GetGlobalObject(aCx);
    NS_ENSURE_STATE(global);

    
    
    uint32 oldOpts =
      JS_SetOptions(aCx, JS_GetOptions(aCx) | JSOPTION_DONT_REPORT_UNCAUGHT);

    jsval val;
    PRBool success = JS_ExecuteScript(aCx, global, script, &val);

    JS_SetOptions(aCx, oldOpts);

    if (!success) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

void
nsDOMWorkerScriptLoader::Cancel()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ASSERTION(!mCanceled, "Cancel called more than once!");
  mCanceled = PR_TRUE;

  for (PRUint32 index = 0; index < mScriptCount; index++) {
    ScriptLoadInfo& loadInfo = mLoadInfos[index];

    nsIRequest* request =
      static_cast<nsIRequest*>(loadInfo.channel.get());
    if (request) {
#ifdef DEBUG
      nsresult rv =
#endif
      request->Cancel(NS_BINDING_ABORTED);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to cancel channel!");
    }
  }

  nsAutoTArray<ScriptLoaderRunnable*, 10> runnables;
  {
    nsAutoLock lock(mWorker->Lock());
    runnables.AppendElements(mPendingRunnables);
    mPendingRunnables.Clear();
  }

  PRUint32 runnableCount = runnables.Length();
  for (PRUint32 index = 0; index < runnableCount; index++) {
    runnables[index]->Revoke();
  }

  
  
  
  NotifyDone();
}

NS_IMETHODIMP
nsDOMWorkerScriptLoader::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  if (mCanceled) {
    return NS_BINDING_ABORTED;
  }

  nsresult rv = RunInternal();
  if (NS_SUCCEEDED(rv)) {
    return rv;
  }

  

  
  
  PRBool needsNotify = PR_TRUE;

  
  for (PRUint32 index = 0; index < mScriptCount; index++) {
    ScriptLoadInfo& loadInfo = mLoadInfos[index];

    nsIRequest* request = static_cast<nsIRequest*>(loadInfo.channel.get());
    if (request) {
#ifdef DEBUG
      nsresult rvInner =
#endif
      request->Cancel(NS_BINDING_ABORTED);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rvInner), "Failed to cancel channel!");

      
      
      needsNotify = PR_FALSE;
    }
    else {
      
      
      loadInfo.done = PR_TRUE;
    }
  }

  if (needsNotify) {
    NotifyDone();
  }

  return rv;
}

NS_IMETHODIMP
nsDOMWorkerScriptLoader::OnStreamComplete(nsIStreamLoader* aLoader,
                                          nsISupports* aContext,
                                          nsresult aStatus,
                                          PRUint32 aStringLen,
                                          const PRUint8* aString)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  if (mCanceled) {
    return NS_BINDING_ABORTED;
  }

  nsresult rv = OnStreamCompleteInternal(aLoader, aContext, aStatus, aStringLen,
                                         aString);

  
  for (PRUint32 index = 0; index < mScriptCount; index++) {
    if (!mLoadInfos[index].done) {
      
      break;
    }

    if (index == mScriptCount - 1) {
      
      NotifyDone();
    }
  }

  return rv;
}

nsresult
nsDOMWorkerScriptLoader::RunInternal()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  nsCOMPtr<nsIDocument> parentDoc = mWorker->Pool()->ParentDocument();
  if (!parentDoc) {
    
    return NS_ERROR_ABORT;
  }

  nsIPrincipal* principal;
  nsIURI* baseURI;

  if (mForWorker) {
    NS_ASSERTION(mScriptCount == 1, "Bad state!");

    nsRefPtr<nsDOMWorker> parentWorker = mWorker->GetParent();
    if (parentWorker) {
      principal = parentWorker->GetPrincipal();
      NS_ENSURE_STATE(principal);

      baseURI = parentWorker->GetURI();
      NS_ENSURE_STATE(baseURI);
    }
    else {
      principal = parentDoc->NodePrincipal();
      NS_ENSURE_STATE(principal);

      baseURI = parentDoc->GetBaseURI();
    }
  }
  else {
    principal = mWorker->GetPrincipal();
    baseURI = mWorker->GetURI();

    NS_ASSERTION(principal && baseURI, "Should have been set already!");
  }

  
  
  nsCOMPtr<nsILoadGroup> loadGroup(parentDoc->GetDocumentLoadGroup());
  nsCOMPtr<nsIIOService> ios(do_GetIOService());

  for (PRUint32 index = 0; index < mScriptCount; index++) {
    ScriptLoadInfo& loadInfo = mLoadInfos[index];
    nsresult& rv = loadInfo.result;

    nsCOMPtr<nsIURI>& uri = loadInfo.finalURI;
    rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(uri),
                                                   loadInfo.url, parentDoc,
                                                   baseURI);
    if (NS_FAILED(rv)) {
      return rv;
    }

    nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
    NS_ENSURE_TRUE(secMan, NS_ERROR_FAILURE);

    PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
    rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_SCRIPT, uri,
                                   principal, parentDoc,
                                   NS_LITERAL_CSTRING("text/javascript"),
                                   nsnull, &shouldLoad,
                                   nsContentUtils::GetContentPolicy(), secMan);
    if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
      if (NS_FAILED(rv) || shouldLoad != nsIContentPolicy::REJECT_TYPE) {
        return NS_ERROR_CONTENT_BLOCKED;
      }
      return NS_ERROR_CONTENT_BLOCKED_SHOW_ALT;
    }

    
    
    
    if (mForWorker) {
      rv = principal->CheckMayLoad(uri, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      
      mWorker->SetPrincipal(principal);

      rv = mWorker->SetURI(uri);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = secMan->CheckLoadURIWithPrincipal(principal, uri, 0);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    nsCOMPtr<nsISupportsPRUint32> indexSupports =
      do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = indexSupports->SetData(index);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCOMPtr<nsIStreamLoader> loader;
    rv = NS_NewStreamLoader(getter_AddRefs(loader), this);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = NS_NewChannel(getter_AddRefs(loadInfo.channel), uri, ios, loadGroup);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = loadInfo.channel->AsyncOpen(loader, indexSupports);
    if (NS_FAILED(rv)) {
      
      loadInfo.channel = nsnull;
      return rv;
    }

    
    nsCOMPtr<nsIChannelClassifier> classifier =
        do_CreateInstance(NS_CHANNELCLASSIFIER_CONTRACTID);
    if (classifier) {
        rv = classifier->Start(loadInfo.channel, PR_TRUE);
        if (NS_FAILED(rv)) {
            loadInfo.channel->Cancel(rv);
            loadInfo.channel = nsnull;
            return rv;
        }
    }
  }

  return NS_OK;
}

nsresult
nsDOMWorkerScriptLoader::OnStreamCompleteInternal(nsIStreamLoader* aLoader,
                                                  nsISupports* aContext,
                                                  nsresult aStatus,
                                                  PRUint32 aStringLen,
                                                  const PRUint8* aString)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsISupportsPRUint32> indexSupports(do_QueryInterface(aContext));
  NS_ENSURE_TRUE(indexSupports, NS_ERROR_NO_INTERFACE);

  PRUint32 index = PR_UINT32_MAX;
  indexSupports->GetData(&index);

  if (index >= mScriptCount) {
    NS_NOTREACHED("This really can't fail or we'll hang!");
    return NS_ERROR_FAILURE;
  }

  ScriptLoadInfo& loadInfo = mLoadInfos[index];

  NS_ASSERTION(!loadInfo.done, "Got complete on the same load twice!");
  loadInfo.done = PR_TRUE;

#ifdef DEBUG
  
  nsCOMPtr<nsIRequest> requestDebug;
  nsresult rvDebug = aLoader->GetRequest(getter_AddRefs(requestDebug));

  
  
  NS_ASSERTION(NS_SUCCEEDED(rvDebug) || mCanceled, "GetRequest failed!");

  if (NS_SUCCEEDED(rvDebug)) {
    nsCOMPtr<nsIChannel> channel(do_QueryInterface(requestDebug));
    NS_ASSERTION(channel, "QI failed!");

    nsCOMPtr<nsISupports> thisChannel(do_QueryInterface(channel));
    NS_ASSERTION(thisChannel, "QI failed!");

    nsCOMPtr<nsISupports> ourChannel(do_QueryInterface(loadInfo.channel));
    NS_ASSERTION(ourChannel, "QI failed!");

    NS_ASSERTION(thisChannel == ourChannel, "Wrong channel!");
  }
#endif

  
  nsresult& rv = loadInfo.result = aStatus;

  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!(aStringLen && aString)) {
    return rv = NS_ERROR_UNEXPECTED;
  }

  
  
  nsCOMPtr<nsIRequest> request;
  rv = aLoader->GetRequest(getter_AddRefs(request));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(request));
  if (httpChannel) {
    PRBool requestSucceeded;
    rv = httpChannel->GetRequestSucceeded(&requestSucceeded);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!requestSucceeded) {
      return rv = NS_ERROR_NOT_AVAILABLE;
    }
  }

  nsIDocument* parentDoc = mWorker->Pool()->ParentDocument();
  if (!parentDoc) {
    NS_ASSERTION(mWorker->IsCanceled(),
                 "Null parent document when we're not canceled?!");
    return rv = NS_ERROR_FAILURE;
  }

  
  
  rv = nsScriptLoader::ConvertToUTF16(loadInfo.channel, aString, aStringLen,
                                      EmptyString(), parentDoc,
                                      loadInfo.scriptText);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (loadInfo.scriptText.IsEmpty()) {
    return rv = NS_ERROR_FAILURE;
  }

  nsCString filename;
  loadInfo.finalURI->GetSpec(filename);

  if (filename.IsEmpty()) {
    filename.Assign(NS_LossyConvertUTF16toASCII(loadInfo.url));
  }
  else {
    
    
    loadInfo.url.Assign(NS_ConvertUTF8toUTF16(filename));
  }

  nsRefPtr<ScriptCompiler> compiler =
    new ScriptCompiler(this, loadInfo.scriptText, filename, loadInfo.scriptObj);
  NS_ASSERTION(compiler, "Out of memory!");
  if (!compiler) {
    return rv = NS_ERROR_OUT_OF_MEMORY;
  }

  rv = mTarget->Dispatch(compiler, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

void
nsDOMWorkerScriptLoader::NotifyDone()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mDoneRunnable) {
    
    return;
  }

  for (PRUint32 index = 0; index < mScriptCount; index++) {
    ScriptLoadInfo& loadInfo = mLoadInfos[index];
    
    
    loadInfo.channel = nsnull;
    loadInfo.finalURI = nsnull;

    if (mCanceled) {
      
      loadInfo.done = PR_TRUE;
      loadInfo.result = NS_BINDING_ABORTED;
    }
  }

#ifdef DEBUG
  nsresult rv =
#endif
  mTarget->Dispatch(mDoneRunnable, NS_DISPATCH_NORMAL);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Couldn't dispatch done event!");

  mDoneRunnable = nsnull;
}

void
nsDOMWorkerScriptLoader::SuspendWorkerEvents()
{
  NS_ASSERTION(mWorker, "No worker yet!");
  mWorker->SuspendFeatures();
}

void
nsDOMWorkerScriptLoader::ResumeWorkerEvents()
{
  NS_ASSERTION(mWorker, "No worker yet!");
  mWorker->ResumeFeatures();
}

nsDOMWorkerScriptLoader::
ScriptLoaderRunnable::ScriptLoaderRunnable(nsDOMWorkerScriptLoader* aLoader)
: mRevoked(PR_FALSE),
  mLoader(aLoader)
{
  nsAutoLock lock(aLoader->Lock());
#ifdef DEBUG
  nsDOMWorkerScriptLoader::ScriptLoaderRunnable** added =
#endif
  aLoader->mPendingRunnables.AppendElement(this);
  NS_ASSERTION(added, "This shouldn't fail because we SetCapacity earlier!");
}

nsDOMWorkerScriptLoader::
ScriptLoaderRunnable::~ScriptLoaderRunnable()
{
  if (!mRevoked) {
    nsAutoLock lock(mLoader->Lock());
#ifdef DEBUG
    PRBool removed =
#endif
    mLoader->mPendingRunnables.RemoveElement(this);
    NS_ASSERTION(removed, "Someone has changed the array!");
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerScriptLoader::ScriptLoaderRunnable,
                              nsIRunnable)

void
nsDOMWorkerScriptLoader::ScriptLoaderRunnable::Revoke()
{
  mRevoked = PR_TRUE;
}

nsDOMWorkerScriptLoader::
ScriptCompiler::ScriptCompiler(nsDOMWorkerScriptLoader* aLoader,
                               const nsString& aScriptText,
                               const nsCString& aFilename,
                               nsAutoJSValHolder& aScriptObj)
: ScriptLoaderRunnable(aLoader),
  mScriptText(aScriptText),
  mFilename(aFilename),
  mScriptObj(aScriptObj)
{
  NS_ASSERTION(!aScriptText.IsEmpty(), "No script to compile!");
  NS_ASSERTION(aScriptObj.IsHeld(), "Should be held!");
}

NS_IMETHODIMP
nsDOMWorkerScriptLoader::ScriptCompiler::Run()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mRevoked) {
    return NS_OK;
  }

  NS_ASSERTION(!mScriptObj.ToJSObject(), "Already have a script object?!");
  NS_ASSERTION(mScriptObj.IsHeld(), "Not held?!");
  NS_ASSERTION(!mScriptText.IsEmpty(), "Shouldn't have empty source here!");

  JSContext* cx = nsDOMThreadService::GetCurrentContext();
  NS_ENSURE_STATE(cx);

  JSAutoRequest ar(cx);

  JSObject* global = JS_GetGlobalObject(cx);
  NS_ENSURE_STATE(global);

  
  
  uint32 oldOpts =
    JS_SetOptions(cx, JS_GetOptions(cx) | JSOPTION_DONT_REPORT_UNCAUGHT);

  JSPrincipals* principal = nsDOMWorkerSecurityManager::WorkerPrincipal();

  JSScript* script =
    JS_CompileUCScriptForPrincipals(cx, global, principal,
                                    reinterpret_cast<const jschar*>
                                               (mScriptText.BeginReading()),
                                    mScriptText.Length(), mFilename.get(), 1);

  JS_SetOptions(cx, oldOpts);

  if (!script) {
    return NS_ERROR_FAILURE;
  }

  mScriptObj = JS_NewScriptObject(cx, script);
  NS_ENSURE_STATE(mScriptObj.ToJSObject());

  return NS_OK;
}

nsDOMWorkerScriptLoader::
ScriptLoaderDone::ScriptLoaderDone(nsDOMWorkerScriptLoader* aLoader,
                                   volatile PRBool* aDoneFlag)
: ScriptLoaderRunnable(aLoader),
  mDoneFlag(aDoneFlag)
{
  NS_ASSERTION(aDoneFlag && !*aDoneFlag, "Bad setup!");
}

NS_IMETHODIMP
nsDOMWorkerScriptLoader::ScriptLoaderDone::Run()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (mRevoked) {
    return NS_OK;
  }

  *mDoneFlag = PR_TRUE;
  return NS_OK;
}

nsDOMWorkerScriptLoader::
AutoSuspendWorkerEvents::AutoSuspendWorkerEvents(nsDOMWorkerScriptLoader* aLoader)
: mLoader(aLoader)
{
  NS_ASSERTION(aLoader, "Don't hand me null!");
  aLoader->SuspendWorkerEvents();
}

nsDOMWorkerScriptLoader::
AutoSuspendWorkerEvents::~AutoSuspendWorkerEvents()
{
  mLoader->ResumeWorkerEvents();
}
