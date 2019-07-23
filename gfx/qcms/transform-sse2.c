#include <emmintrin.h>

#include "qcmsint.h"


#define FLOATSCALE  65536.0f
#define CLAMPMAXVAL ( ((float) (65536 - 1)) / 65536.0f )
static const ALIGN float floatScaleX4[4] =
    { FLOATSCALE, FLOATSCALE, FLOATSCALE, FLOATSCALE};
static const ALIGN float clampMaxValueX4[4] =
    { CLAMPMAXVAL, CLAMPMAXVAL, CLAMPMAXVAL, CLAMPMAXVAL};

void qcms_transform_data_rgb_out_lut_sse2(qcms_transform *transform,
                                          unsigned char *src,
                                          unsigned char *dest,
                                          size_t length)
{
    unsigned int i;
    float (*mat)[4] = transform->matrix;
    char input_back[32];
    



    float const * input = (float*)(((uintptr_t)&input_back[16]) & ~0xf);
    

    uint32_t const * output = (uint32_t*)input;

    
    const float *igtbl_r = transform->input_gamma_table_r;
    const float *igtbl_g = transform->input_gamma_table_g;
    const float *igtbl_b = transform->input_gamma_table_b;

    
    const uint8_t *otdata_r = &transform->output_table_r->data[0];
    const uint8_t *otdata_g = &transform->output_table_g->data[0];
    const uint8_t *otdata_b = &transform->output_table_b->data[0];

    
    const __m128 mat0  = _mm_load_ps(mat[0]);
    const __m128 mat1  = _mm_load_ps(mat[1]);
    const __m128 mat2  = _mm_load_ps(mat[2]);

    
    const __m128 max   = _mm_load_ps(clampMaxValueX4);
    const __m128 min   = _mm_setzero_ps();
    const __m128 scale = _mm_load_ps(floatScaleX4);

    
    __m128 vec_r, vec_g, vec_b, result;

    
    if (!length)
        return;

    
    length--;

    
    vec_r = _mm_load_ss(&igtbl_r[src[0]]);
    vec_g = _mm_load_ss(&igtbl_g[src[1]]);
    vec_b = _mm_load_ss(&igtbl_b[src[2]]);
    src += 3;

    

    for (i=0; i<length; i++)
    {
        
        vec_r = _mm_shuffle_ps(vec_r, vec_r, 0);
        vec_g = _mm_shuffle_ps(vec_g, vec_g, 0);
        vec_b = _mm_shuffle_ps(vec_b, vec_b, 0);

        
        vec_r = _mm_mul_ps(vec_r, mat0);
        vec_g = _mm_mul_ps(vec_g, mat1);
        vec_b = _mm_mul_ps(vec_b, mat2);

        
        vec_r  = _mm_add_ps(vec_r, _mm_add_ps(vec_g, vec_b));
        vec_r  = _mm_max_ps(min, vec_r);
        vec_r  = _mm_min_ps(max, vec_r);
        result = _mm_mul_ps(vec_r, scale);

        
        _mm_store_si128((__m128i*)output, _mm_cvtps_epi32(result));

        
        vec_r = _mm_load_ss(&igtbl_r[src[0]]);
        vec_g = _mm_load_ss(&igtbl_g[src[1]]);
        vec_b = _mm_load_ss(&igtbl_b[src[2]]);
        src += 3;

        
        dest[0] = otdata_r[output[0]];
        dest[1] = otdata_g[output[1]];
        dest[2] = otdata_b[output[2]];
        dest += 3;
    }

    

    vec_r = _mm_shuffle_ps(vec_r, vec_r, 0);
    vec_g = _mm_shuffle_ps(vec_g, vec_g, 0);
    vec_b = _mm_shuffle_ps(vec_b, vec_b, 0);

    vec_r = _mm_mul_ps(vec_r, mat0);
    vec_g = _mm_mul_ps(vec_g, mat1);
    vec_b = _mm_mul_ps(vec_b, mat2);

    vec_r  = _mm_add_ps(vec_r, _mm_add_ps(vec_g, vec_b));
    vec_r  = _mm_max_ps(min, vec_r);
    vec_r  = _mm_min_ps(max, vec_r);
    result = _mm_mul_ps(vec_r, scale);

    _mm_store_si128((__m128i*)output, _mm_cvtps_epi32(result));

    dest[0] = otdata_r[output[0]];
    dest[1] = otdata_g[output[1]];
    dest[2] = otdata_b[output[2]];
}

