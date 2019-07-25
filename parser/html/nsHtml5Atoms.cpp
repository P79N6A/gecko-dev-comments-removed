











































#include "mozilla/Util.h"

#include "nsHtml5Atoms.h"
#include "nsStaticAtom.h"

using namespace mozilla;


#define HTML5_ATOM(_name, _value) nsIAtom* nsHtml5Atoms::_name;
#include "nsHtml5AtomList.h"
#undef HTML5_ATOM

#define HTML5_ATOM(name_, value_) NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "nsHtml5AtomList.h"
#undef HTML5_ATOM

static const nsStaticAtom Html5Atoms_info[] = {
#define HTML5_ATOM(name_, value_) NS_STATIC_ATOM(name_##_buffer, &nsHtml5Atoms::name_),
#include "nsHtml5AtomList.h"
#undef HTML5_ATOM
};

void nsHtml5Atoms::AddRefAtoms()
{
  NS_RegisterStaticAtoms(Html5Atoms_info, ArrayLength(Html5Atoms_info));
}
