






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif



#include "SigProc_FIX.h"


static const opus_int32 sigm_LUT_slope_Q10[ 6 ] = {
    237, 153, 73, 30, 12, 7
};

static const opus_int32 sigm_LUT_pos_Q15[ 6 ] = {
    16384, 23955, 28861, 31213, 32178, 32548
};

static const opus_int32 sigm_LUT_neg_Q15[ 6 ] = {
    16384, 8812, 3906, 1554, 589, 219
};

opus_int silk_sigm_Q15(
    opus_int                    in_Q5               
)
{
    opus_int ind;

    if( in_Q5 < 0 ) {
        
        in_Q5 = -in_Q5;
        if( in_Q5 >= 6 * 32 ) {
            return 0;        
        } else {
            
            ind = silk_RSHIFT( in_Q5, 5 );
            return( sigm_LUT_neg_Q15[ ind ] - silk_SMULBB( sigm_LUT_slope_Q10[ ind ], in_Q5 & 0x1F ) );
        }
    } else {
        
        if( in_Q5 >= 6 * 32 ) {
            return 32767;        
        } else {
            
            ind = silk_RSHIFT( in_Q5, 5 );
            return( sigm_LUT_pos_Q15[ ind ] + silk_SMULBB( sigm_LUT_slope_Q10[ ind ], in_Q5 & 0x1F ) );
        }
    }
}

