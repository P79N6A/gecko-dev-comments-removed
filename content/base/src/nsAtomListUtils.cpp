








































#include "nsAtomListUtils.h"
#include "nsIAtom.h"
#include "nsStaticAtom.h"

 PRBool
nsAtomListUtils::IsMember(nsIAtom *aAtom,
                          const nsStaticAtom* aInfo,
                          PRUint32 aInfoCount)
{
    for (const nsStaticAtom *info = aInfo, *info_end = aInfo + aInfoCount;
         info != info_end; ++info) {
        if (aAtom == *(info->mAtom))
            return PR_TRUE;
    }
    return PR_FALSE;
}
