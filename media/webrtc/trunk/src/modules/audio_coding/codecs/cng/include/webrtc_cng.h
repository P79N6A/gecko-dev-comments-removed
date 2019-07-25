










#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_CNG_MAIN_INTERFACE_WEBRTC_CNG_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_CNG_MAIN_INTERFACE_WEBRTC_CNG_H_

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEBRTC_CNG_MAX_LPC_ORDER 12
#define WEBRTC_CNG_MAX_OUTSIZE_ORDER 640




#define CNG_ENCODER_MEMORY_ALLOCATION_FAILED    6110
#define CNG_ENCODER_NOT_INITIATED               6120
#define CNG_DISALLOWED_LPC_ORDER                6130
#define CNG_DISALLOWED_FRAME_SIZE               6140
#define CNG_DISALLOWED_SAMPLING_FREQUENCY       6150

#define CNG_DECODER_MEMORY_ALLOCATION_FAILED    6210
#define CNG_DECODER_NOT_INITIATED               6220


typedef struct WebRtcCngEncInst         CNG_enc_inst;
typedef struct WebRtcCngDecInst         CNG_dec_inst;















WebRtc_Word16 WebRtcCng_Version(char *version);













WebRtc_Word16 WebRtcCng_AssignSizeEnc(int *sizeinbytes);
WebRtc_Word16 WebRtcCng_AssignSizeDec(int *sizeinbytes);
















WebRtc_Word16 WebRtcCng_AssignEnc(CNG_enc_inst **inst, void *CNG_inst_Addr);
WebRtc_Word16 WebRtcCng_AssignDec(CNG_dec_inst **inst, void *CNG_inst_Addr);














WebRtc_Word16 WebRtcCng_CreateEnc(CNG_enc_inst **cng_inst);
WebRtc_Word16 WebRtcCng_CreateDec(CNG_dec_inst **cng_inst);





















WebRtc_Word16 WebRtcCng_InitEnc(CNG_enc_inst *cng_inst,
                                WebRtc_Word16 fs,
                                WebRtc_Word16 interval,
                                WebRtc_Word16 quality);
WebRtc_Word16 WebRtcCng_InitDec(CNG_dec_inst *cng_dec_inst);

 













WebRtc_Word16 WebRtcCng_FreeEnc(CNG_enc_inst *cng_inst);
WebRtc_Word16 WebRtcCng_FreeDec(CNG_dec_inst *cng_inst);





















WebRtc_Word16 WebRtcCng_Encode(CNG_enc_inst *cng_inst,
                               WebRtc_Word16 *speech,
                               WebRtc_Word16 nrOfSamples,
                               WebRtc_UWord8* SIDdata,
                               WebRtc_Word16 *bytesOut,
                               WebRtc_Word16 forceSID);















WebRtc_Word16 WebRtcCng_UpdateSid(CNG_dec_inst *cng_inst,
                                  WebRtc_UWord8 *SID,
                                  WebRtc_Word16 length);
















WebRtc_Word16 WebRtcCng_Generate(CNG_dec_inst *cng_inst,
                                 WebRtc_Word16 * outData,
                                 WebRtc_Word16 nrOfSamples,
                                 WebRtc_Word16 new_period);
















WebRtc_Word16 WebRtcCng_GetErrorCodeEnc(CNG_enc_inst *cng_inst);
WebRtc_Word16 WebRtcCng_GetErrorCodeDec(CNG_dec_inst *cng_inst);


#ifdef __cplusplus
}
#endif

#endif
