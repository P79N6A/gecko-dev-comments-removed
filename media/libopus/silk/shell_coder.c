






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"



static inline void combine_pulses(
    opus_int         *out,   
    const opus_int   *in,    
    const opus_int   len     
)
{
    opus_int k;
    for( k = 0; k < len; k++ ) {
        out[ k ] = in[ 2 * k ] + in[ 2 * k + 1 ];
    }
}

static inline void encode_split(
    ec_enc                      *psRangeEnc,    
    const opus_int              p_child1,       
    const opus_int              p,              
    const opus_uint8            *shell_table    
)
{
    if( p > 0 ) {
        ec_enc_icdf( psRangeEnc, p_child1, &shell_table[ silk_shell_code_table_offsets[ p ] ], 8 );
    }
}

static inline void decode_split(
    opus_int                    *p_child1,      
    opus_int                    *p_child2,      
    ec_dec                      *psRangeDec,    
    const opus_int              p,              
    const opus_uint8            *shell_table    
)
{
    if( p > 0 ) {
        p_child1[ 0 ] = ec_dec_icdf( psRangeDec, &shell_table[ silk_shell_code_table_offsets[ p ] ], 8 );
        p_child2[ 0 ] = p - p_child1[ 0 ];
    } else {
        p_child1[ 0 ] = 0;
        p_child2[ 0 ] = 0;
    }
}


void silk_shell_encoder(
    ec_enc                      *psRangeEnc,                    
    const opus_int              *pulses0                        
)
{
    opus_int pulses1[ 8 ], pulses2[ 4 ], pulses3[ 2 ], pulses4[ 1 ];

    
    silk_assert( SHELL_CODEC_FRAME_LENGTH == 16 );

    
    combine_pulses( pulses1, pulses0, 8 );
    combine_pulses( pulses2, pulses1, 4 );
    combine_pulses( pulses3, pulses2, 2 );
    combine_pulses( pulses4, pulses3, 1 );

    encode_split( psRangeEnc, pulses3[  0 ], pulses4[ 0 ], silk_shell_code_table3 );

    encode_split( psRangeEnc, pulses2[  0 ], pulses3[ 0 ], silk_shell_code_table2 );

    encode_split( psRangeEnc, pulses1[  0 ], pulses2[ 0 ], silk_shell_code_table1 );
    encode_split( psRangeEnc, pulses0[  0 ], pulses1[ 0 ], silk_shell_code_table0 );
    encode_split( psRangeEnc, pulses0[  2 ], pulses1[ 1 ], silk_shell_code_table0 );

    encode_split( psRangeEnc, pulses1[  2 ], pulses2[ 1 ], silk_shell_code_table1 );
    encode_split( psRangeEnc, pulses0[  4 ], pulses1[ 2 ], silk_shell_code_table0 );
    encode_split( psRangeEnc, pulses0[  6 ], pulses1[ 3 ], silk_shell_code_table0 );

    encode_split( psRangeEnc, pulses2[  2 ], pulses3[ 1 ], silk_shell_code_table2 );

    encode_split( psRangeEnc, pulses1[  4 ], pulses2[ 2 ], silk_shell_code_table1 );
    encode_split( psRangeEnc, pulses0[  8 ], pulses1[ 4 ], silk_shell_code_table0 );
    encode_split( psRangeEnc, pulses0[ 10 ], pulses1[ 5 ], silk_shell_code_table0 );

    encode_split( psRangeEnc, pulses1[  6 ], pulses2[ 3 ], silk_shell_code_table1 );
    encode_split( psRangeEnc, pulses0[ 12 ], pulses1[ 6 ], silk_shell_code_table0 );
    encode_split( psRangeEnc, pulses0[ 14 ], pulses1[ 7 ], silk_shell_code_table0 );
}



void silk_shell_decoder(
    opus_int                    *pulses0,                       
    ec_dec                      *psRangeDec,                    
    const opus_int              pulses4                         
)
{
    opus_int pulses3[ 2 ], pulses2[ 4 ], pulses1[ 8 ];

    
    silk_assert( SHELL_CODEC_FRAME_LENGTH == 16 );

    decode_split( &pulses3[  0 ], &pulses3[  1 ], psRangeDec, pulses4,      silk_shell_code_table3 );

    decode_split( &pulses2[  0 ], &pulses2[  1 ], psRangeDec, pulses3[ 0 ], silk_shell_code_table2 );

    decode_split( &pulses1[  0 ], &pulses1[  1 ], psRangeDec, pulses2[ 0 ], silk_shell_code_table1 );
    decode_split( &pulses0[  0 ], &pulses0[  1 ], psRangeDec, pulses1[ 0 ], silk_shell_code_table0 );
    decode_split( &pulses0[  2 ], &pulses0[  3 ], psRangeDec, pulses1[ 1 ], silk_shell_code_table0 );

    decode_split( &pulses1[  2 ], &pulses1[  3 ], psRangeDec, pulses2[ 1 ], silk_shell_code_table1 );
    decode_split( &pulses0[  4 ], &pulses0[  5 ], psRangeDec, pulses1[ 2 ], silk_shell_code_table0 );
    decode_split( &pulses0[  6 ], &pulses0[  7 ], psRangeDec, pulses1[ 3 ], silk_shell_code_table0 );

    decode_split( &pulses2[  2 ], &pulses2[  3 ], psRangeDec, pulses3[ 1 ], silk_shell_code_table2 );

    decode_split( &pulses1[  4 ], &pulses1[  5 ], psRangeDec, pulses2[ 2 ], silk_shell_code_table1 );
    decode_split( &pulses0[  8 ], &pulses0[  9 ], psRangeDec, pulses1[ 4 ], silk_shell_code_table0 );
    decode_split( &pulses0[ 10 ], &pulses0[ 11 ], psRangeDec, pulses1[ 5 ], silk_shell_code_table0 );

    decode_split( &pulses1[  6 ], &pulses1[  7 ], psRangeDec, pulses2[ 3 ], silk_shell_code_table1 );
    decode_split( &pulses0[ 12 ], &pulses0[ 13 ], psRangeDec, pulses1[ 6 ], silk_shell_code_table0 );
    decode_split( &pulses0[ 14 ], &pulses0[ 15 ], psRangeDec, pulses1[ 7 ], silk_shell_code_table0 );
}
