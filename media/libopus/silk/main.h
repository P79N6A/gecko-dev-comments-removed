






























#ifndef SILK_MAIN_H
#define SILK_MAIN_H

#include "SigProc_FIX.h"
#include "define.h"
#include "structs.h"
#include "tables.h"
#include "PLC.h"
#include "control.h"
#include "debug.h"
#include "entenc.h"
#include "entdec.h"


void silk_stereo_LR_to_MS(
    stereo_enc_state            *state,                         
    opus_int16                  x1[],                           
    opus_int16                  x2[],                           
    opus_int8                   ix[ 2 ][ 3 ],                   
    opus_int8                   *mid_only_flag,                 
    opus_int32                  mid_side_rates_bps[],           
    opus_int32                  total_rate_bps,                 
    opus_int                    prev_speech_act_Q8,             
    opus_int                    toMono,                         
    opus_int                    fs_kHz,                         
    opus_int                    frame_length                    
);


void silk_stereo_MS_to_LR(
    stereo_dec_state            *state,                         
    opus_int16                  x1[],                           
    opus_int16                  x2[],                           
    const opus_int32            pred_Q13[],                     
    opus_int                    fs_kHz,                         
    opus_int                    frame_length                    
);


opus_int32 silk_stereo_find_predictor(                          
    opus_int32                  *ratio_Q14,                     
    const opus_int16            x[],                            
    const opus_int16            y[],                            
    opus_int32                  mid_res_amp_Q0[],               
    opus_int                    length,                         
    opus_int                    smooth_coef_Q16                 
);


void silk_stereo_quant_pred(
    opus_int32                  pred_Q13[],                     
    opus_int8                   ix[ 2 ][ 3 ]                    
);


void silk_stereo_encode_pred(
    ec_enc                      *psRangeEnc,                    
    opus_int8                   ix[ 2 ][ 3 ]                    
);


void silk_stereo_encode_mid_only(
    ec_enc                      *psRangeEnc,                    
    opus_int8                   mid_only_flag
);


void silk_stereo_decode_pred(
    ec_dec                      *psRangeDec,                    
    opus_int32                  pred_Q13[]                      
);


void silk_stereo_decode_mid_only(
    ec_dec                      *psRangeDec,                    
    opus_int                    *decode_only_mid                
);


void silk_encode_signs(
    ec_enc                      *psRangeEnc,                        
    const opus_int8             pulses[],                           
    opus_int                    length,                             
    const opus_int              signalType,                         
    const opus_int              quantOffsetType,                    
    const opus_int              sum_pulses[ MAX_NB_SHELL_BLOCKS ]   
);


void silk_decode_signs(
    ec_dec                      *psRangeDec,                        
    opus_int                    pulses[],                           
    opus_int                    length,                             
    const opus_int              signalType,                         
    const opus_int              quantOffsetType,                    
    const opus_int              sum_pulses[ MAX_NB_SHELL_BLOCKS ]   
);


opus_int check_control_input(
    silk_EncControlStruct        *encControl                    
);


opus_int silk_control_audio_bandwidth(
    silk_encoder_state          *psEncC,                        
    silk_EncControlStruct       *encControl                     
);


opus_int silk_control_SNR(
    silk_encoder_state          *psEncC,                        
    opus_int32                  TargetRate_bps                  
);






void silk_encode_pulses(
    ec_enc                      *psRangeEnc,                    
    const opus_int              signalType,                     
    const opus_int              quantOffsetType,                
    opus_int8                   pulses[],                       
    const opus_int              frame_length                    
);


void silk_shell_encoder(
    ec_enc                      *psRangeEnc,                    
    const opus_int              *pulses0                        
);


void silk_shell_decoder(
    opus_int                    *pulses0,                       
    ec_dec                      *psRangeDec,                    
    const opus_int              pulses4                         
);


void silk_gains_quant(
    opus_int8                   ind[ MAX_NB_SUBFR ],            
    opus_int32                  gain_Q16[ MAX_NB_SUBFR ],       
    opus_int8                   *prev_ind,                      
    const opus_int              conditional,                    
    const opus_int              nb_subfr                        
);


