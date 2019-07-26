









#include "dsp.h"

#include "signal_processing_library.h"

#include "dsp_helpfunctions.h"











#define	 SCRATCH_pw16_corrVec			0
#define	 SCRATCH_pw16_data_ds			0
#define	 SCRATCH_pw32_corr				124

#define NETEQ_CORRELATOR_DSVECLEN 		124	/* 124 = 60 + 10 + 54 */

int16_t WebRtcNetEQ_Correlator(DSPInst_t *inst,
#ifdef SCRATCH
                               int16_t *pw16_scratchPtr,
#endif
                               int16_t *pw16_data,
                               int16_t w16_dataLen,
                               int16_t *pw16_corrOut,
                               int16_t *pw16_corrScale)
{
    int16_t w16_corrLen = 60;
#ifdef SCRATCH
    int16_t *pw16_data_ds = pw16_scratchPtr + SCRATCH_pw16_corrVec;
    int32_t *pw32_corr = (int32_t*) (pw16_scratchPtr + SCRATCH_pw32_corr);
    
#else
    int16_t pw16_data_ds[NETEQ_CORRELATOR_DSVECLEN];
    int32_t pw32_corr[54];
    
#endif
    
    int16_t w16_maxVal;
    int32_t w32_maxVal;
    int16_t w16_normVal;
    int16_t w16_normVal2;
    
    int16_t *pw16_B = NULL;
    int16_t w16_Blen = 0;
    int16_t w16_factor = 0;

    
    if (inst->fs == 8000)
    {
        w16_Blen = 3;
        w16_factor = 2;
        pw16_B = (int16_t*) WebRtcNetEQ_kDownsample8kHzTbl;
#ifdef NETEQ_WIDEBAND
    }
    else if (inst->fs==16000)
    {
        w16_Blen = 5;
        w16_factor = 4;
        pw16_B = (int16_t*)WebRtcNetEQ_kDownsample16kHzTbl;
#endif
#ifdef NETEQ_32KHZ_WIDEBAND
    }
    else if (inst->fs==32000)
    {
        w16_Blen = 7;
        w16_factor = 8;
        pw16_B = (int16_t*)WebRtcNetEQ_kDownsample32kHzTbl;
#endif
#ifdef NETEQ_48KHZ_WIDEBAND
    }
    else 
    {
        w16_Blen = 7;
        w16_factor = 12;
        pw16_B = (int16_t*)WebRtcNetEQ_kDownsample48kHzTbl;
#endif
    }

    
    WebRtcSpl_DownsampleFast(
        pw16_data + w16_dataLen - (NETEQ_CORRELATOR_DSVECLEN * w16_factor),
        (int16_t) (NETEQ_CORRELATOR_DSVECLEN * w16_factor), pw16_data_ds,
        NETEQ_CORRELATOR_DSVECLEN, pw16_B, w16_Blen, w16_factor, (int16_t) 0);

    
    w16_maxVal = WebRtcSpl_MaxAbsValueW16(pw16_data_ds, 124);
    w16_normVal = 16 - WebRtcSpl_NormW32((int32_t) w16_maxVal);
    WebRtcSpl_VectorBitShiftW16(pw16_data_ds, NETEQ_CORRELATOR_DSVECLEN, pw16_data_ds,
        w16_normVal);

    

    WebRtcNetEQ_CrossCorr(
        pw32_corr, &pw16_data_ds[NETEQ_CORRELATOR_DSVECLEN - w16_corrLen],
        &pw16_data_ds[NETEQ_CORRELATOR_DSVECLEN - w16_corrLen - 10], 60, 54,
        6 , -1);

    



    w32_maxVal = WebRtcSpl_MaxAbsValueW32(pw32_corr, 54);
    w16_normVal2 = 18 - WebRtcSpl_NormW32(w32_maxVal);
    w16_normVal2 = WEBRTC_SPL_MAX(w16_normVal2, 0);

    WebRtcSpl_VectorBitShiftW32ToW16(pw16_corrOut, 54, pw32_corr, w16_normVal2);

    
    *pw16_corrScale = 2 * w16_normVal + 6 + w16_normVal2;

    return (50 + 1);
}

#undef	 SCRATCH_pw16_corrVec
#undef	 SCRATCH_pw16_data_ds
#undef	 SCRATCH_pw32_corr

