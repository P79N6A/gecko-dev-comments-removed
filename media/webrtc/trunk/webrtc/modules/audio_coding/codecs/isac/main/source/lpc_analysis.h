
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_LPC_ANALYSIS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_LPC_ANALYSIS_H_

#include "settings.h"
#include "structs.h"

double WebRtcIsac_LevDurb(double *a, double *k, double *r, int order);

void WebRtcIsac_GetVars(const double *input, const WebRtc_Word16 *pitchGains_Q12,
                       double *oldEnergy, double *varscale);

void WebRtcIsac_GetLpcCoefLb(double *inLo, double *inHi, MaskFiltstr *maskdata,
                             double signal_noise_ratio, const WebRtc_Word16 *pitchGains_Q12,
                             double *lo_coeff, double *hi_coeff);


void WebRtcIsac_GetLpcGain(
    double         signal_noise_ratio,
    const double*  filtCoeffVecs,
    int            numVecs,
    double*        gain,
    double         corrLo[][UB_LPC_ORDER + 1],
    const double*  varscale);

void WebRtcIsac_GetLpcCoefUb(
    double*      inSignal,
    MaskFiltstr* maskdata,
    double*      lpCoeff,
    double       corr[][UB_LPC_ORDER + 1],
    double*      varscale,
    WebRtc_Word16  bandwidth);

#endif 
