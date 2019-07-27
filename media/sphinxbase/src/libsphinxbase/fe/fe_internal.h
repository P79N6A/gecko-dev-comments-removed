




































#ifndef __FE_INTERNAL_H__
#define __FE_INTERNAL_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sphinxbase/fe.h"
#include "sphinxbase/fixpoint.h"

#include "fe_noise.h"
#include "fe_prespch_buf.h"
#include "fe_type.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif


enum {
	RAW_LOG_SPEC = 1,
	SMOOTH_LOG_SPEC = 2
};


enum {
	LEGACY_DCT = 0,
	DCT_II = 1,
        DCT_HTK = 2
};

typedef struct melfb_s melfb_t;

struct melfb_s {
    float32 sampling_rate;
    int32 num_cepstra;
    int32 num_filters;
    int32 fft_size;
    float32 lower_filt_freq;
    float32 upper_filt_freq;
    
    mfcc_t **mel_cosine;
    
    mfcc_t *filt_coeffs;
    int16 *spec_start;
    int16 *filt_start;
    int16 *filt_width;
    
    int32 doublewide;
    char const *warp_type;
    char const *warp_params;
    uint32 warp_id;
    
    mfcc_t sqrt_inv_n, sqrt_inv_2n;
    
    int32 lifter_val;
    mfcc_t *lifter;
    
    int32 unit_area;
    

    int32 round_filters;
};

typedef struct ringbuf_s {
	powspec_t** bufs;
	int16 buf_num;
	int32 buf_len;
	int16 start;
	int16 end;
	int32 recs;
} ringbuf_t;


#define SQRT_HALF FLOAT2MFCC(0.707106781186548)

typedef struct vad_data_s {
    uint8 global_state;
    uint8 state_changed;
    uint8 store_pcm;
    int16 prespch_num;
    int16 postspch_num;
    prespch_buf_t* prespch_buf;	
} vad_data_t;


struct fe_s {
    cmd_ln_t *config;
    int refcount;

    int16 prespch_len;
    int16 postspch_len;
    float32 vad_threshold;

    float32 sampling_rate;
    int16 frame_rate;
    int16 frame_shift;

    float32 window_length;
    int16 frame_size;
    int16 fft_size;

    uint8 fft_order;
    uint8 feature_dimension;
    uint8 num_cepstra;
    uint8 remove_dc;
    uint8 log_spec;
    uint8 swap;
    uint8 dither;
    uint8 transform;
    uint8 remove_noise;
    uint8 remove_silence;

    float32 pre_emphasis_alpha;
    int32 seed;

    size_t sample_counter;
    uint8 start_flag;
    uint8 reserved;

    
    frame_t *ccc, *sss;
    
    melfb_t *mel_fb;
    
    window_t *hamming_window;
    
    noise_stats_t *noise_stats;

    
    vad_data_t *vad_data;

    
    
    int16 *spch;
    frame_t *frame;
    powspec_t *spec, *mfspec;
    int16 *overflow_samps;
    int16 num_overflow_samps;    
    int16 prior;
};

void fe_init_dither(int32 seed);


int32 fe_dither(int16 *buffer, int32 nsamps);


int fe_read_frame(fe_t *fe, int16 const *in, int32 len);


int fe_shift_frame(fe_t *fe, int16 const *in, int32 len);


void fe_write_frame(fe_t *fe, mfcc_t *fea);


int32 fe_build_melfilters(melfb_t *MEL_FB);
int32 fe_compute_melcosine(melfb_t *MEL_FB);
void fe_create_hamming(window_t *in, int32 in_len);
void fe_create_twiddle(fe_t *fe);

fixed32 fe_log_add(fixed32 x, fixed32 y);
fixed32 fe_log_sub(fixed32 x, fixed32 y);


void fe_spec2cep(fe_t * fe, const powspec_t * mflogspec, mfcc_t * mfcep);
void fe_dct2(fe_t *fe, const powspec_t *mflogspec, mfcc_t *mfcep, int htk);
void fe_dct3(fe_t *fe, const mfcc_t *mfcep, powspec_t *mflogspec);

#ifdef __cplusplus
}
#endif

#endif
