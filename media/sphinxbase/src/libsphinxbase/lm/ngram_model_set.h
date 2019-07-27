








































#ifndef __NGRAM_MODEL_SET_H__
#define __NGRAM_MODEL_SET_H__

#include "ngram_model_internal.h"
#include "lm3g_model.h"




typedef struct ngram_model_set_s {
    ngram_model_t base;  

    int32 n_models;      
    int32 cur;           
    ngram_model_t **lms; 
    char **names;        
    int32 *lweights;     
    int32 **widmap;      
    int32 *maphist;      
} ngram_model_set_t;




struct ngram_model_set_iter_s {
    ngram_model_set_t *set;
    int32 cur;
};

#endif 
