



































#include "dtmf_tonegen.h"

#include "signal_processing_library.h"

#include "neteq_error_codes.h"

#ifdef NETEQ_ATEVENT_DECODE
























const int16_t WebRtcNetEQ_dtfm_aTbl8Khz[8] =
{
    27980, 26956, 25701, 24219,
    19073, 16325, 13085, 9315
};

#ifdef NETEQ_WIDEBAND
const int16_t WebRtcNetEQ_dtfm_aTbl16Khz[8]=
{
    31548, 31281, 30951, 30556,
    29144, 28361, 27409, 26258
};
#endif

#ifdef NETEQ_32KHZ_WIDEBAND
const int16_t WebRtcNetEQ_dtfm_aTbl32Khz[8]=
{
    32462, 32394, 32311, 32210,
    31849, 31647, 31400, 31098
};
#endif

#ifdef NETEQ_48KHZ_WIDEBAND
const int16_t WebRtcNetEQ_dtfm_aTbl48Khz[8]=
{
    32632, 32602, 32564, 32520,
    32359, 32268, 32157, 32022
};
#endif






const int16_t WebRtcNetEQ_dtfm_yInitTab8Khz[8] =
{
    8528, 9315, 10163, 11036,
    13323, 14206,15021, 15708
};

#ifdef NETEQ_WIDEBAND
const int16_t WebRtcNetEQ_dtfm_yInitTab16Khz[8]=
{
    4429, 4879, 5380, 5918,
    7490, 8207, 8979, 9801
};
#endif

#ifdef NETEQ_32KHZ_WIDEBAND
const int16_t WebRtcNetEQ_dtfm_yInitTab32Khz[8]=
{
    2235, 2468, 2728, 3010,
    3853, 4249, 4685, 5164
};
#endif

#ifdef NETEQ_48KHZ_WIDEBAND
const int16_t WebRtcNetEQ_dtfm_yInitTab48Khz[8]=
{
    1493, 1649, 1823, 2013,
    2582, 2851, 3148, 3476
};
#endif






const int16_t WebRtcNetEQ_dtfm_dBm0[37] = { 16141, 14386, 12821, 11427, 10184, 9077, 8090,
                                                7210, 6426, 5727, 5104, 4549, 4054, 3614,
                                                3221, 2870, 2558, 2280, 2032, 1811, 1614,
                                                1439, 1282, 1143, 1018, 908, 809, 721, 643,
                                                573, 510, 455, 405, 361, 322, 287, 256 };























