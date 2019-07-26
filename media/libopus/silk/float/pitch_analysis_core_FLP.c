






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif




#include "SigProc_FLP.h"
#include "SigProc_FIX.h"
#include "pitch_est_defines.h"

#define SCRATCH_SIZE        22
#define eps                 1.192092896e-07f




static void silk_P_Ana_calc_corr_st3(
    silk_float cross_corr_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ], 
    const silk_float    frame[],            
    opus_int            start_lag,          
    opus_int            sf_length,          
    opus_int            nb_subfr,           
    opus_int            complexity          
);

static void silk_P_Ana_calc_energy_st3(
    silk_float energies_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ], 
    const silk_float    frame[],            
    opus_int            start_lag,          
    opus_int            sf_length,          
    opus_int            nb_subfr,           
    opus_int            complexity          
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
)
{
    opus_int   i, k, d, j;
    silk_float frame_8kHz[  PE_MAX_FRAME_LENGTH_MS * 8 ];
    silk_float frame_4kHz[  PE_MAX_FRAME_LENGTH_MS * 4 ];
    opus_int16 frame_8_FIX[ PE_MAX_FRAME_LENGTH_MS * 8 ];
    opus_int16 frame_4_FIX[ PE_MAX_FRAME_LENGTH_MS * 4 ];
    opus_int32 filt_state[ 6 ];
    silk_float threshold, contour_bias;
    silk_float C[ PE_MAX_NB_SUBFR][ (PE_MAX_LAG >> 1) + 5 ];
    silk_float CC[ PE_NB_CBKS_STAGE2_EXT ];
    const silk_float *target_ptr, *basis_ptr;
    double    cross_corr, normalizer, energy, energy_tmp;
    opus_int   d_srch[ PE_D_SRCH_LENGTH ];
    opus_int16 d_comp[ (PE_MAX_LAG >> 1) + 5 ];
    opus_int   length_d_srch, length_d_comp;
    silk_float Cmax, CCmax, CCmax_b, CCmax_new_b, CCmax_new;
    opus_int   CBimax, CBimax_new, lag, start_lag, end_lag, lag_new;
    opus_int   cbk_size;
    silk_float lag_log2, prevLag_log2, delta_lag_log2_sqr;
    silk_float energies_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ];
    silk_float cross_corr_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ];
    opus_int   lag_counter;
    opus_int   frame_length, frame_length_8kHz, frame_length_4kHz;
    opus_int   sf_length, sf_length_8kHz, sf_length_4kHz;
    opus_int   min_lag, min_lag_8kHz, min_lag_4kHz;
    opus_int   max_lag, max_lag_8kHz, max_lag_4kHz;
    opus_int   nb_cbk_search;
    const opus_int8 *Lag_CB_ptr;

    
    silk_assert( Fs_kHz == 8 || Fs_kHz == 12 || Fs_kHz == 16 );

    
    silk_assert( complexity >= SILK_PE_MIN_COMPLEX );
    silk_assert( complexity <= SILK_PE_MAX_COMPLEX );

    silk_assert( search_thres1 >= 0.0f && search_thres1 <= 1.0f );
    silk_assert( search_thres2 >= 0.0f && search_thres2 <= 1.0f );

    
    frame_length      = ( PE_LTP_MEM_LENGTH_MS + nb_subfr * PE_SUBFR_LENGTH_MS ) * Fs_kHz;
    frame_length_4kHz = ( PE_LTP_MEM_LENGTH_MS + nb_subfr * PE_SUBFR_LENGTH_MS ) * 4;
    frame_length_8kHz = ( PE_LTP_MEM_LENGTH_MS + nb_subfr * PE_SUBFR_LENGTH_MS ) * 8;
    sf_length         = PE_SUBFR_LENGTH_MS * Fs_kHz;
    sf_length_4kHz    = PE_SUBFR_LENGTH_MS * 4;
    sf_length_8kHz    = PE_SUBFR_LENGTH_MS * 8;
    min_lag           = PE_MIN_LAG_MS * Fs_kHz;
    min_lag_4kHz      = PE_MIN_LAG_MS * 4;
    min_lag_8kHz      = PE_MIN_LAG_MS * 8;
    max_lag           = PE_MAX_LAG_MS * Fs_kHz - 1;
    max_lag_4kHz      = PE_MAX_LAG_MS * 4;
    max_lag_8kHz      = PE_MAX_LAG_MS * 8 - 1;

    silk_memset(C, 0, sizeof(silk_float) * nb_subfr * ((PE_MAX_LAG >> 1) + 5));

    
    if( Fs_kHz == 16 ) {
        
        opus_int16 frame_16_FIX[ 16 * PE_MAX_FRAME_LENGTH_MS ];
        silk_float2short_array( frame_16_FIX, frame, frame_length );
        silk_memset( filt_state, 0, 2 * sizeof( opus_int32 ) );
        silk_resampler_down2( filt_state, frame_8_FIX, frame_16_FIX, frame_length );
        silk_short2float_array( frame_8kHz, frame_8_FIX, frame_length_8kHz );
    } else if( Fs_kHz == 12 ) {
        
        opus_int16 frame_12_FIX[ 12 * PE_MAX_FRAME_LENGTH_MS ];
        silk_float2short_array( frame_12_FIX, frame, frame_length );
        silk_memset( filt_state, 0, 6 * sizeof( opus_int32 ) );
        silk_resampler_down2_3( filt_state, frame_8_FIX, frame_12_FIX, frame_length );
        silk_short2float_array( frame_8kHz, frame_8_FIX, frame_length_8kHz );
    } else {
        silk_assert( Fs_kHz == 8 );
        silk_float2short_array( frame_8_FIX, frame, frame_length_8kHz );
    }

    
    silk_memset( filt_state, 0, 2 * sizeof( opus_int32 ) );
    silk_resampler_down2( filt_state, frame_4_FIX, frame_8_FIX, frame_length_8kHz );
    silk_short2float_array( frame_4kHz, frame_4_FIX, frame_length_4kHz );

    
    for( i = frame_length_4kHz - 1; i > 0; i-- ) {
        frame_4kHz[ i ] += frame_4kHz[ i - 1 ];
    }

    


    target_ptr = &frame_4kHz[ silk_LSHIFT( sf_length_4kHz, 2 ) ];
    for( k = 0; k < nb_subfr >> 1; k++ ) {
        
        silk_assert( target_ptr >= frame_4kHz );
        silk_assert( target_ptr + sf_length_8kHz <= frame_4kHz + frame_length_4kHz );

        basis_ptr = target_ptr - min_lag_4kHz;

        
        silk_assert( basis_ptr >= frame_4kHz );
        silk_assert( basis_ptr + sf_length_8kHz <= frame_4kHz + frame_length_4kHz );

        
        cross_corr = silk_inner_product_FLP( target_ptr, basis_ptr, sf_length_8kHz );
        normalizer = silk_energy_FLP( basis_ptr, sf_length_8kHz ) + sf_length_8kHz * 4000.0f;

        C[ 0 ][ min_lag_4kHz ] += (silk_float)(cross_corr / sqrt(normalizer));

        
        for(d = min_lag_4kHz + 1; d <= max_lag_4kHz; d++) {
            basis_ptr--;

            
            silk_assert( basis_ptr >= frame_4kHz );
            silk_assert( basis_ptr + sf_length_8kHz <= frame_4kHz + frame_length_4kHz );

            cross_corr = silk_inner_product_FLP(target_ptr, basis_ptr, sf_length_8kHz);

            
            normalizer +=
                basis_ptr[ 0 ] * (double)basis_ptr[ 0 ] -
                basis_ptr[ sf_length_8kHz ] * (double)basis_ptr[ sf_length_8kHz ];
            C[ 0 ][ d ] += (silk_float)(cross_corr / sqrt( normalizer ));
        }
        
        target_ptr += sf_length_8kHz;
    }

    
    for( i = max_lag_4kHz; i >= min_lag_4kHz; i-- ) {
        C[ 0 ][ i ] -= C[ 0 ][ i ] * i / 4096.0f;
    }

    
    length_d_srch = 4 + 2 * complexity;
    silk_assert( 3 * length_d_srch <= PE_D_SRCH_LENGTH );
    silk_insertion_sort_decreasing_FLP( &C[ 0 ][ min_lag_4kHz ], d_srch, max_lag_4kHz - min_lag_4kHz + 1, length_d_srch );

    
    Cmax = C[ 0 ][ min_lag_4kHz ];
    target_ptr = &frame_4kHz[ silk_SMULBB( sf_length_4kHz, nb_subfr ) ];
    energy = 1000.0f;
    for( i = 0; i < silk_LSHIFT( sf_length_4kHz, 2 ); i++ ) {
        energy += target_ptr[i] * (double)target_ptr[i];
    }
    threshold = Cmax * Cmax;
    if( energy / 16.0f > threshold ) {
        silk_memset( pitch_out, 0, nb_subfr * sizeof( opus_int ) );
        *LTPCorr      = 0.0f;
        *lagIndex     = 0;
        *contourIndex = 0;
        return 1;
    }

    threshold = search_thres1 * Cmax;
    for( i = 0; i < length_d_srch; i++ ) {
        
        if( C[ 0 ][ min_lag_4kHz + i ] > threshold ) {
            d_srch[ i ] = silk_LSHIFT( d_srch[ i ] + min_lag_4kHz, 1 );
        } else {
            length_d_srch = i;
            break;
        }
    }
    silk_assert( length_d_srch > 0 );

    for( i = min_lag_8kHz - 5; i < max_lag_8kHz + 5; i++ ) {
        d_comp[ i ] = 0;
    }
    for( i = 0; i < length_d_srch; i++ ) {
        d_comp[ d_srch[ i ] ] = 1;
    }

    
    for( i = max_lag_8kHz + 3; i >= min_lag_8kHz; i-- ) {
        d_comp[ i ] += d_comp[ i - 1 ] + d_comp[ i - 2 ];
    }

    length_d_srch = 0;
    for( i = min_lag_8kHz; i < max_lag_8kHz + 1; i++ ) {
        if( d_comp[ i + 1 ] > 0 ) {
            d_srch[ length_d_srch ] = i;
            length_d_srch++;
        }
    }

    
    for( i = max_lag_8kHz + 3; i >= min_lag_8kHz; i-- ) {
        d_comp[ i ] += d_comp[ i - 1 ] + d_comp[ i - 2 ] + d_comp[ i - 3 ];
    }

    length_d_comp = 0;
    for( i = min_lag_8kHz; i < max_lag_8kHz + 4; i++ ) {
        if( d_comp[ i ] > 0 ) {
            d_comp[ length_d_comp ] = (opus_int16)( i - 2 );
            length_d_comp++;
        }
    }

    


    


    silk_memset( C, 0, PE_MAX_NB_SUBFR*((PE_MAX_LAG >> 1) + 5) * sizeof(silk_float));

    if( Fs_kHz == 8 ) {
        target_ptr = &frame[ PE_LTP_MEM_LENGTH_MS * 8 ];
    } else {
        target_ptr = &frame_8kHz[ PE_LTP_MEM_LENGTH_MS * 8 ];
    }
    for( k = 0; k < nb_subfr; k++ ) {
        energy_tmp = silk_energy_FLP( target_ptr, sf_length_8kHz );
        for( j = 0; j < length_d_comp; j++ ) {
            d = d_comp[ j ];
            basis_ptr = target_ptr - d;
            cross_corr = silk_inner_product_FLP( basis_ptr, target_ptr, sf_length_8kHz );
            energy     = silk_energy_FLP( basis_ptr, sf_length_8kHz );
            if( cross_corr > 0.0f ) {
                C[ k ][ d ] = (silk_float)(cross_corr * cross_corr / (energy * energy_tmp + eps));
            } else {
                C[ k ][ d ] = 0.0f;
            }
        }
        target_ptr += sf_length_8kHz;
    }

    
    

    CCmax   = 0.0f; 
    CCmax_b = -1000.0f;

    CBimax = 0; 
    lag = -1;   

    if( prevLag > 0 ) {
        if( Fs_kHz == 12 ) {
            prevLag = silk_LSHIFT( prevLag, 1 ) / 3;
        } else if( Fs_kHz == 16 ) {
            prevLag = silk_RSHIFT( prevLag, 1 );
        }
        prevLag_log2 = silk_log2((silk_float)prevLag);
    } else {
        prevLag_log2 = 0;
    }

    
    if( nb_subfr == PE_MAX_NB_SUBFR ) {
        cbk_size   = PE_NB_CBKS_STAGE2_EXT;
        Lag_CB_ptr = &silk_CB_lags_stage2[ 0 ][ 0 ];
        if( Fs_kHz == 8 && complexity > SILK_PE_MIN_COMPLEX ) {
            
            nb_cbk_search = PE_NB_CBKS_STAGE2_EXT;
        } else {
            nb_cbk_search = PE_NB_CBKS_STAGE2;
        }
    } else {
        cbk_size       = PE_NB_CBKS_STAGE2_10MS;
        Lag_CB_ptr     = &silk_CB_lags_stage2_10_ms[ 0 ][ 0 ];
        nb_cbk_search  = PE_NB_CBKS_STAGE2_10MS;
    }

    for( k = 0; k < length_d_srch; k++ ) {
        d = d_srch[ k ];
        for( j = 0; j < nb_cbk_search; j++ ) {
            CC[j] = 0.0f;
            for( i = 0; i < nb_subfr; i++ ) {
                
                CC[ j ] += C[ i ][ d + matrix_ptr( Lag_CB_ptr, i, j, cbk_size )];
            }
        }
        
        CCmax_new  = -1000.0f;
        CBimax_new = 0;
        for( i = 0; i < nb_cbk_search; i++ ) {
            if( CC[ i ] > CCmax_new ) {
                CCmax_new = CC[ i ];
                CBimax_new = i;
            }
        }
        CCmax_new = silk_max_float(CCmax_new, 0.0f); 
        CCmax_new_b = CCmax_new;

        
        lag_log2 = silk_log2((silk_float)d);
        CCmax_new_b -= PE_SHORTLAG_BIAS * nb_subfr * lag_log2;

        
        if( prevLag > 0 ) {
            delta_lag_log2_sqr = lag_log2 - prevLag_log2;
            delta_lag_log2_sqr *= delta_lag_log2_sqr;
            CCmax_new_b -= PE_PREVLAG_BIAS * nb_subfr * (*LTPCorr) * delta_lag_log2_sqr / (delta_lag_log2_sqr + 0.5f);
        }

        if( CCmax_new_b > CCmax_b                                   &&  
            CCmax_new > nb_subfr * search_thres2 * search_thres2    &&  
            silk_CB_lags_stage2[ 0 ][ CBimax_new ] <= min_lag_8kHz      
        ) {
            CCmax_b = CCmax_new_b;
            CCmax   = CCmax_new;
            lag     = d;
            CBimax  = CBimax_new;
        }
    }

    if( lag == -1 ) {
        
        silk_memset( pitch_out, 0, PE_MAX_NB_SUBFR * sizeof(opus_int) );
        *LTPCorr      = 0.0f;
        *lagIndex     = 0;
        *contourIndex = 0;
        return 1;
    }

    if( Fs_kHz > 8 ) {
        

        
        silk_assert( lag == silk_SAT16( lag ) );
        if( Fs_kHz == 12 ) {
            lag = silk_RSHIFT_ROUND( silk_SMULBB( lag, 3 ), 1 );
        } else { 
            lag = silk_LSHIFT( lag, 1 );
        }

        lag = silk_LIMIT_int( lag, min_lag, max_lag );
        start_lag = silk_max_int( lag - 2, min_lag );
        end_lag   = silk_min_int( lag + 2, max_lag );
        lag_new   = lag;                                    
        CBimax    = 0;                                      
        silk_assert( CCmax >= 0.0f );
        *LTPCorr = (silk_float)sqrt( CCmax / nb_subfr );    

        CCmax = -1000.0f;

        
        silk_P_Ana_calc_corr_st3( cross_corr_st3, frame, start_lag, sf_length, nb_subfr, complexity );
        silk_P_Ana_calc_energy_st3( energies_st3, frame, start_lag, sf_length, nb_subfr, complexity );

        lag_counter = 0;
        silk_assert( lag == silk_SAT16( lag ) );
        contour_bias = PE_FLATCONTOUR_BIAS / lag;

        
        if( nb_subfr == PE_MAX_NB_SUBFR ) {
            nb_cbk_search = (opus_int)silk_nb_cbk_searchs_stage3[ complexity ];
            cbk_size      = PE_NB_CBKS_STAGE3_MAX;
            Lag_CB_ptr    = &silk_CB_lags_stage3[ 0 ][ 0 ];
        } else {
            nb_cbk_search = PE_NB_CBKS_STAGE3_10MS;
            cbk_size      = PE_NB_CBKS_STAGE3_10MS;
            Lag_CB_ptr    = &silk_CB_lags_stage3_10_ms[ 0 ][ 0 ];
        }

        for( d = start_lag; d <= end_lag; d++ ) {
            for( j = 0; j < nb_cbk_search; j++ ) {
                cross_corr = 0.0;
                energy = eps;
                for( k = 0; k < nb_subfr; k++ ) {
                    energy     +=   energies_st3[ k ][ j ][ lag_counter ];
                    cross_corr += cross_corr_st3[ k ][ j ][ lag_counter ];
                }
                if( cross_corr > 0.0 ) {
                    CCmax_new = (silk_float)(cross_corr * cross_corr / energy);
                    
                    CCmax_new *= 1.0f - contour_bias * j;
                } else {
                    CCmax_new = 0.0f;
                }

                if( CCmax_new > CCmax &&
                   ( d + (opus_int)silk_CB_lags_stage3[ 0 ][ j ] ) <= max_lag
                   ) {
                    CCmax   = CCmax_new;
                    lag_new = d;
                    CBimax  = j;
                }
            }
            lag_counter++;
        }

        for( k = 0; k < nb_subfr; k++ ) {
            pitch_out[ k ] = lag_new + matrix_ptr( Lag_CB_ptr, k, CBimax, cbk_size );
            pitch_out[ k ] = silk_LIMIT( pitch_out[ k ], min_lag, PE_MAX_LAG_MS * Fs_kHz );
        }
        *lagIndex = (opus_int16)( lag_new - min_lag );
        *contourIndex = (opus_int8)CBimax;
    } else {        
        
        silk_assert( CCmax >= 0.0f );
        *LTPCorr = (silk_float)sqrt( CCmax / nb_subfr ); 
        for( k = 0; k < nb_subfr; k++ ) {
            pitch_out[ k ] = lag + matrix_ptr( Lag_CB_ptr, k, CBimax, cbk_size );
            pitch_out[ k ] = silk_LIMIT( pitch_out[ k ], min_lag_8kHz, PE_MAX_LAG_MS * Fs_kHz );
        }
        *lagIndex = (opus_int16)( lag - min_lag_8kHz );
        *contourIndex = (opus_int8)CBimax;
    }
    silk_assert( *lagIndex >= 0 );
    
    return 0;
}

