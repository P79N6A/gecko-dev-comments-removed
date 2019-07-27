




#ifndef nsINode_h___
#define nsINode_h___

#include "mozilla/Likely.h"
#include "nsCOMPtr.h"               
#include "nsGkAtoms.h"              
#include "nsIDOMNode.h"
#include "mozilla/dom/NodeInfo.h"            
#include "nsINodeList.h"            
#include "nsIVariant.h"             
#include "nsNodeInfoManager.h"      
#include "nsPropertyTable.h"        
#include "nsTObserverArray.h"       
#include "mozilla/ErrorResult.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/EventTarget.h" 
#include "js/TypeDecls.h"     
#include "mozilla/dom/DOMString.h"
#include "mozilla/dom/BindingDeclarations.h"


#ifdef XP_WIN
#ifdef GetClassInfo
#undef GetClassInfo
#endif
#endif

class nsAttrAndChildArray;
class nsChildContentList;
struct nsCSSSelectorList;
class nsDOMAttributeMap;
class nsIAnimationObserver;
class nsIContent;
class nsIDocument;
class nsIDOMElement;
class nsIDOMNodeList;
class nsIEditor;
class nsIFrame;
class nsIMutationObserver;
class nsINode;
class nsIPresShell;
class nsIPrincipal;
class nsIURI;
class nsNodeSupportsWeakRefTearoff;
class nsNodeWeakReference;
class nsXPCClassInfo;
class nsDOMMutationObserver;

namespace mozilla {
class EventListenerManager;
namespace dom {




inline bool IsSpaceCharacter(char16_t aChar) {
  return aChar == ' ' || aChar == '\t' || aChar == '\n' || aChar == '\r' ||
         aChar == '\f';
}
inline bool IsSpaceCharacter(char aChar) {
  return aChar == ' ' || aChar == '\t' || aChar == '\n' || aChar == '\r' ||
         aChar == '\f';
}
struct BoxQuadOptions;
struct ConvertCoordinateOptions;
class DOMPoint;
class DOMQuad;
class DOMRectReadOnly;
class Element;
class EventHandlerNonNull;
class OnErrorEventHandlerNonNull;
template<typename T> class Optional;
class Text;
class TextOrElementOrDocument;
struct DOMPointInit;
} 
} 

#define NODE_FLAG_BIT(n_) \
  (nsWrapperCache::FlagsType(1U) << (WRAPPER_CACHE_FLAGS_BITS_USED + (n_)))

enum {
  
  NODE_HAS_LISTENERMANAGER =              NODE_FLAG_BIT(0),

  
  NODE_HAS_PROPERTIES =                   NODE_FLAG_BIT(1),

  
  
  
  
  
  NODE_IS_ANONYMOUS_ROOT =                NODE_FLAG_BIT(2),

  
  
  
  
  
  
  NODE_IS_IN_NATIVE_ANONYMOUS_SUBTREE =          NODE_FLAG_BIT(3),

  
  
  
  
  NODE_IS_NATIVE_ANONYMOUS_ROOT =         NODE_FLAG_BIT(4),

  
  
  NODE_FORCE_XBL_BINDINGS =               NODE_FLAG_BIT(5),

  
  NODE_MAY_BE_IN_BINDING_MNGR =           NODE_FLAG_BIT(6),

  NODE_IS_EDITABLE =                      NODE_FLAG_BIT(7),

  
  
  NODE_MAY_HAVE_CLASS =                   NODE_FLAG_BIT(8),

  
  NODE_IS_IN_SHADOW_TREE =                NODE_FLAG_BIT(9),

  
  NODE_HAS_EMPTY_SELECTOR =               NODE_FLAG_BIT(10),

  
  
  NODE_HAS_SLOW_SELECTOR =                NODE_FLAG_BIT(11),

  
  
  NODE_HAS_EDGE_CHILD_SELECTOR =          NODE_FLAG_BIT(12),

  
  
  
  
  
  
  
  NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS = NODE_FLAG_BIT(13),

  NODE_ALL_SELECTOR_FLAGS =               NODE_HAS_EMPTY_SELECTOR |
                                          NODE_HAS_SLOW_SELECTOR |
                                          NODE_HAS_EDGE_CHILD_SELECTOR |
                                          NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS,

  
  