int16_t WebRtcNetEQ_DTMFGenerate(dtmf_tone_inst_t *DTMFdecInst, int16_t value,
                                 int16_t volume, int16_t *signal,
                                 uint16_t sampFreq, int16_t extFrameLen)
{
    const int16_t *aTbl; 
    const int16_t *yInitTable; 
    int16_t a1 = 0; 
    int16_t a2 = 0; 
    int i;
    int frameLen; 
    int lowIndex = 0;  
    int highIndex = 4;  
    int32_t tempVal;
    int16_t tempValLow;
    int16_t tempValHigh;

    
    if ((volume < 0) || (volume > 36))
    {
        return DTMF_DEC_PARAMETER_ERROR;
    }

    
    if (extFrameLen < -1)
    {
        return DTMF_DEC_PARAMETER_ERROR;
    }

    
    if (sampFreq == 8000)
    {
        aTbl = WebRtcNetEQ_dtfm_aTbl8Khz;
        yInitTable = WebRtcNetEQ_dtfm_yInitTab8Khz;
        frameLen = 80;
#ifdef NETEQ_WIDEBAND
    }
    else if (sampFreq == 16000)
    {
        aTbl = WebRtcNetEQ_dtfm_aTbl16Khz;
        yInitTable = WebRtcNetEQ_dtfm_yInitTab16Khz;
        frameLen = 160;
#endif
#ifdef NETEQ_32KHZ_WIDEBAND
    }
    else if (sampFreq == 32000)
    {
        aTbl = WebRtcNetEQ_dtfm_aTbl32Khz;
        yInitTable = WebRtcNetEQ_dtfm_yInitTab32Khz;
        frameLen = 320;
#endif
#ifdef NETEQ_48KHZ_WIDEBAND
    }
    else if (sampFreq == 48000)
    {
        aTbl = WebRtcNetEQ_dtfm_aTbl48Khz;
        yInitTable = WebRtcNetEQ_dtfm_yInitTab48Khz;
        frameLen = 480;
#endif
    }
    else
    {
        
        return DTMF_GEN_UNKNOWN_SAMP_FREQ;
    }

    if (extFrameLen >= 0)
    {
        frameLen = extFrameLen;
    }

    
    switch (value)
    {
        case 1:
        case 2:
        case 3:
        case 12: 
        {
            lowIndex = 0; 
            break;
        }
        case 4:
        case 5:
        case 6:
        case 13: 
        {
            lowIndex = 1; 
            break;
        }
        case 7:
        case 8:
        case 9:
        case 14: 
        {
            lowIndex = 2; 
            break;
        }
        case 0:
        case 10:
        case 11:
        case 15: 
        {
            lowIndex = 3; 
            break;
        }
        default:
        {
            return DTMF_DEC_PARAMETER_ERROR;
        }
    } 

    
    switch (value)
    {
        case 1:
        case 4:
        case 7:
        case 10: 
        {
            highIndex = 4; 
            break;
        }
        case 2:
        case 5:
        case 8:
        case 0: 
        {
            highIndex = 5;
            break;
        }
        case 3:
        case 6:
        case 9:
        case 11: 
        {
            highIndex = 6;
            break;
        }
        case 12:
        case 13:
        case 14:
        case 15: 
        {
            highIndex = 7;
            break;
        }
    } 

    
    a1 = aTbl[lowIndex]; 
    a2 = aTbl[highIndex]; 

    if (DTMFdecInst->reinit)
    {
        
        DTMFdecInst->oldOutputLow[0] = yInitTable[lowIndex];
        DTMFdecInst->oldOutputLow[1] = 0;
        DTMFdecInst->oldOutputHigh[0] = yInitTable[highIndex];
        DTMFdecInst->oldOutputHigh[1] = 0;

        
        DTMFdecInst->reinit = 0;
    }

    
    for (i = 0; i < frameLen; i++)
    {

        
        tempValLow
                        = (int16_t) (((WEBRTC_SPL_MUL_16_16(a1, DTMFdecInst->oldOutputLow[1])
                                        + 8192) >> 14) - DTMFdecInst->oldOutputLow[0]);
        tempValHigh
                        = (int16_t) (((WEBRTC_SPL_MUL_16_16(a2, DTMFdecInst->oldOutputHigh[1])
                                        + 8192) >> 14) - DTMFdecInst->oldOutputHigh[0]);

        
        DTMFdecInst->oldOutputLow[0] = DTMFdecInst->oldOutputLow[1];
        DTMFdecInst->oldOutputLow[1] = tempValLow;
        DTMFdecInst->oldOutputHigh[0] = DTMFdecInst->oldOutputHigh[1];
        DTMFdecInst->oldOutputHigh[1] = tempValHigh;

        

        tempVal = WEBRTC_SPL_MUL_16_16(DTMF_AMP_LOW, tempValLow)
                        + WEBRTC_SPL_LSHIFT_W32((int32_t)tempValHigh, 15);

        
        tempVal = (tempVal + 16384) >> 15;

        
        signal[i] = (int16_t) WEBRTC_SPL_RSHIFT_W32(
                               (WEBRTC_SPL_MUL_16_16(tempVal, WebRtcNetEQ_dtfm_dBm0[volume])
                               + 8192), 14); 
    }

    return frameLen;

}

#endif

