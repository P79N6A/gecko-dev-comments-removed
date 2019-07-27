



















































































#ifndef _S3_TMAT_H_
#define _S3_TMAT_H_


#include <stdio.h>


#include <sphinxbase/logmath.h>




#ifdef __cplusplus
extern "C" {
#endif






typedef struct {
    uint8 ***tp;	

    int16 n_tmat;	
    int16 n_state;	

} tmat_t;




tmat_t *tmat_init (char const *tmatfile,
		   logmath_t *lmath,    
		   float64 tpfloor,	
		   int32 breport      
    );
					    




void tmat_dump (tmat_t *tmat,  
		FILE *fp       
    );	






void tmat_free (tmat_t *t 
    );




void tmat_report(tmat_t *t 
    );

#ifdef __cplusplus
}
#endif

#endif
