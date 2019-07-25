




































#ifndef nsINode_h___
#define nsINode_h___

#include "nsPIDOMEventTarget.h"
#include "nsEvent.h"
#include "nsPropertyTable.h"
#include "nsTObserverArray.h"
#include "nsINodeInfo.h"
#include "nsCOMPtr.h"
#include "nsWrapperCache.h"
#include "nsIProgrammingLanguage.h" 
#include "nsDOMError.h"
#include "nsDOMString.h"

class nsIContent;
class nsIDocument;
class nsIDOMEvent;
class nsIDOMNode;
class nsIDOMNodeList;
class nsINodeList;
class nsIPresShell;
class nsEventChainVisitor;
class nsEventChainPreVisitor;
class nsEventChainPostVisitor;
class nsIEventListenerManager;
class nsIPrincipal;
class nsIMutationObserver;
class nsChildContentList;
class nsNodeWeakReference;
class nsNodeSupportsWeakRefTearoff;
class nsIEditor;
class nsIVariant;
class nsIDOMUserDataHandler;
class nsAttrAndChildArray;

namespace mozilla {
namespace dom {
class Element;
} 
} 

enum {
  
  NODE_DOESNT_HAVE_SLOTS =       0x00000001U,

  
  
  NODE_HAS_LISTENERMANAGER =     0x00000002U,

  
  NODE_HAS_PROPERTIES =          0x00000004U,

  
  
  
  
  
  NODE_IS_ANONYMOUS =            0x00000008U,

  
  
  
  
  
  
  NODE_IS_IN_ANONYMOUS_SUBTREE = 0x00000010U,

  
  
  
  
  NODE_IS_NATIVE_ANONYMOUS_ROOT = 0x00000020U,

  
  
  NODE_FORCE_XBL_BINDINGS =      0x00000040U,

  
  NODE_MAY_BE_IN_BINDING_MNGR =  0x00000080U,

  NODE_IS_EDITABLE =             0x00000100U,

  
  
  
  NODE_HAS_ID =                  0x00000200U,
  
  
  NODE_MAY_HAVE_CLASS =          0x00000400U,
  NODE_MAY_HAVE_STYLE =          0x00000800U,

  NODE_IS_INSERTION_PARENT =     0x00001000U,

  
  NODE_HAS_EMPTY_SELECTOR =      0x00002000U,

  
  
  NODE_HAS_SLOW_SELECTOR =       0x00004000U,

  
  
  NODE_HAS_EDGE_CHILD_SELECTOR = 0x00008000U,

  
  
  
  
  
  
  
  NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS
                               = 0x00010000U,

  NODE_ALL_SELECTOR_FLAGS =      NODE_HAS_EMPTY_SELECTOR |
                                 NODE_HAS_SLOW_SELECTOR |
                                 NODE_HAS_EDGE_CHILD_SELECTOR |
                                 NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS,

  NODE_MAY_HAVE_CONTENT_EDITABLE_ATTR
                               = 0x00020000U,

  NODE_ATTACH_BINDING_ON_POSTCREATE
                               = 0x00040000U,

  
  
  NODE_NEEDS_FRAME =             0x00080000U,

  
  
  
  NODE_DESCENDANTS_NEED_FRAMES = 0x00100000U,

  
  NODE_IS_ELEMENT              = 0x00200000U,
  
  
  NODE_HAS_ACCESSKEY           = 0x00400000U,

  
  NODE_HAS_NAME                = 0x00800000U,

  
  NODE_SCRIPT_TYPE_OFFSET =               24,

  NODE_SCRIPT_TYPE_SIZE =                  4,

  
  NODE_TYPE_SPECIFIC_BITS_OFFSET =
    NODE_SCRIPT_TYPE_OFFSET + NODE_SCRIPT_TYPE_SIZE
};






template<class C, class D>
inline nsINode* NODE_FROM(C& aContent, D& aDocument)
{
  if (aContent)
    return static_cast<nsINode*>(aContent);
  return static_cast<nsINode*>(aDocument);
}
























class nsMutationGuard {
public:
  nsMutationGuard()
  {
    mDelta = eMaxMutations - sMutationCount;
    sMutationCount = eMaxMutations;
  }
  ~nsMutationGuard()
  {
    sMutationCount =
      mDelta > sMutationCount ? 0 : sMutationCount - mDelta;
  }

  



