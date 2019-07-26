






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"




#define silk_enc_map(a)                  ( silk_RSHIFT( (a), 15 ) + 1 )
#define silk_dec_map(a)                  ( silk_LSHIFT( (a),  1 ) - 1 )


void silk_encode_signs(
    ec_enc                      *psRangeEnc,                        
    const opus_int8             pulses[],                           
    opus_int                    length,                             
    const opus_int              signalType,                         
    const opus_int              quantOffsetType,                    
    const opus_int              sum_pulses[ MAX_NB_SHELL_BLOCKS ]   
)
{
    opus_int         i, j, p;
    opus_uint8       icdf[ 2 ];
    const opus_int8  *q_ptr;
    const opus_uint8 *icdf_ptr;

    icdf[ 1 ] = 0;
    q_ptr = pulses;
    i = silk_SMULBB( 7, silk_ADD_LSHIFT( quantOffsetType, signalType, 1 ) );
    icdf_ptr = &silk_sign_iCDF[ i ];
    length = silk_RSHIFT( length + SHELL_CODEC_FRAME_LENGTH/2, LOG2_SHELL_CODEC_FRAME_LENGTH );
    for( i = 0; i < length; i++ ) {
        p = sum_pulses[ i ];
        if( p > 0 ) {
            icdf[ 0 ] = icdf_ptr[ silk_min( p & 0x1F, 6 ) ];
            for( j = 0; j < SHELL_CODEC_FRAME_LENGTH; j++ ) {
                if( q_ptr[ j ] != 0 ) {
                    ec_enc_icdf( psRangeEnc, silk_enc_map( q_ptr[ j ]), icdf, 8 );
                }
            }
        }
        q_ptr += SHELL_CODEC_FRAME_LENGTH;
    }
}


void silk_decode_signs(
    ec_dec                      *psRangeDec,                        
    opus_int                    pulses[],                           
    opus_int                    length,                             
    const opus_int              signalType,                         
    const opus_int              quantOffsetType,                    
    const opus_int              sum_pulses[ MAX_NB_SHELL_BLOCKS ]   
)
{
    opus_int         i, j, p;
    opus_uint8       icdf[ 2 ];
    opus_int         *q_ptr;
    const opus_uint8 *icdf_ptr;

    icdf[ 1 ] = 0;
    q_ptr = pulses;
    i = silk_SMULBB( 7, silk_ADD_LSHIFT( quantOffsetType, signalType, 1 ) );
    icdf_ptr = &silk_sign_iCDF[ i ];
    length = silk_RSHIFT( length + SHELL_CODEC_FRAME_LENGTH/2, LOG2_SHELL_CODEC_FRAME_LENGTH );
    for( i = 0; i < length; i++ ) {
        p = sum_pulses[ i ];
        if( p > 0 ) {
            icdf[ 0 ] = icdf_ptr[ silk_min( p & 0x1F, 6 ) ];
            for( j = 0; j < SHELL_CODEC_FRAME_LENGTH; j++ ) {
                if( q_ptr[ j ] > 0 ) {
                    
#if 0
                    
                    if( ec_dec_icdf( psRangeDec, icdf, 8 ) == 0 ) {
                        q_ptr[ j ] = -q_ptr[ j ];
                    }
#else
                    
                    q_ptr[ j ] *= silk_dec_map( ec_dec_icdf( psRangeDec, icdf, 8 ) );
#endif
                }
            }
        }
        q_ptr += SHELL_CODEC_FRAME_LENGTH;
    }
}
