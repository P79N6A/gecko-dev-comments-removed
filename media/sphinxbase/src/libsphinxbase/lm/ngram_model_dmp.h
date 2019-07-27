









































#ifndef __NGRAM_MODEL_DMP_H__
#define __NGRAM_MODEL_DMP_H__

#include "sphinxbase/mmio.h"

#include "ngram_model_internal.h"
#include "lm3g_model.h"




struct bigram_s {
    uint16 wid;	
    uint16 prob2;	
    uint16 bo_wt2;	
    uint16 trigrams;	

};







struct trigram_s {
    uint16 wid;	  
    uint16 prob3; 
};




typedef struct ngram_model_dmp_s {
    ngram_model_t base;  
    lm3g_model_t lm3g;   
    mmio_file_t *dump_mmap; 
} ngram_model_dmp_t;








ngram_model_dmp_t *ngram_model_dmp_build(ngram_model_t *base);


#endif 
