






































#ifndef nsBindingManager_h_
#define nsBindingManager_h_

#include "nsIMutationObserver.h"
#include "pldhash.h"
#include "nsInterfaceHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsURIHashKey.h"
#include "nsCycleCollectionParticipant.h"
#include "nsXBLBinding.h"
#include "nsTArray.h"

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
template<class T> class nsRunnableMethod;

class nsBindingManager : public nsIMutationObserver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIMUTATIONOBSERVER

  nsBindingManager();
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

  





  nsIContent* GetInsertionPoint(nsIContent* aParent,
                                nsIContent* aChild, PRUint32* aIndex);

  




  nsIContent* GetSingleInsertionPoint(nsIContent* aParent, PRUint32* aIndex,
                                      PRBool* aMultipleInsertionPoints);

  nsresult AddLayeredBinding(nsIContent* aContent, nsIURI* aURL);
  nsresult RemoveLayeredBinding(nsIContent* aContent, nsIURI* aURL);
  nsresult LoadBindingDocument(nsIDocument* aBoundDoc, nsIURI* aURL);

  nsresult AddToAttachedQueue(nsXBLBinding* aBinding);
  void ProcessAttachedQueue();

  void ExecuteDetachedHandlers();

  nsresult PutXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo);
  nsIXBLDocumentInfo* GetXBLDocumentInfo(nsIURI* aURI);
  void RemoveXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo);

  nsresult PutLoadingDocListener(nsIURI* aURL, nsIStreamListener* aListener);
  nsIStreamListener* GetLoadingDocListener(nsIURI* aURL);
  void RemoveLoadingDocListener(nsIURI* aURL);

  void FlushSkinBindings();

  nsresult GetBindingImplementation(nsIContent* aContent, REFNSIID aIID, void** aResult);

  PRBool ShouldBuildChildFrames(nsIContent* aContent);

  







  void AddObserver(nsIMutationObserver* aObserver);

  



  PRBool RemoveObserver(nsIMutationObserver* aObserver);  

  
  nsresult WalkRules(nsStyleSet* aStyleSet, 
                     nsIStyleRuleProcessor::EnumFunc aFunc,
                     RuleProcessorData* aData,
                     PRBool* aCutOffInheritance);

  NS_HIDDEN_(void) Traverse(nsIContent *aContent,
                            nsCycleCollectionTraversalCallback &cb);

  NS_DECL_CYCLE_COLLECTION_CLASS(nsBindingManager)

  
  
  void BeginOutermostUpdate();
  void EndOutermostUpdate();

protected:
  nsIXPConnectWrappedJS* GetWrappedJS(nsIContent* aContent);
  nsresult SetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS* aResult);

  nsresult GetXBLChildNodesInternal(nsIContent* aContent,
                                    nsIDOMNodeList** aResult,
                                    PRBool* aIsAnonymousContentList);
  nsresult GetAnonymousNodesInternal(nsIContent* aContent,
                                     nsIDOMNodeList** aResult,
                                     PRBool* aIsAnonymousContentList);

  nsIContent* GetNestedInsertionPoint(nsIContent* aParent, nsIContent* aChild);

#define NS_BINDINGMANAGER_NOTIFY_OBSERVERS(func_, params_) \
  NS_OBSERVER_ARRAY_NOTIFY_OBSERVERS(mObservers, nsIMutationObserver, \
                                     func_, params_);

  
  
  void DoProcessAttachedQueue();


protected: 
  
  
  nsRefPtrHashtable<nsISupportsHashKey,nsXBLBinding> mBindingTable;

  
  
  
  
  PLDHashTable mContentListTable;

  
  
  
  
  
  
  
  PLDHashTable mAnonymousNodesTable;

  
  
  
  PLDHashTable mInsertionParentTable;

  
  
  
  
  
  
  
  PLDHashTable mWrapperTable;

  
  
  
  nsInterfaceHashtable<nsURIHashKey,nsIXBLDocumentInfo> mDocumentTable;

  
  
  
  nsInterfaceHashtable<nsURIHashKey,nsIStreamListener> mLoadingDocTable;

  
  
  
  nsTObserverArray<nsIMutationObserver> mObservers;

  
  nsBindingList mAttachedStack;
  PRPackedBool mProcessingAttachedStack;
  PRPackedBool mProcessOnEndUpdate;

  
  friend class nsRunnableMethod<nsBindingManager>;
  nsCOMPtr<nsIRunnable> mProcessAttachedQueueEvent;
};

#endif
