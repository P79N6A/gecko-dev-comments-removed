










































#include "nsGkAtoms.h"
#include "nsStaticAtom.h"
 

#define GK_ATOM(_name, _value) nsIAtom* nsGkAtoms::_name;
#include "nsGkAtomList.h"
#undef GK_ATOM

static const nsStaticAtom GkAtoms_info[] = {
#define GK_ATOM(name_, value_) { value_, &nsGkAtoms::name_ },
#include "nsGkAtomList.h"
#undef GK_ATOM
};

void nsGkAtoms::AddRefAtoms()
{
  NS_RegisterStaticAtoms(GkAtoms_info, NS_ARRAY_LENGTH(GkAtoms_info));
}

