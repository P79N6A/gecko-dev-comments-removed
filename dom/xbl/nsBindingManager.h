




#ifndef nsBindingManager_h_
#define nsBindingManager_h_

#include "nsIContent.h"
#include "nsStubMutationObserver.h"
#include "nsHashKeys.h"
#include "nsInterfaceHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsURIHashKey.h"
#include "nsCycleCollectionParticipant.h"
#include "nsXBLBinding.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"

struct ElementDependentRuleProcessorData;
class nsIXPConnectWrappedJS;
class nsIAtom;
class nsIDOMNodeList;
class nsIDocument;
class nsIURI;
class nsXBLDocumentInfo;
class nsIStreamListener;
class nsXBLBinding;
template<class E> class nsRefPtr;
typedef nsTArray<nsRefPtr<nsXBLBinding> > nsBindingList;
class nsIPrincipal;
class nsITimer;

namespace mozilla {
class CSSStyleSheet;
} 

class nsBindingManager final : public nsStubMutationObserver
{
  ~nsBindingManager();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  explicit nsBindingManager(nsIDocument* aDocument);

  nsXBLBinding* GetBindingWithContent(nsIContent* aContent);

  void AddBoundContent(nsIContent* aContent);
  void RemoveBoundContent(nsIContent* aContent);

  









  void RemovedFromDocument(nsIContent* aContent, nsIDocument* aOldDocument)
  {
    if (aContent->HasFlag(NODE_MAY_BE_IN_BINDING_MNGR)) {
      RemovedFromDocumentInternal(aContent, aOldDocument);
    }
  }
  void RemovedFromDocumentInternal(nsIContent* aContent,
                                   nsIDocument* aOldDocument);

  nsIAtom* ResolveTag(nsIContent* aContent, int32_t* aNameSpaceID);

  





  nsresult GetAnonymousNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult);
  nsINodeList* GetAnonymousNodesFor(nsIContent* aContent);

  nsresult ClearBinding(nsIContent* aContent);
  nsresult LoadBindingDocument(nsIDocument* aBoundDoc, nsIURI* aURL,
                               nsIPrincipal* aOriginPrincipal);

  nsresult AddToAttachedQueue(nsXBLBinding* aBinding);
  void RemoveFromAttachedQueue(nsXBLBinding* aBinding);
  void ProcessAttachedQueue(uint32_t aSkipSize = 0);

  void ExecuteDetachedHandlers();

  nsresult PutXBLDocumentInfo(nsXBLDocumentInfo* aDocumentInfo);
  nsXBLDocumentInfo* GetXBLDocumentInfo(nsIURI* aURI);
  void RemoveXBLDocumentInfo(nsXBLDocumentInfo* aDocumentInfo);

  nsresult PutLoadingDocListener(nsIURI* aURL, nsIStreamListener* aListener);
  nsIStreamListener* GetLoadingDocListener(nsIURI* aURL);
  void RemoveLoadingDocListener(nsIURI* aURL);

  void FlushSkinBindings();

  nsresult GetBindingImplementation(nsIContent* aContent, REFNSIID aIID, void** aResult);

  
  nsresult WalkRules(nsIStyleRuleProcessor::EnumFunc aFunc,
                     ElementDependentRuleProcessorData* aData,
                     bool* aCutOffInheritance);

  void WalkAllRules(nsIStyleRuleProcessor::EnumFunc aFunc,
                    ElementDependentRuleProcessorData* aData);
  




  nsresult MediumFeaturesChanged(nsPresContext* aPresContext,
                                 bool* aRulesChanged);

  void AppendAllSheets(nsTArray<mozilla::CSSStyleSheet*>& aArray);

  void Traverse(nsIContent *aContent,
                            nsCycleCollectionTraversalCallback &cb);

  NS_DECL_CYCLE_COLLECTION_CLASS(nsBindingManager)

  
  
  void BeginOutermostUpdate();
  void EndOutermostUpdate();

  
  
  void ClearInsertionPointsRecursively(nsIContent* aContent);

  
  void DropDocumentReference();

  nsIContent* FindNestedInsertionPoint(nsIContent* aContainer,
                                       nsIContent* aChild);

  nsIContent* FindNestedSingleInsertionPoint(nsIContent* aContainer, bool* aMulti);

protected:
  nsIXPConnectWrappedJS* GetWrappedJS(nsIContent* aContent);
  nsresult SetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS* aResult);

  
  
  
  
  void HandleChildInsertion(nsIContent* aContainer, nsIContent* aChild,
                            uint32_t aIndexInContainer, bool aAppend);

  
  
  void DoProcessAttachedQueue();

  
  void PostProcessAttachedQueueEvent();

  
  static void PostPAQEventCallback(nsITimer* aTimer, void* aClosure);


protected:
  
  nsAutoPtr<nsTHashtable<nsRefPtrHashKey<nsIContent> > > mBoundContentSet;

  
  
  
  
  
  
  
  typedef nsInterfaceHashtable<nsISupportsHashKey, nsIXPConnectWrappedJS> WrapperHashtable;
  nsAutoPtr<WrapperHashtable> mWrapperTable;

  
  
  
  nsAutoPtr<nsRefPtrHashtable<nsURIHashKey,nsXBLDocumentInfo> > mDocumentTable;

  
  
  
  nsAutoPtr<nsInterfaceHashtable<nsURIHashKey,nsIStreamListener> > mLoadingDocTable;

  
  nsBindingList mAttachedStack;
  bool mProcessingAttachedStack;
  bool mDestroyed;
  uint32_t mAttachedStackSizeOnOutermost;

  
  friend class nsRunnableMethod<nsBindingManager>;
  nsRefPtr< nsRunnableMethod<nsBindingManager> > mProcessAttachedQueueEvent;

  
  nsIDocument* mDocument;
};

#endif
