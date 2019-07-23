




































#include "TestingAtoms.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"


#define TESTING_ATOM(_name, _value) nsIAtom* TestingAtoms::_name;
#include "TestingAtomList.h"
#undef TESTING_ATOM

#define TESTING_ATOM(name_, value_) NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "TestingAtomList.h"
#undef TESTING_ATOM

static const nsStaticAtom TestingAtoms_info[] = {
#define TESTING_ATOM(name_, value_) NS_STATIC_ATOM(name_##_buffer, &TestingAtoms::name_),
#include "TestingAtomList.h"
#undef TESTING_ATOM
};

void TestingAtoms::AddRefAtoms()
{
  NS_RegisterStaticAtoms(TestingAtoms_info, NS_ARRAY_LENGTH(TestingAtoms_info));
}
