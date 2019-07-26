

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_CODEC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_CODEC_H_

#include "structs.h"


void WebRtcIsac_ResetBitstream(Bitstr* bit_stream);

int WebRtcIsac_EstimateBandwidth(BwEstimatorstr* bwest_str, Bitstr* streamdata,
                                 int32_t packet_size,
                                 uint16_t rtp_seq_number,
                                 uint32_t send_ts, uint32_t arr_ts,
                                 enum IsacSamplingRate encoderSampRate,
                                 enum IsacSamplingRate decoderSampRate);

int WebRtcIsac_DecodeLb(float* signal_out, ISACLBDecStruct* ISACdec_obj,
                        int16_t* current_framesamples,
                        int16_t isRCUPayload);

int WebRtcIsac_DecodeRcuLb(float* signal_out, ISACLBDecStruct* ISACdec_obj,
                           int16_t* current_framesamples);

int WebRtcIsac_EncodeLb(float* in, ISACLBEncStruct* ISACencLB_obj,
                        int16_t codingMode, int16_t
                        bottleneckIndex);

int WebRtcIsac_EncodeStoredDataLb(const ISAC_SaveEncData_t* ISACSavedEnc_obj,
                                  Bitstr* ISACBitStr_obj, int BWnumber,
                                  float scale);

int WebRtcIsac_EncodeStoredDataUb(
    const ISACUBSaveEncDataStruct* ISACSavedEnc_obj, Bitstr* bitStream,
    int32_t jitterInfo, float scale, enum ISACBandwidth bandwidth);

int16_t WebRtcIsac_GetRedPayloadUb(
    const ISACUBSaveEncDataStruct* ISACSavedEncObj, Bitstr* bitStreamObj,
    enum ISACBandwidth bandwidth);



















int16_t WebRtcIsac_RateAllocation(int32_t inRateBitPerSec,
                                  double* rateLBBitPerSec,
                                  double* rateUBBitPerSec,
                                  enum ISACBandwidth* bandwidthKHz);

















int WebRtcIsac_DecodeUb16(float* signal_out, ISACUBDecStruct* ISACdec_obj,
                          int16_t isRCUPayload);

















int WebRtcIsac_DecodeUb12(float* signal_out, ISACUBDecStruct* ISACdec_obj,
                          int16_t isRCUPayload);

















int WebRtcIsac_EncodeUb16(float* in, ISACUBEncStruct* ISACenc_obj,
                          int32_t jitterInfo);

















int WebRtcIsac_EncodeUb12(float* in, ISACUBEncStruct* ISACenc_obj,
                          int32_t jitterInfo);



void WebRtcIsac_InitMasking(MaskFiltstr* maskdata);

void WebRtcIsac_InitPreFilterbank(PreFiltBankstr* prefiltdata);

void WebRtcIsac_InitPostFilterbank(PostFiltBankstr* postfiltdata);

void WebRtcIsac_InitPitchFilter(PitchFiltstr* pitchfiltdata);

void WebRtcIsac_InitPitchAnalysis(PitchAnalysisStruct* State);




void WebRtcIsac_InitTransform();

void WebRtcIsac_Time2Spec(double* inre1, double* inre2, int16_t* outre,
                          int16_t* outim, FFTstr* fftstr_obj);

void WebRtcIsac_Spec2time(double* inre, double* inim, double* outre1,
                          double* outre2, FFTstr* fftstr_obj);




void WebRtcIsac_AllPoleFilter(double* InOut, double* Coef, int lengthInOut,
                              int orderCoef);

void WebRtcIsac_AllZeroFilter(double* In, double* Coef, int lengthInOut,
                              int orderCoef, double* Out);

void WebRtcIsac_ZeroPoleFilter(double* In, double* ZeroCoef, double* PoleCoef,
                               int lengthInOut, int orderCoef, double* Out);




void WebRtcIsac_SplitAndFilterFloat(float* in, float* LP, float* HP,
                                    double* LP_la, double* HP_la,
                                    PreFiltBankstr* prefiltdata);


void WebRtcIsac_FilterAndCombineFloat(float* InLP, float* InHP, float* Out,
                                      PostFiltBankstr* postfiltdata);




void WebRtcIsac_NormLatticeFilterMa(int orderCoef, float* stateF, float* stateG,
                                    float* lat_in, double* filtcoeflo,
                                    double* lat_out);

void WebRtcIsac_NormLatticeFilterAr(int orderCoef, float* stateF, float* stateG,
                                    double* lat_in, double* lo_filt_coef,
                                    float* lat_out);

void WebRtcIsac_Dir2Lat(double* a, int orderCoef, float* sth, float* cth);

void WebRtcIsac_AutoCorr(double* r, const double* x, int N, int order);

#endif 
