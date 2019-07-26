
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_LPC_MASKING_MODEL_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_LPC_MASKING_MODEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"

void WebRtcIsacfix_GetVars(const WebRtc_Word16 *input,
                           const WebRtc_Word16 *pitchGains_Q12,
                           WebRtc_UWord32 *oldEnergy,
                           WebRtc_Word16 *varscale);

void WebRtcIsacfix_GetLpcCoef(WebRtc_Word16 *inLoQ0,
                              WebRtc_Word16 *inHiQ0,
                              MaskFiltstr_enc *maskdata,
                              WebRtc_Word16 snrQ10,
                              const WebRtc_Word16 *pitchGains_Q12,
                              WebRtc_Word32 *gain_lo_hiQ17,
                              WebRtc_Word16 *lo_coeffQ15,
                              WebRtc_Word16 *hi_coeffQ15);

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

#ifdef __cplusplus
} 
#endif

#endif
