






























































































#ifndef _LIBFBS_MS_CONT_MGAU_H_
#define _LIBFBS_MS_CONT_MGAU_H_


#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/feat.h>


#include "acmod.h"
#include "bin_mdef.h"
#include "ms_gauden.h"
#include "ms_senone.h"





typedef struct {
    ps_mgau_t base;
    gauden_t* g;   
    senone_t* s;   
    int topn;      

    
    gauden_dist_t ***dist;  
    uint8 *mgau_active;
    cmd_ln_t *config;
} ms_mgau_model_t;  

#define ms_mgau_gauden(msg) (msg->g)
#define ms_mgau_senone(msg) (msg->s)
#define ms_mgau_topn(msg) (msg->topn)

ps_mgau_t* ms_mgau_init(acmod_t *acmod, logmath_t *lmath, bin_mdef_t *mdef);
void ms_mgau_free(ps_mgau_t *g);
int32 ms_cont_mgau_frame_eval(ps_mgau_t * msg,
                              int16 *senscr,
                              uint8 *senone_active,
                              int32 n_senone_active,
                              mfcc_t ** feat,
                              int32 frame,
                              int32 compallsen);
int32 ms_mgau_mllr_transform(ps_mgau_t *s,
                             ps_mllr_t *mllr);

#endif 

