

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_ENCODE_LPC_SWB_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_ENCODE_LPC_SWB_H_

#include "typedefs.h"
#include "settings.h"
#include "structs.h"


















WebRtc_Word16 WebRtcIsac_RemoveLarMean(
    double*     lar,
    WebRtc_Word16 bandwidth);

















WebRtc_Word16 WebRtcIsac_DecorrelateIntraVec(
    const double* inLAR,
    double*       out,
    WebRtc_Word16   bandwidth);



















WebRtc_Word16 WebRtcIsac_DecorrelateInterVec(
    const double* data,
    double*       out,
    WebRtc_Word16   bandwidth);
















double WebRtcIsac_QuantizeUncorrLar(
    double*     data,
    int*        idx,
    WebRtc_Word16 bandwidth);















WebRtc_Word16 WebRtcIsac_CorrelateIntraVec(
    const double* data,
    double*       out,
    WebRtc_Word16   bandwidth);















WebRtc_Word16 WebRtcIsac_CorrelateInterVec(
    const double* data,
    double*       out,
    WebRtc_Word16   bandwidth);















WebRtc_Word16 WebRtcIsac_AddLarMean(
    double*     data,
    WebRtc_Word16 bandwidth);















WebRtc_Word16 WebRtcIsac_DequantizeLpcParam(
    const int*  idx,
    double*     out,
    WebRtc_Word16 bandwidth);













WebRtc_Word16 WebRtcIsac_ToLogDomainRemoveMean(
    double* lpGains);














WebRtc_Word16 WebRtcIsac_DecorrelateLPGain(
    const double* data,
    double*       out);














double WebRtcIsac_QuantizeLpcGain(
    double* lpGains,
    int*    idx);













WebRtc_Word16 WebRtcIsac_DequantizeLpcGain(
    const int* idx,
    double*    lpGains);













WebRtc_Word16 WebRtcIsac_CorrelateLpcGain(
    const double* data,
    double*       out);













WebRtc_Word16 WebRtcIsac_AddMeanToLinearDomain(
    double* lpcGains);


#endif  
