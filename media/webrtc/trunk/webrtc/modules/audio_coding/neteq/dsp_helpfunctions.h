













#ifndef DSP_HELPFUNCTIONS_H
#define DSP_HELPFUNCTIONS_H

#include "typedefs.h"

#include "dsp.h"


















int16_t WebRtcNetEQ_Correlator(DSPInst_t *inst,
#ifdef SCRATCH
                               int16_t *pw16_scratchPtr,
#endif
                               int16_t *pw16_data, int16_t w16_dataLen,
                               int16_t *pw16_corrOut,
                               int16_t *pw16_corrScale);



















int16_t WebRtcNetEQ_PeakDetection(int16_t *pw16_data, int16_t w16_dataLen,
                                  int16_t w16_nmbPeaks, int16_t fs_mult,
                                  int16_t *pw16_corrIndex,
                                  int16_t *pw16_winners);

















int16_t WebRtcNetEQ_PrblFit(int16_t *pw16_3pts, int16_t *pw16_Ind,
                            int16_t *pw16_outVal, int16_t fs_mult);



















int16_t WebRtcNetEQ_MinDistortion(const int16_t *pw16_data,
                                  int16_t w16_minLag, int16_t w16_maxLag,
                                  int16_t len, int32_t *pw16_dist);















void WebRtcNetEQ_RandomVec(uint32_t *w32_seed, int16_t *pw16_randVec,
                           int16_t w16_len, int16_t w16_incval);

















void WebRtcNetEQ_MixVoiceUnvoice(int16_t *pw16_outData, int16_t *pw16_voicedVec,
                                 int16_t *pw16_unvoicedVec,
                                 int16_t *w16_current_vfraction,
                                 int16_t w16_vfraction_change, int16_t N);
















void WebRtcNetEQ_UnmuteSignal(int16_t *pw16_inVec, int16_t *startMuteFact,
                              int16_t *pw16_outVec, int16_t unmuteFact,
                              int16_t N);












void WebRtcNetEQ_MuteSignal(int16_t *pw16_inout, int16_t muteSlope,
                            int16_t N);












int16_t WebRtcNetEQ_CalcFsMult(uint16_t fsHz);






















int WebRtcNetEQ_DownSampleTo4kHz(const int16_t *in, int inLen, uint16_t inFsHz,
                                 int16_t *out, int outLen, int compensateDelay);

#endif

