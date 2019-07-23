



































#ifndef GFX_ATOMS_H
#define GFX_ATOMS_H

#include "prtypes.h"
#include "nsIAtom.h"

class gfxAtoms {
public:
    static void RegisterAtoms();

    






#define GFX_ATOM(_name, _value) static nsIAtom* _name;
#include "gfxAtomList.h"
#undef GFX_ATOM
};

#endif 
