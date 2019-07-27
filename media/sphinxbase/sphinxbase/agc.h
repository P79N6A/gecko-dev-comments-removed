

































































#ifndef _S3_AGC_H_
#define _S3_AGC_H_


#include <sphinxbase/sphinxbase_export.h>

#include <sphinxbase/prim_type.h>
#include <sphinxbase/fe.h>









#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif




typedef enum agc_type_e {
    AGC_NONE = 0,
    AGC_MAX,
    AGC_EMAX,
    AGC_NOISE
} agc_type_t;


SPHINXBASE_EXPORT
agc_type_t agc_type_from_str(const char *str);


SPHINXBASE_EXPORT
extern const char *agc_type_str[];




typedef struct agc_s {
    mfcc_t max;      
    mfcc_t obs_max;  
    int32 obs_frame; 
    int32 obs_utt;   
    mfcc_t obs_max_sum;
    mfcc_t noise_thresh; 
} agc_t;




SPHINXBASE_EXPORT
agc_t *agc_init(void);




SPHINXBASE_EXPORT
void agc_free(agc_t *agc);







SPHINXBASE_EXPORT
void agc_max(agc_t *agc,	
             mfcc_t **mfc,	
             int32 n_frame	
    );






SPHINXBASE_EXPORT
void agc_emax(agc_t *agc,	
              mfcc_t **mfc,	
              int32 n_frame	
    );




SPHINXBASE_EXPORT
void agc_emax_update(agc_t *agc 
    );




SPHINXBASE_EXPORT
float32 agc_emax_get(agc_t *agc);




SPHINXBASE_EXPORT
void agc_emax_set(agc_t *agc, float32 m);




SPHINXBASE_EXPORT
void agc_noise(agc_t *agc,	
               mfcc_t **mfc,	
               int32 n_frame	
    );




SPHINXBASE_EXPORT
float32 agc_get_threshold(agc_t *agc);




SPHINXBASE_EXPORT
void agc_set_threshold(agc_t *agc, float32 threshold);


#ifdef __cplusplus
}
#endif

#endif