  NODE_NEEDS_FRAME =                      NODE_FLAG_BIT(14),

  
  
  
  NODE_DESCENDANTS_NEED_FRAMES =          NODE_FLAG_BIT(15),

  
  NODE_HAS_ACCESSKEY =                    NODE_FLAG_BIT(16),

  
  NODE_HAS_DIRECTION_RTL =                NODE_FLAG_BIT(17),

  
  NODE_HAS_DIRECTION_LTR =                NODE_FLAG_BIT(18),

  NODE_ALL_DIRECTION_FLAGS =              NODE_HAS_DIRECTION_LTR |
                                          NODE_HAS_DIRECTION_RTL,

  NODE_CHROME_ONLY_ACCESS =               NODE_FLAG_BIT(19),

  NODE_IS_ROOT_OF_CHROME_ONLY_ACCESS =    NODE_FLAG_BIT(20),

  
  NODE_TYPE_SPECIFIC_BITS_OFFSET =        21
};


#define ASSERT_NODE_FLAGS_SPACE(n) \
  static_assert(WRAPPER_CACHE_FLAGS_BITS_USED + (n) <=                          \
                  sizeof(nsWrapperCache::FlagsType) * 8,                        \
                "Not enough space for our bits")
ASSERT_NODE_FLAGS_SPACE(NODE_TYPE_SPECIFIC_BITS_OFFSET);







class nsMutationGuard {
public:
  nsMutationGuard()
  {
    mStartingGeneration = sGeneration;
  }

  











  bool Mutated(uint8_t aIgnoreCount)
  {
    return (sGeneration - mStartingGeneration) > aIgnoreCount;
  }

  
  
  
  static void DidMutate()
  {
    sGeneration++;
  }

private:
  
  uint64_t mStartingGeneration;

  
  static uint64_t sGeneration;
};







class nsChildContentList final : public nsINodeList
{
public:
  explicit nsChildContentList(nsINode* aNode)
    : mNode(aNode)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(nsChildContentList)

  
  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

  
  NS_DECL_NSIDOMNODELIST

  
  virtual int32_t IndexOf(nsIContent* aContent) override;
  virtual nsIContent* Item(uint32_t aIndex) override;

  void DropReference()
  {
    mNode = nullptr;
  }

  virtual nsINode* GetParentObject() override
  {
    return mNode;
  }

private:
  ~nsChildContentList() {}

  
  
  
  nsINode* MOZ_NON_OWNING_REF mNode;
};






#define NS_DECL_SIZEOF_EXCLUDING_THIS \
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const override;



#define DOM_USER_DATA         1
#define SMIL_MAPPED_ATTR_ANIMVAL 2


#define NS_INODE_IID \
{ 0xe8fdd227, 0x27da, 0x46ee, \
  { 0xbe, 0xf3, 0x1a, 0xef, 0x5a, 0x8f, 0xc5, 0xb4 } }






class nsINode : public mozilla::dom::EventTarget
{
public:
  typedef mozilla::dom::BoxQuadOptions BoxQuadOptions;
  typedef mozilla::dom::ConvertCoordinateOptions ConvertCoordinateOptions;
  typedef mozilla::dom::DOMPoint DOMPoint;
  typedef mozilla::dom::DOMPointInit DOMPointInit;
  typedef mozilla::dom::DOMQuad DOMQuad;
  typedef mozilla::dom::DOMRectReadOnly DOMRectReadOnly;
  typedef mozilla::dom::TextOrElementOrDocument TextOrElementOrDocument;
  typedef mozilla::ErrorResult ErrorResult;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODE_IID)

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  
  
  
  
  
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  friend class nsNodeUtils;
  friend class nsNodeWeakReference;
  friend class nsNodeSupportsWeakRefTearoff;
  friend class nsAttrAndChildArray;

#ifdef MOZILLA_INTERNAL_API
  explicit nsINode(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : mNodeInfo(aNodeInfo),
    mParent(nullptr),
    mBoolFlags(0),
    mNextSibling(nullptr),
    mPreviousSibling(nullptr),
    mFirstChild(nullptr),
    mSubtreeRoot(this),
    mSlots(nullptr)
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
    
    eDOCUMENT_FRAGMENT   = 1 << 7,
    

    eDATA_NODE           = 1 << 8,
    
    eMEDIA               = 1 << 9,
    
    eANIMATION           = 1 << 10,
    
    eFILTER              = 1 << 11
  };

  







  virtual bool IsNodeOfType(uint32_t aFlags) const = 0;

  virtual JSObject* WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

  



  static bool HasBoxQuadsSupport(JSContext* aCx, JSObject* );

