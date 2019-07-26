














#include "dsp.h"

#include "signal_processing_library.h"

#include "dsp_helpfunctions.h"
#include "neteq_error_codes.h"

#define PREEMPTIVE_CORR_LEN 50
#define PREEMPTIVE_MIN_LAG 10
#define PREEMPTIVE_MAX_LAG 60
#define PREEMPTIVE_DOWNSAMPLED_LEN (PREEMPTIVE_CORR_LEN + PREEMPTIVE_MAX_LAG)











#define     SCRATCH_PW16_DS_SPEECH           0
#define     SCRATCH_PW32_CORR                PREEMPTIVE_DOWNSAMPLED_LEN
#define     SCRATCH_PW16_CORR                0
































int WebRtcNetEQ_PreEmptiveExpand(DSPInst_t *inst,
#ifdef SCRATCH
                                 int16_t *pw16_scratchPtr,
#endif
                                 const int16_t *pw16_decoded, int len, int oldDataLen,
                                 int16_t *pw16_outData, int16_t *pw16_len,
                                 int16_t BGNonly)
{

#ifdef SCRATCH
    
    int16_t *pw16_downSampSpeech = pw16_scratchPtr + SCRATCH_PW16_DS_SPEECH;
    int32_t *pw32_corr = (int32_t*) (pw16_scratchPtr + SCRATCH_PW32_CORR);
    int16_t *pw16_corr = pw16_scratchPtr + SCRATCH_PW16_CORR;
#else
    
    int16_t pw16_downSampSpeech[PREEMPTIVE_DOWNSAMPLED_LEN];
    int32_t pw32_corr[PREEMPTIVE_CORR_LEN];
    int16_t pw16_corr[PREEMPTIVE_CORR_LEN];
#endif
    int16_t w16_decodedMax = 0;
    int16_t w16_tmp = 0;
    int16_t w16_tmp2;
    int32_t w32_tmp;
    int32_t w32_tmp2;

    const int16_t w16_startLag = PREEMPTIVE_MIN_LAG;
    const int16_t w16_endLag = PREEMPTIVE_MAX_LAG;
    const int16_t w16_corrLen = PREEMPTIVE_CORR_LEN;
    const int16_t *pw16_vec1, *pw16_vec2;
    int16_t *pw16_vectmp;
    int16_t w16_inc, w16_startfact;
    int16_t w16_bestIndex, w16_bestVal;
    int16_t w16_VAD = 1;
    int16_t fsMult;
    int16_t fsMult120;
    int32_t w32_en1, w32_en2, w32_cc;
    int16_t w16_en1, w16_en2;
    int16_t w16_en1Scale, w16_en2Scale;
    int16_t w16_sqrtEn1En2;
    int16_t w16_bestCorr = 0;
    int ok;

#ifdef NETEQ_STEREO
    MasterSlaveInfo *msInfo = inst->msInfo;
#endif

    fsMult = WebRtcNetEQ_CalcFsMult(inst->fs); 

    
    fsMult120 = (int16_t) WEBRTC_SPL_MUL_16_16(fsMult, 120); 

    inst->ExpandInst.w16_consecExp = 0; 

    



    if (len < (int16_t) WEBRTC_SPL_MUL_16_16((120 + 119), fsMult) || oldDataLen >= len
        - inst->ExpandInst.w16_overlap)
    {
        
        inst->w16_mode = MODE_UNSUCCESS_PREEMPTIVE;
        *pw16_len = len;

        
        

        WEBRTC_SPL_MEMMOVE_W16(pw16_outData, pw16_decoded, (int16_t) len);

        return NETEQ_OTHER_ERROR;
    }

    
    
    

    
    if (BGNonly)
    {
        
        w16_bestIndex = DEFAULT_TIME_ADJUST * (fsMult << 3); 

        
        if (w16_bestIndex > len)
        { 
            inst->w16_mode = MODE_UNSUCCESS_PREEMPTIVE;
            *pw16_len = len;


            

            WEBRTC_SPL_MEMMOVE_W16(pw16_outData, pw16_decoded, (int16_t) len);

            return NETEQ_OTHER_ERROR;
        }

        
        *pw16_len = len + w16_bestIndex;


        

        WEBRTC_SPL_MEMMOVE_W16(pw16_outData, pw16_decoded, len);
        WEBRTC_SPL_MEMCPY_W16(&pw16_outData[len], pw16_decoded, w16_bestIndex);

        
        inst->w16_mode = MODE_LOWEN_PREEMPTIVE;

        
        inst->statInst.preemptiveLength += w16_bestIndex;
        
        inst->activity_stats.preemptive_expand_bgn_samples += w16_bestIndex;

        return 0;
    } 

#ifdef NETEQ_STEREO

    
    if (msInfo == NULL)
    {
        
        return MASTER_SLAVE_ERROR;
    }

    if ((msInfo->msMode == NETEQ_MASTER) || (msInfo->msMode == NETEQ_MONO))
    {
        

#endif








        w16_decodedMax = WebRtcSpl_MaxAbsValueW16(pw16_decoded, (int16_t) len);

        
        ok = WebRtcNetEQ_DownSampleTo4kHz(pw16_decoded, len, inst->fs, pw16_downSampSpeech,
            PREEMPTIVE_DOWNSAMPLED_LEN, 1 );
        if (ok != 0)
        {
            
            inst->w16_mode = MODE_UNSUCCESS_PREEMPTIVE;
            *pw16_len = len;


            

            WEBRTC_SPL_MEMMOVE_W16(pw16_outData, pw16_decoded, (int16_t) len);

            return NETEQ_OTHER_ERROR;
        }

        



        w16_tmp = 6 - WebRtcSpl_NormW32(WEBRTC_SPL_MUL_16_16(w16_decodedMax, w16_decodedMax));
        w16_tmp = WEBRTC_SPL_MAX(0, w16_tmp);

        WebRtcNetEQ_CrossCorr(
            pw32_corr, &pw16_downSampSpeech[w16_endLag],
            &pw16_downSampSpeech[w16_endLag - w16_startLag], w16_corrLen,
            (int16_t) (w16_endLag - w16_startLag), w16_tmp, -1);

        
        w32_tmp = WebRtcSpl_MaxAbsValueW32(pw32_corr, w16_corrLen);
        w16_tmp = 17 - WebRtcSpl_NormW32(w32_tmp);
        w16_tmp = WEBRTC_SPL_MAX(0, w16_tmp);

        WebRtcSpl_VectorBitShiftW32ToW16(pw16_corr, w16_corrLen, pw32_corr, w16_tmp);

        
        
        w16_tmp = WebRtcSpl_DivW32W16ResW16((int32_t) (NETEQ_MAX_OUTPUT_SIZE - len),
            (int16_t) (fsMult << 1)) - w16_startLag;
        w16_tmp = WEBRTC_SPL_MIN(w16_corrLen, w16_tmp); 

#ifdef NETEQ_STEREO
    } 

    if ((msInfo->msMode == NETEQ_MASTER) || (msInfo->msMode == NETEQ_MONO))
    {
        
        WebRtcNetEQ_PeakDetection(pw16_corr, w16_tmp, 1, fsMult, &w16_bestIndex, &w16_bestVal);
        

        
        w16_bestIndex = w16_bestIndex + w16_startLag * WEBRTC_SPL_LSHIFT_W16(fsMult, 1);
        

        msInfo->bestIndex = w16_bestIndex;
    }
    else if (msInfo->msMode == NETEQ_SLAVE)
    {
        if (msInfo->extraInfo == PE_EXP_FAIL)
        {
            
            w16_bestIndex = 0;
        }
        else
        {
            
            w16_bestIndex = msInfo->bestIndex;
        }
    }
    else
    {
        
        return (MASTER_SLAVE_ERROR);
    }

#else 

    
    WebRtcNetEQ_PeakDetection(pw16_corr, w16_tmp, 1, fsMult, &w16_bestIndex, &w16_bestVal);
    

    
    w16_bestIndex = w16_bestIndex + w16_startLag * WEBRTC_SPL_LSHIFT_W16(fsMult, 1);
    

#endif 

#ifdef NETEQ_STEREO

    if ((msInfo->msMode == NETEQ_MASTER) || (msInfo->msMode == NETEQ_MONO))
    {
        

#endif










        w16_tmp = (31
            - WebRtcSpl_NormW32(WEBRTC_SPL_MUL_16_16(w16_decodedMax, w16_decodedMax)));
        w16_tmp += (31 - WebRtcSpl_NormW32(w16_bestIndex));
        w16_tmp -= 31;
        w16_tmp = WEBRTC_SPL_MAX(0, w16_tmp);

        
        pw16_vec1 = &pw16_decoded[fsMult120 - w16_bestIndex];
        
        pw16_vec2 = &pw16_decoded[fsMult120];

        
        w32_en1 = WebRtcNetEQ_DotW16W16((int16_t*) pw16_vec1,
            (int16_t*) pw16_vec1, w16_bestIndex, w16_tmp);
        w32_en2 = WebRtcNetEQ_DotW16W16((int16_t*) pw16_vec2,
            (int16_t*) pw16_vec2, w16_bestIndex, w16_tmp);

        
        w32_cc = WebRtcNetEQ_DotW16W16((int16_t*) pw16_vec1, (int16_t*) pw16_vec2,
            w16_bestIndex, w16_tmp);

        

        w32_tmp = WEBRTC_SPL_RSHIFT_W32(w32_en1 + w32_en2, 4); 
        if (inst->BGNInst.w16_initialized == 1)
        {
            w32_tmp2 = inst->BGNInst.w32_energy;
        }
        else
        {
            
            w32_tmp2 = 75000;
        }
        w16_tmp2 = 16 - WebRtcSpl_NormW32(w32_tmp2);
        w16_tmp2 = WEBRTC_SPL_MAX(0, w16_tmp2);
        w32_tmp = WEBRTC_SPL_RSHIFT_W32(w32_tmp, w16_tmp2);
        w16_tmp2 = (int16_t) WEBRTC_SPL_RSHIFT_W32(w32_tmp2, w16_tmp2);
        w32_tmp2 = WEBRTC_SPL_MUL_16_16(w16_bestIndex, w16_tmp2);

        
        
        if (WebRtcSpl_NormW32(w32_tmp) < WEBRTC_SPL_LSHIFT_W32(w16_tmp,1))
        {
            
            int16_t tempshift = WebRtcSpl_NormW32(w32_tmp);
            w32_tmp = WEBRTC_SPL_LSHIFT_W32(w32_tmp, tempshift);
            w32_tmp2 = WEBRTC_SPL_RSHIFT_W32(w32_tmp2,
                WEBRTC_SPL_LSHIFT_W32(w16_tmp,1) - tempshift);
        }
        else
        {
            w32_tmp = WEBRTC_SPL_LSHIFT_W32(w32_tmp,
                WEBRTC_SPL_LSHIFT_W32(w16_tmp,1));
        }

        if (w32_tmp <= w32_tmp2) 
        {
            
            w16_VAD = 0;
            w16_bestCorr = 0; 

            

            w16_bestIndex = WEBRTC_SPL_MIN( w16_bestIndex, len - oldDataLen );
        }
        else
        {
            
            w16_VAD = 1;

            

            
            w16_en1Scale = 16 - WebRtcSpl_NormW32(w32_en1);
            w16_en1Scale = WEBRTC_SPL_MAX(0, w16_en1Scale);
            w16_en2Scale = 16 - WebRtcSpl_NormW32(w32_en2);
            w16_en2Scale = WEBRTC_SPL_MAX(0, w16_en2Scale);

            
            if ((w16_en1Scale + w16_en2Scale) & 1)
            {
                w16_en1Scale += 1;
            }

            
            w16_en1 = (int16_t) WEBRTC_SPL_RSHIFT_W32(w32_en1, w16_en1Scale);
            w16_en2 = (int16_t) WEBRTC_SPL_RSHIFT_W32(w32_en2, w16_en2Scale);

            
            w32_tmp = WEBRTC_SPL_MUL_16_16(w16_en1, w16_en2);

            
            w16_sqrtEn1En2 = (int16_t) WebRtcSpl_SqrtFloor(w32_tmp);

            
            w16_tmp = 14 - ((w16_en1Scale + w16_en2Scale) >> 1);
            w32_cc = WEBRTC_SPL_SHIFT_W32(w32_cc, w16_tmp);
            w32_cc = WEBRTC_SPL_MAX(0, w32_cc); 
            w16_bestCorr = (int16_t) WebRtcSpl_DivW32W16(w32_cc, w16_sqrtEn1En2);
            w16_bestCorr = WEBRTC_SPL_MIN(16384, w16_bestCorr); 
        }

#ifdef NETEQ_STEREO

    } 

#endif 

    
    
    

    

#ifdef NETEQ_STEREO
    if (((((w16_bestCorr > 14746) && (oldDataLen <= fsMult120)) || (w16_VAD == 0))
        && (msInfo->msMode != NETEQ_SLAVE)) || ((msInfo->msMode == NETEQ_SLAVE)
        && (msInfo->extraInfo != PE_EXP_FAIL)))
#else
    if (((w16_bestCorr > 14746) && (oldDataLen <= fsMult120))
        || (w16_VAD == 0))
#endif
    {
        

        
        int16_t w16_startIndex = WEBRTC_SPL_MAX(oldDataLen, fsMult120);

        



        w16_inc = (int16_t) WebRtcSpl_DivW32W16((int32_t) 16384,
            (int16_t) (w16_bestIndex + 1)); 

        
        w16_startfact = 16384 - w16_inc;

        
        pw16_vec1 = &pw16_decoded[w16_startIndex - w16_bestIndex];
        
        pw16_vec2 = &pw16_decoded[w16_startIndex];


        

        WEBRTC_SPL_MEMMOVE_W16(pw16_outData, pw16_decoded, w16_startIndex);

        
        pw16_vectmp = pw16_outData + w16_startIndex;
        
        WebRtcNetEQ_MixVoiceUnvoice(pw16_vectmp, (int16_t*) pw16_vec2,
            (int16_t*) pw16_vec1, &w16_startfact, w16_inc, w16_bestIndex);

        
        
        pw16_vec2 = &pw16_decoded[w16_startIndex];
        WEBRTC_SPL_MEMMOVE_W16(&pw16_outData[w16_startIndex + w16_bestIndex], pw16_vec2,
            (int16_t) (len - w16_startIndex));

        
        if (w16_VAD)
        {
            inst->w16_mode = MODE_SUCCESS_PREEMPTIVE;
        }
        else
        {
            inst->w16_mode = MODE_LOWEN_PREEMPTIVE;
        }

        
        *pw16_len = len + w16_bestIndex;

        
        inst->statInst.preemptiveLength += w16_bestIndex;
        
        inst->activity_stats.preemptive_expand_normal_samples += w16_bestIndex;
        return 0;
    }
    else
    {
        

#ifdef NETEQ_STEREO
        
        if (msInfo->msMode == NETEQ_MASTER)
        {
            msInfo->extraInfo = PE_EXP_FAIL;
        }
#endif

        
        inst->w16_mode = MODE_UNSUCCESS_PREEMPTIVE;

        
        *pw16_len = len;


        

        WEBRTC_SPL_MEMMOVE_W16(pw16_outData, pw16_decoded, (int16_t) len);

        return 0;
    }
}

#undef     SCRATCH_PW16_DS_SPEECH
#undef     SCRATCH_PW32_CORR
#undef     SCRATCH_PW16_CORR
