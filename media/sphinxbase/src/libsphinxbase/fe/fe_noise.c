
















































#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include "sphinxbase/prim_type.h"
#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/strfuncs.h"
#include "sphinxbase/err.h"

#include "fe_noise.h"
#include "fe_internal.h"


#define SMOOTH_WINDOW 4
#define LAMBDA_POWER 0.7
#define LAMBDA_A 0.995
#define LAMBDA_B 0.5
#define LAMBDA_T 0.85
#define MU_T 0.2
#define MAX_GAIN 20

struct noise_stats_s {
    
    powspec_t *power;
    
    powspec_t *noise;
    
    powspec_t *floor;
    
    powspec_t *peak;

    
    uint8 undefined;
    
    uint32 num_filters;

    
    powspec_t lambda_power;
    powspec_t comp_lambda_power;
    powspec_t lambda_a;
    powspec_t comp_lambda_a;
    powspec_t lambda_b;
    powspec_t comp_lambda_b;
    powspec_t lambda_t;
    powspec_t mu_t;
    powspec_t max_gain;
    powspec_t inv_max_gain;

    powspec_t smooth_scaling[2 * SMOOTH_WINDOW + 3];
};

static void
fe_lower_envelope(noise_stats_t *noise_stats, powspec_t * buf, powspec_t * floor_buf, int32 num_filt)
{
    int i;

    for (i = 0; i < num_filt; i++) {
#ifndef FIXED_POINT
        if (buf[i] >= floor_buf[i]) {
            floor_buf[i] =
                noise_stats->lambda_a * floor_buf[i] + noise_stats->comp_lambda_a * buf[i];
        }
        else {
            floor_buf[i] =
                noise_stats->lambda_b * floor_buf[i] + noise_stats->comp_lambda_b * buf[i];
        }
#else
        if (buf[i] >= floor_buf[i]) {
            floor_buf[i] = fe_log_add(noise_stats->lambda_a + floor_buf[i],
                                  noise_stats->comp_lambda_a + buf[i]);
        }
        else {
            floor_buf[i] = fe_log_add(noise_stats->lambda_b + floor_buf[i],
                                  noise_stats->comp_lambda_b + buf[i]);
        }
#endif
    }
}


static void
fe_temp_masking(noise_stats_t *noise_stats, powspec_t * buf, powspec_t * peak, int32 num_filt)
{
    powspec_t cur_in;
    int i;

    for (i = 0; i < num_filt; i++) {
        cur_in = buf[i];

#ifndef FIXED_POINT
        peak[i] *= noise_stats->lambda_t;
        if (buf[i] < noise_stats->lambda_t * peak[i])
            buf[i] = peak[i] * noise_stats->mu_t;
#else
        peak[i] += noise_stats->lambda_t;
        if (buf[i] < noise_stats->lambda_t + peak[i])
            buf[i] = peak[i] + noise_stats->mu_t;
#endif

        if (cur_in > peak[i])
            peak[i] = cur_in;
    }
}


static void
fe_weight_smooth(noise_stats_t *noise_stats, powspec_t * buf, powspec_t * coefs, int32 num_filt)
{
    int i, j;
    int l1, l2;
    powspec_t coef;

    for (i = 0; i < num_filt; i++) {
        l1 = ((i - SMOOTH_WINDOW) > 0) ? (i - SMOOTH_WINDOW) : 0;
        l2 = ((i + SMOOTH_WINDOW) <
              (num_filt - 1)) ? (i + SMOOTH_WINDOW) : (num_filt - 1);

#ifndef FIXED_POINT
        coef = 0;
        for (j = l1; j <= l2; j++) {
            coef += coefs[j];
        }
        buf[i] = buf[i] * (coef / (l2 - l1 + 1));
#else
        coef = MIN_FIXLOG;
        for (j = l1; j <= l2; j++) {
            coef = fe_log_add(coef, coefs[j]);
        }        
        buf[i] = buf[i] + coef - noise_stats->smooth_scaling[l2 - l1 + 1];
#endif

    }
}

