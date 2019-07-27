





































#ifndef SPEEX_RESAMPLER_H
#define SPEEX_RESAMPLER_H

#if 1 







#define RANDOM_PREFIX moz_speex
#ifndef RANDOM_PREFIX
#error "Please define RANDOM_PREFIX (above) to something specific to your project to prevent symbol name clashes"
#endif

#define CAT_PREFIX2(a,b) a ## b
#define CAT_PREFIX(a,b) CAT_PREFIX2(a, b)
      
#define speex_resampler_init CAT_PREFIX(RANDOM_PREFIX,_resampler_init)
#define speex_resampler_init_frac CAT_PREFIX(RANDOM_PREFIX,_resampler_init_frac)
#define speex_resampler_destroy CAT_PREFIX(RANDOM_PREFIX,_resampler_destroy)
#define speex_resampler_process_float CAT_PREFIX(RANDOM_PREFIX,_resampler_process_float)
#define speex_resampler_process_int CAT_PREFIX(RANDOM_PREFIX,_resampler_process_int)
#define speex_resampler_process_interleaved_float CAT_PREFIX(RANDOM_PREFIX,_resampler_process_interleaved_float)
#define speex_resampler_process_interleaved_int CAT_PREFIX(RANDOM_PREFIX,_resampler_process_interleaved_int)
#define speex_resampler_set_rate CAT_PREFIX(RANDOM_PREFIX,_resampler_set_rate)
#define speex_resampler_get_rate CAT_PREFIX(RANDOM_PREFIX,_resampler_get_rate)
#define speex_resampler_set_rate_frac CAT_PREFIX(RANDOM_PREFIX,_resampler_set_rate_frac)
#define speex_resampler_get_ratio CAT_PREFIX(RANDOM_PREFIX,_resampler_get_ratio)
#define speex_resampler_set_quality CAT_PREFIX(RANDOM_PREFIX,_resampler_set_quality)
#define speex_resampler_get_quality CAT_PREFIX(RANDOM_PREFIX,_resampler_get_quality)
#define speex_resampler_set_input_stride CAT_PREFIX(RANDOM_PREFIX,_resampler_set_input_stride)
#define speex_resampler_get_input_stride CAT_PREFIX(RANDOM_PREFIX,_resampler_get_input_stride)
#define speex_resampler_set_output_stride CAT_PREFIX(RANDOM_PREFIX,_resampler_set_output_stride)
#define speex_resampler_get_output_stride CAT_PREFIX(RANDOM_PREFIX,_resampler_get_output_stride)
#define speex_resampler_get_input_latency CAT_PREFIX(RANDOM_PREFIX,_resampler_get_input_latency)
#define speex_resampler_get_output_latency CAT_PREFIX(RANDOM_PREFIX,_resampler_get_output_latency)
#define speex_resampler_skip_zeros CAT_PREFIX(RANDOM_PREFIX,_resampler_skip_zeros)
#define speex_resampler_set_skip_frac_num CAT_PREFIX(RANDOM_PREFIX,_resampler_set_skip_frac_num)
#define speex_resampler_reset_mem CAT_PREFIX(RANDOM_PREFIX,_resampler_reset_mem)
#define speex_resampler_strerror CAT_PREFIX(RANDOM_PREFIX,_resampler_strerror)

#define spx_int16_t short
#define spx_int32_t int
#define spx_uint16_t unsigned short
#define spx_uint32_t unsigned int
      
#else 

#ifdef _BUILD_SPEEX
# include "speex_types.h"
#else
# include <speex/speex_types.h>
#endif

#endif 

