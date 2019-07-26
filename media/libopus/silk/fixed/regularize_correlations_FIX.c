






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"


void silk_regularize_correlations_FIX(
    opus_int32                      *XX,                                    
    opus_int32                      *xx,                                    
    opus_int32                      noise,                                  
    opus_int                        D                                       
)
{
    opus_int i;
    for( i = 0; i < D; i++ ) {
        matrix_ptr( &XX[ 0 ], i, i, D ) = silk_ADD32( matrix_ptr( &XX[ 0 ], i, i, D ), noise );
    }
    xx[ 0 ] += noise;
}
