










#ifndef nsGkAtoms_h___
#define nsGkAtoms_h___

class nsIAtom;

class nsGkAtoms {
public:

  static void AddRefAtoms();

  






#define GK_ATOM(_name, _value) static nsIAtom* _name;
#include "nsGkAtomList.h"
#undef GK_ATOM
};

#endif 
