






























#ifndef SILK_DEFINE_H
#define SILK_DEFINE_H

#include "errors.h"
#include "typedef.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define ENCODER_NUM_CHANNELS                    2

#define DECODER_NUM_CHANNELS                    2

#define MAX_FRAMES_PER_PACKET                   3


#define MIN_TARGET_RATE_BPS                     5000
#define MAX_TARGET_RATE_BPS                     80000
#define TARGET_RATE_TAB_SZ                      8


#define LBRR_NB_MIN_RATE_BPS                    12000
#define LBRR_MB_MIN_RATE_BPS                    14000
#define LBRR_WB_MIN_RATE_BPS                    16000


#define NB_SPEECH_FRAMES_BEFORE_DTX             10      /* eq 200 ms */
#define MAX_CONSECUTIVE_DTX                     20      /* eq 400 ms */


#define MAX_FS_KHZ                              16
#define MAX_API_FS_KHZ                          48


#define TYPE_NO_VOICE_ACTIVITY                  0
#define TYPE_UNVOICED                           1
#define TYPE_VOICED                             2


#define CODE_INDEPENDENTLY                      0
#define CODE_INDEPENDENTLY_NO_LTP_SCALING       1
#define CODE_CONDITIONALLY                      2


#define STEREO_QUANT_TAB_SIZE                   16
#define STEREO_QUANT_SUB_STEPS                  5
#define STEREO_INTERP_LEN_MS                    8       /* must be even */
#define STEREO_RATIO_SMOOTH_COEF                0.01    /* smoothing coef for signal norms and stereo width */


#define PITCH_EST_MIN_LAG_MS                    2       /* 2 ms -> 500 Hz */
#define PITCH_EST_MAX_LAG_MS                    18      /* 18 ms -> 56 Hz */


#define MAX_NB_SUBFR                            4


#define LTP_MEM_LENGTH_MS                       20
#define SUB_FRAME_LENGTH_MS                     5
#define MAX_SUB_FRAME_LENGTH                    ( SUB_FRAME_LENGTH_MS * MAX_FS_KHZ )
#define MAX_FRAME_LENGTH_MS                     ( SUB_FRAME_LENGTH_MS * MAX_NB_SUBFR )
#define MAX_FRAME_LENGTH                        ( MAX_FRAME_LENGTH_MS * MAX_FS_KHZ )


#define LA_PITCH_MS                             2
#define LA_PITCH_MAX                            ( LA_PITCH_MS * MAX_FS_KHZ )


#define MAX_FIND_PITCH_LPC_ORDER                16


#define FIND_PITCH_LPC_WIN_MS                   ( 20 + (LA_PITCH_MS << 1) )
#define FIND_PITCH_LPC_WIN_MS_2_SF              ( 10 + (LA_PITCH_MS << 1) )
#define FIND_PITCH_LPC_WIN_MAX                  ( FIND_PITCH_LPC_WIN_MS * MAX_FS_KHZ )


#define LA_SHAPE_MS                             5
#define LA_SHAPE_MAX                            ( LA_SHAPE_MS * MAX_FS_KHZ )


#define SHAPE_LPC_WIN_MAX                       ( 15 * MAX_FS_KHZ )


#define MIN_QGAIN_DB                            2

#define MAX_QGAIN_DB                            88

#define N_LEVELS_QGAIN                          64

#define MAX_DELTA_GAIN_QUANT                    36

#define MIN_DELTA_GAIN_QUANT                    -4


#define OFFSET_VL_Q10                           32
#define OFFSET_VH_Q10                           100
#define OFFSET_UVL_Q10                          100
#define OFFSET_UVH_Q10                          240

#define QUANT_LEVEL_ADJUST_Q10                  80


#define MAX_LPC_STABILIZE_ITERATIONS            16
#define MAX_PREDICTION_POWER_GAIN               1e4f
#define MAX_PREDICTION_POWER_GAIN_AFTER_RESET   1e2f

