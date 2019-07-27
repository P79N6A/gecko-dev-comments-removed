





































#ifndef mozilla_dom_ImportManager_h__
#define mozilla_dom_ImportManager_h__

#include "nsCOMArray.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMEventListener.h"
#include "nsIStreamListener.h"
#include "nsIWeakReferenceUtils.h"
#include "nsRefPtrHashtable.h"
#include "nsURIHashKey.h"

class nsIDocument;
class nsIChannel;
class nsIPrincipal;
class nsINode;
class AutoError;

namespace mozilla {
namespace dom {

class ImportManager;

class ImportLoader MOZ_FINAL : public nsIStreamListener
                             , public nsIDOMEventListener
{
  friend class ::AutoError;
  friend class ImportManager;

public:
  ImportLoader(nsIURI* aURI, nsIDocument* aOriginDocument);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(ImportLoader, nsIStreamListener)
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

  
  
  NS_IMETHOD HandleEvent(nsIDOMEvent *aEvent) MOZ_OVERRIDE;

  
  void Open();
  void AddLinkElement(nsINode* aNode);
  void RemoveLinkElement(nsINode* aNode);
  bool IsReady() { return mReady; }
  bool IsStopped() { return mStopped; }
  bool IsBlocking() { return mBlockingScripts; }
  already_AddRefed<nsIDocument> GetImport()
  {
    return mReady ? nsCOMPtr<nsIDocument>(mDocument).forget() : nullptr;
  }

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
  
  
  nsCOMArray<nsINode> mLinks;
  bool mReady;
  bool mStopped;
  bool mBlockingScripts;
};

class ImportManager MOZ_FINAL : public nsISupports
{
  typedef nsRefPtrHashtable<nsURIHashKey, ImportLoader> ImportMap;

  ~ImportManager() {}

public:
  ImportManager() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(ImportManager)

  already_AddRefed<ImportLoader> Get(nsIURI* aURI, nsINode* aNode,
                                     nsIDocument* aOriginDocument);

private:
  ImportMap mImports;
};

} 
} 

#endif 
