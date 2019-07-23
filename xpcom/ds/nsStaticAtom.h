





































#ifndef nsStaticAtom_h__
#define nsStaticAtom_h__

#include "nsIAtom.h"











struct nsStaticAtom {
    const char* mString;
    nsIAtom ** mAtom;
};




NS_COM nsresult
NS_RegisterStaticAtoms(const nsStaticAtom*, PRUint32 aAtomCount);

#endif
