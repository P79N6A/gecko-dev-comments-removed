













#include "dsp_helpfunctions.h"


WebRtc_Word16 WebRtcNetEQ_CalcFsMult(WebRtc_UWord16 fsHz)
{
    switch (fsHz)
    {
        case 8000:
        {
            return 1;
        }
        case 16000:
        {
            return 2;
        }
        case 32000:
        {
            return 4;
        }
        case 48000:
        {
            return 6;
        }
        default:
        {
            return 1;
        }
    }
}


int WebRtcNetEQ_DownSampleTo4kHz(const WebRtc_Word16 *in, int inLen, WebRtc_UWord16 inFsHz,
                                 WebRtc_Word16 *out, int outLen, int compensateDelay)
{
    WebRtc_Word16 *B; 
    WebRtc_Word16 Blen; 
    WebRtc_Word16 filterDelay; 
    WebRtc_Word16 factor; 
    int ok;

    
    


    switch (inFsHz)
    {
        case 8000:
        {
            Blen = 3;
            factor = 2;
            B = (WebRtc_Word16*) WebRtcNetEQ_kDownsample8kHzTbl;
            filterDelay = 1 + 1;
            break;
        }
#ifdef NETEQ_WIDEBAND
            case 16000:
            {
                Blen = 5;
                factor = 4;
                B = (WebRtc_Word16*) WebRtcNetEQ_kDownsample16kHzTbl;
                filterDelay = 2 + 1;
                break;
            }
#endif
#ifdef NETEQ_32KHZ_WIDEBAND
            case 32000:
            {
                Blen = 7;
                factor = 8;
                B = (WebRtc_Word16*) WebRtcNetEQ_kDownsample32kHzTbl;
                filterDelay = 3 + 1;
                break;
            }
#endif
#ifdef NETEQ_48KHZ_WIDEBAND
            case 48000:
            {
                Blen = 7;
                factor = 12;
                B = (WebRtc_Word16*) WebRtcNetEQ_kDownsample48kHzTbl;
                filterDelay = 3 + 1;
                break;
            }
#endif
        default:
        {
            
            return -1;
        }
    }

    if (!compensateDelay)
    {
        
        filterDelay = 0;
    }

    ok = WebRtcSpl_DownsampleFast((WebRtc_Word16*) &in[Blen - 1],
        (WebRtc_Word16) (inLen - (Blen - 1)), 
        out, (WebRtc_Word16) outLen, 
        B, Blen, factor, filterDelay); 

    return ok; 

}

