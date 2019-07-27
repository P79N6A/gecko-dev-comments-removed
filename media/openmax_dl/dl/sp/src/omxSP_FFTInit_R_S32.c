


























#include "dl/api/armOMX.h"
#include "dl/api/omxtypes.h"
#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"


























OMXResult omxSP_FFTInit_R_S32(
     OMXFFTSpec_R_S32* pFFTSpec,
     OMX_INT order
)
{
    OMX_INT     i,j;
    OMX_SC32    *pTwiddle,*pTwiddle1,*pTwiddle2,*pTwiddle3,*pTwiddle4;
    OMX_S32     *pBuf;
    OMX_U16     *pBitRev;
    OMX_U32     pTmp;
    OMX_INT     Nby2,N,M,diff, step; 
    OMX_S32     x,y,xNeg;
    ARMsFFTSpec_R_SC32 *pFFTStruct = 0;

   
    pFFTStruct = (ARMsFFTSpec_R_SC32 *) pFFTSpec;

    
    if (order == 0)
    {
        pFFTStruct->N = 1;
        pFFTStruct->pTwiddle = NULL;
        pFFTStruct->pBuf = (OMX_S32 *)
               (sizeof(ARMsFFTSpec_R_SC32) + (OMX_S8*) pFFTSpec);
        
        return OMX_Sts_NoErr;
    }

    
    Nby2 = 1 << (order - 1);
    N = Nby2 << 1;
                    
    
    
    pBitRev = NULL ;                
    
    pTwiddle = (OMX_SC32 *) 
        (sizeof(ARMsFFTSpec_R_SC32) + (OMX_S8*) pFFTSpec);
        
    
    pTmp = ((OMX_U32)pTwiddle)&31;              
    if(pTmp != 0)
        pTwiddle = (OMX_SC32*) ((OMX_S8*)pTwiddle + (32-pTmp));                
        
    pBuf = (OMX_S32*)        
        (sizeof(OMX_SC32) * (5*N/8) + (OMX_S8*) pTwiddle);
    
    
    pTmp = ((OMX_U32)pBuf)&31;                 
    if(pTmp != 0)
        pBuf = (OMX_S32*) ((OMX_S8*)pBuf + (32-pTmp));                    
                    
        
    
    
    










    
    M = Nby2>>3;
    diff = 12 - (order-1);
    step = 1<<diff;             
    
    x = armSP_FFT_S32TwiddleTable[0];
    y = armSP_FFT_S32TwiddleTable[1];
    xNeg = 0x7FFFFFFF;
    
    if((order-1) >=3)    
    {
            
            pTwiddle[0].Re = x;
            pTwiddle[0].Im = y;
            pTwiddle[2*M].Re = -y;
            pTwiddle[2*M].Im = xNeg;
            pTwiddle[4*M].Re = xNeg;
            pTwiddle[4*M].Im = y;
            
    
        for (i=1; i<=M; i++)
          {
            j = i*step;
            
            x = armSP_FFT_S32TwiddleTable[2*j];
            y = armSP_FFT_S32TwiddleTable[2*j+1];
            
            pTwiddle[i].Re = x;
            pTwiddle[i].Im = y;
            pTwiddle[2*M-i].Re = -y;
            pTwiddle[2*M-i].Im = -x;
            pTwiddle[2*M+i].Re = y;
            pTwiddle[2*M+i].Im = -x;
            pTwiddle[4*M-i].Re = -x;
            pTwiddle[4*M-i].Im = y;
            pTwiddle[4*M+i].Re = -x;
            pTwiddle[4*M+i].Im = -y;
            pTwiddle[6*M-i].Re = y;
            pTwiddle[6*M-i].Im = x;
        }
        
     
    }
    else
    {
        if ((order-1) == 2)
        {
            pTwiddle[0].Re = x;
            pTwiddle[0].Im = y;
            pTwiddle[1].Re = -y;
            pTwiddle[1].Im = xNeg;
            pTwiddle[2].Re = xNeg;
            pTwiddle[2].Im = y;
        
        }
        if ((order-1) == 1)
        {
            pTwiddle[0].Re = x;
            pTwiddle[0].Im = y;
        
        }        
        
    
    }
    
    
    



     
    M = N>>3;
    diff = 12 - order;
    step = 1<<diff;
    
    pTwiddle1 = pTwiddle + 3*N/8;
    pTwiddle4 = pTwiddle1 + (N/4-1);
    pTwiddle3 = pTwiddle1 + N/8;
    pTwiddle2 = pTwiddle1 + (N/8-1);
    
    x = armSP_FFT_S32TwiddleTable[0];
    y = armSP_FFT_S32TwiddleTable[1];
    xNeg = 0x7FFFFFFF;
    
    if((order) >=3)    
    {
                        
    
        for (i=1; i<=M; i+=2 )
          {
            j = i*step;
            
            x = armSP_FFT_S32TwiddleTable[2*j];
            y = armSP_FFT_S32TwiddleTable[2*j+1];
            
            pTwiddle1[0].Re = x;
            pTwiddle1[0].Im = y;
            pTwiddle1 += 1;
            pTwiddle2[0].Re = -y;
            pTwiddle2[0].Im = -x;
            pTwiddle2 -= 1;
            pTwiddle3[0].Re = y;
            pTwiddle3[0].Im = -x;
            pTwiddle3 += 1;
            pTwiddle4[0].Re = -x;
            pTwiddle4[0].Im = y;
            pTwiddle4 -= 1;
            
        }
        
     
    }
    else
    {
        if (order == 2)
        {
            
            pTwiddle1[0].Re = -y;
            pTwiddle1[0].Im = xNeg;
            
        }
                
    
    }
     
   
    
    pFFTStruct->N = N;
    pFFTStruct->pTwiddle = pTwiddle;
    pFFTStruct->pBitRev = pBitRev;
    pFFTStruct->pBuf = pBuf;

    return OMX_Sts_NoErr;
}




