








#include "compiler/osinclude.h"




OS_TLSIndex OS_AllocTLSIndex()
{
    PRUintn index;
    PRStatus status = PR_NewThreadPrivateIndex(&index, NULL);

    if (status) {
        assert(0 && "OS_AllocTLSIndex(): Unable to allocate Thread Local Storage");
        return OS_INVALID_TLS_INDEX;
    }

    return index;
}


bool OS_SetTLSValue(OS_TLSIndex nIndex, void *lpvValue)
{
    if (nIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "OS_SetTLSValue(): Invalid TLS Index");
        return false;
    }

    return PR_SetThreadPrivate(nIndex, lpvValue) == 0;
}


bool OS_FreeTLSIndex(OS_TLSIndex nIndex)
{
    
    return true;
}
