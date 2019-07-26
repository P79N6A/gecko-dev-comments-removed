
















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

















  WebRtc_Word16 WebRtcIlbcfix_EncoderAssign(iLBC_encinst_t **iLBC_encinst,
					    WebRtc_Word16 *ILBCENC_inst_Addr,
					    WebRtc_Word16 *size);
  WebRtc_Word16 WebRtcIlbcfix_DecoderAssign(iLBC_decinst_t **iLBC_decinst,
					    WebRtc_Word16 *ILBCDEC_inst_Addr,
					    WebRtc_Word16 *size);


  











  WebRtc_Word16 WebRtcIlbcfix_EncoderCreate(iLBC_encinst_t **iLBC_encinst);
  WebRtc_Word16 WebRtcIlbcfix_DecoderCreate(iLBC_decinst_t **iLBC_decinst);

  











  WebRtc_Word16 WebRtcIlbcfix_EncoderFree(iLBC_encinst_t *iLBC_encinst);
  WebRtc_Word16 WebRtcIlbcfix_DecoderFree(iLBC_decinst_t *iLBC_decinst);


  













  WebRtc_Word16 WebRtcIlbcfix_EncoderInit(iLBC_encinst_t *iLBCenc_inst,
					  WebRtc_Word16 frameLen);

  


















  WebRtc_Word16 WebRtcIlbcfix_Encode(iLBC_encinst_t *iLBCenc_inst,
                                     const WebRtc_Word16 *speechIn,
                                     WebRtc_Word16 len,
                                     WebRtc_Word16 *encoded);

  















  WebRtc_Word16 WebRtcIlbcfix_DecoderInit(iLBC_decinst_t *iLBCdec_inst,
					  WebRtc_Word16 frameLen);
  WebRtc_Word16 WebRtcIlbcfix_DecoderInit20Ms(iLBC_decinst_t *iLBCdec_inst);
  WebRtc_Word16 WebRtcIlbcfix_Decoderinit30Ms(iLBC_decinst_t *iLBCdec_inst);

  



















  WebRtc_Word16 WebRtcIlbcfix_Decode(iLBC_decinst_t *iLBCdec_inst,
                                     const WebRtc_Word16* encoded,
                                     WebRtc_Word16 len,
                                     WebRtc_Word16 *decoded,
                                     WebRtc_Word16 *speechType);
  WebRtc_Word16 WebRtcIlbcfix_Decode20Ms(iLBC_decinst_t *iLBCdec_inst,
                                         const WebRtc_Word16 *encoded,
                                         WebRtc_Word16 len,
                                         WebRtc_Word16 *decoded,
                                         WebRtc_Word16 *speechType);
  WebRtc_Word16 WebRtcIlbcfix_Decode30Ms(iLBC_decinst_t *iLBCdec_inst,
                                         const WebRtc_Word16 *encoded,
                                         WebRtc_Word16 len,
                                         WebRtc_Word16 *decoded,
                                         WebRtc_Word16 *speechType);

  

















  WebRtc_Word16 WebRtcIlbcfix_DecodePlc(iLBC_decinst_t *iLBCdec_inst,
					WebRtc_Word16 *decoded,
					WebRtc_Word16 noOfLostFrames);

  

















  WebRtc_Word16 WebRtcIlbcfix_NetEqPlc(iLBC_decinst_t *iLBCdec_inst,
				       WebRtc_Word16 *decoded,
				       WebRtc_Word16 noOfLostFrames);

  








  void WebRtcIlbcfix_version(char *version);

#ifdef __cplusplus
}
#endif

#endif
