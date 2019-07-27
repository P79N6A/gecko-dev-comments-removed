











#ifndef mozilla_dom_Element_h__
#define mozilla_dom_Element_h__

#include "mozilla/dom/FragmentOrElement.h" 
#include "nsChangeHint.h"                  
#include "mozilla/EventStates.h"           
#include "mozilla/dom/DirectionalityUtils.h"
#include "nsIDOMElement.h"
#include "nsILinkHandler.h"
#include "nsNodeUtils.h"
#include "nsAttrAndChildArray.h"
#include "mozFlushType.h"
#include "nsDOMAttributeMap.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsPresContext.h"
#include "nsDOMClassInfoID.h" 
#include "mozilla/CORSMode.h"
#include "mozilla/Attributes.h"
#include "nsIScrollableFrame.h"
#include "mozilla/dom/Attr.h"
#include "nsISMILAttr.h"
#include "mozilla/dom/DOMRect.h"
#include "nsAttrValue.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/WindowBinding.h"
#include "Units.h"

class nsIDOMEventListener;
class nsIFrame;
class nsIDOMMozNamedAttrMap;
class nsIDOMCSSStyleDeclaration;
class nsIURI;
class nsIControllers;
class nsEventChainVisitor;
class nsIScrollableFrame;
class nsAttrValueOrString;
class ContentUnbinder;
class nsContentList;
class nsDOMSettableTokenList;
class nsDOMTokenList;
struct nsRect;
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

  
  
  
  
  ELEMENT_HAS_PENDING_ANIMATION_ONLY_RESTYLE =  ELEMENT_FLAG_BIT(4),

  
  
  
  ELEMENT_IS_POTENTIAL_ANIMATION_ONLY_RESTYLE_ROOT = ELEMENT_FLAG_BIT(5),

  
  ELEMENT_ALL_RESTYLE_FLAGS = ELEMENT_HAS_PENDING_RESTYLE |
                              ELEMENT_IS_POTENTIAL_RESTYLE_ROOT |
                              ELEMENT_HAS_PENDING_ANIMATION_RESTYLE |
                              ELEMENT_IS_POTENTIAL_ANIMATION_RESTYLE_ROOT |
                              ELEMENT_HAS_PENDING_ANIMATION_ONLY_RESTYLE |
                              ELEMENT_IS_POTENTIAL_ANIMATION_ONLY_RESTYLE_ROOT,

  
  ELEMENT_PENDING_RESTYLE_FLAGS = ELEMENT_HAS_PENDING_RESTYLE |
                                  ELEMENT_HAS_PENDING_ANIMATION_RESTYLE |
                                  ELEMENT_HAS_PENDING_ANIMATION_ONLY_RESTYLE,

  
  ELEMENT_TYPE_SPECIFIC_BITS_OFFSET = NODE_TYPE_SPECIFIC_BITS_OFFSET + 6
};

#undef ELEMENT_FLAG_BIT


ASSERT_NODE_FLAGS_SPACE(ELEMENT_TYPE_SPECIFIC_BITS_OFFSET);

namespace mozilla {
class EventChainPostVisitor;
class EventChainPreVisitor;
class EventChainVisitor;
class EventListenerManager;
class EventStateManager;

namespace dom {

class AnimationPlayer;
class Link;
class UndoManager;
class DOMRect;
class DOMRectList;
class DestinationInsertionPointList;


#define NS_ELEMENT_IID \
{ 0xb0135f9d, 0xa476, 0x4711, \
  { 0x8b, 0xb9, 0xca, 0xe5, 0x2a, 0x05, 0xf9, 0xbe } }

class Element : public FragmentOrElement
{
public:
#ifdef MOZILLA_INTERNAL_API
  explicit Element(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo) :
    FragmentOrElement(aNodeInfo),
    mState(NS_EVENT_STATE_MOZ_READONLY)
  {
    NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::ELEMENT_NODE,
                      "Bad NodeType in aNodeInfo");
    SetIsElement();
  }
#endif 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ELEMENT_IID)

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  



  EventStates State() const
  {
    
    
    return mState;
  }

  









  void UpdateState(bool aNotify);
  
  


  void UpdateLinkState(EventStates aState);

  



  bool IsFullScreenAncestor() const {
    return mState.HasAtLeastOneOfStates(NS_EVENT_STATE_FULL_SCREEN_ANCESTOR |
                                        NS_EVENT_STATE_FULL_SCREEN);
  }

  



