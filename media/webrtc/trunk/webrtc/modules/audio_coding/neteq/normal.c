












#include "dsp.h"

#include "signal_processing_library.h"

#include "dsp_helpfunctions.h"











#define     SCRATCH_PW16_EXPANDED           0
#if (defined(NETEQ_48KHZ_WIDEBAND)) 
#define     SCRATCH_NETEQ_EXPAND    756
#elif (defined(NETEQ_32KHZ_WIDEBAND)) 
#define     SCRATCH_NETEQ_EXPAND    504
#elif (defined(NETEQ_WIDEBAND)) 
#define     SCRATCH_NETEQ_EXPAND    252
#else    
#define     SCRATCH_NETEQ_EXPAND    126
#endif



























int WebRtcNetEQ_Normal(DSPInst_t *inst,
#ifdef SCRATCH
                       WebRtc_Word16 *pw16_scratchPtr,
#endif
                       WebRtc_Word16 *pw16_decoded, WebRtc_Word16 len,
                       WebRtc_Word16 *pw16_outData, WebRtc_Word16 *pw16_len)
{

    int i;
    WebRtc_Word16 fs_mult;
    WebRtc_Word16 fs_shift;
    WebRtc_Word32 w32_En_speech;
    WebRtc_Word16 enLen;
    WebRtc_Word16 w16_muted;
    WebRtc_Word16 w16_inc, w16_frac;
    WebRtc_Word16 w16_tmp;
    WebRtc_Word32 w32_tmp;

    
    if (len < 0)
    {
        
        return (-1);
    }

    if (len == 0)
    {
        
        *pw16_len = len;
        return (len);
    }

    fs_mult = WebRtcSpl_DivW32W16ResW16(inst->fs, 8000);
    fs_shift = 30 - WebRtcSpl_NormW32(fs_mult); 

    



    if (inst->w16_mode == MODE_EXPAND || inst->w16_mode == MODE_FADE_TO_BGN)
    {

        
#ifdef SCRATCH
        WebRtc_Word16 *pw16_expanded = pw16_scratchPtr + SCRATCH_PW16_EXPANDED;
#else
        WebRtc_Word16 pw16_expanded[FSMULT * 125];
#endif
        WebRtc_Word16 expandedLen = 0;
        WebRtc_Word16 w16_decodedMax;

        
        w16_decodedMax = WebRtcSpl_MaxAbsValueW16(pw16_decoded, (WebRtc_Word16) len);

        
        
        inst->ExpandInst.w16_lagsPosition = 0;
        inst->ExpandInst.w16_lagsDirection = 0;
        inst->ExpandInst.w16_stopMuting = 1; 

        
        WebRtcNetEQ_Expand(inst,
#ifdef SCRATCH
            pw16_scratchPtr + SCRATCH_NETEQ_EXPAND,
#endif
            pw16_expanded, &expandedLen, (WebRtc_Word16) (inst->w16_mode == MODE_FADE_TO_BGN));

        inst->ExpandInst.w16_stopMuting = 0; 
        inst->ExpandInst.w16_consecExp = 0; 

        
        if (inst->w16_mode == MODE_FADE_TO_BGN)
        {
            
            inst->w16_muteFactor = 0;
        }
        else
        {
            
            inst->w16_muteFactor
                = (WebRtc_Word16) WEBRTC_SPL_MUL_16_16_RSFT(inst->w16_muteFactor,
                    inst->ExpandInst.w16_expandMuteFactor, 14);
        }

        
        enLen = WEBRTC_SPL_MIN(fs_mult<<6, len); 
        w16_tmp = 6 + fs_shift - WebRtcSpl_NormW32(
            WEBRTC_SPL_MUL_16_16(w16_decodedMax, w16_decodedMax));
        w16_tmp = WEBRTC_SPL_MAX(w16_tmp, 0);
        w32_En_speech = WebRtcNetEQ_DotW16W16(pw16_decoded, pw16_decoded, enLen, w16_tmp);
        w32_En_speech = WebRtcSpl_DivW32W16(w32_En_speech, (WebRtc_Word16) (enLen >> w16_tmp));

        if ((w32_En_speech != 0) && (w32_En_speech > inst->BGNInst.w32_energy))
        {
            
            w16_tmp = WebRtcSpl_NormW32(w32_En_speech) - 16;
            
            w32_tmp = WEBRTC_SPL_SHIFT_W32(inst->BGNInst.w32_energy, (w16_tmp+14));
            w16_tmp = (WebRtc_Word16) WEBRTC_SPL_SHIFT_W32(w32_En_speech, w16_tmp);
            w16_tmp = (WebRtc_Word16) WebRtcSpl_DivW32W16(w32_tmp, w16_tmp);
            w16_muted = (WebRtc_Word16) WebRtcSpl_SqrtFloor(
                WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32) w16_tmp,
                    14)); 
        }
        else
        {
            w16_muted = 16384; 
        }
        if (w16_muted > inst->w16_muteFactor)
        {
            inst->w16_muteFactor = WEBRTC_SPL_MIN(w16_muted, 16384);
        }

        
        w16_inc = WebRtcSpl_DivW32W16ResW16(64, fs_mult);
        for (i = 0; i < len; i++)
        {
            
            w32_tmp = WEBRTC_SPL_MUL_16_16(pw16_decoded[i], inst->w16_muteFactor);
            
            pw16_decoded[i] = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32((w32_tmp + 8192), 14);
            
            inst->w16_muteFactor = WEBRTC_SPL_MIN(16384, (inst->w16_muteFactor+w16_inc));
        }

        



        fs_shift = WEBRTC_SPL_MIN(3, fs_shift); 
        w16_inc = 4 >> fs_shift;
        w16_frac = w16_inc;
        for (i = 0; i < 8 * fs_mult; i++)
        {
            pw16_decoded[i] = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(
                (WEBRTC_SPL_MUL_16_16(w16_frac, pw16_decoded[i]) +
                    WEBRTC_SPL_MUL_16_16((32 - w16_frac), pw16_expanded[i]) + 8),
                5);
            w16_frac += w16_inc;
        }