#ifdef __cplusplus
extern "C" {
#endif

#define SPEEX_RESAMPLER_QUALITY_MAX 10
#define SPEEX_RESAMPLER_QUALITY_MIN 0
#define SPEEX_RESAMPLER_QUALITY_DEFAULT 4
#define SPEEX_RESAMPLER_QUALITY_VOIP 3
#define SPEEX_RESAMPLER_QUALITY_DESKTOP 5

enum {
   RESAMPLER_ERR_SUCCESS         = 0,
   RESAMPLER_ERR_ALLOC_FAILED    = 1,
   RESAMPLER_ERR_BAD_STATE       = 2,
   RESAMPLER_ERR_INVALID_ARG     = 3,
   RESAMPLER_ERR_PTR_OVERLAP     = 4,
   
   RESAMPLER_ERR_MAX_ERROR
};

struct SpeexResamplerState_;
typedef struct SpeexResamplerState_ SpeexResamplerState;










SpeexResamplerState *speex_resampler_init(spx_uint32_t nb_channels, 
                                          spx_uint32_t in_rate, 
                                          spx_uint32_t out_rate, 
                                          int quality,
                                          int *err);














SpeexResamplerState *speex_resampler_init_frac(spx_uint32_t nb_channels, 
                                               spx_uint32_t ratio_num, 
                                               spx_uint32_t ratio_den, 
                                               spx_uint32_t in_rate, 
                                               spx_uint32_t out_rate, 
                                               int quality,
                                               int *err);




void speex_resampler_destroy(SpeexResamplerState *st);











int speex_resampler_process_float(SpeexResamplerState *st, 
                                   spx_uint32_t channel_index, 
                                   const float *in, 
                                   spx_uint32_t *in_len, 
                                   float *out, 
                                   spx_uint32_t *out_len);











int speex_resampler_process_int(SpeexResamplerState *st, 
                                 spx_uint32_t channel_index, 
                                 const spx_int16_t *in, 
                                 spx_uint32_t *in_len, 
                                 spx_int16_t *out, 
                                 spx_uint32_t *out_len);










int speex_resampler_process_interleaved_float(SpeexResamplerState *st, 
                                               const float *in, 
                                               spx_uint32_t *in_len, 
                                               float *out, 
                                               spx_uint32_t *out_len);










int speex_resampler_process_interleaved_int(SpeexResamplerState *st, 
                                             const spx_int16_t *in, 
                                             spx_uint32_t *in_len, 
                                             spx_int16_t *out, 
                                             spx_uint32_t *out_len);






int speex_resampler_set_rate(SpeexResamplerState *st, 
                              spx_uint32_t in_rate, 
                              spx_uint32_t out_rate);






void speex_resampler_get_rate(SpeexResamplerState *st, 
                              spx_uint32_t *in_rate, 
                              spx_uint32_t *out_rate);









int speex_resampler_set_rate_frac(SpeexResamplerState *st, 
                                   spx_uint32_t ratio_num, 
                                   spx_uint32_t ratio_den, 
                                   spx_uint32_t in_rate, 
                                   spx_uint32_t out_rate);







void speex_resampler_get_ratio(SpeexResamplerState *st, 
                               spx_uint32_t *ratio_num, 
                               spx_uint32_t *ratio_den);






int speex_resampler_set_quality(SpeexResamplerState *st, 
                                 int quality);






void speex_resampler_get_quality(SpeexResamplerState *st, 
                                 int *quality);





void speex_resampler_set_input_stride(SpeexResamplerState *st, 
                                      spx_uint32_t stride);





void speex_resampler_get_input_stride(SpeexResamplerState *st, 
                                      spx_uint32_t *stride);





void speex_resampler_set_output_stride(SpeexResamplerState *st, 
                                      spx_uint32_t stride);





void speex_resampler_get_output_stride(SpeexResamplerState *st, 
                                      spx_uint32_t *stride);




int speex_resampler_get_input_latency(SpeexResamplerState *st);




int speex_resampler_get_output_latency(SpeexResamplerState *st);









int speex_resampler_skip_zeros(SpeexResamplerState *st);














int speex_resampler_set_skip_frac_num(SpeexResamplerState *st,
                                      spx_uint32_t skip_frac_num);




int speex_resampler_reset_mem(SpeexResamplerState *st);





const char *speex_resampler_strerror(int err);

#ifdef __cplusplus
}
#endif

#endif
