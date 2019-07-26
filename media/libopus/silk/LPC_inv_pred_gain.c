






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"

#define QA                          24
#define A_LIMIT                     SILK_FIX_CONST( 0.99975, QA )

#define MUL32_FRAC_Q(a32, b32, Q)   ((opus_int32)(silk_RSHIFT_ROUND64(silk_SMULL(a32, b32), Q)))



static opus_int32 LPC_inverse_pred_gain_QA(                 
    opus_int32           A_QA[ 2 ][ SILK_MAX_ORDER_LPC ],   
    const opus_int       order                              
)
{
    opus_int   k, n, mult2Q;
    opus_int32 invGain_Q30, rc_Q31, rc_mult1_Q30, rc_mult2, tmp_QA;
    opus_int32 *Aold_QA, *Anew_QA;

    Anew_QA = A_QA[ order & 1 ];

    invGain_Q30 = 1 << 30;
    for( k = order - 1; k > 0; k-- ) {
        
        if( ( Anew_QA[ k ] > A_LIMIT ) || ( Anew_QA[ k ] < -A_LIMIT ) ) {
            return 0;
        }

        
        rc_Q31 = -silk_LSHIFT( Anew_QA[ k ], 31 - QA );

        
        rc_mult1_Q30 = ( (opus_int32)1 << 30 ) - silk_SMMUL( rc_Q31, rc_Q31 );
        silk_assert( rc_mult1_Q30 > ( 1 << 15 ) );                   
        silk_assert( rc_mult1_Q30 <= ( 1 << 30 ) );

        
        mult2Q = 32 - silk_CLZ32( silk_abs( rc_mult1_Q30 ) );
        rc_mult2 = silk_INVERSE32_varQ( rc_mult1_Q30, mult2Q + 30 );

        
        
        invGain_Q30 = silk_LSHIFT( silk_SMMUL( invGain_Q30, rc_mult1_Q30 ), 2 );
        silk_assert( invGain_Q30 >= 0           );
        silk_assert( invGain_Q30 <= ( 1 << 30 ) );

        
        Aold_QA = Anew_QA;
        Anew_QA = A_QA[ k & 1 ];

        
        for( n = 0; n < k; n++ ) {
            tmp_QA = Aold_QA[ n ] - MUL32_FRAC_Q( Aold_QA[ k - n - 1 ], rc_Q31, 31 );
            Anew_QA[ n ] = MUL32_FRAC_Q( tmp_QA, rc_mult2 , mult2Q );
        }
    }

    
    if( ( Anew_QA[ 0 ] > A_LIMIT ) || ( Anew_QA[ 0 ] < -A_LIMIT ) ) {
        return 0;
    }

    
    rc_Q31 = -silk_LSHIFT( Anew_QA[ 0 ], 31 - QA );

    
    rc_mult1_Q30 = ( (opus_int32)1 << 30 ) - silk_SMMUL( rc_Q31, rc_Q31 );

    
    
    invGain_Q30 = silk_LSHIFT( silk_SMMUL( invGain_Q30, rc_mult1_Q30 ), 2 );
    silk_assert( invGain_Q30 >= 0     );
    silk_assert( invGain_Q30 <= 1<<30 );

    return invGain_Q30;
}


opus_int32 silk_LPC_inverse_pred_gain(              
    const opus_int16            *A_Q12,             
    const opus_int              order               
)
{
    opus_int   k;
    opus_int32 Atmp_QA[ 2 ][ SILK_MAX_ORDER_LPC ];
    opus_int32 *Anew_QA;
    opus_int32 DC_resp = 0;

    Anew_QA = Atmp_QA[ order & 1 ];

    
    for( k = 0; k < order; k++ ) {
        DC_resp += (opus_int32)A_Q12[ k ];
        Anew_QA[ k ] = silk_LSHIFT32( (opus_int32)A_Q12[ k ], QA - 12 );
    }
    
    if( DC_resp >= 4096 ) {
        return 0;
    }
    return LPC_inverse_pred_gain_QA( Atmp_QA, order );
}

#ifdef FIXED_POINT


opus_int32 silk_LPC_inverse_pred_gain_Q24(          
    const opus_int32            *A_Q24,             
    const opus_int              order               
)
{
    opus_int   k;
    opus_int32 Atmp_QA[ 2 ][ SILK_MAX_ORDER_LPC ];
    opus_int32 *Anew_QA;

    Anew_QA = Atmp_QA[ order & 1 ];

    
    for( k = 0; k < order; k++ ) {
        Anew_QA[ k ] = silk_RSHIFT32( A_Q24[ k ], 24 - QA );
    }

    return LPC_inverse_pred_gain_QA( Atmp_QA, order );
}
#endif
