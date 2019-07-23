





































#include "nsAccessibilityAtoms.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"


#define ACCESSIBILITY_ATOM(_name, _value) nsIAtom* nsAccessibilityAtoms::_name;
#include "nsAccessibilityAtomList.h"
#undef ACCESSIBILITY_ATOM

#define ACCESSIBILITY_ATOM(name_, value_) NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "nsAccessibilityAtomList.h"
#undef ACCESSIBILITY_ATOM

static const nsStaticAtom atomInfo[] = {
#define ACCESSIBILITY_ATOM(name_, value_) NS_STATIC_ATOM(name_##_buffer, &nsAccessibilityAtoms::name_),
#include "nsAccessibilityAtomList.h"
#undef ACCESSIBILITY_ATOM
};

void nsAccessibilityAtoms::AddRefAtoms()
{
  NS_RegisterStaticAtoms(atomInfo, NS_ARRAY_LENGTH(atomInfo));
}
 
