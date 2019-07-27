










































#ifndef __POCKETSPHINX_INTERNAL_H__
#define __POCKETSPHINX_INTERNAL_H__


#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/fe.h>
#include <sphinxbase/feat.h>
#include <sphinxbase/hash_table.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/profile.h>


#include "pocketsphinx.h"
#include "acmod.h"
#include "dict.h"
#include "dict2pid.h"




typedef struct ps_search_s ps_search_t;

#define PS_DEFAULT_SEARCH  "default"
#define PS_SEARCH_KWS    "kws"
#define PS_SEARCH_FSG    "fsg"
#define PS_SEARCH_NGRAM  "ngram"




typedef struct ps_searchfuncs_s {
    char const *name;

    int (*start)(ps_search_t *search);
    int (*step)(ps_search_t *search, int frame_idx);
    int (*finish)(ps_search_t *search);
    int (*reinit)(ps_search_t *search, dict_t *dict, dict2pid_t *d2p);
    void (*free)(ps_search_t *search);

    ps_lattice_t *(*lattice)(ps_search_t *search);
    char const *(*hyp)(ps_search_t *search, int32 *out_score, int32 *out_is_final);
    int32 (*prob)(ps_search_t *search);
    ps_seg_t *(*seg_iter)(ps_search_t *search, int32 *out_score);
} ps_searchfuncs_t;




struct ps_search_s {
    ps_searchfuncs_t *vt;  

    ps_search_t *pls;      
    cmd_ln_t *config;      
    acmod_t *acmod;        
    dict_t *dict;        
    dict2pid_t *d2p;       
    char *hyp_str;         
    ps_lattice_t *dag;	   
    ps_latlink_t *last_link; 
    int32 post;            
    int32 n_words;         


    
    int32 start_wid;       
    int32 silence_wid;     
    int32 finish_wid;      
};

#define ps_search_base(s) ((ps_search_t *)s)
#define ps_search_config(s) ps_search_base(s)->config
#define ps_search_acmod(s) ps_search_base(s)->acmod
#define ps_search_dict(s) ps_search_base(s)->dict
#define ps_search_dict2pid(s) ps_search_base(s)->d2p
#define ps_search_dag(s) ps_search_base(s)->dag
#define ps_search_last_link(s) ps_search_base(s)->last_link
#define ps_search_post(s) ps_search_base(s)->post
#define ps_search_lookahead(s) ps_search_base(s)->pls
#define ps_search_n_words(s) ps_search_base(s)->n_words

#define ps_search_name(s) ps_search_base(s)->vt->name
#define ps_search_start(s) (*(ps_search_base(s)->vt->start))(s)
#define ps_search_step(s,i) (*(ps_search_base(s)->vt->step))(s,i)
#define ps_search_finish(s) (*(ps_search_base(s)->vt->finish))(s)
#define ps_search_reinit(s,d,d2p) (*(ps_search_base(s)->vt->reinit))(s,d,d2p)
#define ps_search_free(s) (*(ps_search_base(s)->vt->free))(s)
#define ps_search_lattice(s) (*(ps_search_base(s)->vt->lattice))(s)
#define ps_search_hyp(s,sc,final) (*(ps_search_base(s)->vt->hyp))(s,sc,final)
#define ps_search_prob(s) (*(ps_search_base(s)->vt->prob))(s)
#define ps_search_seg_iter(s,sc) (*(ps_search_base(s)->vt->seg_iter))(s,sc)


#define ps_search_silence_wid(s) ps_search_base(s)->silence_wid
#define ps_search_start_wid(s) ps_search_base(s)->start_wid
#define ps_search_finish_wid(s) ps_search_base(s)->finish_wid




void ps_search_init(ps_search_t *search, ps_searchfuncs_t *vt,
                    cmd_ln_t *config, acmod_t *acmod, dict_t *dict,
                    dict2pid_t *d2p);




void ps_search_base_reinit(ps_search_t *search, dict_t *dict,
                           dict2pid_t *d2p);




void ps_search_deinit(ps_search_t *search);

typedef struct ps_segfuncs_s {
    ps_seg_t *(*seg_next)(ps_seg_t *seg);
    void (*seg_free)(ps_seg_t *seg);
} ps_segfuncs_t;




struct ps_seg_s {
    ps_segfuncs_t *vt;     
    ps_search_t *search;   
    char const *word;      
    frame_idx_t sf;        
    frame_idx_t ef;        
    int32 ascr;            
    int32 lscr;            
    int32 prob;            
    

    int32 lback;           
    
    float32 lwf;           
};

#define ps_search_seg_next(seg) (*(seg->vt->seg_next))(seg)
#define ps_search_seg_free(s) (*(seg->vt->seg_free))(seg)





struct ps_decoder_s {
    
    cmd_ln_t *config;  
    int refcount;      

    
    acmod_t *acmod;    
    dict_t *dict;    
    dict2pid_t *d2p;   
    logmath_t *lmath;  

    
    hash_table_t *searches;        
    

    ps_search_t *search;     
    ps_search_t *phone_loop; 
    int pl_window;           

    
    uint32 uttno;       
    ptmr_t perf;        
    uint32 n_frame;     
    char const *mfclogdir; 
    char const *rawlogdir; 
    char const *senlogdir; 
};


struct ps_search_iter_s {
    hash_iter_t itor;
};

#endif 
