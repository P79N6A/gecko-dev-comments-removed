
















#include "signal_processing_library.h"

int32_t WebRtcSpl_SqrtLocal(int32_t in);

int32_t WebRtcSpl_SqrtLocal(int32_t in)
{

    int16_t x_half, t16;
    int32_t A, B, x2;

    







    B = in;

    B = WEBRTC_SPL_RSHIFT_W32(B, 1); 
    B = B - ((int32_t)0x40000000); 
    x_half = (int16_t)WEBRTC_SPL_RSHIFT_W32(B, 16);
    B = B + ((int32_t)0x40000000); 
    B = B + ((int32_t)0x40000000); 

    x2 = ((int32_t)x_half) * ((int32_t)x_half) * 2; 
    A = -x2; 
    B = B + (A >> 1); 

    A = WEBRTC_SPL_RSHIFT_W32(A, 16);
    A = A * A * 2; 
    t16 = (int16_t)WEBRTC_SPL_RSHIFT_W32(A, 16);
    B = B + WEBRTC_SPL_MUL_16_16(-20480, t16) * 2; 
    

    t16 = (int16_t)WEBRTC_SPL_RSHIFT_W32(A, 16);
    A = WEBRTC_SPL_MUL_16_16(x_half, t16) * 2; 
    t16 = (int16_t)WEBRTC_SPL_RSHIFT_W32(A, 16);
    B = B + WEBRTC_SPL_MUL_16_16(28672, t16) * 2; 
    

    t16 = (int16_t)WEBRTC_SPL_RSHIFT_W32(x2, 16);
    A = WEBRTC_SPL_MUL_16_16(x_half, t16) * 2; 

    B = B + (A >> 1); 
    

    B = B + ((int32_t)32768); 

    return B;
}

int32_t WebRtcSpl_Sqrt(int32_t value)
{
    





























































    int16_t x_norm, nshift, t16, sh;
    int32_t A;

    int16_t k_sqrt_2 = 23170; 

    A = value;

    if (A == 0)
        return (int32_t)0; 

    sh = WebRtcSpl_NormW32(A); 
    A = WEBRTC_SPL_LSHIFT_W32(A, sh); 
    if (A < (WEBRTC_SPL_WORD32_MAX - 32767))
    {
        A = A + ((int32_t)32768); 
    } else
    {
        A = WEBRTC_SPL_WORD32_MAX;
    }

    x_norm = (int16_t)WEBRTC_SPL_RSHIFT_W32(A, 16); 

    nshift = WEBRTC_SPL_RSHIFT_W16(sh, 1); 
    nshift = -nshift; 

    A = (int32_t)WEBRTC_SPL_LSHIFT_W32((int32_t)x_norm, 16);
    A = WEBRTC_SPL_ABS_W32(A); 
    A = WebRtcSpl_SqrtLocal(A); 

    if ((-2 * nshift) == sh)
    { 

        t16 = (int16_t)WEBRTC_SPL_RSHIFT_W32(A, 16); 

        A = WEBRTC_SPL_MUL_16_16(k_sqrt_2, t16) * 2; 
        A = A + ((int32_t)32768); 
        A = A & ((int32_t)0x7fff0000); 

        A = WEBRTC_SPL_RSHIFT_W32(A, 15); 

    } else
    {
        A = WEBRTC_SPL_RSHIFT_W32(A, 16); 
    }

    A = A & ((int32_t)0x0000ffff);
    A = (int32_t)WEBRTC_SPL_SHIFT_W32(A, nshift); 

    return A;
}