noise_stats_t *
fe_init_noisestats(int num_filters)
{
    int i;
    noise_stats_t *noise_stats;

    noise_stats = (noise_stats_t *) ckd_calloc(1, sizeof(noise_stats_t));

    noise_stats->power =
        (powspec_t *) ckd_calloc(num_filters, sizeof(powspec_t));
    noise_stats->noise =
        (powspec_t *) ckd_calloc(num_filters, sizeof(powspec_t));
    noise_stats->floor =
        (powspec_t *) ckd_calloc(num_filters, sizeof(powspec_t));
    noise_stats->peak =
        (powspec_t *) ckd_calloc(num_filters, sizeof(powspec_t));

    noise_stats->undefined = TRUE;
    noise_stats->num_filters = num_filters;

#ifndef FIXED_POINT
    noise_stats->lambda_power = LAMBDA_POWER;
    noise_stats->comp_lambda_power = 1 - LAMBDA_POWER;
    noise_stats->lambda_a = LAMBDA_A;
    noise_stats->comp_lambda_a = 1 - LAMBDA_A;
    noise_stats->lambda_b = LAMBDA_B;
    noise_stats->comp_lambda_b = 1 - LAMBDA_B;
    noise_stats->lambda_t = LAMBDA_T;
    noise_stats->mu_t = MU_T;
    noise_stats->max_gain = MAX_GAIN;
    noise_stats->inv_max_gain = 1.0 / MAX_GAIN;
    
    for (i = 1; i < 2 * SMOOTH_WINDOW + 1; i++) {
        noise_stats->smooth_scaling[i] = 1.0 / i;
    }
#else
    noise_stats->lambda_power = FLOAT2FIX(log(LAMBDA_POWER));
    noise_stats->comp_lambda_power = FLOAT2FIX(log(1 - LAMBDA_POWER));
    noise_stats->lambda_a = FLOAT2FIX(log(LAMBDA_A));
    noise_stats->comp_lambda_a = FLOAT2FIX(log(1 - LAMBDA_A));
    noise_stats->lambda_b = FLOAT2FIX(log(LAMBDA_B));
    noise_stats->comp_lambda_b = FLOAT2FIX(log(1 - LAMBDA_B));
    noise_stats->lambda_t = FLOAT2FIX(log(LAMBDA_T));
    noise_stats->mu_t = FLOAT2FIX(log(MU_T));
    noise_stats->max_gain = FLOAT2FIX(log(MAX_GAIN));
    noise_stats->inv_max_gain = FLOAT2FIX(log(1.0 / MAX_GAIN));

    for (i = 1; i < 2 * SMOOTH_WINDOW + 3; i++) {
        noise_stats->smooth_scaling[i] = FLOAT2FIX(log(i));
    }
#endif

    return noise_stats;
}

void
fe_reset_noisestats(noise_stats_t * noise_stats)
{
    if (noise_stats)
        noise_stats->undefined = TRUE;
}

void
fe_free_noisestats(noise_stats_t * noise_stats)
{
    ckd_free(noise_stats->power);
    ckd_free(noise_stats->noise);
    ckd_free(noise_stats->floor);
    ckd_free(noise_stats->peak);
    ckd_free(noise_stats);
}





