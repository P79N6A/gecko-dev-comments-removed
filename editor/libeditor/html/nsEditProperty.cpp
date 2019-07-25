






































#include "mozilla/Util.h"

#include "nsMemory.h"
#include "nsStaticAtom.h"
#include "nsEditProperty.h"

using namespace mozilla;

#define EDITOR_ATOM(name_, value_) nsIAtom* nsEditProperty::name_ = 0;
#include "nsEditPropertyAtomList.h"
#undef EDITOR_ATOM























#define EDITOR_ATOM(name_, value_) NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "nsEditPropertyAtomList.h"
#undef EDITOR_ATOM

void
nsEditProperty::RegisterAtoms()
{
  
  static const nsStaticAtom property_atoms[] = {
#define EDITOR_ATOM(name_, value_) NS_STATIC_ATOM(name_##_buffer, &name_),
#include "nsEditPropertyAtomList.h"
#undef EDITOR_ATOM
  };
  
  NS_RegisterStaticAtoms(property_atoms, ArrayLength(property_atoms));
}
