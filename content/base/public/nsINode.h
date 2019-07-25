




































#ifndef nsINode_h___
#define nsINode_h___

#include "nsIDOMEventTarget.h"
#include "nsEvent.h"
#include "nsPropertyTable.h"
#include "nsTObserverArray.h"
#include "nsINodeInfo.h"
#include "nsCOMPtr.h"
#include "nsWrapperCache.h"
#include "nsIProgrammingLanguage.h" 
#include "nsDOMError.h"
#include "nsDOMString.h"
#include "jspubtd.h"
#include "nsDOMMemoryReporter.h"

class nsIContent;
class nsIDocument;
class nsIDOMEvent;
class nsIDOMNode;
class nsIDOMElement;
class nsIDOMNodeList;
class nsINodeList;
class nsIPresShell;
class nsEventChainVisitor;
class nsEventChainPreVisitor;
class nsEventChainPostVisitor;
class nsEventListenerManager;
class nsIPrincipal;
class nsIMutationObserver;
class nsChildContentList;
class nsNodeWeakReference;
class nsNodeSupportsWeakRefTearoff;
class nsIEditor;
class nsIVariant;
class nsIDOMUserDataHandler;
class nsAttrAndChildArray;
class nsXPCClassInfo;

namespace mozilla {
namespace dom {
class Element;
} 
} 

enum {
  
  
  NODE_HAS_LISTENERMANAGER =     0x00000001U,

  
  NODE_HAS_PROPERTIES =          0x00000002U,

  
  
  
  
  
  NODE_IS_ANONYMOUS =            0x00000004U,

  
  
  
  
  
  
  NODE_IS_IN_ANONYMOUS_SUBTREE = 0x00000008U,

  
  
  
  
  NODE_IS_NATIVE_ANONYMOUS_ROOT = 0x00000010U,

  
  
  NODE_FORCE_XBL_BINDINGS =      0x00000020U,

  
  NODE_MAY_BE_IN_BINDING_MNGR =  0x00000040U,

  NODE_IS_EDITABLE =             0x00000080U,

  
  
  NODE_MAY_HAVE_CLASS =          0x00000100U,

  NODE_IS_INSERTION_PARENT =     0x00000200U,

  
  NODE_HAS_EMPTY_SELECTOR =      0x00000400U,

  
  
  NODE_HAS_SLOW_SELECTOR =       0x00000800U,

  
  
  NODE_HAS_EDGE_CHILD_SELECTOR = 0x00001000U,

  
  
  
  
  
  
  
  NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS
                               = 0x00002000U,

  NODE_ALL_SELECTOR_FLAGS =      NODE_HAS_EMPTY_SELECTOR |
                                 NODE_HAS_SLOW_SELECTOR |
                                 NODE_HAS_EDGE_CHILD_SELECTOR |
                                 NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS,

  NODE_ATTACH_BINDING_ON_POSTCREATE
                               = 0x00004000U,

  
  
  NODE_NEEDS_FRAME =             0x00008000U,

  
  
  
  NODE_DESCENDANTS_NEED_FRAMES = 0x00010000U,

  
  NODE_HAS_ACCESSKEY           = 0x00020000U,

  
  NODE_HANDLING_CLICK          = 0x00040000U,

  
  
  
  
  NODE_SCRIPT_TYPE_OFFSET =               19,

  NODE_SCRIPT_TYPE_SIZE =                  2,

  NODE_SCRIPT_TYPE_MASK =  (1 << NODE_SCRIPT_TYPE_SIZE) - 1,

  
  NODE_TYPE_SPECIFIC_BITS_OFFSET =
    NODE_SCRIPT_TYPE_OFFSET + NODE_SCRIPT_TYPE_SIZE
};

PR_STATIC_ASSERT(PRUint32(nsIProgrammingLanguage::JAVASCRIPT) <=
                   PRUint32(NODE_SCRIPT_TYPE_MASK));
PR_STATIC_ASSERT(PRUint32(nsIProgrammingLanguage::PYTHON) <=
                   PRUint32(NODE_SCRIPT_TYPE_MASK));






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

  



  bool Mutated(PRUint8 aIgnoreCount)
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
{ 0x38f966a0, 0xd652, 0x4979, \
  { 0xae, 0x37, 0xc8, 0x75, 0x84, 0xfc, 0xf8, 0x43 } }