  EventStates StyleState() const
  {
    if (!HasLockedStyleStates()) {
      return mState;
    }
    return StyleStateFromLocks();
  }

  


  EventStates LockedStyleStates() const;

  


  void LockStyleStates(EventStates aStates);

  


  void UnlockStyleStates(EventStates aStates);

  


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

  inline Directionality GetDirectionality() const {
    if (HasFlag(NODE_HAS_DIRECTION_RTL)) {
      return eDir_RTL;
    }

    if (HasFlag(NODE_HAS_DIRECTION_LTR)) {
      return eDir_LTR;
    }

    return eDir_NotSet;
  }

  inline void SetDirectionality(Directionality aDir, bool aNotify) {
    UnsetFlags(NODE_ALL_DIRECTION_FLAGS);
    if (!aNotify) {
      RemoveStatesSilently(DIRECTION_STATES);
    }

    switch (aDir) {
      case (eDir_RTL):
        SetFlags(NODE_HAS_DIRECTION_RTL);
        if (!aNotify) {
          AddStatesSilently(NS_EVENT_STATE_RTL);
        }
        break;

      case(eDir_LTR):
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

  
  
  
  
  inline bool HasDirAuto() const {
    return (!HasFixedDir() &&
            (HasValidDir() || IsHTML(nsGkAtoms::bdi)));
  }

  Directionality GetComputedDirectionality() const;

protected:
  





  virtual EventStates IntrinsicState() const;

  





  void AddStatesSilently(EventStates aStates)
  {
    mState |= aStates;
  }

  





  void RemoveStatesSilently(EventStates aStates)
  {
    mState &= ~aStates;
  }

private:
  
  
  friend class mozilla::EventStateManager;
  friend class ::nsGlobalWindow;
  friend class ::nsFocusManager;

  
  friend class Link;

  void NotifyStateChange(EventStates aStates);

  void NotifyStyleStateChange(EventStates aStates);

  
  EventStates StyleStateFromLocks() const;

protected:
  
  
  
  virtual void AddStates(EventStates aStates)
  {
    NS_PRECONDITION(!aStates.HasAtLeastOneOfStates(INTRINSIC_STATES),
                    "Should only be adding ESM-managed states here");
    AddStatesSilently(aStates);
    NotifyStateChange(aStates);
  }
  virtual void RemoveStates(EventStates aStates)
  {
    NS_PRECONDITION(!aStates.HasAtLeastOneOfStates(INTRINSIC_STATES),
                    "Should only be removing ESM-managed states here");
    RemoveStatesSilently(aStates);
    NotifyStateChange(aStates);
  }
public:
  virtual void UpdateEditableState(bool aNotify) MOZ_OVERRIDE;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;

  









  already_AddRefed<mozilla::dom::NodeInfo>
  GetExistingAttrNameFromQName(const nsAString& aStr) const;

  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }

  















  bool MaybeCheckSameAttrVal(int32_t aNamespaceID, nsIAtom* aName,
                             nsIAtom* aPrefix,
                             const nsAttrValueOrString& aValue,
                             bool aNotify, nsAttrValue& aOldValue,
                             uint8_t* aModType, bool* aHasListeners);

  bool OnlyNotifySameValueSet(int32_t aNamespaceID, nsIAtom* aName,
                              nsIAtom* aPrefix,
                              const nsAttrValueOrString& aValue,
                              bool aNotify, nsAttrValue& aOldValue,
                              uint8_t* aModType, bool* aHasListeners);

  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName, nsIAtom* aPrefix,
                           const nsAString& aValue, bool aNotify) MOZ_OVERRIDE;
  nsresult SetParsedAttr(int32_t aNameSpaceID, nsIAtom* aName, nsIAtom* aPrefix,
                         nsAttrValue& aParsedValue, bool aNotify);
  
  
  bool GetAttr(int32_t aNameSpaceID, nsIAtom* aName,
               nsAString& aResult) const;
  inline bool HasAttr(int32_t aNameSpaceID, nsIAtom* aName) const;
  
