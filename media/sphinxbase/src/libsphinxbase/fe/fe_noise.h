



































#ifndef FE_NOISE_H
#define FE_NOISE_H

#include "sphinxbase/fe.h"
#include "sphinxbase/fixpoint.h"
#include "fe_type.h"

typedef struct noise_stats_s noise_stats_t;


noise_stats_t *fe_init_noisestats(int num_filters);


void fe_reset_noisestats(noise_stats_t * noise_stats);


void fe_free_noisestats(noise_stats_t * noise_stats);





void fe_track_snr(fe_t *fe, int32 *in_speech);




void fe_vad_hangover(fe_t *fe, mfcc_t *fea, int32 is_speech);

#endif                          
