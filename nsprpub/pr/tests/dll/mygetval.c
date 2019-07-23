





































#if defined(WIN16)
#include <windows.h>
#endif
#include "prtypes.h"

extern PRIntn my_global;

PR_IMPLEMENT(PRIntn) My_GetValue()
{
    return my_global;
}

#if defined(WIN16)
int CALLBACK LibMain( HINSTANCE hInst, WORD wDataSeg, 
                      WORD cbHeapSize, LPSTR lpszCmdLine )
{
    return TRUE;
}
#endif 

