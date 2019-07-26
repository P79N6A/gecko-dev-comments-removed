




#ifndef nsINode_h___
#define nsINode_h___

#include "mozilla/ErrorResult.h"
#include "mozilla/Likely.h"
#include "nsCOMPtr.h"               
#include "nsGkAtoms.h"              
#include "nsIDOMEventTarget.h"      
#include "nsIDOMNode.h"
#include "nsIDOMNodeSelector.h"     
#include "nsINodeInfo.h"            
#include "nsIVariant.h"             
#include "nsNodeInfoManager.h"      
#include "nsPropertyTable.h"        
#include "nsTObserverArray.h"       
#include "nsWindowMemoryReporter.h" 
#include "mozilla/dom/EventTarget.h" 


#ifdef XP_WIN
#ifdef GetClassInfo
#undef GetClassInfo
#endif
#endif

class nsAttrAndChildArray;
class nsChildContentList;
class nsDOMAttributeMap;
class nsIContent;
class nsIDocument;
class nsIDOMElement;
class nsIDOMMozNamedAttrMap;
class nsIDOMNodeList;
class nsIDOMUserDataHandler;
class nsIEditor;
class nsIFrame;
class nsIMutationObserver;
class nsINodeList;
class nsIPresShell;
class nsIPrincipal;
class nsIURI;
class nsNodeSupportsWeakRefTearoff;
class nsNodeWeakReference;
class nsXPCClassInfo;

namespace mozilla {
namespace dom {




inline bool IsSpaceCharacter(PRUnichar aChar) {
  return aChar == ' ' || aChar == '\t' || aChar == '\n' || aChar == '\r' ||
         aChar == '\f';
}
inline bool IsSpaceCharacter(char aChar) {
  return aChar == ' ' || aChar == '\t' || aChar == '\n' || aChar == '\r' ||
         aChar == '\f';
}
class Element;
class EventHandlerNonNull;
class OnErrorEventHandlerNonNull;
template<typename T> class Optional;
} 
} 

namespace JS {
class Value;
}

#define NODE_FLAG_BIT(n_) (1U << (n_))

enum {
  
  NODE_HAS_LISTENERMANAGER =              NODE_FLAG_BIT(0),

  
  NODE_HAS_PROPERTIES =                   NODE_FLAG_BIT(1),

  
  
  
  
  
  NODE_IS_ANONYMOUS =                     NODE_FLAG_BIT(2),

  
  
  
  
  
  
  NODE_IS_IN_ANONYMOUS_SUBTREE =          NODE_FLAG_BIT(3),

  
  
  
  
  NODE_IS_NATIVE_ANONYMOUS_ROOT =         NODE_FLAG_BIT(4),

  
  
  NODE_FORCE_XBL_BINDINGS =               NODE_FLAG_BIT(5),

  
  NODE_MAY_BE_IN_BINDING_MNGR =           NODE_FLAG_BIT(6),

  NODE_IS_EDITABLE =                      NODE_FLAG_BIT(7),

  
  
  NODE_MAY_HAVE_CLASS =                   NODE_FLAG_BIT(8),

  NODE_IS_INSERTION_PARENT =              NODE_FLAG_BIT(9),

  
  NODE_HAS_EMPTY_SELECTOR =               NODE_FLAG_BIT(10),

  
  
  NODE_HAS_SLOW_SELECTOR =                NODE_FLAG_BIT(11),

  
  
  NODE_HAS_EDGE_CHILD_SELECTOR =          NODE_FLAG_BIT(12),

  
  
  
  
  
  
  
  NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS = NODE_FLAG_BIT(13),

  NODE_ALL_SELECTOR_FLAGS =               NODE_HAS_EMPTY_SELECTOR |
                                          NODE_HAS_SLOW_SELECTOR |
                                          NODE_HAS_EDGE_CHILD_SELECTOR |
                                          NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS,

  NODE_ATTACH_BINDING_ON_POSTCREATE =     NODE_FLAG_BIT(14),

  
  
  NODE_NEEDS_FRAME =                      NODE_FLAG_BIT(15),

  
  
  
  NODE_DESCENDANTS_NEED_FRAMES =          NODE_FLAG_BIT(16),

  
  NODE_HAS_ACCESSKEY =                    NODE_FLAG_BIT(17),

  
  NODE_HANDLING_CLICK =                   NODE_FLAG_BIT(18),

  
  NODE_HAS_RELEVANT_HOVER_RULES =         NODE_FLAG_BIT(19),

  
  NODE_HAS_DIRECTION_RTL =                NODE_FLAG_BIT(20),

  
  NODE_HAS_DIRECTION_LTR =                NODE_FLAG_BIT(21),

  NODE_ALL_DIRECTION_FLAGS =              NODE_HAS_DIRECTION_LTR |
                                          NODE_HAS_DIRECTION_RTL,

  NODE_CHROME_ONLY_ACCESS =               NODE_FLAG_BIT(22),

  NODE_IS_ROOT_OF_CHROME_ONLY_ACCESS =    NODE_FLAG_BIT(23),

  
  NODE_TYPE_SPECIFIC_BITS_OFFSET =        24
};
























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

  



  bool Mutated(uint8_t aIgnoreCount)
  {
    return sMutationCount < static_cast<uint32_t>(eMaxMutations - aIgnoreCount);
  }

  
  
  
  static void DidMutate()
  {
    if (sMutationCount) {
      --sMutationCount;
    }
  }

