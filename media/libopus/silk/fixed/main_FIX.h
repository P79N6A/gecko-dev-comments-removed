






























#ifndef SILK_MAIN_FIX_H
#define SILK_MAIN_FIX_H

#include "SigProc_FIX.h"
#include "structs_FIX.h"
#include "control.h"
#include "main.h"
#include "PLC.h"
#include "debug.h"
#include "entenc.h"

#ifndef FORCE_CPP_BUILD
#ifdef __cplusplus
extern "C"
{
#endif
#endif

#define silk_encoder_state_Fxx      silk_encoder_state_FIX
#define silk_encode_do_VAD_Fxx      silk_encode_do_VAD_FIX
#define silk_encode_frame_Fxx       silk_encode_frame_FIX






void silk_HP_variable_cutoff(
    silk_encoder_state_Fxx          state_Fxx[]                             
);


void silk_encode_do_VAD_FIX(
    silk_encoder_state_FIX          *psEnc                                  
);


opus_int silk_encode_frame_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    opus_int32                      *pnBytesOut,                            
    ec_enc                          *psRangeEnc,                            
    opus_int                        condCoding,                             
    opus_int                        maxBits,                                
    opus_int                        useCBR                                  
);


opus_int silk_init_encoder(
    silk_encoder_state_Fxx          *psEnc                                  
);


opus_int silk_control_encoder(
    silk_encoder_state_Fxx          *psEnc,                                 
    silk_EncControlStruct           *encControl,                            
    const opus_int32                TargetRate_bps,                         
    const opus_int                  allow_bw_switch,                        
    const opus_int                  channelNb,                              
    const opus_int                  force_fs_kHz
);




void silk_prefilter_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    const silk_encoder_control_FIX  *psEncCtrl,                             
    opus_int32                      xw_Q10[],                               
    const opus_int16                x[]                                     
);





void silk_noise_shape_analysis_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    silk_encoder_control_FIX        *psEncCtrl,                             
    const opus_int16                *pitch_res,                             
    const opus_int16                *x                                      
);


void silk_warped_autocorrelation_FIX(
          opus_int32                *corr,                                  
          opus_int                  *scale,                                 
    const opus_int16                *input,                                 
    const opus_int                  warping_Q16,                            
    const opus_int                  length,                                 
    const opus_int                  order                                   
);


void silk_LTP_scale_ctrl_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    silk_encoder_control_FIX        *psEncCtrl,                             
    opus_int                        condCoding                              
);





void silk_find_pitch_lags_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    silk_encoder_control_FIX        *psEncCtrl,                             
    opus_int16                      res[],                                  
    const opus_int16                x[]                                     
);


void silk_find_pred_coefs_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    silk_encoder_control_FIX        *psEncCtrl,                             
    const opus_int16                res_pitch[],                            
    const opus_int16                x[],                                    
    opus_int                        condCoding                              
);


void silk_find_LPC_FIX(
    silk_encoder_state              *psEncC,                                
    opus_int16                      NLSF_Q15[],                             
    const opus_int16                x[],                                    
    const opus_int32                minInvGain_Q30                          
);


void silk_find_LTP_FIX(
    opus_int16                      b_Q14[ MAX_NB_SUBFR * LTP_ORDER ],      
    opus_int32                      WLTP[ MAX_NB_SUBFR * LTP_ORDER * LTP_ORDER ], 
    opus_int                        *LTPredCodGain_Q7,                      
    const opus_int16                r_lpc[],                                
    const opus_int                  lag[ MAX_NB_SUBFR ],                    
    const opus_int32                Wght_Q15[ MAX_NB_SUBFR ],               
    const opus_int                  subfr_length,                           
    const opus_int                  nb_subfr,                               
    const opus_int                  mem_offset,                             
    opus_int                        corr_rshifts[ MAX_NB_SUBFR ]            
);

void silk_LTP_analysis_filter_FIX(
    opus_int16                      *LTP_res,                               
    const opus_int16                *x,                                     
    const opus_int16                LTPCoef_Q14[ LTP_ORDER * MAX_NB_SUBFR ],
    const opus_int                  pitchL[ MAX_NB_SUBFR ],                 
    const opus_int32                invGains_Q16[ MAX_NB_SUBFR ],           
    const opus_int                  subfr_length,                           
    const opus_int                  nb_subfr,                               
    const opus_int                  pre_length                              
);



void silk_residual_energy_FIX(
          opus_int32                nrgs[ MAX_NB_SUBFR ],                   
          opus_int                  nrgsQ[ MAX_NB_SUBFR ],                  
    const opus_int16                x[],                                    
          opus_int16                a_Q12[ 2 ][ MAX_LPC_ORDER ],            
    const opus_int32                gains[ MAX_NB_SUBFR ],                  
    const opus_int                  subfr_length,                           
    const opus_int                  nb_subfr,                               
    const opus_int                  LPC_order                               
);


opus_int32 silk_residual_energy16_covar_FIX(
    const opus_int16                *c,                                     
    const opus_int32                *wXX,                                   
    const opus_int32                *wXx,                                   
    opus_int32                      wxx,                                    
    opus_int                        D,                                      
    opus_int                        cQ                                      
);


void silk_process_gains_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    silk_encoder_control_FIX        *psEncCtrl,                             
    opus_int                        condCoding                              
);





void silk_corrMatrix_FIX(
    const opus_int16                *x,                                     
    const opus_int                  L,                                      
    const opus_int                  order,                                  
    const opus_int                  head_room,                              
    opus_int32                      *XX,                                    
    opus_int                        *rshifts                                
);


void silk_corrVector_FIX(
    const opus_int16                *x,                                     
    const opus_int16                *t,                                     
    const opus_int                  L,                                      
    const opus_int                  order,                                  
    opus_int32                      *Xt,                                    
    const opus_int                  rshifts                                 
);


void silk_regularize_correlations_FIX(
    opus_int32                      *XX,                                    
    opus_int32                      *xx,                                    
    opus_int32                      noise,                                  
    opus_int                        D                                       
);


void silk_solve_LDL_FIX(
    opus_int32                      *A,                                     
    opus_int                        M,                                      
    const opus_int32                *b,                                     
    opus_int32                      *x_Q16                                  
);

#ifndef FORCE_CPP_BUILD
#ifdef __cplusplus
}
#endif 
#endif 
#endif
