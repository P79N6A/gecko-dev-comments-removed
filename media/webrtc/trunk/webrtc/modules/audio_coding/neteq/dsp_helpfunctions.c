













#include "dsp_helpfunctions.h"


int16_t WebRtcNetEQ_CalcFsMult(uint16_t fsHz)
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


int WebRtcNetEQ_DownSampleTo4kHz(const int16_t *in, int inLen, uint16_t inFsHz,
                                 int16_t *out, int outLen, int compensateDelay)
{
    int16_t *B; 
    int16_t Blen; 
    int16_t filterDelay; 
    int16_t factor; 
    int ok;

    
    


    switch (inFsHz)
    {
        case 8000:
        {
            Blen = 3;
            factor = 2;
            B = (int16_t*) WebRtcNetEQ_kDownsample8kHzTbl;
            filterDelay = 1 + 1;
            break;
        }
#ifdef NETEQ_WIDEBAND
            case 16000:
            {
                Blen = 5;
                factor = 4;
                B = (int16_t*) WebRtcNetEQ_kDownsample16kHzTbl;
                filterDelay = 2 + 1;
                break;
            }
#endif
#ifdef NETEQ_32KHZ_WIDEBAND
            case 32000:
            {
                Blen = 7;
                factor = 8;
                B = (int16_t*) WebRtcNetEQ_kDownsample32kHzTbl;
                filterDelay = 3 + 1;
                break;
            }
#endif
#ifdef NETEQ_48KHZ_WIDEBAND
            case 48000:
            {
                Blen = 7;
                factor = 12;
                B = (int16_t*) WebRtcNetEQ_kDownsample48kHzTbl;
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

    ok = WebRtcSpl_DownsampleFast((int16_t*) &in[Blen - 1],
        (int16_t) (inLen - (Blen - 1)), 
        out, (int16_t) outLen, 
        B, Blen, factor, filterDelay); 

    return ok; 

}

