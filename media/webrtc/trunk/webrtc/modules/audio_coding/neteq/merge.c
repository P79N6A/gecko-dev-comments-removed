













#include "dsp.h"

#include "signal_processing_library.h"

#include "dsp_helpfunctions.h"
#include "neteq_error_codes.h"







































#define SCRATCH_pw16_expanded          0
#if (defined(NETEQ_48KHZ_WIDEBAND)) 
#define SCRATCH_pw16_expandedLB        1260
#define SCRATCH_pw16_decodedLB         1360
#define SCRATCH_pw32_corr              1400
#define SCRATCH_pw16_corrVec           1260
#define SCRATCH_NETEQ_EXPAND            756
#elif (defined(NETEQ_32KHZ_WIDEBAND)) 
#define SCRATCH_pw16_expandedLB        840
#define SCRATCH_pw16_decodedLB         940
#define SCRATCH_pw32_corr              980
#define SCRATCH_pw16_corrVec           840
#define SCRATCH_NETEQ_EXPAND            504
#elif (defined(NETEQ_WIDEBAND)) 
#define SCRATCH_pw16_expandedLB        420
#define SCRATCH_pw16_decodedLB         520
#define SCRATCH_pw32_corr              560
#define SCRATCH_pw16_corrVec           420
#define SCRATCH_NETEQ_EXPAND            252
#else    
#define SCRATCH_pw16_expandedLB        210
#define SCRATCH_pw16_decodedLB         310
#define SCRATCH_pw32_corr              350
#define SCRATCH_pw16_corrVec           210
#define SCRATCH_NETEQ_EXPAND            126
#endif

