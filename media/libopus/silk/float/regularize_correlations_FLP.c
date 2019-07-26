






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"


void silk_regularize_correlations_FLP(
    silk_float                      *XX,                                
    silk_float                      *xx,                                
    const silk_float                noise,                              
    const opus_int                  D                                   
)
{
    opus_int i;

    for( i = 0; i < D; i++ ) {
        matrix_ptr( &XX[ 0 ], i, i, D ) += noise;
    }
    xx[ 0 ] += noise;
}
