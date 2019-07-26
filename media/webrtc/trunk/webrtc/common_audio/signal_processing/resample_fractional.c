
















#include "signal_processing_library.h"


static const WebRtc_Word16 kCoefficients48To32[2][8] = {
        {778, -2050, 1087, 23285, 12903, -3783, 441, 222},
        {222, 441, -3783, 12903, 23285, 1087, -2050, 778}
};

static const WebRtc_Word16 kCoefficients32To24[3][8] = {
        {767, -2362, 2434, 24406, 10620, -3838, 721, 90},
        {386, -381, -2646, 19062, 19062, -2646, -381, 386},
        {90, 721, -3838, 10620, 24406, 2434, -2362, 767}
};

static const WebRtc_Word16 kCoefficients44To32[4][9] = {
        {117, -669, 2245, -6183, 26267, 13529, -3245, 845, -138},
        {-101, 612, -2283, 8532, 29790, -5138, 1789, -524, 91},
        {50, -292, 1016, -3064, 32010, 3933, -1147, 315, -53},
        {-156, 974, -3863, 18603, 21691, -6246, 2353, -712, 126}
};






void WebRtcSpl_Resample48khzTo32khz(const WebRtc_Word32 *In, WebRtc_Word32 *Out,
                                    const WebRtc_Word32 K)
{
    
    
    
    
    
    WebRtc_Word32 tmp;
    WebRtc_Word32 m;

    for (m = 0; m < K; m++)
    {
        tmp = 1 << 14;
        tmp += kCoefficients48To32[0][0] * In[0];
        tmp += kCoefficients48To32[0][1] * In[1];
        tmp += kCoefficients48To32[0][2] * In[2];
        tmp += kCoefficients48To32[0][3] * In[3];
        tmp += kCoefficients48To32[0][4] * In[4];
        tmp += kCoefficients48To32[0][5] * In[5];
        tmp += kCoefficients48To32[0][6] * In[6];
        tmp += kCoefficients48To32[0][7] * In[7];
        Out[0] = tmp;

        tmp = 1 << 14;
        tmp += kCoefficients48To32[1][0] * In[1];
        tmp += kCoefficients48To32[1][1] * In[2];
        tmp += kCoefficients48To32[1][2] * In[3];
        tmp += kCoefficients48To32[1][3] * In[4];
        tmp += kCoefficients48To32[1][4] * In[5];
        tmp += kCoefficients48To32[1][5] * In[6];
        tmp += kCoefficients48To32[1][6] * In[7];
        tmp += kCoefficients48To32[1][7] * In[8];
        Out[1] = tmp;

        
        In += 3;
        Out += 2;
    }
}






void WebRtcSpl_Resample32khzTo24khz(const WebRtc_Word32 *In, WebRtc_Word32 *Out,
                                    const WebRtc_Word32 K)
{
    
    
    
    
    
    WebRtc_Word32 m;
    WebRtc_Word32 tmp;

    for (m = 0; m < K; m++)
    {
        tmp = 1 << 14;
        tmp += kCoefficients32To24[0][0] * In[0];
        tmp += kCoefficients32To24[0][1] * In[1];
        tmp += kCoefficients32To24[0][2] * In[2];
        tmp += kCoefficients32To24[0][3] * In[3];
        tmp += kCoefficients32To24[0][4] * In[4];
        tmp += kCoefficients32To24[0][5] * In[5];
        tmp += kCoefficients32To24[0][6] * In[6];
        tmp += kCoefficients32To24[0][7] * In[7];
        Out[0] = tmp;

        tmp = 1 << 14;
        tmp += kCoefficients32To24[1][0] * In[1];
        tmp += kCoefficients32To24[1][1] * In[2];
        tmp += kCoefficients32To24[1][2] * In[3];
        tmp += kCoefficients32To24[1][3] * In[4];
        tmp += kCoefficients32To24[1][4] * In[5];
        tmp += kCoefficients32To24[1][5] * In[6];
        tmp += kCoefficients32To24[1][6] * In[7];
        tmp += kCoefficients32To24[1][7] * In[8];
        Out[1] = tmp;

        tmp = 1 << 14;
        tmp += kCoefficients32To24[2][0] * In[2];
        tmp += kCoefficients32To24[2][1] * In[3];
        tmp += kCoefficients32To24[2][2] * In[4];
        tmp += kCoefficients32To24[2][3] * In[5];
        tmp += kCoefficients32To24[2][4] * In[6];
        tmp += kCoefficients32To24[2][5] * In[7];
        tmp += kCoefficients32To24[2][6] * In[8];
        tmp += kCoefficients32To24[2][7] * In[9];
        Out[2] = tmp;

        
        In += 4;
        Out += 3;
    }
}








