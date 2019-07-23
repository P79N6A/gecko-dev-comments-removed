










































#ifndef nsGkAtoms_h___
#define nsGkAtoms_h___

#include "nsIAtom.h"

class nsGkAtoms {
public:

  static void AddRefAtoms();

  






#define GK_ATOM(_name, _value) static nsIAtom* _name;
#include "nsGkAtomList.h"
#undef GK_ATOM
};

#endif 
