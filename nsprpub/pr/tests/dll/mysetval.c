




































#include "prtypes.h"

PRIntn my_global = 0;

PR_IMPLEMENT(void) My_SetValue(PRIntn val)
{
    my_global = val;
}
