






























#ifndef SILK_STRUCTS_H
#define SILK_STRUCTS_H

#include "typedef.h"
#include "SigProc_FIX.h"
#include "define.h"
#include "entenc.h"
#include "entdec.h"

#ifdef __cplusplus
extern "C"
{
#endif




typedef struct {
    opus_int16                  xq[           2 * MAX_FRAME_LENGTH ]; 
    opus_int32                  sLTP_shp_Q14[ 2 * MAX_FRAME_LENGTH ];
    opus_int32                  sLPC_Q14[ MAX_SUB_FRAME_LENGTH + NSQ_LPC_BUF_LENGTH ];
    opus_int32                  sAR2_Q14[ MAX_SHAPE_LPC_ORDER ];
    opus_int32                  sLF_AR_shp_Q14;
    opus_int                    lagPrev;
    opus_int                    sLTP_buf_idx;
    opus_int                    sLTP_shp_buf_idx;
    opus_int32                  rand_seed;
    opus_int32                  prev_gain_Q16;
    opus_int                    rewhite_flag;
} silk_nsq_state;




typedef struct {
    opus_int32                  AnaState[ 2 ];                  
    opus_int32                  AnaState1[ 2 ];                 
    opus_int32                  AnaState2[ 2 ];                 
    opus_int32                  XnrgSubfr[ VAD_N_BANDS ];       
    opus_int32                  NrgRatioSmth_Q8[ VAD_N_BANDS ]; 
    opus_int16                  HPstate;                        
    opus_int32                  NL[ VAD_N_BANDS ];              
    opus_int32                  inv_NL[ VAD_N_BANDS ];          
    opus_int32                  NoiseLevelBias[ VAD_N_BANDS ];  
    opus_int32                  counter;                        
} silk_VAD_state;


typedef struct {
    opus_int32                   In_LP_State[ 2 ];           
    opus_int32                   transition_frame_no;        
    opus_int                     mode;                       
} silk_LP_state;


typedef struct {
    const opus_int16             nVectors;
    const opus_int16             order;
    const opus_int16             quantStepSize_Q16;
    const opus_int16             invQuantStepSize_Q6;
    const opus_uint8             *CB1_NLSF_Q8;
    const opus_uint8             *CB1_iCDF;
    const opus_uint8             *pred_Q8;
    const opus_uint8             *ec_sel;
    const opus_uint8             *ec_iCDF;
    const opus_uint8             *ec_Rates_Q5;
    const opus_int16             *deltaMin_Q15;
} silk_NLSF_CB_struct;

typedef struct {
    opus_int16                   pred_prev_Q13[ 2 ];
    opus_int16                   sMid[ 2 ];
    opus_int16                   sSide[ 2 ];
    opus_int32                   mid_side_amp_Q0[ 4 ];
    opus_int16                   smth_width_Q14;
    opus_int16                   width_prev_Q14;
    opus_int16                   silent_side_len;
    opus_int8                    predIx[ MAX_FRAMES_PER_PACKET ][ 2 ][ 3 ];
    opus_int8                    mid_only_flags[ MAX_FRAMES_PER_PACKET ];
} stereo_enc_state;

typedef struct {
    opus_int16                   pred_prev_Q13[ 2 ];
    opus_int16                   sMid[ 2 ];
    opus_int16                   sSide[ 2 ];
} stereo_dec_state;

typedef struct {
    opus_int8                    GainsIndices[ MAX_NB_SUBFR ];
    opus_int8                    LTPIndex[ MAX_NB_SUBFR ];
    opus_int8                    NLSFIndices[ MAX_LPC_ORDER + 1 ];
    opus_int16                   lagIndex;
    opus_int8                    contourIndex;
    opus_int8                    signalType;
    opus_int8                    quantOffsetType;
    opus_int8                    NLSFInterpCoef_Q2;
    opus_int8                    PERIndex;
    opus_int8                    LTP_scaleIndex;
    opus_int8                    Seed;
} SideInfoIndices;




typedef struct {
    opus_int32                   In_HP_State[ 2 ];                  
    opus_int32                   variable_HP_smth1_Q15;             
    opus_int32                   variable_HP_smth2_Q15;             
    silk_LP_state                sLP;                               
    silk_VAD_state               sVAD;                              
    silk_nsq_state               sNSQ;                              
    opus_int16                   prev_NLSFq_Q15[ MAX_LPC_ORDER ];   
    opus_int                     speech_activity_Q8;                
    opus_int                     allow_bandwidth_switch;            
    opus_int8                    LBRRprevLastGainIndex;
    opus_int8                    prevSignalType;
    opus_int                     prevLag;
    opus_int                     pitch_LPC_win_length;
    opus_int                     max_pitch_lag;                     
    opus_int32                   API_fs_Hz;                         
    opus_int32                   prev_API_fs_Hz;                    
    opus_int                     maxInternal_fs_Hz;                 
    opus_int                     minInternal_fs_Hz;                 
    opus_int                     desiredInternal_fs_Hz;             
    opus_int                     fs_kHz;                            
    opus_int                     nb_subfr;                          
    opus_int                     frame_length;                      
    opus_int                     subfr_length;                      
    opus_int                     ltp_mem_length;                    
    opus_int                     la_pitch;                          
    opus_int                     la_shape;                          
    opus_int                     shapeWinLength;                    
    opus_int32                   TargetRate_bps;                    
    opus_int                     PacketSize_ms;                     
    opus_int                     PacketLoss_perc;                   
    opus_int32                   frameCounter;
    opus_int                     Complexity;                        
    opus_int                     nStatesDelayedDecision;            
    opus_int                     useInterpolatedNLSFs;              
    opus_int                     shapingLPCOrder;                   
    opus_int                     predictLPCOrder;                   
    opus_int                     pitchEstimationComplexity;         
    opus_int                     pitchEstimationLPCOrder;           
    opus_int32                   pitchEstimationThreshold_Q16;      
    opus_int                     LTPQuantLowComplexity;             
    opus_int                     mu_LTP_Q9;                         
    opus_int                     NLSF_MSVQ_Survivors;               
    opus_int                     first_frame_after_reset;           
    opus_int                     controlled_since_last_payload;     
    opus_int                     warping_Q16;                       
    opus_int                     useCBR;                            
    opus_int                     prefillFlag;                       
    const opus_uint8             *pitch_lag_low_bits_iCDF;          
    const opus_uint8             *pitch_contour_iCDF;               
    const silk_NLSF_CB_struct    *psNLSF_CB;                        
    opus_int                     input_quality_bands_Q15[ VAD_N_BANDS ];
    opus_int                     input_tilt_Q15;
    opus_int                     SNR_dB_Q7;                         

    opus_int8                    VAD_flags[ MAX_FRAMES_PER_PACKET ];
    opus_int8                    LBRR_flag;
    opus_int                     LBRR_flags[ MAX_FRAMES_PER_PACKET ];

    SideInfoIndices              indices;
    opus_int8                    pulses[ MAX_FRAME_LENGTH ];

    
    opus_int16                   inputBuf[ MAX_FRAME_LENGTH + 2 ];  
    opus_int                     inputBufIx;
    opus_int                     nFramesPerPacket;
    opus_int                     nFramesEncoded;                    

    opus_int                     nChannelsAPI;
    opus_int                     nChannelsInternal;
    opus_int                     channelNb;

    
    opus_int                     frames_since_onset;

    
    opus_int                     ec_prevSignalType;
    opus_int16                   ec_prevLagIndex;

    silk_resampler_state_struct resampler_state;

    
    opus_int                     useDTX;                            
    opus_int                     inDTX;                             
    opus_int                     noSpeechCounter;                   

    
    opus_int                     useInBandFEC;                      
    opus_int                     LBRR_enabled;                      
    opus_int                     LBRR_GainIncreases;                
    SideInfoIndices              indices_LBRR[ MAX_FRAMES_PER_PACKET ];
    opus_int8                    pulses_LBRR[ MAX_FRAMES_PER_PACKET ][ MAX_FRAME_LENGTH ];
} silk_encoder_state;



typedef struct {
    opus_int32                  pitchL_Q8;                          
    opus_int16                  LTPCoef_Q14[ LTP_ORDER ];           
    opus_int16                  prevLPC_Q12[ MAX_LPC_ORDER ];
    opus_int                    last_frame_lost;                    
    opus_int32                  rand_seed;                          
    opus_int16                  randScale_Q14;                      
    opus_int32                  conc_energy;
    opus_int                    conc_energy_shift;
    opus_int16                  prevLTP_scale_Q14;
    opus_int32                  prevGain_Q16[ 2 ];
    opus_int                    fs_kHz;
    opus_int                    nb_subfr;
    opus_int                    subfr_length;
} silk_PLC_struct;


typedef struct {
    opus_int32                  CNG_exc_buf_Q14[ MAX_FRAME_LENGTH ];
    opus_int16                  CNG_smth_NLSF_Q15[ MAX_LPC_ORDER ];
    opus_int32                  CNG_synth_state[ MAX_LPC_ORDER ];
    opus_int32                  CNG_smth_Gain_Q16;
    opus_int32                  rand_seed;
    opus_int                    fs_kHz;
} silk_CNG_struct;




typedef struct {
    opus_int32                  prev_gain_Q16;
    opus_int32                  exc_Q14[ MAX_FRAME_LENGTH ];
    opus_int32                  sLPC_Q14_buf[ MAX_LPC_ORDER ];
    opus_int16                  outBuf[ MAX_FRAME_LENGTH + 2 * MAX_SUB_FRAME_LENGTH ];  
    opus_int                    lagPrev;                            
    opus_int8                   LastGainIndex;                      
    opus_int                    fs_kHz;                             
    opus_int32                  fs_API_hz;                          
    opus_int                    nb_subfr;                           
    opus_int                    frame_length;                       
    opus_int                    subfr_length;                       
    opus_int                    ltp_mem_length;                     
    opus_int                    LPC_order;                          
    opus_int16                  prevNLSF_Q15[ MAX_LPC_ORDER ];      
    opus_int                    first_frame_after_reset;            
    const opus_uint8            *pitch_lag_low_bits_iCDF;           
    const opus_uint8            *pitch_contour_iCDF;                

    
    opus_int                    nFramesDecoded;
    opus_int                    nFramesPerPacket;

    
    opus_int                    ec_prevSignalType;
    opus_int16                  ec_prevLagIndex;

    opus_int                    VAD_flags[ MAX_FRAMES_PER_PACKET ];
    opus_int                    LBRR_flag;
    opus_int                    LBRR_flags[ MAX_FRAMES_PER_PACKET ];

    silk_resampler_state_struct resampler_state;

    const silk_NLSF_CB_struct   *psNLSF_CB;                         

    
    SideInfoIndices             indices;

    
    silk_CNG_struct             sCNG;

    
    opus_int                    lossCnt;
    opus_int                    prevSignalType;

    silk_PLC_struct sPLC;

} silk_decoder_state;




typedef struct {
    
    opus_int                    pitchL[ MAX_NB_SUBFR ];
    opus_int32                  Gains_Q16[ MAX_NB_SUBFR ];
    
    silk_DWORD_ALIGN opus_int16 PredCoef_Q12[ 2 ][ MAX_LPC_ORDER ];
    opus_int16                  LTPCoef_Q14[ LTP_ORDER * MAX_NB_SUBFR ];
    opus_int                    LTP_scale_Q14;
} silk_decoder_control;


#ifdef __cplusplus
}
#endif

#endif
