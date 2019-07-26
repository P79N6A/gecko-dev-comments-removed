






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif




#include "SigProc_FIX.h"
#include "pitch_est_defines.h"

void silk_decode_pitch(
    opus_int16                  lagIndex,           
    opus_int8                   contourIndex,       
    opus_int                    pitch_lags[],       
    const opus_int              Fs_kHz,             
    const opus_int              nb_subfr            
)
{
    opus_int   lag, k, min_lag, max_lag, cbk_size;
    const opus_int8 *Lag_CB_ptr;

    if( Fs_kHz == 8 ) {
        if( nb_subfr == PE_MAX_NB_SUBFR ) {
            Lag_CB_ptr = &silk_CB_lags_stage2[ 0 ][ 0 ];
            cbk_size   = PE_NB_CBKS_STAGE2_EXT;
        } else {
            silk_assert( nb_subfr == PE_MAX_NB_SUBFR >> 1 );
            Lag_CB_ptr = &silk_CB_lags_stage2_10_ms[ 0 ][ 0 ];
            cbk_size   = PE_NB_CBKS_STAGE2_10MS;
        }
    } else {
        if( nb_subfr == PE_MAX_NB_SUBFR ) {
            Lag_CB_ptr = &silk_CB_lags_stage3[ 0 ][ 0 ];
            cbk_size   = PE_NB_CBKS_STAGE3_MAX;
        } else {
            silk_assert( nb_subfr == PE_MAX_NB_SUBFR >> 1 );
            Lag_CB_ptr = &silk_CB_lags_stage3_10_ms[ 0 ][ 0 ];
            cbk_size   = PE_NB_CBKS_STAGE3_10MS;
        }
    }

    min_lag = silk_SMULBB( PE_MIN_LAG_MS, Fs_kHz );
    max_lag = silk_SMULBB( PE_MAX_LAG_MS, Fs_kHz );
    lag = min_lag + lagIndex;

    for( k = 0; k < nb_subfr; k++ ) {
        pitch_lags[ k ] = lag + matrix_ptr( Lag_CB_ptr, k, contourIndex, cbk_size );
        pitch_lags[ k ] = silk_LIMIT( pitch_lags[ k ], min_lag, max_lag );
    }
}
