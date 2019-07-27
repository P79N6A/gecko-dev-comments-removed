






































#ifndef FE_INTERNAL_H
#define FE_INTERNAL_H

#include "sphinxbase/fe.h"

typedef struct prespch_buf_s prespch_buf_t;


prespch_buf_t *fe_prespch_init(int num_frames, int num_cepstra,
                               int num_samples);


void fe_prespch_extend_pcm(prespch_buf_t* prespch_buf, int num_frames_pcm);


int fe_prespch_read_cep(prespch_buf_t * prespch_buf, mfcc_t * fea);


void fe_prespch_write_cep(prespch_buf_t * prespch_buf, mfcc_t * fea);


void fe_prespch_read_pcm(prespch_buf_t * prespch_buf, int16 ** samples,
                         int32 * samples_num);


void fe_prespch_write_pcm(prespch_buf_t * prespch_buf, int16 * samples);


void fe_prespch_reset_cep(prespch_buf_t * prespch_buf);


void fe_prespch_reset_pcm(prespch_buf_t * prespch_buf);


void fe_prespch_free(prespch_buf_t * prespch_buf);


int32 fe_prespch_ncep(prespch_buf_t * prespch_buf);

#endif                          
