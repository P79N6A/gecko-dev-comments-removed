






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif



















#include "resampler_private.h"


static const opus_int8 delay_matrix_enc[ 5 ][ 3 ] = {

   {  6,  0,  3 },
   {  0,  7,  3 },
   {  0,  1, 10 },
   {  0,  2,  6 },
   { 18, 10, 12 }
};

static const opus_int8 delay_matrix_dec[ 3 ][ 5 ] = {

   {  4,  0,  2,  0,  0 },
   {  0,  9,  4,  7,  4 },
   {  0,  3, 12,  7,  7 }
};


#define rateID(R) ( ( ( ((R)>>12) - ((R)>16000) ) >> ((R)>24000) ) - 1 )

#define USE_silk_resampler_copy                     (0)
#define USE_silk_resampler_private_up2_HQ_wrapper   (1)
#define USE_silk_resampler_private_IIR_FIR          (2)
#define USE_silk_resampler_private_down_FIR         (3)


opus_int silk_resampler_init(
    silk_resampler_state_struct *S,                 
    opus_int32                  Fs_Hz_in,           
    opus_int32                  Fs_Hz_out,          
    opus_int                    forEnc              
)
{
    opus_int up2x;

    
    silk_memset( S, 0, sizeof( silk_resampler_state_struct ) );

    
    if( forEnc ) {
        if( ( Fs_Hz_in  != 8000 && Fs_Hz_in  != 12000 && Fs_Hz_in  != 16000 && Fs_Hz_in  != 24000 && Fs_Hz_in  != 48000 ) ||
            ( Fs_Hz_out != 8000 && Fs_Hz_out != 12000 && Fs_Hz_out != 16000 ) ) {
            silk_assert( 0 );
            return -1;
        }
        S->inputDelay = delay_matrix_enc[ rateID( Fs_Hz_in ) ][ rateID( Fs_Hz_out ) ];
    } else {
        if( ( Fs_Hz_in  != 8000 && Fs_Hz_in  != 12000 && Fs_Hz_in  != 16000 ) ||
            ( Fs_Hz_out != 8000 && Fs_Hz_out != 12000 && Fs_Hz_out != 16000 && Fs_Hz_out != 24000 && Fs_Hz_out != 48000 ) ) {
            silk_assert( 0 );
            return -1;
        }
        S->inputDelay = delay_matrix_dec[ rateID( Fs_Hz_in ) ][ rateID( Fs_Hz_out ) ];
    }

    S->Fs_in_kHz  = silk_DIV32_16( Fs_Hz_in,  1000 );
    S->Fs_out_kHz = silk_DIV32_16( Fs_Hz_out, 1000 );

    
    S->batchSize = S->Fs_in_kHz * RESAMPLER_MAX_BATCH_SIZE_MS;

    
    up2x = 0;
    if( Fs_Hz_out > Fs_Hz_in ) {
        
        if( Fs_Hz_out == silk_MUL( Fs_Hz_in, 2 ) ) {                            
            
            S->resampler_function = USE_silk_resampler_private_up2_HQ_wrapper;
        } else {
            
            S->resampler_function = USE_silk_resampler_private_IIR_FIR;
            up2x = 1;
        }
    } else if ( Fs_Hz_out < Fs_Hz_in ) {
        
         S->resampler_function = USE_silk_resampler_private_down_FIR;
        if( silk_MUL( Fs_Hz_out, 4 ) == silk_MUL( Fs_Hz_in, 3 ) ) {             
            S->FIR_Fracs = 3;
            S->FIR_Order = RESAMPLER_DOWN_ORDER_FIR0;
            S->Coefs = silk_Resampler_3_4_COEFS;
        } else if( silk_MUL( Fs_Hz_out, 3 ) == silk_MUL( Fs_Hz_in, 2 ) ) {      
            S->FIR_Fracs = 2;
            S->FIR_Order = RESAMPLER_DOWN_ORDER_FIR0;
            S->Coefs = silk_Resampler_2_3_COEFS;
        } else if( silk_MUL( Fs_Hz_out, 2 ) == Fs_Hz_in ) {                     
            S->FIR_Fracs = 1;
            S->FIR_Order = RESAMPLER_DOWN_ORDER_FIR1;
            S->Coefs = silk_Resampler_1_2_COEFS;
        } else if( silk_MUL( Fs_Hz_out, 3 ) == Fs_Hz_in ) {                     
            S->FIR_Fracs = 1;
            S->FIR_Order = RESAMPLER_DOWN_ORDER_FIR2;
            S->Coefs = silk_Resampler_1_3_COEFS;
        } else if( silk_MUL( Fs_Hz_out, 4 ) == Fs_Hz_in ) {                     
            S->FIR_Fracs = 1;
            S->FIR_Order = RESAMPLER_DOWN_ORDER_FIR2;
            S->Coefs = silk_Resampler_1_4_COEFS;
        } else if( silk_MUL( Fs_Hz_out, 6 ) == Fs_Hz_in ) {                     
            S->FIR_Fracs = 1;
            S->FIR_Order = RESAMPLER_DOWN_ORDER_FIR2;
            S->Coefs = silk_Resampler_1_6_COEFS;
        } else {
            
            silk_assert( 0 );
            return -1;
        }
    } else {
        
        S->resampler_function = USE_silk_resampler_copy;
    }

    
    S->invRatio_Q16 = silk_LSHIFT32( silk_DIV32( silk_LSHIFT32( Fs_Hz_in, 14 + up2x ), Fs_Hz_out ), 2 );
    
    while( silk_SMULWW( S->invRatio_Q16, Fs_Hz_out ) < silk_LSHIFT32( Fs_Hz_in, up2x ) ) {
        S->invRatio_Q16++;
    }

    return 0;
}



