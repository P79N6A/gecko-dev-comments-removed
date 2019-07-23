






































#include "nsMemory.h"
#include "nsStaticAtom.h"
#include "nsEditProperty.h"


#define EDITOR_ATOM(name_, value_) nsIAtom* nsEditProperty::name_ = 0;
#include "nsEditPropertyAtomList.h"
#undef EDITOR_ATOM






















void
nsEditProperty::RegisterAtoms()
{
  
  static const nsStaticAtom property_atoms[] = {
#define EDITOR_ATOM(name_, value_) { value_, &name_ },
#include "nsEditPropertyAtomList.h"
#undef EDITOR_ATOM
  };
  
  NS_RegisterStaticAtoms(property_atoms, NS_ARRAY_LENGTH(property_atoms));
}
