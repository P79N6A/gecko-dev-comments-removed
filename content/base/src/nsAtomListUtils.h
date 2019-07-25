








































#ifndef nsAtomListUtils_h__
#define nsAtomListUtils_h__

#include "prtypes.h"

class nsIAtom;
struct nsStaticAtom;

class nsAtomListUtils {
public:
    static PRBool IsMember(nsIAtom *aAtom,
                           const nsStaticAtom* aInfo,
                           PRUint32 aInfoCount);
};

#endif 
