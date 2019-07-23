



































#ifndef nsAccessibilityAtoms_h___
#define nsAccessibilityAtoms_h___

#include "nsIAtom.h"









class nsAccessibilityAtoms {
public:

  static void AddRefAtoms();
  static void ReleaseAtoms();

  






#define ACCESSIBILITY_ATOM(_name, _value) static nsIAtom* _name;
#include "nsAccessibilityAtomList.h"
#undef ACCESSIBILITY_ATOM
};

#endif 
