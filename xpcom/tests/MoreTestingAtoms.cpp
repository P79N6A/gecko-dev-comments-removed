




































#include "MoreTestingAtoms.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"


#define MORE_TESTING_ATOM(_name, _value) nsIAtom* MoreTestingAtoms::_name;
#include "MoreTestingAtomList.h"
#undef MORE_TESTING_ATOM

static const nsStaticAtom MoreTestingAtoms_info[] = {

#define MORE_TESTING_ATOM(name_, value_) { value_, &MoreTestingAtoms::name_ },
#include "MoreTestingAtomList.h"
#undef MORE_TESTING_ATOM
};

void MoreTestingAtoms::AddRefAtoms()
{
  NS_RegisterStaticAtoms(MoreTestingAtoms_info, 
                         NS_ARRAY_LENGTH(MoreTestingAtoms_info));
}
