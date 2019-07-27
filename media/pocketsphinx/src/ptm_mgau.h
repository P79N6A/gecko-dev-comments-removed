








































#ifndef __PTM_MGAU_H__
#define __PTM_MGAU_H__


#include <sphinxbase/fe.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/mmio.h>


#include "acmod.h"
#include "hmm.h"
#include "bin_mdef.h"
#include "ms_gauden.h"

typedef struct ptm_mgau_s ptm_mgau_t;

typedef struct ptm_topn_s {
    int32 cw;    
    int32 score; 
} ptm_topn_t;

typedef struct ptm_fast_eval_s {
    ptm_topn_t ***topn;     
    bitvec_t *mgau_active; 
} ptm_fast_eval_t;

struct ptm_mgau_s {
    ps_mgau_t base;     
    cmd_ln_t *config;   
    gauden_t *g;        
    int32 n_sen;       
    uint8 *sen2cb;     
    uint8 ***mixw;     
    mmio_file_t *sendump_mmap;
    uint8 *mixw_cb;    
    int16 max_topn;
    int16 ds_ratio;

    ptm_fast_eval_t *hist;   
    ptm_fast_eval_t *f;      
    int n_fast_hist;         

    
    logmath_t *lmath_8b;
    
    logmath_t *lmath;
};

ps_mgau_t *ptm_mgau_init(acmod_t *acmod, bin_mdef_t *mdef);
void ptm_mgau_free(ps_mgau_t *s);
int ptm_mgau_frame_eval(ps_mgau_t *s,
                        int16 *senone_scores,
                        uint8 *senone_active,
                        int32 n_senone_active,
                        mfcc_t **featbuf,
                        int32 frame,
                        int32 compallsen);
int ptm_mgau_mllr_transform(ps_mgau_t *s,
                            ps_mllr_t *mllr);


#endif 