int WebRtcNetEQ_Merge(DSPInst_t *inst,
#ifdef SCRATCH
                      WebRtc_Word16 *pw16_scratchPtr,
#endif
                      WebRtc_Word16 *pw16_decoded, int len, WebRtc_Word16 *pw16_outData,
                      WebRtc_Word16 *pw16_len)
{

    WebRtc_Word16 fs_mult;
    WebRtc_Word16 fs_shift;
    WebRtc_Word32 w32_En_new_frame, w32_En_old_frame;
    WebRtc_Word16 w16_expmax, w16_newmax;
    WebRtc_Word16 w16_tmp, w16_tmp2;
    WebRtc_Word32 w32_tmp;
#ifdef SCRATCH
    WebRtc_Word16 *pw16_expanded = pw16_scratchPtr + SCRATCH_pw16_expanded;
    WebRtc_Word16 *pw16_expandedLB = pw16_scratchPtr + SCRATCH_pw16_expandedLB;
    WebRtc_Word16 *pw16_decodedLB = pw16_scratchPtr + SCRATCH_pw16_decodedLB;
    WebRtc_Word32 *pw32_corr = (WebRtc_Word32*) (pw16_scratchPtr + SCRATCH_pw32_corr);
    WebRtc_Word16 *pw16_corrVec = pw16_scratchPtr + SCRATCH_pw16_corrVec;
#else
    WebRtc_Word16 pw16_expanded[(125+80+5)*FSMULT];
    WebRtc_Word16 pw16_expandedLB[100];
    WebRtc_Word16 pw16_decodedLB[40];
    WebRtc_Word32 pw32_corr[60];
    WebRtc_Word16 pw16_corrVec[4+60+4];
#endif
    WebRtc_Word16 *pw16_corr = &pw16_corrVec[4];
    WebRtc_Word16 w16_stopPos = 0, w16_bestIndex, w16_interpLen;
    WebRtc_Word16 w16_bestVal; 
    WebRtc_Word16 w16_startfact, w16_inc;
    WebRtc_Word16 w16_expandedLen;
    WebRtc_Word16 w16_startPos;
    WebRtc_Word16 w16_expLen, w16_newLen = 0;
    WebRtc_Word16 *pw16_decodedOut;
    WebRtc_Word16 w16_muted;

    int w16_decodedLen = len;

#ifdef NETEQ_STEREO
    MasterSlaveInfo *msInfo = inst->msInfo;
#endif

    fs_mult = WebRtcSpl_DivW32W16ResW16(inst->fs, 8000);
    fs_shift = 30 - WebRtcSpl_NormW32(fs_mult); 

    


    



    w16_startPos = inst->endPosition - inst->curPosition;
    
    inst->ExpandInst.w16_stopMuting = 1;
    inst->ExpandInst.w16_lagsDirection = 1; 
    inst->ExpandInst.w16_lagsPosition = -1; 
    w16_expandedLen = 0; 

    if (w16_startPos >= 210 * FSMULT)
    {
        






        w16_tmp = w16_startPos - 210 * FSMULT; 

        WEBRTC_SPL_MEMMOVE_W16(&inst->speechBuffer[inst->curPosition+w16_tmp] ,
                               &inst->speechBuffer[inst->curPosition], 210*FSMULT);

        inst->curPosition += w16_tmp; 
        w16_startPos = 210 * FSMULT; 
    }

    WebRtcNetEQ_Expand(inst,
#ifdef SCRATCH
        pw16_scratchPtr + SCRATCH_NETEQ_EXPAND,
#endif
        pw16_expanded, 
        &w16_newLen, 0);

    




    WEBRTC_SPL_MEMMOVE_W16(&pw16_expanded[w16_startPos], pw16_expanded,
                           WEBRTC_SPL_MIN(w16_newLen,
                               WEBRTC_SPL_MAX(210*FSMULT - w16_startPos, 0) ) );

    inst->ExpandInst.w16_stopMuting = 0;

    

    WEBRTC_SPL_MEMCPY_W16(pw16_expanded, &inst->speechBuffer[inst->curPosition], w16_startPos);

    



    w16_expandedLen = (120 + 80 + 2) * fs_mult;
    w16_expLen = w16_startPos + w16_newLen;

    if (w16_expLen < w16_expandedLen)
    {
        while ((w16_expLen + w16_newLen) < w16_expandedLen)
        {
            WEBRTC_SPL_MEMCPY_W16(&pw16_expanded[w16_expLen], &pw16_expanded[w16_startPos],
                w16_newLen);
            w16_expLen += w16_newLen;
        }

        

        WEBRTC_SPL_MEMCPY_W16(&pw16_expanded[w16_expLen], &pw16_expanded[w16_startPos],
                              (w16_expandedLen-w16_expLen));
    }
    w16_expLen = w16_expandedLen;

    
    inst->w16_muteFactor
        = (WebRtc_Word16) WEBRTC_SPL_MUL_16_16_RSFT(inst->w16_muteFactor,
            inst->ExpandInst.w16_expandMuteFactor, 14);

    
    len = WEBRTC_SPL_MIN(64*fs_mult, w16_decodedLen);
    w16_expmax = WebRtcSpl_MaxAbsValueW16(pw16_expanded, (WebRtc_Word16) len);
    w16_newmax = WebRtcSpl_MaxAbsValueW16(pw16_decoded, (WebRtc_Word16) len);

    
    w16_tmp = 6 + fs_shift - WebRtcSpl_NormW32(WEBRTC_SPL_MUL_16_16(w16_expmax, w16_expmax));
    w16_tmp = WEBRTC_SPL_MAX(w16_tmp,0);
    w32_En_old_frame = WebRtcNetEQ_DotW16W16(pw16_expanded, pw16_expanded, len, w16_tmp);

    
    w16_tmp2 = 6 + fs_shift - WebRtcSpl_NormW32(WEBRTC_SPL_MUL_16_16(w16_newmax, w16_newmax));
    w16_tmp2 = WEBRTC_SPL_MAX(w16_tmp2,0);
    w32_En_new_frame = WebRtcNetEQ_DotW16W16(pw16_decoded, pw16_decoded, len, w16_tmp2);

    
    if (w16_tmp2 > w16_tmp)
    {
        w32_En_old_frame = WEBRTC_SPL_RSHIFT_W32(w32_En_old_frame, (w16_tmp2-w16_tmp));
    }
    else
    {
        w32_En_new_frame = WEBRTC_SPL_RSHIFT_W32(w32_En_new_frame, (w16_tmp-w16_tmp2));
    }

    
    if (w32_En_new_frame > w32_En_old_frame)
    {
        
        w16_tmp = WebRtcSpl_NormW32(w32_En_new_frame) - 17;
        w32_En_new_frame = WEBRTC_SPL_SHIFT_W32(w32_En_new_frame, w16_tmp);

        



        w16_tmp = w16_tmp + 14;
        w32_En_old_frame = WEBRTC_SPL_SHIFT_W32(w32_En_old_frame, w16_tmp);
        w16_tmp
            = WebRtcSpl_DivW32W16ResW16(w32_En_old_frame, (WebRtc_Word16) w32_En_new_frame);
        
        w16_muted = (WebRtc_Word16) WebRtcSpl_SqrtFloor(
            WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)w16_tmp,14));
    }
    else
    {
        w16_muted = 16384; 
    }

    
    if (w16_muted > inst->w16_muteFactor)
    {
        inst->w16_muteFactor = WEBRTC_SPL_MIN(w16_muted, 16384);
    }

