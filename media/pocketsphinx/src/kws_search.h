





































#ifndef __KWS_SEARCH_H__
#define __KWS_SEARCH_H__


#include <sphinxbase/glist.h>
#include <sphinxbase/cmd_ln.h>


#include "pocketsphinx_internal.h"
#include "kws_detections.h"
#include "hmm.h"




typedef struct kws_seg_s {
    ps_seg_t base;       
    gnode_t *detection;  
} kws_seg_t;

typedef struct kws_keyword_s {
    char* word;
    int32 threshold;
    hmm_t* hmms;
    int32 n_hmms;
} kws_keyword_t;




typedef struct kws_search_s {
    ps_search_t base;

    hmm_context_t *hmmctx;        

    kws_detections_t *detections; 
    kws_keyword_t* keyphrases;   
    int n_keyphrases;             
    frame_idx_t frame;            

    int32 beam;

    int32 plp;                    
    int32 bestscore;              
    int32 def_threshold;          

    int32 n_pl;                   
    hmm_t *pl_hmms;               

} kws_search_t;





ps_search_t *kws_search_init(const char *keyphrase,
			     const char *keyfile,
                             cmd_ln_t * config,
                             acmod_t * acmod,
                             dict_t * dict, dict2pid_t * d2p);




void kws_search_free(ps_search_t * search);




int kws_search_reinit(ps_search_t * kwss, dict_t * dict, dict2pid_t * d2p);





int kws_search_start(ps_search_t * search);




int kws_search_step(ps_search_t * search, int frame_idx);




int kws_search_finish(ps_search_t * search);




char const *kws_search_hyp(ps_search_t * search, int32 * out_score,
                           int32 * out_is_final);



char* kws_search_get_keywords(ps_search_t * search);

#endif                          
