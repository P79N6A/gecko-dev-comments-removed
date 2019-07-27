





































#ifndef mozilla_dom_ImportManager_h__
#define mozilla_dom_ImportManager_h__

#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMEventListener.h"
#include "nsIStreamListener.h"
#include "nsIWeakReferenceUtils.h"
#include "nsRefPtrHashtable.h"
#include "nsScriptLoader.h"
#include "nsURIHashKey.h"

class nsIDocument;
class nsIPrincipal;
class nsINode;
class AutoError;

namespace mozilla {
namespace dom {

class ImportManager;

typedef nsTHashtable<nsPtrHashKey<nsINode>> NodeTable;

class ImportLoader final : public nsIStreamListener
                         , public nsIDOMEventListener
{

  
  
  class Updater {

  public:
    explicit Updater(ImportLoader* aLoader) : mLoader(aLoader)
    {}

    
    
    
    
    
    
    
    
    void UpdateSpanningTree(nsINode* aNode);

  private:
    
    
    
    void GetReferrerChain(nsINode* aNode, nsTArray<nsINode*>& aResult);

    
    
    bool ShouldUpdate(nsTArray<nsINode*>& aNewPath);
    void UpdateMainReferrer(uint32_t newIdx);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsINode* NextDependant(nsINode* aCurrentLink,
                           nsTArray<nsINode*>& aPath,
                           NodeTable& aVisitedLinks, bool aSkipChildren);

    
    
    
    void UpdateDependants(nsINode* aNode, nsTArray<nsINode*>& aPath);

    ImportLoader* mLoader;
  };

  friend class ::AutoError;
  friend class ImportManager;
  friend class Updater;

public:
  ImportLoader(nsIURI* aURI, nsIDocument* aOriginDocument);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(ImportLoader, nsIStreamListener)
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

  
  
  NS_IMETHOD HandleEvent(nsIDOMEvent *aEvent) override;

  
  void Open();
  void AddLinkElement(nsINode* aNode);
  void RemoveLinkElement(nsINode* aNode);
  bool IsReady() { return mReady; }
  bool IsStopped() { return mStopped; }
  bool IsBlocking() { return mBlockingScripts; }

  ImportManager* Manager() {
    MOZ_ASSERT(mDocument || mImportParent, "One of them should be always set");
    return (mDocument ? mDocument : mImportParent)->ImportManager();
  }

  
  
  nsIDocument* GetDocument()
  {
    return mDocument;
  }

  
  
  nsIDocument* GetImport()
  {
    return mReady ? mDocument : nullptr;
  }

  
  
  
  
  
  
  nsINode* GetMainReferrer()
  {
    return mLinks.IsEmpty() ? nullptr : mLinks[mMainReferrer];
  }

  
  
  
  
  
  void AddBlockedScriptLoader(nsScriptLoader* aScriptLoader);
  bool RemoveBlockedScriptLoader(nsScriptLoader* aScriptLoader);
  void SetBlockingPredecessor(ImportLoader* aLoader);

private:
  ~ImportLoader() {}

  
  
  
  void DispatchEventIfFinished(nsINode* aNode);

  
  void DispatchErrorEvent(nsINode* aNode);
  void DispatchLoadEvent(nsINode* aNode);

  
  void Error(bool aUnblockScripts);

  
  void Done();

  
  
  
  void ReleaseResources();

  
  
  void BlockScripts();
  void UnblockScripts();

  nsIPrincipal* Principal();

  nsCOMPtr<nsIDocument> mDocument;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIStreamListener> mParserStreamListener;
  nsCOMPtr<nsIDocument> mImportParent;
  ImportLoader* mBlockingPredecessor;

  
  
  nsTArray<nsCOMPtr<nsINode>> mLinks;

  
  
  nsTArray<nsRefPtr<nsScriptLoader>> mBlockedScriptLoaders;

  
  
  
  
  uint32_t mMainReferrer;
  bool mReady;
  bool mStopped;
  bool mBlockingScripts;
  Updater mUpdater;
};

class ImportManager final : public nsISupports
{
  typedef nsRefPtrHashtable<nsURIHashKey, ImportLoader> ImportMap;

  ~ImportManager() {}

public:
  ImportManager() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(ImportManager)

  
  ImportLoader* Find(nsIDocument* aImport);

  
  ImportLoader* Find(nsINode* aLink);

  void AddLoaderWithNewURI(ImportLoader* aLoader, nsIURI* aNewURI);

  
  
  
  already_AddRefed<ImportLoader> Get(nsIURI* aURI, nsINode* aNode,
                                     nsIDocument* aOriginDocument);

  
  
  nsRefPtr<ImportLoader> GetNearestPredecessor(nsINode* aNode);

private:
  ImportMap mImports;
};

} 
} 

#endif 
