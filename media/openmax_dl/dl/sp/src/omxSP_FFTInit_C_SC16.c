


























#include "dl/api/armOMX.h"
#include "dl/api/omxtypes.h"
#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"


























OMXResult omxSP_FFTInit_C_SC16(
     OMXFFTSpec_C_SC16* pFFTSpec,
     OMX_INT order
 )
 {
    OMX_INT     i,j;
    OMX_SC16    *pTwiddle, *pBuf;
    OMX_U16     *pBitRev;
    OMX_INT     Nby2,N,M,diff,step;
    OMX_U32             pTmp;
    ARMsFFTSpec_SC16 *pFFTStruct = 0;
    OMX_S16     x,y,xNeg;
    OMX_S32     xS32,yS32;


    pFFTStruct = (ARMsFFTSpec_SC16 *) pFFTSpec;

    
    if (order == 0)
    {
        pFFTStruct->N = 1;
        return OMX_Sts_NoErr;
    }

    
    Nby2 = 1 << (order - 1);
    N = Nby2 << 1;
    M = N>>3;

    pBitRev = NULL ;

    pTwiddle = (OMX_SC16 *)
        (sizeof(ARMsFFTSpec_SC16) + (OMX_S8*) pFFTSpec);

    
    pTmp = ((OMX_U32)pTwiddle)&31;              
    if(pTmp != 0)
        pTwiddle = (OMX_SC16*) ((OMX_S8*)pTwiddle + (32-pTmp));

    pBuf = (OMX_SC16 *)
        (sizeof(OMX_SC16) * (3*N/4) + (OMX_S8*) pTwiddle);

    
    pTmp = ((OMX_U32)pBuf)&31;                 
    if(pTmp != 0)
        pBuf = (OMX_SC16*) ((OMX_S8*)pBuf + (32-pTmp));



    












    diff = 12 - order;
    step = 1<<diff;             

    xS32 = armSP_FFT_S32TwiddleTable[0];
    yS32 = armSP_FFT_S32TwiddleTable[1];
    x = (xS32+0x8000)>>16;
    y = (yS32+0x8000)>>16;

    xNeg = 0x7FFF;

    if(order >=3)
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

            xS32 = armSP_FFT_S32TwiddleTable[2*j];
            yS32 = armSP_FFT_S32TwiddleTable[2*j+1];
            x = (xS32+0x8000)>>16;
            y = (yS32+0x8000)>>16;

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
        if (order == 2)
        {
            pTwiddle[0].Re = x;
            pTwiddle[0].Im = y;
            pTwiddle[1].Re = -y;
            pTwiddle[1].Im = xNeg;
            pTwiddle[2].Re = xNeg;
            pTwiddle[2].Im = y;

        }
        if (order == 1)
        {
            pTwiddle[0].Re = x;
            pTwiddle[0].Im = y;

        }


    }


    
    pFFTStruct->N = N;
    pFFTStruct->pTwiddle = pTwiddle;
    pFFTStruct->pBitRev = pBitRev;
    pFFTStruct->pBuf = pBuf;

    return OMX_Sts_NoErr;
}





