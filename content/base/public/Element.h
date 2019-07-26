











#ifndef mozilla_dom_Element_h__
#define mozilla_dom_Element_h__

#include "mozilla/dom/FragmentOrElement.h" 
#include "nsChangeHint.h"                  
#include "nsEventStates.h"                 
#include "mozilla/dom/DirectionalityUtils.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocumentFragment.h"
#include "nsILinkHandler.h"
#include "nsNodeUtils.h"
#include "nsAttrAndChildArray.h"
#include "mozFlushType.h"
#include "nsDOMAttributeMap.h"
#include "nsIWeakReference.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDocument.h"
#include "nsIDOMNodeSelector.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsPresContext.h"
#include "nsDOMClassInfoID.h" 
#include "nsIDOMTouchEvent.h"
#include "nsIInlineEventHandlers.h"
#include "mozilla/CORSMode.h"
#include "mozilla/Attributes.h"
#include "nsContentUtils.h"
#include "nsINodeList.h"
#include "mozilla/ErrorResult.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMAttr.h"
#include "nsISMILAttr.h"
#include "nsClientRect.h"
#include "nsIDOMDOMTokenList.h"

class nsIDOMEventListener;
class nsIFrame;
class nsIDOMNamedNodeMap;
class nsIDOMCSSStyleDeclaration;
class nsIURI;
class nsINodeInfo;
class nsIControllers;
class nsEventChainVisitor;
class nsEventListenerManager;
class nsIScrollableFrame;
class nsAttrValueOrString;
class ContentUnbinder;
class nsClientRect;
class nsClientRectList;
class nsIHTMLCollection;
class nsContentList;
class nsDOMTokenList;
struct nsRect;
class nsEventStateManager;
class nsFocusManager;
class nsGlobalWindow;
class nsICSSDeclaration;
class nsISMILAttr;

already_AddRefed<nsContentList>
NS_GetContentList(nsINode* aRootNode,
                  int32_t  aMatchNameSpaceId,
                  const nsAString& aTagname);

#define ELEMENT_FLAG_BIT(n_) NODE_FLAG_BIT(NODE_TYPE_SPECIFIC_BITS_OFFSET + (n_))


enum {
  
  ELEMENT_HAS_PENDING_RESTYLE =                 ELEMENT_FLAG_BIT(0),

  
  
  
  ELEMENT_IS_POTENTIAL_RESTYLE_ROOT =           ELEMENT_FLAG_BIT(1),

  
  ELEMENT_HAS_PENDING_ANIMATION_RESTYLE =       ELEMENT_FLAG_BIT(2),

  
  
  
  ELEMENT_IS_POTENTIAL_ANIMATION_RESTYLE_ROOT = ELEMENT_FLAG_BIT(3),

  
  ELEMENT_ALL_RESTYLE_FLAGS = ELEMENT_HAS_PENDING_RESTYLE |
                              ELEMENT_IS_POTENTIAL_RESTYLE_ROOT |
                              ELEMENT_HAS_PENDING_ANIMATION_RESTYLE |
                              ELEMENT_IS_POTENTIAL_ANIMATION_RESTYLE_ROOT,

  
  ELEMENT_PENDING_RESTYLE_FLAGS = ELEMENT_HAS_PENDING_RESTYLE |
                                  ELEMENT_HAS_PENDING_ANIMATION_RESTYLE,

  
  ELEMENT_TYPE_SPECIFIC_BITS_OFFSET = NODE_TYPE_SPECIFIC_BITS_OFFSET + 4
};

#undef ELEMENT_FLAG_BIT

namespace mozilla {
namespace dom {

class Link;
class Element;

} 
} 

typedef mozilla::dom::Element nsGenericElement;

namespace mozilla {
namespace dom {


#define NS_ELEMENT_IID \
{ 0xc6c049a1, 0x96e8, 0x4580, \
  { 0xa6, 0x93, 0xb9, 0x5f, 0x53, 0xbe, 0xe8, 0x1c } }

class Element : public FragmentOrElement
{
public:
#ifdef MOZILLA_INTERNAL_API
  Element(already_AddRefed<nsINodeInfo> aNodeInfo) :
    FragmentOrElement(aNodeInfo),
    mState(NS_EVENT_STATE_MOZ_READONLY)
  {
    NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::ELEMENT_NODE,
                      "Bad NodeType in aNodeInfo");
    SetIsElement();
  }
#endif 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ELEMENT_IID)

  



  nsEventStates State() const {
    
    
    return mState;
  }

  









  void UpdateState(bool aNotify);
  
  


  void UpdateLinkState(nsEventStates aState);

  



  bool IsFullScreenAncestor() const {
    return mState.HasAtLeastOneOfStates(NS_EVENT_STATE_FULL_SCREEN_ANCESTOR |
                                        NS_EVENT_STATE_FULL_SCREEN);
  }

  



  nsEventStates StyleState() const {
    if (!HasLockedStyleStates()) {
      return mState;
    }
    return StyleStateFromLocks();
  }

  


