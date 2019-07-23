



































 
#ifndef TestingAtoms_h_
#define TestingAtoms_h_

#include "nsIAtom.h"

class TestingAtoms {
  public:
    static void AddRefAtoms();
#define TESTING_ATOM(_name, _value) static nsIAtom* _name;
#include "TestingAtomList.h"
#undef TESTING_ATOM
};

#endif 
