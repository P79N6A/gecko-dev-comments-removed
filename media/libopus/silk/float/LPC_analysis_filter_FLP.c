






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "main_FLP.h"









static inline void silk_LPC_analysis_filter16_FLP(
          silk_float                 r_LPC[],            
    const silk_float                 PredCoef[],         
    const silk_float                 s[],                
    const opus_int                   length              
)
{
    opus_int   ix;
    silk_float LPC_pred;
    const silk_float *s_ptr;

    for( ix = 16; ix < length; ix++ ) {
        s_ptr = &s[ix - 1];

        
        LPC_pred = s_ptr[  0 ]  * PredCoef[ 0 ]  +
                   s_ptr[ -1 ]  * PredCoef[ 1 ]  +
                   s_ptr[ -2 ]  * PredCoef[ 2 ]  +
                   s_ptr[ -3 ]  * PredCoef[ 3 ]  +
                   s_ptr[ -4 ]  * PredCoef[ 4 ]  +
                   s_ptr[ -5 ]  * PredCoef[ 5 ]  +
                   s_ptr[ -6 ]  * PredCoef[ 6 ]  +
                   s_ptr[ -7 ]  * PredCoef[ 7 ]  +
                   s_ptr[ -8 ]  * PredCoef[ 8 ]  +
                   s_ptr[ -9 ]  * PredCoef[ 9 ]  +
                   s_ptr[ -10 ] * PredCoef[ 10 ] +
                   s_ptr[ -11 ] * PredCoef[ 11 ] +
                   s_ptr[ -12 ] * PredCoef[ 12 ] +
                   s_ptr[ -13 ] * PredCoef[ 13 ] +
                   s_ptr[ -14 ] * PredCoef[ 14 ] +
                   s_ptr[ -15 ] * PredCoef[ 15 ];

        
        r_LPC[ix] = s_ptr[ 1 ] - LPC_pred;
    }
}


static inline void silk_LPC_analysis_filter12_FLP(
          silk_float                 r_LPC[],            
    const silk_float                 PredCoef[],         
    const silk_float                 s[],                
    const opus_int                   length              
)
{
    opus_int   ix;
    silk_float LPC_pred;
    const silk_float *s_ptr;

    for( ix = 12; ix < length; ix++ ) {
        s_ptr = &s[ix - 1];

        
        LPC_pred = s_ptr[  0 ]  * PredCoef[ 0 ]  +
                   s_ptr[ -1 ]  * PredCoef[ 1 ]  +
                   s_ptr[ -2 ]  * PredCoef[ 2 ]  +
                   s_ptr[ -3 ]  * PredCoef[ 3 ]  +
                   s_ptr[ -4 ]  * PredCoef[ 4 ]  +
                   s_ptr[ -5 ]  * PredCoef[ 5 ]  +
                   s_ptr[ -6 ]  * PredCoef[ 6 ]  +
                   s_ptr[ -7 ]  * PredCoef[ 7 ]  +
                   s_ptr[ -8 ]  * PredCoef[ 8 ]  +
                   s_ptr[ -9 ]  * PredCoef[ 9 ]  +
                   s_ptr[ -10 ] * PredCoef[ 10 ] +
                   s_ptr[ -11 ] * PredCoef[ 11 ];

        
        r_LPC[ix] = s_ptr[ 1 ] - LPC_pred;
    }
}