private:
  
  
  
  uint32_t mDelta;

  
  
  enum { eMaxMutations = 300 };

  
  
  
  
  static uint32_t sMutationCount;
};



#define DOM_USER_DATA         1
#define DOM_USER_DATA_HANDLER 2
#define SMIL_MAPPED_ATTR_ANIMVAL 3


#define NS_INODE_IID \
{ 0xb3ee8053, 0x43b0, 0x44bc, \
  { 0xa0, 0x97, 0x18, 0x24, 0xd2, 0xac, 0x65, 0xb6 } }






class nsINode : public mozilla::dom::EventTarget
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODE_IID)

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  NS_DECL_SIZEOF_EXCLUDING_THIS

  
  
  
  
  
  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  friend class nsNodeUtils;
  friend class nsNodeWeakReference;
  friend class nsNodeSupportsWeakRefTearoff;
  friend class nsAttrAndChildArray;

#ifdef MOZILLA_INTERNAL_API
#ifdef _MSC_VER
#pragma warning(push)

#pragma warning(disable:4355)
#endif
  nsINode(already_AddRefed<nsINodeInfo> aNodeInfo)
  : mNodeInfo(aNodeInfo),
    mParent(nullptr),
    mFlags(0),
    mBoolFlags(0),
    mNextSibling(nullptr),
    mPreviousSibling(nullptr),
    mFirstChild(nullptr),
    mSubtreeRoot(this),
    mSlots(nullptr)
  {
  }

#ifdef _MSC_VER
#pragma warning(pop)
#endif
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
    
    eDOCUMENT_FRAGMENT   = 1 << 7,
    

    eDATA_NODE           = 1 << 8,
    
    eMEDIA               = 1 << 9,
    
    eANIMATION           = 1 << 10,
    
    eFILTER              = 1 << 11
  };

  







  virtual bool IsNodeOfType(uint32_t aFlags) const = 0;

  virtual JSObject* WrapObject(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

protected:
  




  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope)
  {
    MOZ_ASSERT(!IsDOMBinding(), "Someone forgot to override WrapNode");
    return nullptr;
  }

public:
  nsIDocument* GetParentObject() const
  {
    
    
    
    
    
    
    return OwnerDoc();
  }

  


  bool IsElement() const {
    return GetBoolFlag(NodeIsElement);
  }

  



  mozilla::dom::Element* AsElement();
  const mozilla::dom::Element* AsElement() const;

  



  nsIContent* AsContent();

  virtual nsIDOMNode* AsDOMNode() = 0;

  


  bool HasChildren() const { return !!mFirstChild; }

  



  virtual uint32_t GetChildCount() const = 0;

  




  virtual nsIContent* GetChildAt(uint32_t aIndex) const = 0;

  








  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const = 0;

  







  virtual int32_t IndexOf(const nsINode* aPossibleChild) const = 0;

  





  nsIDocument *OwnerDoc() const
  {
    return mNodeInfo->GetDocument();
  }

  



  nsINode *OwnerDocAsNode() const;

  




  bool IsInDoc() const
  {
    return GetBoolFlag(IsInDocument);
  }

  





  nsIDocument *GetCurrentDoc() const
  {
    return IsInDoc() ? OwnerDoc() : nullptr;
  }

  



  uint16_t NodeType() const
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

  





  nsIAtom* Tag() const
  {
    return mNodeInfo->NameAtom();
  }

  



















  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) = 0;

  

















  nsresult AppendChildTo(nsIContent* aKid, bool aNotify)
  {
    return InsertChildAt(aKid, GetChildCount(), aNotify);
  }
  
  










  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) = 0;

  










  void* GetProperty(nsIAtom *aPropertyName,
                    nsresult *aStatus = nullptr) const
  {
    return GetProperty(0, aPropertyName, aStatus);
  }

  











  virtual void* GetProperty(uint16_t aCategory,
                            nsIAtom *aPropertyName,
                            nsresult *aStatus = nullptr) const;

  
















  nsresult SetProperty(nsIAtom *aPropertyName,
                       void *aValue,
                       NSPropertyDtorFunc aDtor = nullptr,
                       bool aTransfer = false)
  {
    return SetProperty(0, aPropertyName, aValue, aDtor, aTransfer);
  }

  


















  virtual nsresult SetProperty(uint16_t aCategory,
                               nsIAtom *aPropertyName,
                               void *aValue,
                               NSPropertyDtorFunc aDtor = nullptr,
                               bool aTransfer = false,
                               void **aOldValue = nullptr);

  





  void DeleteProperty(nsIAtom *aPropertyName)
  {
    DeleteProperty(0, aPropertyName);
  }

  






  virtual void DeleteProperty(uint16_t aCategory, nsIAtom *aPropertyName);

  












  void* UnsetProperty(nsIAtom  *aPropertyName,
                      nsresult *aStatus = nullptr)
  {
    return UnsetProperty(0, aPropertyName, aStatus);
  }

  













  virtual void* UnsetProperty(uint16_t aCategory,
                              nsIAtom *aPropertyName,
                              nsresult *aStatus = nullptr);
  
  bool HasProperties() const
  {
    return HasFlag(NODE_HAS_PROPERTIES);
  }

  



  nsIPrincipal* NodePrincipal() const {
    return mNodeInfo->NodeInfoManager()->DocumentPrincipal();
  }

  



  nsIContent* GetParent() const {
    return MOZ_LIKELY(GetBoolFlag(ParentIsContent)) ?
      reinterpret_cast<nsIContent*>(mParent) : nullptr;
  }

  




  nsINode* GetParentNode() const
  {
    return mParent;
  }
  
  



  mozilla::dom::Element* GetParentElement() const
  {
    return mParent && mParent->IsElement() ? mParent->AsElement() : nullptr;
  }

  




  nsINode* SubtreeRoot() const
  {
    
    
    
    
    
    
    nsINode* node = IsInDoc() ? OwnerDocAsNode() : mSubtreeRoot;
    NS_ASSERTION(node, "Should always have a node here!");
#ifdef DEBUG
    {
      const nsINode* slowNode = this;
      const nsINode* iter = slowNode;
      while ((iter = iter->GetParentNode())) {
        slowNode = iter;
      }

      NS_ASSERTION(slowNode == node, "These should always be in sync!");
    }
#endif
    return node;
  }

  


  NS_DECL_NSIDOMEVENTTARGET
  using nsIDOMEventTarget::AddEventListener;
  using nsIDOMEventTarget::AddSystemEventListener;

  









  void AddMutationObserver(nsIMutationObserver* aMutationObserver)
  {
    nsSlots* s = Slots();
    NS_ASSERTION(s->mMutationObservers.IndexOf(aMutationObserver) ==
                 nsTArray<int>::NoIndex,
                 "Observer already in the list");
    s->mMutationObservers.AppendElement(aMutationObserver);
  }

  



  void AddMutationObserverUnlessExists(nsIMutationObserver* aMutationObserver)
  {
    nsSlots* s = Slots();
    s->mMutationObservers.AppendElementUnlessExists(aMutationObserver);
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
      : mChildNodes(nullptr),
        mWeakReference(nullptr)
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
    return Slots();
  }
