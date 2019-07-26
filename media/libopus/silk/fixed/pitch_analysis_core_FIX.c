


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif




#include "SigProc_FIX.h"
#include "pitch_est_defines.h"
#include "stack_alloc.h"
#include "debug.h"
#include "pitch.h"

#define SCRATCH_SIZE    22
#define SF_LENGTH_4KHZ  ( PE_SUBFR_LENGTH_MS * 4 )
#define SF_LENGTH_8KHZ  ( PE_SUBFR_LENGTH_MS * 8 )
#define MIN_LAG_4KHZ    ( PE_MIN_LAG_MS * 4 )
#define MIN_LAG_8KHZ    ( PE_MIN_LAG_MS * 8 )
#define MAX_LAG_4KHZ    ( PE_MAX_LAG_MS * 4 )
#define MAX_LAG_8KHZ    ( PE_MAX_LAG_MS * 8 - 1 )
#define CSTRIDE_4KHZ    ( MAX_LAG_4KHZ + 1 - MIN_LAG_4KHZ )
#define CSTRIDE_8KHZ    ( MAX_LAG_8KHZ + 3 - ( MIN_LAG_8KHZ - 2 ) )
#define D_COMP_MIN      ( MIN_LAG_8KHZ - 3 )
#define D_COMP_MAX      ( MAX_LAG_8KHZ + 4 )
#define D_COMP_STRIDE   ( D_COMP_MAX - D_COMP_MIN )

typedef opus_int32 silk_pe_stage3_vals[ PE_NB_STAGE3_LAGS ];




static void silk_P_Ana_calc_corr_st3(
    silk_pe_stage3_vals cross_corr_st3[],              
    const opus_int16  frame[],                         
    opus_int          start_lag,                       
    opus_int          sf_length,                       
    opus_int          nb_subfr,                        
    opus_int          complexity                       
);

static void silk_P_Ana_calc_energy_st3(
    silk_pe_stage3_vals energies_st3[],                
    const opus_int16  frame[],                         
    opus_int          start_lag,                       
    opus_int          sf_length,                       
    opus_int          nb_subfr,                        
    opus_int          complexity                       
);




