









































#ifndef __NGRAM_MODEL_ARPA_H__
#define __NGRAM_MODEL_ARPA_H__

#include "ngram_model_internal.h"
#include "lm3g_model.h"




struct bigram_s {
    uint32 wid;	
    uint16 prob2;	
    uint16 bo_wt2;	
    uint16 trigrams;	

};







struct trigram_s {
    uint32 wid;	  
    uint16 prob3; 
};





typedef struct ngram_model_arpa_s {
    ngram_model_t base;  
    lm3g_model_t lm3g;  

    

    sorted_list_t sorted_prob2;
    sorted_list_t sorted_bo_wt2;
    sorted_list_t sorted_prob3;
} ngram_model_arpa_t;

#endif 