static void silk_P_Ana_calc_corr_st3(
    silk_float cross_corr_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ], 
    const silk_float    frame[],            
    opus_int            start_lag,          
    opus_int            sf_length,          
    opus_int            nb_subfr,           
    opus_int            complexity          
)
    












{
    const silk_float *target_ptr, *basis_ptr;
    opus_int   i, j, k, lag_counter, lag_low, lag_high;
    opus_int   nb_cbk_search, delta, idx, cbk_size;
    silk_float scratch_mem[ SCRATCH_SIZE ];
    const opus_int8 *Lag_range_ptr, *Lag_CB_ptr;

    silk_assert( complexity >= SILK_PE_MIN_COMPLEX );
    silk_assert( complexity <= SILK_PE_MAX_COMPLEX );

    if( nb_subfr == PE_MAX_NB_SUBFR ) {
        Lag_range_ptr = &silk_Lag_range_stage3[ complexity ][ 0 ][ 0 ];
        Lag_CB_ptr    = &silk_CB_lags_stage3[ 0 ][ 0 ];
        nb_cbk_search = silk_nb_cbk_searchs_stage3[ complexity ];
        cbk_size      = PE_NB_CBKS_STAGE3_MAX;
    } else {
        silk_assert( nb_subfr == PE_MAX_NB_SUBFR >> 1);
        Lag_range_ptr = &silk_Lag_range_stage3_10_ms[ 0 ][ 0 ];
        Lag_CB_ptr    = &silk_CB_lags_stage3_10_ms[ 0 ][ 0 ];
        nb_cbk_search = PE_NB_CBKS_STAGE3_10MS;
        cbk_size      = PE_NB_CBKS_STAGE3_10MS;
    }

    target_ptr = &frame[ silk_LSHIFT( sf_length, 2 ) ]; 
    for( k = 0; k < nb_subfr; k++ ) {
        lag_counter = 0;

        
        lag_low  = matrix_ptr( Lag_range_ptr, k, 0, 2 );
        lag_high = matrix_ptr( Lag_range_ptr, k, 1, 2 );
        for( j = lag_low; j <= lag_high; j++ ) {
            basis_ptr = target_ptr - ( start_lag + j );
            silk_assert( lag_counter < SCRATCH_SIZE );
            scratch_mem[ lag_counter ] = (silk_float)silk_inner_product_FLP( target_ptr, basis_ptr, sf_length );
            lag_counter++;
        }

        delta = matrix_ptr( Lag_range_ptr, k, 0, 2 );
        for( i = 0; i < nb_cbk_search; i++ ) {
            
            
            idx = matrix_ptr( Lag_CB_ptr, k, i, cbk_size ) - delta;
            for( j = 0; j < PE_NB_STAGE3_LAGS; j++ ) {
                silk_assert( idx + j < SCRATCH_SIZE );
                silk_assert( idx + j < lag_counter );
                cross_corr_st3[ k ][ i ][ j ] = scratch_mem[ idx + j ];
            }
        }
        target_ptr += sf_length;
    }
}

