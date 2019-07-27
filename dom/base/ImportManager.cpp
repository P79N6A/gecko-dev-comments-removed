





#include "ImportManager.h"

#include "mozilla/EventListenerManager.h"
#include "HTMLLinkElement.h"
#include "nsContentPolicyUtils.h"
#include "nsContentUtils.h"
#include "nsCORSListenerProxy.h"
#include "nsIChannel.h"
#include "nsIContentPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEvent.h"
#include "nsIPrincipal.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsScriptLoader.h"
#include "nsNetUtil.h"





class AutoError {
public:
  explicit AutoError(mozilla::dom::ImportLoader* loader, bool scriptsBlocked = true)
    : mLoader(loader)
    , mPassed(false)
    , mScriptsBlocked(scriptsBlocked)
  {}

  ~AutoError()
  {
    if (!mPassed) {
      mLoader->Error(mScriptsBlocked);
    }
  }

  void Pass() { mPassed = true; }

private:
  mozilla::dom::ImportLoader* mLoader;
  bool mPassed;
  bool mScriptsBlocked;
};

namespace mozilla {
namespace dom {





void
ImportLoader::Updater::GetReferrerChain(nsINode* aNode,
                                        nsTArray<nsINode*>& aResult)
{
  
  MOZ_ASSERT(mLoader->mLinks.Contains(aNode));

  aResult.AppendElement(aNode);
  nsINode* node = aNode;
  nsRefPtr<ImportManager> manager = mLoader->Manager();
  for (ImportLoader* referrersLoader = manager->Find(node->OwnerDoc());
       referrersLoader;
       referrersLoader = manager->Find(node->OwnerDoc()))
  {
    
    
    node = referrersLoader->GetMainReferrer();
    MOZ_ASSERT(node);
    aResult.AppendElement(node);
  }

  
  
  
  
  uint32_t l = aResult.Length();
  for (uint32_t i = 0; i < l / 2; i++) {
    Swap(aResult[i], aResult[l - i - 1]);
  }
}

bool
ImportLoader::Updater::ShouldUpdate(nsTArray<nsINode*>& aNewPath)
{
  if (mLoader->Manager()->GetNearestPredecessor(mLoader->GetMainReferrer()) !=
      mLoader->mBlockingPredecessor) {
    return true;
  }
  
  
  
  
  
  
  nsTArray<nsINode*> oldPath;
  GetReferrerChain(mLoader->mLinks[mLoader->mMainReferrer], oldPath);
  uint32_t max = std::min(oldPath.Length(), aNewPath.Length());
  MOZ_ASSERT(max > 0);
  uint32_t lastCommonImportAncestor = 0;

  for (uint32_t i = 0;
       i < max && oldPath[i]->OwnerDoc() == aNewPath[i]->OwnerDoc();
       i++)
  {
    lastCommonImportAncestor = i;
  }

  MOZ_ASSERT(lastCommonImportAncestor < max);
  nsINode* oldLink = oldPath[lastCommonImportAncestor];
  nsINode* newLink = aNewPath[lastCommonImportAncestor];

  if ((lastCommonImportAncestor == max - 1) &&
      newLink == oldLink ) {
    
    
    MOZ_ASSERT(oldPath.Length() != aNewPath.Length(),
               "This would mean that new link == main referrer link");
    return false;
  }

  MOZ_ASSERT(aNewPath != oldPath,
             "How could this happen?");
  nsIDocument* doc = oldLink->OwnerDoc();
  MOZ_ASSERT(doc->HasSubImportLink(newLink));
  MOZ_ASSERT(doc->HasSubImportLink(oldLink));

  return doc->IndexOfSubImportLink(newLink) < doc->IndexOfSubImportLink(oldLink);
}

void
ImportLoader::Updater::UpdateMainReferrer(uint32_t aNewIdx)
{
  MOZ_ASSERT(aNewIdx < mLoader->mLinks.Length());
  nsINode* newMainReferrer = mLoader->mLinks[aNewIdx];

  
  
  
  
  if (mLoader->IsBlocking()) {
    
    
    newMainReferrer->OwnerDoc()->ScriptLoader()->AddExecuteBlocker();
    newMainReferrer->OwnerDoc()->BlockDOMContentLoaded();
  }

  if (mLoader->mDocument) {
    
    
    nsRefPtr<ImportManager> manager = mLoader->Manager();
    nsScriptLoader* loader = mLoader->mDocument->ScriptLoader();
    ImportLoader*& pred = mLoader->mBlockingPredecessor;
    ImportLoader* newPred = manager->GetNearestPredecessor(newMainReferrer);
    if (pred) {
      if (newPred) {
        newPred->AddBlockedScriptLoader(loader);
      }
      pred->RemoveBlockedScriptLoader(loader);
    }
  }

  if (mLoader->IsBlocking()) {
    mLoader->mImportParent->ScriptLoader()->RemoveExecuteBlocker();
    mLoader->mImportParent->UnblockDOMContentLoaded();
  }

  
  mLoader->mMainReferrer = aNewIdx;
  mLoader->mImportParent = newMainReferrer->OwnerDoc();
}

nsINode*
ImportLoader::Updater::NextDependant(nsINode* aCurrentLink,
                                     nsTArray<nsINode*>& aPath,
                                     NodeTable& aVisitedNodes, bool aSkipChildren)
{
  
  if (!aSkipChildren) {
    
    ImportLoader* loader = mLoader->Manager()->Find(aCurrentLink);
    if (loader && loader->GetDocument()) {
      nsINode* firstSubImport = loader->GetDocument()->GetSubImportLink(0);
      if (firstSubImport && !aVisitedNodes.Contains(firstSubImport)) {
        aPath.AppendElement(aCurrentLink);
        aVisitedNodes.PutEntry(firstSubImport);
        return firstSubImport;
      }
    }
  }

  aPath.AppendElement(aCurrentLink);
  
  while(aPath.Length() > 1) {
    aCurrentLink = aPath[aPath.Length() - 1];
    aPath.RemoveElementAt(aPath.Length() - 1);

    
    ImportLoader* loader =  mLoader->Manager()->Find(aCurrentLink->OwnerDoc());
    MOZ_ASSERT(loader && loader->GetDocument(), "How can this happend?");
    nsIDocument* doc = loader->GetDocument();
    MOZ_ASSERT(doc->HasSubImportLink(aCurrentLink));
    uint32_t idx = doc->IndexOfSubImportLink(aCurrentLink);
    nsINode* next = doc->GetSubImportLink(idx + 1);
    if (next) {
      
      
      
      MOZ_ASSERT(!aVisitedNodes.Contains(next));
      aVisitedNodes.PutEntry(next);
      return next;
    }
  }

  return nullptr;
}

void
ImportLoader::Updater::UpdateDependants(nsINode* aNode,
                                        nsTArray<nsINode*>& aPath)
{
  NodeTable visitedNodes;
  nsINode* current = aNode;
  uint32_t initialLength = aPath.Length();
  bool neededUpdate = true;
  while ((current = NextDependant(current, aPath, visitedNodes, !neededUpdate))) {
    if (!current || aPath.Length() <= initialLength) {
      break;
    }
    ImportLoader* loader = mLoader->Manager()->Find(current);
    if (!loader) {
      continue;
    }
    Updater& updater = loader->mUpdater;
    neededUpdate = updater.ShouldUpdate(aPath);
    if (neededUpdate) {
      updater.UpdateMainReferrer(loader->mLinks.IndexOf(current));
    }
  }
}

void
ImportLoader::Updater::UpdateSpanningTree(nsINode* aNode)
{
  if (mLoader->mReady || mLoader->mStopped) {
    
    return;
  }

  if (mLoader->mLinks.Length() == 1) {
    
    mLoader->mMainReferrer = 0;
    return;
  }

  nsTArray<nsINode*> newReferrerChain;
  GetReferrerChain(aNode, newReferrerChain);
  if (ShouldUpdate(newReferrerChain)) {
    UpdateMainReferrer(mLoader->mLinks.Length() - 1);
    UpdateDependants(aNode, newReferrerChain);
  }
}





NS_INTERFACE_MAP_BEGIN(ImportLoader)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(ImportLoader)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(ImportLoader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ImportLoader)

NS_IMPL_CYCLE_COLLECTION(ImportLoader,
                         mDocument,
                         mImportParent,
                         mLinks)

ImportLoader::ImportLoader(nsIURI* aURI, nsIDocument* aImportParent)
  : mURI(aURI)
  , mImportParent(aImportParent)
  , mBlockingPredecessor(nullptr)
  , mReady(false)
  , mStopped(false)
  , mBlockingScripts(false)
  , mUpdater(this)
{
}

void
ImportLoader::BlockScripts()
{
  MOZ_ASSERT(!mBlockingScripts);
  mImportParent->ScriptLoader()->AddExecuteBlocker();
  mImportParent->BlockDOMContentLoaded();
  mBlockingScripts = true;
}

void
ImportLoader::UnblockScripts()
{
  MOZ_ASSERT(mBlockingScripts);
  mImportParent->ScriptLoader()->RemoveExecuteBlocker();
  mImportParent->UnblockDOMContentLoaded();
  for (uint32_t i = 0; i < mBlockedScriptLoaders.Length(); i++) {
    mBlockedScriptLoaders[i]->RemoveExecuteBlocker();
  }
  mBlockedScriptLoaders.Clear();
  mBlockingScripts = false;
}

void
ImportLoader::SetBlockingPredecessor(ImportLoader* aLoader)
{
  mBlockingPredecessor = aLoader;
}

void
ImportLoader::DispatchEventIfFinished(nsINode* aNode)
{
  MOZ_ASSERT(!(mReady && mStopped));
  if (mReady) {
    DispatchLoadEvent(aNode);
  }
  if (mStopped) {
    DispatchErrorEvent(aNode);
  }
}

void
ImportLoader::AddBlockedScriptLoader(nsScriptLoader* aScriptLoader)
{
  if (mBlockedScriptLoaders.Contains(aScriptLoader)) {
    return;
  }

  aScriptLoader->AddExecuteBlocker();

  
  mBlockedScriptLoaders.AppendElement(aScriptLoader);
}

bool
ImportLoader::RemoveBlockedScriptLoader(nsScriptLoader* aScriptLoader)
{
  aScriptLoader->RemoveExecuteBlocker();
  return mBlockedScriptLoaders.RemoveElement(aScriptLoader);
}

void
ImportLoader::AddLinkElement(nsINode* aNode)
{
  
  
  
  
  mLinks.AppendElement(aNode);
  mUpdater.UpdateSpanningTree(aNode);
  DispatchEventIfFinished(aNode);
}

void
ImportLoader::RemoveLinkElement(nsINode* aNode)
{
  mLinks.RemoveElement(aNode);
}





class AsyncEvent : public nsRunnable {
public:
  AsyncEvent(nsINode* aNode, bool aSuccess)
    : mNode(aNode)
    , mSuccess(aSuccess)
  {
    MOZ_ASSERT(mNode);
  }