#endif

  bool HasFlag(uintptr_t aFlag) const
  {
    return !!(GetFlags() & aFlag);
  }

  uint32_t GetFlags() const
  {
    return mFlags;
  }

  void SetFlags(uint32_t aFlagsToSet)
  {
    NS_ASSERTION(!(aFlagsToSet & (NODE_IS_ANONYMOUS |
                                  NODE_IS_NATIVE_ANONYMOUS_ROOT |
                                  NODE_IS_IN_ANONYMOUS_SUBTREE |
                                  NODE_ATTACH_BINDING_ON_POSTCREATE |
                                  NODE_DESCENDANTS_NEED_FRAMES |
                                  NODE_NEEDS_FRAME |
                                  NODE_CHROME_ONLY_ACCESS)) ||
                 IsNodeOfType(eCONTENT),
                 "Flag only permitted on nsIContent nodes");
    mFlags |= aFlagsToSet;
  }

  void UnsetFlags(uint32_t aFlagsToUnset)
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

  
  
  bool ChromeOnlyAccess() const
  {
    return HasFlag(NODE_IS_IN_ANONYMOUS_SUBTREE | NODE_CHROME_ONLY_ACCESS);
  }

  





  bool IsSelectionDescendant() const
  {
    return IsDescendantOfCommonAncestorForRangeInSelection() ||
           IsCommonAncestorForRangeInSelection();
  }

  




  nsIContent* GetTextEditorRootContent(nsIEditor** aEditor = nullptr);

  







  nsIContent* GetSelectionRootContent(nsIPresShell* aPresShell);

  virtual nsINodeList* ChildNodes();
  nsIContent* GetFirstChild() const { return mFirstChild; }
  nsIContent* GetLastChild() const
  {
    uint32_t count;
    nsIContent* const* children = GetChildArray(&count);

    return count > 0 ? children[count - 1] : nullptr;
  }

  



  nsIDocument* GetOwnerDocument() const;

  void Normalize();

  







  virtual already_AddRefed<nsIURI> GetBaseURI() const = 0;
  already_AddRefed<nsIURI> GetBaseURIObject() const
  {
    return GetBaseURI();
  }

  


  nsresult SetExplicitBaseURI(nsIURI* aURI);
  


protected:
  nsIURI* GetExplicitBaseURI() const {
    if (HasExplicitBaseURI()) {
      return static_cast<nsIURI*>(GetProperty(nsGkAtoms::baseURIProperty));
    }
    return nullptr;
  }
  
