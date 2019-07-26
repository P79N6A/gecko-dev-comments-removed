
















#include "signal_processing_library.h"

WebRtc_Word32 WebRtcSpl_SqrtLocal(WebRtc_Word32 in);

WebRtc_Word32 WebRtcSpl_SqrtLocal(WebRtc_Word32 in)
{

    WebRtc_Word16 x_half, t16;
    WebRtc_Word32 A, B, x2;

    







    B = in;

    B = WEBRTC_SPL_RSHIFT_W32(B, 1); 
    B = B - ((WebRtc_Word32)0x40000000); 
    x_half = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(B, 16);
    B = B + ((WebRtc_Word32)0x40000000); 
    B = B + ((WebRtc_Word32)0x40000000); 

    x2 = ((WebRtc_Word32)x_half) * ((WebRtc_Word32)x_half) * 2; 
    A = -x2; 
    B = B + (A >> 1); 

    A = WEBRTC_SPL_RSHIFT_W32(A, 16);
    A = A * A * 2; 
    t16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(A, 16);
    B = B + WEBRTC_SPL_MUL_16_16(-20480, t16) * 2; 
    

    t16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(A, 16);
    A = WEBRTC_SPL_MUL_16_16(x_half, t16) * 2; 
    t16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(A, 16);
    B = B + WEBRTC_SPL_MUL_16_16(28672, t16) * 2; 
    

    t16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(x2, 16);
    A = WEBRTC_SPL_MUL_16_16(x_half, t16) * 2; 

    B = B + (A >> 1); 
    

    B = B + ((WebRtc_Word32)32768); 

    return B;
}

WebRtc_Word32 WebRtcSpl_Sqrt(WebRtc_Word32 value)
{
    





























































    WebRtc_Word16 x_norm, nshift, t16, sh;
    WebRtc_Word32 A;

    WebRtc_Word16 k_sqrt_2 = 23170; 

    A = value;

    if (A == 0)
        return (WebRtc_Word32)0; 

    sh = WebRtcSpl_NormW32(A); 
    A = WEBRTC_SPL_LSHIFT_W32(A, sh); 
    if (A < (WEBRTC_SPL_WORD32_MAX - 32767))
    {
        A = A + ((WebRtc_Word32)32768); 
    } else
    {
        A = WEBRTC_SPL_WORD32_MAX;
    }

    x_norm = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(A, 16); 

    nshift = WEBRTC_SPL_RSHIFT_W16(sh, 1); 
    nshift = -nshift; 

    A = (WebRtc_Word32)WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)x_norm, 16);
    A = WEBRTC_SPL_ABS_W32(A); 
    A = WebRtcSpl_SqrtLocal(A); 

    if ((-2 * nshift) == sh)
    { 

        t16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(A, 16); 

        A = WEBRTC_SPL_MUL_16_16(k_sqrt_2, t16) * 2; 
        A = A + ((WebRtc_Word32)32768); 
        A = A & ((WebRtc_Word32)0x7fff0000); 

        A = WEBRTC_SPL_RSHIFT_W32(A, 15); 

    } else
    {
        A = WEBRTC_SPL_RSHIFT_W32(A, 16); 
    }

    A = A & ((WebRtc_Word32)0x0000ffff);
    A = (WebRtc_Word32)WEBRTC_SPL_SHIFT_W32(A, nshift); 

    return A;
}
