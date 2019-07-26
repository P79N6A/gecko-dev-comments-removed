

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_ENCODE_LPC_SWB_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_ENCODE_LPC_SWB_H_

#include "typedefs.h"
#include "settings.h"
#include "structs.h"


















int16_t WebRtcIsac_RemoveLarMean(
    double*     lar,
    int16_t bandwidth);

















int16_t WebRtcIsac_DecorrelateIntraVec(
    const double* inLAR,
    double*       out,
    int16_t   bandwidth);



















int16_t WebRtcIsac_DecorrelateInterVec(
    const double* data,
    double*       out,
    int16_t   bandwidth);
















double WebRtcIsac_QuantizeUncorrLar(
    double*     data,
    int*        idx,
    int16_t bandwidth);















int16_t WebRtcIsac_CorrelateIntraVec(
    const double* data,
    double*       out,
    int16_t   bandwidth);















int16_t WebRtcIsac_CorrelateInterVec(
    const double* data,
    double*       out,
    int16_t   bandwidth);















int16_t WebRtcIsac_AddLarMean(
    double*     data,
    int16_t bandwidth);















int16_t WebRtcIsac_DequantizeLpcParam(
    const int*  idx,
    double*     out,
    int16_t bandwidth);













int16_t WebRtcIsac_ToLogDomainRemoveMean(
    double* lpGains);














int16_t WebRtcIsac_DecorrelateLPGain(
    const double* data,
    double*       out);














double WebRtcIsac_QuantizeLpcGain(
    double* lpGains,
    int*    idx);













int16_t WebRtcIsac_DequantizeLpcGain(
    const int* idx,
    double*    lpGains);













int16_t WebRtcIsac_CorrelateLpcGain(
    const double* data,
    double*       out);













int16_t WebRtcIsac_AddMeanToLinearDomain(
    double* lpcGains);


#endif  
