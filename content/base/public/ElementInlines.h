





#ifndef mozilla_dom_ElementInlines_h
#define mozilla_dom_ElementInlines_h

#include "mozilla/dom/Element.h"
#include "nsIDocument.h"

namespace mozilla {
namespace dom {

inline void
Element::RegisterActivityObserver()
{
  OwnerDoc()->RegisterActivityObserver(this);
}

inline void
Element::UnregisterActivityObserver()
{
  OwnerDoc()->UnregisterActivityObserver(this);
}

}
}

#endif 
