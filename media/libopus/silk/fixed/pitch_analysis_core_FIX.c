






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif




#include "SigProc_FIX.h"
#include "pitch_est_defines.h"
#include "debug.h"

#define SCRATCH_SIZE    22




void silk_P_Ana_calc_corr_st3(
    opus_int32        cross_corr_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ],
    const opus_int16  frame[],                         
    opus_int          start_lag,                       
    opus_int          sf_length,                       
    opus_int          nb_subfr,                        
    opus_int          complexity                       
);

void silk_P_Ana_calc_energy_st3(
    opus_int32        energies_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ],
    const opus_int16  frame[],                         
    opus_int          start_lag,                       
    opus_int          sf_length,                       
    opus_int          nb_subfr,                        
    opus_int          complexity                       
);

opus_int32 silk_P_Ana_find_scaling(
    const opus_int16  *frame,
    const opus_int    frame_length,
    const opus_int    sum_sqr_len
);




opus_int silk_pitch_analysis_core(                  
    const opus_int16            *frame,             
    opus_int                    *pitch_out,         
    opus_int16                  *lagIndex,          
    opus_int8                   *contourIndex,      
    opus_int                    *LTPCorr_Q15,       
    opus_int                    prevLag,            
    const opus_int32            search_thres1_Q16,  
    const opus_int              search_thres2_Q15,  
    const opus_int              Fs_kHz,             
    const opus_int              complexity,         
    const opus_int              nb_subfr            
)
{
    opus_int16 frame_8kHz[ PE_MAX_FRAME_LENGTH_ST_2 ];
    opus_int16 frame_4kHz[ PE_MAX_FRAME_LENGTH_ST_1 ];
    opus_int32 filt_state[ 6 ];
    opus_int32 scratch_mem[ 3 * PE_MAX_FRAME_LENGTH ];
    opus_int16 *input_frame_ptr;
    opus_int   i, k, d, j;
    opus_int16 C[ PE_MAX_NB_SUBFR ][ ( PE_MAX_LAG >> 1 ) + 5 ];
    const opus_int16 *target_ptr, *basis_ptr;
    opus_int32 cross_corr, normalizer, energy, shift, energy_basis, energy_target;
    opus_int   d_srch[ PE_D_SRCH_LENGTH ], Cmax, length_d_srch, length_d_comp;
    opus_int16 d_comp[ ( PE_MAX_LAG >> 1 ) + 5 ];
    opus_int32 sum, threshold, temp32, lag_counter;
    opus_int   CBimax, CBimax_new, CBimax_old, lag, start_lag, end_lag, lag_new;
    opus_int32 CC[ PE_NB_CBKS_STAGE2_EXT ], CCmax, CCmax_b, CCmax_new_b, CCmax_new;
    opus_int32 energies_st3[  PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ];
    opus_int32 crosscorr_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ];
    opus_int   frame_length, frame_length_8kHz, frame_length_4kHz, max_sum_sq_length;
    opus_int   sf_length, sf_length_8kHz, sf_length_4kHz;
    opus_int   min_lag, min_lag_8kHz, min_lag_4kHz;
    opus_int   max_lag, max_lag_8kHz, max_lag_4kHz;
    opus_int32 contour_bias_Q20, diff, lz, lshift;
    opus_int   nb_cbk_search, cbk_size;
    opus_int32 delta_lag_log2_sqr_Q7, lag_log2_Q7, prevLag_log2_Q7, prev_lag_bias_Q15, corr_thres_Q15;
    const opus_int8 *Lag_CB_ptr;
    
    silk_assert( Fs_kHz == 8 || Fs_kHz == 12 || Fs_kHz == 16 );

    
    silk_assert( complexity >= SILK_PE_MIN_COMPLEX );
    silk_assert( complexity <= SILK_PE_MAX_COMPLEX );

    silk_assert( search_thres1_Q16 >= 0 && search_thres1_Q16 <= (1<<16) );
    silk_assert( search_thres2_Q15 >= 0 && search_thres2_Q15 <= (1<<15) );

    
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

    silk_memset( C, 0, sizeof( opus_int16 ) * nb_subfr * ( ( PE_MAX_LAG >> 1 ) + 5) );

    
    if( Fs_kHz == 16 ) {
        silk_memset( filt_state, 0, 2 * sizeof( opus_int32 ) );
        silk_resampler_down2( filt_state, frame_8kHz, frame, frame_length );
    } else if( Fs_kHz == 12 ) {
        silk_memset( filt_state, 0, 6 * sizeof( opus_int32 ) );
        silk_resampler_down2_3( filt_state, frame_8kHz, frame, frame_length );
    } else {
        silk_assert( Fs_kHz == 8 );
        silk_memcpy( frame_8kHz, frame, frame_length_8kHz * sizeof(opus_int16) );
    }

    
    silk_memset( filt_state, 0, 2 * sizeof( opus_int32 ) );
    silk_resampler_down2( filt_state, frame_4kHz, frame_8kHz, frame_length_8kHz );

    
    for( i = frame_length_4kHz - 1; i > 0; i-- ) {
        frame_4kHz[ i ] = silk_ADD_SAT16( frame_4kHz[ i ], frame_4kHz[ i - 1 ] );
    }

    




    
    max_sum_sq_length = silk_max_32( sf_length_8kHz, silk_LSHIFT( sf_length_4kHz, 2 ) );
    shift = silk_P_Ana_find_scaling( frame_4kHz, frame_length_4kHz, max_sum_sq_length );
    if( shift > 0 ) {
        for( i = 0; i < frame_length_4kHz; i++ ) {
            frame_4kHz[ i ] = silk_RSHIFT( frame_4kHz[ i ], shift );
        }
    }

    


    target_ptr = &frame_4kHz[ silk_LSHIFT( sf_length_4kHz, 2 ) ];
    for( k = 0; k < nb_subfr >> 1; k++ ) {
        
        silk_assert( target_ptr >= frame_4kHz );
        silk_assert( target_ptr + sf_length_8kHz <= frame_4kHz + frame_length_4kHz );

        basis_ptr = target_ptr - min_lag_4kHz;

        
        silk_assert( basis_ptr >= frame_4kHz );
        silk_assert( basis_ptr + sf_length_8kHz <= frame_4kHz + frame_length_4kHz );

        
        cross_corr = silk_inner_prod_aligned( target_ptr, basis_ptr, sf_length_8kHz );
        normalizer = silk_inner_prod_aligned( basis_ptr,  basis_ptr, sf_length_8kHz );
        normalizer = silk_ADD_SAT32( normalizer, silk_SMULBB( sf_length_8kHz, 4000 ) );

        temp32 = silk_DIV32( cross_corr, silk_SQRT_APPROX( normalizer ) + 1 );
        C[ k ][ min_lag_4kHz ] = (opus_int16)silk_SAT16( temp32 );        

        
        for( d = min_lag_4kHz + 1; d <= max_lag_4kHz; d++ ) {
            basis_ptr--;

            
            silk_assert( basis_ptr >= frame_4kHz );
            silk_assert( basis_ptr + sf_length_8kHz <= frame_4kHz + frame_length_4kHz );

            cross_corr = silk_inner_prod_aligned( target_ptr, basis_ptr, sf_length_8kHz );

            
            normalizer +=
                silk_SMULBB( basis_ptr[ 0 ], basis_ptr[ 0 ] ) -
                silk_SMULBB( basis_ptr[ sf_length_8kHz ], basis_ptr[ sf_length_8kHz ] );

            temp32 = silk_DIV32( cross_corr, silk_SQRT_APPROX( normalizer ) + 1 );
            C[ k ][ d ] = (opus_int16)silk_SAT16( temp32 );                        
        }
        
        target_ptr += sf_length_8kHz;
    }

    
    if( nb_subfr == PE_MAX_NB_SUBFR ) {
        for( i = max_lag_4kHz; i >= min_lag_4kHz; i-- ) {
            sum = (opus_int32)C[ 0 ][ i ] + (opus_int32)C[ 1 ][ i ];                
            silk_assert( silk_RSHIFT( sum, 1 ) == silk_SAT16( silk_RSHIFT( sum, 1 ) ) );
            sum = silk_RSHIFT( sum, 1 );                                           
            silk_assert( silk_LSHIFT( (opus_int32)-i, 4 ) == silk_SAT16( silk_LSHIFT( (opus_int32)-i, 4 ) ) );
            sum = silk_SMLAWB( sum, sum, silk_LSHIFT( -i, 4 ) );                    
            silk_assert( sum == silk_SAT16( sum ) );
            C[ 0 ][ i ] = (opus_int16)sum;                                         
        }
    } else {
        
        for( i = max_lag_4kHz; i >= min_lag_4kHz; i-- ) {
            sum = (opus_int32)C[ 0 ][ i ];
            sum = silk_SMLAWB( sum, sum, silk_LSHIFT( -i, 4 ) );                    
            C[ 0 ][ i ] = (opus_int16)sum;                                         
        }
    }

    
    length_d_srch = silk_ADD_LSHIFT32( 4, complexity, 1 );
    silk_assert( 3 * length_d_srch <= PE_D_SRCH_LENGTH );
    silk_insertion_sort_decreasing_int16( &C[ 0 ][ min_lag_4kHz ], d_srch, max_lag_4kHz - min_lag_4kHz + 1, length_d_srch );

    
    target_ptr = &frame_4kHz[ silk_SMULBB( sf_length_4kHz, nb_subfr ) ];
    energy = silk_inner_prod_aligned( target_ptr, target_ptr, silk_LSHIFT( sf_length_4kHz, 2 ) );
    energy = silk_ADD_SAT32( energy, 1000 );                                  
    Cmax = (opus_int)C[ 0 ][ min_lag_4kHz ];                                  
    threshold = silk_SMULBB( Cmax, Cmax );                                    

    
    if( silk_RSHIFT( energy, 4 + 2 ) > threshold ) {
        silk_memset( pitch_out, 0, nb_subfr * sizeof( opus_int ) );
        *LTPCorr_Q15  = 0;
        *lagIndex     = 0;
        *contourIndex = 0;
        return 1;
    }

    threshold = silk_SMULWB( search_thres1_Q16, Cmax );
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
            d_comp[ length_d_comp ] = i - 2;
            length_d_comp++;
        }
    }

    



    


    
    shift = silk_P_Ana_find_scaling( frame_8kHz, frame_length_8kHz, sf_length_8kHz );
    if( shift > 0 ) {
        for( i = 0; i < frame_length_8kHz; i++ ) {
            frame_8kHz[ i ] = silk_RSHIFT( frame_8kHz[ i ], shift );
        }
    }

    


    silk_memset( C, 0, PE_MAX_NB_SUBFR * ( ( PE_MAX_LAG >> 1 ) + 5 ) * sizeof( opus_int16 ) );

    target_ptr = &frame_8kHz[ PE_LTP_MEM_LENGTH_MS * 8 ];
    for( k = 0; k < nb_subfr; k++ ) {

        
        silk_assert( target_ptr >= frame_8kHz );
        silk_assert( target_ptr + sf_length_8kHz <= frame_8kHz + frame_length_8kHz );

        energy_target = silk_inner_prod_aligned( target_ptr, target_ptr, sf_length_8kHz );
        for( j = 0; j < length_d_comp; j++ ) {
            d = d_comp[ j ];
            basis_ptr = target_ptr - d;

            
            silk_assert( basis_ptr >= frame_8kHz );
            silk_assert( basis_ptr + sf_length_8kHz <= frame_8kHz + frame_length_8kHz );

            cross_corr   = silk_inner_prod_aligned( target_ptr, basis_ptr, sf_length_8kHz );
            energy_basis = silk_inner_prod_aligned( basis_ptr,  basis_ptr, sf_length_8kHz );
            if( cross_corr > 0 ) {
                energy = silk_max( energy_target, energy_basis ); 
                lz = silk_CLZ32( cross_corr );
                lshift = silk_LIMIT_32( lz - 1, 0, 15 );
                temp32 = silk_DIV32( silk_LSHIFT( cross_corr, lshift ), silk_RSHIFT( energy, 15 - lshift ) + 1 ); 
                silk_assert( temp32 == silk_SAT16( temp32 ) );
                temp32 = silk_SMULWB( cross_corr, temp32 ); 
                temp32 = silk_ADD_SAT32( temp32, temp32 );  
                lz = silk_CLZ32( temp32 );
                lshift = silk_LIMIT_32( lz - 1, 0, 15 );
                energy = silk_min( energy_target, energy_basis );
                C[ k ][ d ] = silk_DIV32( silk_LSHIFT( temp32, lshift ), silk_RSHIFT( energy, 15 - lshift ) + 1 ); 
            } else {
                C[ k ][ d ] = 0;
            }
        }
        target_ptr += sf_length_8kHz;
    }

    
    

    CCmax   = silk_int32_MIN;
    CCmax_b = silk_int32_MIN;

    CBimax = 0; 
    lag = -1;   

    if( prevLag > 0 ) {
        if( Fs_kHz == 12 ) {
            prevLag = silk_DIV32_16( silk_LSHIFT( prevLag, 1 ), 3 );
        } else if( Fs_kHz == 16 ) {
            prevLag = silk_RSHIFT( prevLag, 1 );
        }
        prevLag_log2_Q7 = silk_lin2log( (opus_int32)prevLag );
    } else {
        prevLag_log2_Q7 = 0;
    }
    silk_assert( search_thres2_Q15 == silk_SAT16( search_thres2_Q15 ) );
    
    if( nb_subfr == PE_MAX_NB_SUBFR ) {
        cbk_size   = PE_NB_CBKS_STAGE2_EXT;
        Lag_CB_ptr = &silk_CB_lags_stage2[ 0 ][ 0 ];
        if( Fs_kHz == 8 && complexity > SILK_PE_MIN_COMPLEX ) {
            
            nb_cbk_search = PE_NB_CBKS_STAGE2_EXT;
        } else {
            nb_cbk_search = PE_NB_CBKS_STAGE2;
        }
        corr_thres_Q15 = silk_RSHIFT( silk_SMULBB( search_thres2_Q15, search_thres2_Q15 ), 13 );
    } else {
        cbk_size       = PE_NB_CBKS_STAGE2_10MS;
        Lag_CB_ptr     = &silk_CB_lags_stage2_10_ms[ 0 ][ 0 ];
        nb_cbk_search  = PE_NB_CBKS_STAGE2_10MS;
        corr_thres_Q15 = silk_RSHIFT( silk_SMULBB( search_thres2_Q15, search_thres2_Q15 ), 14 );
    }

    for( k = 0; k < length_d_srch; k++ ) {
        d = d_srch[ k ];
        for( j = 0; j < nb_cbk_search; j++ ) {
            CC[ j ] = 0;
            for( i = 0; i < nb_subfr; i++ ) {
                
                CC[ j ] = CC[ j ] + (opus_int32)C[ i ][ d + matrix_ptr( Lag_CB_ptr, i, j, cbk_size )];
            }
        }
        
        CCmax_new = silk_int32_MIN;
        CBimax_new = 0;
        for( i = 0; i < nb_cbk_search; i++ ) {
            if( CC[ i ] > CCmax_new ) {
                CCmax_new = CC[ i ];
                CBimax_new = i;
            }
        }

        
        lag_log2_Q7 = silk_lin2log( (opus_int32)d ); 
        silk_assert( lag_log2_Q7 == silk_SAT16( lag_log2_Q7 ) );
        silk_assert( nb_subfr * SILK_FIX_CONST( PE_SHORTLAG_BIAS, 15 ) == silk_SAT16( nb_subfr * SILK_FIX_CONST( PE_SHORTLAG_BIAS, 15 ) ) );
        CCmax_new_b = CCmax_new - silk_RSHIFT( silk_SMULBB( nb_subfr * SILK_FIX_CONST( PE_SHORTLAG_BIAS, 15 ), lag_log2_Q7 ), 7 ); 

        
        silk_assert( nb_subfr * SILK_FIX_CONST( PE_PREVLAG_BIAS, 15 ) == silk_SAT16( nb_subfr * SILK_FIX_CONST( PE_PREVLAG_BIAS, 15 ) ) );
        if( prevLag > 0 ) {
            delta_lag_log2_sqr_Q7 = lag_log2_Q7 - prevLag_log2_Q7;
            silk_assert( delta_lag_log2_sqr_Q7 == silk_SAT16( delta_lag_log2_sqr_Q7 ) );
            delta_lag_log2_sqr_Q7 = silk_RSHIFT( silk_SMULBB( delta_lag_log2_sqr_Q7, delta_lag_log2_sqr_Q7 ), 7 );
            prev_lag_bias_Q15 = silk_RSHIFT( silk_SMULBB( nb_subfr * SILK_FIX_CONST( PE_PREVLAG_BIAS, 15 ), *LTPCorr_Q15 ), 15 ); 
            prev_lag_bias_Q15 = silk_DIV32( silk_MUL( prev_lag_bias_Q15, delta_lag_log2_sqr_Q7 ), delta_lag_log2_sqr_Q7 + ( 1 << 6 ) );
            CCmax_new_b -= prev_lag_bias_Q15; 
        }

        if( CCmax_new_b > CCmax_b                                   &&  
            CCmax_new > corr_thres_Q15                              &&  
            silk_CB_lags_stage2[ 0 ][ CBimax_new ] <= min_lag_8kHz      
         ) {
            CCmax_b = CCmax_new_b;
            CCmax   = CCmax_new;
            lag     = d;
            CBimax  = CBimax_new;
        }
    }

    if( lag == -1 ) {
        
        silk_memset( pitch_out, 0, nb_subfr * sizeof( opus_int ) );
        *LTPCorr_Q15  = 0;
        *lagIndex     = 0;
        *contourIndex = 0;
        return 1;
    }

    if( Fs_kHz > 8 ) {
        
        
        
        
        shift = silk_P_Ana_find_scaling( frame, frame_length, sf_length );
        if( shift > 0 ) {
            
            
            input_frame_ptr = (opus_int16*)scratch_mem;
            for( i = 0; i < frame_length; i++ ) {
                input_frame_ptr[ i ] = silk_RSHIFT( frame[ i ], shift );
            }
        } else {
            input_frame_ptr = (opus_int16*)frame;
        }

        

        CBimax_old = CBimax;
        
        silk_assert( lag == silk_SAT16( lag ) );
        if( Fs_kHz == 12 ) {
            lag = silk_RSHIFT( silk_SMULBB( lag, 3 ), 1 );
        } else if( Fs_kHz == 16 ) {
            lag = silk_LSHIFT( lag, 1 );
        } else {
            lag = silk_SMULBB( lag, 3 );
        }

        lag = silk_LIMIT_int( lag, min_lag, max_lag );
        start_lag = silk_max_int( lag - 2, min_lag );
        end_lag   = silk_min_int( lag + 2, max_lag );
        lag_new   = lag;                                    
        CBimax    = 0;                                        
        silk_assert( silk_LSHIFT( CCmax, 13 ) >= 0 );
        *LTPCorr_Q15 = (opus_int)silk_SQRT_APPROX( silk_LSHIFT( CCmax, 13 ) ); 

        CCmax = silk_int32_MIN;
        
        for( k = 0; k < nb_subfr; k++ ) {
            pitch_out[ k ] = lag + 2 * silk_CB_lags_stage2[ k ][ CBimax_old ];
        }
        
        silk_P_Ana_calc_corr_st3(  crosscorr_st3, input_frame_ptr, start_lag, sf_length, nb_subfr, complexity );
        silk_P_Ana_calc_energy_st3( energies_st3, input_frame_ptr, start_lag, sf_length, nb_subfr, complexity );

        lag_counter = 0;
        silk_assert( lag == silk_SAT16( lag ) );
        contour_bias_Q20 = silk_DIV32_16( SILK_FIX_CONST( PE_FLATCONTOUR_BIAS, 20 ), lag );

        
        if( nb_subfr == PE_MAX_NB_SUBFR ) {
            nb_cbk_search   = (opus_int)silk_nb_cbk_searchs_stage3[ complexity ];
            cbk_size        = PE_NB_CBKS_STAGE3_MAX;
            Lag_CB_ptr      = &silk_CB_lags_stage3[ 0 ][ 0 ];
        } else {
            nb_cbk_search   = PE_NB_CBKS_STAGE3_10MS;
            cbk_size        = PE_NB_CBKS_STAGE3_10MS;
            Lag_CB_ptr      = &silk_CB_lags_stage3_10_ms[ 0 ][ 0 ];
        }
        for( d = start_lag; d <= end_lag; d++ ) {
            for( j = 0; j < nb_cbk_search; j++ ) {
                cross_corr = 0;
                energy     = 0;
                for( k = 0; k < nb_subfr; k++ ) {
                    silk_assert( PE_MAX_NB_SUBFR == 4 );
                    energy     += silk_RSHIFT( energies_st3[  k ][ j ][ lag_counter ], 2 ); 
                    silk_assert( energy >= 0 );
                    cross_corr += silk_RSHIFT( crosscorr_st3[ k ][ j ][ lag_counter ], 2 ); 
                }
                if( cross_corr > 0 ) {
                    
                    lz = silk_CLZ32( cross_corr );
                    
                    lshift = silk_LIMIT_32( lz - 1, 0, 13 );
                    CCmax_new = silk_DIV32( silk_LSHIFT( cross_corr, lshift ), silk_RSHIFT( energy, 13 - lshift ) + 1 );
                    CCmax_new = silk_SAT16( CCmax_new );
                    CCmax_new = silk_SMULWB( cross_corr, CCmax_new );
                    
                    if( CCmax_new > silk_RSHIFT( silk_int32_MAX, 3 ) ) {
                        CCmax_new = silk_int32_MAX;
                    } else {
                        CCmax_new = silk_LSHIFT( CCmax_new, 3 );
                    }
                    
                    diff = silk_int16_MAX - silk_RSHIFT( silk_MUL( contour_bias_Q20, j ), 5 ); 
                    silk_assert( diff == silk_SAT16( diff ) );
                    CCmax_new = silk_LSHIFT( silk_SMULWB( CCmax_new, diff ), 1 );
                } else {
                    CCmax_new = 0;
                }

                if( CCmax_new > CCmax                                               &&
                   ( d + silk_CB_lags_stage3[ 0 ][ j ] ) <= max_lag
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
        *lagIndex = (opus_int16)( lag_new - min_lag);
        *contourIndex = (opus_int8)CBimax;
    } else {        
        
        CCmax = silk_max( CCmax, 0 );
        *LTPCorr_Q15 = (opus_int)silk_SQRT_APPROX( silk_LSHIFT( CCmax, 13 ) ); 
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





void silk_P_Ana_calc_corr_st3(
    opus_int32        cross_corr_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ],
    const opus_int16  frame[],                         
    opus_int          start_lag,                       
    opus_int          sf_length,                       
    opus_int          nb_subfr,                        
    opus_int          complexity                       
)
{
    const opus_int16 *target_ptr, *basis_ptr;
    opus_int32 cross_corr;
    opus_int   i, j, k, lag_counter, lag_low, lag_high;
    opus_int   nb_cbk_search, delta, idx, cbk_size;
    opus_int32 scratch_mem[ SCRATCH_SIZE ];
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
            cross_corr = silk_inner_prod_aligned( (opus_int16*)target_ptr, (opus_int16*)basis_ptr, sf_length );
            silk_assert( lag_counter < SCRATCH_SIZE );
            scratch_mem[ lag_counter ] = cross_corr;
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





void silk_P_Ana_calc_energy_st3(
    opus_int32        energies_st3[ PE_MAX_NB_SUBFR ][ PE_NB_CBKS_STAGE3_MAX ][ PE_NB_STAGE3_LAGS ],
    const opus_int16  frame[],                         
    opus_int          start_lag,                       
    opus_int          sf_length,                       
    opus_int          nb_subfr,                     
    opus_int          complexity                       
)
{
    const opus_int16 *target_ptr, *basis_ptr;
    opus_int32 energy;
    opus_int   k, i, j, lag_counter;
    opus_int   nb_cbk_search, delta, idx, cbk_size, lag_diff;
    opus_int32 scratch_mem[ SCRATCH_SIZE ];
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
        energy = silk_inner_prod_aligned( basis_ptr, basis_ptr, sf_length );
        silk_assert( energy >= 0 );
        scratch_mem[ lag_counter ] = energy;
        lag_counter++;

        lag_diff = ( matrix_ptr( Lag_range_ptr, k, 1, 2 ) -  matrix_ptr( Lag_range_ptr, k, 0, 2 ) + 1 );
        for( i = 1; i < lag_diff; i++ ) {
            
            energy -= silk_SMULBB( basis_ptr[ sf_length - i ], basis_ptr[ sf_length - i ] );
            silk_assert( energy >= 0 );

            
            energy = silk_ADD_SAT32( energy, silk_SMULBB( basis_ptr[ -i ], basis_ptr[ -i ] ) );
            silk_assert( energy >= 0 );
            silk_assert( lag_counter < SCRATCH_SIZE );
            scratch_mem[ lag_counter ] = energy;
            lag_counter++;
        }

        delta = matrix_ptr( Lag_range_ptr, k, 0, 2 );
        for( i = 0; i < nb_cbk_search; i++ ) {
            
            
            idx = matrix_ptr( Lag_CB_ptr, k, i, cbk_size ) - delta;
            for( j = 0; j < PE_NB_STAGE3_LAGS; j++ ) {
                silk_assert( idx + j < SCRATCH_SIZE );
                silk_assert( idx + j < lag_counter );
                energies_st3[ k ][ i ][ j ] = scratch_mem[ idx + j ];
                silk_assert( energies_st3[ k ][ i ][ j ] >= 0 );
            }
        }
        target_ptr += sf_length;
    }
}

opus_int32 silk_P_Ana_find_scaling(
    const opus_int16  *frame,
    const opus_int    frame_length,
    const opus_int    sum_sqr_len
)
{
    opus_int32 nbits, x_max;

    x_max = silk_int16_array_maxabs( frame, frame_length );

    if( x_max < silk_int16_MAX ) {
        
        nbits = 32 - silk_CLZ32( silk_SMULBB( x_max, x_max ) );
    } else {
        
        nbits = 30;
    }
    nbits += 17 - silk_CLZ16( sum_sqr_len );

    
    if( nbits < 31 ) {
        return 0;
    } else {
        return( nbits - 30 );
    }
}
