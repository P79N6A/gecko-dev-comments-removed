





#ifndef mozilla_dom_Element_h__
#define mozilla_dom_Element_h__

#include "mozilla/dom/FragmentOrElement.h" 
#include "nsChangeHint.h"                  
#include "nsEventStates.h"                 
#include "mozilla/dom/DirectionalityUtils.h"

class nsEventStateManager;
class nsFocusManager;
class nsGlobalWindow;
class nsICSSDeclaration;
class nsISMILAttr;


enum {
  
  ELEMENT_HAS_PENDING_RESTYLE     = (1 << NODE_TYPE_SPECIFIC_BITS_OFFSET),

  
  
  
  ELEMENT_IS_POTENTIAL_RESTYLE_ROOT =
    (1 << (NODE_TYPE_SPECIFIC_BITS_OFFSET + 1)),

  
  ELEMENT_HAS_PENDING_ANIMATION_RESTYLE =
    (1 << (NODE_TYPE_SPECIFIC_BITS_OFFSET + 2)),

  
  
  
  ELEMENT_IS_POTENTIAL_ANIMATION_RESTYLE_ROOT =
    (1 << (NODE_TYPE_SPECIFIC_BITS_OFFSET + 3)),

  
  ELEMENT_ALL_RESTYLE_FLAGS = ELEMENT_HAS_PENDING_RESTYLE |
                              ELEMENT_IS_POTENTIAL_RESTYLE_ROOT |
                              ELEMENT_HAS_PENDING_ANIMATION_RESTYLE |
                              ELEMENT_IS_POTENTIAL_ANIMATION_RESTYLE_ROOT,

  
  ELEMENT_PENDING_RESTYLE_FLAGS = ELEMENT_HAS_PENDING_RESTYLE |
                                  ELEMENT_HAS_PENDING_ANIMATION_RESTYLE,

  
  ELEMENT_TYPE_SPECIFIC_BITS_OFFSET = NODE_TYPE_SPECIFIC_BITS_OFFSET + 4
};

namespace mozilla {
namespace dom {

class Link;


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
  {}
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

  


  virtual css::StyleRule* GetInlineStyleRule() = 0;

  



  virtual nsresult SetInlineStyleRule(css::StyleRule* aStyleRule,
                                      const nsAString* aSerialized,
                                      bool aNotify) = 0;

  



  virtual css::StyleRule* GetSMILOverrideStyleRule() = 0;

  




  virtual nsresult SetSMILOverrideStyleRule(css::StyleRule* aStyleRule,
                                            bool aNotify) = 0;

  





  virtual nsISMILAttr* GetAnimatedAttr(int32_t aNamespaceID, nsIAtom* aName) = 0;

  







  virtual nsICSSDeclaration* GetSMILOverrideStyle() = 0;

  


  virtual bool IsLabelable() const = 0;

  






  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const = 0;

  





  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const = 0;

  




  virtual nsIAtom *GetClassAttributeName() const = 0;

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

#endif 
