






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"











static opus_int16 freq_table_Q16[ 27 ] = {
   12111,    9804,    8235,    7100,    6239,    5565,    5022,    4575,    4202,
    3885,    3612,    3375,    3167,    2984,    2820,    2674,    2542,    2422,
    2313,    2214,    2123,    2038,    1961,    1889,    1822,    1760,    1702,
};

void silk_apply_sine_window(
    opus_int16                  px_win[],           
    const opus_int16            px[],               
    const opus_int              win_type,           
    const opus_int              length              
)
{
    opus_int   k, f_Q16, c_Q16;
    opus_int32 S0_Q16, S1_Q16;

    silk_assert( win_type == 1 || win_type == 2 );

    
    silk_assert( length >= 16 && length <= 120 );
    silk_assert( ( length & 3 ) == 0 );

    
    k = ( length >> 2 ) - 4;
    silk_assert( k >= 0 && k <= 26 );
    f_Q16 = (opus_int)freq_table_Q16[ k ];

    
    c_Q16 = silk_SMULWB( f_Q16, -f_Q16 );
    silk_assert( c_Q16 >= -32768 );

    
    if( win_type == 1 ) {
        
        S0_Q16 = 0;
        
        S1_Q16 = f_Q16 + silk_RSHIFT( length, 3 );
    } else {
        
        S0_Q16 = ( 1 << 16 );
        
        S1_Q16 = ( 1 << 16 ) + silk_RSHIFT( c_Q16, 1 ) + silk_RSHIFT( length, 4 );
    }

    
    
    for( k = 0; k < length; k += 4 ) {
        px_win[ k ]     = (opus_int16)silk_SMULWB( silk_RSHIFT( S0_Q16 + S1_Q16, 1 ), px[ k ] );
        px_win[ k + 1 ] = (opus_int16)silk_SMULWB( S1_Q16, px[ k + 1] );
        S0_Q16 = silk_SMULWB( S1_Q16, c_Q16 ) + silk_LSHIFT( S1_Q16, 1 ) - S0_Q16 + 1;
        S0_Q16 = silk_min( S0_Q16, ( 1 << 16 ) );

        px_win[ k + 2 ] = (opus_int16)silk_SMULWB( silk_RSHIFT( S0_Q16 + S1_Q16, 1 ), px[ k + 2] );
        px_win[ k + 3 ] = (opus_int16)silk_SMULWB( S0_Q16, px[ k + 3 ] );
        S1_Q16 = silk_SMULWB( S0_Q16, c_Q16 ) + silk_LSHIFT( S0_Q16, 1 ) - S1_Q16;
        S1_Q16 = silk_min( S1_Q16, ( 1 << 16 ) );
    }
}
