






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "define.h"
#include "API.h"
#include "control.h"
#include "typedef.h"
#include "structs.h"
#include "tuning_parameters.h"
#ifdef FIXED_POINT
#include "main_FIX.h"
#else
#include "main_FLP.h"
#endif





opus_int silk_Get_Encoder_Size(                         
    opus_int                        *encSizeBytes       
)
{
    opus_int ret = SILK_NO_ERROR;

    *encSizeBytes = sizeof( silk_encoder );

    return ret;
}




opus_int silk_InitEncoder(                              
    void                            *encState,          
    silk_EncControlStruct           *encStatus          
)
{
    silk_encoder *psEnc;
    opus_int n, ret = SILK_NO_ERROR;

    psEnc = (silk_encoder *)encState;

    
    silk_memset( psEnc, 0, sizeof( silk_encoder ) );
    for( n = 0; n < ENCODER_NUM_CHANNELS; n++ ) {
        if( ret += silk_init_encoder( &psEnc->state_Fxx[ n ] ) ) {
            silk_assert( 0 );
        }
    }

    psEnc->nChannelsAPI = 1;
    psEnc->nChannelsInternal = 1;

    
    if( ret += silk_QueryEncoder( encState, encStatus ) ) {
        silk_assert( 0 );
    }

    return ret;
}




opus_int silk_QueryEncoder(                             
    const void                      *encState,          
    silk_EncControlStruct           *encStatus          
)
{
    opus_int ret = SILK_NO_ERROR;
    silk_encoder_state_Fxx *state_Fxx;
    silk_encoder *psEnc = (silk_encoder *)encState;

    state_Fxx = psEnc->state_Fxx;

    encStatus->nChannelsAPI              = psEnc->nChannelsAPI;
    encStatus->nChannelsInternal         = psEnc->nChannelsInternal;
    encStatus->API_sampleRate            = state_Fxx[ 0 ].sCmn.API_fs_Hz;
    encStatus->maxInternalSampleRate     = state_Fxx[ 0 ].sCmn.maxInternal_fs_Hz;
    encStatus->minInternalSampleRate     = state_Fxx[ 0 ].sCmn.minInternal_fs_Hz;
    encStatus->desiredInternalSampleRate = state_Fxx[ 0 ].sCmn.desiredInternal_fs_Hz;
    encStatus->payloadSize_ms            = state_Fxx[ 0 ].sCmn.PacketSize_ms;
    encStatus->bitRate                   = state_Fxx[ 0 ].sCmn.TargetRate_bps;
    encStatus->packetLossPercentage      = state_Fxx[ 0 ].sCmn.PacketLoss_perc;
    encStatus->complexity                = state_Fxx[ 0 ].sCmn.Complexity;
    encStatus->useInBandFEC              = state_Fxx[ 0 ].sCmn.useInBandFEC;
    encStatus->useDTX                    = state_Fxx[ 0 ].sCmn.useDTX;
    encStatus->useCBR                    = state_Fxx[ 0 ].sCmn.useCBR;
    encStatus->internalSampleRate        = silk_SMULBB( state_Fxx[ 0 ].sCmn.fs_kHz, 1000 );
    encStatus->allowBandwidthSwitch      = state_Fxx[ 0 ].sCmn.allow_bandwidth_switch;
    encStatus->inWBmodeWithoutVariableLP = state_Fxx[ 0 ].sCmn.fs_kHz == 16 && state_Fxx[ 0 ].sCmn.sLP.mode == 0;

    return ret;
}