public:
  void GetTextContent(nsAString& aTextContent)
  {
    GetTextContentInternal(aTextContent);
  }
  void SetTextContent(const nsAString& aTextContent,
                      mozilla::ErrorResult& aError)
  {
    SetTextContentInternal(aTextContent, aError);
  }

  mozilla::dom::Element* QuerySelector(const nsAString& aSelector,
                                       mozilla::ErrorResult& aResult);
  already_AddRefed<nsINodeList> QuerySelectorAll(const nsAString& aSelector,
                                                 mozilla::ErrorResult& aResult);

  













  nsresult SetUserData(const nsAString& aKey, nsIVariant* aData,
                       nsIDOMUserDataHandler* aHandler, nsIVariant** aResult);

  







  nsIVariant* GetUserData(const nsAString& aKey)
  {
    nsCOMPtr<nsIAtom> key = do_GetAtom(aKey);
    if (!key) {
      return nullptr;
    }

    return static_cast<nsIVariant*>(GetProperty(DOM_USER_DATA, key));
  }

  nsresult GetUserData(const nsAString& aKey, nsIVariant** aResult)
  {
    NS_IF_ADDREF(*aResult = GetUserData(aKey));
  
    return NS_OK;
  }

  



  static bool ShouldExposeUserData(JSContext* aCx, JSObject* );

  void LookupPrefix(const nsAString& aNamespace, nsAString& aResult);
  bool IsDefaultNamespace(const nsAString& aNamespaceURI)
  {
    nsAutoString defaultNamespace;
    LookupNamespaceURI(EmptyString(), defaultNamespace);
    return aNamespaceURI.Equals(defaultNamespace);
  }
  void LookupNamespaceURI(const nsAString& aNamespacePrefix,
                          nsAString& aNamespaceURI);

  nsresult IsEqualNode(nsIDOMNode* aOther, bool* aReturn);

  nsIContent* GetNextSibling() const { return mNextSibling; }
  nsIContent* GetPreviousSibling() const { return mPreviousSibling; }

  






  nsIContent* GetNextNode(const nsINode* aRoot = nullptr) const
  {
    return GetNextNodeImpl(aRoot, false);
  }

  






  nsIContent* GetNextNonChildNode(const nsINode* aRoot = nullptr) const
  {
    return GetNextNodeImpl(aRoot, true);
  }

  




  bool Contains(const nsINode* aOther) const;
  nsresult Contains(nsIDOMNode* aOther, bool* aReturn);

  bool UnoptimizableCCNode() const;

private:

  nsIContent* GetNextNodeImpl(const nsINode* aRoot,
                              const bool aSkipChildren) const
  {
    
    
#ifdef DEBUG
    if (aRoot) {
      const nsINode* cur = this;
      for (; cur; cur = cur->GetParentNode())
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
      return nullptr;
    }
    const nsINode* cur = this;
    while (1) {
      nsIContent* next = cur->GetNextSibling();
      if (next) {
        return next;
      }
      nsINode* parent = cur->GetParentNode();
      if (parent == aRoot) {
        return nullptr;
      }
      cur = parent;
    }
    NS_NOTREACHED("How did we get here?");
  }

