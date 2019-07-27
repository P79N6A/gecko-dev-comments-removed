













































#ifndef __PHONE_LOOP_SEARCH_H__
#define __PHONE_LOOP_SEARCH_H__


#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/ngram_model.h>
#include <sphinxbase/listelem_alloc.h>


#include "pocketsphinx_internal.h"
#include "hmm.h"




struct phone_loop_renorm_s {
    int frame_idx;  
    int32 norm;     
};
typedef struct phone_loop_renorm_s phone_loop_renorm_t;




struct phone_loop_search_s {
    ps_search_t base;                  
    hmm_t *hmms;                       
    hmm_context_t *hmmctx;             
    int16 frame;                       
    int16 n_phones;                    
    int32 **pen_buf;                
    int16 pen_buf_ptr;                 
    int32 *penalties;                  
    float64 penalty_weight;            

    int32 best_score;                  
    int32 beam;                        
    int32 pbeam;                       
    int32 pip;                         
    int window;                        
    glist_t renorm;                    
};
typedef struct phone_loop_search_s phone_loop_search_t;

ps_search_t *phone_loop_search_init(cmd_ln_t *config,
                                    acmod_t *acmod,
                                    dict_t *dict);




#define phone_loop_search_score(pls,ci) \
    ((pls == NULL) ? 0 : (pls->penalties[ci]))

#endif 