  nsEventStates LockedStyleStates() const;

  


  void LockStyleStates(nsEventStates aStates);

  


  void UnlockStyleStates(nsEventStates aStates);

  


  void ClearStyleStateLocks();

  


  virtual css::StyleRule* GetInlineStyleRule();

  



  virtual nsresult SetInlineStyleRule(css::StyleRule* aStyleRule,
                                      const nsAString* aSerialized,
                                      bool aNotify);

  



  virtual css::StyleRule* GetSMILOverrideStyleRule();

  




  virtual nsresult SetSMILOverrideStyleRule(css::StyleRule* aStyleRule,
                                            bool aNotify);

  





  virtual nsISMILAttr* GetAnimatedAttr(int32_t aNamespaceID, nsIAtom* aName)
  {
    return nullptr;
  }

  







  virtual nsICSSDeclaration* GetSMILOverrideStyle();

  


  virtual bool IsLabelable() const;

  






  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  





  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;

  




  virtual nsIAtom *GetClassAttributeName() const;

  inline mozilla::directionality::Directionality GetDirectionality() const {
    if (HasFlag(NODE_HAS_DIRECTION_RTL)) {
      return mozilla::directionality::eDir_RTL;
    }

    if (HasFlag(NODE_HAS_DIRECTION_LTR)) {
      return mozilla::directionality::eDir_LTR;
    }

    return mozilla::directionality::eDir_NotSet;
  }

  inline void SetDirectionality(mozilla::directionality::Directionality aDir,
                                bool aNotify) {
    UnsetFlags(NODE_ALL_DIRECTION_FLAGS);
    if (!aNotify) {
      RemoveStatesSilently(DIRECTION_STATES);
    }

    switch (aDir) {
      case (mozilla::directionality::eDir_RTL):
        SetFlags(NODE_HAS_DIRECTION_RTL);
        if (!aNotify) {
          AddStatesSilently(NS_EVENT_STATE_RTL);
        }
        break;

      case(mozilla::directionality::eDir_LTR):
        SetFlags(NODE_HAS_DIRECTION_LTR);
        if (!aNotify) {
          AddStatesSilently(NS_EVENT_STATE_LTR);
        }
        break;

      default:
        break;
    }

    




    if (aNotify) {
      UpdateState(true);
    }
  }

  bool GetBindingURL(nsIDocument *aDocument, css::URLValue **aResult);

protected:
  





  virtual nsEventStates IntrinsicState() const;

  





  void AddStatesSilently(nsEventStates aStates) {
    mState |= aStates;
  }

  





  void RemoveStatesSilently(nsEventStates aStates) {
    mState &= ~aStates;
  }

private:
  
  
  friend class ::nsEventStateManager;
  friend class ::nsGlobalWindow;
  friend class ::nsFocusManager;

  
  friend class Link;

  void NotifyStateChange(nsEventStates aStates);

  void NotifyStyleStateChange(nsEventStates aStates);

  
  nsEventStates StyleStateFromLocks() const;

  
  
  
  void AddStates(nsEventStates aStates) {
    NS_PRECONDITION(!aStates.HasAtLeastOneOfStates(INTRINSIC_STATES),
                    "Should only be adding ESM-managed states here");
    AddStatesSilently(aStates);
    NotifyStateChange(aStates);
  }
  void RemoveStates(nsEventStates aStates) {
    NS_PRECONDITION(!aStates.HasAtLeastOneOfStates(INTRINSIC_STATES),
                    "Should only be removing ESM-managed states here");
    RemoveStatesSilently(aStates);
    NotifyStateChange(aStates);
  }

  nsEventStates mState;

public:
  virtual void UpdateEditableState(bool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const;
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }

  NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes);

  















  bool MaybeCheckSameAttrVal(int32_t aNamespaceID, nsIAtom* aName,
                             nsIAtom* aPrefix,
                             const nsAttrValueOrString& aValue,
                             bool aNotify, nsAttrValue& aOldValue,
                             uint8_t* aModType, bool* aHasListeners);

