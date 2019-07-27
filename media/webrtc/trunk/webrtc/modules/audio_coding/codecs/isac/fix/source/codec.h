

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_CODEC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_CODEC_H_

#include "structs.h"

#ifdef __cplusplus
extern "C" {
#endif

int WebRtcIsacfix_EstimateBandwidth(BwEstimatorstr* bwest_str,
                                    Bitstr_dec* streamdata,
                                    int32_t packet_size,
                                    uint16_t rtp_seq_number,
                                    uint32_t send_ts,
                                    uint32_t arr_ts);

int16_t WebRtcIsacfix_DecodeImpl(int16_t* signal_out16,
                                       ISACFIX_DecInst_t* ISACdec_obj,
                                       int16_t* current_framesamples);

int16_t WebRtcIsacfix_DecodePlcImpl(int16_t* decoded,
                                          ISACFIX_DecInst_t* ISACdec_obj,
                                          int16_t* current_framesample );

int WebRtcIsacfix_EncodeImpl(int16_t* in,
                             ISACFIX_EncInst_t* ISACenc_obj,
                             BwEstimatorstr* bw_estimatordata,
                             int16_t CodingMode);

int WebRtcIsacfix_EncodeStoredData(ISACFIX_EncInst_t* ISACenc_obj,
                                   int BWnumber,
                                   float scale);



void WebRtcIsacfix_InitMaskingEnc(MaskFiltstr_enc* maskdata);
void WebRtcIsacfix_InitMaskingDec(MaskFiltstr_dec* maskdata);

void WebRtcIsacfix_InitPreFilterbank(PreFiltBankstr* prefiltdata);

void WebRtcIsacfix_InitPostFilterbank(PostFiltBankstr* postfiltdata);

void WebRtcIsacfix_InitPitchFilter(PitchFiltstr* pitchfiltdata);

void WebRtcIsacfix_InitPitchAnalysis(PitchAnalysisStruct* State);

void WebRtcIsacfix_InitPlc(PLCstr* State);




void WebRtcIsacfix_InitTransform();

typedef void (*Time2Spec)(int16_t* inre1Q9,
                          int16_t* inre2Q9,
                          int16_t* outre,
                          int16_t* outim);
typedef void (*Spec2Time)(int16_t* inreQ7,
                          int16_t* inimQ7,
                          int32_t* outre1Q16,
                          int32_t* outre2Q16);

extern Time2Spec WebRtcIsacfix_Time2Spec;
extern Spec2Time WebRtcIsacfix_Spec2Time;

void WebRtcIsacfix_Time2SpecC(int16_t* inre1Q9,
                              int16_t* inre2Q9,
                              int16_t* outre,
                              int16_t* outim);
void WebRtcIsacfix_Spec2TimeC(int16_t* inreQ7,
                              int16_t* inimQ7,
                              int32_t* outre1Q16,
                              int32_t* outre2Q16);

#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
void WebRtcIsacfix_Time2SpecNeon(int16_t* inre1Q9,
                                 int16_t* inre2Q9,
                                 int16_t* outre,
                                 int16_t* outim);
void WebRtcIsacfix_Spec2TimeNeon(int16_t* inreQ7,
                                 int16_t* inimQ7,
                                 int32_t* outre1Q16,
                                 int32_t* outre2Q16);
#endif

#if defined(MIPS32_LE)
void WebRtcIsacfix_Time2SpecMIPS(int16_t* inre1Q9,
                                 int16_t* inre2Q9,
                                 int16_t* outre,
                                 int16_t* outim);
void WebRtcIsacfix_Spec2TimeMIPS(int16_t* inreQ7,
                                 int16_t* inimQ7,
                                 int32_t* outre1Q16,
                                 int32_t* outre2Q16);
#endif



void WebRtcIsacfix_SplitAndFilter1(int16_t* in,
                                   int16_t* LP16,
                                   int16_t* HP16,
                                   PreFiltBankstr* prefiltdata);

void WebRtcIsacfix_FilterAndCombine1(int16_t* tempin_ch1,
                                     int16_t* tempin_ch2,
                                     int16_t* out16,
                                     PostFiltBankstr* postfiltdata);

#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED

void WebRtcIsacfix_SplitAndFilter2(int16_t* in,
                                   int16_t* LP16,
                                   int16_t* HP16,
                                   PreFiltBankstr* prefiltdata);

void WebRtcIsacfix_FilterAndCombine2(int16_t* tempin_ch1,
                                     int16_t* tempin_ch2,
                                     int16_t* out16,
                                     PostFiltBankstr* postfiltdata,
                                     int16_t len);

#endif



void WebRtcIsacfix_NormLatticeFilterMa(int16_t orderCoef,
                                       int32_t* stateGQ15,
                                       int16_t* lat_inQ0,
                                       int16_t* filt_coefQ15,
                                       int32_t* gain_lo_hiQ17,
                                       int16_t lo_hi,
                                       int16_t* lat_outQ9);

void WebRtcIsacfix_NormLatticeFilterAr(int16_t orderCoef,
                                       int16_t* stateGQ0,
                                       int32_t* lat_inQ25,
                                       int16_t* filt_coefQ15,
                                       int32_t* gain_lo_hiQ17,
                                       int16_t lo_hi,
                                       int16_t* lat_outQ0);





int WebRtcIsacfix_AutocorrC(int32_t* __restrict r,
                            const int16_t* __restrict x,
                            int16_t N,
                            int16_t order,
                            int16_t* __restrict scale);

void WebRtcIsacfix_FilterMaLoopC(int16_t input0,
                                 int16_t input1,
                                 int32_t input2,
                                 int32_t* ptr0,
                                 int32_t* ptr1,
                                 int32_t* ptr2);

#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int WebRtcIsacfix_AutocorrNeon(int32_t* __restrict r,
                               const int16_t* __restrict x,
                               int16_t N,
                               int16_t order,
                               int16_t* __restrict scale);

void WebRtcIsacfix_FilterMaLoopNeon(int16_t input0,
                                    int16_t input1,
                                    int32_t input2,
                                    int32_t* ptr0,
                                    int32_t* ptr1,
                                    int32_t* ptr2);
#endif

#if defined(MIPS32_LE)
int WebRtcIsacfix_AutocorrMIPS(int32_t* __restrict r,
                               const int16_t* __restrict x,
                               int16_t N,
                               int16_t order,
                               int16_t* __restrict scale);

void WebRtcIsacfix_FilterMaLoopMIPS(int16_t input0,
                                    int16_t input1,
                                    int32_t input2,
                                    int32_t* ptr0,
                                    int32_t* ptr1,
                                    int32_t* ptr2);
#endif



typedef int (*AutocorrFix)(int32_t* __restrict r,
                           const int16_t* __restrict x,
                           int16_t N,
                           int16_t order,
                           int16_t* __restrict scale);
extern AutocorrFix WebRtcIsacfix_AutocorrFix;

typedef void (*FilterMaLoopFix)(int16_t input0,
                                int16_t input1,
                                int32_t input2,
                                int32_t* ptr0,
                                int32_t* ptr1,
                                int32_t* ptr2);
extern FilterMaLoopFix WebRtcIsacfix_FilterMaLoopFix;

#ifdef __cplusplus
}  
#endif

#endif
