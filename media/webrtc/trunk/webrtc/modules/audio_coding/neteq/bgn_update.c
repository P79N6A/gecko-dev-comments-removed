













#include "dsp.h"

#include "signal_processing_library.h"

#include "dsp_helpfunctions.h"













#if (BGN_LPC_ORDER > 10) && (defined SCRATCH)
#error BGN_LPC_ORDER is too large for current scratch memory allocation
#endif

#define	 SCRATCH_PW32_AUTO_CORR			0
#define	 SCRATCH_PW16_TEMP_VEC			22
#define	 SCRATCH_PW16_RC				32
#define	 SCRATCH_PW16_OUT_VEC			0

#define NETEQFIX_BGNFRAQINCQ16	229 /* 0.0035 in Q16 */
















void WebRtcNetEQ_BGNUpdate(
#ifdef SCRATCH
                           DSPInst_t *inst, WebRtc_Word16 *pw16_scratchPtr
#else
                           DSPInst_t *inst
#endif
)
{
    const WebRtc_Word16 w16_vecLen = 256;
    BGNInst_t *BGN_Inst = &(inst->BGNInst);
#ifdef SCRATCH
    WebRtc_Word32 *pw32_autoCorr = (WebRtc_Word32*) (pw16_scratchPtr + SCRATCH_PW32_AUTO_CORR);
    WebRtc_Word16 *pw16_tempVec = pw16_scratchPtr + SCRATCH_PW16_TEMP_VEC;
    WebRtc_Word16 *pw16_rc = pw16_scratchPtr + SCRATCH_PW16_RC;
    WebRtc_Word16 *pw16_outVec = pw16_scratchPtr + SCRATCH_PW16_OUT_VEC;
#else
    WebRtc_Word32 pw32_autoCorr[BGN_LPC_ORDER + 1];
    WebRtc_Word16 pw16_tempVec[BGN_LPC_ORDER];
    WebRtc_Word16 pw16_outVec[BGN_LPC_ORDER + 64];
    WebRtc_Word16 pw16_rc[BGN_LPC_ORDER];
#endif
    WebRtc_Word16 pw16_A[BGN_LPC_ORDER + 1];
    WebRtc_Word32 w32_tmp;
    WebRtc_Word16 *pw16_vec;
    WebRtc_Word16 w16_maxSample;
    WebRtc_Word16 w16_tmp, w16_tmp2;
    WebRtc_Word16 w16_enSampleShift;
    WebRtc_Word32 w32_en, w32_enBGN;
    WebRtc_Word32 w32_enUpdateThreashold;
    WebRtc_Word16 stability;

    pw16_vec = inst->pw16_speechHistory + inst->w16_speechHistoryLen - w16_vecLen;

#ifdef NETEQ_VAD
    if( !inst->VADInst.VADEnabled 
        || inst->VADInst.VADDecision == 0 )
    { 
#endif


    WEBRTC_SPL_MEMCPY_W16(pw16_tempVec, pw16_vec - BGN_LPC_ORDER, BGN_LPC_ORDER);
    WebRtcSpl_MemSetW16(pw16_vec - BGN_LPC_ORDER, 0, BGN_LPC_ORDER);

    w16_maxSample = WebRtcSpl_MaxAbsValueW16(pw16_vec, w16_vecLen);
    w16_tmp = 8 
        - WebRtcSpl_NormW32(WEBRTC_SPL_MUL_16_16(w16_maxSample, w16_maxSample));
    w16_tmp = WEBRTC_SPL_MAX(0, w16_tmp);

    WebRtcNetEQ_CrossCorr(pw32_autoCorr, pw16_vec, pw16_vec, w16_vecLen, BGN_LPC_ORDER + 1,
        w16_tmp, -1);

    
    WEBRTC_SPL_MEMCPY_W16(pw16_vec - BGN_LPC_ORDER, pw16_tempVec, BGN_LPC_ORDER);

    w16_enSampleShift = 8 - w16_tmp; 
    
    w32_en = WEBRTC_SPL_RSHIFT_W32(pw32_autoCorr[0], w16_enSampleShift);
    if ((w32_en < BGN_Inst->w32_energyUpdate
#ifdef NETEQ_VAD
        
         && !inst->VADInst.VADEnabled)
    
    || (inst->VADInst.VADEnabled && inst->VADInst.VADDecision == 0)
#else
    ) 
#endif
    )
    {
        
        if (pw32_autoCorr[0] > 0)
        {
            


            if (w32_en < BGN_Inst->w32_energyUpdate)
            {
                
                BGN_Inst->w32_energyUpdate = WEBRTC_SPL_MAX(w32_en, 1);
                BGN_Inst->w32_energyUpdateLow = 0;
            }

            stability = WebRtcSpl_LevinsonDurbin(pw32_autoCorr, pw16_A, pw16_rc, BGN_LPC_ORDER);
            
            if (stability != 1)
            {
                return;
            }
        }
        else
        {
            
            return;
        }
        
        WebRtcSpl_FilterMAFastQ12(pw16_vec + w16_vecLen - 64, pw16_outVec, pw16_A,
            BGN_LPC_ORDER + 1, 64);
        w32_enBGN = WebRtcNetEQ_DotW16W16(pw16_outVec, pw16_outVec, 64, 0);
        

        






        if ((WEBRTC_SPL_MUL_32_16(w32_enBGN, 20) >= WEBRTC_SPL_LSHIFT_W32(w32_en, 6))
            && (w32_en > 0))
        {
            

            WEBRTC_SPL_MEMCPY_W16(BGN_Inst->pw16_filter, pw16_A, BGN_LPC_ORDER+1);
            WEBRTC_SPL_MEMCPY_W16(BGN_Inst->pw16_filterState,
                pw16_vec + w16_vecLen - BGN_LPC_ORDER, BGN_LPC_ORDER);

            
            BGN_Inst->w32_energy = WEBRTC_SPL_MAX(w32_en, 1);

            
            
            BGN_Inst->w32_energyUpdate = WEBRTC_SPL_MAX(w32_en, 1);
            BGN_Inst->w32_energyUpdateLow = 0;

            
            w16_tmp2 = WebRtcSpl_NormW32(w32_enBGN) - 1;
            if (w16_tmp2 & 0x1)
            {
                w16_tmp2 -= 1; 
            }
            w32_enBGN = WEBRTC_SPL_SHIFT_W32(w32_enBGN, w16_tmp2);

            
            BGN_Inst->w16_scale = (WebRtc_Word16) WebRtcSpl_SqrtFloor(w32_enBGN);
            BGN_Inst->w16_scaleShift = 13 + ((6 + w16_tmp2) >> 1); 
            

            BGN_Inst->w16_initialized = 1;
        }

    }
    else
    {
        





        w32_tmp = WEBRTC_SPL_MUL_16_16_RSFT(NETEQFIX_BGNFRAQINCQ16,
            BGN_Inst->w32_energyUpdateLow, 16);
        w32_tmp += WEBRTC_SPL_MUL_16_16(NETEQFIX_BGNFRAQINCQ16,
            (WebRtc_Word16)(BGN_Inst->w32_energyUpdate & 0xFF));
        w32_tmp += (WEBRTC_SPL_MUL_16_16(NETEQFIX_BGNFRAQINCQ16,
            (WebRtc_Word16)((BGN_Inst->w32_energyUpdate>>8) & 0xFF)) << 8);
        BGN_Inst->w32_energyUpdateLow += w32_tmp;

        BGN_Inst->w32_energyUpdate += WEBRTC_SPL_MUL_16_16(NETEQFIX_BGNFRAQINCQ16,
            (WebRtc_Word16)(BGN_Inst->w32_energyUpdate>>16));
        BGN_Inst->w32_energyUpdate += BGN_Inst->w32_energyUpdateLow >> 16;
        BGN_Inst->w32_energyUpdateLow = (BGN_Inst->w32_energyUpdateLow & 0x0FFFF);

        
        
        BGN_Inst->w32_energyMax = BGN_Inst->w32_energyMax - (BGN_Inst->w32_energyMax >> 10);
        if (w32_en > BGN_Inst->w32_energyMax)
        {
            BGN_Inst->w32_energyMax = w32_en;
        }

        
        w32_enUpdateThreashold = (BGN_Inst->w32_energyMax + 524288) >> 20;
        if (w32_enUpdateThreashold > BGN_Inst->w32_energyUpdate)
        {
            BGN_Inst->w32_energyUpdate = w32_enUpdateThreashold;
        }
    }

#ifdef NETEQ_VAD
} 
#endif 

    return;
}

#undef	 SCRATCH_PW32_AUTO_CORR
#undef	 SCRATCH_PW16_TEMP_VEC
#undef	 SCRATCH_PW16_RC
#undef	 SCRATCH_PW16_OUT_VEC

