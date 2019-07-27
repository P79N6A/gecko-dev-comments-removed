

































#ifndef _OMXSP_H_
#define _OMXSP_H_

#include "omxtypes.h"

#ifdef __cplusplus
extern "C" {
#endif



 typedef void OMXFFTSpec_C_SC16;
 typedef void OMXFFTSpec_C_SC32;
 typedef void OMXFFTSpec_R_S16S32;
 typedef void OMXFFTSpec_R_S32;




























OMXResult omxSP_Copy_S16 (
    const OMX_S16 *pSrc,
    OMX_S16 *pDst,
    OMX_INT len
);




























OMX_S32 omxSP_DotProd_S16 (
    const OMX_S16 *pSrc1,
    const OMX_S16 *pSrc2,
    OMX_INT len
);

































OMX_S32 omxSP_DotProd_S16_Sfs (
    const OMX_S16 *pSrc1,
    const OMX_S16 *pSrc2,
    OMX_INT len,
    OMX_INT scaleFactor
);

































OMX_INT omxSP_BlockExp_S16 (
    const OMX_S16 *pSrc,
    OMX_INT len
);

































OMX_INT omxSP_BlockExp_S32 (
    const OMX_S32 *pSrc,
    OMX_INT len
);






























































OMXResult omxSP_FIR_Direct_S16 (
    const OMX_S16 *pSrc,
    OMX_S16 *pDst,
    OMX_INT sampLen,
    const OMX_S16 *pTapsQ15,
    OMX_INT tapsLen,
    OMX_S16 *pDelayLine,
    OMX_INT *pDelayLineIndex
);





























































OMXResult omxSP_FIR_Direct_S16_I (
    OMX_S16 *pSrcDst,
    OMX_INT sampLen,
    const OMX_S16 *pTapsQ15,
    OMX_INT tapsLen,
    OMX_S16 *pDelayLine,
    OMX_INT *pDelayLineIndex
);






























































OMXResult omxSP_FIR_Direct_S16_Sfs (
    const OMX_S16 *pSrc,
    OMX_S16 *pDst,
    OMX_INT sampLen,
    const OMX_S16 *pTapsQ15,
    OMX_INT tapsLen,
    OMX_S16 *pDelayLine,
    OMX_INT *pDelayLineIndex,
    OMX_INT scaleFactor
);






























































OMXResult omxSP_FIR_Direct_S16_ISfs (
    OMX_S16 *pSrcDst,
    OMX_INT sampLen,
    const OMX_S16 *pTapsQ15,
    OMX_INT tapsLen,
    OMX_S16 *pDelayLine,
    OMX_INT *pDelayLineIndex,
    OMX_INT scaleFactor
);
























































OMXResult omxSP_FIROne_Direct_S16 (
    OMX_S16 val,
    OMX_S16 *pResult,
    const OMX_S16 *pTapsQ15,
    OMX_INT tapsLen,
    OMX_S16 *pDelayLine,
    OMX_INT *pDelayLineIndex
);
























































OMXResult omxSP_FIROne_Direct_S16_I (
    OMX_S16 *pValResult,
    const OMX_S16 *pTapsQ15,
    OMX_INT tapsLen,
    OMX_S16 *pDelayLine,
    OMX_INT *pDelayLineIndex
);

























































OMXResult omxSP_FIROne_Direct_S16_Sfs (
    OMX_S16 val,
    OMX_S16 *pResult,
    const OMX_S16 *pTapsQ15,
    OMX_INT tapsLen,
    OMX_S16 *pDelayLine,
    OMX_INT *pDelayLineIndex,
    OMX_INT scaleFactor
);

























































OMXResult omxSP_FIROne_Direct_S16_ISfs (
    OMX_S16 *pValResult,
    const OMX_S16 *pTapsQ15,
    OMX_INT tapsLen,
    OMX_S16 *pDelayLine,
    OMX_INT *pDelayLineIndex,
    OMX_INT scaleFactor
);

























































OMXResult omxSP_IIR_Direct_S16 (
    const OMX_S16 *pSrc,
    OMX_S16 *pDst,
    OMX_INT len,
    const OMX_S16 *pTaps,
    OMX_INT order,
    OMX_S32 *pDelayLine
);
























































OMXResult omxSP_IIR_Direct_S16_I (
    OMX_S16 *pSrcDst,
    OMX_INT len,
    const OMX_S16 *pTaps,
    OMX_INT order,
    OMX_S32 *pDelayLine
);



















































OMXResult omxSP_IIROne_Direct_S16 (
    OMX_S16 val,
    OMX_S16 *pResult,
    const OMX_S16 *pTaps,
    OMX_INT order,
    OMX_S32 *pDelayLine
);



















































OMXResult omxSP_IIROne_Direct_S16_I (
    OMX_S16 *pValResult,
    const OMX_S16 *pTaps,
    OMX_INT order,
    OMX_S32 *pDelayLine
);


















































OMXResult omxSP_IIR_BiQuadDirect_S16 (
    const OMX_S16 *pSrc,
    OMX_S16 *pDst,
    OMX_INT len,
    const OMX_S16 *pTaps,
    OMX_INT numBiquad,
    OMX_S32 *pDelayLine
);


















































OMXResult omxSP_IIR_BiQuadDirect_S16_I (
    OMX_S16 *pSrcDst,
    OMX_INT len,
    const OMX_S16 *pTaps,
    OMX_INT numBiquad,
    OMX_S32 *pDelayLine
);
















































OMXResult omxSP_IIROne_BiQuadDirect_S16 (
    OMX_S16 val,
    OMX_S16 *pResult,
    const OMX_S16 *pTaps,
    OMX_INT numBiquad,
    OMX_S32 *pDelayLine
);
















































OMXResult omxSP_IIROne_BiQuadDirect_S16_I (
    OMX_S16 *pValResult,
    const OMX_S16 *pTaps,
    OMX_INT numBiquad,
    OMX_S32 *pDelayLine
);




































OMXResult omxSP_FilterMedian_S32 (
    const OMX_S32 *pSrc,
    OMX_S32 *pDst,
    OMX_INT len,
    OMX_INT maskSize
);




































OMXResult omxSP_FilterMedian_S32_I (
    OMX_S32 *pSrcDst,
    OMX_INT len,
    OMX_INT maskSize
);







































OMXResult omxSP_FFTInit_C_SC16 (
    OMXFFTSpec_C_SC16 *pFFTSpec,
    OMX_INT order
);







































OMXResult omxSP_FFTInit_C_SC32 (
    OMXFFTSpec_C_SC32 *pFFTSpec,
    OMX_INT order
);




































OMXResult omxSP_FFTInit_R_S16S32 (
    OMXFFTSpec_R_S16S32*pFFTFwdSpec,
    OMX_INT order
);





































OMXResult omxSP_FFTInit_R_S32 (
    OMXFFTSpec_R_S32*pFFTFwdSpec,
    OMX_INT order
);































OMXResult omxSP_FFTGetBufSize_C_SC16 (
    OMX_INT order,
    OMX_INT *pSize
);































OMXResult omxSP_FFTGetBufSize_C_SC32 (
    OMX_INT order,
    OMX_INT *pSize
);






























OMXResult omxSP_FFTGetBufSize_R_S16S32 (
    OMX_INT order,
    OMX_INT *pSize
);






























OMXResult omxSP_FFTGetBufSize_R_S32 (
    OMX_INT order,
    OMX_INT *pSize
);









































OMXResult omxSP_FFTFwd_CToC_SC16_Sfs (
    const OMX_SC16 *pSrc,
    OMX_SC16 *pDst,
    const OMXFFTSpec_C_SC16 *pFFTSpec,
    OMX_INT scaleFactor
);









































OMXResult omxSP_FFTFwd_CToC_SC32_Sfs (
    const OMX_SC32 *pSrc,
    OMX_SC32 *pDst,
    const OMXFFTSpec_C_SC32 *pFFTSpec,
    OMX_INT scaleFactor
);









































OMXResult omxSP_FFTInv_CToC_SC16_Sfs (
    const OMX_SC16 *pSrc,
    OMX_SC16 *pDst,
    const OMXFFTSpec_C_SC16 *pFFTSpec,
    OMX_INT scaleFactor
);









































OMXResult omxSP_FFTInv_CToC_SC32_Sfs (
    const OMX_SC32 *pSrc,
    OMX_SC32 *pDst,
    const OMXFFTSpec_C_SC32 *pFFTSpec,
    OMX_INT scaleFactor
);

















































OMXResult omxSP_FFTFwd_RToCCS_S16S32_Sfs (
    const OMX_S16 *pSrc,
    OMX_S32 *pDst,
    const OMXFFTSpec_R_S16S32 *pFFTSpec,
    OMX_INT scaleFactor
);

















































OMXResult omxSP_FFTFwd_RToCCS_S32_Sfs (
    const OMX_S32 *pSrc,
    OMX_S32 *pDst,
    const OMXFFTSpec_R_S32 *pFFTSpec,
    OMX_INT scaleFactor
);











































OMXResult omxSP_FFTInv_CCSToR_S32S16_Sfs (
    const OMX_S32 *pSrc,
    OMX_S16 *pDst,
    const OMXFFTSpec_R_S16S32 *pFFTSpec,
    OMX_INT scaleFactor
);











































OMXResult omxSP_FFTInv_CCSToR_S32_Sfs (
    const OMX_S32 *pSrc,
    OMX_S32 *pDst,
    const OMXFFTSpec_R_S32 *pFFTSpec,
    OMX_INT scaleFactor
);



#ifdef __cplusplus
}
#endif

#endif