  bool OnlyNotifySameValueSet(int32_t aNamespaceID, nsIAtom* aName,
                              nsIAtom* aPrefix,
                              const nsAttrValueOrString& aValue,
                              bool aNotify, nsAttrValue& aOldValue,
                              uint8_t* aModType, bool* aHasListeners)
  {
    if (MaybeCheckSameAttrVal(aNamespaceID, aName, aPrefix, aValue, aNotify,
                              aOldValue, aModType, aHasListeners)) {
      nsAutoScriptBlocker scriptBlocker;
      nsNodeUtils::AttributeSetToCurrentValue(this, aNamespaceID, aName);
      return true;
    }
    return false;
  }

  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName, nsIAtom* aPrefix,
                           const nsAString& aValue, bool aNotify);
  virtual nsresult SetParsedAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 nsIAtom* aPrefix, nsAttrValue& aParsedValue,
                                 bool aNotify);
  virtual bool GetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                         nsAString& aResult) const;
  virtual bool HasAttr(int32_t aNameSpaceID, nsIAtom* aName) const;
  virtual bool AttrValueIs(int32_t aNameSpaceID, nsIAtom* aName,
                             const nsAString& aValue,
                             nsCaseTreatment aCaseSensitive) const;
  virtual bool AttrValueIs(int32_t aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aValue,
                             nsCaseTreatment aCaseSensitive) const;
  virtual int32_t FindAttrValueIn(int32_t aNameSpaceID,
                                  nsIAtom* aName,
                                  AttrValuesArray* aValues,
                                  nsCaseTreatment aCaseSensitive) const;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);
  virtual const nsAttrName* GetAttrNameAt(uint32_t aIndex) const;
  virtual uint32_t GetAttrCount() const;
  virtual bool IsNodeOfType(uint32_t aFlags) const;

#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const
  {
    List(out, aIndent, EmptyCString());
  }
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const;
  void List(FILE* out, int32_t aIndent, const nsCString& aPrefix) const;
  void ListAttributes(FILE* out) const;
#endif

  


  struct MappedAttributeEntry {
    nsIAtom** attribute;
  };

  





  template<size_t N>
  static bool
  FindAttributeDependence(const nsIAtom* aAttribute,
                          const MappedAttributeEntry* const (&aMaps)[N])
  {
    return FindAttributeDependence(aAttribute, aMaps, N);
  }

private:
  static bool
  FindAttributeDependence(const nsIAtom* aAttribute,
                          const MappedAttributeEntry* const aMaps[],
                          uint32_t aMapCount);

