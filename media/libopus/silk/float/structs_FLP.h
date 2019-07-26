






























#ifndef SILK_STRUCTS_FLP_H
#define SILK_STRUCTS_FLP_H

#include "typedef.h"
#include "main.h"
#include "structs.h"

#ifdef __cplusplus
extern "C"
{
#endif




typedef struct {
    opus_int8                   LastGainIndex;
    silk_float                  HarmBoost_smth;
    silk_float                  HarmShapeGain_smth;
    silk_float                  Tilt_smth;
} silk_shape_state_FLP;




typedef struct {
    silk_float                  sLTP_shp[ LTP_BUF_LENGTH ];
    silk_float                  sAR_shp[ MAX_SHAPE_LPC_ORDER + 1 ];
    opus_int                    sLTP_shp_buf_idx;
    silk_float                  sLF_AR_shp;
    silk_float                  sLF_MA_shp;
    silk_float                  sHarmHP;
    opus_int32                  rand_seed;
    opus_int                    lagPrev;
} silk_prefilter_state_FLP;




typedef struct {
    silk_encoder_state          sCmn;                               
    silk_shape_state_FLP        sShape;                             
    silk_prefilter_state_FLP    sPrefilt;                           

    
    silk_float                  x_buf[ 2 * MAX_FRAME_LENGTH + LA_SHAPE_MAX ];
    silk_float                  LTPCorr;                            
} silk_encoder_state_FLP;




typedef struct {
    
    silk_float                  Gains[ MAX_NB_SUBFR ];
    silk_float                  PredCoef[ 2 ][ MAX_LPC_ORDER ];     
    silk_float                  LTPCoef[LTP_ORDER * MAX_NB_SUBFR];
    silk_float                  LTP_scale;
    opus_int                    pitchL[ MAX_NB_SUBFR ];

    
    silk_float                  AR1[ MAX_NB_SUBFR * MAX_SHAPE_LPC_ORDER ];
    silk_float                  AR2[ MAX_NB_SUBFR * MAX_SHAPE_LPC_ORDER ];
    silk_float                  LF_MA_shp[     MAX_NB_SUBFR ];
    silk_float                  LF_AR_shp[     MAX_NB_SUBFR ];
    silk_float                  GainsPre[      MAX_NB_SUBFR ];
    silk_float                  HarmBoost[     MAX_NB_SUBFR ];
    silk_float                  Tilt[          MAX_NB_SUBFR ];
    silk_float                  HarmShapeGain[ MAX_NB_SUBFR ];
    silk_float                  Lambda;
    silk_float                  input_quality;
    silk_float                  coding_quality;

    
    silk_float                  sparseness;
    silk_float                  predGain;
    silk_float                  LTPredCodGain;
    silk_float                  ResNrg[ MAX_NB_SUBFR ];             

    
    opus_int32                  GainsUnq_Q16[ MAX_NB_SUBFR ];
    opus_int8                   lastGainIndexPrev;
} silk_encoder_control_FLP;




typedef struct {
    silk_encoder_state_FLP      state_Fxx[ ENCODER_NUM_CHANNELS ];
    stereo_enc_state            sStereo;
    opus_int32                  nBitsExceeded;
    opus_int                    nChannelsAPI;
    opus_int                    nChannelsInternal;
    opus_int                    nPrevChannelsInternal;
    opus_int                    timeSinceSwitchAllowed_ms;
    opus_int                    allowBandwidthSwitch;
    opus_int                    prev_decode_only_middle;
} silk_encoder;

#ifdef __cplusplus
}
#endif

#endif
