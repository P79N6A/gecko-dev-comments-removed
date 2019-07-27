



























#ifndef _armSP_H_
#define _armSP_H_

#include "dl/api/omxtypes.h"

#ifdef __cplusplus
extern "C" {
#endif


extern  OMX_S32 armSP_FFT_S32TwiddleTable[1026];
extern OMX_F32 armSP_FFT_F32TwiddleTable[];

typedef struct  ARMsFFTSpec_SC32_Tag 
{
    OMX_U32     N;
    OMX_U16     *pBitRev;    
    OMX_SC32    *pTwiddle;
    OMX_SC32    *pBuf;
}ARMsFFTSpec_SC32;


typedef struct  ARMsFFTSpec_SC16_Tag 
{
    OMX_U32     N;
    OMX_U16     *pBitRev;    
    OMX_SC16    *pTwiddle;
    OMX_SC16    *pBuf;
}ARMsFFTSpec_SC16;

typedef struct  ARMsFFTSpec_R_SC32_Tag 
{
    OMX_U32     N;
    OMX_U16     *pBitRev;    
    OMX_SC32    *pTwiddle;
    OMX_S32     *pBuf;
}ARMsFFTSpec_R_SC32;

typedef struct ARMsFFTSpec_R_FC32_Tag
{
    OMX_U32 N;
    OMX_U16* pBitRev;
    OMX_FC32* pTwiddle;
    OMX_F32* pBuf;
} ARMsFFTSpec_R_FC32;

typedef struct ARMsFFTSpec_FC32_Tag
{
    OMX_U32 N;
    OMX_U16* pBitRev;
    OMX_FC32* pTwiddle;
    OMX_FC32* pBuf;
} ARMsFFTSpec_FC32;

#ifdef __cplusplus
}
#endif

#endif