void silk_gains_dequant(
    opus_int32                  gain_Q16[ MAX_NB_SUBFR ],       
    const opus_int8             ind[ MAX_NB_SUBFR ],            
    opus_int8                   *prev_ind,                      
    const opus_int              conditional,                    
    const opus_int              nb_subfr                        
);


opus_int32 silk_gains_ID(                                       
    const opus_int8             ind[ MAX_NB_SUBFR ],            
    const opus_int              nb_subfr                        
);


void silk_interpolate(
    opus_int16                  xi[ MAX_LPC_ORDER ],            
    const opus_int16            x0[ MAX_LPC_ORDER ],            
    const opus_int16            x1[ MAX_LPC_ORDER ],            
    const opus_int              ifact_Q2,                       
    const opus_int              d                               
);


void silk_quant_LTP_gains(
    opus_int16                  B_Q14[ MAX_NB_SUBFR * LTP_ORDER ],          
    opus_int8                   cbk_index[ MAX_NB_SUBFR ],                  
    opus_int8                   *periodicity_index,                         
    const opus_int32            W_Q18[ MAX_NB_SUBFR*LTP_ORDER*LTP_ORDER ],  
    opus_int                    mu_Q9,                                      
    opus_int                    lowComplexity,                              
    const opus_int              nb_subfr                                    
);


void silk_VQ_WMat_EC(
    opus_int8                   *ind,                           
    opus_int32                  *rate_dist_Q14,                 
    const opus_int16            *in_Q14,                        
    const opus_int32            *W_Q18,                         
    const opus_int8             *cb_Q7,                         
    const opus_uint8            *cl_Q5,                         
    const opus_int              mu_Q9,                          
    opus_int                    L                               
);




void silk_NSQ(
    const silk_encoder_state    *psEncC,                                    
    silk_nsq_state              *NSQ,                                       
    SideInfoIndices             *psIndices,                                 
    const opus_int32            x_Q3[],                                     
    opus_int8                   pulses[],                                   
    const opus_int16            PredCoef_Q12[ 2 * MAX_LPC_ORDER ],          
    const opus_int16            LTPCoef_Q14[ LTP_ORDER * MAX_NB_SUBFR ],    
    const opus_int16            AR2_Q13[ MAX_NB_SUBFR * MAX_SHAPE_LPC_ORDER ], 
    const opus_int              HarmShapeGain_Q14[ MAX_NB_SUBFR ],          
    const opus_int              Tilt_Q14[ MAX_NB_SUBFR ],                   
    const opus_int32            LF_shp_Q14[ MAX_NB_SUBFR ],                 
    const opus_int32            Gains_Q16[ MAX_NB_SUBFR ],                  
    const opus_int              pitchL[ MAX_NB_SUBFR ],                     
    const opus_int              Lambda_Q10,                                 
    const opus_int              LTP_scale_Q14                               
);


void silk_NSQ_del_dec(
    const silk_encoder_state    *psEncC,                                    
    silk_nsq_state              *NSQ,                                       
    SideInfoIndices             *psIndices,                                 
    const opus_int32            x_Q3[],                                     
    opus_int8                   pulses[],                                   
    const opus_int16            PredCoef_Q12[ 2 * MAX_LPC_ORDER ],          
    const opus_int16            LTPCoef_Q14[ LTP_ORDER * MAX_NB_SUBFR ],    
    const opus_int16            AR2_Q13[ MAX_NB_SUBFR * MAX_SHAPE_LPC_ORDER ], 
    const opus_int              HarmShapeGain_Q14[ MAX_NB_SUBFR ],          
    const opus_int              Tilt_Q14[ MAX_NB_SUBFR ],                   
    const opus_int32            LF_shp_Q14[ MAX_NB_SUBFR ],                 
    const opus_int32            Gains_Q16[ MAX_NB_SUBFR ],                  
    const opus_int              pitchL[ MAX_NB_SUBFR ],                     
    const opus_int              Lambda_Q10,                                 
    const opus_int              LTP_scale_Q14                               
);





opus_int silk_VAD_Init(                                         
    silk_VAD_state              *psSilk_VAD                     
);


opus_int silk_VAD_GetSA_Q8(                                     
    silk_encoder_state          *psEncC,                        
    const opus_int16            pIn[]                           
);