public:
  void GetTagName(nsAString& aTagName) const
  {
    aTagName = NodeName();
  }
  void GetId(nsAString& aId) const
  {
    GetAttr(kNameSpaceID_None, nsGkAtoms::id, aId);
  }
  void SetId(const nsAString& aId)
  {
    SetAttr(kNameSpaceID_None, nsGkAtoms::id, aId, true);
  }

  nsDOMTokenList* ClassList();
  nsDOMAttributeMap* GetAttributes()
  {
    nsDOMSlots *slots = DOMSlots();
    if (!slots->mAttributeMap) {
      slots->mAttributeMap = new nsDOMAttributeMap(this);
    }

    return slots->mAttributeMap;
  }
  virtual void GetAttribute(const nsAString& aName, nsString& aReturn);
  void GetAttributeNS(const nsAString& aNamespaceURI,
                      const nsAString& aLocalName,
                      nsAString& aReturn);
  void SetAttribute(const nsAString& aName, const nsAString& aValue,
                    mozilla::ErrorResult& aError);
  void SetAttributeNS(const nsAString& aNamespaceURI,
                      const nsAString& aLocalName,
                      const nsAString& aValue,
                      mozilla::ErrorResult& aError);
  virtual void RemoveAttribute(const nsAString& aName,
                               mozilla::ErrorResult& aError);
  void RemoveAttributeNS(const nsAString& aNamespaceURI,
                         const nsAString& aLocalName,
                         mozilla::ErrorResult& aError);
  virtual bool HasAttribute(const nsAString& aName) const
  {
    return InternalGetExistingAttrNameFromQName(aName) != nullptr;
  }
  bool HasAttributeNS(const nsAString& aNamespaceURI,
                      const nsAString& aLocalName) const;
  already_AddRefed<nsIHTMLCollection>
    GetElementsByTagName(const nsAString& aQualifiedName);
  already_AddRefed<nsIHTMLCollection>
    GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                           const nsAString& aLocalName,
                           mozilla::ErrorResult& aError);
  already_AddRefed<nsIHTMLCollection>
    GetElementsByClassName(const nsAString& aClassNames);
  nsGenericElement* GetFirstElementChild() const;
  nsGenericElement* GetLastElementChild() const;
  nsGenericElement* GetPreviousElementSibling() const
  {
    nsIContent* previousSibling = GetPreviousSibling();
    while (previousSibling) {
      if (previousSibling->IsElement()) {
        return static_cast<nsGenericElement*>(previousSibling);
      }
      previousSibling = previousSibling->GetPreviousSibling();
    }

    return nullptr;
  }
  nsGenericElement* GetNextElementSibling() const
  {
    nsIContent* nextSibling = GetNextSibling();
    while (nextSibling) {
      if (nextSibling->IsElement()) {
        return static_cast<nsGenericElement*>(nextSibling);
      }
      nextSibling = nextSibling->GetNextSibling();
    }

    return nullptr;
  }
  uint32_t ChildElementCount()
  {
    return Children()->Length();
  }
  bool MozMatchesSelector(const nsAString& aSelector,
                          mozilla::ErrorResult& aError);
  void SetCapture(bool aRetargetToElement)
  {
    
    
    
    if (!nsIPresShell::GetCapturingContent()) {
      nsIPresShell::SetCapturingContent(this, CAPTURE_PREVENTDRAG |
        (aRetargetToElement ? CAPTURE_RETARGETTOELEMENT : 0));
    }
  }
  void ReleaseCapture()
  {
    if (nsIPresShell::GetCapturingContent() == this) {
      nsIPresShell::SetCapturingContent(nullptr, 0);
    }
  }
  void MozRequestFullScreen();
  void MozRequestPointerLock()
  {
    OwnerDoc()->RequestPointerLock(this);
  }
  nsIDOMAttr* GetAttributeNode(const nsAString& aName);
  already_AddRefed<nsIDOMAttr> SetAttributeNode(nsIDOMAttr* aNewAttr,
                                                mozilla::ErrorResult& aError);
  already_AddRefed<nsIDOMAttr> RemoveAttributeNode(nsIDOMAttr* aOldAttr,
                                                   mozilla::ErrorResult& aError);
  nsIDOMAttr* GetAttributeNodeNS(const nsAString& aNamespaceURI,
                                 const nsAString& aLocalName,
                                 mozilla::ErrorResult& aError);
  already_AddRefed<nsIDOMAttr> SetAttributeNodeNS(nsIDOMAttr* aNewAttr,
                                                  mozilla::ErrorResult& aError);

  already_AddRefed<nsClientRectList> GetClientRects(mozilla::ErrorResult& aError);
  already_AddRefed<nsClientRect> GetBoundingClientRect();
  void ScrollIntoView(bool aTop);
  int32_t ScrollTop()
  {
    nsIScrollableFrame* sf = GetScrollFrame();
    return sf ? sf->GetScrollPositionCSSPixels().y : 0;
  }
  void SetScrollTop(int32_t aScrollTop)
  {
    nsIScrollableFrame* sf = GetScrollFrame();
    if (sf) {
      sf->ScrollToCSSPixels(nsIntPoint(sf->GetScrollPositionCSSPixels().x,
                                       aScrollTop));
    }
  }
  int32_t ScrollLeft()
  {
    nsIScrollableFrame* sf = GetScrollFrame();
    return sf ? sf->GetScrollPositionCSSPixels().x : 0;
  }
  void SetScrollLeft(int32_t aScrollLeft)
  {
    nsIScrollableFrame* sf = GetScrollFrame();
    if (sf) {
      sf->ScrollToCSSPixels(nsIntPoint(aScrollLeft,
                                       sf->GetScrollPositionCSSPixels().y));
    }
  }
  int32_t ScrollWidth();
  int32_t ScrollHeight();
  int32_t ClientTop()
  {
    return nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().y);
  }
  int32_t ClientLeft()
  {
    return nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().x);
  }
  int32_t ClientWidth()
  {
    return nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().width);
  }
  int32_t ClientHeight()
  {
    return nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().height);
  }
  int32_t ScrollTopMax()
  {
    nsIScrollableFrame* sf = GetScrollFrame();
    return sf ?
           nsPresContext::AppUnitsToIntCSSPixels(sf->GetScrollRange().YMost()) :
           0;
  }
  int32_t ScrollLeftMax()
  {
    nsIScrollableFrame* sf = GetScrollFrame();
    return sf ?
           nsPresContext::AppUnitsToIntCSSPixels(sf->GetScrollRange().XMost()) :
           0;
  }

  

  






  nsresult SetEventHandler(nsIAtom* aEventName,
                           const nsAString& aValue,
                           bool aDefer = true);

  


  nsresult LeaveLink(nsPresContext* aPresContext);

  static bool ShouldBlur(nsIContent *aContent);

  






  static nsresult DispatchClickEvent(nsPresContext* aPresContext,
                                     nsInputEvent* aSourceEvent,
                                     nsIContent* aTarget,
                                     bool aFullDispatch,
                                     uint32_t aFlags,
                                     nsEventStatus* aStatus);

  






  using nsIContent::DispatchEvent;
  static nsresult DispatchEvent(nsPresContext* aPresContext,
                                nsEvent* aEvent,
                                nsIContent* aTarget,
                                bool aFullDispatch,
                                nsEventStatus* aStatus);

  






  nsIFrame* GetPrimaryFrame(mozFlushType aType);
  
  nsIFrame* GetPrimaryFrame() const { return nsIContent::GetPrimaryFrame(); }

  



  struct nsAttrInfo {
    nsAttrInfo(const nsAttrName* aName, const nsAttrValue* aValue) :
      mName(aName), mValue(aValue) {}
    nsAttrInfo(const nsAttrInfo& aOther) :
      mName(aOther.mName), mValue(aOther.mValue) {}

    const nsAttrName* mName;
    const nsAttrValue* mValue;
  };

  
  
  const nsAttrValue* GetParsedAttr(nsIAtom* aAttr) const
  {
    return mAttrsAndChildren.GetAttr(aAttr);
  }

  




  nsDOMAttributeMap *GetAttributeMap()
  {
    nsDOMSlots *slots = GetExistingDOMSlots();

    return slots ? slots->mAttributeMap.get() : nullptr;
  }

  virtual void RecompileScriptEventListeners()
  {
  }

  







  virtual nsAttrInfo GetAttrInfo(int32_t aNamespaceID, nsIAtom* aName) const;

  virtual void NodeInfoChanged(nsINodeInfo* aOldNodeInfo)
  {
  }

  




  static void ParseCORSValue(const nsAString& aValue, nsAttrValue& aResult);

  


  static mozilla::CORSMode StringToCORSMode(const nsAString& aValue);
  
  



  static mozilla::CORSMode AttrValueToCORSMode(const nsAttrValue* aValue);

  
  
  void
    GetElementsByTagName(const nsAString& aQualifiedName,
                         nsIDOMHTMLCollection** aResult);
  nsresult
    GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                           const nsAString& aLocalName,
                           nsIDOMHTMLCollection** aResult);
  nsresult
    GetElementsByClassName(const nsAString& aClassNames,
                           nsIDOMHTMLCollection** aResult);
  void GetClassList(nsIDOMDOMTokenList** aClassList);

  virtual JSObject* WrapObject(JSContext *aCx, JSObject *aScope,
                               bool *aTriedToWrap) MOZ_FINAL;