opus_int silk_pitch_analysis_core(                  
    const opus_int16            *frame,             
    opus_int                    *pitch_out,         
    opus_int16                  *lagIndex,          
    opus_int8                   *contourIndex,      
    opus_int                    *LTPCorr_Q15,       
    opus_int                    prevLag,            
    const opus_int32            search_thres1_Q16,  
    const opus_int              search_thres2_Q13,  
    const opus_int              Fs_kHz,             
    const opus_int              complexity,         
    const opus_int              nb_subfr            
)
{
    VARDECL( opus_int16, frame_8kHz );
    VARDECL( opus_int16, frame_4kHz );
    opus_int32 filt_state[ 6 ];
    const opus_int16 *input_frame_ptr;
    opus_int   i, k, d, j;
    VARDECL( opus_int16, C );
    VARDECL( opus_int32, xcorr32 );
    const opus_int16 *target_ptr, *basis_ptr;
    opus_int32 cross_corr, normalizer, energy, shift, energy_basis, energy_target;
    opus_int   d_srch[ PE_D_SRCH_LENGTH ], Cmax, length_d_srch, length_d_comp;
    VARDECL( opus_int16, d_comp );
    opus_int32 sum, threshold, lag_counter;
    opus_int   CBimax, CBimax_new, CBimax_old, lag, start_lag, end_lag, lag_new;
    opus_int32 CC[ PE_NB_CBKS_STAGE2_EXT ], CCmax, CCmax_b, CCmax_new_b, CCmax_new;
    VARDECL( silk_pe_stage3_vals, energies_st3 );
    VARDECL( silk_pe_stage3_vals, cross_corr_st3 );
    opus_int   frame_length, frame_length_8kHz, frame_length_4kHz;
    opus_int   sf_length;
    opus_int   min_lag;
    opus_int   max_lag;
    opus_int32 contour_bias_Q15, diff;
    opus_int   nb_cbk_search, cbk_size;
    opus_int32 delta_lag_log2_sqr_Q7, lag_log2_Q7, prevLag_log2_Q7, prev_lag_bias_Q13;
    const opus_int8 *Lag_CB_ptr;
    SAVE_STACK;
    
    silk_assert( Fs_kHz == 8 || Fs_kHz == 12 || Fs_kHz == 16 );

    
    silk_assert( complexity >= SILK_PE_MIN_COMPLEX );
    silk_assert( complexity <= SILK_PE_MAX_COMPLEX );

    silk_assert( search_thres1_Q16 >= 0 && search_thres1_Q16 <= (1<<16) );
    silk_assert( search_thres2_Q13 >= 0 && search_thres2_Q13 <= (1<<13) );

    
    frame_length      = ( PE_LTP_MEM_LENGTH_MS + nb_subfr * PE_SUBFR_LENGTH_MS ) * Fs_kHz;
    frame_length_4kHz = ( PE_LTP_MEM_LENGTH_MS + nb_subfr * PE_SUBFR_LENGTH_MS ) * 4;
    frame_length_8kHz = ( PE_LTP_MEM_LENGTH_MS + nb_subfr * PE_SUBFR_LENGTH_MS ) * 8;
    sf_length         = PE_SUBFR_LENGTH_MS * Fs_kHz;
    min_lag           = PE_MIN_LAG_MS * Fs_kHz;
    max_lag           = PE_MAX_LAG_MS * Fs_kHz - 1;

    
    ALLOC( frame_8kHz, frame_length_8kHz, opus_int16 );
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
    ALLOC( frame_4kHz, frame_length_4kHz, opus_int16 );
    silk_resampler_down2( filt_state, frame_4kHz, frame_8kHz, frame_length_8kHz );

    
    for( i = frame_length_4kHz - 1; i > 0; i-- ) {
        frame_4kHz[ i ] = silk_ADD_SAT16( frame_4kHz[ i ], frame_4kHz[ i - 1 ] );
    }

    




    
    silk_sum_sqr_shift( &energy, &shift, frame_4kHz, frame_length_4kHz );
    if( shift > 0 ) {
        shift = silk_RSHIFT( shift, 1 );
        for( i = 0; i < frame_length_4kHz; i++ ) {
            frame_4kHz[ i ] = silk_RSHIFT( frame_4kHz[ i ], shift );
        }
    }

    


    ALLOC( C, nb_subfr * CSTRIDE_8KHZ, opus_int16 );
    ALLOC( xcorr32, MAX_LAG_4KHZ-MIN_LAG_4KHZ+1, opus_int32 );
    silk_memset( C, 0, (nb_subfr >> 1) * CSTRIDE_4KHZ * sizeof( opus_int16 ) );
    target_ptr = &frame_4kHz[ silk_LSHIFT( SF_LENGTH_4KHZ, 2 ) ];
    for( k = 0; k < nb_subfr >> 1; k++ ) {
        
        silk_assert( target_ptr >= frame_4kHz );
        silk_assert( target_ptr + SF_LENGTH_8KHZ <= frame_4kHz + frame_length_4kHz );

        basis_ptr = target_ptr - MIN_LAG_4KHZ;

        
        silk_assert( basis_ptr >= frame_4kHz );
        silk_assert( basis_ptr + SF_LENGTH_8KHZ <= frame_4kHz + frame_length_4kHz );

        celt_pitch_xcorr( target_ptr, target_ptr - MAX_LAG_4KHZ, xcorr32, SF_LENGTH_8KHZ, MAX_LAG_4KHZ - MIN_LAG_4KHZ + 1 );

        
        cross_corr = xcorr32[ MAX_LAG_4KHZ - MIN_LAG_4KHZ ];
        normalizer = silk_inner_prod_aligned( target_ptr, target_ptr, SF_LENGTH_8KHZ );
        normalizer = silk_ADD32( normalizer, silk_inner_prod_aligned( basis_ptr,  basis_ptr, SF_LENGTH_8KHZ ) );
        normalizer = silk_ADD32( normalizer, silk_SMULBB( SF_LENGTH_8KHZ, 4000 ) );

        matrix_ptr( C, k, 0, CSTRIDE_4KHZ ) =
            (opus_int16)silk_DIV32_varQ( cross_corr, normalizer, 13 + 1 );                      

        
        for( d = MIN_LAG_4KHZ + 1; d <= MAX_LAG_4KHZ; d++ ) {
            basis_ptr--;

            
            silk_assert( basis_ptr >= frame_4kHz );
            silk_assert( basis_ptr + SF_LENGTH_8KHZ <= frame_4kHz + frame_length_4kHz );

            cross_corr = xcorr32[ MAX_LAG_4KHZ - d ];

            
            normalizer = silk_ADD32( normalizer,
                silk_SMULBB( basis_ptr[ 0 ], basis_ptr[ 0 ] ) -
                silk_SMULBB( basis_ptr[ SF_LENGTH_8KHZ ], basis_ptr[ SF_LENGTH_8KHZ ] ) );

            matrix_ptr( C, k, d - MIN_LAG_4KHZ, CSTRIDE_4KHZ) =
                (opus_int16)silk_DIV32_varQ( cross_corr, normalizer, 13 + 1 );                  
        }
        
        target_ptr += SF_LENGTH_8KHZ;
    }

    
    if( nb_subfr == PE_MAX_NB_SUBFR ) {
        for( i = MAX_LAG_4KHZ; i >= MIN_LAG_4KHZ; i-- ) {
            sum = (opus_int32)matrix_ptr( C, 0, i - MIN_LAG_4KHZ, CSTRIDE_4KHZ )
                + (opus_int32)matrix_ptr( C, 1, i - MIN_LAG_4KHZ, CSTRIDE_4KHZ );               
            sum = silk_SMLAWB( sum, sum, silk_LSHIFT( -i, 4 ) );                                
            C[ i - MIN_LAG_4KHZ ] = (opus_int16)sum;                                            
        }
    } else {
        
        for( i = MAX_LAG_4KHZ; i >= MIN_LAG_4KHZ; i-- ) {
            sum = silk_LSHIFT( (opus_int32)C[ i - MIN_LAG_4KHZ ], 1 );                          
            sum = silk_SMLAWB( sum, sum, silk_LSHIFT( -i, 4 ) );                                
            C[ i - MIN_LAG_4KHZ ] = (opus_int16)sum;                                            
        }
    }

    
    length_d_srch = silk_ADD_LSHIFT32( 4, complexity, 1 );
    silk_assert( 3 * length_d_srch <= PE_D_SRCH_LENGTH );
    silk_insertion_sort_decreasing_int16( C, d_srch, CSTRIDE_4KHZ,
                                          length_d_srch );

    
    Cmax = (opus_int)C[ 0 ];                                                    
    if( Cmax < SILK_FIX_CONST( 0.2, 14 ) ) {
        silk_memset( pitch_out, 0, nb_subfr * sizeof( opus_int ) );
        *LTPCorr_Q15  = 0;
        *lagIndex     = 0;
        *contourIndex = 0;
        RESTORE_STACK;
        return 1;
    }

    threshold = silk_SMULWB( search_thres1_Q16, Cmax );
    for( i = 0; i < length_d_srch; i++ ) {
        
        if( C[ i ] > threshold ) {
            d_srch[ i ] = silk_LSHIFT( d_srch[ i ] + MIN_LAG_4KHZ, 1 );
        } else {
            length_d_srch = i;
            break;
        }
    }
    silk_assert( length_d_srch > 0 );

    ALLOC( d_comp, D_COMP_STRIDE, opus_int16 );
    for( i = D_COMP_MIN; i < D_COMP_MAX; i++ ) {
        d_comp[ i - D_COMP_MIN ] = 0;
    }
    for( i = 0; i < length_d_srch; i++ ) {
        d_comp[ d_srch[ i ] - D_COMP_MIN ] = 1;
    }

    
    for( i = D_COMP_MAX - 1; i >= MIN_LAG_8KHZ; i-- ) {
        d_comp[ i - D_COMP_MIN ] +=
            d_comp[ i - 1 - D_COMP_MIN ] + d_comp[ i - 2 - D_COMP_MIN ];
    }

    length_d_srch = 0;
    for( i = MIN_LAG_8KHZ; i < MAX_LAG_8KHZ + 1; i++ ) {
        if( d_comp[ i + 1 - D_COMP_MIN ] > 0 ) {
            d_srch[ length_d_srch ] = i;
            length_d_srch++;
        }
    }

    
    for( i = D_COMP_MAX - 1; i >= MIN_LAG_8KHZ; i-- ) {
        d_comp[ i - D_COMP_MIN ] += d_comp[ i - 1 - D_COMP_MIN ]
            + d_comp[ i - 2 - D_COMP_MIN ] + d_comp[ i - 3 - D_COMP_MIN ];
    }

    length_d_comp = 0;
    for( i = MIN_LAG_8KHZ; i < D_COMP_MAX; i++ ) {
        if( d_comp[ i - D_COMP_MIN ] > 0 ) {
            d_comp[ length_d_comp ] = i - 2;
            length_d_comp++;
        }
    }

    



    


    
    silk_sum_sqr_shift( &energy, &shift, frame_8kHz, frame_length_8kHz );
    if( shift > 0 ) {
        shift = silk_RSHIFT( shift, 1 );
        for( i = 0; i < frame_length_8kHz; i++ ) {
            frame_8kHz[ i ] = silk_RSHIFT( frame_8kHz[ i ], shift );
        }
    }

    


    silk_memset( C, 0, nb_subfr * CSTRIDE_8KHZ * sizeof( opus_int16 ) );

    target_ptr = &frame_8kHz[ PE_LTP_MEM_LENGTH_MS * 8 ];
    for( k = 0; k < nb_subfr; k++ ) {

        
        silk_assert( target_ptr >= frame_8kHz );
        silk_assert( target_ptr + SF_LENGTH_8KHZ <= frame_8kHz + frame_length_8kHz );

        energy_target = silk_ADD32( silk_inner_prod_aligned( target_ptr, target_ptr, SF_LENGTH_8KHZ ), 1 );
        for( j = 0; j < length_d_comp; j++ ) {
            d = d_comp[ j ];
            basis_ptr = target_ptr - d;

            
            silk_assert( basis_ptr >= frame_8kHz );
            silk_assert( basis_ptr + SF_LENGTH_8KHZ <= frame_8kHz + frame_length_8kHz );

            cross_corr = silk_inner_prod_aligned( target_ptr, basis_ptr, SF_LENGTH_8KHZ );
            if( cross_corr > 0 ) {
                energy_basis = silk_inner_prod_aligned( basis_ptr, basis_ptr, SF_LENGTH_8KHZ );
                matrix_ptr( C, k, d - ( MIN_LAG_8KHZ - 2 ), CSTRIDE_8KHZ ) =
                    (opus_int16)silk_DIV32_varQ( cross_corr,
                                                 silk_ADD32( energy_target,
                                                             energy_basis ),
                                                 13 + 1 );                                      
            } else {
                matrix_ptr( C, k, d - ( MIN_LAG_8KHZ - 2 ), CSTRIDE_8KHZ ) = 0;
            }
        }
        target_ptr += SF_LENGTH_8KHZ;
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
    silk_assert( search_thres2_Q13 == silk_SAT16( search_thres2_Q13 ) );
    
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
            CC[ j ] = 0;
            for( i = 0; i < nb_subfr; i++ ) {
                opus_int d_subfr;
                
                d_subfr = d + matrix_ptr( Lag_CB_ptr, i, j, cbk_size );
                CC[ j ] = CC[ j ]
                    + (opus_int32)matrix_ptr( C, i,
                                              d_subfr - ( MIN_LAG_8KHZ - 2 ),
                                              CSTRIDE_8KHZ );
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

        
        lag_log2_Q7 = silk_lin2log( d ); 
        silk_assert( lag_log2_Q7 == silk_SAT16( lag_log2_Q7 ) );
        silk_assert( nb_subfr * SILK_FIX_CONST( PE_SHORTLAG_BIAS, 13 ) == silk_SAT16( nb_subfr * SILK_FIX_CONST( PE_SHORTLAG_BIAS, 13 ) ) );
        CCmax_new_b = CCmax_new - silk_RSHIFT( silk_SMULBB( nb_subfr * SILK_FIX_CONST( PE_SHORTLAG_BIAS, 13 ), lag_log2_Q7 ), 7 ); 

        
        silk_assert( nb_subfr * SILK_FIX_CONST( PE_PREVLAG_BIAS, 13 ) == silk_SAT16( nb_subfr * SILK_FIX_CONST( PE_PREVLAG_BIAS, 13 ) ) );
        if( prevLag > 0 ) {
            delta_lag_log2_sqr_Q7 = lag_log2_Q7 - prevLag_log2_Q7;
            silk_assert( delta_lag_log2_sqr_Q7 == silk_SAT16( delta_lag_log2_sqr_Q7 ) );
            delta_lag_log2_sqr_Q7 = silk_RSHIFT( silk_SMULBB( delta_lag_log2_sqr_Q7, delta_lag_log2_sqr_Q7 ), 7 );
            prev_lag_bias_Q13 = silk_RSHIFT( silk_SMULBB( nb_subfr * SILK_FIX_CONST( PE_PREVLAG_BIAS, 13 ), *LTPCorr_Q15 ), 15 ); 
            prev_lag_bias_Q13 = silk_DIV32( silk_MUL( prev_lag_bias_Q13, delta_lag_log2_sqr_Q7 ), delta_lag_log2_sqr_Q7 + SILK_FIX_CONST( 0.5, 7 ) );
            CCmax_new_b -= prev_lag_bias_Q13; 
        }

        if( CCmax_new_b > CCmax_b                                   &&  
            CCmax_new > silk_SMULBB( nb_subfr, search_thres2_Q13 )  &&  
            silk_CB_lags_stage2[ 0 ][ CBimax_new ] <= MIN_LAG_8KHZ      
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
        RESTORE_STACK;
        return 1;
    }

    
    *LTPCorr_Q15 = (opus_int)silk_LSHIFT( silk_DIV32_16( CCmax, nb_subfr ), 2 );
    silk_assert( *LTPCorr_Q15 >= 0 );

    if( Fs_kHz > 8 ) {
        VARDECL( opus_int16, scratch_mem );
        
        
        
        
        silk_sum_sqr_shift( &energy, &shift, frame, frame_length );
        ALLOC( scratch_mem, shift > 0 ? frame_length : 0, opus_int16 );
        if( shift > 0 ) {
            
            shift = silk_RSHIFT( shift, 1 );
            for( i = 0; i < frame_length; i++ ) {
                scratch_mem[ i ] = silk_RSHIFT( frame[ i ], shift );
            }
            input_frame_ptr = scratch_mem;
        } else {
            input_frame_ptr = frame;
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

        CCmax = silk_int32_MIN;
        
        for( k = 0; k < nb_subfr; k++ ) {
            pitch_out[ k ] = lag + 2 * silk_CB_lags_stage2[ k ][ CBimax_old ];
        }

        
        if( nb_subfr == PE_MAX_NB_SUBFR ) {
            nb_cbk_search   = (opus_int)silk_nb_cbk_searchs_stage3[ complexity ];
            cbk_size        = PE_NB_CBKS_STAGE3_MAX;
            Lag_CB_ptr      = &silk_CB_lags_stage3[ 0 ][ 0 ];
        } else {
            nb_cbk_search   = PE_NB_CBKS_STAGE3_10MS;
            cbk_size        = PE_NB_CBKS_STAGE3_10MS;
            Lag_CB_ptr      = &silk_CB_lags_stage3_10_ms[ 0 ][ 0 ];
        }

        
        ALLOC( energies_st3, nb_subfr * nb_cbk_search, silk_pe_stage3_vals );
        ALLOC( cross_corr_st3, nb_subfr * nb_cbk_search, silk_pe_stage3_vals );
        silk_P_Ana_calc_corr_st3(  cross_corr_st3, input_frame_ptr, start_lag, sf_length, nb_subfr, complexity );
        silk_P_Ana_calc_energy_st3( energies_st3, input_frame_ptr, start_lag, sf_length, nb_subfr, complexity );

        lag_counter = 0;
        silk_assert( lag == silk_SAT16( lag ) );
        contour_bias_Q15 = silk_DIV32_16( SILK_FIX_CONST( PE_FLATCONTOUR_BIAS, 15 ), lag );

        target_ptr = &input_frame_ptr[ PE_LTP_MEM_LENGTH_MS * Fs_kHz ];
        energy_target = silk_ADD32( silk_inner_prod_aligned( target_ptr, target_ptr, nb_subfr * sf_length ), 1 );
        for( d = start_lag; d <= end_lag; d++ ) {
            for( j = 0; j < nb_cbk_search; j++ ) {
                cross_corr = 0;
                energy     = energy_target;
                for( k = 0; k < nb_subfr; k++ ) {
                    cross_corr = silk_ADD32( cross_corr,
                        matrix_ptr( cross_corr_st3, k, j,
                                    nb_cbk_search )[ lag_counter ] );
                    energy     = silk_ADD32( energy,
                        matrix_ptr( energies_st3, k, j,
                                    nb_cbk_search )[ lag_counter ] );
                    silk_assert( energy >= 0 );
                }
                if( cross_corr > 0 ) {
                    CCmax_new = silk_DIV32_varQ( cross_corr, energy, 13 + 1 );          
                    
                    diff = silk_int16_MAX - silk_MUL( contour_bias_Q15, j );            
                    silk_assert( diff == silk_SAT16( diff ) );
                    CCmax_new = silk_SMULWB( CCmax_new, diff );                         
                } else {
                    CCmax_new = 0;
                }

                if( CCmax_new > CCmax && ( d + silk_CB_lags_stage3[ 0 ][ j ] ) <= max_lag ) {
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
        
        for( k = 0; k < nb_subfr; k++ ) {
            pitch_out[ k ] = lag + matrix_ptr( Lag_CB_ptr, k, CBimax, cbk_size );
            pitch_out[ k ] = silk_LIMIT( pitch_out[ k ], MIN_LAG_8KHZ, PE_MAX_LAG_MS * 8 );
        }
        *lagIndex = (opus_int16)( lag - MIN_LAG_8KHZ );
        *contourIndex = (opus_int8)CBimax;
    }
    silk_assert( *lagIndex >= 0 );
    
    RESTORE_STACK;
    return 0;
}














static void silk_P_Ana_calc_corr_st3(
    silk_pe_stage3_vals cross_corr_st3[],              
    const opus_int16  frame[],                         
    opus_int          start_lag,                       
    opus_int          sf_length,                       
    opus_int          nb_subfr,                        
    opus_int          complexity                       
)
{
    const opus_int16 *target_ptr;
    opus_int   i, j, k, lag_counter, lag_low, lag_high;
    opus_int   nb_cbk_search, delta, idx, cbk_size;
    VARDECL( opus_int32, scratch_mem );
    VARDECL( opus_int32, xcorr32 );
    const opus_int8 *Lag_range_ptr, *Lag_CB_ptr;
    SAVE_STACK;

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
    ALLOC( scratch_mem, SCRATCH_SIZE, opus_int32 );
    ALLOC( xcorr32, SCRATCH_SIZE, opus_int32 );

    target_ptr = &frame[ silk_LSHIFT( sf_length, 2 ) ]; 
    for( k = 0; k < nb_subfr; k++ ) {
        lag_counter = 0;

        
        lag_low  = matrix_ptr( Lag_range_ptr, k, 0, 2 );
        lag_high = matrix_ptr( Lag_range_ptr, k, 1, 2 );
        silk_assert(lag_high-lag_low+1 <= SCRATCH_SIZE);
        celt_pitch_xcorr( target_ptr, target_ptr - start_lag - lag_high, xcorr32, sf_length, lag_high - lag_low + 1 );
        for( j = lag_low; j <= lag_high; j++ ) {
            silk_assert( lag_counter < SCRATCH_SIZE );
            scratch_mem[ lag_counter ] = xcorr32[ lag_high - j ];
            lag_counter++;
        }

        delta = matrix_ptr( Lag_range_ptr, k, 0, 2 );
        for( i = 0; i < nb_cbk_search; i++ ) {
            
            
            idx = matrix_ptr( Lag_CB_ptr, k, i, cbk_size ) - delta;
            for( j = 0; j < PE_NB_STAGE3_LAGS; j++ ) {
                silk_assert( idx + j < SCRATCH_SIZE );
                silk_assert( idx + j < lag_counter );
                matrix_ptr( cross_corr_st3, k, i, nb_cbk_search )[ j ] =
                    scratch_mem[ idx + j ];
            }
        }
        target_ptr += sf_length;
    }
    RESTORE_STACK;
}





static void silk_P_Ana_calc_energy_st3(
    silk_pe_stage3_vals energies_st3[],                 
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
    VARDECL( opus_int32, scratch_mem );
    const opus_int8 *Lag_range_ptr, *Lag_CB_ptr;
    SAVE_STACK;

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
    ALLOC( scratch_mem, SCRATCH_SIZE, opus_int32 );

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
                matrix_ptr( energies_st3, k, i, nb_cbk_search )[ j ] =
                    scratch_mem[ idx + j ];
                silk_assert(
                    matrix_ptr( energies_st3, k, i, nb_cbk_search )[ j ] >= 0 );
            }
        }
        target_ptr += sf_length;
    }
    RESTORE_STACK;
}
