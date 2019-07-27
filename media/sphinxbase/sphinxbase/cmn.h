

















































































#ifndef _S3_CMN_H_
#define _S3_CMN_H_


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/fe.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif












typedef enum cmn_type_e {
    CMN_NONE = 0,
    CMN_CURRENT,
    CMN_PRIOR
} cmn_type_t;


SPHINXBASE_EXPORT
extern const char *cmn_type_str[];


SPHINXBASE_EXPORT
cmn_type_t cmn_type_from_str(const char *str);





typedef struct {
    mfcc_t *cmn_mean;   
    mfcc_t *cmn_var;    
    mfcc_t *sum;        
    int32 nframe;	
    int32 veclen;	
} cmn_t;

SPHINXBASE_EXPORT
cmn_t* cmn_init(int32 veclen);




SPHINXBASE_EXPORT
void cmn (cmn_t *cmn,   
          mfcc_t **mfc,	
	  int32 varnorm,


	  int32 n_frame 
	);

#define CMN_WIN_HWM     800     /* #frames after which window shifted */
#define CMN_WIN         500




SPHINXBASE_EXPORT
void cmn_prior(cmn_t *cmn,        

               mfcc_t **incep,  
	       int32 varnorm,    
	       int32 nfr         
    );




SPHINXBASE_EXPORT
void cmn_prior_update(cmn_t *cmn);




SPHINXBASE_EXPORT
void cmn_prior_set(cmn_t *cmn, mfcc_t const *vec);




SPHINXBASE_EXPORT
void cmn_prior_get(cmn_t *cmn, mfcc_t *vec);


SPHINXBASE_EXPORT
void cmn_free (cmn_t *cmn);

#ifdef __cplusplus
}
#endif

#endif
