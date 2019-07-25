














#include "signal_processing_library.h"


enum
{
    kBandFrameLength = 160
};


static const WebRtc_UWord16 WebRtcSpl_kAllPassFilter1[3] = {6418, 36982, 57261};
static const WebRtc_UWord16 WebRtcSpl_kAllPassFilter2[3] = {21333, 49062, 63010};



















void WebRtcSpl_AllPassQMF(WebRtc_Word32* in_data, const WebRtc_Word16 data_length,
                          WebRtc_Word32* out_data, const WebRtc_UWord16* filter_coefficients,
                          WebRtc_Word32* filter_state)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 k;
    WebRtc_Word32 diff;
    

    
    

    
    
    diff = WEBRTC_SPL_SUB_SAT_W32(in_data[0], filter_state[1]); 
    
    out_data[0] = WEBRTC_SPL_SCALEDIFF32(filter_coefficients[0], diff, filter_state[0]);

    
    for (k = 1; k < data_length; k++)
    {
        diff = WEBRTC_SPL_SUB_SAT_W32(in_data[k], out_data[k - 1]); 
        
        out_data[k] = WEBRTC_SPL_SCALEDIFF32(filter_coefficients[0], diff, in_data[k - 1]);
    }

    
    filter_state[0] = in_data[data_length - 1]; 
    filter_state[1] = out_data[data_length - 1]; 

    
    diff = WEBRTC_SPL_SUB_SAT_W32(out_data[0], filter_state[3]); 
    
    in_data[0] = WEBRTC_SPL_SCALEDIFF32(filter_coefficients[1], diff, filter_state[2]);
    for (k = 1; k < data_length; k++)
    {
        diff = WEBRTC_SPL_SUB_SAT_W32(out_data[k], in_data[k - 1]); 
        
        in_data[k] = WEBRTC_SPL_SCALEDIFF32(filter_coefficients[1], diff, out_data[k-1]);
    }

    filter_state[2] = out_data[data_length - 1]; 
    filter_state[3] = in_data[data_length - 1]; 

    
    diff = WEBRTC_SPL_SUB_SAT_W32(in_data[0], filter_state[5]); 
    
    out_data[0] = WEBRTC_SPL_SCALEDIFF32(filter_coefficients[2], diff, filter_state[4]);
    for (k = 1; k < data_length; k++)
    {
        diff = WEBRTC_SPL_SUB_SAT_W32(in_data[k], out_data[k - 1]); 
        
        out_data[k] = WEBRTC_SPL_SCALEDIFF32(filter_coefficients[2], diff, in_data[k-1]);
    }
    filter_state[4] = in_data[data_length - 1]; 
    filter_state[5] = out_data[data_length - 1]; 
}

void WebRtcSpl_AnalysisQMF(const WebRtc_Word16* in_data, WebRtc_Word16* low_band,
                           WebRtc_Word16* high_band, WebRtc_Word32* filter_state1,
                           WebRtc_Word32* filter_state2)
{
    WebRtc_Word16 i;
    WebRtc_Word16 k;
    WebRtc_Word32 tmp;
    WebRtc_Word32 half_in1[kBandFrameLength];
    WebRtc_Word32 half_in2[kBandFrameLength];
    WebRtc_Word32 filter1[kBandFrameLength];
    WebRtc_Word32 filter2[kBandFrameLength];

    
    for (i = 0, k = 0; i < kBandFrameLength; i++, k += 2)
    {
        half_in2[i] = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)in_data[k], 10);
        half_in1[i] = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)in_data[k + 1], 10);
    }

    
    WebRtcSpl_AllPassQMF(half_in1, kBandFrameLength, filter1, WebRtcSpl_kAllPassFilter1,
                         filter_state1);
    WebRtcSpl_AllPassQMF(half_in2, kBandFrameLength, filter2, WebRtcSpl_kAllPassFilter2,
                         filter_state2);

    
    
    for (i = 0; i < kBandFrameLength; i++)
    {
        tmp = filter1[i] + filter2[i] + 1024;
        tmp = WEBRTC_SPL_RSHIFT_W32(tmp, 11);
        low_band[i] = WebRtcSpl_SatW32ToW16(tmp);

        tmp = filter1[i] - filter2[i] + 1024;
        tmp = WEBRTC_SPL_RSHIFT_W32(tmp, 11);
        high_band[i] = WebRtcSpl_SatW32ToW16(tmp);
    }
}

void WebRtcSpl_SynthesisQMF(const WebRtc_Word16* low_band, const WebRtc_Word16* high_band,
                            WebRtc_Word16* out_data, WebRtc_Word32* filter_state1,
                            WebRtc_Word32* filter_state2)
{
    WebRtc_Word32 tmp;
    WebRtc_Word32 half_in1[kBandFrameLength];
    WebRtc_Word32 half_in2[kBandFrameLength];
    WebRtc_Word32 filter1[kBandFrameLength];
    WebRtc_Word32 filter2[kBandFrameLength];
    WebRtc_Word16 i;
    WebRtc_Word16 k;

    
    
    for (i = 0; i < kBandFrameLength; i++)
    {
        tmp = (WebRtc_Word32)low_band[i] + (WebRtc_Word32)high_band[i];
        half_in1[i] = WEBRTC_SPL_LSHIFT_W32(tmp, 10);
        tmp = (WebRtc_Word32)low_band[i] - (WebRtc_Word32)high_band[i];
        half_in2[i] = WEBRTC_SPL_LSHIFT_W32(tmp, 10);
    }

    
    WebRtcSpl_AllPassQMF(half_in1, kBandFrameLength, filter1, WebRtcSpl_kAllPassFilter2,
                         filter_state1);
    WebRtcSpl_AllPassQMF(half_in2, kBandFrameLength, filter2, WebRtcSpl_kAllPassFilter1,
                         filter_state2);

    
    
    
    for (i = 0, k = 0; i < kBandFrameLength; i++)
    {
        tmp = WEBRTC_SPL_RSHIFT_W32(filter2[i] + 512, 10);
        out_data[k++] = WebRtcSpl_SatW32ToW16(tmp);

        tmp = WEBRTC_SPL_RSHIFT_W32(filter1[i] + 512, 10);
        out_data[k++] = WebRtcSpl_SatW32ToW16(tmp);
    }

}
