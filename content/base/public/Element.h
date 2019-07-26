





#ifndef mozilla_dom_Element_h__
#define mozilla_dom_Element_h__

#include "nsIContent.h"
#include "nsEventStates.h"

class nsEventStateManager;
class nsGlobalWindow;
class nsFocusManager;
class nsICSSDeclaration;


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
{ 0x8493dd61, 0x4d59, 0x410a, \
  { 0x97, 0x40, 0xd0, 0xa0, 0xc2, 0x03, 0xff, 0x52 } }

class Element : public nsIContent
{
public:
#ifdef MOZILLA_INTERNAL_API
  Element(already_AddRefed<nsINodeInfo> aNodeInfo) :
    nsIContent(aNodeInfo),
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
  };

  


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

  





  virtual nsISMILAttr* GetAnimatedAttr(PRInt32 aNamespaceID, nsIAtom* aName) = 0;

  







  virtual nsICSSDeclaration* GetSMILOverrideStyle() = 0;

  


  virtual bool IsLabelable() const = 0;

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
