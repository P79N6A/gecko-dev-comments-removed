













#include "mcu.h"

#include "dtmf_buffer.h"
#include "neteq_error_codes.h"

int WebRtcNetEQ_McuSetFs(MCUInst_t *inst, WebRtc_UWord16 fs)
{
    WebRtc_Word16 ok = 0;

    switch (fs)
    {
        case 8000:
        {
#ifdef NETEQ_ATEVENT_DECODE
            ok = WebRtcNetEQ_DtmfDecoderInit(&inst->DTMF_inst, 8000, 560);
#endif
            inst->timestampsPerCall = inst->millisecondsPerCall * 8;
            break;
        }

#ifdef NETEQ_WIDEBAND
        case 16000:
        {
#ifdef NETEQ_ATEVENT_DECODE
            ok = WebRtcNetEQ_DtmfDecoderInit(&inst->DTMF_inst, 16000, 1120);
#endif
            inst->timestampsPerCall = inst->millisecondsPerCall * 16;
            break;
        }
#endif

#ifdef NETEQ_32KHZ_WIDEBAND
        case 32000:
        {
#ifdef NETEQ_ATEVENT_DECODE
            ok = WebRtcNetEQ_DtmfDecoderInit(&inst->DTMF_inst, 32000, 2240);
#endif
            inst->timestampsPerCall = inst->millisecondsPerCall * 32;
            break;
        }
#endif

#ifdef NETEQ_48KHZ_WIDEBAND
        case 48000:
        {
#ifdef NETEQ_ATEVENT_DECODE
            ok = WebRtcNetEQ_DtmfDecoderInit(&inst->DTMF_inst, 48000, 3360);
#endif
            inst->timestampsPerCall = inst->millisecondsPerCall * 48;
            break;
        }
#endif

        default:
        {
            
            return CODEC_DB_UNSUPPORTED_FS;
        }
    } 

    inst->fs = fs;

    return ok;
}
