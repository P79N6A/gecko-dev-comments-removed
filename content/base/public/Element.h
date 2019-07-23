






































#ifndef mozilla_dom_Element_h__
#define mozilla_dom_Element_h__

#include "nsIContent.h"

namespace mozilla {
namespace dom {

class Element : public nsIContent
{
public:
#ifdef MOZILLA_INTERNAL_API
  Element(nsINodeInfo* aNodeInfo) : nsIContent(aNodeInfo) {}
#endif 
};

} 
} 

inline mozilla::dom::Element* nsINode::AsElement() {
  NS_ASSERTION(IsElement(), "Not an element?");
  return static_cast<mozilla::dom::Element*>(this);
}

#endif 
