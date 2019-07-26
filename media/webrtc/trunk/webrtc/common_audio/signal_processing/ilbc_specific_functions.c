




















#include "signal_processing_library.h"

void WebRtcSpl_ReverseOrderMultArrayElements(WebRtc_Word16 *out, G_CONST WebRtc_Word16 *in,
                                             G_CONST WebRtc_Word16 *win,
                                             WebRtc_Word16 vector_length,
                                             WebRtc_Word16 right_shifts)
{
    int i;
    WebRtc_Word16 *outptr = out;
    G_CONST WebRtc_Word16 *inptr = in;
    G_CONST WebRtc_Word16 *winptr = win;
    for (i = 0; i < vector_length; i++)
    {
        (*outptr++) = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(*inptr++,
                                                               *winptr--, right_shifts);
    }
}

void WebRtcSpl_ElementwiseVectorMult(WebRtc_Word16 *out, G_CONST WebRtc_Word16 *in,
                                     G_CONST WebRtc_Word16 *win, WebRtc_Word16 vector_length,
                                     WebRtc_Word16 right_shifts)
{
    int i;
    WebRtc_Word16 *outptr = out;
    G_CONST WebRtc_Word16 *inptr = in;
    G_CONST WebRtc_Word16 *winptr = win;
    for (i = 0; i < vector_length; i++)
    {
        (*outptr++) = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(*inptr++,
                                                               *winptr++, right_shifts);
    }
}

void WebRtcSpl_AddVectorsAndShift(WebRtc_Word16 *out, G_CONST WebRtc_Word16 *in1,
                                  G_CONST WebRtc_Word16 *in2, WebRtc_Word16 vector_length,
                                  WebRtc_Word16 right_shifts)
{
    int i;
    WebRtc_Word16 *outptr = out;
    G_CONST WebRtc_Word16 *in1ptr = in1;
    G_CONST WebRtc_Word16 *in2ptr = in2;
    for (i = vector_length; i > 0; i--)
    {
        (*outptr++) = (WebRtc_Word16)(((*in1ptr++) + (*in2ptr++)) >> right_shifts);
    }
}

void WebRtcSpl_AddAffineVectorToVector(WebRtc_Word16 *out, WebRtc_Word16 *in,
                                       WebRtc_Word16 gain, WebRtc_Word32 add_constant,
                                       WebRtc_Word16 right_shifts, int vector_length)
{
    WebRtc_Word16 *inPtr;
    WebRtc_Word16 *outPtr;
    int i;

    inPtr = in;
    outPtr = out;
    for (i = 0; i < vector_length; i++)
    {
        (*outPtr++) += (WebRtc_Word16)((WEBRTC_SPL_MUL_16_16((*inPtr++), gain)
                + (WebRtc_Word32)add_constant) >> right_shifts);
    }
}

void WebRtcSpl_AffineTransformVector(WebRtc_Word16 *out, WebRtc_Word16 *in,
                                     WebRtc_Word16 gain, WebRtc_Word32 add_constant,
                                     WebRtc_Word16 right_shifts, int vector_length)
{
    WebRtc_Word16 *inPtr;
    WebRtc_Word16 *outPtr;
    int i;

    inPtr = in;
    outPtr = out;
    for (i = 0; i < vector_length; i++)
    {
        (*outPtr++) = (WebRtc_Word16)((WEBRTC_SPL_MUL_16_16((*inPtr++), gain)
                + (WebRtc_Word32)add_constant) >> right_shifts);
    }
}
