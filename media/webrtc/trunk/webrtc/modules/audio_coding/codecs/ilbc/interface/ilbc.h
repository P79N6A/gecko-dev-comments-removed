
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_INTERFACE_ILBC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_INTERFACE_ILBC_H_





#include "typedefs.h"






typedef struct iLBC_encinst_t_ iLBC_encinst_t;

typedef struct iLBC_decinst_t_ iLBC_decinst_t;





#define ILBC_SPEECH 1
#define ILBC_CNG  2

#ifdef __cplusplus
extern "C" {
#endif

















  int16_t WebRtcIlbcfix_EncoderAssign(iLBC_encinst_t **iLBC_encinst,
                                      int16_t *ILBCENC_inst_Addr,
                                      int16_t *size);
  int16_t WebRtcIlbcfix_DecoderAssign(iLBC_decinst_t **iLBC_decinst,
                                      int16_t *ILBCDEC_inst_Addr,
                                      int16_t *size);


  











  int16_t WebRtcIlbcfix_EncoderCreate(iLBC_encinst_t **iLBC_encinst);
  int16_t WebRtcIlbcfix_DecoderCreate(iLBC_decinst_t **iLBC_decinst);

  











  int16_t WebRtcIlbcfix_EncoderFree(iLBC_encinst_t *iLBC_encinst);
  int16_t WebRtcIlbcfix_DecoderFree(iLBC_decinst_t *iLBC_decinst);


  













  int16_t WebRtcIlbcfix_EncoderInit(iLBC_encinst_t *iLBCenc_inst,
                                    int16_t frameLen);

  


















  int16_t WebRtcIlbcfix_Encode(iLBC_encinst_t *iLBCenc_inst,
                               const int16_t *speechIn,
                               int16_t len,
                               int16_t *encoded);

  















  int16_t WebRtcIlbcfix_DecoderInit(iLBC_decinst_t *iLBCdec_inst,
                                    int16_t frameLen);
  int16_t WebRtcIlbcfix_DecoderInit20Ms(iLBC_decinst_t *iLBCdec_inst);
  int16_t WebRtcIlbcfix_Decoderinit30Ms(iLBC_decinst_t *iLBCdec_inst);

  



















  int16_t WebRtcIlbcfix_Decode(iLBC_decinst_t *iLBCdec_inst,
                               const int16_t* encoded,
                               int16_t len,
                               int16_t *decoded,
                               int16_t *speechType);
  int16_t WebRtcIlbcfix_Decode20Ms(iLBC_decinst_t *iLBCdec_inst,
                                   const int16_t *encoded,
                                   int16_t len,
                                   int16_t *decoded,
                                   int16_t *speechType);
  int16_t WebRtcIlbcfix_Decode30Ms(iLBC_decinst_t *iLBCdec_inst,
                                   const int16_t *encoded,
                                   int16_t len,
                                   int16_t *decoded,
                                   int16_t *speechType);

  

















  int16_t WebRtcIlbcfix_DecodePlc(iLBC_decinst_t *iLBCdec_inst,
                                  int16_t *decoded,
                                  int16_t noOfLostFrames);

  

















  int16_t WebRtcIlbcfix_NetEqPlc(iLBC_decinst_t *iLBCdec_inst,
                                 int16_t *decoded,
                                 int16_t noOfLostFrames);

  








  void WebRtcIlbcfix_version(char *version);

#ifdef __cplusplus
}
#endif

#endif
