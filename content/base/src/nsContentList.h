










#ifndef nsContentList_h___
#define nsContentList_h___

#include "mozilla/Attributes.h"
#include "nsContentListDeclarations.h"
#include "nsISupports.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsIHTMLCollection.h"
#include "nsIDOMNodeList.h"
#include "nsINodeList.h"
#include "nsStubMutationObserver.h"
#include "nsIAtom.h"
#include "nsCycleCollectionParticipant.h"
#include "nsNameSpaceManager.h"
#include "nsWrapperCache.h"
#include "nsHashKeys.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/dom/NameSpaceConstants.h"

namespace mozilla {
namespace dom {
class Element;
}
}


class nsBaseContentList : public nsINodeList
{
public:
  nsBaseContentList()
  {
    SetIsDOMBinding();
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMNODELIST

  
  virtual int32_t IndexOf(nsIContent* aContent) MOZ_OVERRIDE;
  virtual nsIContent* Item(uint32_t aIndex) MOZ_OVERRIDE;

  uint32_t Length() const { 
    return mElements.Length();
  }

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(nsBaseContentList)

  void AppendElement(nsIContent *aContent)
  {
    mElements.AppendElement(aContent);
  }
  void MaybeAppendElement(nsIContent* aContent)
  {
    if (aContent)
      AppendElement(aContent);
  }

  





  void InsertElementAt(nsIContent* aContent, int32_t aIndex)
  {
    NS_ASSERTION(aContent, "Element to insert must not be null");
    mElements.InsertElementAt(aIndex, aContent);
  }

  void RemoveElement(nsIContent *aContent)
  {
    mElements.RemoveElement(aContent);
  }

  void Reset() {
    mElements.Clear();
  }

  virtual int32_t IndexOf(nsIContent *aContent, bool aDoFlush);

  virtual JSObject* WrapObject(JSContext *cx)
    MOZ_OVERRIDE = 0;

  void SetCapacity(uint32_t aCapacity)
  {
    mElements.SetCapacity(aCapacity);
  }
protected:
  virtual ~nsBaseContentList();

  



  virtual void RemoveFromCaches()
  {
  }

  nsTArray< nsCOMPtr<nsIContent> > mElements;
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

  virtual nsINode* GetParentObject() MOZ_OVERRIDE
  {
    return mRoot;
  }
  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

protected:
  virtual ~nsSimpleContentList() {}

private:
  
  nsCOMPtr<nsINode> mRoot;
};





struct nsContentListKey
{
  nsContentListKey(nsINode* aRootNode,
                   int32_t aMatchNameSpaceId,
                   const nsAString& aTagname)
    : mRootNode(aRootNode),
      mMatchNameSpaceId(aMatchNameSpaceId),
      mTagname(aTagname),
      mHash(mozilla::AddToHash(mozilla::HashString(aTagname), mRootNode,
                               mMatchNameSpaceId))
  {
  }

  nsContentListKey(const nsContentListKey& aContentListKey)
    : mRootNode(aContentListKey.mRootNode),
      mMatchNameSpaceId(aContentListKey.mMatchNameSpaceId),
      mTagname(aContentListKey.mTagname),
      mHash(aContentListKey.mHash)
  {
  }

  inline uint32_t GetHash(void) const
  {
    return mHash;
  }
  
  nsINode* const mRootNode; 
  const int32_t mMatchNameSpaceId;
  const nsAString& mTagname;
  const uint32_t mHash;
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
                int32_t aMatchNameSpaceId,
                nsIAtom* aHTMLMatchAtom,
                nsIAtom* aXMLMatchAtom,
                bool aDeep = true);

  














  
  nsContentList(nsINode* aRootNode,
                nsContentListMatchFunc aFunc,
                nsContentListDestroyFunc aDestroyFunc,
                void* aData,
                bool aDeep = true,
                nsIAtom* aMatchAtom = nullptr,
                int32_t aMatchNameSpaceId = kNameSpaceID_None,
                bool aFuncMayDependOnAttr = true);

  
  using nsWrapperCache::GetWrapperPreserveColor;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
protected:
  virtual ~nsContentList();

  virtual JSObject* GetWrapperPreserveColorInternal() MOZ_OVERRIDE
  {
    return nsWrapperCache::GetWrapperPreserveColor();
  }
public:

  
  NS_DECL_NSIDOMHTMLCOLLECTION

  
  virtual int32_t IndexOf(nsIContent *aContent, bool aDoFlush) MOZ_OVERRIDE;
  virtual int32_t IndexOf(nsIContent* aContent) MOZ_OVERRIDE;
  virtual nsINode* GetParentObject() MOZ_OVERRIDE
  {
    return mRootNode;
  }

  virtual nsIContent* Item(uint32_t aIndex) MOZ_OVERRIDE;
  virtual mozilla::dom::Element* GetElementAt(uint32_t index) MOZ_OVERRIDE;
  virtual mozilla::dom::Element*
  GetFirstNamedElement(const nsAString& aName, bool& aFound) MOZ_OVERRIDE
  {
    mozilla::dom::Element* item = NamedItem(aName, true);
    aFound = !!item;
    return item;
  }
  virtual void GetSupportedNames(unsigned aFlags,
                                 nsTArray<nsString>& aNames) MOZ_OVERRIDE;

  
  uint32_t Length(bool aDoFlush);
  nsIContent* Item(uint32_t aIndex, bool aDoFlush);
  mozilla::dom::Element*
  NamedItem(const nsAString& aName, bool aDoFlush);

  
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