#ifdef NETEQ_STEREO

    
    if (msInfo == NULL)
    {
        
        return MASTER_SLAVE_ERROR;
    }

    
    if ((msInfo->msMode == NETEQ_MASTER) || (msInfo->msMode == NETEQ_MONO))
    {
#endif






        if (inst->fs == 8000)
        {
            WebRtcSpl_DownsampleFast(&pw16_expanded[2], (WebRtc_Word16) (w16_expandedLen - 2),
                pw16_expandedLB, (WebRtc_Word16) (100),
                (WebRtc_Word16*) WebRtcNetEQ_kDownsample8kHzTbl, (WebRtc_Word16) 3,
                (WebRtc_Word16) 2, (WebRtc_Word16) 0);
            if (w16_decodedLen <= 80)
            {
                
                WebRtc_Word16 temp_len = w16_decodedLen - 2;
                w16_tmp = temp_len / 2;
                WebRtcSpl_DownsampleFast(&pw16_decoded[2], temp_len,
                                         pw16_decodedLB, w16_tmp,
                                         (WebRtc_Word16*) WebRtcNetEQ_kDownsample8kHzTbl,
                    (WebRtc_Word16) 3, (WebRtc_Word16) 2, (WebRtc_Word16) 0);
                WebRtcSpl_MemSetW16(&pw16_decodedLB[w16_tmp], 0, (40 - w16_tmp));
            }
            else
            {
                WebRtcSpl_DownsampleFast(&pw16_decoded[2],
                    (WebRtc_Word16) (w16_decodedLen - 2), pw16_decodedLB,
                    (WebRtc_Word16) (40), (WebRtc_Word16*) WebRtcNetEQ_kDownsample8kHzTbl,
                    (WebRtc_Word16) 3, (WebRtc_Word16) 2, (WebRtc_Word16) 0);
            }
#ifdef NETEQ_WIDEBAND
        }
        else if (inst->fs==16000)
        {
            WebRtcSpl_DownsampleFast(
                &pw16_expanded[4], (WebRtc_Word16)(w16_expandedLen-4),
                pw16_expandedLB, (WebRtc_Word16)(100),
                (WebRtc_Word16*)WebRtcNetEQ_kDownsample16kHzTbl, (WebRtc_Word16)5,
                (WebRtc_Word16)4, (WebRtc_Word16)0);
            if (w16_decodedLen<=160)
            {
                
                WebRtc_Word16 temp_len = w16_decodedLen - 4;
                w16_tmp = temp_len / 4;
                WebRtcSpl_DownsampleFast(
                    &pw16_decoded[4], temp_len,
                    pw16_decodedLB, w16_tmp,
                    (WebRtc_Word16*)WebRtcNetEQ_kDownsample16kHzTbl, (WebRtc_Word16)5,
                    (WebRtc_Word16)4, (WebRtc_Word16)0);
                WebRtcSpl_MemSetW16(&pw16_decodedLB[w16_tmp], 0, (40-w16_tmp));
            }
            else
            {
                WebRtcSpl_DownsampleFast(
                    &pw16_decoded[4], (WebRtc_Word16)(w16_decodedLen-4),
                    pw16_decodedLB, (WebRtc_Word16)(40),
                    (WebRtc_Word16*)WebRtcNetEQ_kDownsample16kHzTbl, (WebRtc_Word16)5,
                    (WebRtc_Word16)4, (WebRtc_Word16)0);
            }
#endif
#ifdef NETEQ_32KHZ_WIDEBAND
        }
        else if (inst->fs==32000)
        {
            


            WebRtcSpl_DownsampleFast(
                &pw16_expanded[6], (WebRtc_Word16)(w16_expandedLen-6),
                pw16_expandedLB, (WebRtc_Word16)(100),
                (WebRtc_Word16*)WebRtcNetEQ_kDownsample32kHzTbl, (WebRtc_Word16)7,
                (WebRtc_Word16)8, (WebRtc_Word16)0);
            if (w16_decodedLen<=320)
            {
                
                WebRtc_Word16 temp_len = w16_decodedLen - 6;
                w16_tmp = temp_len / 8;
                WebRtcSpl_DownsampleFast(
                      &pw16_decoded[6], temp_len,
                      pw16_decodedLB, w16_tmp,
                      (WebRtc_Word16*)WebRtcNetEQ_kDownsample32kHzTbl, (WebRtc_Word16)7,
                      (WebRtc_Word16)8, (WebRtc_Word16)0);
                WebRtcSpl_MemSetW16(&pw16_decodedLB[w16_tmp], 0, (40-w16_tmp));
            }
            else
            {
                WebRtcSpl_DownsampleFast(
                    &pw16_decoded[6], (WebRtc_Word16)(w16_decodedLen-6),
                    pw16_decodedLB, (WebRtc_Word16)(40),
                    (WebRtc_Word16*)WebRtcNetEQ_kDownsample32kHzTbl, (WebRtc_Word16)7,
                    (WebRtc_Word16)8, (WebRtc_Word16)0);
            }
#endif
#ifdef NETEQ_48KHZ_WIDEBAND
        }
        else 
        {
            


            WebRtcSpl_DownsampleFast(
                &pw16_expanded[6], (WebRtc_Word16)(w16_expandedLen-6),
                pw16_expandedLB, (WebRtc_Word16)(100),
                (WebRtc_Word16*)WebRtcNetEQ_kDownsample48kHzTbl, (WebRtc_Word16)7,
                (WebRtc_Word16)12, (WebRtc_Word16)0);
            if (w16_decodedLen<=320)
            {
                
                




                WebRtc_Word16 temp_len = w16_decodedLen - 6;
                w16_tmp = temp_len / 8;
                WebRtcSpl_DownsampleFast(
                    &pw16_decoded[6], temp_len,
                    pw16_decodedLB, w16_tmp,
                    (WebRtc_Word16*)WebRtcNetEQ_kDownsample48kHzTbl, (WebRtc_Word16)7,
                    (WebRtc_Word16)12, (WebRtc_Word16)0);
                WebRtcSpl_MemSetW16(&pw16_decodedLB[w16_tmp], 0, (40-w16_tmp));
            }
            else
            {
                WebRtcSpl_DownsampleFast(
                    &pw16_decoded[6], (WebRtc_Word16)(w16_decodedLen-6),
                    pw16_decodedLB, (WebRtc_Word16)(40),
                    (WebRtc_Word16*)WebRtcNetEQ_kDownsample48kHzTbl, (WebRtc_Word16)7,
                    (WebRtc_Word16)12, (WebRtc_Word16)0);
            }
#endif
        }

        
        w16_tmp = WebRtcSpl_DivW32W16ResW16((WebRtc_Word32) inst->ExpandInst.w16_maxLag,
            (WebRtc_Word16) (fs_mult * 2)) + 1;
        w16_stopPos = WEBRTC_SPL_MIN(60, w16_tmp);
        w32_tmp = WEBRTC_SPL_MUL_16_16(w16_expmax, w16_newmax);
        if (w32_tmp > 26843546)
        {
            w16_tmp = 3;
        }
        else
        {
            w16_tmp = 0;
        }

        WebRtcNetEQ_CrossCorr(pw32_corr, pw16_decodedLB, pw16_expandedLB, 40,
            (WebRtc_Word16) w16_stopPos, w16_tmp, 1);

        
        WebRtcSpl_MemSetW16(pw16_corrVec, 0, (4 + 60 + 4));
        w32_tmp = WebRtcSpl_MaxAbsValueW32(pw32_corr, w16_stopPos);
        w16_tmp = 17 - WebRtcSpl_NormW32(w32_tmp);
        w16_tmp = WEBRTC_SPL_MAX(0, w16_tmp);

        WebRtcSpl_VectorBitShiftW32ToW16(pw16_corr, w16_stopPos, pw32_corr, w16_tmp);

        



        w16_tmp = WEBRTC_SPL_MAX(0, WEBRTC_SPL_MAX(w16_startPos,
                inst->timestampsPerCall+inst->ExpandInst.w16_overlap) - w16_decodedLen);
        
        w16_tmp2 = WebRtcSpl_DivW32W16ResW16((WebRtc_Word32) w16_tmp,
            (WebRtc_Word16) (fs_mult << 1));

#ifdef NETEQ_STEREO
    } 

    if ((msInfo->msMode == NETEQ_MASTER) || (msInfo->msMode == NETEQ_MONO))
    {
        
        WebRtcNetEQ_PeakDetection(&pw16_corr[w16_tmp2], w16_stopPos, 1, fs_mult, &w16_bestIndex,
            &w16_bestVal);
        w16_bestIndex += w16_tmp; 
        msInfo->bestIndex = w16_bestIndex;
    }
    else if (msInfo->msMode == NETEQ_SLAVE)
    {
        
        w16_bestIndex = msInfo->bestIndex;
    }
    else
    {
        
        return MASTER_SLAVE_ERROR;
    }

