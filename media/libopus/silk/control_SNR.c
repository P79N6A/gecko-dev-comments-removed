






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include "tuning_parameters.h"


opus_int silk_control_SNR(
    silk_encoder_state          *psEncC,                        
    opus_int32                  TargetRate_bps                  
)
{
    opus_int k, ret = SILK_NO_ERROR;
    opus_int32 frac_Q6;
    const opus_int32 *rateTable;

    
    TargetRate_bps = silk_LIMIT( TargetRate_bps, MIN_TARGET_RATE_BPS, MAX_TARGET_RATE_BPS );
    if( TargetRate_bps != psEncC->TargetRate_bps ) {
        psEncC->TargetRate_bps = TargetRate_bps;

        
        if( psEncC->fs_kHz == 8 ) {
            rateTable = silk_TargetRate_table_NB;
        } else if( psEncC->fs_kHz == 12 ) {
            rateTable = silk_TargetRate_table_MB;
        } else {
            rateTable = silk_TargetRate_table_WB;
        }

        
        if( psEncC->nb_subfr == 2 ) {
            TargetRate_bps -= REDUCE_BITRATE_10_MS_BPS;
        }

        
        for( k = 1; k < TARGET_RATE_TAB_SZ; k++ ) {
            if( TargetRate_bps <= rateTable[ k ] ) {
                frac_Q6 = silk_DIV32( silk_LSHIFT( TargetRate_bps - rateTable[ k - 1 ], 6 ),
                                                 rateTable[ k ] - rateTable[ k - 1 ] );
                psEncC->SNR_dB_Q7 = silk_LSHIFT( silk_SNR_table_Q1[ k - 1 ], 6 ) + silk_MUL( frac_Q6, silk_SNR_table_Q1[ k ] - silk_SNR_table_Q1[ k - 1 ] );
                break;
            }
        }

        
        if( psEncC->LBRR_enabled ) {
            psEncC->SNR_dB_Q7 = silk_SMLABB( psEncC->SNR_dB_Q7, 12 - psEncC->LBRR_GainIncreases, SILK_FIX_CONST( -0.25, 7 ) );
        }
    }

    return ret;
}
