
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_LPC_MASKING_MODEL_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_LPC_MASKING_MODEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"

void WebRtcIsacfix_GetVars(const int16_t *input,
                           const int16_t *pitchGains_Q12,
                           uint32_t *oldEnergy,
                           int16_t *varscale);

void WebRtcIsacfix_GetLpcCoef(int16_t *inLoQ0,
                              int16_t *inHiQ0,
                              MaskFiltstr_enc *maskdata,
                              int16_t snrQ10,
                              const int16_t *pitchGains_Q12,
                              int32_t *gain_lo_hiQ17,
                              int16_t *lo_coeffQ15,
                              int16_t *hi_coeffQ15);

typedef int32_t (*CalculateResidualEnergy)(int lpc_order,
                                           int32_t q_val_corr,
                                           int q_val_polynomial,
                                           int16_t* a_polynomial,
                                           int32_t* corr_coeffs,
                                           int* q_val_residual_energy);
extern CalculateResidualEnergy WebRtcIsacfix_CalculateResidualEnergy;

int32_t WebRtcIsacfix_CalculateResidualEnergyC(int lpc_order,
                                               int32_t q_val_corr,
                                               int q_val_polynomial,
                                               int16_t* a_polynomial,
                                               int32_t* corr_coeffs,
                                               int* q_val_residual_energy);

#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int32_t WebRtcIsacfix_CalculateResidualEnergyNeon(int lpc_order,
                                                  int32_t q_val_corr,
                                                  int q_val_polynomial,
                                                  int16_t* a_polynomial,
                                                  int32_t* corr_coeffs,
                                                  int* q_val_residual_energy);
#endif

#if defined(MIPS_DSP_R2_LE)
int32_t WebRtcIsacfix_CalculateResidualEnergyMIPS(int lpc_order,
                                                  int32_t q_val_corr,
                                                  int q_val_polynomial,
                                                  int16_t* a_polynomial,
                                                  int32_t* corr_coeffs,
                                                  int* q_val_residual_energy);
#endif

#ifdef __cplusplus
} 
#endif

#endif
