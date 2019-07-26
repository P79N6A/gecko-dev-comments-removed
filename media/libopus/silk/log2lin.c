






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"



opus_int32 silk_log2lin(
    const opus_int32            inLog_Q7            
)
{
    opus_int32 out, frac_Q7;

    if( inLog_Q7 < 0 ) {
        return 0;
    }

    out = silk_LSHIFT( 1, silk_RSHIFT( inLog_Q7, 7 ) );
    frac_Q7 = inLog_Q7 & 0x7F;
    if( inLog_Q7 < 2048 ) {
        
        out = silk_ADD_RSHIFT( out, silk_MUL( out, silk_SMLAWB( frac_Q7, silk_SMULBB( frac_Q7, 128 - frac_Q7 ), -174 ) ), 7 );
    } else {
        
        out = silk_MLA( out, silk_RSHIFT( out, 7 ), silk_SMLAWB( frac_Q7, silk_SMULBB( frac_Q7, 128 - frac_Q7 ), -174 ) );
    }
    return out;
}