void qcms_transform_data_rgba_out_lut_sse2(qcms_transform *transform,
                                           unsigned char *src,
                                           unsigned char *dest,
                                           size_t length)
{
    unsigned int i;
    float (*mat)[4] = transform->matrix;
    char input_back[32];
    



    float const * input = (float*)(((uintptr_t)&input_back[16]) & ~0xf);
    

    uint32_t const * output = (uint32_t*)input;

    
    const float *igtbl_r = transform->input_gamma_table_r;
    const float *igtbl_g = transform->input_gamma_table_g;
    const float *igtbl_b = transform->input_gamma_table_b;

    
    const uint8_t *otdata_r = &transform->output_table_r->data[0];
    const uint8_t *otdata_g = &transform->output_table_g->data[0];
    const uint8_t *otdata_b = &transform->output_table_b->data[0];

    
    const __m128 mat0  = _mm_load_ps(mat[0]);
    const __m128 mat1  = _mm_load_ps(mat[1]);
    const __m128 mat2  = _mm_load_ps(mat[2]);

    
    const __m128 max   = _mm_load_ps(clampMaxValueX4);
    const __m128 min   = _mm_setzero_ps();
    const __m128 scale = _mm_load_ps(floatScaleX4);

    
    __m128 vec_r, vec_g, vec_b, result;
    unsigned char alpha;

    
    if (!length)
        return;

    
    length--;

    
    vec_r = _mm_load_ss(&igtbl_r[src[0]]);
    vec_g = _mm_load_ss(&igtbl_g[src[1]]);
    vec_b = _mm_load_ss(&igtbl_b[src[2]]);
    alpha = src[3];
    src += 4;

    

    for (i=0; i<length; i++)
    {
        
        vec_r = _mm_shuffle_ps(vec_r, vec_r, 0);
        vec_g = _mm_shuffle_ps(vec_g, vec_g, 0);
        vec_b = _mm_shuffle_ps(vec_b, vec_b, 0);

        
        vec_r = _mm_mul_ps(vec_r, mat0);
        vec_g = _mm_mul_ps(vec_g, mat1);
        vec_b = _mm_mul_ps(vec_b, mat2);

        
        dest[3] = alpha;
        alpha   = src[3];

        
        vec_r  = _mm_add_ps(vec_r, _mm_add_ps(vec_g, vec_b));
        vec_r  = _mm_max_ps(min, vec_r);
        vec_r  = _mm_min_ps(max, vec_r);
        result = _mm_mul_ps(vec_r, scale);

        
        _mm_store_si128((__m128i*)output, _mm_cvtps_epi32(result));

        
        vec_r = _mm_load_ss(&igtbl_r[src[0]]);
        vec_g = _mm_load_ss(&igtbl_g[src[1]]);
        vec_b = _mm_load_ss(&igtbl_b[src[2]]);
        src += 4;

        
        dest[0] = otdata_r[output[0]];
        dest[1] = otdata_g[output[1]];
        dest[2] = otdata_b[output[2]];
        dest += 4;
    }

    

    vec_r = _mm_shuffle_ps(vec_r, vec_r, 0);
    vec_g = _mm_shuffle_ps(vec_g, vec_g, 0);
    vec_b = _mm_shuffle_ps(vec_b, vec_b, 0);

    vec_r = _mm_mul_ps(vec_r, mat0);
    vec_g = _mm_mul_ps(vec_g, mat1);
    vec_b = _mm_mul_ps(vec_b, mat2);

    dest[3] = alpha;

    vec_r  = _mm_add_ps(vec_r, _mm_add_ps(vec_g, vec_b));
    vec_r  = _mm_max_ps(min, vec_r);
    vec_r  = _mm_min_ps(max, vec_r);
    result = _mm_mul_ps(vec_r, scale);

    _mm_store_si128((__m128i*)output, _mm_cvtps_epi32(result));

    dest[0] = otdata_r[output[0]];
    dest[1] = otdata_g[output[1]];
    dest[2] = otdata_b[output[2]];
}


