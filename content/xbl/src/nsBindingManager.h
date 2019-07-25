






































#ifndef nsBindingManager_h_
#define nsBindingManager_h_

#include "nsStubMutationObserver.h"
#include "pldhash.h"
#include "nsInterfaceHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsURIHashKey.h"
#include "nsCycleCollectionParticipant.h"
#include "nsXBLBinding.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"

class nsIContent;
class nsIXPConnectWrappedJS;
class nsIAtom;
class nsIDOMNodeList;
class nsIDocument;
class nsIURI;
class nsXBLDocumentInfo;
class nsIStreamListener;
class nsStyleSet;
class nsXBLBinding;
template<class E> class nsRefPtr;
typedef nsTArray<nsRefPtr<nsXBLBinding> > nsBindingList;
class nsIPrincipal;

class nsBindingManager : public nsStubMutationObserver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  nsBindingManager(nsIDocument* aDocument);
  ~nsBindingManager();

  nsXBLBinding* GetBinding(nsIContent* aContent);
  nsresult SetBinding(nsIContent* aContent, nsXBLBinding* aBinding);

  nsIContent* GetInsertionParent(nsIContent* aContent);
  nsresult SetInsertionParent(nsIContent* aContent, nsIContent* aResult);

  









  void RemovedFromDocument(nsIContent* aContent, nsIDocument* aOldDocument)
  {
    if (aContent->HasFlag(NODE_MAY_BE_IN_BINDING_MNGR)) {
      RemovedFromDocumentInternal(aContent, aOldDocument);
    }
  }
  void RemovedFromDocumentInternal(nsIContent* aContent,
                                   nsIDocument* aOldDocument);

  nsIAtom* ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID);

  



  nsresult GetContentListFor(nsIContent* aContent, nsIDOMNodeList** aResult);

  


  nsINodeList* GetContentListFor(nsIContent* aContent);

  



  nsresult SetContentListFor(nsIContent* aContent,
                             nsInsertionPointList* aList);

  



  bool HasContentListFor(nsIContent* aContent);

  





  nsresult GetAnonymousNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult);

  


  nsINodeList* GetAnonymousNodesFor(nsIContent* aContent);

  



  nsresult SetAnonymousNodesFor(nsIContent* aContent,
                                nsInsertionPointList* aList);

  





  nsresult GetXBLChildNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult);

  


  nsINodeList* GetXBLChildNodesFor(nsIContent* aContent);

  





  
  
  
  
  
  
  nsIContent* GetInsertionPoint(nsIContent* aParent,
                                const nsIContent* aChild, PRUint32* aIndex);

  




  nsIContent* GetSingleInsertionPoint(nsIContent* aParent, PRUint32* aIndex,
                                      bool* aMultipleInsertionPoints);

  nsIContent* GetNestedInsertionPoint(nsIContent* aParent,
                                      const nsIContent* aChild);
  nsIContent* GetNestedSingleInsertionPoint(nsIContent* aParent,
                                            bool* aMultipleInsertionPoints);

  nsresult AddLayeredBinding(nsIContent* aContent, nsIURI* aURL,
                             nsIPrincipal* aOriginPrincipal);
  nsresult RemoveLayeredBinding(nsIContent* aContent, nsIURI* aURL);
  nsresult LoadBindingDocument(nsIDocument* aBoundDoc, nsIURI* aURL,
                               nsIPrincipal* aOriginPrincipal);

  nsresult AddToAttachedQueue(nsXBLBinding* aBinding);
  void ProcessAttachedQueue(PRUint32 aSkipSize = 0);

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
                     RuleProcessorData* aData,
                     bool* aCutOffInheritance);

  void WalkAllRules(nsIStyleRuleProcessor::EnumFunc aFunc,
                    RuleProcessorData* aData);
  




  nsresult MediumFeaturesChanged(nsPresContext* aPresContext,
                                 bool* aRulesChanged);

  void AppendAllSheets(nsTArray<nsCSSStyleSheet*>& aArray);

  NS_HIDDEN_(void) Traverse(nsIContent *aContent,
                            nsCycleCollectionTraversalCallback &cb);

  NS_DECL_CYCLE_COLLECTION_CLASS(nsBindingManager)

  
  
  void BeginOutermostUpdate();
  void EndOutermostUpdate();

  
  void DropDocumentReference();

protected:
  nsIXPConnectWrappedJS* GetWrappedJS(nsIContent* aContent);
  nsresult SetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS* aResult);

  nsINodeList* GetXBLChildNodesInternal(nsIContent* aContent,
                                        bool* aIsAnonymousContentList);
  nsINodeList* GetAnonymousNodesInternal(nsIContent* aContent,
                                         bool* aIsAnonymousContentList);

  
  
  
  
  void HandleChildInsertion(nsIContent* aContainer, nsIContent* aChild,
                            PRUint32 aIndexInContainer, bool aAppend);

  
  
  
  
  
  
  
  nsXBLInsertionPoint* FindInsertionPointAndIndex(nsIContent* aContainer,
                                                  nsIContent* aInsertionParent,
                                                  PRUint32 aIndexInContainer,
                                                  PRInt32 aAppend,
                                                  PRInt32* aInsertionIndex);

  
  
  void DoProcessAttachedQueue();

  
  void PostProcessAttachedQueueEvent();


protected: 
  void RemoveInsertionParent(nsIContent* aParent);
  
  
  nsRefPtrHashtable<nsISupportsHashKey,nsXBLBinding> mBindingTable;

  
  
  
  
  
  PLDHashTable mContentListTable;

  
  
  
  
  
  
  
  
  
  PLDHashTable mAnonymousNodesTable;

  
  
  
  PLDHashTable mInsertionParentTable;

  
  
  
  
  
  
  
  PLDHashTable mWrapperTable;

  
  
  
  nsRefPtrHashtable<nsURIHashKey,nsXBLDocumentInfo> mDocumentTable;

  
  
  
  nsInterfaceHashtable<nsURIHashKey,nsIStreamListener> mLoadingDocTable;

  
  nsBindingList mAttachedStack;
  bool mProcessingAttachedStack;
  bool mDestroyed;
  PRUint32 mAttachedStackSizeOnOutermost;

  
  friend class nsRunnableMethod<nsBindingManager>;
  nsRefPtr< nsRunnableMethod<nsBindingManager> > mProcessAttachedQueueEvent;

  
  nsIDocument* mDocument; 
};

#endif