  PRBool Mutated(PRUint8 aIgnoreCount)
  {
    return sMutationCount < static_cast<PRUint32>(eMaxMutations - aIgnoreCount);
  }

  
  
  
  static void DidMutate()
  {
    if (sMutationCount) {
      --sMutationCount;
    }
  }

private:
  
  
  
  PRUint32 mDelta;

  
  
  enum { eMaxMutations = 300 };

  
  
  
  
  static PRUint32 sMutationCount;
};



#define DOM_USER_DATA         1
#define DOM_USER_DATA_HANDLER 2
#ifdef MOZ_SMIL
#define SMIL_MAPPED_ATTR_ANIMVAL 3
#endif 


#define NS_INODE_IID \
{ 0x58695b2f, 0x39bd, 0x434d, \
  { 0xa2, 0x0b, 0xde, 0xd3, 0xa8, 0xa0, 0xb6, 0x95 } } 






class nsINode : public nsPIDOMEventTarget,
                public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODE_IID)

  friend class nsNodeUtils;
  friend class nsNodeWeakReference;
  friend class nsNodeSupportsWeakRefTearoff;
  friend class nsAttrAndChildArray;

#ifdef MOZILLA_INTERNAL_API
  nsINode(nsINodeInfo* aNodeInfo)
    : mNodeInfo(aNodeInfo),
      mParentPtrBits(0),
      mFlagsOrSlots(NODE_DOESNT_HAVE_SLOTS),
      mNextSibling(nsnull),
      mPreviousSibling(nsnull),
      mFirstChild(nsnull)
  {
  }
