










































#ifndef __NGRAM_MODEL_LM3G_H__
#define __NGRAM_MODEL_LM3G_H__

#include "sphinxbase/listelem_alloc.h"

#include "ngram_model_internal.h"




typedef union {
    float32 f;
    int32 l;
} lmprob_t;










typedef struct sorted_entry_s {
    lmprob_t val;               
    uint32 lower;               


    uint32 higher;              


} sorted_entry_t;





typedef struct {
    sorted_entry_t *list;
    int32 free;                 
    int32 size;
} sorted_list_t;




typedef struct unigram_s {
    lmprob_t prob1;     
    lmprob_t bo_wt1;    
    int32 bigrams;	
} unigram_t;




typedef struct bigram_s bigram_t;



typedef struct trigram_s trigram_t;














#define BG_SEG_SZ	512	/* chosen so that #trigram/segment <= 2**16 */
#define LOG_BG_SEG_SZ	9








typedef struct tginfo_s {
    int32 w1;			

    int32 n_tg;			
    int32 bowt;                 
    int32 used;			
    trigram_t *tg;		
    struct tginfo_s *next;      
} tginfo_t;




typedef struct lm3g_model_s {
    unigram_t *unigrams;
    bigram_t *bigrams;
    trigram_t *trigrams;
    lmprob_t *prob2;	     
    int32 n_prob2;	     
    lmprob_t *bo_wt2;	     
    int32 n_bo_wt2;	     
    lmprob_t *prob3;	     
    int32 n_prob3;	     
    int32 *tseg_base;    

    tginfo_t **tginfo;   

    listelem_alloc_t *le; 
} lm3g_model_t;

void lm3g_tginfo_free(ngram_model_t *base, lm3g_model_t *lm3g);
void lm3g_tginfo_reset(ngram_model_t *base, lm3g_model_t *lm3g);
void lm3g_apply_weights(ngram_model_t *base,
			lm3g_model_t *lm3g,
			float32 lw, float32 wip, float32 uw);
int32 lm3g_add_ug(ngram_model_t *base,
                  lm3g_model_t *lm3g, int32 wid, int32 lweight);






void init_sorted_list(sorted_list_t *l);
void free_sorted_list(sorted_list_t *l);
lmprob_t *vals_in_sorted_list(sorted_list_t *l);
int32 sorted_id(sorted_list_t * l, int32 *val);

#endif 