static void WebRtcSpl_ResampDotProduct(const WebRtc_Word32 *in1, const WebRtc_Word32 *in2,
                               const WebRtc_Word16 *coef_ptr, WebRtc_Word32 *out1,
                               WebRtc_Word32 *out2)
{
    WebRtc_Word32 tmp1 = 16384;
    WebRtc_Word32 tmp2 = 16384;
    WebRtc_Word16 coef;

    coef = coef_ptr[0];
    tmp1 += coef * in1[0];
    tmp2 += coef * in2[-0];

    coef = coef_ptr[1];
    tmp1 += coef * in1[1];
    tmp2 += coef * in2[-1];

    coef = coef_ptr[2];
    tmp1 += coef * in1[2];
    tmp2 += coef * in2[-2];

    coef = coef_ptr[3];
    tmp1 += coef * in1[3];
    tmp2 += coef * in2[-3];

    coef = coef_ptr[4];
    tmp1 += coef * in1[4];
    tmp2 += coef * in2[-4];

    coef = coef_ptr[5];
    tmp1 += coef * in1[5];
    tmp2 += coef * in2[-5];

    coef = coef_ptr[6];
    tmp1 += coef * in1[6];
    tmp2 += coef * in2[-6];

    coef = coef_ptr[7];
    tmp1 += coef * in1[7];
    tmp2 += coef * in2[-7];

    coef = coef_ptr[8];
    *out1 = tmp1 + coef * in1[8];
    *out2 = tmp2 + coef * in2[-8];
}






void WebRtcSpl_Resample44khzTo32khz(const WebRtc_Word32 *In, WebRtc_Word32 *Out,
                                    const WebRtc_Word32 K)
{
    
    
    
    
    
    WebRtc_Word32 tmp;
    WebRtc_Word32 m;

    for (m = 0; m < K; m++)
    {
        tmp = 1 << 14;

        
        Out[0] = ((WebRtc_Word32)In[3] << 15) + tmp;

        
        tmp += kCoefficients44To32[3][0] * In[5];
        tmp += kCoefficients44To32[3][1] * In[6];
        tmp += kCoefficients44To32[3][2] * In[7];
        tmp += kCoefficients44To32[3][3] * In[8];
        tmp += kCoefficients44To32[3][4] * In[9];
        tmp += kCoefficients44To32[3][5] * In[10];
        tmp += kCoefficients44To32[3][6] * In[11];
        tmp += kCoefficients44To32[3][7] * In[12];
        tmp += kCoefficients44To32[3][8] * In[13];
        Out[4] = tmp;

        
        WebRtcSpl_ResampDotProduct(&In[0], &In[17], kCoefficients44To32[0], &Out[1], &Out[7]);

        
        WebRtcSpl_ResampDotProduct(&In[2], &In[15], kCoefficients44To32[1], &Out[2], &Out[6]);

        
        WebRtcSpl_ResampDotProduct(&In[3], &In[14], kCoefficients44To32[2], &Out[3], &Out[5]);

        
        In += 11;
        Out += 8;
    }
}
