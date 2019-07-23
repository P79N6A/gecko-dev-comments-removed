



































 
#ifndef MoreTestingAtoms_h_
#define MoreTestingAtoms_h_

#include "nsIAtom.h"

class MoreTestingAtoms {
  public:
    static void AddRefAtoms();
#define MORE_TESTING_ATOM(_name, _value) static nsIAtom* _name;
#include "MoreTestingAtomList.h"
#undef MORE_TESTING_ATOM
};

#endif 
