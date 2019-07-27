






































#ifndef __S2_FSG_SEARCH_H__
#define __S2_FSG_SEARCH_H__



#include <sphinxbase/glist.h>
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/fsg_model.h>


#include "pocketsphinx_internal.h"
#include "hmm.h"
#include "fsg_history.h"
#include "fsg_lextree.h"




typedef struct fsg_seg_s {
    ps_seg_t base;  
    fsg_hist_entry_t **hist;   
    int16 n_hist;  
    int16 cur;      
} fsg_seg_t;




typedef struct fsg_search_s {
    ps_search_t base;

    hmm_context_t *hmmctx; 

    fsg_model_t *fsg;		
    struct fsg_lextree_s *lextree;

    struct fsg_history_s *history;
  
    glist_t pnode_active;	
    glist_t pnode_active_next;	
  
    int32 beam_orig;		
    int32 pbeam_orig;		
    int32 wbeam_orig;		
    float32 beam_factor;	


    int32 beam, pbeam, wbeam;	
    int32 lw, pip, wip;         
  
    frame_idx_t frame;		
    uint8 final;		
    uint8 bestpath;		

    float32 ascale;             

    int32 bestscore;		
    int32 bpidx_start;		
  
    int32 ascr, lscr;		
  
    int32 n_hmm_eval;		
    int32 n_sen_eval;		
} fsg_search_t;


#define fsg_search_frame(s)	((s)->frame)




ps_search_t *fsg_search_init(fsg_model_t *fsg,
                             cmd_ln_t *config,
                             acmod_t *acmod,
                             dict_t *dict,
                             dict2pid_t *d2p);




void fsg_search_free(ps_search_t *search);




int fsg_search_reinit(ps_search_t *fsgs, dict_t *dict, dict2pid_t *d2p);





int fsg_search_start(ps_search_t *search);




int fsg_search_step(ps_search_t *search, int frame_idx);




int fsg_search_finish(ps_search_t *search);




char const *fsg_search_hyp(ps_search_t *search, int32 *out_score, int32 *out_is_final);

#endif
