











































































































#ifndef _LIBFBS_SENONE_H_
#define _LIBFBS_SENONE_H_



#include <sphinxbase/err.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/logmath.h>


#include "ms_gauden.h"
#include "bin_mdef.h"









#ifdef __cplusplus
extern "C" {
#endif

typedef uint8 senprob_t;	








typedef struct {
    senprob_t ***pdf;		






    logmath_t *lmath;           
    uint32 n_sen;		
    uint32 n_feat;		 
    uint32 n_cw;		
    uint32 n_gauden;		
    float32 mixwfloor;		
    uint32 *mgau;		
    int32 *featscr;              
    int32 aw;			
} senone_t;








senone_t *senone_init (gauden_t *g,             
                       char const *mixwfile,	
		       char const *mgau_mapfile,


		       float32 mixwfloor,	
                       logmath_t *lmath,        
                       bin_mdef_t *mdef         
    );


void senone_free(senone_t *s); 





int32 senone_eval (senone_t *s, int id,		
		   gauden_dist_t **dist,	



		   int n_top		
    );

#ifdef __cplusplus
}
#endif

#endif
