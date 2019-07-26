










#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_CNG_MAIN_INTERFACE_WEBRTC_CNG_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_CNG_MAIN_INTERFACE_WEBRTC_CNG_H_

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEBRTC_CNG_MAX_LPC_ORDER 12
#define WEBRTC_CNG_MAX_OUTSIZE_ORDER 640




#define CNG_ENCODER_NOT_INITIATED               6120
#define CNG_DISALLOWED_LPC_ORDER                6130
#define CNG_DISALLOWED_FRAME_SIZE               6140
#define CNG_DISALLOWED_SAMPLING_FREQUENCY       6150

#define CNG_DECODER_NOT_INITIATED               6220

typedef struct WebRtcCngEncInst CNG_enc_inst;
typedef struct WebRtcCngDecInst CNG_dec_inst;












int16_t WebRtcCng_CreateEnc(CNG_enc_inst** cng_inst);
int16_t WebRtcCng_CreateDec(CNG_dec_inst** cng_inst);




















int16_t WebRtcCng_InitEnc(CNG_enc_inst* cng_inst, uint16_t fs, int16_t interval,
                          int16_t quality);
int16_t WebRtcCng_InitDec(CNG_dec_inst* cng_inst);












int16_t WebRtcCng_FreeEnc(CNG_enc_inst* cng_inst);
int16_t WebRtcCng_FreeDec(CNG_dec_inst* cng_inst);


















int16_t WebRtcCng_Encode(CNG_enc_inst* cng_inst, int16_t* speech,
                         int16_t nrOfSamples, uint8_t* SIDdata,
                         int16_t* bytesOut, int16_t forceSID);














int16_t WebRtcCng_UpdateSid(CNG_dec_inst* cng_inst, uint8_t* SID,
                            int16_t length);















int16_t WebRtcCng_Generate(CNG_dec_inst* cng_inst, int16_t* outData,
                           int16_t nrOfSamples, int16_t new_period);














int16_t WebRtcCng_GetErrorCodeEnc(CNG_enc_inst* cng_inst);
int16_t WebRtcCng_GetErrorCodeDec(CNG_dec_inst* cng_inst);

#ifdef __cplusplus
}
#endif

#endif
