








































#ifndef __NGRAM_SEARCH_H__
#define __NGRAM_SEARCH_H__


#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/ngram_model.h>
#include <sphinxbase/listelem_alloc.h>
#include <sphinxbase/err.h>


#include "pocketsphinx_internal.h"
#include "hmm.h"









typedef struct chan_s {
    hmm_t hmm;                  


    struct chan_s *next;	


    struct chan_s *alt;		

    int32    ciphone;		
    union {
	int32 penult_phn_wid;	



	int32 rc_id;		
    } info;
} chan_t;








typedef struct root_chan_s {
    hmm_t hmm;                  


    chan_t *next;		

    int32  penult_phn_wid;
    int32  this_phn_wid;	


    int16    ciphone;		

    int16    ci2phone;		

} root_chan_t;




typedef struct bptbl_s {
    frame_idx_t  frame;		
    uint8    valid;		
    uint8    refcnt;            
    int32    wid;		
    int32    bp;		
    int32    score;		
    int32    s_idx;		
    int32    real_wid;		
    int32    prev_real_wid;	
    int16    last_phone;        
    int16    last2_phone;       
} bptbl_t;




typedef struct bptbl_seg_s {
    ps_seg_t base;  
    int32 *bpidx;   
    int16 n_bpidx;  
    int16 cur;      
} bptbl_seg_t;






typedef struct lastphn_cand_s {
    int32 wid;
    int32 score;
    int32 bp;
    int32 next;                 
} lastphn_cand_t;







typedef struct {
    int32 sf;                   
    int32 dscr;                 
    int32 bp;                   
} last_ltrans_t;

#define CAND_SF_ALLOCSIZE	32
typedef struct {
    int32 bp_ef;
    int32 cand;
} cand_sf_t;







typedef struct bestbp_rc_s {
    int32 score;
    int32 path;                 
    int32 lc;                   
} bestbp_rc_t;

#define NO_BP		-1




typedef struct ngram_search_stats_s {
    int32 n_phone_eval;
    int32 n_root_chan_eval;
    int32 n_nonroot_chan_eval;
    int32 n_last_chan_eval;
    int32 n_word_lastchan_eval;
    int32 n_lastphn_cand_utt;
    int32 n_fwdflat_chan;
    int32 n_fwdflat_words;
    int32 n_fwdflat_word_transition;
    int32 n_senone_active_utt;
} ngram_search_stats_t;





struct ngram_search_s {
    ps_search_t base;
    ngram_model_t *lmset;  
    hmm_context_t *hmmctx; 

    
    uint8 fwdtree;
    uint8 fwdflat;
    uint8 bestpath;

    
    uint8 done;

    
    listelem_alloc_t *chan_alloc; 
    listelem_alloc_t *root_chan_alloc; 
    listelem_alloc_t *latnode_alloc; 

    















    root_chan_t *root_chan;  
    int32 n_root_chan_alloc; 
    int32 n_root_chan;       
    int32 n_nonroot_chan;    
    int32 max_nonroot_chan;  
    root_chan_t *rhmm_1ph;   

    







    chan_t **word_chan;
    bitvec_t *word_active;      

    













    int32 *homophone_set;
    int32 *single_phone_wid; 
    int32 n_1ph_words;       
    int32 n_1ph_LMwords;     

    






    chan_t ***active_chan_list;
    int32 n_active_chan[2];  
    









    int32 **active_word_list;
    int32 n_active_word[2];  

    


    lastphn_cand_t *lastphn_cand;
    int32 n_lastphn_cand;
    last_ltrans_t *last_ltrans;      
    int32 cand_sf_alloc;
    cand_sf_t *cand_sf;
    bestbp_rc_t *bestbp_rc;

    bptbl_t *bp_table;       
    int32 bpidx;             
    int32 bp_table_size;
    int32 *bscore_stack;     
    int32 bss_head;          
    int32 bscore_stack_size;

    int32 n_frame_alloc; 
    int32 n_frame;       
    int32 *bp_table_idx; 
    int32 *word_lat_idx; 


    


    ps_latnode_t **frm_wordlist;   
    int32 *fwdflat_wordlist;    
    bitvec_t *expand_word_flag;
    int32 *expand_word_list;
    int32 n_expand_words;
    int32 min_ef_width;
    int32 max_sf_win;
    float32 fwdflat_fwdtree_lw_ratio;

    int32 best_score; 
    int32 last_phone_best_score; 
    int32 renormalized;

    


    float32 bestpath_fwdtree_lw_ratio;
    float32 ascale; 
    
    ngram_search_stats_t st; 
    ptmr_t fwdtree_perf;
    ptmr_t fwdflat_perf;
    ptmr_t bestpath_perf;
    int32 n_tot_frame;

    
    int32 beam;
    int32 dynamic_beam;
    int32 pbeam;
    int32 wbeam;
    int32 lpbeam;
    int32 lponlybeam;
    int32 fwdflatbeam;
    int32 fwdflatwbeam;
    int32 fillpen;
    int32 silpen;
    int32 wip;
    int32 nwpen;
    int32 pip;
    int32 maxwpf;
    int32 maxhmmpf;
};
typedef struct ngram_search_s ngram_search_t;




ps_search_t *ngram_search_init(ngram_model_t *lm,
                               cmd_ln_t *config,
                               acmod_t *acmod,
                               dict_t *dict,
                               dict2pid_t *d2p);




void ngram_search_free(ps_search_t *ngs);






int ngram_search_mark_bptable(ngram_search_t *ngs, int frame_idx);




void ngram_search_save_bp(ngram_search_t *ngs, int frame_idx, int32 w,
                          int32 score, int32 path, int32 rc);




void ngram_search_alloc_all_rc(ngram_search_t *ngs, int32 w);




void ngram_search_free_all_rc(ngram_search_t *ngs, int32 w);






int ngram_search_find_exit(ngram_search_t *ngs, int frame_idx, int32 *out_best_score, int32 *out_is_final);






char const *ngram_search_bp_hyp(ngram_search_t *ngs, int bpidx);




void ngram_compute_seg_scores(ngram_search_t *ngs, float32 lwf);




ps_lattice_t *ngram_search_lattice(ps_search_t *search);




int32 ngram_search_exit_score(ngram_search_t *ngs, bptbl_t *pbe, int rcphone);






void ngram_search_set_lm(ngram_model_t *lm);

#endif 
