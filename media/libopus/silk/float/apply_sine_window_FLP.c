






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"





void silk_apply_sine_window_FLP(
    silk_float                      px_win[],                           
    const silk_float                px[],                               
    const opus_int                  win_type,                           
    const opus_int                  length                              
)
{
    opus_int   k;
    silk_float freq, c, S0, S1;

    silk_assert( win_type == 1 || win_type == 2 );

    
    silk_assert( ( length & 3 ) == 0 );

    freq = PI / ( length + 1 );

    
    c = 2.0f - freq * freq;

    
    if( win_type < 2 ) {
        
        S0 = 0.0f;
        
        S1 = freq;
    } else {
        
        S0 = 1.0f;
        
        S1 = 0.5f * c;
    }

    
    
    for( k = 0; k < length; k += 4 ) {
        px_win[ k + 0 ] = px[ k + 0 ] * 0.5f * ( S0 + S1 );
        px_win[ k + 1 ] = px[ k + 1 ] * S1;
        S0 = c * S1 - S0;
        px_win[ k + 2 ] = px[ k + 2 ] * 0.5f * ( S1 + S0 );
        px_win[ k + 3 ] = px[ k + 3 ] * S0;
        S1 = c * S0 - S1;
    }
}
