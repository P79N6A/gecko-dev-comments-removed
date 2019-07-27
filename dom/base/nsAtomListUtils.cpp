








#include "nsAtomListUtils.h"
#include "nsIAtom.h"
#include "nsStaticAtom.h"

 bool
nsAtomListUtils::IsMember(nsIAtom *aAtom,
                          const nsStaticAtom* aInfo,
                          uint32_t aInfoCount)
{
    for (const nsStaticAtom *info = aInfo, *info_end = aInfo + aInfoCount;
         info != info_end; ++info) {
        if (aAtom == *(info->mAtom))
            return true;
    }
    return false;
}