static inline void silk_LPC_analysis_filter10_FLP(
          silk_float                 r_LPC[],            
    const silk_float                 PredCoef[],         
    const silk_float                 s[],                
    const opus_int                   length              
)
{
    opus_int   ix;
    silk_float LPC_pred;
    const silk_float *s_ptr;

    for( ix = 10; ix < length; ix++ ) {
        s_ptr = &s[ix - 1];

        
        LPC_pred = s_ptr[  0 ] * PredCoef[ 0 ]  +
                   s_ptr[ -1 ] * PredCoef[ 1 ]  +
                   s_ptr[ -2 ] * PredCoef[ 2 ]  +
                   s_ptr[ -3 ] * PredCoef[ 3 ]  +
                   s_ptr[ -4 ] * PredCoef[ 4 ]  +
                   s_ptr[ -5 ] * PredCoef[ 5 ]  +
                   s_ptr[ -6 ] * PredCoef[ 6 ]  +
                   s_ptr[ -7 ] * PredCoef[ 7 ]  +
                   s_ptr[ -8 ] * PredCoef[ 8 ]  +
                   s_ptr[ -9 ] * PredCoef[ 9 ];

        
        r_LPC[ix] = s_ptr[ 1 ] - LPC_pred;
    }
}


static inline void silk_LPC_analysis_filter8_FLP(
          silk_float                 r_LPC[],            
    const silk_float                 PredCoef[],         
    const silk_float                 s[],                
    const opus_int                   length              
)
{
    opus_int   ix;
    silk_float LPC_pred;
    const silk_float *s_ptr;

    for( ix = 8; ix < length; ix++ ) {
        s_ptr = &s[ix - 1];

        
        LPC_pred = s_ptr[  0 ] * PredCoef[ 0 ]  +
                   s_ptr[ -1 ] * PredCoef[ 1 ]  +
                   s_ptr[ -2 ] * PredCoef[ 2 ]  +
                   s_ptr[ -3 ] * PredCoef[ 3 ]  +
                   s_ptr[ -4 ] * PredCoef[ 4 ]  +
                   s_ptr[ -5 ] * PredCoef[ 5 ]  +
                   s_ptr[ -6 ] * PredCoef[ 6 ]  +
                   s_ptr[ -7 ] * PredCoef[ 7 ];

        
        r_LPC[ix] = s_ptr[ 1 ] - LPC_pred;
    }
}


static inline void silk_LPC_analysis_filter6_FLP(
          silk_float                 r_LPC[],            
    const silk_float                 PredCoef[],         
    const silk_float                 s[],                
    const opus_int                   length              
)
{
    opus_int   ix;
    silk_float LPC_pred;
    const silk_float *s_ptr;

    for( ix = 6; ix < length; ix++ ) {
        s_ptr = &s[ix - 1];

        
        LPC_pred = s_ptr[  0 ] * PredCoef[ 0 ]  +
                   s_ptr[ -1 ] * PredCoef[ 1 ]  +
                   s_ptr[ -2 ] * PredCoef[ 2 ]  +
                   s_ptr[ -3 ] * PredCoef[ 3 ]  +
                   s_ptr[ -4 ] * PredCoef[ 4 ]  +
                   s_ptr[ -5 ] * PredCoef[ 5 ];

        
        r_LPC[ix] = s_ptr[ 1 ] - LPC_pred;
    }
}







void silk_LPC_analysis_filter_FLP(
    silk_float                      r_LPC[],                            
    const silk_float                PredCoef[],                         
    const silk_float                s[],                                
    const opus_int                  length,                             
    const opus_int                  Order                               
)
{
    silk_assert( Order <= length );

    switch( Order ) {
        case 6:
            silk_LPC_analysis_filter6_FLP(  r_LPC, PredCoef, s, length );
        break;

        case 8:
            silk_LPC_analysis_filter8_FLP(  r_LPC, PredCoef, s, length );
        break;

        case 10:
            silk_LPC_analysis_filter10_FLP( r_LPC, PredCoef, s, length );
        break;

        case 12:
            silk_LPC_analysis_filter12_FLP( r_LPC, PredCoef, s, length );
        break;

        case 16:
            silk_LPC_analysis_filter16_FLP( r_LPC, PredCoef, s, length );
        break;

        default:
            silk_assert( 0 );
        break;
    }

    
    silk_memset( r_LPC, 0, Order * sizeof( silk_float ) );
}