#ifdef NETEQ_CNG_CODEC
    }
    else if (inst->w16_mode==MODE_RFC3389CNG)
    { 
        WebRtc_Word16 pw16_CngInterp[32];
        
        inst->w16_muteFactor = 16384;
        if (inst->CNG_Codec_inst != NULL)
        {
            
            if(WebRtcCng_Generate(inst->CNG_Codec_inst,pw16_CngInterp, 32, 0)<0)
            {
                
                WebRtcSpl_MemSetW16(pw16_CngInterp, 0, 32);
            }
        }
        else
        {
            



            WEBRTC_SPL_MEMCPY_W16(pw16_CngInterp, pw16_decoded, fs_mult * 8);
        }
        



        fs_shift = WEBRTC_SPL_MIN(3, fs_shift); 
        w16_inc = 4>>fs_shift;
        w16_frac = w16_inc;
        for (i = 0; i < 8 * fs_mult; i++)
        {
            pw16_decoded[i] = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(
                (WEBRTC_SPL_MUL_16_16(w16_frac, pw16_decoded[i]) +
                    WEBRTC_SPL_MUL_16_16((32-w16_frac), pw16_CngInterp[i]) + 8),
                5);
            w16_frac += w16_inc;
        }
#endif

    }
    else if (inst->w16_muteFactor < 16384)
    {
        




        w16_inc = WebRtcSpl_DivW32W16ResW16(64, fs_mult);
        for (i = 0; i < len; i++)
        {
            
            w32_tmp = WEBRTC_SPL_MUL_16_16(pw16_decoded[i], inst->w16_muteFactor);
            
            pw16_decoded[i] = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32((w32_tmp + 8192), 14);
            
            inst->w16_muteFactor = WEBRTC_SPL_MIN(16384, (inst->w16_muteFactor+w16_inc));
        }
    }

    WEBRTC_SPL_MEMMOVE_W16(pw16_outData, pw16_decoded, len);

    inst->w16_mode = MODE_NORMAL;
    *pw16_len = len;
    return (len);

}

#undef SCRATCH_PW16_EXPANDED
#undef SCRATCH_NETEQ_EXPAND

