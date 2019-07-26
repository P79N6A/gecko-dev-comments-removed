






























#ifndef SILK_TABLES_H
#define SILK_TABLES_H

#include "define.h"
#include "structs.h"

#ifdef __cplusplus
extern "C"
{
#endif


extern const opus_uint8  silk_gain_iCDF[ 3 ][ N_LEVELS_QGAIN / 8 ];                                 
extern const opus_uint8  silk_delta_gain_iCDF[ MAX_DELTA_GAIN_QUANT - MIN_DELTA_GAIN_QUANT + 1 ];   

extern const opus_uint8  silk_pitch_lag_iCDF[ 2 * ( PITCH_EST_MAX_LAG_MS - PITCH_EST_MIN_LAG_MS ) ];
extern const opus_uint8  silk_pitch_delta_iCDF[ 21 ];                                               
extern const opus_uint8  silk_pitch_contour_iCDF[ 34 ];                                             
extern const opus_uint8  silk_pitch_contour_NB_iCDF[ 11 ];                                          
extern const opus_uint8  silk_pitch_contour_10_ms_iCDF[ 12 ];                                       
extern const opus_uint8  silk_pitch_contour_10_ms_NB_iCDF[ 3 ];                                     

extern const opus_uint8  silk_pulses_per_block_iCDF[ N_RATE_LEVELS ][ MAX_PULSES + 2 ];             
extern const opus_uint8  silk_pulses_per_block_BITS_Q5[ N_RATE_LEVELS - 1 ][ MAX_PULSES + 2 ];      

extern const opus_uint8  silk_rate_levels_iCDF[ 2 ][ N_RATE_LEVELS - 1 ];                           
extern const opus_uint8  silk_rate_levels_BITS_Q5[ 2 ][ N_RATE_LEVELS - 1 ];                        

extern const opus_uint8  silk_max_pulses_table[ 4 ];                                                

extern const opus_uint8  silk_shell_code_table0[ 152 ];                                             
extern const opus_uint8  silk_shell_code_table1[ 152 ];                                             
extern const opus_uint8  silk_shell_code_table2[ 152 ];                                             
extern const opus_uint8  silk_shell_code_table3[ 152 ];                                             
extern const opus_uint8  silk_shell_code_table_offsets[ MAX_PULSES + 1 ];                           

extern const opus_uint8  silk_lsb_iCDF[ 2 ];                                                        

extern const opus_uint8  silk_sign_iCDF[ 42 ];                                                      

extern const opus_uint8  silk_uniform3_iCDF[ 3 ];                                                   
extern const opus_uint8  silk_uniform4_iCDF[ 4 ];                                                   
extern const opus_uint8  silk_uniform5_iCDF[ 5 ];                                                   
extern const opus_uint8  silk_uniform6_iCDF[ 6 ];                                                   
extern const opus_uint8  silk_uniform8_iCDF[ 8 ];                                                   

extern const opus_uint8  silk_NLSF_EXT_iCDF[ 7 ];                                                   

extern const opus_uint8  silk_LTP_per_index_iCDF[ 3 ];                                              
extern const opus_uint8  * const silk_LTP_gain_iCDF_ptrs[ NB_LTP_CBKS ];                            
extern const opus_uint8  * const silk_LTP_gain_BITS_Q5_ptrs[ NB_LTP_CBKS ];                         
extern const opus_int16  silk_LTP_gain_middle_avg_RD_Q14;
extern const opus_int8   * const silk_LTP_vq_ptrs_Q7[ NB_LTP_CBKS ];                                
extern const opus_int8   silk_LTP_vq_sizes[ NB_LTP_CBKS ];                                          

extern const opus_uint8  silk_LTPscale_iCDF[ 3 ];                                                   
extern const opus_int16  silk_LTPScales_table_Q14[ 3 ];                                             

extern const opus_uint8  silk_type_offset_VAD_iCDF[ 4 ];                                            
extern const opus_uint8  silk_type_offset_no_VAD_iCDF[ 2 ];                                         

extern const opus_int16  silk_stereo_pred_quant_Q13[ STEREO_QUANT_TAB_SIZE ];                       
extern const opus_uint8  silk_stereo_pred_joint_iCDF[ 25 ];                                         
extern const opus_uint8  silk_stereo_only_code_mid_iCDF[ 2 ];                                       

extern const opus_uint8  * const silk_LBRR_flags_iCDF_ptr[ 2 ];                                     

extern const opus_uint8  silk_NLSF_interpolation_factor_iCDF[ 5 ];                                  

extern const silk_NLSF_CB_struct silk_NLSF_CB_WB;                                                   
extern const silk_NLSF_CB_struct silk_NLSF_CB_NB_MB;                                                


extern const opus_int32  silk_TargetRate_table_NB[  TARGET_RATE_TAB_SZ ];                           
extern const opus_int32  silk_TargetRate_table_MB[  TARGET_RATE_TAB_SZ ];                           
extern const opus_int32  silk_TargetRate_table_WB[  TARGET_RATE_TAB_SZ ];                           
extern const opus_int16  silk_SNR_table_Q1[         TARGET_RATE_TAB_SZ ];                           


extern const opus_int16  silk_Quantization_Offsets_Q10[ 2 ][ 2 ];                                   


extern const opus_int32  silk_Transition_LP_B_Q28[ TRANSITION_INT_NUM ][ TRANSITION_NB ];           
extern const opus_int32  silk_Transition_LP_A_Q28[ TRANSITION_INT_NUM ][ TRANSITION_NA ];           


extern const opus_int16  silk_LSFCosTab_FIX_Q12[ LSF_COS_TAB_SZ_FIX + 1 ];                          

#ifdef __cplusplus
}
#endif

#endif
