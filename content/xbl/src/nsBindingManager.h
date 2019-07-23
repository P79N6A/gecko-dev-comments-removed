






































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
class nsIXBLDocumentInfo;
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

  














  nsresult ChangeDocumentFor(nsIContent* aContent, nsIDocument* aOldDocument,
                             nsIDocument* aNewDocument);

  nsIAtom* ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID);

  



  nsresult GetContentListFor(nsIContent* aContent, nsIDOMNodeList** aResult);

  



  nsresult SetContentListFor(nsIContent* aContent,
                             nsInsertionPointList* aList);

  



  PRBool HasContentListFor(nsIContent* aContent);

  





  nsresult GetAnonymousNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult);

  



  nsresult SetAnonymousNodesFor(nsIContent* aContent,
                                nsInsertionPointList* aList);

  





  nsresult GetXBLChildNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult);

  


  nsINodeList* GetXBLChildNodesFor(nsIContent* aContent);

  





  
  
  
  
  
  
  nsIContent* GetInsertionPoint(nsIContent* aParent,
                                nsIContent* aChild, PRUint32* aIndex);

  




  nsIContent* GetSingleInsertionPoint(nsIContent* aParent, PRUint32* aIndex,
                                      PRBool* aMultipleInsertionPoints);

  nsresult AddLayeredBinding(nsIContent* aContent, nsIURI* aURL,
                             nsIPrincipal* aOriginPrincipal);
  nsresult RemoveLayeredBinding(nsIContent* aContent, nsIURI* aURL);
  nsresult LoadBindingDocument(nsIDocument* aBoundDoc, nsIURI* aURL,
                               nsIPrincipal* aOriginPrincipal);

  nsresult AddToAttachedQueue(nsXBLBinding* aBinding);
  void ProcessAttachedQueue(PRUint32 aSkipSize = 0);

  void ExecuteDetachedHandlers();

  nsresult PutXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo);
  nsIXBLDocumentInfo* GetXBLDocumentInfo(nsIURI* aURI);
  void RemoveXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo);

  nsresult PutLoadingDocListener(nsIURI* aURL, nsIStreamListener* aListener);
  nsIStreamListener* GetLoadingDocListener(nsIURI* aURL);
  void RemoveLoadingDocListener(nsIURI* aURL);

  void FlushSkinBindings();

  nsresult GetBindingImplementation(nsIContent* aContent, REFNSIID aIID, void** aResult);

  
  nsresult WalkRules(nsIStyleRuleProcessor::EnumFunc aFunc,
                     RuleProcessorData* aData,
                     PRBool* aCutOffInheritance);
  




  nsresult MediumFeaturesChanged(nsPresContext* aPresContext,
                                 PRBool* aRulesChanged);

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
                                        PRBool* aIsAnonymousContentList);
  nsINodeList* GetAnonymousNodesInternal(nsIContent* aContent,
                                         PRBool* aIsAnonymousContentList);

  nsIContent* GetNestedInsertionPoint(nsIContent* aParent, nsIContent* aChild);
  nsIContent* GetNestedSingleInsertionPoint(nsIContent* aParent,
                                            PRBool* aMultipleInsertionPoints);

  
  
  
  
  void HandleChildInsertion(nsIContent* aContainer, nsIContent* aChild,
                            PRUint32 aIndexInContainer, PRBool aAppend);

  
  
  
  
  
  
  
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

  
  
  
  nsInterfaceHashtable<nsURIHashKey,nsIXBLDocumentInfo> mDocumentTable;

  
  
  
  nsInterfaceHashtable<nsURIHashKey,nsIStreamListener> mLoadingDocTable;

  
  nsBindingList mAttachedStack;
  PRPackedBool mProcessingAttachedStack;
  PRPackedBool mDestroyed;
  PRUint32 mAttachedStackSizeOnOutermost;

  
  friend class nsRunnableMethod<nsBindingManager>;
  nsRefPtr< nsRunnableMethod<nsBindingManager> > mProcessAttachedQueueEvent;

  
  nsIDocument* mDocument; 
};

#endif