protected:
  








  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) = 0;

public:
  mozilla::dom::ParentObject GetParentObject() const; 

  




  virtual nsINode* GetScopeChainParent() const;

  


  bool IsElement() const {
    return GetBoolFlag(NodeIsElement);
  }

  



  mozilla::dom::Element* AsElement();
  const mozilla::dom::Element* AsElement() const;

  



  nsIContent* AsContent();
  const nsIContent* AsContent() const
  {
    return const_cast<nsINode*>(this)->AsContent();
  }

  



  mozilla::dom::Text* GetAsText();
  const mozilla::dom::Text* GetAsText() const;

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

  




  bool IsInUncomposedDoc() const
  {
    return GetBoolFlag(IsInDocument);
  }

  


  bool IsInDoc() const
  {
    return IsInUncomposedDoc();
  }

  






  nsIDocument* GetUncomposedDoc() const
  {
    return IsInUncomposedDoc() ? OwnerDoc() : nullptr;
  }

  


  nsIDocument *GetCurrentDoc() const
  {
    return GetUncomposedDoc();
  }

  




  nsIDocument* GetComposedDoc() const
  {
    return IsInShadowTree() ?
      GetComposedDocInternal() : GetUncomposedDoc();
  }

  


  nsIDocument* GetCrossShadowCurrentDoc() const
  {
    return GetComposedDoc();
  }

  


  bool IsInComposedDoc() const
  {
    return IsInUncomposedDoc() || (IsInShadowTree() && GetComposedDocInternal());
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

  



  inline mozilla::dom::NodeInfo* NodeInfo() const
  {
    return mNodeInfo;
  }

  inline bool IsInNamespace(int32_t aNamespace) const
  {
    return mNodeInfo->NamespaceID() == aNamespace;
  }

protected:
  
  
  inline bool IsNodeInternal() const
  {
    return false;
  }

  template<typename First, typename... Args>
  inline bool IsNodeInternal(First aFirst, Args... aArgs) const
  {
    return mNodeInfo->Equals(aFirst) || IsNodeInternal(aArgs...);
  }

public:
  inline bool IsHTMLElement() const
  {
    return IsElement() && IsInNamespace(kNameSpaceID_XHTML);
  }

  inline bool IsHTMLElement(nsIAtom* aTag) const
  {
    return IsElement() && mNodeInfo->Equals(aTag, kNameSpaceID_XHTML);
  }

  template<typename First, typename... Args>
  inline bool IsAnyOfHTMLElements(First aFirst, Args... aArgs) const
  {
    return IsHTMLElement() && IsNodeInternal(aFirst, aArgs...);
  }

  inline bool IsSVGElement() const
  {
    return IsElement() && IsInNamespace(kNameSpaceID_SVG);
  }

  inline bool IsSVGElement(nsIAtom* aTag) const
  {
    return IsElement() && mNodeInfo->Equals(aTag, kNameSpaceID_SVG);
  }

  template<typename First, typename... Args>
  inline bool IsAnyOfSVGElements(First aFirst, Args... aArgs) const
  {
    return IsSVGElement() && IsNodeInternal(aFirst, aArgs...);
  }

  inline bool IsXULElement() const
  {
    return IsElement() && IsInNamespace(kNameSpaceID_XUL);
  }

  inline bool IsXULElement(nsIAtom* aTag) const
  {
    return IsElement() && mNodeInfo->Equals(aTag, kNameSpaceID_XUL);
  }

  template<typename First, typename... Args>
  inline bool IsAnyOfXULElements(First aFirst, Args... aArgs) const
  {
    return IsXULElement() && IsNodeInternal(aFirst, aArgs...);
  }

  inline bool IsMathMLElement() const
  {
    return IsElement() && IsInNamespace(kNameSpaceID_MathML);
  }

  inline bool IsMathMLElement(nsIAtom* aTag) const
  {
    return IsElement() && mNodeInfo->Equals(aTag, kNameSpaceID_MathML);
  }

  template<typename First, typename... Args>
  inline bool IsAnyOfMathMLElements(First aFirst, Args... aArgs) const
  {
    return IsMathMLElement() && IsNodeInternal(aFirst, aArgs...);
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

  


  template<class T>
  static void DeleteProperty(void *, nsIAtom *, void *aPropertyValue, void *)
  {
    delete static_cast<T *>(aPropertyValue);
  }

  





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

  



  mozilla::dom::Element* GetParentElementCrossingShadowRoot() const;

  




  nsINode* SubtreeRoot() const;

  


  NS_DECL_NSIDOMEVENTTARGET

  virtual mozilla::EventListenerManager*
    GetExistingListenerManager() const override;
  virtual mozilla::EventListenerManager*
    GetOrCreateListenerManager() override;

  using mozilla::dom::EventTarget::RemoveEventListener;
  using nsIDOMEventTarget::AddEventListener;
  virtual void AddEventListener(const nsAString& aType,
                                mozilla::dom::EventListener* aListener,
                                bool aUseCapture,
                                const mozilla::dom::Nullable<bool>& aWantsUntrusted,
                                mozilla::ErrorResult& aRv) override;
  using nsIDOMEventTarget::AddSystemEventListener;
  virtual nsIDOMWindow* GetOwnerGlobal() override;

  












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

  





  void AddAnimationObserver(nsIAnimationObserver* aAnimationObserver);

  



  void AddAnimationObserverUnlessExists(nsIAnimationObserver* aAnimationObserver);

  


  void RemoveMutationObserver(nsIMutationObserver* aMutationObserver)
  {
    nsSlots* s = GetExistingSlots();
    if (s) {
      s->mMutationObservers.RemoveElement(aMutationObserver);
    }
  }

  








  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const = 0;

  
  
  class nsSlots
  {
  public:
    nsSlots()
      : mWeakReference(nullptr)
    {
    }

    
    
    virtual ~nsSlots();

    void Traverse(nsCycleCollectionTraversalCallback &cb);
    void Unlink();

    


    nsTObserverArray<nsIMutationObserver*> mMutationObservers;

    




    nsRefPtr<nsChildContentList> mChildNodes;

    



    nsNodeWeakReference* MOZ_NON_OWNING_REF mWeakReference;
  };

  


#ifdef DEBUG
  nsSlots* DebugGetSlots()
  {
    return Slots();
  }
#endif

  void SetFlags(FlagsType aFlagsToSet)
  {
    NS_ASSERTION(!(aFlagsToSet & (NODE_IS_ANONYMOUS_ROOT |
                                  NODE_IS_NATIVE_ANONYMOUS_ROOT |
                                  NODE_IS_IN_NATIVE_ANONYMOUS_SUBTREE |
                                  NODE_DESCENDANTS_NEED_FRAMES |
                                  NODE_NEEDS_FRAME |
                                  NODE_CHROME_ONLY_ACCESS)) ||
                 IsNodeOfType(eCONTENT),
                 "Flag only permitted on nsIContent nodes");
    nsWrapperCache::SetFlags(aFlagsToSet);
  }

  void UnsetFlags(FlagsType aFlagsToUnset)
  {
    NS_ASSERTION(!(aFlagsToUnset &
                   (NODE_IS_ANONYMOUS_ROOT |
                    NODE_IS_IN_NATIVE_ANONYMOUS_SUBTREE |
                    NODE_IS_NATIVE_ANONYMOUS_ROOT)),
                 "Trying to unset write-only flags");
    nsWrapperCache::UnsetFlags(aFlagsToUnset);
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
#ifdef MOZILLA_INTERNAL_API
    return IsEditableInternal();
#else
    return IsEditableExternal();
#endif
  }

  


  bool IsInNativeAnonymousSubtree() const
  {
#ifdef DEBUG
    if (HasFlag(NODE_IS_IN_NATIVE_ANONYMOUS_SUBTREE)) {
      return true;
    }
    CheckNotNativeAnonymous();
    return false;
#else
    return HasFlag(NODE_IS_IN_NATIVE_ANONYMOUS_SUBTREE);
#endif
  }

  bool IsInAnonymousSubtree() const;

  
  bool IsAnonymousContentInSVGUseSubtree() const;

  
  
  bool ChromeOnlyAccess() const
  {
    return HasFlag(NODE_IS_IN_NATIVE_ANONYMOUS_SUBTREE | NODE_CHROME_ONLY_ACCESS);
  }

  bool IsInShadowTree() const
  {
    return HasFlag(NODE_IS_IN_SHADOW_TREE);
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

  







  virtual already_AddRefed<nsIURI> GetBaseURI(bool aTryUseXHRDocBaseURI = false) const = 0;
  already_AddRefed<nsIURI> GetBaseURIObject() const;

  


  nsresult SetExplicitBaseURI(nsIURI* aURI);
  


protected:
  nsIURI* GetExplicitBaseURI() const {
    if (HasExplicitBaseURI()) {
      return static_cast<nsIURI*>(GetProperty(nsGkAtoms::baseURIProperty));
    }
    return nullptr;
  }
  
public:
  void GetTextContent(nsAString& aTextContent,
                      mozilla::ErrorResult& aError)
  {
    GetTextContentInternal(aTextContent, aError);
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

  nsresult QuerySelector(const nsAString& aSelector, nsIDOMElement **aReturn);
  nsresult QuerySelectorAll(const nsAString& aSelector, nsIDOMNodeList **aReturn);

protected:
  
  
  mozilla::dom::Element* GetElementById(const nsAString& aId);

public:
  











  nsresult SetUserData(const nsAString& aKey, nsIVariant* aData,
                       nsIVariant** aResult);

  







  nsIVariant* GetUserData(const nsAString& aKey);

  nsresult GetUserData(const nsAString& aKey, nsIVariant** aResult)
  {
    NS_IF_ADDREF(*aResult = GetUserData(aKey));
  
    return NS_OK;
  }

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

  nsIDocument* GetComposedDocInternal() const;

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
    
    NodeHandlingClick,
    
    NodeHasRelevantHoverRules,
    
    
    ElementHasWeirdParserInsertionMode,
    
    ParserHasNotified,
    
    BooleanFlagCount
  };

  void SetBoolFlag(BooleanFlag name, bool value) {
    static_assert(BooleanFlagCount <= 8*sizeof(mBoolFlags),
                  "Too many boolean flags");
    mBoolFlags = (mBoolFlags & ~(1 << name)) | (value << name);
  }

  void SetBoolFlag(BooleanFlag name) {
    static_assert(BooleanFlagCount <= 8*sizeof(mBoolFlags),
                  "Too many boolean flags");
    mBoolFlags |= (1 << name);
  }

  void ClearBoolFlag(BooleanFlag name) {
    static_assert(BooleanFlagCount <= 8*sizeof(mBoolFlags),
                  "Too many boolean flags");
    mBoolFlags &= ~(1 << name);
  }

  bool GetBoolFlag(BooleanFlag name) const {
    static_assert(BooleanFlagCount <= 8*sizeof(mBoolFlags),
                  "Too many boolean flags");
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
  bool HasRelevantHoverRules() const { return GetBoolFlag(NodeHasRelevantHoverRules); }
  void SetHasRelevantHoverRules() { SetBoolFlag(NodeHasRelevantHoverRules); }
  void SetParserHasNotified() { SetBoolFlag(ParserHasNotified); };
  bool HasParserNotified() { return GetBoolFlag(ParserHasNotified); }
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
  void SetHasWeirdParserInsertionMode() { SetBoolFlag(ElementHasWeirdParserInsertionMode); }
  bool HasWeirdParserInsertionMode() const
  { return GetBoolFlag(ElementHasWeirdParserInsertionMode); }
  bool HandlingClick() const { return GetBoolFlag(NodeHandlingClick); }
  void SetHandlingClick() { SetBoolFlag(NodeHandlingClick); }
  void ClearHandlingClick() { ClearBoolFlag(NodeHandlingClick); }

  void SetSubtreeRootPointer(nsINode* aSubtreeRoot)
  {
    NS_ASSERTION(aSubtreeRoot, "aSubtreeRoot can never be null!");
    NS_ASSERTION(!(IsNodeOfType(eCONTENT) && IsInDoc()) &&
                 !IsInShadowTree(), "Shouldn't be here!");
    mSubtreeRoot = aSubtreeRoot;
  }

  void ClearSubtreeRootPointer()
  {
    mSubtreeRoot = nullptr;
  }

public:
  
  void BindObject(nsISupports* aObject);
  
  
  void UnbindObject(nsISupports* aObject);

  void GetBoundMutationObservers(nsTArray<nsRefPtr<nsDOMMutationObserver> >& aResult);

  



  uint32_t Length() const;

  void GetNodeName(mozilla::dom::DOMString& aNodeName)
  {
    const nsString& nodeName = NodeName();
    aNodeName.SetStringBuffer(nsStringBuffer::FromString(nodeName),
                              nodeName.Length());
  }
  void GetBaseURI(nsAString& aBaseURI) const;
  
  
  
  
  void GetBaseURIFromJS(nsAString& aBaseURI) const;
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
  void GetLocalName(mozilla::dom::DOMString& aLocalName)
  {
    const nsString& localName = LocalName();
    if (localName.IsVoid()) {
      aLocalName.SetNull();
    } else {
      aLocalName.SetStringBuffer(nsStringBuffer::FromString(localName),
                                 localName.Length());
    }
  }

  nsDOMAttributeMap* GetAttributes();
  void SetUserData(JSContext* aCx, const nsAString& aKey,
                   JS::Handle<JS::Value> aData,
                   JS::MutableHandle<JS::Value> aRetval,
                   mozilla::ErrorResult& aError);
  void GetUserData(JSContext* aCx, const nsAString& aKey,
                   JS::MutableHandle<JS::Value> aRetval,
                   mozilla::ErrorResult& aError);

  
  
  
  nsresult RemoveFromParent()
  {
    nsINode* parent = GetParentNode();
    mozilla::ErrorResult rv;
    parent->RemoveChild(*this, rv);
    return rv.ErrorCode();
  }

  
  mozilla::dom::Element* GetPreviousElementSibling() const;
  mozilla::dom::Element* GetNextElementSibling() const;
  


  void Remove();

  
  mozilla::dom::Element* GetFirstElementChild() const;
  mozilla::dom::Element* GetLastElementChild() const;

  void GetBoxQuads(const BoxQuadOptions& aOptions,
                   nsTArray<nsRefPtr<DOMQuad> >& aResult,
                   mozilla::ErrorResult& aRv);

  already_AddRefed<DOMQuad> ConvertQuadFromNode(DOMQuad& aQuad,
                                                const TextOrElementOrDocument& aFrom,
                                                const ConvertCoordinateOptions& aOptions,
                                                ErrorResult& aRv);
  already_AddRefed<DOMQuad> ConvertRectFromNode(DOMRectReadOnly& aRect,
                                                const TextOrElementOrDocument& aFrom,
                                                const ConvertCoordinateOptions& aOptions,
                                                ErrorResult& aRv);
  already_AddRefed<DOMPoint> ConvertPointFromNode(const DOMPointInit& aPoint,
                                                  const TextOrElementOrDocument& aFrom,
                                                  const ConvertCoordinateOptions& aOptions,
                                                  ErrorResult& aRv);

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

  virtual void GetTextContentInternal(nsAString& aTextContent,
                                      mozilla::ErrorResult& aError);
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

  







  nsCSSSelectorList* ParseSelectorList(const nsAString& aSelectorString,
                                       mozilla::ErrorResult& aRv);

public:
  







#define EVENT(name_, id_, type_, struct_)                             \
  mozilla::dom::EventHandlerNonNull* GetOn##name_();                  \
  void SetOn##name_(mozilla::dom::EventHandlerNonNull* listener);
#define TOUCH_EVENT EVENT
#define DOCUMENT_ONLY_EVENT EVENT
#include "mozilla/EventNameList.h"
#undef DOCUMENT_ONLY_EVENT
#undef TOUCH_EVENT
#undef EVENT

protected:
  static bool Traverse(nsINode *tmp, nsCycleCollectionTraversalCallback &cb);
  static void Unlink(nsINode *tmp);

  nsRefPtr<mozilla::dom::NodeInfo> mNodeInfo;

  nsINode* mParent;

private:
  
  uint32_t mBoolFlags;

protected:
  
  
  nsIContent* MOZ_NON_OWNING_REF mNextSibling;
  nsIContent* MOZ_NON_OWNING_REF mPreviousSibling;
  
  
  
  nsIContent* MOZ_NON_OWNING_REF mFirstChild;

  union {
    
    nsIFrame* mPrimaryFrame;

    
    
    
    nsINode* MOZ_NON_OWNING_REF mSubtreeRoot;
  };

  
  nsSlots* mSlots;
};

inline nsIDOMNode* GetAsDOMNode(nsINode* aNode)
{
  return aNode ? aNode->AsDOMNode() : nullptr;
}






template<class C, class D>
inline nsINode* NODE_FROM(C& aContent, D& aDocument)
{
  if (aContent)
    return static_cast<nsINode*>(aContent);
  return static_cast<nsINode*>(aDocument);
}

NS_DEFINE_STATIC_IID_ACCESSOR(nsINode, NS_INODE_IID)

inline nsISupports*
ToSupports(nsINode* aPointer)
{
  return aPointer;
}

inline nsISupports*
ToCanonicalSupports(nsINode* aPointer)
{
  return aPointer;
}

#define NS_FORWARD_NSIDOMNODE_TO_NSINODE_HELPER(...) \
  NS_IMETHOD GetNodeName(nsAString& aNodeName) __VA_ARGS__ override \
  { \
    aNodeName = nsINode::NodeName(); \
    return NS_OK; \
  } \
  NS_IMETHOD GetNodeValue(nsAString& aNodeValue) __VA_ARGS__ override \
  { \
    nsINode::GetNodeValue(aNodeValue); \
    return NS_OK; \
  } \
  NS_IMETHOD SetNodeValue(const nsAString& aNodeValue) __VA_ARGS__ override \
  { \
    mozilla::ErrorResult rv; \
    nsINode::SetNodeValue(aNodeValue, rv); \
    return rv.ErrorCode(); \
  } \
  NS_IMETHOD GetNodeType(uint16_t* aNodeType) __VA_ARGS__ override \
  { \
    *aNodeType = nsINode::NodeType(); \
    return NS_OK; \
  } \
  NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode) __VA_ARGS__ override \
  { \
    return nsINode::GetParentNode(aParentNode); \
  } \
  NS_IMETHOD GetParentElement(nsIDOMElement** aParentElement) __VA_ARGS__ override \
  { \
    return nsINode::GetParentElement(aParentElement); \
  } \
  NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes) __VA_ARGS__ override \
  { \
    return nsINode::GetChildNodes(aChildNodes); \
  } \
  NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild) __VA_ARGS__ override \
  { \
    return nsINode::GetFirstChild(aFirstChild); \
  } \
  NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild) __VA_ARGS__ override \
  { \
    return nsINode::GetLastChild(aLastChild); \
  } \
  NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling) __VA_ARGS__ override \
  { \
    return nsINode::GetPreviousSibling(aPreviousSibling); \
  } \
  NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling) __VA_ARGS__ override \
  { \
    return nsINode::GetNextSibling(aNextSibling); \
  } \
  NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument) __VA_ARGS__ override \
  { \
    return nsINode::GetOwnerDocument(aOwnerDocument); \
  } \
  NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aResult) __VA_ARGS__ override \
  { \
    return ReplaceOrInsertBefore(false, aNewChild, aRefChild, aResult); \
  } \
  NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aResult) __VA_ARGS__ override \
  { \
    return ReplaceOrInsertBefore(true, aNewChild, aOldChild, aResult); \
  } \
  NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aResult) __VA_ARGS__ override \
  { \
    return nsINode::RemoveChild(aOldChild, aResult); \
  } \
  NS_IMETHOD AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aResult) __VA_ARGS__ override \
  { \
    return InsertBefore(aNewChild, nullptr, aResult); \
  } \
  NS_IMETHOD HasChildNodes(bool* aResult) __VA_ARGS__ override \
  { \
    *aResult = nsINode::HasChildNodes(); \
    return NS_OK; \
  } \
  NS_IMETHOD CloneNode(bool aDeep, uint8_t aArgc, nsIDOMNode** aResult) __VA_ARGS__ override \
  { \
    if (aArgc == 0) { \
      aDeep = true; \
    } \
    mozilla::ErrorResult rv; \
    nsCOMPtr<nsINode> clone = nsINode::CloneNode(aDeep, rv); \
    if (rv.Failed()) { \
      return rv.ErrorCode(); \
    } \
    *aResult = clone.forget().take()->AsDOMNode(); \
    return NS_OK; \
  } \
  NS_IMETHOD Normalize() __VA_ARGS__ override \
  { \
    nsINode::Normalize(); \
    return NS_OK; \
  } \
  NS_IMETHOD GetNamespaceURI(nsAString& aNamespaceURI) __VA_ARGS__ override \
  { \
    nsINode::GetNamespaceURI(aNamespaceURI); \
    return NS_OK; \
  } \
  NS_IMETHOD GetPrefix(nsAString& aPrefix) __VA_ARGS__ override \
  { \
    nsINode::GetPrefix(aPrefix); \
    return NS_OK; \
  } \
  NS_IMETHOD GetLocalName(nsAString& aLocalName) __VA_ARGS__ override \
  { \
    aLocalName = nsINode::LocalName(); \
    return NS_OK; \
  } \
  NS_IMETHOD UnusedPlaceholder(bool* aResult) __VA_ARGS__ override \
  { \
    *aResult = false; \
    return NS_OK; \
  } \
  NS_IMETHOD GetDOMBaseURI(nsAString& aBaseURI) __VA_ARGS__ override \
  { \
    nsINode::GetBaseURI(aBaseURI); \
    return NS_OK; \
  } \
  NS_IMETHOD CompareDocumentPosition(nsIDOMNode* aOther, uint16_t* aResult) __VA_ARGS__ override \
  { \
    return nsINode::CompareDocumentPosition(aOther, aResult); \
  } \
  NS_IMETHOD GetTextContent(nsAString& aTextContent) __VA_ARGS__ override \
  { \
    mozilla::ErrorResult rv; \
    nsINode::GetTextContent(aTextContent, rv); \
    return rv.ErrorCode(); \
  } \
  NS_IMETHOD SetTextContent(const nsAString& aTextContent) __VA_ARGS__ override \
  { \
    mozilla::ErrorResult rv; \
    nsINode::SetTextContent(aTextContent, rv); \
    return rv.ErrorCode(); \
  } \
  NS_IMETHOD LookupPrefix(const nsAString& aNamespaceURI, nsAString& aResult) __VA_ARGS__ override \
  { \
    nsINode::LookupPrefix(aNamespaceURI, aResult); \
    return NS_OK; \
  } \
  NS_IMETHOD IsDefaultNamespace(const nsAString& aNamespaceURI, bool* aResult) __VA_ARGS__ override \
  { \
    *aResult = nsINode::IsDefaultNamespace(aNamespaceURI); \
    return NS_OK; \
  } \
  NS_IMETHOD LookupNamespaceURI(const nsAString& aPrefix, nsAString& aResult) __VA_ARGS__ override \
  { \
    nsINode::LookupNamespaceURI(aPrefix, aResult); \
    return NS_OK; \
  } \
  NS_IMETHOD IsEqualNode(nsIDOMNode* aArg, bool* aResult) __VA_ARGS__ override \
  { \
    return nsINode::IsEqualNode(aArg, aResult); \
  } \
  NS_IMETHOD SetUserData(const nsAString& aKey, nsIVariant* aData, nsIVariant** aResult) __VA_ARGS__ override \
  { \
    return nsINode::SetUserData(aKey, aData, aResult); \
  } \
  NS_IMETHOD GetUserData(const nsAString& aKey, nsIVariant** aResult) __VA_ARGS__ override \
  { \
    return nsINode::GetUserData(aKey, aResult); \
  } \
  NS_IMETHOD Contains(nsIDOMNode* aOther, bool* aResult) __VA_ARGS__ override \
  { \
    return nsINode::Contains(aOther, aResult); \
  }

#define NS_FORWARD_NSIDOMNODE_TO_NSINODE \
  NS_FORWARD_NSIDOMNODE_TO_NSINODE_HELPER(final)

#define NS_FORWARD_NSIDOMNODE_TO_NSINODE_OVERRIDABLE \
  NS_FORWARD_NSIDOMNODE_TO_NSINODE_HELPER()

#endif 
