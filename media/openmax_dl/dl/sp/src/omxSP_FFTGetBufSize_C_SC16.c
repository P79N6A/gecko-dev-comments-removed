


























#include "dl/api/armOMX.h"
#include "dl/api/omxtypes.h"
#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"






























OMXResult omxSP_FFTGetBufSize_C_SC16(
     OMX_INT order,
     OMX_INT *pSize)
{
    
    OMX_INT     N,twiddleSize;
    
    
    if (order == 0)
    {
        *pSize = sizeof(ARMsFFTSpec_SC16);   
        return OMX_Sts_NoErr;
    }

    
    N = 1 << order;
    
    
    twiddleSize = 3*N/4;

    
    *pSize = sizeof(ARMsFFTSpec_SC16)
        
           + sizeof(OMX_SC16) * twiddleSize
           
           + sizeof(OMX_SC16) * N
           + 62 ;  
        
    return OMX_Sts_NoErr;
}





