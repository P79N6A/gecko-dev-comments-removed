










































#ifndef nsContentList_h___
#define nsContentList_h___

#include "nsISupports.h"
#include "nsCOMArray.h"
#include "nsString.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMNodeList.h"
#include "nsStubMutationObserver.h"
#include "nsIAtom.h"
#include "nsINameSpaceManager.h"
#include "nsCycleCollectionParticipant.h"



#define kNameSpaceID_Wildcard PR_INT32_MIN





typedef PRBool (*nsContentListMatchFunc)(nsIContent* aContent,
                                         PRInt32 aNamespaceID,
                                         nsIAtom* aAtom,
                                         void* aData);

typedef void (*nsContentListDestroyFunc)(void* aData);

class nsIDocument;
class nsIDOMHTMLFormElement;


class nsBaseContentList : public nsIDOMNodeList
{
public:
  nsBaseContentList();
  virtual ~nsBaseContentList();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMNODELIST
  NS_DECL_CYCLE_COLLECTION_CLASS(nsBaseContentList)

  virtual void AppendElement(nsIContent *aContent);
  virtual void RemoveElement(nsIContent *aContent);
  virtual PRInt32 IndexOf(nsIContent *aContent, PRBool aDoFlush);
  virtual void Reset();

  static void Shutdown();

protected:
  nsCOMArray<nsIContent> mElements;
};





class nsFormContentList : public nsBaseContentList
{
public:
  nsFormContentList(nsIDOMHTMLFormElement *aForm,
                    nsBaseContentList& aContentList);
};





class nsContentListKey
{
public:
  nsContentListKey(nsINode* aRootNode,
                   nsIAtom* aMatchAtom, 
                   PRInt32 aMatchNameSpaceId)
    : mMatchAtom(aMatchAtom),
      mMatchNameSpaceId(aMatchNameSpaceId),
      mRootNode(aRootNode)
  {
  }
  
  nsContentListKey(const nsContentListKey& aContentListKey)
    : mMatchAtom(aContentListKey.mMatchAtom),
      mMatchNameSpaceId(aContentListKey.mMatchNameSpaceId),
      mRootNode(aContentListKey.mRootNode)
  {
  }

  PRBool Equals(const nsContentListKey& aContentListKey) const
  {
    return
      mMatchAtom == aContentListKey.mMatchAtom &&
      mMatchNameSpaceId == aContentListKey.mMatchNameSpaceId &&
      mRootNode == aContentListKey.mRootNode;
  }
  inline PRUint32 GetHash(void) const
  {
    return
      NS_PTR_TO_INT32(mMatchAtom.get()) ^
      (NS_PTR_TO_INT32(mRootNode) << 12) ^
      (mMatchNameSpaceId << 24);
  }
  
protected:
  nsCOMPtr<nsIAtom> mMatchAtom;
  PRInt32 mMatchNameSpaceId;
  nsINode* mRootNode; 
};





#define LIST_UP_TO_DATE 0





#define LIST_DIRTY 1







#define LIST_LAZY 2





class nsContentList : public nsBaseContentList,
                      protected nsContentListKey,
                      public nsIDOMHTMLCollection,
                      public nsStubMutationObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  














  
  nsContentList(nsINode* aRootNode,
                nsIAtom* aMatchAtom, 
                PRInt32 aMatchNameSpaceId,
                PRBool aDeep = PR_TRUE);

  











  
  nsContentList(nsINode* aRootNode,
                nsContentListMatchFunc aFunc,
                nsContentListDestroyFunc aDestroyFunc,
                void* aData,
                PRBool aDeep = PR_TRUE,
                nsIAtom* aMatchAtom = nsnull,
                PRInt32 aMatchNameSpaceId = kNameSpaceID_None,
                PRBool aFuncMayDependOnAttr = PR_TRUE);
  virtual ~nsContentList();

  
  NS_DECL_NSIDOMHTMLCOLLECTION

  
  virtual PRInt32 IndexOf(nsIContent *aContent, PRBool aDoFlush);

  
  NS_HIDDEN_(nsISupports*) GetParentObject();
  NS_HIDDEN_(PRUint32) Length(PRBool aDoFlush);
  NS_HIDDEN_(nsIContent*) Item(PRUint32 aIndex, PRBool aDoFlush);
  NS_HIDDEN_(nsIContent*) NamedItem(const nsAString& aName, PRBool aDoFlush);

  nsContentListKey* GetKey() {
    return NS_STATIC_CAST(nsContentListKey*, this);
  }
  

  
  virtual void AttributeChanged(nsIDocument *aDocument, nsIContent* aContent,
                                PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                PRInt32 aModType);
  virtual void ContentAppended(nsIDocument *aDocument, nsIContent* aContainer,
                               PRInt32 aNewIndexInContainer);
  virtual void ContentInserted(nsIDocument *aDocument, nsIContent* aContainer,
                               nsIContent* aChild, PRInt32 aIndexInContainer);
  virtual void ContentRemoved(nsIDocument *aDocument, nsIContent* aContainer,
                              nsIContent* aChild, PRInt32 aIndexInContainer);
  virtual void NodeWillBeDestroyed(const nsINode *aNode);

  static void OnDocumentDestroy(nsIDocument *aDocument);

protected:
  





  PRBool Match(nsIContent *aContent);
  






  PRBool MatchSelf(nsIContent *aContent);

  












  void PopulateWith(nsIContent *aContent, PRUint32 & aElementsToAppend);
  











  void PopulateWithStartingAfter(nsINode *aStartRoot,
                                 nsINode *aStartChild,
                                 PRUint32 & aElementsToAppend);
  








  void PopulateSelf(PRUint32 aNeededLength);

  






  PRBool MayContainRelevantNodes(nsINode* aContainer)
  {
    return mDeep || aContainer == mRootNode;
  }

  



  void RemoveFromHashtable();
  



  inline void BringSelfUpToDate(PRBool aDoFlush);

  



  void SetDirty()
  {
    mState = LIST_DIRTY;
    Reset();
  }

  



  nsContentListMatchFunc mFunc;
  


  nsContentListDestroyFunc mDestroyFunc;
  


  void* mData;
  


  PRPackedBool mMatchAll;
  



  PRUint8 mState;
  



  PRPackedBool mDeep;
  



  PRPackedBool mFuncMayDependOnAttr;

#ifdef DEBUG_CONTENT_LIST
  void AssertInSync();
#endif
};

already_AddRefed<nsContentList>
NS_GetContentList(nsINode* aRootNode, nsIAtom* aMatchAtom,
                  PRInt32 aMatchNameSpaceId);

#endif 