  NS_IMETHOD Run() {
    return nsContentUtils::DispatchTrustedEvent(mNode->OwnerDoc(),
                                                mNode,
                                                mSuccess ? NS_LITERAL_STRING("load")
                                                         : NS_LITERAL_STRING("error"),
                                                 false,
                                                 false);
  }

private:
  nsCOMPtr<nsINode> mNode;
  bool mSuccess;
};

void
ImportLoader::DispatchErrorEvent(nsINode* aNode)
{
  nsContentUtils::AddScriptRunner(new AsyncEvent(aNode,  false));
}

void
ImportLoader::DispatchLoadEvent(nsINode* aNode)
{
  nsContentUtils::AddScriptRunner(new AsyncEvent(aNode,  true));
}

void
ImportLoader::Done()
{
  mReady = true;
  uint32_t l = mLinks.Length();
  for (uint32_t i = 0; i < l; i++) {
    DispatchLoadEvent(mLinks[i]);
  }
  UnblockScripts();
  ReleaseResources();
}

void
ImportLoader::Error(bool aUnblockScripts)
{
  mDocument = nullptr;
  mStopped = true;
  uint32_t l = mLinks.Length();
  for (uint32_t i = 0; i < l; i++) {
    DispatchErrorEvent(mLinks[i]);
  }
  if (aUnblockScripts) {
    UnblockScripts();
  }
  ReleaseResources();
}



void ImportLoader::ReleaseResources()
{
  mParserStreamListener = nullptr;
  mImportParent = nullptr;
}

nsIPrincipal*
ImportLoader::Principal()
{
  MOZ_ASSERT(mImportParent);
  nsCOMPtr<nsIDocument> master = mImportParent->MasterDocument();
  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(master);
  MOZ_ASSERT(sop);
  return sop->GetPrincipal();
}

void
ImportLoader::Open()
{
  AutoError ae(this, false);
  
  nsCOMPtr<nsIDocument> master = mImportParent->MasterDocument();
  nsIPrincipal* principal = Principal();

  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  nsresult rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_SUBDOCUMENT,
                                          mURI,
                                          principal,
                                          mImportParent,
                                          NS_LITERAL_CSTRING("text/html"),
                                           nullptr,
                                          &shouldLoad,
                                          nsContentUtils::GetContentPolicy(),
                                          nsContentUtils::GetSecurityManager());
  if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
    NS_WARN_IF_FALSE(NS_CP_ACCEPTED(shouldLoad), "ImportLoader rejected by CSP");
    return;
  }

  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  rv = secMan->CheckLoadURIWithPrincipal(principal, mURI,
                                         nsIScriptSecurityManager::STANDARD);
  NS_ENSURE_SUCCESS_VOID(rv);