protected:
  



  static const bool kFireMutationEvent           = true;
  static const bool kDontFireMutationEvent       = false;
  static const bool kNotifyDocumentObservers     = true;
  static const bool kDontNotifyDocumentObservers = false;
  static const bool kCallAfterSetAttr            = true;
  static const bool kDontCallAfterSetAttr        = false;

  





















  nsresult SetAttrAndNotify(int32_t aNamespaceID,
                            nsIAtom* aName,
                            nsIAtom* aPrefix,
                            const nsAttrValue& aOldValue,
                            nsAttrValue& aParsedValue,
                            uint8_t aModType,
                            bool aFireMutation,
                            bool aNotify,
                            bool aCallAfterSetAttr);

  










  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  












  virtual bool SetMappedAttribute(nsIDocument* aDocument,
                                    nsIAtom* aName,
                                    nsAttrValue& aValue,
                                    nsresult* aRetval);

  













  
  
  virtual nsresult BeforeSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify)
  {
    return NS_OK;
  }

  











  
  
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify)
  {
    return NS_OK;
  }

  



  virtual nsEventListenerManager*
    GetEventListenerManagerForAttr(nsIAtom* aAttrName, bool* aDefer);

  


  virtual const nsAttrName* InternalGetExistingAttrNameFromQName(const nsAString& aStr) const;

  





  virtual nsGenericElement* GetOffsetRect(nsRect& aRect);

  




  nsIntSize GetPaddingRectSize();

  nsIFrame* GetStyledFrame();

  virtual mozilla::dom::Element* GetNameSpaceElement()
  {
    return this;
  }

  nsIDOMAttr* GetAttributeNodeNSInternal(const nsAString& aNamespaceURI,
                                         const nsAString& aLocalName,
                                         mozilla::ErrorResult& aError);

  void RegisterFreezableElement() {
    OwnerDoc()->RegisterFreezableElement(this);
  }
  void UnregisterFreezableElement() {
    OwnerDoc()->UnregisterFreezableElement(this);
  }

  


  void AddToIdTable(nsIAtom* aId) {
    NS_ASSERTION(HasID(), "Node doesn't have an ID?");
    nsIDocument* doc = GetCurrentDoc();
    if (doc && (!IsInAnonymousSubtree() || doc->IsXUL())) {
      doc->AddToIdTable(this, aId);
    }
  }
  void RemoveFromIdTable() {
    if (HasID()) {
      nsIDocument* doc = GetCurrentDoc();
      if (doc) {
        nsIAtom* id = DoGetID();
        
        
        
        if (id) {
          doc->RemoveFromIdTable(this, DoGetID());
        }
      }
    }
  }

  




  







  bool CheckHandleEventForLinksPrecondition(nsEventChainVisitor& aVisitor,
                                              nsIURI** aURI) const;

  


  nsresult PreHandleEventForLinks(nsEventChainPreVisitor& aVisitor);

  


  nsresult PostHandleEventForLinks(nsEventChainPostVisitor& aVisitor);

  









  virtual void GetLinkTarget(nsAString& aTarget);

private:
  



  nsRect GetClientAreaRect();

  nsIScrollableFrame* GetScrollFrame(nsIFrame **aStyledFrame = nullptr);
};

NS_DEFINE_STATIC_IID_ACCESSOR(Element, NS_ELEMENT_IID)

} 
} 

inline mozilla::dom::Element* nsINode::AsElement()
{
  MOZ_ASSERT(IsElement());
  return static_cast<mozilla::dom::Element*>(this);
}

inline const mozilla::dom::Element* nsINode::AsElement() const
{
  MOZ_ASSERT(IsElement());
  return static_cast<const mozilla::dom::Element*>(this);
}