opus_int silk_resampler(
    silk_resampler_state_struct *S,                 
    opus_int16                  out[],              
    const opus_int16            in[],               
    opus_int32                  inLen               
)
{
    opus_int nSamples;

    
    silk_assert( inLen >= S->Fs_in_kHz );
    
    silk_assert( S->inputDelay <= S->Fs_in_kHz );

    nSamples = S->Fs_in_kHz - S->inputDelay;

    
    silk_memcpy( &S->delayBuf[ S->inputDelay ], in, nSamples * sizeof( opus_int16 ) );

    switch( S->resampler_function ) {
        case USE_silk_resampler_private_up2_HQ_wrapper:
            silk_resampler_private_up2_HQ_wrapper( S, out, S->delayBuf, S->Fs_in_kHz );
            silk_resampler_private_up2_HQ_wrapper( S, &out[ S->Fs_out_kHz ], &in[ nSamples ], inLen - S->Fs_in_kHz );
            break;
        case USE_silk_resampler_private_IIR_FIR:
            silk_resampler_private_IIR_FIR( S, out, S->delayBuf, S->Fs_in_kHz );
            silk_resampler_private_IIR_FIR( S, &out[ S->Fs_out_kHz ], &in[ nSamples ], inLen - S->Fs_in_kHz );
            break;
        case USE_silk_resampler_private_down_FIR:
            silk_resampler_private_down_FIR( S, out, S->delayBuf, S->Fs_in_kHz );
            silk_resampler_private_down_FIR( S, &out[ S->Fs_out_kHz ], &in[ nSamples ], inLen - S->Fs_in_kHz );
            break;
        default:
            silk_memcpy( out, S->delayBuf, S->Fs_in_kHz * sizeof( opus_int16 ) );
            silk_memcpy( &out[ S->Fs_out_kHz ], &in[ nSamples ], ( inLen - S->Fs_in_kHz ) * sizeof( opus_int16 ) );
    }

    
    silk_memcpy( S->delayBuf, &in[ inLen - S->inputDelay ], S->inputDelay * sizeof( opus_int16 ) );

    return 0;
}
