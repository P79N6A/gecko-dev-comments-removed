














#include "signal_processing_library.h"


enum
{
    kBandFrameLength = 160
};


static const uint16_t WebRtcSpl_kAllPassFilter1[3] = {6418, 36982, 57261};
static const uint16_t WebRtcSpl_kAllPassFilter2[3] = {21333, 49062, 63010};



















void WebRtcSpl_AllPassQMF(int32_t* in_data, const int16_t data_length,
                          int32_t* out_data, const uint16_t* filter_coefficients,
                          int32_t* filter_state)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int16_t k;
    int32_t diff;
    

    
    

    
    
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

void WebRtcSpl_AnalysisQMF(const int16_t* in_data, int16_t* low_band,
                           int16_t* high_band, int32_t* filter_state1,
                           int32_t* filter_state2)
{
    int16_t i;
    int16_t k;
    int32_t tmp;
    int32_t half_in1[kBandFrameLength];
    int32_t half_in2[kBandFrameLength];
    int32_t filter1[kBandFrameLength];
    int32_t filter2[kBandFrameLength];

    
    for (i = 0, k = 0; i < kBandFrameLength; i++, k += 2)
    {
        half_in2[i] = WEBRTC_SPL_LSHIFT_W32((int32_t)in_data[k], 10);
        half_in1[i] = WEBRTC_SPL_LSHIFT_W32((int32_t)in_data[k + 1], 10);
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

void WebRtcSpl_SynthesisQMF(const int16_t* low_band, const int16_t* high_band,
                            int16_t* out_data, int32_t* filter_state1,
                            int32_t* filter_state2)
{
    int32_t tmp;
    int32_t half_in1[kBandFrameLength];
    int32_t half_in2[kBandFrameLength];
    int32_t filter1[kBandFrameLength];
    int32_t filter2[kBandFrameLength];
    int16_t i;
    int16_t k;

    
    
    for (i = 0; i < kBandFrameLength; i++)
    {
        tmp = (int32_t)low_band[i] + (int32_t)high_band[i];
        half_in1[i] = WEBRTC_SPL_LSHIFT_W32(tmp, 10);
        tmp = (int32_t)low_band[i] - (int32_t)high_band[i];
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
