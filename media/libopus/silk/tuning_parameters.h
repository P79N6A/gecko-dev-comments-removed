






























#ifndef SILK_TUNING_PARAMETERS_H
#define SILK_TUNING_PARAMETERS_H

#ifdef __cplusplus
extern "C"
{
#endif


#define BITRESERVOIR_DECAY_TIME_MS                      500






#define FIND_PITCH_WHITE_NOISE_FRACTION                 1e-3f


#define FIND_PITCH_BANDWITH_EXPANSION                   0.99f






#define FIND_LPC_COND_FAC                               1e-5f


#define FIND_LTP_COND_FAC                               1e-5f
#define LTP_DAMPING                                     0.05f
#define LTP_SMOOTHING                                   0.1f


#define MU_LTP_QUANT_NB                                 0.03f
#define MU_LTP_QUANT_MB                                 0.025f
#define MU_LTP_QUANT_WB                                 0.02f






#define VARIABLE_HP_SMTH_COEF1                          0.1f
#define VARIABLE_HP_SMTH_COEF2                          0.015f
#define VARIABLE_HP_MAX_DELTA_FREQ                      0.4f


#define VARIABLE_HP_MIN_CUTOFF_HZ                       60
#define VARIABLE_HP_MAX_CUTOFF_HZ                       100






#define SPEECH_ACTIVITY_DTX_THRES                       0.05f


#define LBRR_SPEECH_ACTIVITY_THRES                      0.3f






#define BG_SNR_DECR_dB                                  2.0f


#define HARM_SNR_INCR_dB                                2.0f


#define SPARSE_SNR_INCR_dB                              2.0f


#define SPARSENESS_THRESHOLD_QNT_OFFSET                 0.75f


#define WARPING_MULTIPLIER                              0.015f


#define SHAPE_WHITE_NOISE_FRACTION                      5e-5f


#define BANDWIDTH_EXPANSION                             0.95f


#define LOW_RATE_BANDWIDTH_EXPANSION_DELTA              0.01f


#define LOW_RATE_HARMONIC_BOOST                         0.1f


#define LOW_INPUT_QUALITY_HARMONIC_BOOST                0.1f


#define HARMONIC_SHAPING                                0.3f


#define HIGH_RATE_OR_LOW_QUALITY_HARMONIC_SHAPING       0.2f


#define HP_NOISE_COEF                                   0.25f


#define HARM_HP_NOISE_COEF                              0.35f


#define INPUT_TILT                                      0.05f


#define HIGH_RATE_INPUT_TILT                            0.1f


#define LOW_FREQ_SHAPING                                4.0f


#define LOW_QUALITY_LOW_FREQ_SHAPING_DECR               0.5f


#define SUBFR_SMTH_COEF                                 0.4f


#define LAMBDA_OFFSET                                   1.2f
#define LAMBDA_SPEECH_ACT                               -0.2f
#define LAMBDA_DELAYED_DECISIONS                        -0.05f
#define LAMBDA_INPUT_QUALITY                            -0.1f
#define LAMBDA_CODING_QUALITY                           -0.2f
#define LAMBDA_QUANT_OFFSET                             0.8f


#define REDUCE_BITRATE_10_MS_BPS                        2200


#define MAX_BANDWIDTH_SWITCH_DELAY_MS                   5000

#ifdef __cplusplus
}
#endif

#endif