  inline bool AttrValueIs(int32_t aNameSpaceID, nsIAtom* aName,
                          const nsAString& aValue,
                          nsCaseTreatment aCaseSensitive) const;
  inline bool AttrValueIs(int32_t aNameSpaceID, nsIAtom* aName,
                          nsIAtom* aValue,
                          nsCaseTreatment aCaseSensitive) const;
  virtual int32_t FindAttrValueIn(int32_t aNameSpaceID,
                                  nsIAtom* aName,
                                  AttrValuesArray* aValues,
                                  nsCaseTreatment aCaseSensitive) const MOZ_OVERRIDE;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify) MOZ_OVERRIDE;
  virtual const nsAttrName* GetAttrNameAt(uint32_t aIndex) const MOZ_OVERRIDE;
  virtual uint32_t GetAttrCount() const MOZ_OVERRIDE;
  virtual bool IsNodeOfType(uint32_t aFlags) const MOZ_OVERRIDE;

#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE
  {
    List(out, aIndent, EmptyCString());
  }
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const MOZ_OVERRIDE;
  void List(FILE* out, int32_t aIndent, const nsCString& aPrefix) const;
  void ListAttributes(FILE* out) const;
#endif

  void Describe(nsAString& aOutDescription) const MOZ_OVERRIDE;

  


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

  static nsIAtom*** HTMLSVGPropertiesToTraverseAndUnlink();

private:
  void DescribeAttribute(uint32_t index, nsAString& aOutDescription) const;

  static bool
  FindAttributeDependence(const nsIAtom* aAttribute,
                          const MappedAttributeEntry* const aMaps[],
                          uint32_t aMapCount);

protected:
  inline bool GetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                      DOMString& aResult) const
  {
    NS_ASSERTION(nullptr != aName, "must have attribute name");
    NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown,
                 "must have a real namespace ID!");
    MOZ_ASSERT(aResult.HasStringBuffer() && aResult.StringBufferLength() == 0,
               "Should have empty string coming in");
    const nsAttrValue* val = mAttrsAndChildren.GetAttr(aName, aNameSpaceID);
    if (val) {
      val->ToString(aResult);
      return true;
    }
    
    return false;
  }

