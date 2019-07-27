






































#ifndef __ALLPHONE_SEARCH_H__
#define __ALLPHONE_SEARCH_H__



#include <sphinxbase/glist.h>
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/ngram_model.h>
#include <sphinxbase/bitvec.h>


#include "pocketsphinx_internal.h"
#include "blkarray_list.h"
#include "hmm.h"






typedef struct phmm_s {
    hmm_t hmm;          
    s3pid_t pid;        
    s3cipid_t ci;       
    bitvec_t *lc;         
    bitvec_t *rc;         
    struct phmm_s *next;        
    struct plink_s *succlist;   
} phmm_t;




typedef struct plink_s {
    phmm_t *phmm;               
    struct plink_s *next;       
} plink_t;




typedef struct history_s {
    phmm_t *phmm;       
    int32 score;        
    int32 tscore;       
    frame_idx_t ef;     
    int32 hist;         
} history_t;




typedef struct phseg_s {
    s3cipid_t ci;               
    frame_idx_t sf, ef;         
    int32 score;                
    int32 tscore;               
} phseg_t;




typedef struct phseg_iter_s {
    ps_seg_t base;
    glist_t seg;
} phseg_iter_t;




typedef struct allphone_search_s {
    ps_search_t base;

    hmm_context_t *hmmctx;    
    ngram_model_t *lm;        
    int32 ci_only; 	      
    phmm_t **ci_phmm;         
    int32 *ci2lmwid;          

    int32 beam, pbeam;        
    int32 lw, inspen;         

    frame_idx_t frame;          
    float32 ascale;           

    int32 n_tot_frame;         
    int32 n_hmm_eval;          
    int32 n_sen_eval;          

    
    blkarray_list_t *history;     
    
    glist_t segments;

    ptmr_t perf; 

} allphone_search_t;




ps_search_t *allphone_search_init(ngram_model_t * lm,
                                  cmd_ln_t * config,
                                  acmod_t * acmod,
                                  dict_t * dict, dict2pid_t * d2p);




void allphone_search_free(ps_search_t * search);




int allphone_search_reinit(ps_search_t * search, dict_t * dict,
                           dict2pid_t * d2p);





int allphone_search_start(ps_search_t * search);




int allphone_search_step(ps_search_t * search, int frame_idx);




int allphone_search_finish(ps_search_t * search);




char const *allphone_search_hyp(ps_search_t * search, int32 * out_score,
                                int32 * out_is_final);

#endif                          