public:

  






  nsIContent* GetPreviousContent(const nsINode* aRoot = nullptr) const
  {
      
      
#ifdef DEBUG
      if (aRoot) {
        const nsINode* cur = this;
        for (; cur; cur = cur->GetParentNode())
          if (cur == aRoot) break;
        NS_ASSERTION(cur, "aRoot not an ancestor of |this|?");
      }
#endif

    if (this == aRoot) {
      return nullptr;
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
    
    
    NodeIsCommonAncestorForRangeInSelection,
    
    NodeIsDescendantOfCommonAncestorForRangeInSelection,
    
    NodeIsCCMarkedRoot,
    
    NodeIsCCBlackTree,
    
    
    NodeIsPurpleRoot,
    
    NodeHasExplicitBaseURI,
    
    ElementHasLockedStyleStates,
    
    ElementHasPointerLock,
    
    NodeMayHaveDOMMutationObserver,
    
    NodeIsContent,
    
    ElementHasAnimations,
    
    NodeHasValidDirAttribute,
    
    NodeHasFixedDir,
    
    
    NodeHasDirAutoSet,
    
    
    
    NodeHasTextNodeDirectionalityMap,
    
    NodeHasDirAuto,
    
    NodeAncestorHasDirAuto,
    
    
    ElementIsInStyleScope,
    
    ElementIsScopedStyleRoot,
    
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
  bool IsContent() const { return GetBoolFlag(NodeIsContent); }
  bool HasID() const { return GetBoolFlag(ElementHasID); }
  bool MayHaveStyle() const { return GetBoolFlag(ElementMayHaveStyle); }
  bool HasName() const { return GetBoolFlag(ElementHasName); }
  bool MayHaveContentEditableAttr() const
    { return GetBoolFlag(ElementMayHaveContentEditableAttr); }
  bool IsCommonAncestorForRangeInSelection() const
    { return GetBoolFlag(NodeIsCommonAncestorForRangeInSelection); }
  void SetCommonAncestorForRangeInSelection()
    { SetBoolFlag(NodeIsCommonAncestorForRangeInSelection); }
  void ClearCommonAncestorForRangeInSelection()
    { ClearBoolFlag(NodeIsCommonAncestorForRangeInSelection); }
  bool IsDescendantOfCommonAncestorForRangeInSelection() const
    { return GetBoolFlag(NodeIsDescendantOfCommonAncestorForRangeInSelection); }
  void SetDescendantOfCommonAncestorForRangeInSelection()
    { SetBoolFlag(NodeIsDescendantOfCommonAncestorForRangeInSelection); }
  void ClearDescendantOfCommonAncestorForRangeInSelection()
    { ClearBoolFlag(NodeIsDescendantOfCommonAncestorForRangeInSelection); }

  void SetCCMarkedRoot(bool aValue)
    { SetBoolFlag(NodeIsCCMarkedRoot, aValue); }
  bool CCMarkedRoot() const { return GetBoolFlag(NodeIsCCMarkedRoot); }
  void SetInCCBlackTree(bool aValue)
    { SetBoolFlag(NodeIsCCBlackTree, aValue); }
  bool InCCBlackTree() const { return GetBoolFlag(NodeIsCCBlackTree); }
  void SetIsPurpleRoot(bool aValue)
    { SetBoolFlag(NodeIsPurpleRoot, aValue); }
  bool IsPurpleRoot() const { return GetBoolFlag(NodeIsPurpleRoot); }
  bool MayHaveDOMMutationObserver()
    { return GetBoolFlag(NodeMayHaveDOMMutationObserver); }
  void SetMayHaveDOMMutationObserver()
    { SetBoolFlag(NodeMayHaveDOMMutationObserver, true); }
  bool HasListenerManager() { return HasFlag(NODE_HAS_LISTENERMANAGER); }
  bool HasPointerLock() const { return GetBoolFlag(ElementHasPointerLock); }
  void SetPointerLock() { SetBoolFlag(ElementHasPointerLock); }
  void ClearPointerLock() { ClearBoolFlag(ElementHasPointerLock); }
  bool MayHaveAnimations() { return GetBoolFlag(ElementHasAnimations); }
  void SetMayHaveAnimations() { SetBoolFlag(ElementHasAnimations); }
  void SetHasValidDir() { SetBoolFlag(NodeHasValidDirAttribute); }
  void ClearHasValidDir() { ClearBoolFlag(NodeHasValidDirAttribute); }
  bool HasValidDir() const { return GetBoolFlag(NodeHasValidDirAttribute); }
  void SetHasFixedDir() {
    MOZ_ASSERT(NodeType() != nsIDOMNode::TEXT_NODE,
               "SetHasFixedDir on text node");
    SetBoolFlag(NodeHasFixedDir);
  }
  void ClearHasFixedDir() {
    MOZ_ASSERT(NodeType() != nsIDOMNode::TEXT_NODE,
               "ClearHasFixedDir on text node");
    ClearBoolFlag(NodeHasFixedDir);
  }
  bool HasFixedDir() const { return GetBoolFlag(NodeHasFixedDir); }
  void SetHasDirAutoSet() {
    MOZ_ASSERT(NodeType() != nsIDOMNode::TEXT_NODE,
               "SetHasDirAutoSet on text node");
    SetBoolFlag(NodeHasDirAutoSet);
  }
  void ClearHasDirAutoSet() {
    MOZ_ASSERT(NodeType() != nsIDOMNode::TEXT_NODE,
               "ClearHasDirAutoSet on text node");
    ClearBoolFlag(NodeHasDirAutoSet);
  }
  bool HasDirAutoSet() const
    { return GetBoolFlag(NodeHasDirAutoSet); }
  void SetHasTextNodeDirectionalityMap() {
    MOZ_ASSERT(NodeType() == nsIDOMNode::TEXT_NODE,
               "SetHasTextNodeDirectionalityMap on non-text node");
    SetBoolFlag(NodeHasTextNodeDirectionalityMap);
  }
  void ClearHasTextNodeDirectionalityMap() {
    MOZ_ASSERT(NodeType() == nsIDOMNode::TEXT_NODE,
               "ClearHasTextNodeDirectionalityMap on non-text node");
    ClearBoolFlag(NodeHasTextNodeDirectionalityMap);
  }
  bool HasTextNodeDirectionalityMap() const
    { return GetBoolFlag(NodeHasTextNodeDirectionalityMap); }

  void SetHasDirAuto() { SetBoolFlag(NodeHasDirAuto); }
  void ClearHasDirAuto() { ClearBoolFlag(NodeHasDirAuto); }
  bool HasDirAuto() const { return GetBoolFlag(NodeHasDirAuto); }

  void SetAncestorHasDirAuto() { SetBoolFlag(NodeAncestorHasDirAuto); }
  void ClearAncestorHasDirAuto() { ClearBoolFlag(NodeAncestorHasDirAuto); }
  bool AncestorHasDirAuto() const { return GetBoolFlag(NodeAncestorHasDirAuto); }

  bool NodeOrAncestorHasDirAuto() const
    { return HasDirAuto() || AncestorHasDirAuto(); }

  void SetIsElementInStyleScope(bool aValue) {
    MOZ_ASSERT(IsElement(), "SetIsInStyleScope on a non-Element node");
    SetBoolFlag(ElementIsInStyleScope, aValue);
  }
  void SetIsElementInStyleScope() {
    MOZ_ASSERT(IsElement(), "SetIsInStyleScope on a non-Element node");
    SetBoolFlag(ElementIsInStyleScope);
  }
  void ClearIsElementInStyleScope() {
    MOZ_ASSERT(IsElement(), "ClearIsInStyleScope on a non-Element node");
    ClearBoolFlag(ElementIsInStyleScope);
  }
  bool IsElementInStyleScope() const { return GetBoolFlag(ElementIsInStyleScope); }

  void SetIsScopedStyleRoot() { SetBoolFlag(ElementIsScopedStyleRoot); }
  void ClearIsScopedStyleRoot() { ClearBoolFlag(ElementIsScopedStyleRoot); }
  bool IsScopedStyleRoot() { return GetBoolFlag(ElementIsScopedStyleRoot); }
protected:
  void SetParentIsContent(bool aValue) { SetBoolFlag(ParentIsContent, aValue); }
  void SetInDocument() { SetBoolFlag(IsInDocument); }
  void SetNodeIsContent() { SetBoolFlag(NodeIsContent); }
  void ClearInDocument() { ClearBoolFlag(IsInDocument); }
  void SetIsElement() { SetBoolFlag(NodeIsElement); }
  void SetHasID() { SetBoolFlag(ElementHasID); }
  void ClearHasID() { ClearBoolFlag(ElementHasID); }
  void SetMayHaveStyle() { SetBoolFlag(ElementMayHaveStyle); }
  void SetHasName() { SetBoolFlag(ElementHasName); }
  void ClearHasName() { ClearBoolFlag(ElementHasName); }
  void SetMayHaveContentEditableAttr()
    { SetBoolFlag(ElementMayHaveContentEditableAttr); }
  bool HasExplicitBaseURI() const { return GetBoolFlag(NodeHasExplicitBaseURI); }
  void SetHasExplicitBaseURI() { SetBoolFlag(NodeHasExplicitBaseURI); }
  void SetHasLockedStyleStates() { SetBoolFlag(ElementHasLockedStyleStates); }
  void ClearHasLockedStyleStates() { ClearBoolFlag(ElementHasLockedStyleStates); }
  bool HasLockedStyleStates() const
    { return GetBoolFlag(ElementHasLockedStyleStates); }

  void SetSubtreeRootPointer(nsINode* aSubtreeRoot)
  {
    NS_ASSERTION(aSubtreeRoot, "aSubtreeRoot can never be null!");
    NS_ASSERTION(!(IsNodeOfType(eCONTENT) && IsInDoc()), "Shouldn't be here!");
    mSubtreeRoot = aSubtreeRoot;
  }

  void ClearSubtreeRootPointer()
  {
    mSubtreeRoot = nullptr;
  }

public:
  
  virtual nsXPCClassInfo* GetClassInfo()
  {
    return nullptr;
  }

  
  void BindObject(nsISupports* aObject);
  
  
  void UnbindObject(nsISupports* aObject);

  



  uint32_t Length() const;

  void GetNodeName(nsAString& aNodeName) const
  {
    aNodeName = NodeName();
  }
  void GetBaseURI(nsAString& aBaseURI) const;
  bool HasChildNodes() const
  {
    return HasChildren();
  }
  uint16_t CompareDocumentPosition(nsINode& aOther) const;
  void GetNodeValue(nsAString& aNodeValue)
  {
    GetNodeValueInternal(aNodeValue);
  }
  void SetNodeValue(const nsAString& aNodeValue,
                    mozilla::ErrorResult& aError)
  {
    SetNodeValueInternal(aNodeValue, aError);
  }
  virtual void GetNodeValueInternal(nsAString& aNodeValue);
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    mozilla::ErrorResult& aError)
  {
    
    
  }
  nsINode* InsertBefore(nsINode& aNode, nsINode* aChild,
                        mozilla::ErrorResult& aError)
  {
    return ReplaceOrInsertBefore(false, &aNode, aChild, aError);
  }
  nsINode* AppendChild(nsINode& aNode, mozilla::ErrorResult& aError)
  {
    return InsertBefore(aNode, nullptr, aError);
  }
  nsINode* ReplaceChild(nsINode& aNode, nsINode& aChild,
                        mozilla::ErrorResult& aError)
  {
    return ReplaceOrInsertBefore(true, &aNode, &aChild, aError);
  }
  nsINode* RemoveChild(nsINode& aChild, mozilla::ErrorResult& aError);
  already_AddRefed<nsINode> CloneNode(bool aDeep, mozilla::ErrorResult& aError);
  bool IsEqualNode(nsINode* aNode);
  void GetNamespaceURI(nsAString& aNamespaceURI) const
  {
    mNodeInfo->GetNamespaceURI(aNamespaceURI);
  }
#ifdef MOZILLA_INTERNAL_API
  void GetPrefix(nsAString& aPrefix)
  {
    mNodeInfo->GetPrefix(aPrefix);
  }
#endif
  void GetLocalName(nsAString& aLocalName)
  {
    aLocalName = mNodeInfo->LocalName();
  }
  
  bool HasAttributes() const;
  nsDOMAttributeMap* GetAttributes();
  JS::Value SetUserData(JSContext* aCx, const nsAString& aKey, JS::Value aData,
                        nsIDOMUserDataHandler* aHandler,
                        mozilla::ErrorResult& aError);
  JS::Value GetUserData(JSContext* aCx, const nsAString& aKey,
                        mozilla::ErrorResult& aError);

  
  
  
  nsresult RemoveFromParent()
  {
    nsINode* parent = GetParentNode();
    mozilla::ErrorResult rv;
    parent->RemoveChild(*this, rv);
    return rv.ErrorCode();
  }

protected:

  
  
  virtual nsINode::nsSlots* CreateSlots();

  bool HasSlots() const
  {
    return mSlots != nullptr;
  }

  nsSlots* GetExistingSlots() const
  {
    return mSlots;
  }

  nsSlots* Slots()
  {
    if (!HasSlots()) {
      mSlots = CreateSlots();
      MOZ_ASSERT(mSlots);
    }
    return GetExistingSlots();
  }

  nsTObserverArray<nsIMutationObserver*> *GetMutationObservers()
  {
    return HasSlots() ? &GetExistingSlots()->mMutationObservers : nullptr;
  }

  bool IsEditableInternal() const;
  virtual bool IsEditableExternal() const
  {
    return IsEditableInternal();
  }

  virtual void GetTextContentInternal(nsAString& aTextContent);
  virtual void SetTextContentInternal(const nsAString& aTextContent,
                                      mozilla::ErrorResult& aError)
  {
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
  nsresult CompareDocumentPosition(nsIDOMNode* aOther,
                                   uint16_t* aReturn);
  nsresult GetAttributes(nsIDOMMozNamedAttrMap** aAttributes);

  nsresult ReplaceOrInsertBefore(bool aReplace, nsIDOMNode *aNewChild,
                                 nsIDOMNode *aRefChild, nsIDOMNode **aReturn);
  nsINode* ReplaceOrInsertBefore(bool aReplace, nsINode* aNewChild,
                                 nsINode* aRefChild,
                                 mozilla::ErrorResult& aError);
  nsresult RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn);

  





  virtual mozilla::dom::Element* GetNameSpaceElement() = 0;

  










  void doRemoveChildAt(uint32_t aIndex, bool aNotify, nsIContent* aKid,
                       nsAttrAndChildArray& aChildArray);

  









  nsresult doInsertChildAt(nsIContent* aKid, uint32_t aIndex,
                           bool aNotify, nsAttrAndChildArray& aChildArray);

public:
  







#define EVENT(name_, id_, type_, struct_)                             \
  mozilla::dom::EventHandlerNonNull* GetOn##name_();                  \
  void SetOn##name_(mozilla::dom::EventHandlerNonNull* listener,      \
                    mozilla::ErrorResult& error);                     \
  NS_IMETHOD GetOn##name_(JSContext *cx, JS::Value *vp);              \
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

  uint32_t mFlags;

