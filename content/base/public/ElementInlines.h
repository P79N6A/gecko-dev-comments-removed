





#ifndef mozilla_dom_ElementInlines_h
#define mozilla_dom_ElementInlines_h

#include "mozilla/dom/Element.h"
#include "nsIDocument.h"

namespace mozilla {
namespace dom {

inline void
Element::AddToIdTable(nsIAtom* aId)
{
  NS_ASSERTION(HasID(), "Node doesn't have an ID?");
  nsIDocument* doc = GetCurrentDoc();
  if (doc && (!IsInAnonymousSubtree() || doc->IsXUL())) {
    doc->AddToIdTable(this, aId);
  }
}

inline void
Element::RemoveFromIdTable()
{
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