class nsINode : public nsIDOMEventTarget,
                public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODE_IID)

  NS_DECL_DOM_MEMORY_REPORTER_SIZEOF

  friend class nsNodeUtils;
  friend class nsNodeWeakReference;
  friend class nsNodeSupportsWeakRefTearoff;
  friend class nsAttrAndChildArray;

#ifdef MOZILLA_INTERNAL_API
  nsINode(already_AddRefed<nsINodeInfo> aNodeInfo)
  : mNodeInfo(aNodeInfo),
    mParent(nsnull),
    mFlags(0),
    mBoolFlags(0),
    mNextSibling(nsnull),
    mPreviousSibling(nsnull),
    mFirstChild(nsnull),
    mSlots(nsnull)
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

  







  virtual bool IsNodeOfType(PRUint32 aFlags) const = 0;

  


  bool IsElement() const {
    return GetBoolFlag(NodeIsElement);
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

  




  bool IsInDoc() const
  {
    return GetBoolFlag(IsInDocument);
  }

  





  nsIDocument *GetCurrentDoc() const
  {
    return IsInDoc() ? GetOwnerDoc() : nsnull;
  }

  



  PRUint16 NodeType() const
  {
    return mNodeInfo->NodeType();
  }
  const nsString& NodeName() const
  {
    return mNodeInfo->NodeName();
  }
  const nsString& LocalName() const
  {
    return mNodeInfo->LocalName();
  }

  nsINode*
  InsertBefore(nsINode *aNewChild, nsINode *aRefChild, nsresult *aReturn)
  {
    return ReplaceOrInsertBefore(false, aNewChild, aRefChild, aReturn);
  }
  nsINode*
  ReplaceChild(nsINode *aNewChild, nsINode *aOldChild, nsresult *aReturn)
  {
    return ReplaceOrInsertBefore(true, aNewChild, aOldChild, aReturn);
  }
  nsINode*
  AppendChild(nsINode *aNewChild, nsresult *aReturn)
  {
    return InsertBefore(aNewChild, nsnull, aReturn);
  }
  nsresult RemoveChild(nsINode *aOldChild);

  



















  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 bool aNotify) = 0;

  

















  nsresult AppendChildTo(nsIContent* aKid, bool aNotify)
  {
    return InsertChildAt(aKid, GetChildCount(), aNotify);
  }
  
  











  virtual nsresult RemoveChildAt(PRUint32 aIndex, 
                                 bool aNotify) = 0;

  










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
                       bool aTransfer = false)
  {
    return SetProperty(0, aPropertyName, aValue, aDtor, aTransfer);
  }

  


















  virtual nsresult SetProperty(PRUint16 aCategory,
                               nsIAtom *aPropertyName,
                               void *aValue,
                               NSPropertyDtorFunc aDtor = nsnull,
                               bool aTransfer = false,
                               void **aOldValue = nsnull);

  





  void DeleteProperty(nsIAtom *aPropertyName)
  {
    DeleteProperty(0, aPropertyName);
  }

  






  virtual void DeleteProperty(PRUint16 aCategory, nsIAtom *aPropertyName);

  












  void* UnsetProperty(nsIAtom  *aPropertyName,
                      nsresult *aStatus = nsnull)
  {
    return UnsetProperty(0, aPropertyName, aStatus);
  }

  













  virtual void* UnsetProperty(PRUint16 aCategory,
                              nsIAtom *aPropertyName,
                              nsresult *aStatus = nsnull);
  
  bool HasProperties() const
  {
    return HasFlag(NODE_HAS_PROPERTIES);
  }

  



  nsIPrincipal* NodePrincipal() const {
    return mNodeInfo->NodeInfoManager()->DocumentPrincipal();
  }

  



  nsIContent* GetParent() const {
    return NS_LIKELY(GetBoolFlag(ParentIsContent)) ?
      reinterpret_cast<nsIContent*>(mParent) : nsnull;
  }

  




  nsINode* GetNodeParent() const
  {
    return mParent;
  }
  
  



  nsINode* GetElementParent() const
  {
    return mParent && mParent->IsElement() ? mParent : nsnull;
  }

  


  NS_DECL_NSIDOMEVENTTARGET
  using nsIDOMEventTarget::AddEventListener;

  









  void AddMutationObserver(nsIMutationObserver* aMutationObserver)
  {
    nsSlots* s = GetSlots();
    if (s) {
      NS_ASSERTION(s->mMutationObservers.IndexOf(aMutationObserver) ==
                   nsTArray<int>::NoIndex,
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

  









  bool HasSameOwnerDoc(nsINode *aOther)
  {
    
    
    return mNodeInfo->NodeInfoManager() == aOther->mNodeInfo->NodeInfoManager();
  }

  
  
  class nsSlots
  {
  public:
    nsSlots()
      : mChildNodes(nsnull),
        mWeakReference(nsnull)
    {
    }

    
    
    virtual ~nsSlots();

    void Traverse(nsCycleCollectionTraversalCallback &cb);
    void Unlink();

    


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

  bool HasFlag(PtrBits aFlag) const
  {
    return !!(GetFlags() & aFlag);
  }

  PRUint32 GetFlags() const
  {
    return mFlags;
  }

  void SetFlags(PRUint32 aFlagsToSet)
  {
    NS_ASSERTION(!(aFlagsToSet & (NODE_IS_ANONYMOUS |
                                  NODE_IS_NATIVE_ANONYMOUS_ROOT |
                                  NODE_IS_IN_ANONYMOUS_SUBTREE |
                                  NODE_ATTACH_BINDING_ON_POSTCREATE |
                                  NODE_DESCENDANTS_NEED_FRAMES |
                                  NODE_NEEDS_FRAME)) ||
                 IsNodeOfType(eCONTENT),
                 "Flag only permitted on nsIContent nodes");
    mFlags |= aFlagsToSet;
  }

  void UnsetFlags(PRUint32 aFlagsToUnset)
  {
    NS_ASSERTION(!(aFlagsToUnset &
                   (NODE_IS_ANONYMOUS |
                    NODE_IS_IN_ANONYMOUS_SUBTREE |
                    NODE_IS_NATIVE_ANONYMOUS_ROOT)),
                 "Trying to unset write-only flags");
    mFlags &= ~aFlagsToUnset;
  }

  void SetEditableFlag(bool aEditable)
  {
    if (aEditable) {
      SetFlags(NODE_IS_EDITABLE);
    }
    else {
      UnsetFlags(NODE_IS_EDITABLE);
    }
  }

  bool IsEditable() const
  {
#ifdef _IMPL_NS_LAYOUT
    return IsEditableInternal();
#else
    return IsEditableExternal();
#endif
  }

  


  bool IsInNativeAnonymousSubtree() const
  {
#ifdef DEBUG
    if (HasFlag(NODE_IS_IN_ANONYMOUS_SUBTREE)) {
      return true;
    }
    CheckNotNativeAnonymous();
    return false;
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

  



  virtual PRUint32 GetScriptTypeID() const
  { return nsIProgrammingLanguage::JAVASCRIPT; }

  


  NS_IMETHOD SetScriptTypeID(PRUint32 aLang)
  {
    NS_NOTREACHED("SetScriptTypeID not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsresult Normalize();

  







  virtual already_AddRefed<nsIURI> GetBaseURI() const = 0;

  nsresult GetDOMBaseURI(nsAString &aURI) const;

  
  
  NS_IMETHOD GetTextContent(nsAString &aTextContent)
  {
    SetDOMStringToNull(aTextContent);
    return NS_OK;
  }
  NS_IMETHOD SetTextContent(const nsAString& aTextContent)
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

  nsresult GetUserData(const nsAString& aKey, nsIVariant** aResult)
  {
    NS_IF_ADDREF(*aResult = GetUserData(aKey));
  
    return NS_OK;
  }


  










  PRUint16 CompareDocPosition(nsINode* aOtherNode);
  nsresult CompareDocPosition(nsINode* aOtherNode, PRUint16* aReturn)
  {
    NS_ENSURE_ARG(aOtherNode);
    *aReturn = CompareDocPosition(aOtherNode);
    return NS_OK;
  }
  nsresult CompareDocumentPosition(nsIDOMNode* aOther,
                                   PRUint16* aReturn);

  nsresult IsSameNode(nsIDOMNode* aOther,
                      bool* aReturn);

  nsresult LookupPrefix(const nsAString& aNamespaceURI, nsAString& aPrefix);
  nsresult IsDefaultNamespace(const nsAString& aNamespaceURI, bool* aResult)
  {
    nsAutoString defaultNamespace;
    LookupNamespaceURI(EmptyString(), defaultNamespace);
    *aResult = aNamespaceURI.Equals(defaultNamespace);
    return NS_OK;
  }
  nsresult LookupNamespaceURI(const nsAString& aNamespacePrefix,
                              nsAString& aNamespaceURI);

  nsresult IsEqualNode(nsIDOMNode* aOther, bool* aReturn);
  bool IsEqualTo(nsINode* aOther);

  nsIContent* GetNextSibling() const { return mNextSibling; }
  nsIContent* GetPreviousSibling() const { return mPreviousSibling; }

  






  nsIContent* GetNextNode(const nsINode* aRoot = nsnull) const
  {
    return GetNextNodeImpl(aRoot, false);
  }

  






  nsIContent* GetNextNonChildNode(const nsINode* aRoot = nsnull) const
  {
    return GetNextNodeImpl(aRoot, true);
  }

  




  bool Contains(const nsINode* aOther) const;
  nsresult Contains(nsIDOMNode* aOther, bool* aReturn);

private:

  nsIContent* GetNextNodeImpl(const nsINode* aRoot,
                              const bool aSkipChildren) const
  {
    
    
#ifdef DEBUG
    if (aRoot) {
      const nsINode* cur = this;
      for (; cur; cur = cur->GetNodeParent())
        if (cur == aRoot) break;
      NS_ASSERTION(cur, "aRoot not an ancestor of |this|?");
    }
#endif
    if (!aSkipChildren) {
      nsIContent* kid = GetFirstChild();
      if (kid) {
        return kid;
      }
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

public:

  






  nsIContent* GetPreviousContent(const nsINode* aRoot = nsnull) const
  {
      
      
#ifdef DEBUG
      if (aRoot) {
        const nsINode* cur = this;
        for (; cur; cur = cur->GetNodeParent())
          if (cur == aRoot) break;
        NS_ASSERTION(cur, "aRoot not an ancestor of |this|?");
      }
#endif

    if (this == aRoot) {
      return nsnull;
    }
    nsIContent* cur = this->GetParent();
    nsIContent* iter = this->GetPreviousSibling();
    while (iter) {
      cur = iter;
      iter = reinterpret_cast<nsINode*>(iter)->GetLastChild();
    }
    return cur;
  }

  


private:
  enum BooleanFlag {
    
    NodeHasRenderingObservers,
    
    
    IsInDocument,
    
    ParentIsContent,
    
    NodeIsElement,
    
    
    
    ElementHasID,
    
    ElementMayHaveStyle,
    
    ElementHasName,
    
    ElementMayHaveContentEditableAttr,
    
    BooleanFlagCount
  };

  void SetBoolFlag(BooleanFlag name, bool value) {
    PR_STATIC_ASSERT(BooleanFlagCount <= 8*sizeof(mBoolFlags));
    mBoolFlags = (mBoolFlags & ~(1 << name)) | (value << name);
  }

  void SetBoolFlag(BooleanFlag name) {
    PR_STATIC_ASSERT(BooleanFlagCount <= 8*sizeof(mBoolFlags));
    mBoolFlags |= (1 << name);
  }

  void ClearBoolFlag(BooleanFlag name) {
    PR_STATIC_ASSERT(BooleanFlagCount <= 8*sizeof(mBoolFlags));
    mBoolFlags &= ~(1 << name);
  }

  bool GetBoolFlag(BooleanFlag name) const {
    PR_STATIC_ASSERT(BooleanFlagCount <= 8*sizeof(mBoolFlags));
    return mBoolFlags & (1 << name);
  }

public:
  bool HasRenderingObservers() const
    { return GetBoolFlag(NodeHasRenderingObservers); }
  void SetHasRenderingObservers(bool aValue)
    { SetBoolFlag(NodeHasRenderingObservers, aValue); }
  bool HasID() const { return GetBoolFlag(ElementHasID); }
  bool MayHaveStyle() const { return GetBoolFlag(ElementMayHaveStyle); }
  bool HasName() const { return GetBoolFlag(ElementHasName); }
  bool MayHaveContentEditableAttr() const
    { return GetBoolFlag(ElementMayHaveContentEditableAttr); }

protected:
  void SetParentIsContent(bool aValue) { SetBoolFlag(ParentIsContent, aValue); }
  void SetInDocument() { SetBoolFlag(IsInDocument); }
  void ClearInDocument() { ClearBoolFlag(IsInDocument); }
  void SetIsElement() { SetBoolFlag(NodeIsElement); }
  void ClearIsElement() { ClearBoolFlag(NodeIsElement); }
  void SetHasID() { SetBoolFlag(ElementHasID); }
  void ClearHasID() { ClearBoolFlag(ElementHasID); }
  void SetMayHaveStyle() { SetBoolFlag(ElementMayHaveStyle); }
  void SetHasName() { SetBoolFlag(ElementHasName); }
  void ClearHasName() { ClearBoolFlag(ElementHasName); }
  void SetMayHaveContentEditableAttr()
    { SetBoolFlag(ElementMayHaveContentEditableAttr); }

public:
  
  virtual nsXPCClassInfo* GetClassInfo() = 0;
protected:

  
  virtual nsINode::nsSlots* CreateSlots();

  bool HasSlots() const
  {
    return mSlots != nsnull;
  }

  nsSlots* GetExistingSlots() const
  {
    return mSlots;
  }

  nsSlots* GetSlots()
  {
    if (!HasSlots()) {
      mSlots = CreateSlots();
    }
    return GetExistingSlots();
  }

  nsTObserverArray<nsIMutationObserver*> *GetMutationObservers()
  {
    return HasSlots() ? &GetExistingSlots()->mMutationObservers : nsnull;
  }

  bool IsEditableInternal() const;
  virtual bool IsEditableExternal() const
  {
    return IsEditableInternal();
  }

#ifdef DEBUG
  
  
  virtual void CheckNotNativeAnonymous() const;
#endif

  nsresult GetParentNode(nsIDOMNode** aParentNode);
  nsresult GetParentElement(nsIDOMElement** aParentElement);
  nsresult GetChildNodes(nsIDOMNodeList** aChildNodes);
  nsresult GetFirstChild(nsIDOMNode** aFirstChild);
  nsresult GetLastChild(nsIDOMNode** aLastChild);
  nsresult GetPreviousSibling(nsIDOMNode** aPrevSibling);
  nsresult GetNextSibling(nsIDOMNode** aNextSibling);
  nsresult GetOwnerDocument(nsIDOMDocument** aOwnerDocument);

  nsresult ReplaceOrInsertBefore(bool aReplace, nsIDOMNode *aNewChild,
                                 nsIDOMNode *aRefChild, nsIDOMNode **aReturn);
  nsINode* ReplaceOrInsertBefore(bool aReplace, nsINode *aNewChild,
                                 nsINode *aRefChild, nsresult *aReturn)
  {
    *aReturn = ReplaceOrInsertBefore(aReplace, aNewChild, aRefChild);
    if (NS_FAILED(*aReturn)) {
      return nsnull;
    }

    return aReplace ? aRefChild : aNewChild;
  }
  virtual nsresult ReplaceOrInsertBefore(bool aReplace, nsINode* aNewChild,
                                         nsINode* aRefChild);
  nsresult RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn);

  





  virtual mozilla::dom::Element* GetNameSpaceElement() = 0;

  










  nsresult doRemoveChildAt(PRUint32 aIndex, bool aNotify, nsIContent* aKid,
                           nsAttrAndChildArray& aChildArray);

  









  nsresult doInsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                           bool aNotify, nsAttrAndChildArray& aChildArray);

public:
  







#define EVENT(name_, id_, type_, struct_)                         \
  NS_IMETHOD GetOn##name_(JSContext *cx, JS::Value *vp);          \
  NS_IMETHOD SetOn##name_(JSContext *cx, const JS::Value &v);
#define TOUCH_EVENT EVENT
#define DOCUMENT_ONLY_EVENT EVENT
#include "nsEventNameList.h"
#undef DOCUMENT_ONLY_EVENT
#undef TOUCH_EVENT
#undef EVENT  

protected:
  static void Trace(nsINode *tmp, TraceCallback cb, void *closure);
  static bool Traverse(nsINode *tmp, nsCycleCollectionTraversalCallback &cb);
  static void Unlink(nsINode *tmp);

  nsCOMPtr<nsINodeInfo> mNodeInfo;

  nsINode* mParent;

  PRUint32 mFlags;

private:
  
  PRUint32 mBoolFlags;

protected:
  nsIContent* mNextSibling;
  nsIContent* mPreviousSibling;
  nsIContent* mFirstChild;

  
  nsSlots* mSlots;
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