  nsCOMPtr<nsILoadGroup> loadGroup = master->GetDocumentLoadGroup();
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     mURI,
                     mImportParent,
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_SUBDOCUMENT,
                     loadGroup,
                     nullptr,  
                     nsIRequest::LOAD_BACKGROUND);

  NS_ENSURE_SUCCESS_VOID(rv);

  
  nsRefPtr<nsCORSListenerProxy> corsListener =
    new nsCORSListenerProxy(this, principal,
                             false);
  rv = corsListener->Init(channel, DataURIHandling::Allow);
  NS_ENSURE_SUCCESS_VOID(rv);

  rv = channel->AsyncOpen(corsListener, nullptr);
  NS_ENSURE_SUCCESS_VOID(rv);

  BlockScripts();
  ae.Pass();
}

NS_IMETHODIMP
ImportLoader::OnDataAvailable(nsIRequest* aRequest,
                              nsISupports* aContext,
                              nsIInputStream* aStream,
                              uint64_t aOffset,
                              uint32_t aCount)
{
  MOZ_ASSERT(mParserStreamListener);

  AutoError ae(this);
  nsresult rv;
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mParserStreamListener->OnDataAvailable(channel, aContext,
                                              aStream, aOffset,
                                              aCount);
  NS_ENSURE_SUCCESS(rv, rv);
  ae.Pass();
  return rv;
}