  bool MatchesKey(const nsContentListKey& aKey) const
  {
    
    
    
    NS_PRECONDITION(mXMLMatchAtom,
                    "How did we get here with a null match atom on our list?");
    return
      mXMLMatchAtom->Equals(aKey.mTagname) &&
      mRootNode == aKey.mRootNode &&
      mMatchNameSpaceId == aKey.mMatchNameSpaceId;
  }

  



  void SetDirty()
  {
    mState = LIST_DIRTY;
    Reset();
  }

protected:
  





  bool Match(mozilla::dom::Element *aElement);
  






  bool MatchSelf(nsIContent *aContent);

  








  void PopulateSelf(uint32_t aNeededLength);

  






  bool MayContainRelevantNodes(nsINode* aContainer)
  {
    return mDeep || aContainer == mRootNode;
  }

  



  void RemoveFromHashtable();
  



  inline void BringSelfUpToDate(bool aDoFlush);

  




  virtual void RemoveFromCaches() MOZ_OVERRIDE
  {
    RemoveFromHashtable();
  }

  nsINode* mRootNode; 
  int32_t mMatchNameSpaceId;
  nsCOMPtr<nsIAtom> mHTMLMatchAtom;
  nsCOMPtr<nsIAtom> mXMLMatchAtom;

  



  nsContentListMatchFunc mFunc;
  


  nsContentListDestroyFunc mDestroyFunc;
  


  void* mData;
  



  uint8_t mState;

  
  
  
  
  
  


  uint8_t mMatchAll : 1;
  



  uint8_t mDeep : 1;
  



  uint8_t mFuncMayDependOnAttr : 1;
  


  uint8_t mFlushesNeeded : 1;

#ifdef DEBUG_CONTENT_LIST
  void AssertInSync();
#endif
};




class nsCacheableFuncStringContentList;

class MOZ_STACK_CLASS nsFuncStringCacheKey {
public:
  nsFuncStringCacheKey(nsINode* aRootNode,
                       nsContentListMatchFunc aFunc,
                       const nsAString& aString) :
    mRootNode(aRootNode),
    mFunc(aFunc),
    mString(aString)
    {}

  uint32_t GetHash(void) const
  {
    uint32_t hash = mozilla::HashString(mString);
    return mozilla::AddToHash(hash, mRootNode, mFunc);
  }

private:
  friend class nsCacheableFuncStringContentList;

  nsINode* const mRootNode;
  const nsContentListMatchFunc mFunc;
  const nsAString& mString;
};



class nsCacheableFuncStringContentList : public nsContentList {
public:
  virtual ~nsCacheableFuncStringContentList();

  bool Equals(const nsFuncStringCacheKey* aKey) {
    return mRootNode == aKey->mRootNode && mFunc == aKey->mFunc &&
      mString == aKey->mString;
  }

#ifdef DEBUG
  enum ContentListType {
    eNodeList,
    eHTMLCollection
  };
  ContentListType mType;
#endif

protected:
  nsCacheableFuncStringContentList(nsINode* aRootNode,
                                   nsContentListMatchFunc aFunc,
                                   nsContentListDestroyFunc aDestroyFunc,
                                   nsFuncStringContentListDataAllocator aDataAllocator,
                                   const nsAString& aString) :
    nsContentList(aRootNode, aFunc, aDestroyFunc, nullptr),
    mString(aString)
  {
    mData = (*aDataAllocator)(aRootNode, &mString);
    MOZ_ASSERT(mData);
  }

  virtual void RemoveFromCaches() MOZ_OVERRIDE {
    RemoveFromFuncStringHashtable();
  }
  void RemoveFromFuncStringHashtable();

  nsString mString;
};

class nsCacheableFuncStringNodeList
  : public nsCacheableFuncStringContentList
{
public:
  nsCacheableFuncStringNodeList(nsINode* aRootNode,
                                nsContentListMatchFunc aFunc,
                                nsContentListDestroyFunc aDestroyFunc,
                                nsFuncStringContentListDataAllocator aDataAllocator,
                                const nsAString& aString)
    : nsCacheableFuncStringContentList(aRootNode, aFunc, aDestroyFunc,
                                       aDataAllocator, aString)
  {
#ifdef DEBUG
    mType = eNodeList;
#endif
  }

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

#ifdef DEBUG
  static const ContentListType sType;
#endif
};

class nsCacheableFuncStringHTMLCollection
  : public nsCacheableFuncStringContentList
{
public:
  nsCacheableFuncStringHTMLCollection(nsINode* aRootNode,
                                      nsContentListMatchFunc aFunc,
                                      nsContentListDestroyFunc aDestroyFunc,
                                      nsFuncStringContentListDataAllocator aDataAllocator,
                                      const nsAString& aString)
    : nsCacheableFuncStringContentList(aRootNode, aFunc, aDestroyFunc,
                                       aDataAllocator, aString)
  {
#ifdef DEBUG
    mType = eHTMLCollection;
#endif
  }

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

#ifdef DEBUG
  static const ContentListType sType;
#endif
};

#endif 
