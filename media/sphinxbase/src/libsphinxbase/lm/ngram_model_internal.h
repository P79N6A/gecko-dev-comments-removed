









































#ifndef __NGRAM_MODEL_INTERNAL_H__
#define __NGRAM_MODEL_INTERNAL_H__

#include "sphinxbase/ngram_model.h"
#include "sphinxbase/hash_table.h"







struct ngram_model_s {
    int refcount;       
    int32 *n_counts;    
    int32 n_1g_alloc;   
    int32 n_words;      

    uint8 n;            
    uint8 n_classes;    
    uint8 writable;     
    uint8 flags;        

    logmath_t *lmath;   
    float32 lw;         
    int32 log_wip;      
    int32 log_uw;       
    int32 log_uniform;  
    int32 log_uniform_weight; 
    int32 log_zero;     
    char **word_str;    
    hash_table_t *wid;  
    int32 *tmp_wids;    
    struct ngram_class_s **classes; 
    struct ngram_funcs_s *funcs;   
};




struct ngram_class_s {
    int32 tag_wid;  
    int32 start_wid; 
    int32 n_words;   
    int32 *prob1;    
    


    struct ngram_hash_s {
        int32 wid;    
        int32 prob1;  
        int32 next;   
    } *nword_hash;
    int32 n_hash;       
    int32 n_hash_inuse; 
};

#define NGRAM_HASH_SIZE 128

#define NGRAM_BASEWID(wid) ((wid)&0xffffff)
#define NGRAM_CLASSID(wid) (((wid)>>24) & 0x7f)
#define NGRAM_CLASSWID(wid,classid) (((classid)<<24) | 0x80000000 | (wid))
#define NGRAM_IS_CLASSWID(wid) ((wid)&0x80000000)

#define UG_ALLOC_STEP 10


typedef struct ngram_funcs_s {
    


    void (*free)(ngram_model_t *model);
    


    int (*apply_weights)(ngram_model_t *model,
                         float32 lw,
                         float32 wip,
                         float32 uw);
    


    int32 (*score)(ngram_model_t *model,
                   int32 wid,
                   int32 *history,
                   int32 n_hist,
                   int32 *n_used);
    



    int32 (*raw_score)(ngram_model_t *model,
                       int32 wid,
                       int32 *history,
                       int32 n_hist,
                       int32 *n_used);
    










    int32 (*add_ug)(ngram_model_t *model,
                    int32 wid, int32 lweight);
    


    void (*flush)(ngram_model_t *model);

    


    ngram_iter_t * (*iter)(ngram_model_t *model, int32 wid, int32 *history, int32 n_hist);

    


    ngram_iter_t * (*mgrams)(ngram_model_t *model, int32 m);

    


    ngram_iter_t * (*successors)(ngram_iter_t *itor);

    


    int32 const * (*iter_get)(ngram_iter_t *itor,
                              int32 *out_score,
                              int32 *out_bowt);

    


    ngram_iter_t * (*iter_next)(ngram_iter_t *itor);

    


    void (*iter_free)(ngram_iter_t *itor);
} ngram_funcs_t;




struct ngram_iter_s {
    ngram_model_t *model;
    int32 *wids;      
    int16 m;          
    int16 successor;  
};




typedef struct classdef_s {
    char **words;
    float32 *weights;
    int32 n_words;
} classdef_t;




int32
ngram_model_init(ngram_model_t *model,
                 ngram_funcs_t *funcs,
                 logmath_t *lmath,
                 int32 n, int32 n_unigram);




ngram_model_t *ngram_model_arpa_read(cmd_ln_t *config,
				     const char *file_name,
				     logmath_t *lmath);



ngram_model_t *ngram_model_dmp_read(cmd_ln_t *config,
				    const char *file_name,
				    logmath_t *lmath);



ngram_model_t *ngram_model_dmp32_read(cmd_ln_t *config,
				     const char *file_name,
				     logmath_t *lmath);




int ngram_model_arpa_write(ngram_model_t *model,
			   const char *file_name);



int ngram_model_dmp_write(ngram_model_t *model,
			  const char *file_name);




int32 read_classdef_file(hash_table_t *classes, const char *classdef_file);




void classdef_free(classdef_t *classdef);




ngram_class_t *ngram_class_new(ngram_model_t *model, int32 tag_wid,
                               int32 start_wid, glist_t classwords);




void ngram_class_free(ngram_class_t *lmclass);






int32 ngram_class_prob(ngram_class_t *lmclass, int32 wid);




void ngram_iter_init(ngram_iter_t *itor, ngram_model_t *model,
                     int m, int successor);

#endif 