NS_IMETHODIMP
ImportLoader::HandleEvent(nsIDOMEvent *aEvent)
{
#ifdef DEBUG
  nsAutoString type;
  aEvent->GetType(type);
  MOZ_ASSERT(type.EqualsLiteral("DOMContentLoaded"));
#endif
  Done();
  return NS_OK;
}

NS_IMETHODIMP
ImportLoader::OnStopRequest(nsIRequest* aRequest,
                            nsISupports* aContext,
                            nsresult aStatus)
{
  
  
  if (aStatus == NS_ERROR_DOM_ABORT_ERR) {
    
    
    MOZ_ASSERT(mStopped);
    return NS_OK;
  }

  if (mParserStreamListener) {
    mParserStreamListener->OnStopRequest(aRequest, aContext, aStatus);
  }

  if (!mDocument) {
    
    
    return NS_ERROR_DOM_ABORT_ERR;
  }

  nsCOMPtr<EventTarget> eventTarget = do_QueryInterface(mDocument);
  EventListenerManager* manager = eventTarget->GetOrCreateListenerManager();
  manager->AddEventListenerByType(this,
                                  NS_LITERAL_STRING("DOMContentLoaded"),
                                  TrustedEventsAtSystemGroupBubble());
  return NS_OK;
}

NS_IMETHODIMP
ImportLoader::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  AutoError ae(this);
  nsIPrincipal* principal = Principal();

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
  if (!channel) {
    return NS_ERROR_DOM_ABORT_ERR;
  }

  if (nsContentUtils::IsSystemPrincipal(principal)) {
    
    nsCOMPtr<nsIPrincipal> channelPrincipal;
    nsContentUtils::GetSecurityManager()->GetChannelResultPrincipal(channel,
                                                                    getter_AddRefs(channelPrincipal));
    if (!nsContentUtils::IsSystemPrincipal(channelPrincipal)) {
      return NS_ERROR_FAILURE;
    }
  }
  channel->SetOwner(principal);

  nsAutoCString type;
  channel->GetContentType(type);
  if (!type.EqualsLiteral("text/html")) {
    NS_WARNING("ImportLoader wrong content type");
    return NS_ERROR_DOM_ABORT_ERR;
  }

  
  
  nsCOMPtr<nsIGlobalObject> global = mImportParent->GetScopeObject();
  nsCOMPtr<nsIDOMDocument> importDoc;
  nsCOMPtr<nsIURI> baseURI = mImportParent->GetBaseURI();
  const nsAString& emptyStr = EmptyString();
  nsresult rv = NS_NewDOMDocument(getter_AddRefs(importDoc),
                                  emptyStr, emptyStr, nullptr, mURI,
                                  baseURI, principal, false, global,
                                  DocumentFlavorHTML);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_ABORT_ERR);

  
  mDocument = do_QueryInterface(importDoc);
  nsCOMPtr<nsIDocument> master = mImportParent->MasterDocument();
  mDocument->SetMasterDocument(master);

  
  mDocument->SetSandboxFlags(master->GetSandboxFlags());

  
  
  nsCOMPtr<nsIStreamListener> listener;
  nsCOMPtr<nsILoadGroup> loadGroup;
  channel->GetLoadGroup(getter_AddRefs(loadGroup));
  nsCOMPtr<nsILoadGroup> newLoadGroup = do_CreateInstance(NS_LOADGROUP_CONTRACTID);
  NS_ENSURE_TRUE(newLoadGroup, NS_ERROR_OUT_OF_MEMORY);
  newLoadGroup->SetLoadGroup(loadGroup);
  rv = mDocument->StartDocumentLoad("import", channel, newLoadGroup,
                                    nullptr, getter_AddRefs(listener),
                                    true);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_ABORT_ERR);

  nsCOMPtr<nsIURI> originalURI;
  rv = channel->GetOriginalURI(getter_AddRefs(originalURI));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_ABORT_ERR);

  nsCOMPtr<nsIURI> URI;
  rv = channel->GetURI(getter_AddRefs(URI));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_ABORT_ERR);
  MOZ_ASSERT(URI, "URI of a channel should never be null");

  bool equals;
  rv = URI->Equals(originalURI, &equals);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_ABORT_ERR);

  if (!equals) {
    
    Manager()->AddLoaderWithNewURI(this, URI);
  }

  
  mParserStreamListener = listener;
  rv = listener->OnStartRequest(aRequest, aContext);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_ABORT_ERR);

  ae.Pass();
  return NS_OK;
}