#endif

  virtual ~nsINode();

  


  enum {
    
    eCONTENT             = 1 << 0,
    
    eDOCUMENT            = 1 << 1,
    
    eATTRIBUTE           = 1 << 2,
    
    eTEXT                = 1 << 3,
    
    ePROCESSING_INSTRUCTION = 1 << 4,
    
    eCOMMENT             = 1 << 5,
    
    eHTML_FORM_CONTROL   = 1 << 6,
    
    eSVG                 = 1 << 7,
    
    eDOCUMENT_FRAGMENT   = 1 << 8,
    

    eDATA_NODE           = 1 << 19,
    
    eMEDIA               = 1 << 10,
    
    eANIMATION           = 1 << 11
  };

  







  virtual PRBool IsNodeOfType(PRUint32 aFlags) const = 0;

  


  PRBool IsElement() const {
    return HasFlag(NODE_IS_ELEMENT);
  }

  



  mozilla::dom::Element* AsElement();

  



  virtual PRUint32 GetChildCount() const = 0;

  




  virtual nsIContent* GetChildAt(PRUint32 aIndex) const = 0;

  








  virtual nsIContent * const * GetChildArray(PRUint32* aChildCount) const = 0;

  







  virtual PRInt32 IndexOf(nsINode* aPossibleChild) const = 0;

  





  nsIDocument *GetOwnerDoc() const
  {
    return mNodeInfo->GetDocument();
  }

  




  PRBool IsInDoc() const
  {
    return mParentPtrBits & PARENT_BIT_INDOCUMENT;
  }

  





  nsIDocument *GetCurrentDoc() const
  {
    return IsInDoc() ? GetOwnerDoc() : nsnull;
  }

  NS_IMETHOD GetNodeType(PRUint16* aNodeType) = 0;

  nsINode*
  InsertBefore(nsINode *aNewChild, nsINode *aRefChild, nsresult *aReturn)
  {
    return ReplaceOrInsertBefore(PR_FALSE, aNewChild, aRefChild, aReturn);
  }
  nsINode*
  ReplaceChild(nsINode *aNewChild, nsINode *aOldChild, nsresult *aReturn)
  {
    return ReplaceOrInsertBefore(PR_TRUE, aNewChild, aOldChild, aReturn);
  }
  nsINode*
  AppendChild(nsINode *aNewChild, nsresult *aReturn)
  {
    return InsertBefore(aNewChild, nsnull, aReturn);
  }
  nsresult RemoveChild(nsINode *aOldChild)
  {
    if (!aOldChild) {
      return NS_ERROR_NULL_POINTER;
    }

    if (IsNodeOfType(eDATA_NODE)) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }

    PRInt32 index = IndexOf(aOldChild);
    if (index == -1) {
      
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }

    return RemoveChildAt(index, PR_TRUE);
  }

  



















  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify) = 0;

  

















  nsresult AppendChildTo(nsIContent* aKid, PRBool aNotify)
  {
    return InsertChildAt(aKid, GetChildCount(), aNotify);
  }
  
  











  virtual nsresult RemoveChildAt(PRUint32 aIndex, 
                                 PRBool aNotify, 
                                 PRBool aMutationEvent = PR_TRUE) = 0;

  










  void* GetProperty(nsIAtom *aPropertyName,
                    nsresult *aStatus = nsnull) const
  {
    return GetProperty(0, aPropertyName, aStatus);
  }

  











  virtual void* GetProperty(PRUint16 aCategory,
                            nsIAtom *aPropertyName,
                            nsresult *aStatus = nsnull) const;

  
















  nsresult SetProperty(nsIAtom *aPropertyName,
                       void *aValue,
                       NSPropertyDtorFunc aDtor = nsnull,
                       PRBool aTransfer = PR_FALSE)
  {
    return SetProperty(0, aPropertyName, aValue, aDtor, aTransfer);
  }

  


















  virtual nsresult SetProperty(PRUint16 aCategory,
                               nsIAtom *aPropertyName,
                               void *aValue,
                               NSPropertyDtorFunc aDtor = nsnull,
                               PRBool aTransfer = PR_FALSE,
                               void **aOldValue = nsnull);

  







  nsresult DeleteProperty(nsIAtom *aPropertyName)
  {
    return DeleteProperty(0, aPropertyName);
  }

  








  virtual nsresult DeleteProperty(PRUint16 aCategory, nsIAtom *aPropertyName);

  












  void* UnsetProperty(nsIAtom  *aPropertyName,
                      nsresult *aStatus = nsnull)
  {
    return UnsetProperty(0, aPropertyName, aStatus);
  }

  













  virtual void* UnsetProperty(PRUint16 aCategory,
                              nsIAtom *aPropertyName,
                              nsresult *aStatus = nsnull);
  
  PRBool HasProperties() const
  {
    return HasFlag(NODE_HAS_PROPERTIES);
  }

  



  nsIPrincipal* NodePrincipal() const {
    return mNodeInfo->NodeInfoManager()->DocumentPrincipal();
  }

  



  nsIContent* GetParent() const
  {
    return NS_LIKELY(mParentPtrBits & PARENT_BIT_PARENT_IS_CONTENT) ?
           reinterpret_cast<nsIContent*>
                           (mParentPtrBits & ~kParentBitMask) :
           nsnull;
  }

  




  nsINode* GetNodeParent() const
  {
    return reinterpret_cast<nsINode*>(mParentPtrBits & ~kParentBitMask);
  }

  









  void AddMutationObserver(nsIMutationObserver* aMutationObserver)
  {
    nsSlots* s = GetSlots();
    if (s) {
      NS_ASSERTION(s->mMutationObservers.IndexOf(aMutationObserver) ==
                   nsTArray_base::NoIndex,
                   "Observer already in the list");
      s->mMutationObservers.AppendElement(aMutationObserver);
    }
  }

  



  void AddMutationObserverUnlessExists(nsIMutationObserver* aMutationObserver)
  {
    nsSlots* s = GetSlots();
    if (s) {
      s->mMutationObservers.AppendElementUnlessExists(aMutationObserver);
    }
  }

  


  void RemoveMutationObserver(nsIMutationObserver* aMutationObserver)
  {
    nsSlots* s = GetExistingSlots();
    if (s) {
      s->mMutationObservers.RemoveElement(aMutationObserver);
    }
  }

  








  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const = 0;

  









  PRBool HasSameOwnerDoc(nsINode *aOther)
  {
    
    
    return mNodeInfo->NodeInfoManager() == aOther->mNodeInfo->NodeInfoManager();
  }

  
  
  class nsSlots
  {
  public:
    nsSlots(PtrBits aFlags)
      : mFlags(aFlags),
        mChildNodes(nsnull),
        mWeakReference(nsnull)
    {
    }

    
    
    virtual ~nsSlots();

    




    PtrBits mFlags;

    


    nsTObserverArray<nsIMutationObserver*> mMutationObservers;

    






    nsChildContentList* mChildNodes;

    


    nsNodeWeakReference* mWeakReference;
  };

  


#ifdef DEBUG
  nsSlots* DebugGetSlots()
  {
    return GetSlots();
  }