inline bool nsINode::HasAttributes() const
{
  return IsElement() && AsElement()->GetAttrCount() > 0;
}





#define NS_IMPL_ELEMENT_CLONE(_elementName)                                 \
nsresult                                                                    \
_elementName::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const        \
{                                                                           \
  *aResult = nullptr;                                                        \
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;                                     \
  _elementName *it = new _elementName(ni.forget());                         \
  if (!it) {                                                                \
    return NS_ERROR_OUT_OF_MEMORY;                                          \
  }                                                                         \
                                                                            \
  nsCOMPtr<nsINode> kungFuDeathGrip = it;                                   \
  nsresult rv = const_cast<_elementName*>(this)->CopyInnerTo(it);           \
  if (NS_SUCCEEDED(rv)) {                                                   \
    kungFuDeathGrip.swap(*aResult);                                         \
  }                                                                         \
                                                                            \
  return rv;                                                                \
}

#define NS_IMPL_ELEMENT_CLONE_WITH_INIT(_elementName)                       \
nsresult                                                                    \
_elementName::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const        \
{                                                                           \
  *aResult = nullptr;                                                        \
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;                                     \
  _elementName *it = new _elementName(ni.forget());                         \
  if (!it) {                                                                \
    return NS_ERROR_OUT_OF_MEMORY;                                          \
  }                                                                         \
                                                                            \
  nsCOMPtr<nsINode> kungFuDeathGrip = it;                                   \
  nsresult rv = it->Init();                                                 \
  nsresult rv2 = const_cast<_elementName*>(this)->CopyInnerTo(it);          \
  if (NS_FAILED(rv2)) {                                                     \
    rv = rv2;                                                               \
  }                                                                         \
  if (NS_SUCCEEDED(rv)) {                                                   \
    kungFuDeathGrip.swap(*aResult);                                         \
  }                                                                         \
                                                                            \
  return rv;                                                                \
}

