


























































#include <string.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sphinxbase/err.h"
#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/agc.h"


const char *agc_type_str[] = {
    "none",
    "max",
    "emax",
    "noise"
};
static const int n_agc_type_str = sizeof(agc_type_str)/sizeof(agc_type_str[0]);

agc_type_t
agc_type_from_str(const char *str)
{
    int i;

    for (i = 0; i < n_agc_type_str; ++i) {
        if (0 == strcmp(str, agc_type_str[i]))
            return (agc_type_t)i;
    }
    E_FATAL("Unknown AGC type '%s'\n", str);
    return AGC_NONE;
}

agc_t *agc_init(void)
{
    agc_t *agc;
    agc = ckd_calloc(1, sizeof(*agc));
    agc->noise_thresh = FLOAT2MFCC(2.0);
    
    return agc;
}

void agc_free(agc_t *agc)
{
    ckd_free(agc);
}




void
agc_max(agc_t *agc, mfcc_t **mfc, int32 n_frame)
{
    int32 i;

    if (n_frame <= 0)
        return;
    agc->obs_max = mfc[0][0];
    for (i = 1; i < n_frame; i++) {
        if (mfc[i][0] > agc->obs_max) {
            agc->obs_max = mfc[i][0];
            agc->obs_frame = 1;
        }
    }

    E_INFO("AGCMax: obs=max= %.2f\n", agc->obs_max);
    for (i = 0; i < n_frame; i++)
        mfc[i][0] -= agc->obs_max;
}

void
agc_emax_set(agc_t *agc, float32 m)
{
    agc->max = FLOAT2MFCC(m);
    E_INFO("AGCEMax: max= %.2f\n", m);
}

float32
agc_emax_get(agc_t *agc)
{
    return MFCC2FLOAT(agc->max);
}

void
agc_emax(agc_t *agc, mfcc_t **mfc, int32 n_frame)
{
    int i;

    if (n_frame <= 0)
        return;
    for (i = 0; i < n_frame; ++i) {
        if (mfc[i][0] > agc->obs_max) {
            agc->obs_max = mfc[i][0];
            agc->obs_frame = 1;
        }
        mfc[i][0] -= agc->max;
    }
}


void
agc_emax_update(agc_t *agc)
{
    if (agc->obs_frame) {            
        agc->obs_max_sum += agc->obs_max;
        agc->obs_utt++;

        
        agc->max = agc->obs_max_sum / agc->obs_utt;
        if (agc->obs_utt == 16) {
            agc->obs_max_sum /= 2;
            agc->obs_utt = 8;
        }
    }
    E_INFO("AGCEMax: obs= %.2f, new= %.2f\n", agc->obs_max, agc->max);

    
    agc->obs_frame = 0;
    agc->obs_max = FLOAT2MFCC(-1000.0); 
}

void
agc_noise(agc_t *agc,
          mfcc_t **cep,
          int32 nfr)
{
    mfcc_t min_energy; 
    mfcc_t noise_level;        
    int32 i;           
    int32 noise_frames;        

    
    min_energy = cep[0][0];
    for (i = 0; i < nfr; ++i) {
        if (cep[i][0] < min_energy)
            min_energy = cep[i][0];
    }

    
    noise_frames = 0;
    noise_level = 0;
    min_energy += agc->noise_thresh;
    for (i = 0; i < nfr; ++i) {
        if (cep[i][0] < min_energy) {
            noise_level += cep[i][0];
            noise_frames++;
        }
    }

    if (noise_frames > 0) {
        noise_level /= noise_frames;
        E_INFO("AGC NOISE: max= %6.3f\n", MFCC2FLOAT(noise_level));
        
        for (i = 0; i < nfr; i++) {
            cep[i][0] -= noise_level;
        }
    }
}

void
agc_set_threshold(agc_t *agc, float32 threshold)
{
    agc->noise_thresh = FLOAT2MFCC(threshold);
}

float32
agc_get_threshold(agc_t *agc)
{
    return FLOAT2MFCC(agc->noise_thresh);
}
