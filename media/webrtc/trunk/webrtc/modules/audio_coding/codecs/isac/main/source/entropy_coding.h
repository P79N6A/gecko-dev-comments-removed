

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_ENTROPY_CODING_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_ENTROPY_CODING_H_

#include "settings.h"
#include "structs.h"

























int WebRtcIsac_DecodeSpec(Bitstr* streamdata, WebRtc_Word16 AvgPitchGain_Q12,
                          enum ISACBand band, double* fr, double* fi);
























int WebRtcIsac_EncodeSpec(const WebRtc_Word16* fr, const WebRtc_Word16* fi,
                          WebRtc_Word16 AvgPitchGain_Q12, enum ISACBand band,
                          Bitstr* streamdata);


int WebRtcIsac_DecodeLpcCoef(Bitstr* streamdata, double* LPCCoef);
int WebRtcIsac_DecodeLpcCoefUB(Bitstr* streamdata, double* lpcVecs,
                               double* percepFilterGains,
                               WebRtc_Word16 bandwidth);

int WebRtcIsac_DecodeLpc(Bitstr* streamdata, double* LPCCoef_lo,
                         double* LPCCoef_hi);


void WebRtcIsac_EncodeLpcLb(double* LPCCoef_lo, double* LPCCoef_hi,
                            Bitstr* streamdata, ISAC_SaveEncData_t* encData);

void WebRtcIsac_EncodeLpcGainLb(double* LPCCoef_lo, double* LPCCoef_hi,
                                Bitstr* streamdata,
                                ISAC_SaveEncData_t* encData);


































WebRtc_Word16 WebRtcIsac_EncodeLpcUB(double* lpcCoeff, Bitstr* streamdata,
                                     double* interpolLPCCoeff,
                                     WebRtc_Word16 bandwidth,
                                     ISACUBSaveEncDataStruct* encData);





























WebRtc_Word16 WebRtcIsac_DecodeInterpolLpcUb(Bitstr* streamdata,
                                             double* percepFilterParam,
                                             WebRtc_Word16 bandwidth);


int WebRtcIsac_DecodeRc(Bitstr* streamdata, WebRtc_Word16* RCQ15);


void WebRtcIsac_EncodeRc(WebRtc_Word16* RCQ15, Bitstr* streamdata);


int WebRtcIsac_DecodeGain2(Bitstr* streamdata, WebRtc_Word32* Gain2);


int WebRtcIsac_EncodeGain2(WebRtc_Word32* gain2, Bitstr* streamdata);

void WebRtcIsac_EncodePitchGain(WebRtc_Word16* PitchGains_Q12,
                                Bitstr* streamdata,
                                ISAC_SaveEncData_t* encData);

void WebRtcIsac_EncodePitchLag(double* PitchLags, WebRtc_Word16* PitchGain_Q12,
                               Bitstr* streamdata, ISAC_SaveEncData_t* encData);

int WebRtcIsac_DecodePitchGain(Bitstr* streamdata,
                               WebRtc_Word16* PitchGain_Q12);
int WebRtcIsac_DecodePitchLag(Bitstr* streamdata, WebRtc_Word16* PitchGain_Q12,
                              double* PitchLag);

int WebRtcIsac_DecodeFrameLen(Bitstr* streamdata, WebRtc_Word16* framelength);
int WebRtcIsac_EncodeFrameLen(WebRtc_Word16 framelength, Bitstr* streamdata);
int WebRtcIsac_DecodeSendBW(Bitstr* streamdata, WebRtc_Word16* BWno);
void WebRtcIsac_EncodeReceiveBw(int* BWno, Bitstr* streamdata);


void WebRtcIsac_Poly2Rc(double* a, int N, double* RC);


void WebRtcIsac_Rc2Poly(double* RC, int N, double* a);

void WebRtcIsac_TranscodeLPCCoef(double* LPCCoef_lo, double* LPCCoef_hi,
                                 int* index_g);


















void WebRtcIsac_EncodeLpcGainUb(double* lpGains, Bitstr* streamdata,
                                int* lpcGainIndex);















void WebRtcIsac_StoreLpcGainUb(double* lpGains, Bitstr* streamdata);

















WebRtc_Word16 WebRtcIsac_DecodeLpcGainUb(double* lpGains, Bitstr* streamdata);


















WebRtc_Word16 WebRtcIsac_EncodeBandwidth(enum ISACBandwidth bandwidth,
                                         Bitstr* streamData);



















WebRtc_Word16 WebRtcIsac_DecodeBandwidth(Bitstr* streamData,
                                         enum ISACBandwidth* bandwidth);



















WebRtc_Word16 WebRtcIsac_EncodeJitterInfo(WebRtc_Word32 jitterIndex,
                                          Bitstr* streamData);



















WebRtc_Word16 WebRtcIsac_DecodeJitterInfo(Bitstr* streamData,
                                          WebRtc_Word32* jitterInfo);

#endif 
