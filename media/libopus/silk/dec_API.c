






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "API.h"
#include "main.h"




typedef struct {
    silk_decoder_state          channel_state[ DECODER_NUM_CHANNELS ];
    stereo_dec_state                sStereo;
    opus_int                         nChannelsAPI;
    opus_int                         nChannelsInternal;
    opus_int                         prev_decode_only_middle;
} silk_decoder;





opus_int silk_Get_Decoder_Size(                         
    opus_int                        *decSizeBytes       
)
{
    opus_int ret = SILK_NO_ERROR;

    *decSizeBytes = sizeof( silk_decoder );

    return ret;
}


opus_int silk_InitDecoder(                              
    void                            *decState           
)
{
    opus_int n, ret = SILK_NO_ERROR;
    silk_decoder_state *channel_state = ((silk_decoder *)decState)->channel_state;

    for( n = 0; n < DECODER_NUM_CHANNELS; n++ ) {
        ret  = silk_init_decoder( &channel_state[ n ] );
    }

    return ret;
}


opus_int silk_Decode(                                   
    void*                           decState,           
    silk_DecControlStruct*          decControl,         
    opus_int                        lostFlag,           
    opus_int                        newPacketFlag,      
    ec_dec                          *psRangeDec,        
    opus_int16                      *samplesOut,        
    opus_int32                      *nSamplesOut        
)
{
    opus_int   i, n, decode_only_middle = 0, ret = SILK_NO_ERROR;
    opus_int32 nSamplesOutDec, LBRR_symbol;
    opus_int16 samplesOut1_tmp[ 2 ][ MAX_FS_KHZ * MAX_FRAME_LENGTH_MS + 2 ];
    opus_int16 samplesOut2_tmp[ MAX_API_FS_KHZ * MAX_FRAME_LENGTH_MS ];
    opus_int32 MS_pred_Q13[ 2 ] = { 0 };
    opus_int16 *resample_out_ptr;
    silk_decoder *psDec = ( silk_decoder * )decState;
    silk_decoder_state *channel_state = psDec->channel_state;
    opus_int has_side;
    opus_int stereo_to_mono;

    
    
    
    if( newPacketFlag ) {
        for( n = 0; n < decControl->nChannelsInternal; n++ ) {
            channel_state[ n ].nFramesDecoded = 0;  
        }
    }

    
    if( decControl->nChannelsInternal > psDec->nChannelsInternal ) {
        ret += silk_init_decoder( &channel_state[ 1 ] );
    }

    stereo_to_mono = decControl->nChannelsInternal == 1 && psDec->nChannelsInternal == 2 &&
                     ( decControl->internalSampleRate == 1000*channel_state[ 0 ].fs_kHz );

    if( channel_state[ 0 ].nFramesDecoded == 0 ) {
        for( n = 0; n < decControl->nChannelsInternal; n++ ) {
            opus_int fs_kHz_dec;
            if( decControl->payloadSize_ms == 0 ) {
                
                channel_state[ n ].nFramesPerPacket = 1;
                channel_state[ n ].nb_subfr = 2;
            } else if( decControl->payloadSize_ms == 10 ) {
                channel_state[ n ].nFramesPerPacket = 1;
                channel_state[ n ].nb_subfr = 2;
            } else if( decControl->payloadSize_ms == 20 ) {
                channel_state[ n ].nFramesPerPacket = 1;
                channel_state[ n ].nb_subfr = 4;
            } else if( decControl->payloadSize_ms == 40 ) {
                channel_state[ n ].nFramesPerPacket = 2;
                channel_state[ n ].nb_subfr = 4;
            } else if( decControl->payloadSize_ms == 60 ) {
                channel_state[ n ].nFramesPerPacket = 3;
                channel_state[ n ].nb_subfr = 4;
            } else {
                silk_assert( 0 );
                return SILK_DEC_INVALID_FRAME_SIZE;
            }
            fs_kHz_dec = ( decControl->internalSampleRate >> 10 ) + 1;
            if( fs_kHz_dec != 8 && fs_kHz_dec != 12 && fs_kHz_dec != 16 ) {
                silk_assert( 0 );
                return SILK_DEC_INVALID_SAMPLING_FREQUENCY;
            }
            ret += silk_decoder_set_fs( &channel_state[ n ], fs_kHz_dec, decControl->API_sampleRate );
        }
    }

    if( decControl->nChannelsAPI == 2 && decControl->nChannelsInternal == 2 && ( psDec->nChannelsAPI == 1 || psDec->nChannelsInternal == 1 ) ) {
        silk_memset( psDec->sStereo.pred_prev_Q13, 0, sizeof( psDec->sStereo.pred_prev_Q13 ) );
        silk_memset( psDec->sStereo.sSide, 0, sizeof( psDec->sStereo.sSide ) );
        silk_memcpy( &channel_state[ 1 ].resampler_state, &channel_state[ 0 ].resampler_state, sizeof( silk_resampler_state_struct ) );
    }
    psDec->nChannelsAPI      = decControl->nChannelsAPI;
    psDec->nChannelsInternal = decControl->nChannelsInternal;

    if( decControl->API_sampleRate > MAX_API_FS_KHZ * 1000 || decControl->API_sampleRate < 8000 ) {
        ret = SILK_DEC_INVALID_SAMPLING_FREQUENCY;
        return( ret );
    }

    if( lostFlag != FLAG_PACKET_LOST && channel_state[ 0 ].nFramesDecoded == 0 ) {
        
        
        for( n = 0; n < decControl->nChannelsInternal; n++ ) {
            for( i = 0; i < channel_state[ n ].nFramesPerPacket; i++ ) {
                channel_state[ n ].VAD_flags[ i ] = ec_dec_bit_logp(psRangeDec, 1);
            }
            channel_state[ n ].LBRR_flag = ec_dec_bit_logp(psRangeDec, 1);
        }
        
        for( n = 0; n < decControl->nChannelsInternal; n++ ) {
            silk_memset( channel_state[ n ].LBRR_flags, 0, sizeof( channel_state[ n ].LBRR_flags ) );
            if( channel_state[ n ].LBRR_flag ) {
                if( channel_state[ n ].nFramesPerPacket == 1 ) {
                    channel_state[ n ].LBRR_flags[ 0 ] = 1;
                } else {
                    LBRR_symbol = ec_dec_icdf( psRangeDec, silk_LBRR_flags_iCDF_ptr[ channel_state[ n ].nFramesPerPacket - 2 ], 8 ) + 1;
                    for( i = 0; i < channel_state[ n ].nFramesPerPacket; i++ ) {
                        channel_state[ n ].LBRR_flags[ i ] = silk_RSHIFT( LBRR_symbol, i ) & 1;
                    }
                }
            }
        }

        if( lostFlag == FLAG_DECODE_NORMAL ) {
            
            for( i = 0; i < channel_state[ 0 ].nFramesPerPacket; i++ ) {
                for( n = 0; n < decControl->nChannelsInternal; n++ ) {
                    if( channel_state[ n ].LBRR_flags[ i ] ) {
                        opus_int pulses[ MAX_FRAME_LENGTH ];
                        opus_int condCoding;

                        if( decControl->nChannelsInternal == 2 && n == 0 ) {
                            silk_stereo_decode_pred( psRangeDec, MS_pred_Q13 );
                            if( channel_state[ 1 ].LBRR_flags[ i ] == 0 ) {
                                silk_stereo_decode_mid_only( psRangeDec, &decode_only_middle );
                            }
                        }
                        
                        if( i > 0 && channel_state[ n ].LBRR_flags[ i - 1 ] ) {
                            condCoding = CODE_CONDITIONALLY;
                        } else {
                            condCoding = CODE_INDEPENDENTLY;
                        }
                        silk_decode_indices( &channel_state[ n ], psRangeDec, i, 1, condCoding );
                        silk_decode_pulses( psRangeDec, pulses, channel_state[ n ].indices.signalType,
                            channel_state[ n ].indices.quantOffsetType, channel_state[ n ].frame_length );
                    }
                }
            }
        }
    }

    
    if( decControl->nChannelsInternal == 2 ) {
        if(   lostFlag == FLAG_DECODE_NORMAL ||
            ( lostFlag == FLAG_DECODE_LBRR && channel_state[ 0 ].LBRR_flags[ channel_state[ 0 ].nFramesDecoded ] == 1 ) )
        {
            silk_stereo_decode_pred( psRangeDec, MS_pred_Q13 );
            
            if( ( lostFlag == FLAG_DECODE_NORMAL && channel_state[ 1 ].VAD_flags[ channel_state[ 0 ].nFramesDecoded ] == 0 ) ||
                ( lostFlag == FLAG_DECODE_LBRR && channel_state[ 1 ].LBRR_flags[ channel_state[ 0 ].nFramesDecoded ] == 0 ) )
            {
                silk_stereo_decode_mid_only( psRangeDec, &decode_only_middle );
            } else {
                decode_only_middle = 0;
            }
        } else {
            for( n = 0; n < 2; n++ ) {
                MS_pred_Q13[ n ] = psDec->sStereo.pred_prev_Q13[ n ];
            }
        }
    }

    
    if( decControl->nChannelsInternal == 2 && decode_only_middle == 0 && psDec->prev_decode_only_middle == 1 ) {
        silk_memset( psDec->channel_state[ 1 ].outBuf, 0, sizeof(psDec->channel_state[ 1 ].outBuf) );
        silk_memset( psDec->channel_state[ 1 ].sLPC_Q14_buf, 0, sizeof(psDec->channel_state[ 1 ].sLPC_Q14_buf) );
        psDec->channel_state[ 1 ].lagPrev        = 100;
        psDec->channel_state[ 1 ].LastGainIndex  = 10;
        psDec->channel_state[ 1 ].prevSignalType = TYPE_NO_VOICE_ACTIVITY;
        psDec->channel_state[ 1 ].first_frame_after_reset = 1;
    }

    if( lostFlag == FLAG_DECODE_NORMAL ) {
        has_side = !decode_only_middle;
    } else {
        has_side = !psDec->prev_decode_only_middle
              || (decControl->nChannelsInternal == 2 && lostFlag == FLAG_DECODE_LBRR && channel_state[1].LBRR_flags[ channel_state[1].nFramesDecoded ] == 1 );
    }
    
    for( n = 0; n < decControl->nChannelsInternal; n++ ) {
        if( n == 0 || has_side ) {
            opus_int FrameIndex;
            opus_int condCoding;

            FrameIndex = channel_state[ 0 ].nFramesDecoded - n;
            
            if( FrameIndex <= 0 ) {
                condCoding = CODE_INDEPENDENTLY;
            } else if( lostFlag == FLAG_DECODE_LBRR ) {
                condCoding = channel_state[ n ].LBRR_flags[ FrameIndex - 1 ] ? CODE_CONDITIONALLY : CODE_INDEPENDENTLY;
            } else if( n > 0 && psDec->prev_decode_only_middle ) {
                

                condCoding = CODE_INDEPENDENTLY_NO_LTP_SCALING;
            } else {
                condCoding = CODE_CONDITIONALLY;
            }
            ret += silk_decode_frame( &channel_state[ n ], psRangeDec, &samplesOut1_tmp[ n ][ 2 ], &nSamplesOutDec, lostFlag, condCoding);
        } else {
            silk_memset( &samplesOut1_tmp[ n ][ 2 ], 0, nSamplesOutDec * sizeof( opus_int16 ) );
        }
        channel_state[ n ].nFramesDecoded++;
    }

    if( decControl->nChannelsAPI == 2 && decControl->nChannelsInternal == 2 ) {
        
        silk_stereo_MS_to_LR( &psDec->sStereo, samplesOut1_tmp[ 0 ], samplesOut1_tmp[ 1 ], MS_pred_Q13, channel_state[ 0 ].fs_kHz, nSamplesOutDec );
    } else {
        
        silk_memcpy( samplesOut1_tmp[ 0 ], psDec->sStereo.sMid, 2 * sizeof( opus_int16 ) );
        silk_memcpy( psDec->sStereo.sMid, &samplesOut1_tmp[ 0 ][ nSamplesOutDec ], 2 * sizeof( opus_int16 ) );
    }

    
    *nSamplesOut = silk_DIV32( nSamplesOutDec * decControl->API_sampleRate, silk_SMULBB( channel_state[ 0 ].fs_kHz, 1000 ) );

    
    if( decControl->nChannelsAPI == 2 ) {
        resample_out_ptr = samplesOut2_tmp;
    } else {
        resample_out_ptr = samplesOut;
    }

    for( n = 0; n < silk_min( decControl->nChannelsAPI, decControl->nChannelsInternal ); n++ ) {

        
        ret += silk_resampler( &channel_state[ n ].resampler_state, resample_out_ptr, &samplesOut1_tmp[ n ][ 1 ], nSamplesOutDec );

        
        if( decControl->nChannelsAPI == 2 ) {
            for( i = 0; i < *nSamplesOut; i++ ) {
                samplesOut[ n + 2 * i ] = resample_out_ptr[ i ];
            }
        }
    }

    
    if( decControl->nChannelsAPI == 2 && decControl->nChannelsInternal == 1 ) {
        if ( stereo_to_mono ){
            

            ret += silk_resampler( &channel_state[ 1 ].resampler_state, resample_out_ptr, &samplesOut1_tmp[ 0 ][ 1 ], nSamplesOutDec );

            for( i = 0; i < *nSamplesOut; i++ ) {
                samplesOut[ 1 + 2 * i ] = resample_out_ptr[ i ];
            }
        } else {
            for( i = 0; i < *nSamplesOut; i++ ) {
                samplesOut[ 1 + 2 * i ] = samplesOut[ 0 + 2 * i ];
            }
        }
    }

    
    if( channel_state[ 0 ].prevSignalType == TYPE_VOICED ) {
        int mult_tab[ 3 ] = { 6, 4, 3 };
        decControl->prevPitchLag = channel_state[ 0 ].lagPrev * mult_tab[ ( channel_state[ 0 ].fs_kHz - 8 ) >> 2 ];
    } else {
        decControl->prevPitchLag = 0;
    }

    if( lostFlag == FLAG_PACKET_LOST ) {
       

       for ( i = 0; i < psDec->nChannelsInternal; i++ )
          psDec->channel_state[ i ].LastGainIndex = 10;
    } else {
       psDec->prev_decode_only_middle = decode_only_middle;
    }
    return ret;
}


opus_int silk_get_TOC(
    const opus_uint8                *payload,           
    const opus_int                  nBytesIn,           
    const opus_int                  nFramesPerPayload,  
    silk_TOC_struct                 *Silk_TOC           
)
{
    opus_int i, flags, ret = SILK_NO_ERROR;

    if( nBytesIn < 1 ) {
        return -1;
    }
    if( nFramesPerPayload < 0 || nFramesPerPayload > 3 ) {
        return -1;
    }

    silk_memset( Silk_TOC, 0, sizeof( *Silk_TOC ) );

    
    flags = silk_RSHIFT( payload[ 0 ], 7 - nFramesPerPayload ) & ( silk_LSHIFT( 1, nFramesPerPayload + 1 ) - 1 );

    Silk_TOC->inbandFECFlag = flags & 1;
    for( i = nFramesPerPayload - 1; i >= 0 ; i-- ) {
        flags = silk_RSHIFT( flags, 1 );
        Silk_TOC->VADFlags[ i ] = flags & 1;
        Silk_TOC->VADFlag |= flags & 1;
    }

    return ret;
}
