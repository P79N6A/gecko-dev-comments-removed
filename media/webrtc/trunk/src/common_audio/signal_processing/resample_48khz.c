
















#include <string.h>
#include "signal_processing_library.h"
#include "resample_by_2_internal.h"






void WebRtcSpl_Resample48khzTo16khz(const WebRtc_Word16* in, WebRtc_Word16* out,
                                    WebRtcSpl_State48khzTo16khz* state, WebRtc_Word32* tmpmem)
{
    
    
    
    
    WebRtcSpl_LPBy2ShortToInt(in, 480, tmpmem + 16, state->S_48_48);

    
    
    
    
    
    memcpy(tmpmem + 8, state->S_48_32, 8 * sizeof(WebRtc_Word32));
    memcpy(state->S_48_32, tmpmem + 488, 8 * sizeof(WebRtc_Word32));
    WebRtcSpl_Resample48khzTo32khz(tmpmem + 8, tmpmem, 160);

    
    
    
    
    WebRtcSpl_DownBy2IntToShort(tmpmem, 320, out, state->S_32_16);
}


void WebRtcSpl_ResetResample48khzTo16khz(WebRtcSpl_State48khzTo16khz* state)
{
    memset(state->S_48_48, 0, 16 * sizeof(WebRtc_Word32));
    memset(state->S_48_32, 0, 8 * sizeof(WebRtc_Word32));
    memset(state->S_32_16, 0, 8 * sizeof(WebRtc_Word32));
}






void WebRtcSpl_Resample16khzTo48khz(const WebRtc_Word16* in, WebRtc_Word16* out,
                                    WebRtcSpl_State16khzTo48khz* state, WebRtc_Word32* tmpmem)
{
    
    
    
    
    WebRtcSpl_UpBy2ShortToInt(in, 160, tmpmem + 16, state->S_16_32);

    
    
    
    
    
    memcpy(tmpmem + 8, state->S_32_24, 8 * sizeof(WebRtc_Word32));
    memcpy(state->S_32_24, tmpmem + 328, 8 * sizeof(WebRtc_Word32));
    WebRtcSpl_Resample32khzTo24khz(tmpmem + 8, tmpmem, 80);

    
    
    
    
    WebRtcSpl_UpBy2IntToShort(tmpmem, 240, out, state->S_24_48);
}


void WebRtcSpl_ResetResample16khzTo48khz(WebRtcSpl_State16khzTo48khz* state)
{
    memset(state->S_16_32, 0, 8 * sizeof(WebRtc_Word32));
    memset(state->S_32_24, 0, 8 * sizeof(WebRtc_Word32));
    memset(state->S_24_48, 0, 8 * sizeof(WebRtc_Word32));
}






void WebRtcSpl_Resample48khzTo8khz(const WebRtc_Word16* in, WebRtc_Word16* out,
                                   WebRtcSpl_State48khzTo8khz* state, WebRtc_Word32* tmpmem)
{
    
    
    
    
    WebRtcSpl_DownBy2ShortToInt(in, 480, tmpmem + 256, state->S_48_24);

    
    
    
    
    WebRtcSpl_LPBy2IntToInt(tmpmem + 256, 240, tmpmem + 16, state->S_24_24);

    
    
    
    
    
    memcpy(tmpmem + 8, state->S_24_16, 8 * sizeof(WebRtc_Word32));
    memcpy(state->S_24_16, tmpmem + 248, 8 * sizeof(WebRtc_Word32));
    WebRtcSpl_Resample48khzTo32khz(tmpmem + 8, tmpmem, 80);

    
    
    
    
    WebRtcSpl_DownBy2IntToShort(tmpmem, 160, out, state->S_16_8);
}


void WebRtcSpl_ResetResample48khzTo8khz(WebRtcSpl_State48khzTo8khz* state)
{
    memset(state->S_48_24, 0, 8 * sizeof(WebRtc_Word32));
    memset(state->S_24_24, 0, 16 * sizeof(WebRtc_Word32));
    memset(state->S_24_16, 0, 8 * sizeof(WebRtc_Word32));
    memset(state->S_16_8, 0, 8 * sizeof(WebRtc_Word32));
}






void WebRtcSpl_Resample8khzTo48khz(const WebRtc_Word16* in, WebRtc_Word16* out,
                                   WebRtcSpl_State8khzTo48khz* state, WebRtc_Word32* tmpmem)
{
    
    
    
    
    WebRtcSpl_UpBy2ShortToInt(in, 80, tmpmem + 264, state->S_8_16);

    
    
    
    
    
    memcpy(tmpmem + 256, state->S_16_12, 8 * sizeof(WebRtc_Word32));
    memcpy(state->S_16_12, tmpmem + 416, 8 * sizeof(WebRtc_Word32));
    WebRtcSpl_Resample32khzTo24khz(tmpmem + 256, tmpmem + 240, 40);

    
    
    
    
    WebRtcSpl_UpBy2IntToInt(tmpmem + 240, 120, tmpmem, state->S_12_24);

    
    
    
    
    WebRtcSpl_UpBy2IntToShort(tmpmem, 240, out, state->S_24_48);
}


void WebRtcSpl_ResetResample8khzTo48khz(WebRtcSpl_State8khzTo48khz* state)
{
    memset(state->S_8_16, 0, 8 * sizeof(WebRtc_Word32));
    memset(state->S_16_12, 0, 8 * sizeof(WebRtc_Word32));
    memset(state->S_12_24, 0, 8 * sizeof(WebRtc_Word32));
    memset(state->S_24_48, 0, 8 * sizeof(WebRtc_Word32));
}
