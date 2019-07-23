










































#ifndef nsContentList_h___
#define nsContentList_h___

#include "nsISupports.h"
#include "nsCOMArray.h"
#include "nsString.h"
#include "nsIHTMLCollection.h"
#include "nsIDOMNodeList.h"
#include "nsINodeList.h"
#include "nsStubMutationObserver.h"
#include "nsIAtom.h"
#include "nsINameSpaceManager.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"



#define kNameSpaceID_Wildcard PR_INT32_MIN





typedef PRBool (*nsContentListMatchFunc)(nsIContent* aContent,
                                         PRInt32 aNamespaceID,
                                         nsIAtom* aAtom,
                                         void* aData);

typedef void (*nsContentListDestroyFunc)(void* aData);

class nsIDocument;
class nsIDOMHTMLFormElement;


class nsBaseContentList : public nsINodeList
{
public:
  virtual ~nsBaseContentList();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMNODELIST

  
  virtual nsIContent* GetNodeAt(PRUint32 aIndex);
  virtual PRInt32 IndexOf(nsIContent* aContent);
  
  PRUint32 Length() const { 
    return mElements.Count();
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsBaseContentList, nsINodeList)

  void AppendElement(nsIContent *aContent);

  





  void InsertElementAt(nsIContent* aContent, PRInt32 aIndex);

  void RemoveElement(nsIContent *aContent); 

  void Reset() {
    mElements.Clear();
  }


  virtual PRInt32 IndexOf(nsIContent *aContent, PRBool aDoFlush);

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
                      public nsIHTMLCollection,
                      public nsStubMutationObserver,
                      public nsWrapperCache
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
  virtual nsIContent* GetNodeAt(PRUint32 aIndex);
  virtual PRInt32 IndexOf(nsIContent* aContent);

  
  virtual nsISupports* GetNodeAt(PRUint32 aIndex, nsresult* aResult);
  virtual nsISupports* GetNamedItem(const nsAString& aName, nsresult* aResult);

  
  NS_HIDDEN_(nsINode*) GetParentObject() { return mRootNode; }
  NS_HIDDEN_(PRUint32) Length(PRBool aDoFlush);
  NS_HIDDEN_(nsIContent*) Item(PRUint32 aIndex, PRBool aDoFlush);
  NS_HIDDEN_(nsIContent*) NamedItem(const nsAString& aName, PRBool aDoFlush);

  nsContentListKey* GetKey() {
    return static_cast<nsContentListKey*>(this);
  }
  

  
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
  NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED
  
  static void OnDocumentDestroy(nsIDocument *aDocument);

  static nsContentList* FromSupports(nsISupports* aSupports)
  {
    nsINodeList* list = static_cast<nsINodeList*>(aSupports);
#ifdef DEBUG
    {
      nsCOMPtr<nsINodeList> list_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(list_qi == list, "Uh, fix QI!");
    }
#endif
    return static_cast<nsContentList*>(list);
  }

protected:
  





  PRBool Match(nsIContent *aContent);
  






  PRBool MatchSelf(nsIContent *aContent);

  












  void NS_FASTCALL PopulateWith(nsIContent *aContent,
                                PRUint32 & aElementsToAppend);

  











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