static void silk_P_Ana_calc_energy_st3(
    silk_float energies_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ], 
    const silk_float    frame[],            
    opus_int            start_lag,          
    opus_int            sf_length,          
    opus_int            nb_subfr,           
    opus_int            complexity          
)




{
    const silk_float *target_ptr, *basis_ptr;
    double    energy;
    opus_int   k, i, j, lag_counter;
    opus_int   nb_cbk_search, delta, idx, cbk_size, lag_diff;
    silk_float scratch_mem[ SCRATCH_SIZE ];
    const opus_int8 *Lag_range_ptr, *Lag_CB_ptr;

    silk_assert( complexity >= SILK_PE_MIN_COMPLEX );
    silk_assert( complexity <= SILK_PE_MAX_COMPLEX );

    if( nb_subfr == PE_MAX_NB_SUBFR ) {
        Lag_range_ptr = &silk_Lag_range_stage3[ complexity ][ 0 ][ 0 ];
        Lag_CB_ptr    = &silk_CB_lags_stage3[ 0 ][ 0 ];
        nb_cbk_search = silk_nb_cbk_searchs_stage3[ complexity ];
        cbk_size      = PE_NB_CBKS_STAGE3_MAX;
    } else {
        silk_assert( nb_subfr == PE_MAX_NB_SUBFR >> 1);
        Lag_range_ptr = &silk_Lag_range_stage3_10_ms[ 0 ][ 0 ];
        Lag_CB_ptr    = &silk_CB_lags_stage3_10_ms[ 0 ][ 0 ];
        nb_cbk_search = PE_NB_CBKS_STAGE3_10MS;
        cbk_size      = PE_NB_CBKS_STAGE3_10MS;
    }

    target_ptr = &frame[ silk_LSHIFT( sf_length, 2 ) ];
    for( k = 0; k < nb_subfr; k++ ) {
        lag_counter = 0;

        
        basis_ptr = target_ptr - ( start_lag + matrix_ptr( Lag_range_ptr, k, 0, 2 ) );
        energy = silk_energy_FLP( basis_ptr, sf_length ) + 1e-3;
        silk_assert( energy >= 0.0 );
        scratch_mem[lag_counter] = (silk_float)energy;
        lag_counter++;

        lag_diff = ( matrix_ptr( Lag_range_ptr, k, 1, 2 ) -  matrix_ptr( Lag_range_ptr, k, 0, 2 ) + 1 );
        for( i = 1; i < lag_diff; i++ ) {
            
            energy -= basis_ptr[sf_length - i] * (double)basis_ptr[sf_length - i];
            silk_assert( energy >= 0.0 );

            
            energy += basis_ptr[ -i ] * (double)basis_ptr[ -i ];
            silk_assert( energy >= 0.0 );
            silk_assert( lag_counter < SCRATCH_SIZE );
            scratch_mem[lag_counter] = (silk_float)energy;
            lag_counter++;
        }

        delta = matrix_ptr( Lag_range_ptr, k, 0, 2 );
        for( i = 0; i < nb_cbk_search; i++ ) {
            
            
            idx = matrix_ptr( Lag_CB_ptr, k, i, cbk_size ) - delta;
            for( j = 0; j < PE_NB_STAGE3_LAGS; j++ ) {
                silk_assert( idx + j < SCRATCH_SIZE );
                silk_assert( idx + j < lag_counter );
                energies_st3[ k ][ i ][ j ] = scratch_mem[ idx + j ];
                silk_assert( energies_st3[ k ][ i ][ j ] >= 0.0f );
            }
        }
        target_ptr += sf_length;
    }
}