NS_IMPL_CYCLE_COLLECTION(ImportManager,
                         mImports)

NS_INTERFACE_MAP_BEGIN(ImportManager)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(ImportManager)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(ImportManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ImportManager)

already_AddRefed<ImportLoader>
ImportManager::Get(nsIURI* aURI, nsINode* aNode, nsIDocument* aOrigDocument)
{
  
  
  nsRefPtr<ImportLoader> loader;
  mImports.Get(aURI, getter_AddRefs(loader));
  bool needToStart = false;
  if (!loader) {
    loader = new ImportLoader(aURI, aOrigDocument);
    mImports.Put(aURI, loader);
    needToStart = true;
  }

  MOZ_ASSERT(loader);
  
  
  
  
  if (!aOrigDocument->HasSubImportLink(aNode)) {
    aOrigDocument->AddSubImportLink(aNode);
  }

  loader->AddLinkElement(aNode);

  if (needToStart) {
    loader->Open();
  }

  return loader.forget();
}

ImportLoader*
ImportManager::Find(nsIDocument* aImport)
{
  return mImports.GetWeak(aImport->GetDocumentURIObject());
}

ImportLoader*
ImportManager::Find(nsINode* aLink)
{
  HTMLLinkElement* linkElement = static_cast<HTMLLinkElement*>(aLink);
  nsCOMPtr<nsIURI> uri = linkElement->GetHrefURI();
  return mImports.GetWeak(uri);
}

void
ImportManager::AddLoaderWithNewURI(ImportLoader* aLoader, nsIURI* aNewURI)
{
  mImports.Put(aNewURI, aLoader);
}

nsRefPtr<ImportLoader> ImportManager::GetNearestPredecessor(nsINode* aNode)
{
  
  nsIDocument* doc = aNode->OwnerDoc();
  int32_t idx = doc->IndexOfSubImportLink(aNode);
  MOZ_ASSERT(idx != -1, "aNode must be a sub import link of its owner document");

  for (; idx > 0; idx--) {
    HTMLLinkElement* link =
      static_cast<HTMLLinkElement*>(doc->GetSubImportLink(idx - 1));
    nsCOMPtr<nsIURI> uri = link->GetHrefURI();
    nsRefPtr<ImportLoader> ret;
    mImports.Get(uri, getter_AddRefs(ret));
    
    if (ret->GetMainReferrer() == link) {
      return ret;
    }
  }

  if (idx == 0) {
    if (doc->IsMasterDocument()) {
      
      
      return nullptr;
    }
    
    
    ImportLoader* owner = Find(doc);
    MOZ_ASSERT(owner);
    nsCOMPtr<nsINode> mainReferrer = owner->GetMainReferrer();
    return GetNearestPredecessor(mainReferrer);
  }

  return nullptr;
}

} 
} 
