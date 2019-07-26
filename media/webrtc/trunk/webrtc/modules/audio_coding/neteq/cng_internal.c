














#include "dsp.h"

#include "signal_processing_library.h"
#include "webrtc_cng.h"

#include "dsp_helpfunctions.h"
#include "neteq_error_codes.h"




















#ifdef NETEQ_CNG_CODEC


int WebRtcNetEQ_Cng(DSPInst_t *inst, WebRtc_Word16 *pw16_outData, int len)
{
    WebRtc_Word16 w16_winMute = 0; 
    WebRtc_Word16 w16_winUnMute = 0; 
    WebRtc_Word16 w16_winMuteInc = 0; 
    WebRtc_Word16 w16_winUnMuteInc = 0; 
    int i;

    



    if (inst->w16_mode != MODE_RFC3389CNG)
    {
        

        
        if (WebRtcCng_Generate(inst->CNG_Codec_inst, pw16_outData,
            (WebRtc_Word16) (len + inst->ExpandInst.w16_overlap), 1) < 0)
        {
            
            return -WebRtcCng_GetErrorCodeDec(inst->CNG_Codec_inst);
        }

        
        if (inst->fs == 8000)
        {
            
            w16_winMute = NETEQ_OVERLAP_WINMUTE_8KHZ_START;
            w16_winMuteInc = NETEQ_OVERLAP_WINMUTE_8KHZ_INC;
            w16_winUnMute = NETEQ_OVERLAP_WINUNMUTE_8KHZ_START;
            w16_winUnMuteInc = NETEQ_OVERLAP_WINUNMUTE_8KHZ_INC;
#ifdef NETEQ_WIDEBAND
        }
        else if (inst->fs == 16000)
        {
            
            w16_winMute = NETEQ_OVERLAP_WINMUTE_16KHZ_START;
            w16_winMuteInc = NETEQ_OVERLAP_WINMUTE_16KHZ_INC;
            w16_winUnMute = NETEQ_OVERLAP_WINUNMUTE_16KHZ_START;
            w16_winUnMuteInc = NETEQ_OVERLAP_WINUNMUTE_16KHZ_INC;
#endif
#ifdef NETEQ_32KHZ_WIDEBAND
        }
        else if (inst->fs == 32000)
        {
            
            w16_winMute = NETEQ_OVERLAP_WINMUTE_32KHZ_START;
            w16_winMuteInc = NETEQ_OVERLAP_WINMUTE_32KHZ_INC;
            w16_winUnMute = NETEQ_OVERLAP_WINUNMUTE_32KHZ_START;
            w16_winUnMuteInc = NETEQ_OVERLAP_WINUNMUTE_32KHZ_INC;
#endif
#ifdef NETEQ_48KHZ_WIDEBAND
        }
        else if (inst->fs == 48000)
        {
            
            w16_winMute = NETEQ_OVERLAP_WINMUTE_48KHZ_START;
            w16_winMuteInc = NETEQ_OVERLAP_WINMUTE_48KHZ_INC;
            w16_winUnMute = NETEQ_OVERLAP_WINUNMUTE_48KHZ_START;
            w16_winUnMuteInc = NETEQ_OVERLAP_WINUNMUTE_48KHZ_INC;
#endif
        }
        else
        {
            
            return NETEQ_OTHER_ERROR;
        }

        
        for (i = 0; i < inst->ExpandInst.w16_overlap; i++)
        {
            
            inst->ExpandInst.pw16_overlapVec[i] = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(
                WEBRTC_SPL_MUL_16_16(
                    inst->ExpandInst.pw16_overlapVec[i], w16_winMute) +
                WEBRTC_SPL_MUL_16_16(pw16_outData[i], w16_winUnMute)
                + 16384, 15); 

            w16_winMute += w16_winMuteInc; 
            w16_winUnMute += w16_winUnMuteInc; 

        }

        




        WEBRTC_SPL_MEMMOVE_W16(pw16_outData, pw16_outData+inst->ExpandInst.w16_overlap, len);

    }
    else
    {
        

        
        if (WebRtcCng_Generate(inst->CNG_Codec_inst, pw16_outData, (WebRtc_Word16) len, 0) < 0)
        {
            
            return -WebRtcCng_GetErrorCodeDec(inst->CNG_Codec_inst);
        }
    }

    return 0;

}

#endif

