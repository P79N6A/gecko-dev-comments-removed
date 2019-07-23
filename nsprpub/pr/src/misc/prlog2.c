




































#include "prbit.h"




PR_IMPLEMENT(PRIntn) PR_CeilingLog2(PRUint32 n)
{
    PRIntn log2;
    PR_CEILING_LOG2(log2, n);
    return log2;
}





PR_IMPLEMENT(PRIntn) PR_FloorLog2(PRUint32 n)
{
    PRIntn log2;
    PR_FLOOR_LOG2(log2, n);
    return log2;
}
