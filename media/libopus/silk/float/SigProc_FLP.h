






























#ifndef SILK_SIGPROC_FLP_H
#define SILK_SIGPROC_FLP_H

#include "SigProc_FIX.h"
#include "float_cast.h"
#include <math.h>

#ifdef  __cplusplus
extern "C"
{
#endif






void silk_bwexpander_FLP(
    silk_float          *ar,                
    const opus_int      d,                  
    const silk_float    chirp               
);




silk_float silk_LPC_inverse_pred_gain_FLP(  
    const silk_float    *A,                 
    opus_int32          order               
);

silk_float silk_schur_FLP(                  
    silk_float          refl_coef[],        
    const silk_float    auto_corr[],        
    opus_int            order               
);

void silk_k2a_FLP(
    silk_float          *A,                 
    const silk_float    *rc,                
    opus_int32          order               
);


silk_float silk_levinsondurbin_FLP(         
    silk_float          A[],                
    const silk_float    corr[],             
    const opus_int      order               
);


void silk_autocorrelation_FLP(
    silk_float          *results,           
    const silk_float    *inputData,         
    opus_int            inputDataSize,      
    opus_int            correlationCount    
);

opus_int silk_pitch_analysis_core_FLP(      
    const silk_float    *frame,             
    opus_int            *pitch_out,         
    opus_int16          *lagIndex,          
    opus_int8           *contourIndex,      
    silk_float          *LTPCorr,           
    opus_int            prevLag,            
    const silk_float    search_thres1,      
    const silk_float    search_thres2,      
    const opus_int      Fs_kHz,             
    const opus_int      complexity,         
    const opus_int      nb_subfr            
);

void silk_insertion_sort_decreasing_FLP(
    silk_float          *a,                 
    opus_int            *idx,               
    const opus_int      L,                  
    const opus_int      K                   
);


silk_float silk_burg_modified_FLP(          
    silk_float          A[],                
    const silk_float    x[],                
    const silk_float    minInvGain,         
    const opus_int      subfr_length,       
    const opus_int      nb_subfr,           
    const opus_int      D                   
);


void silk_scale_vector_FLP(
    silk_float          *data1,
    silk_float          gain,
    opus_int            dataSize
);


void silk_scale_copy_vector_FLP(
    silk_float          *data_out,
    const silk_float    *data_in,
    silk_float          gain,
    opus_int            dataSize
);


double silk_inner_product_FLP(
    const silk_float    *data1,
    const silk_float    *data2,
    opus_int            dataSize
);


double silk_energy_FLP(
    const silk_float    *data,
    opus_int            dataSize
);





#define PI              (3.1415926536f)

#define silk_min_float( a, b )                  (((a) < (b)) ? (a) :  (b))
#define silk_max_float( a, b )                  (((a) > (b)) ? (a) :  (b))
#define silk_abs_float( a )                     ((silk_float)fabs(a))

#define silk_LIMIT_float( a, limit1, limit2 )   ((limit1) > (limit2) ? ((a) > (limit1) ? (limit1) : ((a) < (limit2) ? (limit2) : (a))) \
                                                                     : ((a) > (limit2) ? (limit2) : ((a) < (limit1) ? (limit1) : (a))))


static inline silk_float silk_sigmoid( silk_float x )
{
    return (silk_float)(1.0 / (1.0 + exp(-x)));
}


static inline opus_int32 silk_float2int( silk_float x )
{
    return (opus_int32)float2int( x );
}


static inline void silk_float2short_array(
    opus_int16       *out,
    const silk_float *in,
    opus_int32       length
)
{
    opus_int32 k;
    for( k = length - 1; k >= 0; k-- ) {
        out[k] = silk_SAT16( (opus_int32)float2int( in[k] ) );
    }
}


static inline void silk_short2float_array(
    silk_float       *out,
    const opus_int16 *in,
    opus_int32       length
)
{
    opus_int32 k;
    for( k = length - 1; k >= 0; k-- ) {
        out[k] = (silk_float)in[k];
    }
}


static inline silk_float silk_log2( double x )
{
    return ( silk_float )( 3.32192809488736 * log10( x ) );
}

#ifdef  __cplusplus
}
#endif

#endif
