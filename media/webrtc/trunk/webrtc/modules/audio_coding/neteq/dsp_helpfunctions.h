













#ifndef DSP_HELPFUNCTIONS_H
#define DSP_HELPFUNCTIONS_H

#include "typedefs.h"

#include "dsp.h"


















WebRtc_Word16 WebRtcNetEQ_Correlator(DSPInst_t *inst,
#ifdef SCRATCH
                                     WebRtc_Word16 *pw16_scratchPtr,
#endif
                                     WebRtc_Word16 *pw16_data, WebRtc_Word16 w16_dataLen,
                                     WebRtc_Word16 *pw16_corrOut,
                                     WebRtc_Word16 *pw16_corrScale);



















WebRtc_Word16 WebRtcNetEQ_PeakDetection(WebRtc_Word16 *pw16_data, WebRtc_Word16 w16_dataLen,
                                        WebRtc_Word16 w16_nmbPeaks, WebRtc_Word16 fs_mult,
                                        WebRtc_Word16 *pw16_corrIndex,
                                        WebRtc_Word16 *pw16_winners);

















WebRtc_Word16 WebRtcNetEQ_PrblFit(WebRtc_Word16 *pw16_3pts, WebRtc_Word16 *pw16_Ind,
                                  WebRtc_Word16 *pw16_outVal, WebRtc_Word16 fs_mult);



















WebRtc_Word16 WebRtcNetEQ_MinDistortion(const WebRtc_Word16 *pw16_data,
                                        WebRtc_Word16 w16_minLag, WebRtc_Word16 w16_maxLag,
                                        WebRtc_Word16 len, WebRtc_Word32 *pw16_dist);















void WebRtcNetEQ_RandomVec(WebRtc_UWord32 *w32_seed, WebRtc_Word16 *pw16_randVec,
                           WebRtc_Word16 w16_len, WebRtc_Word16 w16_incval);

















void WebRtcNetEQ_MixVoiceUnvoice(WebRtc_Word16 *pw16_outData, WebRtc_Word16 *pw16_voicedVec,
                                 WebRtc_Word16 *pw16_unvoicedVec,
                                 WebRtc_Word16 *w16_current_vfraction,
                                 WebRtc_Word16 w16_vfraction_change, WebRtc_Word16 N);
















void WebRtcNetEQ_UnmuteSignal(WebRtc_Word16 *pw16_inVec, WebRtc_Word16 *startMuteFact,
                              WebRtc_Word16 *pw16_outVec, WebRtc_Word16 unmuteFact,
                              WebRtc_Word16 N);












void WebRtcNetEQ_MuteSignal(WebRtc_Word16 *pw16_inout, WebRtc_Word16 muteSlope,
                            WebRtc_Word16 N);












WebRtc_Word16 WebRtcNetEQ_CalcFsMult(WebRtc_UWord16 fsHz);






















int WebRtcNetEQ_DownSampleTo4kHz(const WebRtc_Word16 *in, int inLen, WebRtc_UWord16 inFsHz,
                                 WebRtc_Word16 *out, int outLen, int compensateDelay);

#endif

