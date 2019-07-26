






























#ifndef SILK_MAIN_FLP_H
#define SILK_MAIN_FLP_H

#include "SigProc_FLP.h"
#include "SigProc_FIX.h"
#include "structs_FLP.h"
#include "main.h"
#include "define.h"
#include "debug.h"
#include "entenc.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define silk_encoder_state_Fxx      silk_encoder_state_FLP
#define silk_encode_do_VAD_Fxx      silk_encode_do_VAD_FLP
#define silk_encode_frame_Fxx       silk_encode_frame_FLP






void silk_HP_variable_cutoff(
    silk_encoder_state_Fxx          state_Fxx[]                         
);


void silk_encode_do_VAD_FLP(
    silk_encoder_state_FLP          *psEnc                              
);


opus_int silk_encode_frame_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    opus_int32                      *pnBytesOut,                        
    ec_enc                          *psRangeEnc,                        
    opus_int                        condCoding,                         
    opus_int                        maxBits,                            
    opus_int                        useCBR                              
);


opus_int silk_init_encoder(
    silk_encoder_state_FLP          *psEnc                              
);


opus_int silk_control_encoder(
    silk_encoder_state_FLP          *psEnc,                             
    silk_EncControlStruct           *encControl,                        
    const opus_int32                TargetRate_bps,                     
    const opus_int                  allow_bw_switch,                    
    const opus_int                  channelNb,                          
    const opus_int                  force_fs_kHz
);




void silk_prefilter_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    const silk_encoder_control_FLP  *psEncCtrl,                         
    silk_float                      xw[],                               
    const silk_float                x[]                                 
);





void silk_noise_shape_analysis_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    silk_encoder_control_FLP        *psEncCtrl,                         
    const silk_float                *pitch_res,                         
    const silk_float                *x                                  
);


void silk_warped_autocorrelation_FLP(
    silk_float                      *corr,                              
    const silk_float                *input,                             
    const silk_float                warping,                            
    const opus_int                  length,                             
    const opus_int                  order                               
);


void silk_LTP_scale_ctrl_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    silk_encoder_control_FLP        *psEncCtrl,                         
    opus_int                        condCoding                          
);





void silk_find_pitch_lags_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    silk_encoder_control_FLP        *psEncCtrl,                         
    silk_float                      res[],                              
    const silk_float                x[]                                 
);


void silk_find_pred_coefs_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    silk_encoder_control_FLP        *psEncCtrl,                         
    const silk_float                res_pitch[],                        
    const silk_float                x[],                                
    opus_int                        condCoding                          
);


void silk_find_LPC_FLP(
    silk_encoder_state              *psEncC,                            
    opus_int16                      NLSF_Q15[],                         
    const silk_float                x[],                                
    const silk_float                minInvGain                          
);


void silk_find_LTP_FLP(
    silk_float                      b[ MAX_NB_SUBFR * LTP_ORDER ],      
    silk_float                      WLTP[ MAX_NB_SUBFR * LTP_ORDER * LTP_ORDER ], 
    silk_float                      *LTPredCodGain,                     
    const silk_float                r_lpc[],                            
    const opus_int                  lag[  MAX_NB_SUBFR ],               
    const silk_float                Wght[ MAX_NB_SUBFR ],               
    const opus_int                  subfr_length,                       
    const opus_int                  nb_subfr,                           
    const opus_int                  mem_offset                          
);

void silk_LTP_analysis_filter_FLP(
    silk_float                      *LTP_res,                           
    const silk_float                *x,                                 
    const silk_float                B[ LTP_ORDER * MAX_NB_SUBFR ],      
    const opus_int                  pitchL[   MAX_NB_SUBFR ],           
    const silk_float                invGains[ MAX_NB_SUBFR ],           
    const opus_int                  subfr_length,                       
    const opus_int                  nb_subfr,                           
    const opus_int                  pre_length                          
);



void silk_residual_energy_FLP(
    silk_float                      nrgs[ MAX_NB_SUBFR ],               
    const silk_float                x[],                                
    silk_float                      a[ 2 ][ MAX_LPC_ORDER ],            
    const silk_float                gains[],                            
    const opus_int                  subfr_length,                       
    const opus_int                  nb_subfr,                           
    const opus_int                  LPC_order                           
);


void silk_LPC_analysis_filter_FLP(
    silk_float                      r_LPC[],                            
    const silk_float                PredCoef[],                         
    const silk_float                s[],                                
    const opus_int                  length,                             
    const opus_int                  Order                               
);


void silk_quant_LTP_gains_FLP(
    silk_float                      B[ MAX_NB_SUBFR * LTP_ORDER ],      
    opus_int8                       cbk_index[ MAX_NB_SUBFR ],          
    opus_int8                       *periodicity_index,                 
    const silk_float                W[ MAX_NB_SUBFR * LTP_ORDER * LTP_ORDER ], 
    const opus_int                  mu_Q10,                             
    const opus_int                  lowComplexity,                      
    const opus_int                  nb_subfr                            
);


silk_float silk_residual_energy_covar_FLP(                              
    const silk_float                *c,                                 
    silk_float                      *wXX,                               
    const silk_float                *wXx,                               
    const silk_float                wxx,                                
    const opus_int                  D                                   
);


void silk_process_gains_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    silk_encoder_control_FLP        *psEncCtrl,                         
    opus_int                        condCoding                          
);





void silk_corrMatrix_FLP(
    const silk_float                *x,                                 
    const opus_int                  L,                                  
    const opus_int                  Order,                              
    silk_float                      *XX                                 
);


void silk_corrVector_FLP(
    const silk_float                *x,                                 
    const silk_float                *t,                                 
    const opus_int                  L,                                  
    const opus_int                  Order,                              
    silk_float                      *Xt                                 
);


void silk_regularize_correlations_FLP(
    silk_float                      *XX,                                
    silk_float                      *xx,                                
    const silk_float                noise,                              
    const opus_int                  D                                   
);


void silk_solve_LDL_FLP(
    silk_float                      *A,                                 
    const opus_int                  M,                                  
    const silk_float                *b,                                 
    silk_float                      *x                                  
);





void silk_apply_sine_window_FLP(
    silk_float                      px_win[],                           
    const silk_float                px[],                               
    const opus_int                  win_type,                           
    const opus_int                  length                              
);




void silk_A2NLSF_FLP(
    opus_int16                      *NLSF_Q15,                          
    const silk_float                *pAR,                               
    const opus_int                  LPC_order                           
);


void silk_NLSF2A_FLP(
    silk_float                      *pAR,                               
    const opus_int16                *NLSF_Q15,                          
    const opus_int                  LPC_order                           
);


void silk_process_NLSFs_FLP(
    silk_encoder_state              *psEncC,                            
    silk_float                      PredCoef[ 2 ][ MAX_LPC_ORDER ],     
    opus_int16                      NLSF_Q15[      MAX_LPC_ORDER ],     
    const opus_int16                prev_NLSF_Q15[ MAX_LPC_ORDER ]      
);


void silk_NSQ_wrapper_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    silk_encoder_control_FLP        *psEncCtrl,                         
    SideInfoIndices                 *psIndices,                         
    silk_nsq_state                  *psNSQ,                             
    opus_int8                       pulses[],                           
    const silk_float                x[]                                 
);

#ifdef __cplusplus
}
#endif

#endif
