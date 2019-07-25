






































#ifndef mozilla_dom_Element_h__
#define mozilla_dom_Element_h__

#include "nsIContent.h"
#include "nsEventStates.h"
#include "nsDOMMemoryReporter.h"

class nsEventStateManager;
class nsGlobalWindow;
class nsFocusManager;


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

class Element : public nsIContent
{
public:
#ifdef MOZILLA_INTERNAL_API
  Element(already_AddRefed<nsINodeInfo> aNodeInfo) :
    nsIContent(aNodeInfo),
    mState(NS_EVENT_STATE_MOZ_READONLY)
  {}
#endif 

  NS_DECL_AND_IMPL_DOM_MEMORY_REPORTER_SIZEOF(Element, nsIContent)

  



  nsEventStates State() const {
    
    
    return mState;
  }

  








  virtual void RequestLinkStateUpdate();

  









  void UpdateState(bool aNotify);

protected:
  





  virtual nsEventStates IntrinsicState() const;

  


  void UpdateLinkState(nsEventStates aState);

  





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

} 
} 

inline mozilla::dom::Element* nsINode::AsElement() {
  NS_ASSERTION(IsElement(), "Not an element?");
  return static_cast<mozilla::dom::Element*>(this);
}

#endif 