private:
  
  uint32_t mBoolFlags;

protected:
  nsIContent* mNextSibling;
  nsIContent* mPreviousSibling;
  nsIContent* mFirstChild;

  union {
    
    nsIFrame* mPrimaryFrame;

    
    nsINode* mSubtreeRoot;
  };

  
  nsSlots* mSlots;
};






template<class C, class D>
inline nsINode* NODE_FROM(C& aContent, D& aDocument)
{
  if (aContent)
    return static_cast<nsINode*>(aContent);
  return static_cast<nsINode*>(aDocument);
}




class nsNodeSelectorTearoff MOZ_FINAL : public nsIDOMNodeSelector
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIDOMNODESELECTOR

  NS_DECL_CYCLE_COLLECTION_CLASS(nsNodeSelectorTearoff)

  nsNodeSelectorTearoff(nsINode *aNode) : mNode(aNode)
  {
  }

private:
  ~nsNodeSelectorTearoff() {}

private:
  nsCOMPtr<nsINode> mNode;
};

extern const nsIID kThisPtrOffsetsSID;


#define NS_OFFSET_AND_INTERFACE_TABLE_BEGIN_AMBIGUOUS(_class, _implClass)     \
  static const QITableEntry offsetAndQITable[] = {                            \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsISupports, _implClass)