void
fe_track_snr(fe_t * fe, int32 *in_speech)
{
    powspec_t *signal;
    powspec_t *gain;
    noise_stats_t *noise_stats;
    powspec_t *mfspec;
    int32 i, num_filts;
    powspec_t lrt, snr, max_signal, log_signal;

    if (!(fe->remove_noise || fe->remove_silence)) {
        *in_speech = TRUE;
        return;
    }

    noise_stats = fe->noise_stats;
    mfspec = fe->mfspec;
    num_filts = noise_stats->num_filters;

    signal = (powspec_t *) ckd_calloc(num_filts, sizeof(powspec_t));

    if (noise_stats->undefined) {
        for (i = 0; i < num_filts; i++) {
            noise_stats->power[i] = mfspec[i];
            noise_stats->noise[i] = mfspec[i];
#ifndef FIXED_POINT
            noise_stats->floor[i] = mfspec[i] / noise_stats->max_gain;
            noise_stats->peak[i] = 0.0;       
#else
            noise_stats->floor[i] = mfspec[i] - noise_stats->max_gain;
            noise_stats->peak[i] = MIN_FIXLOG;
#endif
        }
        noise_stats->undefined = FALSE;
    }

    
    for (i = 0; i < num_filts; i++) {
#ifndef FIXED_POINT
        noise_stats->power[i] =
            noise_stats->lambda_power * noise_stats->power[i] + noise_stats->comp_lambda_power * mfspec[i];   
#else
        noise_stats->power[i] = fe_log_add(noise_stats->lambda_power + noise_stats->power[i],
            noise_stats->comp_lambda_power + mfspec[i]);
#endif
    }

    
    fe_lower_envelope(noise_stats, noise_stats->power, noise_stats->noise, num_filts);

    lrt = FLOAT2FIX(0.0f);
    max_signal = FLOAT2FIX(0.0f);
    for (i = 0; i < num_filts; i++) {
#ifndef FIXED_POINT
        signal[i] = noise_stats->power[i] - noise_stats->noise[i];
        if (signal[i] < 1.0)
            signal[i] = 1.0;
        snr = log(noise_stats->power[i] / noise_stats->noise[i]);
        log_signal = log(signal[i]);
#else
        signal[i] = fe_log_sub(noise_stats->power[i], noise_stats->noise[i]);
        snr = noise_stats->power[i] - noise_stats->noise[i];
        log_signal = signal[i];
#endif    
        if (snr > lrt) {
            lrt = snr;
            if (log_signal > max_signal) {
		max_signal = log_signal;
    	    }
    	}
    }

#ifndef FIXED_POINT
    if (fe->remove_silence && (lrt < fe->vad_threshold || max_signal < fe->vad_threshold)) {
#else
    if (fe->remove_silence && (lrt < FLOAT2FIX(fe->vad_threshold) || max_signal < FLOAT2FIX(fe->vad_threshold))) {
#endif
        *in_speech = FALSE;
    } else {
	*in_speech = TRUE;
    }

    fe_lower_envelope(noise_stats, signal, noise_stats->floor, num_filts);

    fe_temp_masking(noise_stats, signal, noise_stats->peak, num_filts);

    if (!fe->remove_noise) {
        
        ckd_free(signal);
        return;
    }

    for (i = 0; i < num_filts; i++) {
        if (signal[i] < noise_stats->floor[i])
            signal[i] = noise_stats->floor[i];
    }

    gain = (powspec_t *) ckd_calloc(num_filts, sizeof(powspec_t));
#ifndef FIXED_POINT
    for (i = 0; i < num_filts; i++) {
        if (signal[i] < noise_stats->max_gain * noise_stats->power[i])
            gain[i] = signal[i] / noise_stats->power[i];
        else
            gain[i] = noise_stats->max_gain;
        if (gain[i] < noise_stats->inv_max_gain)
            gain[i] = noise_stats->inv_max_gain;
    }
#else
    for (i = 0; i < num_filts; i++) {
        gain[i] = signal[i] - noise_stats->power[i];
        if (gain[i] > noise_stats->max_gain)
            gain[i] = noise_stats->max_gain;
        if (gain[i] < noise_stats->inv_max_gain)
            gain[i] = noise_stats->inv_max_gain;
    }
#endif

    
    fe_weight_smooth(noise_stats, mfspec, gain, num_filts);

    ckd_free(gain);
    ckd_free(signal);
}

void
fe_vad_hangover(fe_t * fe, mfcc_t * fea, int32 is_speech)
{
    
    fe->vad_data->state_changed = 0;
    if (is_speech) {
        fe->vad_data->postspch_num = 0;
        if (!fe->vad_data->global_state) {
            fe->vad_data->prespch_num++;
            fe_prespch_write_cep(fe->vad_data->prespch_buf, fea);
            
            if (fe->vad_data->prespch_num >= fe->prespch_len) {
                fe->vad_data->prespch_num = 0;
                fe->vad_data->global_state = 1;
                
                fe->vad_data->state_changed = 1;
            }
        }
    } else {
        fe->vad_data->prespch_num = 0;
        fe_prespch_reset_cep(fe->vad_data->prespch_buf);
        if (fe->vad_data->global_state) {
            fe->vad_data->postspch_num++;
            
            if (fe->vad_data->postspch_num >= fe->postspch_len) {
                fe->vad_data->postspch_num = 0;
                fe->vad_data->global_state = 0;
                
                fe->vad_data->state_changed = 1;
            }
        }
    }

    if (fe->vad_data->store_pcm) {
        if (is_speech || fe->vad_data->global_state)
            fe_prespch_write_pcm(fe->vad_data->prespch_buf, fe->spch);
        if (!is_speech && !fe->vad_data->global_state)
            fe_prespch_reset_pcm(fe->vad_data->prespch_buf);
    }
}