void silk_LP_variable_cutoff(
    silk_LP_state               *psLP,                          
    opus_int16                  *frame,                         
    const opus_int              frame_length                    
);





void silk_process_NLSFs(
    silk_encoder_state          *psEncC,                            
    opus_int16                  PredCoef_Q12[ 2 ][ MAX_LPC_ORDER ], 
    opus_int16                  pNLSF_Q15[         MAX_LPC_ORDER ], 
    const opus_int16            prev_NLSFq_Q15[    MAX_LPC_ORDER ]  
);

opus_int32 silk_NLSF_encode(                                    
          opus_int8             *NLSFIndices,                   
          opus_int16            *pNLSF_Q15,                     
    const silk_NLSF_CB_struct   *psNLSF_CB,                     
    const opus_int16            *pW_QW,                         
    const opus_int              NLSF_mu_Q20,                    
    const opus_int              nSurvivors,                     
    const opus_int              signalType                      
);


void silk_NLSF_VQ(
    opus_int32                  err_Q26[],                      
    const opus_int16            in_Q15[],                       
    const opus_uint8            pCB_Q8[],                       
    const opus_int              K,                              
    const opus_int              LPC_order                       
);


opus_int32 silk_NLSF_del_dec_quant(                             
    opus_int8                   indices[],                      
    const opus_int16            x_Q10[],                        
    const opus_int16            w_Q5[],                         
    const opus_uint8            pred_coef_Q8[],                 
    const opus_int16            ec_ix[],                        
    const opus_uint8            ec_rates_Q5[],                  
    const opus_int              quant_step_size_Q16,            
    const opus_int16            inv_quant_step_size_Q6,         
    const opus_int32            mu_Q20,                         
    const opus_int16            order                           
);


void silk_NLSF_unpack(
          opus_int16            ec_ix[],                        
          opus_uint8            pred_Q8[],                      
    const silk_NLSF_CB_struct   *psNLSF_CB,                     
    const opus_int              CB1_index                       
);




void silk_NLSF_decode(
          opus_int16            *pNLSF_Q15,                     
          opus_int8             *NLSFIndices,                   
    const silk_NLSF_CB_struct   *psNLSF_CB                      
);




opus_int silk_init_decoder(
    silk_decoder_state          *psDec                          
);


opus_int silk_decoder_set_fs(
    silk_decoder_state          *psDec,                         
    opus_int                    fs_kHz,                         
    opus_int                    fs_API_Hz                       
);




opus_int silk_decode_frame(
    silk_decoder_state          *psDec,                         
    ec_dec                      *psRangeDec,                    
    opus_int16                  pOut[],                         
    opus_int32                  *pN,                            
    opus_int                    lostFlag,                       
    opus_int                    condCoding                      
);


void silk_decode_indices(
    silk_decoder_state          *psDec,                         
    ec_dec                      *psRangeDec,                    
    opus_int                    FrameIndex,                     
    opus_int                    decode_LBRR,                    
    opus_int                    condCoding                      
);


void silk_decode_parameters(
    silk_decoder_state          *psDec,                         
    silk_decoder_control        *psDecCtrl,                     
    opus_int                    condCoding                      
);


void silk_decode_core(
    silk_decoder_state          *psDec,                         
    silk_decoder_control        *psDecCtrl,                     
    opus_int16                  xq[],                           
    const opus_int              pulses[ MAX_FRAME_LENGTH ]      
);


void silk_decode_pulses(
    ec_dec                      *psRangeDec,                    
    opus_int                    pulses[],                       
    const opus_int              signalType,                     
    const opus_int              quantOffsetType,                
    const opus_int              frame_length                    
);






void silk_CNG_Reset(
    silk_decoder_state          *psDec                          
);


void silk_CNG(
    silk_decoder_state          *psDec,                         
    silk_decoder_control        *psDecCtrl,                     
    opus_int16                  frame[],                        
    opus_int                    length                          
);


void silk_encode_indices(
    silk_encoder_state          *psEncC,                        
    ec_enc                      *psRangeEnc,                    
    opus_int                    FrameIndex,                     
    opus_int                    encode_LBRR,                    
    opus_int                    condCoding                      
);

#endif