#define NS_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                           \
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN_AMBIGUOUS(_class, _class)

#define NS_OFFSET_AND_INTERFACE_TABLE_END                                     \
  { nullptr, 0 } };                                                            \
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

#define NS_NODE_INTERFACE_TABLE9(_class, _i1, _i2, _i3, _i4, _i5, _i6, _i7,   \
                                 _i8, _i9)                                    \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i8)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i9)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END                                           \
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE


NS_DEFINE_STATIC_IID_ACCESSOR(nsINode, NS_INODE_IID)

#define NS_FORWARD_NSIDOMNODE_TO_NSINODE_HELPER(...) \
  NS_IMETHOD GetNodeName(nsAString& aNodeName) __VA_ARGS__ \
  { \
    nsINode::GetNodeName(aNodeName); \
    return NS_OK; \
  } \
  NS_IMETHOD GetNodeValue(nsAString& aNodeValue) __VA_ARGS__ \
  { \
    nsINode::GetNodeValue(aNodeValue); \
    return NS_OK; \
  } \
  NS_IMETHOD SetNodeValue(const nsAString& aNodeValue) __VA_ARGS__ \
  { \
    mozilla::ErrorResult rv; \
    nsINode::SetNodeValue(aNodeValue, rv); \
    return rv.ErrorCode(); \
  } \
  NS_IMETHOD GetNodeType(uint16_t* aNodeType) __VA_ARGS__ \
  { \
    *aNodeType = nsINode::NodeType(); \
    return NS_OK; \
  } \
  NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode) __VA_ARGS__ \
  { \
    return nsINode::GetParentNode(aParentNode); \
  } \
  NS_IMETHOD GetParentElement(nsIDOMElement** aParentElement) __VA_ARGS__ \
  { \
    return nsINode::GetParentElement(aParentElement); \
  } \
  NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes) __VA_ARGS__ \
  { \
    return nsINode::GetChildNodes(aChildNodes); \
  } \
  NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild) __VA_ARGS__ \
  { \
    return nsINode::GetFirstChild(aFirstChild); \
  } \
  NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild) __VA_ARGS__ \
  { \
    return nsINode::GetLastChild(aLastChild); \
  } \
  NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling) __VA_ARGS__ \
  { \
    return nsINode::GetPreviousSibling(aPreviousSibling); \
  } \
  NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling) __VA_ARGS__ \
  { \
    return nsINode::GetNextSibling(aNextSibling); \
  } \
  NS_IMETHOD GetAttributes(nsIDOMMozNamedAttrMap** aAttributes) __VA_ARGS__ \
  { \
    return nsINode::GetAttributes(aAttributes); \
  } \
  NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument) __VA_ARGS__ \
  { \
    return nsINode::GetOwnerDocument(aOwnerDocument); \
  } \
  NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aResult) __VA_ARGS__ \
  { \
    return ReplaceOrInsertBefore(false, aNewChild, aRefChild, aResult); \
  } \
  NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aResult) __VA_ARGS__ \
  { \
    return ReplaceOrInsertBefore(true, aNewChild, aOldChild, aResult); \
  } \
  NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aResult) __VA_ARGS__ \
  { \
    return nsINode::RemoveChild(aOldChild, aResult); \
  } \
  NS_IMETHOD AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aResult) __VA_ARGS__ \
  { \
    return InsertBefore(aNewChild, nullptr, aResult); \
  } \
  NS_IMETHOD HasChildNodes(bool* aResult) __VA_ARGS__ \
  { \
    *aResult = nsINode::HasChildNodes(); \
    return NS_OK; \
  } \
  NS_IMETHOD CloneNode(bool aDeep, uint8_t aArgc, nsIDOMNode** aResult) __VA_ARGS__ \
  { \
    if (aArgc == 0) { \
      aDeep = true; \
    } \
    mozilla::ErrorResult rv; \
    nsCOMPtr<nsINode> clone = nsINode::CloneNode(aDeep, rv); \
    if (rv.Failed()) { \
      return rv.ErrorCode(); \
    } \
    *aResult = clone.forget().get()->AsDOMNode(); \
    return NS_OK; \
  } \
  NS_IMETHOD Normalize() __VA_ARGS__ \
  { \
    nsINode::Normalize(); \
    return NS_OK; \
  } \
  NS_IMETHOD GetNamespaceURI(nsAString& aNamespaceURI) __VA_ARGS__ \
  { \
    nsINode::GetNamespaceURI(aNamespaceURI); \
    return NS_OK; \
  } \
  NS_IMETHOD GetPrefix(nsAString& aPrefix) __VA_ARGS__ \
  { \
    nsINode::GetPrefix(aPrefix); \
    return NS_OK; \
  } \
  NS_IMETHOD GetLocalName(nsAString& aLocalName) __VA_ARGS__ \
  { \
    nsINode::GetLocalName(aLocalName); \
    return NS_OK; \
  } \
  using nsINode::HasAttributes; \
  NS_IMETHOD HasAttributes(bool* aResult) __VA_ARGS__ \
  { \
    *aResult = nsINode::HasAttributes(); \
    return NS_OK; \
  } \
  NS_IMETHOD GetDOMBaseURI(nsAString& aBaseURI) __VA_ARGS__ \
  { \
    nsINode::GetBaseURI(aBaseURI); \
    return NS_OK; \
  } \
  NS_IMETHOD CompareDocumentPosition(nsIDOMNode* aOther, uint16_t* aResult) __VA_ARGS__ \
  { \
    return nsINode::CompareDocumentPosition(aOther, aResult); \
  } \
  NS_IMETHOD GetTextContent(nsAString& aTextContent) __VA_ARGS__ \
  { \
    nsINode::GetTextContent(aTextContent); \
    return NS_OK; \
  } \
  NS_IMETHOD SetTextContent(const nsAString& aTextContent) __VA_ARGS__ \
  { \
    mozilla::ErrorResult rv; \
    nsINode::SetTextContent(aTextContent, rv); \
    return rv.ErrorCode(); \
  } \
  NS_IMETHOD LookupPrefix(const nsAString& aNamespaceURI, nsAString& aResult) __VA_ARGS__ \
  { \
    nsINode::LookupPrefix(aNamespaceURI, aResult); \
    return NS_OK; \
  } \
  NS_IMETHOD IsDefaultNamespace(const nsAString& aNamespaceURI, bool* aResult) __VA_ARGS__ \
  { \
    *aResult = nsINode::IsDefaultNamespace(aNamespaceURI); \
    return NS_OK; \
  } \
  NS_IMETHOD LookupNamespaceURI(const nsAString& aPrefix, nsAString& aResult) __VA_ARGS__ \
  { \
    nsINode::LookupNamespaceURI(aPrefix, aResult); \
    return NS_OK; \
  } \
  NS_IMETHOD IsEqualNode(nsIDOMNode* aArg, bool* aResult) __VA_ARGS__ \
  { \
    return nsINode::IsEqualNode(aArg, aResult); \
  } \
  NS_IMETHOD SetUserData(const nsAString& aKey, nsIVariant* aData, nsIDOMUserDataHandler* aHandler, nsIVariant** aResult) __VA_ARGS__ \
  { \
    return nsINode::SetUserData(aKey, aData, aHandler, aResult); \
  } \
  NS_IMETHOD GetUserData(const nsAString& aKey, nsIVariant** aResult) __VA_ARGS__ \
  { \
    return nsINode::GetUserData(aKey, aResult); \
  } \
  NS_IMETHOD Contains(nsIDOMNode* aOther, bool* aResult) __VA_ARGS__ \
  { \
    return nsINode::Contains(aOther, aResult); \
  }

#define NS_FORWARD_NSIDOMNODE_TO_NSINODE \
  NS_FORWARD_NSIDOMNODE_TO_NSINODE_HELPER(MOZ_FINAL)

#define NS_FORWARD_NSIDOMNODE_TO_NSINODE_OVERRIDABLE \
  NS_FORWARD_NSIDOMNODE_TO_NSINODE_HELPER()

#endif 
