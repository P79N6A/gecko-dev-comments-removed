








#ifndef nsAtomListUtils_h__
#define nsAtomListUtils_h__

#include "mozilla/StandardInteger.h"

class nsIAtom;
struct nsStaticAtom;

class nsAtomListUtils {
public:
    static bool IsMember(nsIAtom *aAtom,
                           const nsStaticAtom* aInfo,
                           uint32_t aInfoCount);
};

#endif 
