












#include "dl/api/armOMX.h"
#include "dl/api/omxtypes.h"
#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"



























OMXResult omxSP_FFTInit_C_FC32(OMXFFTSpec_C_FC32* pFFTSpec, OMX_INT order) {
  OMX_INT i;
  OMX_INT j;
  OMX_FC32* pTwiddle;
  OMX_FC32* pBuf;
  OMX_U16* pBitRev;
  OMX_U32 pTmp;
  OMX_INT Nby2;
  OMX_INT N;
  OMX_INT M;
  OMX_INT diff;
  OMX_INT step;
  ARMsFFTSpec_FC32* pFFTStruct = 0;
  OMX_F32 x;
  OMX_F32 y;
  OMX_F32 xNeg;

  pFFTStruct = (ARMsFFTSpec_FC32 *) pFFTSpec;

  
  if (!pFFTSpec || (order < 1) || (order > TWIDDLE_TABLE_ORDER))
    return OMX_Sts_BadArgErr;

  
  Nby2 = 1 << (order - 1);
  N = Nby2 << 1;
  M = N >> 3;

  
  pBitRev = NULL;

  pTwiddle = (OMX_FC32 *) (sizeof(ARMsFFTSpec_FC32) + (OMX_S8*) pFFTSpec);

  
  pTmp = ((OMX_U32) pTwiddle) & 31;
  if (pTmp)
    pTwiddle = (OMX_FC32*) ((OMX_S8*)pTwiddle + (32 - pTmp));

  pBuf = (OMX_FC32*) (sizeof(OMX_FC32) * (3 * N / 4) + (OMX_S8*) pTwiddle);

  
  pTmp = ((OMX_U32)pBuf) & 31;
  if (pTmp)
    pBuf = (OMX_FC32*) ((OMX_S8*)pBuf + (32 - pTmp));

  












  diff = TWIDDLE_TABLE_ORDER - order;
  
  step = 1 << diff;

  x = armSP_FFT_F32TwiddleTable[0];
  y = armSP_FFT_F32TwiddleTable[1];
  xNeg = 1;

  if (order >= 3) {
    
    pTwiddle[0].Re = x;
    pTwiddle[0].Im = y;
    pTwiddle[2 * M].Re = -y;
    pTwiddle[2 * M].Im = xNeg;
    pTwiddle[4 * M].Re = xNeg;
    pTwiddle[4 * M].Im = y;

    for (i = 1; i <= M; i++) {
      j = i * step;

      x = armSP_FFT_F32TwiddleTable[2 * j];
      y = armSP_FFT_F32TwiddleTable[2 * j + 1];

      pTwiddle[i].Re = x;
      pTwiddle[i].Im = y;
      pTwiddle[2 * M - i].Re = -y;
      pTwiddle[2 * M - i].Im = -x;
      pTwiddle[2 * M + i].Re = y;
      pTwiddle[2 * M + i].Im = -x;
      pTwiddle[4 * M - i].Re = -x;
      pTwiddle[4 * M - i].Im = y;
      pTwiddle[4 * M + i].Re = -x;
      pTwiddle[4 * M + i].Im = -y;
      pTwiddle[6 * M - i].Re = y;
      pTwiddle[6 * M - i].Im = x;
    }
  } else if (order == 2) {
    pTwiddle[0].Re = x;
    pTwiddle[0].Im = y;
    pTwiddle[1].Re = -y;
    pTwiddle[1].Im = xNeg;
    pTwiddle[2].Re = xNeg;
    pTwiddle[2].Im = y;
  } else if (order == 1) {
    pTwiddle[0].Re = x;
    pTwiddle[0].Im = y;
  }

  
  pFFTStruct->N = N;
  pFFTStruct->pTwiddle = pTwiddle;
  pFFTStruct->pBitRev = pBitRev;
  pFFTStruct->pBuf = pBuf;

  return OMX_Sts_NoErr;
}