#else 

    
    WebRtcNetEQ_PeakDetection(&pw16_corr[w16_tmp2], w16_stopPos, 1, fs_mult, &w16_bestIndex,
        &w16_bestVal);
    w16_bestIndex += w16_tmp; 

#endif 

    




    while ((w16_bestIndex + w16_decodedLen) < (inst->timestampsPerCall
        + inst->ExpandInst.w16_overlap) || w16_bestIndex + w16_decodedLen < w16_startPos)
    {
        w16_bestIndex += w16_newLen; 
    }
    pw16_decodedOut = pw16_outData + w16_bestIndex;

    
    w16_interpLen = WEBRTC_SPL_MIN(60*fs_mult,
        w16_expandedLen-w16_bestIndex); 
    w16_interpLen = WEBRTC_SPL_MIN(w16_interpLen, w16_decodedLen);
    w16_inc = WebRtcSpl_DivW32W16ResW16(4194,
        fs_mult); 
    if (inst->w16_muteFactor < 16384)
    {
        WebRtcNetEQ_UnmuteSignal(pw16_decoded, &inst->w16_muteFactor, pw16_decoded, w16_inc,
            (WebRtc_Word16) w16_interpLen);
        WebRtcNetEQ_UnmuteSignal(&pw16_decoded[w16_interpLen], &inst->w16_muteFactor,
            &pw16_decodedOut[w16_interpLen], w16_inc,
            (WebRtc_Word16) (w16_decodedLen - w16_interpLen));
    }
    else
    {
        

        WEBRTC_SPL_MEMMOVE_W16(&pw16_decodedOut[w16_interpLen], &pw16_decoded[w16_interpLen],
            (w16_decodedLen-w16_interpLen));
    }

    
    w16_inc = WebRtcSpl_DivW32W16ResW16(16384, (WebRtc_Word16) (w16_interpLen + 1)); 
    w16_startfact = (16384 - w16_inc);
    WEBRTC_SPL_MEMMOVE_W16(pw16_outData, pw16_expanded, w16_bestIndex);
    WebRtcNetEQ_MixVoiceUnvoice(pw16_decodedOut, &pw16_expanded[w16_bestIndex], pw16_decoded,
        &w16_startfact, w16_inc, w16_interpLen);

    inst->w16_mode = MODE_MERGE;
    inst->ExpandInst.w16_consecExp = 0; 

    
    *pw16_len = w16_bestIndex + w16_decodedLen - w16_startPos;

    
    inst->w16_concealedTS += (*pw16_len - w16_decodedLen);
    inst->w16_concealedTS = WEBRTC_SPL_MAX(0, inst->w16_concealedTS);

    
    if (inst->ExpandInst.w16_expandMuteFactor == 0)
    {
        
        inst->statInst.expandedNoiseSamples += (*pw16_len - w16_decodedLen);
    }
    else
    {
        
        inst->statInst.expandedVoiceSamples += (*pw16_len - w16_decodedLen);
    }
    inst->statInst.expandLength += (*pw16_len - w16_decodedLen);


    

    WEBRTC_SPL_MEMCPY_W16(&inst->speechBuffer[inst->curPosition], pw16_outData, w16_startPos);


    

    WEBRTC_SPL_MEMMOVE_W16(pw16_outData, &pw16_outData[w16_startPos], (*pw16_len));

    return 0;
}

#undef     SCRATCH_pw16_expanded
#undef     SCRATCH_pw16_expandedLB
#undef     SCRATCH_pw16_decodedLB
#undef     SCRATCH_pw32_corr
#undef     SCRATCH_pw16_corrVec
#undef     SCRATCH_NETEQ_EXPAND
