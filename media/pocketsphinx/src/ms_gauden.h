




































#ifndef _LIBFBS_GAUDEN_H_
#define _LIBFBS_GAUDEN_H_














#include <sphinxbase/feat.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/cmd_ln.h>


#include "vector.h"
#include "pocketsphinx_internal.h"
#include "hmm.h"

#ifdef __cplusplus
extern "C" {
#endif





typedef struct {
    int32 id;		
    mfcc_t dist;		


} gauden_dist_t;





typedef struct {
    mfcc_t ****mean;	
    mfcc_t ****var;	
    mfcc_t ***det;	

    logmath_t *lmath;   
    int32 n_mgau;	
    int32 n_feat;	
    int32 n_density;	
    int32 *featlen;	
} gauden_t;








gauden_t *
gauden_init (char const *meanfile,
	     char const *varfile,
	     float32 varfloor,	
             logmath_t *lmath
    );


void gauden_free(gauden_t *g); 


int32 gauden_mllr_transform(gauden_t *s, ps_mllr_t *mllr, cmd_ln_t *config);







int32
gauden_dist (gauden_t *g,	
	     int mgau,		

	     int n_top,		
	     mfcc_t **obs,	
	     gauden_dist_t **out_dist
	     



    );




void gauden_dump (const gauden_t *g  
    );




void gauden_dump_ind (const gauden_t *g,  
		      int senidx          
    );

#ifdef __cplusplus
}
#endif

#endif 
