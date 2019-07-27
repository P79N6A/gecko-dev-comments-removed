

















#include "libavutil/attributes.h"
#include "libavutil/mem.h"
#include "avfft.h"
#include "fft.h"
#include "rdft.h"
#include "dct.h"



FFTContext *av_fft_init(int nbits, int inverse)
{
    FFTContext *s = av_malloc(sizeof(*s));

    if (s && ff_fft_init(s, nbits, inverse))
        av_freep(&s);

    return s;
}

void av_fft_permute(FFTContext *s, FFTComplex *z)
{
    s->fft_permute(s, z);
}

void av_fft_calc(FFTContext *s, FFTComplex *z)
{
    s->fft_calc(s, z);
}

av_cold void av_fft_end(FFTContext *s)
{
    if (s) {
        ff_fft_end(s);
        av_free(s);
    }
}

#if CONFIG_MDCT

FFTContext *av_mdct_init(int nbits, int inverse, double scale)
{
    FFTContext *s = av_malloc(sizeof(*s));

    if (s && ff_mdct_init(s, nbits, inverse, scale))
        av_freep(&s);

    return s;
}

void av_imdct_calc(FFTContext *s, FFTSample *output, const FFTSample *input)
{
    s->imdct_calc(s, output, input);
}

void av_imdct_half(FFTContext *s, FFTSample *output, const FFTSample *input)
{
    s->imdct_half(s, output, input);
}

void av_mdct_calc(FFTContext *s, FFTSample *output, const FFTSample *input)
{
    s->mdct_calc(s, output, input);
}

av_cold void av_mdct_end(FFTContext *s)
{
    if (s) {
        ff_mdct_end(s);
        av_free(s);
    }
}

#endif 

#if CONFIG_RDFT

RDFTContext *av_rdft_init(int nbits, enum RDFTransformType trans)
{
    RDFTContext *s = av_malloc(sizeof(*s));

    if (s && ff_rdft_init(s, nbits, trans))
        av_freep(&s);

    return s;
}

void av_rdft_calc(RDFTContext *s, FFTSample *data)
{
    s->rdft_calc(s, data);
}

av_cold void av_rdft_end(RDFTContext *s)
{
    if (s) {
        ff_rdft_end(s);
        av_free(s);
    }
}

#endif 

#if CONFIG_DCT

DCTContext *av_dct_init(int nbits, enum DCTTransformType inverse)
{
    DCTContext *s = av_malloc(sizeof(*s));

    if (s && ff_dct_init(s, nbits, inverse))
        av_freep(&s);

    return s;
}

void av_dct_calc(DCTContext *s, FFTSample *data)
{
    s->dct_calc(s, data);
}

av_cold void av_dct_end(DCTContext *s)
{
    if (s) {
        ff_dct_end(s);
        av_free(s);
    }
}

#endif 
