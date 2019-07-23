



































 






#ifndef nsHtml5Atoms_h___
#define nsHtml5Atoms_h___

#include "nsIAtom.h"

class nsHtml5Atoms {
  public:
    static void AddRefAtoms();
    




#define HTML5_ATOM(_name, _value) static nsIAtom* _name;
#include "nsHtml5AtomList.h"
#undef HTML5_ATOM
};

#endif 