public:
  bool HasAttrs() const { return mAttrsAndChildren.HasAttrs(); }

  inline bool GetAttr(const nsAString& aName, DOMString& aResult) const
  {
    MOZ_ASSERT(aResult.HasStringBuffer() && aResult.StringBufferLength() == 0,
               "Should have empty string coming in");
    const nsAttrValue* val = mAttrsAndChildren.GetAttr(aName);
    if (val) {
      val->ToString(aResult);
      return true;
    }
    
    return false;
  }

  void GetTagName(nsAString& aTagName) const
  {
    aTagName = NodeName();
  }
  void GetId(nsAString& aId) const
  {
    GetAttr(kNameSpaceID_None, nsGkAtoms::id, aId);
  }
  void GetId(DOMString& aId) const
  {
    GetAttr(kNameSpaceID_None, nsGkAtoms::id, aId);
  }
  void SetId(const nsAString& aId)
  {
    SetAttr(kNameSpaceID_None, nsGkAtoms::id, aId, true);
  }
  void GetClassName(nsAString& aClassName)
  {
    GetAttr(kNameSpaceID_None, nsGkAtoms::_class, aClassName);
  }
  void GetClassName(DOMString& aClassName)
  {
    GetAttr(kNameSpaceID_None, nsGkAtoms::_class, aClassName);
  }
  void SetClassName(const nsAString& aClassName)
  {
    SetAttr(kNameSpaceID_None, nsGkAtoms::_class, aClassName, true);
  }

  nsDOMTokenList* ClassList();
  nsDOMAttributeMap* Attributes()
  {
    nsDOMSlots* slots = DOMSlots();
    if (!slots->mAttributeMap) {
      slots->mAttributeMap = new nsDOMAttributeMap(this);
    }

    return slots->mAttributeMap;
  }
  void GetAttribute(const nsAString& aName, nsString& aReturn)
  {
    DOMString str;
    GetAttribute(aName, str);
    str.ToString(aReturn);
  }

  void GetAttribute(const nsAString& aName, DOMString& aReturn);
  void GetAttributeNS(const nsAString& aNamespaceURI,
                      const nsAString& aLocalName,
                      nsAString& aReturn);
  void SetAttribute(const nsAString& aName, const nsAString& aValue,
                    ErrorResult& aError);
  void SetAttributeNS(const nsAString& aNamespaceURI,
                      const nsAString& aLocalName,
                      const nsAString& aValue,
                      ErrorResult& aError);
  void RemoveAttribute(const nsAString& aName,
                       ErrorResult& aError);
  void RemoveAttributeNS(const nsAString& aNamespaceURI,
                         const nsAString& aLocalName,
                         ErrorResult& aError);
  bool HasAttribute(const nsAString& aName) const
  {
    return InternalGetExistingAttrNameFromQName(aName) != nullptr;
  }
  bool HasAttributeNS(const nsAString& aNamespaceURI,
                      const nsAString& aLocalName) const;
  bool Matches(const nsAString& aSelector,
               ErrorResult& aError);
  already_AddRefed<nsIHTMLCollection>
    GetElementsByTagName(const nsAString& aQualifiedName);
  already_AddRefed<nsIHTMLCollection>
    GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                           const nsAString& aLocalName,
                           ErrorResult& aError);
  already_AddRefed<nsIHTMLCollection>
    GetElementsByClassName(const nsAString& aClassNames);
  bool MozMatchesSelector(const nsAString& aSelector,
                          ErrorResult& aError)
  {
    return Matches(aSelector, aError);
  }
  void SetPointerCapture(int32_t aPointerId, ErrorResult& aError)
  {
    bool activeState = false;
    if (!nsIPresShell::GetPointerInfo(aPointerId, activeState)) {
      aError.Throw(NS_ERROR_DOM_INVALID_POINTER_ERR);
      return;
    }
    if (!activeState) {
      return;
    }
    nsIPresShell::SetPointerCapturingContent(aPointerId, this);
  }
  void ReleasePointerCapture(int32_t aPointerId, ErrorResult& aError)
  {
    bool activeState = false;
    if (!nsIPresShell::GetPointerInfo(aPointerId, activeState)) {
      aError.Throw(NS_ERROR_DOM_INVALID_POINTER_ERR);
      return;
    }

    
    
    if (nsIPresShell::GetPointerCapturingContent(aPointerId) == this) {
      nsIPresShell::ReleasePointerCapturingContent(aPointerId, this);
    }
  }
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
  void MozRequestPointerLock();
  Attr* GetAttributeNode(const nsAString& aName);
  already_AddRefed<Attr> SetAttributeNode(Attr& aNewAttr,
                                          ErrorResult& aError);
  already_AddRefed<Attr> RemoveAttributeNode(Attr& aOldAttr,
                                             ErrorResult& aError);
  Attr* GetAttributeNodeNS(const nsAString& aNamespaceURI,
                           const nsAString& aLocalName);
  already_AddRefed<Attr> SetAttributeNodeNS(Attr& aNewAttr,
                                            ErrorResult& aError);

  already_AddRefed<DOMRectList> GetClientRects();
  already_AddRefed<DOMRect> GetBoundingClientRect();

  already_AddRefed<ShadowRoot> CreateShadowRoot(ErrorResult& aError);
  already_AddRefed<DestinationInsertionPointList> GetDestinationInsertionPoints();

  void ScrollIntoView();
  void ScrollIntoView(bool aTop, const ScrollOptions &aOptions);
  int32_t ScrollTop()
  {
    nsIScrollableFrame* sf = GetScrollFrame();
    return sf ? sf->GetScrollPositionCSSPixels().y : 0;
  }
  void SetScrollTop(int32_t aScrollTop)
  {
    nsIScrollableFrame* sf = GetScrollFrame();
    if (sf) {
      sf->ScrollToCSSPixels(CSSIntPoint(sf->GetScrollPositionCSSPixels().x,
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
      sf->ScrollToCSSPixels(CSSIntPoint(aScrollLeft,
                                        sf->GetScrollPositionCSSPixels().y));
    }
  }
  



  bool ScrollByNoFlush(int32_t aDx, int32_t aDy);
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

  virtual already_AddRefed<UndoManager> GetUndoManager()
  {
    return nullptr;
  }

  virtual bool UndoScope()
  {
    return false;
  }

  virtual void SetUndoScope(bool aUndoScope, ErrorResult& aError)
  {
  }

  void GetAnimationPlayers(nsTArray<nsRefPtr<AnimationPlayer> >& aPlayers);

  NS_IMETHOD GetInnerHTML(nsAString& aInnerHTML);
  virtual void SetInnerHTML(const nsAString& aInnerHTML, ErrorResult& aError);
  void GetOuterHTML(nsAString& aOuterHTML);
  void SetOuterHTML(const nsAString& aOuterHTML, ErrorResult& aError);
  void InsertAdjacentHTML(const nsAString& aPosition, const nsAString& aText,
                          ErrorResult& aError);

  

  






  nsresult SetEventHandler(nsIAtom* aEventName,
                           const nsAString& aValue,
                           bool aDefer = true);

  


  nsresult LeaveLink(nsPresContext* aPresContext);

  static bool ShouldBlur(nsIContent *aContent);

  









  static nsresult DispatchClickEvent(nsPresContext* aPresContext,
                                     WidgetInputEvent* aSourceEvent,
                                     nsIContent* aTarget,
                                     bool aFullDispatch,
                                     const EventFlags* aFlags,
                                     nsEventStatus* aStatus);

  






  using nsIContent::DispatchEvent;
  static nsresult DispatchEvent(nsPresContext* aPresContext,
                                WidgetEvent* aEvent,
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

  virtual void NodeInfoChanged(mozilla::dom::NodeInfo* aOldNodeInfo)
  {
  }

  




  static void ParseCORSValue(const nsAString& aValue, nsAttrValue& aResult);

  


  static CORSMode StringToCORSMode(const nsAString& aValue);
  
  



  static CORSMode AttrValueToCORSMode(const nsAttrValue* aValue);

  
  
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
  void GetClassList(nsISupports** aClassList);

  virtual JSObject* WrapObject(JSContext *aCx) MOZ_FINAL MOZ_OVERRIDE;

  


  nsIEditor* GetEditorInternal();

  







  bool GetBoolAttr(nsIAtom* aAttr) const
  {
    return HasAttr(kNameSpaceID_None, aAttr);
  }

  







  nsresult SetBoolAttr(nsIAtom* aAttr, bool aValue);

  








  void GetEnumAttr(nsIAtom* aAttr,
                   const char* aDefault,
                   nsAString& aResult) const;

  










  void GetEnumAttr(nsIAtom* aAttr,
                   const char* aDefaultMissing,
                   const char* aDefaultInvalid,
                   nsAString& aResult) const;

  













  float FontSizeInflation();

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

  



  virtual EventListenerManager*
    GetEventListenerManagerForAttr(nsIAtom* aAttrName, bool* aDefer);

  


  virtual const nsAttrName* InternalGetExistingAttrNameFromQName(const nsAString& aStr) const;

  nsIFrame* GetStyledFrame();

  virtual Element* GetNameSpaceElement()
  {
    return this;
  }

  Attr* GetAttributeNodeNSInternal(const nsAString& aNamespaceURI,
                                   const nsAString& aLocalName);

  inline void RegisterActivityObserver();
  inline void UnregisterActivityObserver();

  


  void AddToIdTable(nsIAtom* aId);
  void RemoveFromIdTable();

  




  







  bool CheckHandleEventForLinksPrecondition(EventChainVisitor& aVisitor,
                                            nsIURI** aURI) const;

  


  nsresult PreHandleEventForLinks(EventChainPreVisitor& aVisitor);

  


  nsresult PostHandleEventForLinks(EventChainPostVisitor& aVisitor);

  









  virtual void GetLinkTarget(nsAString& aTarget);

  nsDOMSettableTokenList* GetTokenList(nsIAtom* aAtom);
  void GetTokenList(nsIAtom* aAtom, nsIVariant** aResult);
  nsresult SetTokenList(nsIAtom* aAtom, nsIVariant* aValue);

private:
  



  nsRect GetClientAreaRect();

  nsIScrollableFrame* GetScrollFrame(nsIFrame **aStyledFrame = nullptr,
                                     bool aFlushLayout = true);

  
  EventStates mState;
};

class DestinationInsertionPointList : public nsINodeList
{
public:
  explicit DestinationInsertionPointList(Element* aElement);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DestinationInsertionPointList)

  
  NS_DECL_NSIDOMNODELIST

  
  virtual nsIContent* Item(uint32_t aIndex);
  virtual int32_t IndexOf(nsIContent* aContent);
  virtual nsINode* GetParentObject() { return mParent; }
  virtual uint32_t Length() const;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
protected:
  virtual ~DestinationInsertionPointList();

  nsRefPtr<Element> mParent;
  nsCOMArray<nsIContent> mDestinationPoints;
};

NS_DEFINE_STATIC_IID_ACCESSOR(Element, NS_ELEMENT_IID)

inline bool
Element::HasAttr(int32_t aNameSpaceID, nsIAtom* aName) const
{
  NS_ASSERTION(nullptr != aName, "must have attribute name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown,
               "must have a real namespace ID!");

  return mAttrsAndChildren.IndexOfAttr(aName, aNameSpaceID) >= 0;
}

inline bool
Element::AttrValueIs(int32_t aNameSpaceID,
                     nsIAtom* aName,
                     const nsAString& aValue,
                     nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");

  const nsAttrValue* val = mAttrsAndChildren.GetAttr(aName, aNameSpaceID);
  return val && val->Equals(aValue, aCaseSensitive);
}

inline bool
Element::AttrValueIs(int32_t aNameSpaceID,
                     nsIAtom* aName,
                     nsIAtom* aValue,
                     nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");
  NS_ASSERTION(aValue, "Null value atom");

  const nsAttrValue* val = mAttrsAndChildren.GetAttr(aName, aNameSpaceID);
  return val && val->Equals(aValue, aCaseSensitive);
}

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
  return IsElement() && AsElement()->HasAttrs();
}