opus_int silk_Encode(                                   
    void                            *encState,          
    silk_EncControlStruct           *encControl,        
    const opus_int16                *samplesIn,         
    opus_int                        nSamplesIn,         
    ec_enc                          *psRangeEnc,        
    opus_int                        *nBytesOut,         
    const opus_int                  prefillFlag         
)
{
    opus_int   n, i, nBits, flags, tmp_payloadSize_ms = 0, tmp_complexity = 0, ret = 0;
    opus_int   nSamplesToBuffer, nBlocksOf10ms, nSamplesFromInput = 0;
    opus_int   speech_act_thr_for_switch_Q8;
    opus_int32 TargetRate_bps, MStargetRates_bps[ 2 ], channelRate_bps, LBRR_symbol, sum;
    silk_encoder *psEnc = ( silk_encoder * )encState;
    opus_int16 buf[ MAX_FRAME_LENGTH_MS * MAX_API_FS_KHZ ];
    opus_int transition, curr_block, tot_blocks;

    psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded = psEnc->state_Fxx[ 1 ].sCmn.nFramesEncoded = 0;

    
    if( ( ret = check_control_input( encControl ) != 0 ) ) {
        silk_assert( 0 );
        return ret;
    }

    encControl->switchReady = 0;

    if( encControl->nChannelsInternal > psEnc->nChannelsInternal ) {
        
        ret += silk_init_encoder( &psEnc->state_Fxx[ 1 ] );
        silk_memset( psEnc->sStereo.pred_prev_Q13, 0, sizeof( psEnc->sStereo.pred_prev_Q13 ) );
        silk_memset( psEnc->sStereo.sSide, 0, sizeof( psEnc->sStereo.sSide ) );
        psEnc->sStereo.mid_side_amp_Q0[ 0 ] = 0;
        psEnc->sStereo.mid_side_amp_Q0[ 1 ] = 1;
        psEnc->sStereo.mid_side_amp_Q0[ 2 ] = 0;
        psEnc->sStereo.mid_side_amp_Q0[ 3 ] = 1;
        psEnc->sStereo.width_prev_Q14 = 0;
        psEnc->sStereo.smth_width_Q14 = SILK_FIX_CONST( 1, 14 );
        if( psEnc->nChannelsAPI == 2 ) {
            silk_memcpy( &psEnc->state_Fxx[ 1 ].sCmn.resampler_state, &psEnc->state_Fxx[ 0 ].sCmn.resampler_state, sizeof( silk_resampler_state_struct ) );
            silk_memcpy( &psEnc->state_Fxx[ 1 ].sCmn.In_HP_State,     &psEnc->state_Fxx[ 0 ].sCmn.In_HP_State,     sizeof( psEnc->state_Fxx[ 1 ].sCmn.In_HP_State ) );
        }
    }

    transition = (encControl->payloadSize_ms != psEnc->state_Fxx[ 0 ].sCmn.PacketSize_ms) || (psEnc->nChannelsInternal != encControl->nChannelsInternal);

    psEnc->nChannelsAPI = encControl->nChannelsAPI;
    psEnc->nChannelsInternal = encControl->nChannelsInternal;

    nBlocksOf10ms = silk_DIV32( 100 * nSamplesIn, encControl->API_sampleRate );
    tot_blocks = ( nBlocksOf10ms > 1 ) ? nBlocksOf10ms >> 1 : 1;
    curr_block = 0;
    if( prefillFlag ) {
        
        if( nBlocksOf10ms != 1 ) {
            ret = SILK_ENC_INPUT_INVALID_NO_OF_SAMPLES;
            silk_assert( 0 );
            return ret;
        }
        
        for( n = 0; n < encControl->nChannelsInternal; n++ ) {
            if( (ret = silk_init_encoder( &psEnc->state_Fxx[ n ] ) ) != 0 ) {
                silk_assert( 0 );
            }
        }
        tmp_payloadSize_ms = encControl->payloadSize_ms;
        encControl->payloadSize_ms = 10;
        tmp_complexity = encControl->complexity;
        encControl->complexity = 0;
        for( n = 0; n < encControl->nChannelsInternal; n++ ) {
            psEnc->state_Fxx[ n ].sCmn.controlled_since_last_payload = 0;
            psEnc->state_Fxx[ n ].sCmn.prefillFlag = 1;
        }
    } else {
        
        if( nBlocksOf10ms * encControl->API_sampleRate != 100 * nSamplesIn || nSamplesIn < 0 ) {
            ret = SILK_ENC_INPUT_INVALID_NO_OF_SAMPLES;
            silk_assert( 0 );
            return ret;
        }
        
        if( 1000 * (opus_int32)nSamplesIn > encControl->payloadSize_ms * encControl->API_sampleRate ) {
            ret = SILK_ENC_INPUT_INVALID_NO_OF_SAMPLES;
            silk_assert( 0 );
            return ret;
        }
    }

    TargetRate_bps = silk_RSHIFT32( encControl->bitRate, encControl->nChannelsInternal - 1 );
    for( n = 0; n < encControl->nChannelsInternal; n++ ) {
        
        opus_int force_fs_kHz = (n==1) ? psEnc->state_Fxx[0].sCmn.fs_kHz : 0;
        if( ( ret = silk_control_encoder( &psEnc->state_Fxx[ n ], encControl, TargetRate_bps, psEnc->allowBandwidthSwitch, n, force_fs_kHz ) ) != 0 ) {
            silk_assert( 0 );
            return ret;
        }
        if( psEnc->state_Fxx[n].sCmn.first_frame_after_reset || transition ) {
            for( i = 0; i < psEnc->state_Fxx[ 0 ].sCmn.nFramesPerPacket; i++ ) {
                psEnc->state_Fxx[ n ].sCmn.LBRR_flags[ i ] = 0;
            }
        }
        psEnc->state_Fxx[ n ].sCmn.inDTX = psEnc->state_Fxx[ n ].sCmn.useDTX;
    }
    silk_assert( encControl->nChannelsInternal == 1 || psEnc->state_Fxx[ 0 ].sCmn.fs_kHz == psEnc->state_Fxx[ 1 ].sCmn.fs_kHz );

    
    while( 1 ) {
        nSamplesToBuffer  = psEnc->state_Fxx[ 0 ].sCmn.frame_length - psEnc->state_Fxx[ 0 ].sCmn.inputBufIx;
        nSamplesToBuffer  = silk_min( nSamplesToBuffer, 10 * nBlocksOf10ms * psEnc->state_Fxx[ 0 ].sCmn.fs_kHz );
        nSamplesFromInput = silk_DIV32_16( nSamplesToBuffer * psEnc->state_Fxx[ 0 ].sCmn.API_fs_Hz, psEnc->state_Fxx[ 0 ].sCmn.fs_kHz * 1000 );
        
        if( encControl->nChannelsAPI == 2 && encControl->nChannelsInternal == 2 ) {
            opus_int id = psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded;
            for( n = 0; n < nSamplesFromInput; n++ ) {
                buf[ n ] = samplesIn[ 2 * n ];
            }
            
            if( psEnc->nPrevChannelsInternal == 1 && id==0 ) {
               silk_memcpy( &psEnc->state_Fxx[ 1 ].sCmn.resampler_state, &psEnc->state_Fxx[ 0 ].sCmn.resampler_state, sizeof(psEnc->state_Fxx[ 1 ].sCmn.resampler_state));
            }

            ret += silk_resampler( &psEnc->state_Fxx[ 0 ].sCmn.resampler_state,
                &psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ psEnc->state_Fxx[ 0 ].sCmn.inputBufIx + 2 ], buf, nSamplesFromInput );
            psEnc->state_Fxx[ 0 ].sCmn.inputBufIx += nSamplesToBuffer;

            nSamplesToBuffer  = psEnc->state_Fxx[ 1 ].sCmn.frame_length - psEnc->state_Fxx[ 1 ].sCmn.inputBufIx;
            nSamplesToBuffer  = silk_min( nSamplesToBuffer, 10 * nBlocksOf10ms * psEnc->state_Fxx[ 1 ].sCmn.fs_kHz );
            for( n = 0; n < nSamplesFromInput; n++ ) {
                buf[ n ] = samplesIn[ 2 * n + 1 ];
            }
            ret += silk_resampler( &psEnc->state_Fxx[ 1 ].sCmn.resampler_state,
                &psEnc->state_Fxx[ 1 ].sCmn.inputBuf[ psEnc->state_Fxx[ 1 ].sCmn.inputBufIx + 2 ], buf, nSamplesFromInput );

            psEnc->state_Fxx[ 1 ].sCmn.inputBufIx += nSamplesToBuffer;
        } else if( encControl->nChannelsAPI == 2 && encControl->nChannelsInternal == 1 ) {
            
            for( n = 0; n < nSamplesFromInput; n++ ) {
                sum = samplesIn[ 2 * n ] + samplesIn[ 2 * n + 1 ];
                buf[ n ] = (opus_int16)silk_RSHIFT_ROUND( sum,  1 );
            }
            ret += silk_resampler( &psEnc->state_Fxx[ 0 ].sCmn.resampler_state,
                &psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ psEnc->state_Fxx[ 0 ].sCmn.inputBufIx + 2 ], buf, nSamplesFromInput );
            
            if( psEnc->nPrevChannelsInternal == 2 && psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded == 0 ) {
               ret += silk_resampler( &psEnc->state_Fxx[ 1 ].sCmn.resampler_state,
                   &psEnc->state_Fxx[ 1 ].sCmn.inputBuf[ psEnc->state_Fxx[ 1 ].sCmn.inputBufIx + 2 ], buf, nSamplesFromInput );
               for( n = 0; n < psEnc->state_Fxx[ 0 ].sCmn.frame_length; n++ ) {
                  psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ psEnc->state_Fxx[ 0 ].sCmn.inputBufIx+n+2 ] =
                        silk_RSHIFT(psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ psEnc->state_Fxx[ 0 ].sCmn.inputBufIx+n+2 ]
                                  + psEnc->state_Fxx[ 1 ].sCmn.inputBuf[ psEnc->state_Fxx[ 1 ].sCmn.inputBufIx+n+2 ], 1);
               }
            }
            psEnc->state_Fxx[ 0 ].sCmn.inputBufIx += nSamplesToBuffer;
        } else {
            silk_assert( encControl->nChannelsAPI == 1 && encControl->nChannelsInternal == 1 );
            silk_memcpy(buf, samplesIn, nSamplesFromInput*sizeof(opus_int16));
            ret += silk_resampler( &psEnc->state_Fxx[ 0 ].sCmn.resampler_state,
                &psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ psEnc->state_Fxx[ 0 ].sCmn.inputBufIx + 2 ], buf, nSamplesFromInput );
            psEnc->state_Fxx[ 0 ].sCmn.inputBufIx += nSamplesToBuffer;
        }

        samplesIn  += nSamplesFromInput * encControl->nChannelsAPI;
        nSamplesIn -= nSamplesFromInput;

        
        psEnc->allowBandwidthSwitch = 0;

        
        if( psEnc->state_Fxx[ 0 ].sCmn.inputBufIx >= psEnc->state_Fxx[ 0 ].sCmn.frame_length ) {
            
            silk_assert( psEnc->state_Fxx[ 0 ].sCmn.inputBufIx == psEnc->state_Fxx[ 0 ].sCmn.frame_length );
            silk_assert( encControl->nChannelsInternal == 1 || psEnc->state_Fxx[ 1 ].sCmn.inputBufIx == psEnc->state_Fxx[ 1 ].sCmn.frame_length );

            
            if( psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded == 0 && !prefillFlag ) {
                
                opus_uint8 iCDF[ 2 ] = { 0, 0 };
                iCDF[ 0 ] = 256 - silk_RSHIFT( 256, ( psEnc->state_Fxx[ 0 ].sCmn.nFramesPerPacket + 1 ) * encControl->nChannelsInternal );
                ec_enc_icdf( psRangeEnc, 0, iCDF, 8 );

                
                
                for( n = 0; n < encControl->nChannelsInternal; n++ ) {
                    LBRR_symbol = 0;
                    for( i = 0; i < psEnc->state_Fxx[ n ].sCmn.nFramesPerPacket; i++ ) {
                        LBRR_symbol |= silk_LSHIFT( psEnc->state_Fxx[ n ].sCmn.LBRR_flags[ i ], i );
                    }
                    psEnc->state_Fxx[ n ].sCmn.LBRR_flag = LBRR_symbol > 0 ? 1 : 0;
                    if( LBRR_symbol && psEnc->state_Fxx[ n ].sCmn.nFramesPerPacket > 1 ) {
                        ec_enc_icdf( psRangeEnc, LBRR_symbol - 1, silk_LBRR_flags_iCDF_ptr[ psEnc->state_Fxx[ n ].sCmn.nFramesPerPacket - 2 ], 8 );
                    }
                }

                
                for( i = 0; i < psEnc->state_Fxx[ 0 ].sCmn.nFramesPerPacket; i++ ) {
                    for( n = 0; n < encControl->nChannelsInternal; n++ ) {
                        if( psEnc->state_Fxx[ n ].sCmn.LBRR_flags[ i ] ) {
                            opus_int condCoding;

                            if( encControl->nChannelsInternal == 2 && n == 0 ) {
                                silk_stereo_encode_pred( psRangeEnc, psEnc->sStereo.predIx[ i ] );
                                
                                if( psEnc->state_Fxx[ 1 ].sCmn.LBRR_flags[ i ] == 0 ) {
                                    silk_stereo_encode_mid_only( psRangeEnc, psEnc->sStereo.mid_only_flags[ i ] );
                                }
                            }
                            
                            if( i > 0 && psEnc->state_Fxx[ n ].sCmn.LBRR_flags[ i - 1 ] ) {
                                condCoding = CODE_CONDITIONALLY;
                            } else {
                                condCoding = CODE_INDEPENDENTLY;
                            }
                            silk_encode_indices( &psEnc->state_Fxx[ n ].sCmn, psRangeEnc, i, 1, condCoding );
                            silk_encode_pulses( psRangeEnc, psEnc->state_Fxx[ n ].sCmn.indices_LBRR[i].signalType, psEnc->state_Fxx[ n ].sCmn.indices_LBRR[i].quantOffsetType,
                                psEnc->state_Fxx[ n ].sCmn.pulses_LBRR[ i ], psEnc->state_Fxx[ n ].sCmn.frame_length );
                        }
                    }
                }

                
                for( n = 0; n < encControl->nChannelsInternal; n++ ) {
                    silk_memset( psEnc->state_Fxx[ n ].sCmn.LBRR_flags, 0, sizeof( psEnc->state_Fxx[ n ].sCmn.LBRR_flags ) );
                }
            }

            silk_HP_variable_cutoff( psEnc->state_Fxx );

            
            nBits = silk_DIV32_16( silk_MUL( encControl->bitRate, encControl->payloadSize_ms ), 1000 );
            
            if( !prefillFlag ) {
                nBits -= ec_tell( psRangeEnc ) >> 1;
            }
            
            nBits = silk_DIV32_16( nBits, psEnc->state_Fxx[ 0 ].sCmn.nFramesPerPacket - psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded );
            
            if( encControl->payloadSize_ms == 10 ) {
                TargetRate_bps = silk_SMULBB( nBits, 100 );
            } else {
                TargetRate_bps = silk_SMULBB( nBits, 50 );
            }
            
            TargetRate_bps -= silk_DIV32_16( silk_MUL( psEnc->nBitsExceeded, 1000 ), BITRESERVOIR_DECAY_TIME_MS );
            
            TargetRate_bps = silk_LIMIT( TargetRate_bps, encControl->bitRate, 5000 );

            
            if( encControl->nChannelsInternal == 2 ) {
                silk_stereo_LR_to_MS( &psEnc->sStereo, &psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ 2 ], &psEnc->state_Fxx[ 1 ].sCmn.inputBuf[ 2 ],
                    psEnc->sStereo.predIx[ psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded ], &psEnc->sStereo.mid_only_flags[ psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded ],
                    MStargetRates_bps, TargetRate_bps, psEnc->state_Fxx[ 0 ].sCmn.speech_activity_Q8, encControl->toMono,
                    psEnc->state_Fxx[ 0 ].sCmn.fs_kHz, psEnc->state_Fxx[ 0 ].sCmn.frame_length );
                if( psEnc->sStereo.mid_only_flags[ psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded ] == 0 ) {
                    
                    if( psEnc->prev_decode_only_middle == 1 ) {
                        silk_memset( &psEnc->state_Fxx[ 1 ].sShape,               0, sizeof( psEnc->state_Fxx[ 1 ].sShape ) );
                        silk_memset( &psEnc->state_Fxx[ 1 ].sPrefilt,             0, sizeof( psEnc->state_Fxx[ 1 ].sPrefilt ) );
                        silk_memset( &psEnc->state_Fxx[ 1 ].sCmn.sNSQ,            0, sizeof( psEnc->state_Fxx[ 1 ].sCmn.sNSQ ) );
                        silk_memset( psEnc->state_Fxx[ 1 ].sCmn.prev_NLSFq_Q15,   0, sizeof( psEnc->state_Fxx[ 1 ].sCmn.prev_NLSFq_Q15 ) );
                        silk_memset( &psEnc->state_Fxx[ 1 ].sCmn.sLP.In_LP_State, 0, sizeof( psEnc->state_Fxx[ 1 ].sCmn.sLP.In_LP_State ) );
                        psEnc->state_Fxx[ 1 ].sCmn.prevLag                 = 100;
                        psEnc->state_Fxx[ 1 ].sCmn.sNSQ.lagPrev            = 100;
                        psEnc->state_Fxx[ 1 ].sShape.LastGainIndex         = 10;
                        psEnc->state_Fxx[ 1 ].sCmn.prevSignalType          = TYPE_NO_VOICE_ACTIVITY;
                        psEnc->state_Fxx[ 1 ].sCmn.sNSQ.prev_gain_Q16      = 65536;
                        psEnc->state_Fxx[ 1 ].sCmn.first_frame_after_reset = 1;
                    }
                    silk_encode_do_VAD_Fxx( &psEnc->state_Fxx[ 1 ] );
                } else {
                    psEnc->state_Fxx[ 1 ].sCmn.VAD_flags[ psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded ] = 0;
                }
                if( !prefillFlag ) {
                    silk_stereo_encode_pred( psRangeEnc, psEnc->sStereo.predIx[ psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded ] );
                    if( psEnc->state_Fxx[ 1 ].sCmn.VAD_flags[ psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded ] == 0 ) {
                        silk_stereo_encode_mid_only( psRangeEnc, psEnc->sStereo.mid_only_flags[ psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded ] );
                    }
                }
            } else {
                
                silk_memcpy( psEnc->state_Fxx[ 0 ].sCmn.inputBuf, psEnc->sStereo.sMid, 2 * sizeof( opus_int16 ) );
                silk_memcpy( psEnc->sStereo.sMid, &psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ psEnc->state_Fxx[ 0 ].sCmn.frame_length ], 2 * sizeof( opus_int16 ) );
            }
            silk_encode_do_VAD_Fxx( &psEnc->state_Fxx[ 0 ] );

            
            for( n = 0; n < encControl->nChannelsInternal; n++ ) {
                opus_int maxBits, useCBR;

                
                maxBits = encControl->maxBits;
                if( tot_blocks == 2 && curr_block == 0 ) {
                    maxBits = maxBits * 3 / 5;
                } else if( tot_blocks == 3 ) {
                    if( curr_block == 0 ) {
                        maxBits = maxBits * 2 / 5;
                    } else if( curr_block == 1 ) {
                        maxBits = maxBits * 3 / 4;
                    }
                }
                useCBR = encControl->useCBR && curr_block == tot_blocks - 1;

                if( encControl->nChannelsInternal == 1 ) {
                    channelRate_bps = TargetRate_bps;
                } else {
                    channelRate_bps = MStargetRates_bps[ n ];
                    if( n == 0 && MStargetRates_bps[ 1 ] > 0 ) {
                        useCBR = 0;
                        
                        maxBits -= encControl->maxBits / ( tot_blocks * 2 );
                    }
                }

                if( channelRate_bps > 0 ) {
                    opus_int condCoding;

                    silk_control_SNR( &psEnc->state_Fxx[ n ].sCmn, channelRate_bps );

                    
                    if( psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded - n <= 0 ) {
                        condCoding = CODE_INDEPENDENTLY;
                    } else if( n > 0 && psEnc->prev_decode_only_middle ) {
                        

                        condCoding = CODE_INDEPENDENTLY_NO_LTP_SCALING;
                    } else {
                        condCoding = CODE_CONDITIONALLY;
                    }
                    if( ( ret = silk_encode_frame_Fxx( &psEnc->state_Fxx[ n ], nBytesOut, psRangeEnc, condCoding, maxBits, useCBR ) ) != 0 ) {
                        silk_assert( 0 );
                    }
                }
                psEnc->state_Fxx[ n ].sCmn.controlled_since_last_payload = 0;
                psEnc->state_Fxx[ n ].sCmn.inputBufIx = 0;
                psEnc->state_Fxx[ n ].sCmn.nFramesEncoded++;
            }
            psEnc->prev_decode_only_middle = psEnc->sStereo.mid_only_flags[ psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded - 1 ];

            
            if( *nBytesOut > 0 && psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded == psEnc->state_Fxx[ 0 ].sCmn.nFramesPerPacket) {
                flags = 0;
                for( n = 0; n < encControl->nChannelsInternal; n++ ) {
                    for( i = 0; i < psEnc->state_Fxx[ n ].sCmn.nFramesPerPacket; i++ ) {
                        flags  = silk_LSHIFT( flags, 1 );
                        flags |= psEnc->state_Fxx[ n ].sCmn.VAD_flags[ i ];
                    }
                    flags  = silk_LSHIFT( flags, 1 );
                    flags |= psEnc->state_Fxx[ n ].sCmn.LBRR_flag;
                }
                if( !prefillFlag ) {
                    ec_enc_patch_initial_bits( psRangeEnc, flags, ( psEnc->state_Fxx[ 0 ].sCmn.nFramesPerPacket + 1 ) * encControl->nChannelsInternal );
                }

                
                if( psEnc->state_Fxx[ 0 ].sCmn.inDTX && ( encControl->nChannelsInternal == 1 || psEnc->state_Fxx[ 1 ].sCmn.inDTX ) ) {
                    *nBytesOut = 0;
                }

                psEnc->nBitsExceeded += *nBytesOut * 8;
                psEnc->nBitsExceeded -= silk_DIV32_16( silk_MUL( encControl->bitRate, encControl->payloadSize_ms ), 1000 );
                psEnc->nBitsExceeded  = silk_LIMIT( psEnc->nBitsExceeded, 0, 10000 );

                
                speech_act_thr_for_switch_Q8 = silk_SMLAWB( SILK_FIX_CONST( SPEECH_ACTIVITY_DTX_THRES, 8 ),
                    SILK_FIX_CONST( ( 1 - SPEECH_ACTIVITY_DTX_THRES ) / MAX_BANDWIDTH_SWITCH_DELAY_MS, 16 + 8 ), psEnc->timeSinceSwitchAllowed_ms );
                if( psEnc->state_Fxx[ 0 ].sCmn.speech_activity_Q8 < speech_act_thr_for_switch_Q8 ) {
                    psEnc->allowBandwidthSwitch = 1;
                    psEnc->timeSinceSwitchAllowed_ms = 0;
                } else {
                    psEnc->allowBandwidthSwitch = 0;
                    psEnc->timeSinceSwitchAllowed_ms += encControl->payloadSize_ms;
                }
            }

            if( nSamplesIn == 0 ) {
                break;
            }
        } else {
            break;
        }
        curr_block++;
    }

    psEnc->nPrevChannelsInternal = encControl->nChannelsInternal;

    encControl->allowBandwidthSwitch = psEnc->allowBandwidthSwitch;
    encControl->inWBmodeWithoutVariableLP = psEnc->state_Fxx[ 0 ].sCmn.fs_kHz == 16 && psEnc->state_Fxx[ 0 ].sCmn.sLP.mode == 0;
    encControl->internalSampleRate = silk_SMULBB( psEnc->state_Fxx[ 0 ].sCmn.fs_kHz, 1000 );
    encControl->stereoWidth_Q14 = encControl->toMono ? 0 : psEnc->sStereo.smth_width_Q14;
    if( prefillFlag ) {
        encControl->payloadSize_ms = tmp_payloadSize_ms;
        encControl->complexity = tmp_complexity;
        for( n = 0; n < encControl->nChannelsInternal; n++ ) {
            psEnc->state_Fxx[ n ].sCmn.controlled_since_last_payload = 0;
            psEnc->state_Fxx[ n ].sCmn.prefillFlag = 0;
        }
    }

    return ret;
}