#define DOMCI_NODE_DATA(_interface, _class)                             \
  DOMCI_DATA(_interface, _class)                                        \
  nsXPCClassInfo* _class::GetClassInfo()                                \
  {                                                                     \
    return static_cast<nsXPCClassInfo*>(                                \
      NS_GetDOMClassInfoInstance(eDOMClassInfo_##_interface##_id));     \
  }








#define NS_IMPL_STRING_ATTR(_class, _method, _atom)                     \
  NS_IMETHODIMP                                                         \
  _class::Get##_method(nsAString& aValue)                               \
  {                                                                     \
    GetAttr(kNameSpaceID_None, nsGkAtoms::_atom, aValue);               \
    return NS_OK;                                                       \
  }                                                                     \
  NS_IMETHODIMP                                                         \
  _class::Set##_method(const nsAString& aValue)                         \
  {                                                                     \
    return SetAttr(kNameSpaceID_None, nsGkAtoms::_atom, nullptr, aValue, true); \
  }

#define NS_FORWARD_NSIDOMELEMENT_TO_GENERIC                                   \
NS_IMETHOD GetTagName(nsAString& aTagName) MOZ_FINAL                          \
{                                                                             \
  nsGenericElement::GetTagName(aTagName);                                     \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClassList(nsIDOMDOMTokenList** aClassList) MOZ_FINAL            \
{                                                                             \
  nsGenericElement::GetClassList(aClassList);                                 \
  return NS_OK;                                                               \
}                                                                             \
using nsGenericElement::GetAttribute;                                         \
NS_IMETHOD GetAttribute(const nsAString& name, nsAString& _retval) MOZ_FINAL  \
{                                                                             \
  nsString attr;                                                              \
  GetAttribute(name, attr);                                                   \
  _retval = attr;                                                             \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetAttributeNS(const nsAString& namespaceURI,                      \
                          const nsAString& localName,                         \
                          nsAString& _retval) MOZ_FINAL                       \
{                                                                             \
  nsGenericElement::GetAttributeNS(namespaceURI, localName, _retval);         \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetAttribute(const nsAString& name,                                \
                        const nsAString& value)                               \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  nsGenericElement::SetAttribute(name, value, rv);                            \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD SetAttributeNS(const nsAString& namespaceURI,                      \
                          const nsAString& qualifiedName,                     \
                          const nsAString& value) MOZ_FINAL                   \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  nsGenericElement::SetAttributeNS(namespaceURI, qualifiedName, value, rv);   \
  return rv.ErrorCode();                                                      \
}                                                                             \
using nsGenericElement::RemoveAttribute;                                      \
NS_IMETHOD RemoveAttribute(const nsAString& name) MOZ_FINAL                   \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  RemoveAttribute(name, rv);                                                  \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD RemoveAttributeNS(const nsAString& namespaceURI,                   \
                             const nsAString& localName) MOZ_FINAL            \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  nsGenericElement::RemoveAttributeNS(namespaceURI, localName, rv);           \
  return rv.ErrorCode();                                                      \
}                                                                             \
using nsGenericElement::HasAttribute;                                         \
NS_IMETHOD HasAttribute(const nsAString& name,                                \
                           bool* _retval) MOZ_FINAL                           \
{                                                                             \
  *_retval = HasAttribute(name);                                              \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD HasAttributeNS(const nsAString& namespaceURI,                      \
                          const nsAString& localName,                         \
                          bool* _retval) MOZ_FINAL                            \
{                                                                             \
  *_retval = nsGenericElement::HasAttributeNS(namespaceURI, localName);       \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetAttributeNode(const nsAString& name,                            \
                            nsIDOMAttr** _retval) MOZ_FINAL                   \
{                                                                             \
  NS_IF_ADDREF(*_retval = nsGenericElement::GetAttributeNode(name));          \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetAttributeNode(nsIDOMAttr* newAttr,                              \
                            nsIDOMAttr** _retval) MOZ_FINAL                   \
{                                                                             \
  if (!newAttr) {                                                             \
    return NS_ERROR_INVALID_POINTER;                                          \
  }                                                                           \
  mozilla::ErrorResult rv;                                                    \
  *_retval = nsGenericElement::SetAttributeNode(newAttr, rv).get();           \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD RemoveAttributeNode(nsIDOMAttr* oldAttr,                           \
                               nsIDOMAttr** _retval) MOZ_FINAL                \
{                                                                             \
  if (!oldAttr) {                                                             \
    return NS_ERROR_INVALID_POINTER;                                          \
  }                                                                           \
  mozilla::ErrorResult rv;                                                    \
  *_retval = nsGenericElement::RemoveAttributeNode(oldAttr, rv).get();        \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD GetAttributeNodeNS(const nsAString& namespaceURI,                  \
                              const nsAString& localName,                     \
                              nsIDOMAttr** _retval) MOZ_FINAL                 \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  NS_IF_ADDREF(*_retval = nsGenericElement::GetAttributeNodeNS(namespaceURI,  \
                                                               localName,     \
                                                               rv));          \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD SetAttributeNodeNS(nsIDOMAttr* newAttr,                            \
                              nsIDOMAttr** _retval) MOZ_FINAL                 \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  *_retval = nsGenericElement::SetAttributeNodeNS(newAttr, rv).get();         \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD GetElementsByTagName(const nsAString& name,                        \
                                nsIDOMHTMLCollection** _retval) MOZ_FINAL     \
{                                                                             \
  nsGenericElement::GetElementsByTagName(name, _retval);                      \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetElementsByTagNameNS(const nsAString& namespaceURI,              \
                                  const nsAString& localName,                 \
                                  nsIDOMHTMLCollection** _retval) MOZ_FINAL   \
{                                                                             \
  return nsGenericElement::GetElementsByTagNameNS(namespaceURI, localName,    \
                                                  _retval);                   \
}                                                                             \
NS_IMETHOD GetElementsByClassName(const nsAString& classes,                   \
                                  nsIDOMHTMLCollection** _retval) MOZ_FINAL   \
{                                                                             \
  return nsGenericElement::GetElementsByClassName(classes, _retval);          \
}                                                                             \
NS_IMETHOD GetChildElements(nsIDOMNodeList** aChildElements) MOZ_FINAL        \
{                                                                             \
  nsIHTMLCollection* list = FragmentOrElement::Children();                    \
  return CallQueryInterface(list, aChildElements);                            \
}                                                                             \
NS_IMETHOD GetFirstElementChild(nsIDOMElement** aFirstElementChild) MOZ_FINAL \
{                                                                             \
  nsGenericElement* element = nsGenericElement::GetFirstElementChild();       \
  if (!element) {                                                             \
    *aFirstElementChild = nullptr;                                            \
    return NS_OK;                                                             \
  }                                                                           \
  return CallQueryInterface(element, aFirstElementChild);                     \
}                                                                             \
NS_IMETHOD GetLastElementChild(nsIDOMElement** aLastElementChild) MOZ_FINAL   \
{                                                                             \
  nsGenericElement* element = nsGenericElement::GetLastElementChild();        \
  if (!element) {                                                             \
    *aLastElementChild = nullptr;                                             \
    return NS_OK;                                                             \
  }                                                                           \
  return CallQueryInterface(element, aLastElementChild);                      \
}                                                                             \
NS_IMETHOD GetPreviousElementSibling(nsIDOMElement** aPreviousElementSibling) \
  MOZ_FINAL                                                                   \
{                                                                             \
  nsGenericElement* element = nsGenericElement::GetPreviousElementSibling();  \
  if (!element) {                                                             \
    *aPreviousElementSibling = nullptr;                                       \
    return NS_OK;                                                             \
  }                                                                           \
  return CallQueryInterface(element, aPreviousElementSibling);                \
}                                                                             \
NS_IMETHOD GetNextElementSibling(nsIDOMElement** aNextElementSibling)         \
  MOZ_FINAL                                                                   \
{                                                                             \
  nsGenericElement* element = nsGenericElement::GetNextElementSibling();      \
  if (!element) {                                                             \
    *aNextElementSibling = nullptr;                                           \
    return NS_OK;                                                             \
  }                                                                           \
  return CallQueryInterface(element, aNextElementSibling);                    \
}                                                                             \
NS_IMETHOD GetChildElementCount(uint32_t* aChildElementCount) MOZ_FINAL       \
{                                                                             \
  *aChildElementCount = nsGenericElement::ChildElementCount();                \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetOnmouseenter(JSContext* cx, JS::Value* aOnmouseenter) MOZ_FINAL \
{                                                                             \
  return nsGenericElement::GetOnmouseenter(cx, aOnmouseenter);                \
}                                                                             \
NS_IMETHOD SetOnmouseenter(JSContext* cx,                                     \
                           const JS::Value& aOnmouseenter) MOZ_FINAL          \
{                                                                             \
  return nsGenericElement::SetOnmouseenter(cx, aOnmouseenter);                \
}                                                                             \
NS_IMETHOD GetOnmouseleave(JSContext* cx, JS::Value* aOnmouseleave) MOZ_FINAL \
{                                                                             \
  return nsGenericElement::GetOnmouseleave(cx, aOnmouseleave);                \
}                                                                             \
NS_IMETHOD SetOnmouseleave(JSContext* cx,                                     \
                           const JS::Value& aOnmouseleave) MOZ_FINAL          \
{                                                                             \
  return nsGenericElement::SetOnmouseleave(cx, aOnmouseleave);                \
}                                                                             \
NS_IMETHOD GetClientRects(nsIDOMClientRectList** _retval) MOZ_FINAL           \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  *_retval = nsGenericElement::GetClientRects(rv).get();                      \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD GetBoundingClientRect(nsIDOMClientRect** _retval) MOZ_FINAL        \
{                                                                             \
  *_retval = nsGenericElement::GetBoundingClientRect().get();                 \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollTop(int32_t* aScrollTop) MOZ_FINAL                        \
{                                                                             \
  *aScrollTop = nsGenericElement::ScrollTop();                                \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetScrollTop(int32_t aScrollTop) MOZ_FINAL                         \
{                                                                             \
  nsGenericElement::SetScrollTop(aScrollTop);                                 \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollLeft(int32_t* aScrollLeft) MOZ_FINAL                      \
{                                                                             \
  *aScrollLeft = nsGenericElement::ScrollLeft();                              \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetScrollLeft(int32_t aScrollLeft) MOZ_FINAL                       \
{                                                                             \
  nsGenericElement::SetScrollLeft(aScrollLeft);                               \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollWidth(int32_t* aScrollWidth) MOZ_FINAL                    \
{                                                                             \
  *aScrollWidth = nsGenericElement::ScrollWidth();                            \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollHeight(int32_t* aScrollHeight) MOZ_FINAL                  \
{                                                                             \
  *aScrollHeight = nsGenericElement::ScrollHeight();                          \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClientTop(int32_t* aClientTop) MOZ_FINAL                        \
{                                                                             \
  *aClientTop = nsGenericElement::ClientTop();                                \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClientLeft(int32_t* aClientLeft) MOZ_FINAL                      \
{                                                                             \
  *aClientLeft = nsGenericElement::ClientLeft();                              \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClientWidth(int32_t* aClientWidth) MOZ_FINAL                    \
{                                                                             \
  *aClientWidth = nsGenericElement::ClientWidth();                            \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClientHeight(int32_t* aClientHeight) MOZ_FINAL                  \
{                                                                             \
  *aClientHeight = nsGenericElement::ClientHeight();                          \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollLeftMax(int32_t* aScrollLeftMax) MOZ_FINAL                \
{                                                                             \
  *aScrollLeftMax = nsGenericElement::ScrollLeftMax();                        \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollTopMax(int32_t* aScrollTopMax) MOZ_FINAL                  \
{                                                                             \
  *aScrollTopMax = nsGenericElement::ScrollTopMax();                          \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD MozMatchesSelector(const nsAString& selector,                      \
                              bool* _retval) MOZ_FINAL                        \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  *_retval = nsGenericElement::MozMatchesSelector(selector, rv);              \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD SetCapture(bool retargetToElement) MOZ_FINAL                       \
{                                                                             \
  nsGenericElement::SetCapture(retargetToElement);                            \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD ReleaseCapture(void) MOZ_FINAL                                     \
{                                                                             \
  nsGenericElement::ReleaseCapture();                                         \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD MozRequestFullScreen(void) MOZ_FINAL                               \
{                                                                             \
  nsGenericElement::MozRequestFullScreen();                                   \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD MozRequestPointerLock(void) MOZ_FINAL                              \
{                                                                             \
  nsGenericElement::MozRequestPointerLock();                                  \
  return NS_OK;                                                               \
}

#endif 