#define MAX_LPC_ORDER                           16
#define MIN_LPC_ORDER                           10


#define LTP_ORDER                               5


#define NB_LTP_CBKS                             3


#define USE_HARM_SHAPING                        1


#define MAX_SHAPE_LPC_ORDER                     16

#define HARM_SHAPE_FIR_TAPS                     3


#define MAX_DEL_DEC_STATES                      4

#define LTP_BUF_LENGTH                          512
#define LTP_MASK                                ( LTP_BUF_LENGTH - 1 )

#define DECISION_DELAY                          32
#define DECISION_DELAY_MASK                     ( DECISION_DELAY - 1 )


#define SHELL_CODEC_FRAME_LENGTH                16
#define LOG2_SHELL_CODEC_FRAME_LENGTH           4
#define MAX_NB_SHELL_BLOCKS                     ( MAX_FRAME_LENGTH / SHELL_CODEC_FRAME_LENGTH )


#define N_RATE_LEVELS                           10


#define MAX_PULSES                              16

#define MAX_MATRIX_SIZE                         MAX_LPC_ORDER /* Max of LPC Order and LTP order */

#if( MAX_LPC_ORDER > DECISION_DELAY )
# define NSQ_LPC_BUF_LENGTH                     MAX_LPC_ORDER
#else
# define NSQ_LPC_BUF_LENGTH                     DECISION_DELAY
#endif




#define HIGH_PASS_INPUT                         1




#define VAD_N_BANDS                             4

#define VAD_INTERNAL_SUBFRAMES_LOG2             2
#define VAD_INTERNAL_SUBFRAMES                  ( 1 << VAD_INTERNAL_SUBFRAMES_LOG2 )

#define VAD_NOISE_LEVEL_SMOOTH_COEF_Q16         1024    /* Must be <  4096 */
#define VAD_NOISE_LEVELS_BIAS                   50


#define VAD_NEGATIVE_OFFSET_Q5                  128     /* sigmoid is 0 at -128 */
#define VAD_SNR_FACTOR_Q16                      45000


#define VAD_SNR_SMOOTH_COEF_Q18                 4096


#define LSF_COS_TAB_SZ_FIX                      128




#define NLSF_W_Q                                2
#define NLSF_VQ_MAX_VECTORS                     32
#define NLSF_VQ_MAX_SURVIVORS                   32
#define NLSF_QUANT_MAX_AMPLITUDE                4
#define NLSF_QUANT_MAX_AMPLITUDE_EXT            10
#define NLSF_QUANT_LEVEL_ADJ                    0.1
#define NLSF_QUANT_DEL_DEC_STATES_LOG2          2
#define NLSF_QUANT_DEL_DEC_STATES               ( 1 << NLSF_QUANT_DEL_DEC_STATES_LOG2 )


#define TRANSITION_TIME_MS                      5120    /* 5120 = 64 * FRAME_LENGTH_MS * ( TRANSITION_INT_NUM - 1 ) = 64*(20*4)*/
#define TRANSITION_NB                           3       /* Hardcoded in tables */
#define TRANSITION_NA                           2       /* Hardcoded in tables */
#define TRANSITION_INT_NUM                      5       /* Hardcoded in tables */
#define TRANSITION_FRAMES                       ( TRANSITION_TIME_MS / MAX_FRAME_LENGTH_MS )
#define TRANSITION_INT_STEPS                    ( TRANSITION_FRAMES  / ( TRANSITION_INT_NUM - 1 ) )


#define BWE_AFTER_LOSS_Q16                      63570


#define CNG_BUF_MASK_MAX                        255     /* 2^floor(log2(MAX_FRAME_LENGTH))-1    */
#define CNG_GAIN_SMTH_Q16                       4634    /* 0.25^(1/4)                           */
#define CNG_NLSF_SMTH_Q16                       16348   /* 0.25                                 */

#ifdef __cplusplus
}
#endif

#endif
