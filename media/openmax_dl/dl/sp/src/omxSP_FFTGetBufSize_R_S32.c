


























#include "dl/api/armOMX.h"
#include "dl/api/omxtypes.h"
#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"























OMXResult omxSP_FFTGetBufSize_R_S32(
     OMX_INT order,     
     OMX_INT *pSize
 )
{
    OMX_INT     NBy2,N,twiddleSize;
    
    
    
    if (order == 0)
    {
        *pSize = sizeof(ARMsFFTSpec_R_SC32)
                + sizeof(OMX_S32) * (2);    

        return OMX_Sts_NoErr;
    }
    
    NBy2 = 1 << (order - 1);
    N = NBy2<<1;
    twiddleSize = 5*N/8;            
    
    
    *pSize = sizeof(ARMsFFTSpec_R_SC32)
        
           + sizeof(OMX_SC32) * twiddleSize
              
           + sizeof(OMX_S32) * (N<<1)  
           + 62 ;   
           
           
    return OMX_Sts_NoErr;
}





