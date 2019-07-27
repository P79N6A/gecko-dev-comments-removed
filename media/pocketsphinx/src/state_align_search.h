








































#ifndef __STATE_ALIGN_SEARCH_H__
#define __STATE_ALIGN_SEARCH_H__


#include <sphinxbase/prim_type.h>


#include "pocketsphinx_internal.h"
#include "ps_alignment.h"
#include "hmm.h"




struct state_align_search_s {
    ps_search_t base;       
    hmm_context_t *hmmctx;  
    ps_alignment_t *al;     
    hmm_t *hmms;            
    int n_phones;	    

    int frame;              
    int32 best_score;       

    int n_emit_state;       
    uint16 *tokens;         
    int n_fr_alloc;         
};
typedef struct state_align_search_s state_align_search_t;

ps_search_t *state_align_search_init(cmd_ln_t *config,
                                     acmod_t *acmod,
                                     ps_alignment_t *al);

#endif 