#endif

  PRBool HasFlag(PtrBits aFlag) const
  {
    return !!(GetFlags() & aFlag);
  }

  PtrBits GetFlags() const
  {
    return NS_UNLIKELY(HasSlots()) ? FlagsAsSlots()->mFlags : mFlagsOrSlots;
  }

  void SetFlags(PtrBits aFlagsToSet)
  {
    NS_ASSERTION(!(aFlagsToSet & (NODE_IS_ANONYMOUS |
                                  NODE_IS_NATIVE_ANONYMOUS_ROOT |
                                  NODE_IS_IN_ANONYMOUS_SUBTREE |
                                  NODE_ATTACH_BINDING_ON_POSTCREATE |
                                  NODE_DESCENDANTS_NEED_FRAMES |
                                  NODE_NEEDS_FRAME)) ||
                 IsNodeOfType(eCONTENT),
                 "Flag only permitted on nsIContent nodes");
    PtrBits* flags = HasSlots() ? &FlagsAsSlots()->mFlags :
                                  &mFlagsOrSlots;
    *flags |= aFlagsToSet;
  }

  void UnsetFlags(PtrBits aFlagsToUnset)
  {
    NS_ASSERTION(!(aFlagsToUnset &
                   (NODE_IS_ANONYMOUS |
                    NODE_IS_IN_ANONYMOUS_SUBTREE |
                    NODE_IS_NATIVE_ANONYMOUS_ROOT)),
                 "Trying to unset write-only flags");
    PtrBits* flags = HasSlots() ? &FlagsAsSlots()->mFlags :
                                  &mFlagsOrSlots;
    *flags &= ~aFlagsToUnset;
  }

  void SetEditableFlag(PRBool aEditable)
  {
    if (aEditable) {
      SetFlags(NODE_IS_EDITABLE);
    }
    else {
      UnsetFlags(NODE_IS_EDITABLE);
    }
  }

  PRBool IsEditable() const
  {
#ifdef _IMPL_NS_LAYOUT
    return IsEditableInternal();
#else
    return IsEditableExternal();
#endif
  }

  


  PRBool IsInNativeAnonymousSubtree() const
  {
#ifdef DEBUG
    if (HasFlag(NODE_IS_IN_ANONYMOUS_SUBTREE)) {
      return PR_TRUE;
    }
    CheckNotNativeAnonymous();
    return PR_FALSE;
#else
    return HasFlag(NODE_IS_IN_ANONYMOUS_SUBTREE);
#endif
  }

  




  nsIContent* GetTextEditorRootContent(nsIEditor** aEditor = nsnull);

  







  nsIContent* GetSelectionRootContent(nsIPresShell* aPresShell);

  virtual nsINodeList* GetChildNodesList();
  nsIContent* GetFirstChild() const { return mFirstChild; }
  nsIContent* GetLastChild() const
  {
    PRUint32 count;
    nsIContent* const* children = GetChildArray(&count);

    return count > 0 ? children[count - 1] : nsnull;
  }

  



  nsIDocument* GetOwnerDocument() const;

  



  class ChildIterator {
  public:
    ChildIterator(const nsINode* aNode) { Init(aNode); }
    ChildIterator(const nsINode* aNode, PRUint32 aOffset) {
      Init(aNode);
      Advance(aOffset);
    }
    ~ChildIterator() {
      NS_ASSERTION(!mGuard.Mutated(0), "Unexpected mutations happened");
    }

    PRBool IsDone() const { return mCur == mEnd; }
    operator nsIContent*() const { return *mCur; }
    void Next() { NS_PRECONDITION(mCur != mEnd, "Check IsDone"); ++mCur; }
    void Advance(PRUint32 aOffset) {
      NS_ASSERTION(mCur + aOffset <= mEnd, "Unexpected offset");
      mCur += aOffset;
    }
  private:
    void Init(const nsINode* aNode) {
      NS_PRECONDITION(aNode, "Must have node here!");
      PRUint32 childCount;
      mCur = aNode->GetChildArray(&childCount);
      mEnd = mCur + childCount;
    }
#ifdef DEBUG
    nsMutationGuard mGuard;
#endif
    nsIContent* const * mCur;
    nsIContent* const * mEnd;
  };

  



  virtual PRUint32 GetScriptTypeID() const
  { return nsIProgrammingLanguage::JAVASCRIPT; }

  


  NS_IMETHOD SetScriptTypeID(PRUint32 aLang)
  {
    NS_NOTREACHED("SetScriptTypeID not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  







  virtual already_AddRefed<nsIURI> GetBaseURI() const = 0;

  void GetBaseURI(nsAString &aURI) const;

  virtual void GetTextContent(nsAString &aTextContent)
  {
    SetDOMStringToNull(aTextContent);
  }
  virtual nsresult SetTextContent(const nsAString& aTextContent)
  {
    return NS_OK;
  }

  













  nsresult SetUserData(const nsAString& aKey, nsIVariant* aData,
                       nsIDOMUserDataHandler* aHandler, nsIVariant** aResult);

  







  nsIVariant* GetUserData(const nsAString& aKey)
  {
    nsCOMPtr<nsIAtom> key = do_GetAtom(aKey);
    if (!key) {
      return nsnull;
    }

    return static_cast<nsIVariant*>(GetProperty(DOM_USER_DATA, key));
  }

  nsresult GetFeature(const nsAString& aFeature,
                      const nsAString& aVersion,
                      nsISupports** aReturn);

  











  PRUint16 CompareDocumentPosition(nsINode* aOtherNode);
  nsresult CompareDocumentPosition(nsINode* aOtherNode, PRUint16* aResult)
  {
    NS_ENSURE_ARG(aOtherNode);

    *aResult = CompareDocumentPosition(aOtherNode);

    return NS_OK;
  }

  PRBool IsSameNode(nsINode *aOtherNode)
  {
    return aOtherNode == this;
  }

  virtual PRBool IsEqualNode(nsINode *aOtherNode) = 0;

  void LookupPrefix(const nsAString& aNamespaceURI, nsAString& aPrefix);
  PRBool IsDefaultNamespace(const nsAString& aNamespaceURI)
  {
    nsAutoString defaultNamespace;
    LookupNamespaceURI(EmptyString(), defaultNamespace);
    return aNamespaceURI.Equals(defaultNamespace);
  }
  void LookupNamespaceURI(const nsAString& aNamespacePrefix,
                          nsAString& aNamespaceURI);

  nsIContent* GetNextSibling() const { return mNextSibling; }
  nsIContent* GetPreviousSibling() const { return mPreviousSibling; }

  






  nsIContent* GetNextNode(const nsINode* aRoot = nsnull) const
  {
    
    
#ifdef DEBUG
    if (aRoot) {
      const nsINode* cur = this;
      for (; cur; cur = cur->GetNodeParent())
        if (cur == aRoot) break;
      NS_ASSERTION(cur, "aRoot not an ancestor of |this|?");
    }
#endif
    nsIContent* kid = GetFirstChild();
    if (kid) {
      return kid;
    }
    if (this == aRoot) {
      return nsnull;
    }
    const nsINode* cur = this;
    while (1) {
      nsIContent* next = cur->GetNextSibling();
      if (next) {
        return next;
      }
      nsINode* parent = cur->GetNodeParent();
      if (parent == aRoot) {
        return nsnull;
      }
      cur = parent;
    }
    NS_NOTREACHED("How did we get here?");
  }

protected:

  
  virtual nsINode::nsSlots* CreateSlots();

  PRBool HasSlots() const
  {
    return !(mFlagsOrSlots & NODE_DOESNT_HAVE_SLOTS);
  }

  nsSlots* FlagsAsSlots() const
  {
    NS_ASSERTION(HasSlots(), "check HasSlots first");
    return reinterpret_cast<nsSlots*>(mFlagsOrSlots);
  }

  nsSlots* GetExistingSlots() const
  {
    return HasSlots() ? FlagsAsSlots() : nsnull;
  }

  nsSlots* GetSlots()
  {
    if (HasSlots()) {
      return FlagsAsSlots();
    }

    nsSlots* newSlots = CreateSlots();
    if (newSlots) {
      mFlagsOrSlots = reinterpret_cast<PtrBits>(newSlots);
    }

    return newSlots;
  }

  nsTObserverArray<nsIMutationObserver*> *GetMutationObservers()
  {
    return HasSlots() ? &FlagsAsSlots()->mMutationObservers : nsnull;
  }

  PRBool IsEditableInternal() const;
  virtual PRBool IsEditableExternal() const
  {
    return IsEditableInternal();
  }

#ifdef DEBUG
  
  
  virtual void CheckNotNativeAnonymous() const;
#endif

  nsresult GetParentNode(nsIDOMNode** aParentNode);
  nsresult GetChildNodes(nsIDOMNodeList** aChildNodes);
  nsresult GetFirstChild(nsIDOMNode** aFirstChild);
  nsresult GetLastChild(nsIDOMNode** aLastChild);
  nsresult GetPreviousSibling(nsIDOMNode** aPrevSibling);
  nsresult GetNextSibling(nsIDOMNode** aNextSibling);
  nsresult GetOwnerDocument(nsIDOMDocument** aOwnerDocument);

  nsresult ReplaceOrInsertBefore(PRBool aReplace, nsIDOMNode *aNewChild,
                                 nsIDOMNode *aRefChild, nsIDOMNode **aReturn);
  nsINode* ReplaceOrInsertBefore(PRBool aReplace, nsINode *aNewChild,
                                 nsINode *aRefChild, nsresult *aReturn)
  {
    *aReturn = ReplaceOrInsertBefore(aReplace, aNewChild, aRefChild);
    if (NS_FAILED(*aReturn)) {
      return nsnull;
    }

    return aReplace ? aRefChild : aNewChild;
  }
  virtual nsresult ReplaceOrInsertBefore(PRBool aReplace, nsINode* aNewChild,
                                         nsINode* aRefChild);
  nsresult RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn);

  





  virtual mozilla::dom::Element* GetNameSpaceElement() = 0;

  










  nsresult doRemoveChildAt(PRUint32 aIndex, PRBool aNotify, nsIContent* aKid,
                           nsAttrAndChildArray& aChildArray,
                           PRBool aMutationEvent);

  









  nsresult doInsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                           PRBool aNotify, nsAttrAndChildArray& aChildArray);

  nsCOMPtr<nsINodeInfo> mNodeInfo;

  enum { PARENT_BIT_INDOCUMENT = 1 << 0, PARENT_BIT_PARENT_IS_CONTENT = 1 << 1 };
  enum { kParentBitMask = 0x3 };

  PtrBits mParentPtrBits;

  





  PtrBits mFlagsOrSlots;

  nsIContent* mNextSibling;
  nsIContent* mPreviousSibling;
  nsIContent* mFirstChild;
};


