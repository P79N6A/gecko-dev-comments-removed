










































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
#include "nsCRT.h"
#include "mozilla/dom/Element.h"



#define kNameSpaceID_Wildcard PR_INT32_MIN





typedef PRBool (*nsContentListMatchFunc)(nsIContent* aContent,
                                         PRInt32 aNamespaceID,
                                         nsIAtom* aAtom,
                                         void* aData);

typedef void (*nsContentListDestroyFunc)(void* aData);

class nsIDocument;


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
  void MaybeAppendElement(nsIContent* aContent)
  {
    if (aContent)
      AppendElement(aContent);
  }

  





  void InsertElementAt(nsIContent* aContent, PRInt32 aIndex);

  void RemoveElement(nsIContent *aContent); 

  void Reset() {
    mElements.Clear();
  }


  virtual PRInt32 IndexOf(nsIContent *aContent, PRBool aDoFlush);

protected:
  nsCOMArray<nsIContent> mElements;
};


class nsSimpleContentList : public nsBaseContentList
{
public:
  nsSimpleContentList(nsINode *aRoot) : nsBaseContentList(),
                                        mRoot(aRoot)
  {
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsSimpleContentList,
                                           nsBaseContentList)

  virtual nsINode* GetParentObject()
  {
    return mRoot;
  }

private:
  
  nsCOMPtr<nsINode> mRoot;
};




class nsFormContentList : public nsSimpleContentList
{
public:
  nsFormContentList(nsIContent *aForm,
                    nsBaseContentList& aContentList);
};





struct nsContentListKey
{
  nsContentListKey(nsINode* aRootNode,
                   PRInt32 aMatchNameSpaceId,
                   const nsAString& aTagname)
    : mRootNode(aRootNode),
      mMatchNameSpaceId(aMatchNameSpaceId),
      mTagname(aTagname)
  {
  }

  nsContentListKey(const nsContentListKey& aContentListKey)
    : mRootNode(aContentListKey.mRootNode),
      mMatchNameSpaceId(aContentListKey.mMatchNameSpaceId),
      mTagname(aContentListKey.mTagname)
  {
  }

  inline PRUint32 GetHash(void) const
  {
    return
      HashString(mTagname) ^
      (NS_PTR_TO_INT32(mRootNode) << 12) ^
      (mMatchNameSpaceId << 24);
  }
  
  nsINode* const mRootNode; 
  const PRInt32 mMatchNameSpaceId;
  const nsAString& mTagname;
};





#define LIST_UP_TO_DATE 0





#define LIST_DIRTY 1







#define LIST_LAZY 2





class nsContentList : public nsBaseContentList,
                      public nsIHTMLCollection,
                      public nsStubMutationObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  














  
  nsContentList(nsINode* aRootNode,
                PRInt32 aMatchNameSpaceId,
                nsIAtom* aHTMLMatchAtom,
                nsIAtom* aXMLMatchAtom,
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
  virtual nsINode* GetParentObject()
  {
    return mRootNode;
  }

  
  
  virtual nsISupports* GetNamedItem(const nsAString& aName,
                                    nsWrapperCache** aCache);

  
  NS_HIDDEN_(PRUint32) Length(PRBool aDoFlush);
  NS_HIDDEN_(nsIContent*) Item(PRUint32 aIndex, PRBool aDoFlush);
  NS_HIDDEN_(nsIContent*) NamedItem(const nsAString& aName, PRBool aDoFlush);

  
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
  NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED
  
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

  PRBool MatchesKey(const nsContentListKey& aKey) const
  {
    
    
    
    NS_PRECONDITION(mXMLMatchAtom,
                    "How did we get here with a null match atom on our list?");
    return
      mXMLMatchAtom->Equals(aKey.mTagname) &&
      mRootNode == aKey.mRootNode &&
      mMatchNameSpaceId == aKey.mMatchNameSpaceId;
  }

protected:
  





  PRBool Match(mozilla::dom::Element *aElement);
  






  PRBool MatchSelf(nsIContent *aContent);

  








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

  




  virtual void RemoveFromCaches() {
    RemoveFromHashtable();
  }

  nsINode* mRootNode; 
  PRInt32 mMatchNameSpaceId;
  nsCOMPtr<nsIAtom> mHTMLMatchAtom;
  nsCOMPtr<nsIAtom> mXMLMatchAtom;

  



  nsContentListMatchFunc mFunc;
  


  nsContentListDestroyFunc mDestroyFunc;
  


  void* mData;
  



  PRUint8 mState;

  
  
  
  
  
  


  PRUint8 mMatchAll : 1;
  



  PRUint8 mDeep : 1;
  



  PRUint8 mFuncMayDependOnAttr : 1;
  


  PRUint8 mFlushesNeeded : 1;

#ifdef DEBUG_CONTENT_LIST
  void AssertInSync();
#endif
};




class nsCacheableFuncStringContentList;

class NS_STACK_CLASS nsFuncStringCacheKey {
public:
  nsFuncStringCacheKey(nsINode* aRootNode,
                       nsContentListMatchFunc aFunc,
                       const nsAString& aString) :
    mRootNode(aRootNode),
    mFunc(aFunc),
    mString(aString)
    {}

  PRUint32 GetHash(void) const
  {
    return NS_PTR_TO_INT32(mRootNode) ^ (NS_PTR_TO_INT32(mFunc) << 12) ^
      nsCRT::HashCode(mString.BeginReading(), mString.Length());
  }

private:
  friend class nsCacheableFuncStringContentList;

  nsINode* const mRootNode;
  const nsContentListMatchFunc mFunc;
  const nsAString& mString;
};






typedef void* (*nsFuncStringContentListDataAllocator)(nsINode* aRootNode,
                                                      const nsString* aString);


class nsCacheableFuncStringContentList : public nsContentList {
public:
  nsCacheableFuncStringContentList(nsINode* aRootNode,
                                   nsContentListMatchFunc aFunc,
                                   nsContentListDestroyFunc aDestroyFunc,
                                   nsFuncStringContentListDataAllocator aDataAllocator,
                                   const nsAString& aString) :
    nsContentList(aRootNode, aFunc, aDestroyFunc, nsnull),
    mString(aString)
  {
    mData = (*aDataAllocator)(aRootNode, &mString);
  }

  virtual ~nsCacheableFuncStringContentList();

  PRBool Equals(const nsFuncStringCacheKey* aKey) {
    return mRootNode == aKey->mRootNode && mFunc == aKey->mFunc &&
      mString == aKey->mString;
  }

  PRBool AllocatedData() const { return !!mData; }
protected:
  virtual void RemoveFromCaches() {
    RemoveFromFuncStringHashtable();
  }
  void RemoveFromFuncStringHashtable();

  nsString mString;
};






already_AddRefed<nsContentList>
NS_GetContentList(nsINode* aRootNode,
                  PRInt32 aMatchNameSpaceId,
                  const nsAString& aTagname);

already_AddRefed<nsContentList>
NS_GetFuncStringContentList(nsINode* aRootNode,
                            nsContentListMatchFunc aFunc,
                            nsContentListDestroyFunc aDestroyFunc,
                            nsFuncStringContentListDataAllocator aDataAllocator,
                            const nsAString& aString);
#endif 