#define NS_IMPL_ELEMENT_CLONE(_elementName)                                 \
nsresult                                                                    \
_elementName::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const \
{                                                                           \
  *aResult = nullptr;                                                       \
  already_AddRefed<mozilla::dom::NodeInfo> ni =                             \
    nsRefPtr<mozilla::dom::NodeInfo>(aNodeInfo).forget();                   \
  _elementName *it = new _elementName(ni);                                  \
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
_elementName::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const \
{                                                                           \
  *aResult = nullptr;                                                       \
  already_AddRefed<mozilla::dom::NodeInfo> ni =                             \
    nsRefPtr<mozilla::dom::NodeInfo>(aNodeInfo).forget();                   \
  _elementName *it = new _elementName(ni);                                  \
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






#define NS_IMPL_BOOL_ATTR(_class, _method, _atom)                     \
  NS_IMETHODIMP                                                       \
  _class::Get##_method(bool* aValue)                                  \
  {                                                                   \
    *aValue = GetBoolAttr(nsGkAtoms::_atom);                          \
    return NS_OK;                                                     \
  }                                                                   \
  NS_IMETHODIMP                                                       \
  _class::Set##_method(bool aValue)                                   \
  {                                                                   \
    return SetBoolAttr(nsGkAtoms::_atom, aValue);                     \
  }

#define NS_FORWARD_NSIDOMELEMENT_TO_GENERIC                                   \
typedef mozilla::dom::Element Element;                                        \
NS_IMETHOD GetTagName(nsAString& aTagName) MOZ_FINAL                          \
{                                                                             \
  Element::GetTagName(aTagName);                                              \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetId(nsAString& aId) MOZ_FINAL                                    \
{                                                                             \
  Element::GetId(aId);                                                        \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetId(const nsAString& aId) MOZ_FINAL                              \
{                                                                             \
  Element::SetId(aId);                                                        \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClassName(nsAString& aClassName) MOZ_FINAL                      \
{                                                                             \
  Element::GetClassName(aClassName);                                          \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetClassName(const nsAString& aClassName) MOZ_FINAL                \
{                                                                             \
  Element::SetClassName(aClassName);                                          \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClassList(nsISupports** aClassList) MOZ_FINAL                   \
{                                                                             \
  Element::GetClassList(aClassList);                                          \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetAttributes(nsIDOMMozNamedAttrMap** aAttributes) MOZ_FINAL       \
{                                                                             \
  NS_ADDREF(*aAttributes = Attributes());                                     \
  return NS_OK;                                                               \
}                                                                             \
using Element::GetAttribute;                                                  \
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
  Element::GetAttributeNS(namespaceURI, localName, _retval);                  \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetAttribute(const nsAString& name,                                \
                        const nsAString& value)                               \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  Element::SetAttribute(name, value, rv);                                     \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD SetAttributeNS(const nsAString& namespaceURI,                      \
                          const nsAString& qualifiedName,                     \
                          const nsAString& value) MOZ_FINAL                   \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  Element::SetAttributeNS(namespaceURI, qualifiedName, value, rv);            \
  return rv.ErrorCode();                                                      \
}                                                                             \
using Element::RemoveAttribute;                                               \
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
  Element::RemoveAttributeNS(namespaceURI, localName, rv);                    \
  return rv.ErrorCode();                                                      \
}                                                                             \
using Element::HasAttribute;                                                  \
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
  *_retval = Element::HasAttributeNS(namespaceURI, localName);                \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetAttributeNode(const nsAString& name,                            \
                            nsIDOMAttr** _retval) MOZ_FINAL                   \
{                                                                             \
  NS_IF_ADDREF(*_retval = Element::GetAttributeNode(name));                   \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetAttributeNode(nsIDOMAttr* newAttr,                              \
                            nsIDOMAttr** _retval) MOZ_FINAL                   \
{                                                                             \
  if (!newAttr) {                                                             \
    return NS_ERROR_INVALID_POINTER;                                          \
  }                                                                           \
  mozilla::ErrorResult rv;                                                    \
  mozilla::dom::Attr* attr = static_cast<mozilla::dom::Attr*>(newAttr);       \
  *_retval = Element::SetAttributeNode(*attr, rv).take();                     \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD RemoveAttributeNode(nsIDOMAttr* oldAttr,                           \
                               nsIDOMAttr** _retval) MOZ_FINAL                \
{                                                                             \
  if (!oldAttr) {                                                             \
    return NS_ERROR_INVALID_POINTER;                                          \
  }                                                                           \
  mozilla::ErrorResult rv;                                                    \
  mozilla::dom::Attr* attr = static_cast<mozilla::dom::Attr*>(oldAttr);       \
  *_retval = Element::RemoveAttributeNode(*attr, rv).take();                  \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD GetAttributeNodeNS(const nsAString& namespaceURI,                  \
                              const nsAString& localName,                     \
                              nsIDOMAttr** _retval) MOZ_FINAL                 \
{                                                                             \
  NS_IF_ADDREF(*_retval = Element::GetAttributeNodeNS(namespaceURI,           \
                                                      localName));            \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetAttributeNodeNS(nsIDOMAttr* newAttr,                            \
                              nsIDOMAttr** _retval) MOZ_FINAL                 \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  mozilla::dom::Attr* attr = static_cast<mozilla::dom::Attr*>(newAttr);       \
  *_retval = Element::SetAttributeNodeNS(*attr, rv).take();                   \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD GetElementsByTagName(const nsAString& name,                        \
                                nsIDOMHTMLCollection** _retval) MOZ_FINAL     \
{                                                                             \
  Element::GetElementsByTagName(name, _retval);                               \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetElementsByTagNameNS(const nsAString& namespaceURI,              \
                                  const nsAString& localName,                 \
                                  nsIDOMHTMLCollection** _retval) MOZ_FINAL   \
{                                                                             \
  return Element::GetElementsByTagNameNS(namespaceURI, localName,             \
                                         _retval);                            \
}                                                                             \
NS_IMETHOD GetElementsByClassName(const nsAString& classes,                   \
                                  nsIDOMHTMLCollection** _retval) MOZ_FINAL   \
{                                                                             \
  return Element::GetElementsByClassName(classes, _retval);                   \
}                                                                             \
NS_IMETHOD GetChildElements(nsIDOMNodeList** aChildElements) MOZ_FINAL        \
{                                                                             \
  nsIHTMLCollection* list = FragmentOrElement::Children();                    \
  return CallQueryInterface(list, aChildElements);                            \
}                                                                             \
NS_IMETHOD GetFirstElementChild(nsIDOMElement** aFirstElementChild) MOZ_FINAL \
{                                                                             \
  Element* element = Element::GetFirstElementChild();                         \
  if (!element) {                                                             \
    *aFirstElementChild = nullptr;                                            \
    return NS_OK;                                                             \
  }                                                                           \
  return CallQueryInterface(element, aFirstElementChild);                     \
}                                                                             \
NS_IMETHOD GetLastElementChild(nsIDOMElement** aLastElementChild) MOZ_FINAL   \
{                                                                             \
  Element* element = Element::GetLastElementChild();                          \
  if (!element) {                                                             \
    *aLastElementChild = nullptr;                                             \
    return NS_OK;                                                             \
  }                                                                           \
  return CallQueryInterface(element, aLastElementChild);                      \
}                                                                             \
NS_IMETHOD GetPreviousElementSibling(nsIDOMElement** aPreviousElementSibling) \
  MOZ_FINAL                                                                   \
{                                                                             \
  Element* element = Element::GetPreviousElementSibling();                    \
  if (!element) {                                                             \
    *aPreviousElementSibling = nullptr;                                       \
    return NS_OK;                                                             \
  }                                                                           \
  return CallQueryInterface(element, aPreviousElementSibling);                \
}                                                                             \
NS_IMETHOD GetNextElementSibling(nsIDOMElement** aNextElementSibling)         \
  MOZ_FINAL                                                                   \
{                                                                             \
  Element* element = Element::GetNextElementSibling();                        \
  if (!element) {                                                             \
    *aNextElementSibling = nullptr;                                           \
    return NS_OK;                                                             \
  }                                                                           \
  return CallQueryInterface(element, aNextElementSibling);                    \
}                                                                             \
NS_IMETHOD GetChildElementCount(uint32_t* aChildElementCount) MOZ_FINAL       \
{                                                                             \
  *aChildElementCount = Element::ChildElementCount();                         \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD MozRemove() MOZ_FINAL                                              \
{                                                                             \
  nsINode::Remove();                                                          \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClientRects(nsIDOMClientRectList** _retval) MOZ_FINAL           \
{                                                                             \
  *_retval = Element::GetClientRects().take();                                \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetBoundingClientRect(nsIDOMClientRect** _retval) MOZ_FINAL        \
{                                                                             \
  *_retval = Element::GetBoundingClientRect().take();                         \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollTop(int32_t* aScrollTop) MOZ_FINAL                        \
{                                                                             \
  *aScrollTop = Element::ScrollTop();                                         \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetScrollTop(int32_t aScrollTop) MOZ_FINAL                         \
{                                                                             \
  Element::SetScrollTop(aScrollTop);                                          \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollLeft(int32_t* aScrollLeft) MOZ_FINAL                      \
{                                                                             \
  *aScrollLeft = Element::ScrollLeft();                                       \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD SetScrollLeft(int32_t aScrollLeft) MOZ_FINAL                       \
{                                                                             \
  Element::SetScrollLeft(aScrollLeft);                                        \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollWidth(int32_t* aScrollWidth) MOZ_FINAL                    \
{                                                                             \
  *aScrollWidth = Element::ScrollWidth();                                     \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollHeight(int32_t* aScrollHeight) MOZ_FINAL                  \
{                                                                             \
  *aScrollHeight = Element::ScrollHeight();                                   \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClientTop(int32_t* aClientTop) MOZ_FINAL                        \
{                                                                             \
  *aClientTop = Element::ClientTop();                                         \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClientLeft(int32_t* aClientLeft) MOZ_FINAL                      \
{                                                                             \
  *aClientLeft = Element::ClientLeft();                                       \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClientWidth(int32_t* aClientWidth) MOZ_FINAL                    \
{                                                                             \
  *aClientWidth = Element::ClientWidth();                                     \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetClientHeight(int32_t* aClientHeight) MOZ_FINAL                  \
{                                                                             \
  *aClientHeight = Element::ClientHeight();                                   \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollLeftMax(int32_t* aScrollLeftMax) MOZ_FINAL                \
{                                                                             \
  *aScrollLeftMax = Element::ScrollLeftMax();                                 \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD GetScrollTopMax(int32_t* aScrollTopMax) MOZ_FINAL                  \
{                                                                             \
  *aScrollTopMax = Element::ScrollTopMax();                                   \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD MozMatchesSelector(const nsAString& selector,                      \
                              bool* _retval) MOZ_FINAL                        \
{                                                                             \
  mozilla::ErrorResult rv;                                                    \
  *_retval = Element::MozMatchesSelector(selector, rv);                       \
  return rv.ErrorCode();                                                      \
}                                                                             \
NS_IMETHOD SetCapture(bool retargetToElement) MOZ_FINAL                       \
{                                                                             \
  Element::SetCapture(retargetToElement);                                     \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD ReleaseCapture(void) MOZ_FINAL                                     \
{                                                                             \
  Element::ReleaseCapture();                                                  \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD MozRequestFullScreen(void) MOZ_FINAL                               \
{                                                                             \
  Element::MozRequestFullScreen();                                            \
  return NS_OK;                                                               \
}                                                                             \
NS_IMETHOD MozRequestPointerLock(void) MOZ_FINAL                              \
{                                                                             \
  Element::MozRequestPointerLock();                                           \
  return NS_OK;                                                               \
}                                                                             \
using nsINode::QuerySelector;                                                 \
NS_IMETHOD QuerySelector(const nsAString& aSelector,                          \
                         nsIDOMElement **aReturn) MOZ_FINAL                   \
{                                                                             \
  return nsINode::QuerySelector(aSelector, aReturn);                          \
}                                                                             \
using nsINode::QuerySelectorAll;                                              \
NS_IMETHOD QuerySelectorAll(const nsAString& aSelector,                       \
                            nsIDOMNodeList **aReturn) MOZ_FINAL               \
{                                                                             \
  return nsINode::QuerySelectorAll(aSelector, aReturn);                       \
}

#endif 