extern const nsIID kThisPtrOffsetsSID;


#define NS_OFFSET_AND_INTERFACE_TABLE_BEGIN_AMBIGUOUS(_class, _implClass)     \
  static const QITableEntry offsetAndQITable[] = {                            \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _implClass)

#define NS_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                           \
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN_AMBIGUOUS(_class, _class)

#define NS_OFFSET_AND_INTERFACE_TABLE_END                                     \
  { nsnull, 0 } };                                                            \
  if (aIID.Equals(kThisPtrOffsetsSID)) {                                      \
    *aInstancePtr =                                                           \
      const_cast<void*>(static_cast<const void*>(&offsetAndQITable));         \
    return NS_OK;                                                             \
  }

#define NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE                            \
  rv = NS_TableDrivenQI(this, offsetAndQITable, aIID, aInstancePtr);          \
  NS_INTERFACE_TABLE_TO_MAP_SEGUE




#define NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                      \
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN_AMBIGUOUS(_class, nsINode)              \
    NS_INTERFACE_TABLE_ENTRY(_class, nsINode)                       

#define NS_NODE_INTERFACE_TABLE2(_class, _i1, _i2)                            \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END                                           \
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE

#define NS_NODE_INTERFACE_TABLE3(_class, _i1, _i2, _i3)                       \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END                                           \
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE

#define NS_NODE_INTERFACE_TABLE4(_class, _i1, _i2, _i3, _i4)                  \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END                                           \
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE

#define NS_NODE_INTERFACE_TABLE5(_class, _i1, _i2, _i3, _i4, _i5)             \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END                                           \
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE

#define NS_NODE_INTERFACE_TABLE6(_class, _i1, _i2, _i3, _i4, _i5, _i6)        \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END                                           \
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE

#define NS_NODE_INTERFACE_TABLE7(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7)   \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END                                           \
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE

#define NS_NODE_INTERFACE_TABLE8(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7,   \
                                 _i8)                                         \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i8)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END                                           \
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE


NS_DEFINE_STATIC_IID_ACCESSOR(nsINode, NS_INODE_IID)


#define NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER \
  nsContentUtils::TraceWrapper(tmp, aCallback, aClosure);

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER \
  nsContentUtils::ReleaseWrapper(s, tmp);


#endif 
