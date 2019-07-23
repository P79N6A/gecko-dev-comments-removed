











































#include "nsHtml5Atoms.h"
#include "nsStaticAtom.h"


#define HTML5_ATOM(_name, _value) nsIAtom* nsHtml5Atoms::_name;
#include "nsHtml5AtomList.h"
#undef HTML5_ATOM

static const nsStaticAtom Html5Atoms_info[] = {
#define HTML5_ATOM(name_, value_) { value_, &nsHtml5Atoms::name_ },
#include "nsHtml5AtomList.h"
#undef HTML5_ATOM
};

void nsHtml5Atoms::AddRefAtoms()
{
  NS_RegisterStaticAtoms(Html5Atoms_info, NS_ARRAY_LENGTH(Html5Atoms_info));
}

