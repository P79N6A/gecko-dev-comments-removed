








































#ifndef __S2_SEMI_MGAU_H__
#define __S2_SEMI_MGAU_H__


#include <sphinxbase/fe.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/mmio.h>


#include "acmod.h"
#include "hmm.h"
#include "bin_mdef.h"
#include "ms_gauden.h"

typedef struct vqFeature_s vqFeature_t;

typedef struct s2_semi_mgau_s s2_semi_mgau_t;
struct s2_semi_mgau_s {
    ps_mgau_t base;     
    cmd_ln_t *config;   

    gauden_t *g;        

    uint8 ***mixw;     
    mmio_file_t *sendump_mmap;

    uint8 *mixw_cb;    
    int32 n_sen;	
    uint8 *topn_beam;   
    int16 max_topn;
    int16 ds_ratio;

    vqFeature_t ***topn_hist; 
    uint8 **topn_hist_n;      
    vqFeature_t **f;          
    int n_topn_hist;          

    
    logmath_t *lmath_8b;
    
    logmath_t *lmath;
};

ps_mgau_t *s2_semi_mgau_init(acmod_t *acmod);
void s2_semi_mgau_free(ps_mgau_t *s);
int s2_semi_mgau_frame_eval(ps_mgau_t *s,
                            int16 *senone_scores,
                            uint8 *senone_active,
                            int32 n_senone_active,
                            mfcc_t **featbuf,
                            int32 frame,
                            int32 compallsen);
int s2_semi_mgau_mllr_transform(ps_mgau_t *s,
                                ps_mllr_t *mllr);


#endif 
