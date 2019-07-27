








































#ifndef __HMM_H__
#define __HMM_H__


#include <stdio.h>


#include <sphinxbase/fixpoint.h>
#include <sphinxbase/listelem_alloc.h>


#include "bin_mdef.h"

#ifdef __cplusplus
extern "C" {
#endif






typedef int32 frame_idx_t;




#define MAX_N_FRAMES MAX_INT32



#define SENSCR_SHIFT 10










#define WORST_SCORE		((int)0xE0000000)





#define TMAT_WORST_SCORE	(-255)




#define BETTER_THAN >




#define WORSE_THAN <













































typedef struct hmm_context_s {
    int32 n_emit_state;     
    uint8 ** const *tp;	    
    int16 const *senscore;  

    uint16 * const *sseq;   
    int32 *st_sen_scr;      
    listelem_alloc_t *mpx_ssid_alloc; 
    void *udata;            
} hmm_context_t;




#define HMM_MAX_NSTATE 5









typedef struct hmm_s {
    hmm_context_t *ctx;            
    int32 score[HMM_MAX_NSTATE];   
    int32 history[HMM_MAX_NSTATE]; 
    int32 out_score;               
    int32 out_history;             
    uint16 ssid;                   
    uint16 senid[HMM_MAX_NSTATE];  
    int32 bestscore;	
    int16 tmatid;       
    frame_idx_t frame;  
    uint8 mpx;          
    uint8 n_emit_state; 
} hmm_t;


#define hmm_context(h) (h)->ctx
#define hmm_is_mpx(h) (h)->mpx

#define hmm_in_score(h) (h)->score[0]
#define hmm_score(h,st) (h)->score[st]
#define hmm_out_score(h) (h)->out_score

#define hmm_in_history(h) (h)->history[0]
#define hmm_history(h,st) (h)->history[st]
#define hmm_out_history(h) (h)->out_history

#define hmm_bestscore(h) (h)->bestscore
#define hmm_frame(h) (h)->frame
#define hmm_mpx_ssid(h,st) (h)->senid[st]
#define hmm_nonmpx_ssid(h) (h)->ssid
#define hmm_ssid(h,st) (hmm_is_mpx(h)                                   \
                        ? hmm_mpx_ssid(h,st) : hmm_nonmpx_ssid(h))
#define hmm_mpx_senid(h,st) (hmm_mpx_ssid(h,st) == BAD_SENID \
                             ? BAD_SENID : (h)->ctx->sseq[hmm_mpx_ssid(h,st)][st])
#define hmm_nonmpx_senid(h,st) ((h)->senid[st])
#define hmm_senid(h,st) (hmm_is_mpx(h)                                  \
                         ? hmm_mpx_senid(h,st) : hmm_nonmpx_senid(h,st))
#define hmm_senscr(h,st) (hmm_senid(h,st) == BAD_SENID                  \
                          ? WORST_SCORE                                 \
                          : -(h)->ctx->senscore[hmm_senid(h,st)])
#define hmm_tmatid(h) (h)->tmatid
#define hmm_tprob(h,i,j) (-(h)->ctx->tp[hmm_tmatid(h)][i][j])
#define hmm_n_emit_state(h) ((h)->n_emit_state)
#define hmm_n_state(h) ((h)->n_emit_state + 1)




hmm_context_t *hmm_context_init(int32 n_emit_state,
                                uint8 ** const *tp,
                                int16 const *senscore,
                                uint16 * const *sseq);




#define hmm_context_set_senscore(ctx, senscr) ((ctx)->senscore = (senscr))








void hmm_context_free(hmm_context_t *ctx);




void hmm_init(hmm_context_t *ctx, hmm_t *hmm, int mpx, int ssid, int tmatid);




void hmm_deinit(hmm_t *hmm);






void hmm_clear(hmm_t *h);




void hmm_clear_scores(hmm_t *h);




void hmm_normalize(hmm_t *h, int32 bestscr);




void hmm_enter(hmm_t *h, int32 score,
               int32 histid, int frame);













int32 hmm_vit_eval(hmm_t *hmm);
  




int32 hmm_dump_vit_eval(hmm_t *hmm,  
                        FILE *fp 
    );





void hmm_dump(hmm_t *h,  
              FILE *fp 
    );


#ifdef __cplusplus
}
#endif

#endif
