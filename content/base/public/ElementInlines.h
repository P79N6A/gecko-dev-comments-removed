





#ifndef mozilla_dom_ElementInlines_h
#define mozilla_dom_ElementInlines_h

#include "mozilla/dom/Element.h"
#include "nsIDocument.h"

namespace mozilla {
namespace dom {

inline void
Element::MozRequestPointerLock()
{
  OwnerDoc()->RequestPointerLock(this);
}

inline void
Element::RegisterFreezableElement()
{
  OwnerDoc()->RegisterFreezableElement(this);
}

inline void
Element::UnregisterFreezableElement()
{
  OwnerDoc()->UnregisterFreezableElement(this);
}

}
}

#endif 
