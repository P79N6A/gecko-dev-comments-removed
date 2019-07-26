

















#include "ilbc.h"
#include "defines.h"
#include "init_encode.h"
#include "encode.h"
#include "init_decode.h"
#include "decode.h"
#include <stdlib.h>


int16_t WebRtcIlbcfix_EncoderAssign(iLBC_encinst_t **iLBC_encinst, int16_t *ILBCENC_inst_Addr, int16_t *size) {
  *iLBC_encinst=(iLBC_encinst_t*)ILBCENC_inst_Addr;
  *size=sizeof(iLBC_Enc_Inst_t)/sizeof(int16_t);
  if (*iLBC_encinst!=NULL) {
    return(0);
  } else {
    return(-1);
  }
}

int16_t WebRtcIlbcfix_DecoderAssign(iLBC_decinst_t **iLBC_decinst, int16_t *ILBCDEC_inst_Addr, int16_t *size) {
  *iLBC_decinst=(iLBC_decinst_t*)ILBCDEC_inst_Addr;
  *size=sizeof(iLBC_Dec_Inst_t)/sizeof(int16_t);
  if (*iLBC_decinst!=NULL) {
    return(0);
  } else {
    return(-1);
  }
}

int16_t WebRtcIlbcfix_EncoderCreate(iLBC_encinst_t **iLBC_encinst) {
  *iLBC_encinst=(iLBC_encinst_t*)malloc(sizeof(iLBC_Enc_Inst_t));
  if (*iLBC_encinst!=NULL) {
    WebRtcSpl_Init();
    return(0);
  } else {
    return(-1);
  }
}

int16_t WebRtcIlbcfix_DecoderCreate(iLBC_decinst_t **iLBC_decinst) {
  *iLBC_decinst=(iLBC_decinst_t*)malloc(sizeof(iLBC_Dec_Inst_t));
  if (*iLBC_decinst!=NULL) {
    WebRtcSpl_Init();
    return(0);
  } else {
    return(-1);
  }
}

int16_t WebRtcIlbcfix_EncoderFree(iLBC_encinst_t *iLBC_encinst) {
  free(iLBC_encinst);
  return(0);
}

int16_t WebRtcIlbcfix_DecoderFree(iLBC_decinst_t *iLBC_decinst) {
  free(iLBC_decinst);
  return(0);
}


int16_t WebRtcIlbcfix_EncoderInit(iLBC_encinst_t *iLBCenc_inst, int16_t mode)
{
  if ((mode==20)||(mode==30)) {
    WebRtcIlbcfix_InitEncode((iLBC_Enc_Inst_t*) iLBCenc_inst, mode);
    return(0);
  } else {
    return(-1);
  }
}

int16_t WebRtcIlbcfix_Encode(iLBC_encinst_t *iLBCenc_inst, const int16_t *speechIn, int16_t len, int16_t *encoded) {

  int16_t pos = 0;
  int16_t encpos = 0;

  if ((len != ((iLBC_Enc_Inst_t*)iLBCenc_inst)->blockl) &&
#ifdef SPLIT_10MS
      (len != 80) &&
#endif
      (len != 2*((iLBC_Enc_Inst_t*)iLBCenc_inst)->blockl) &&
      (len != 3*((iLBC_Enc_Inst_t*)iLBCenc_inst)->blockl))
  {
    
    return(-1);
  } else {

    
    while (pos<len) {
      WebRtcIlbcfix_EncodeImpl((uint16_t*) &encoded[encpos], &speechIn[pos], (iLBC_Enc_Inst_t*) iLBCenc_inst);
#ifdef SPLIT_10MS
      pos += 80;
      if(((iLBC_Enc_Inst_t*)iLBCenc_inst)->section == 0)
#else
        pos += ((iLBC_Enc_Inst_t*)iLBCenc_inst)->blockl;
#endif
      encpos += ((iLBC_Enc_Inst_t*)iLBCenc_inst)->no_of_words;
    }
    return (encpos*2);
  }
}

int16_t WebRtcIlbcfix_DecoderInit(iLBC_decinst_t *iLBCdec_inst, int16_t mode) {
  if ((mode==20)||(mode==30)) {
    WebRtcIlbcfix_InitDecode((iLBC_Dec_Inst_t*) iLBCdec_inst, mode, 1);
    return(0);
  } else {
    return(-1);
  }
}
int16_t WebRtcIlbcfix_DecoderInit20Ms(iLBC_decinst_t *iLBCdec_inst) {
  WebRtcIlbcfix_InitDecode((iLBC_Dec_Inst_t*) iLBCdec_inst, 20, 1);
  return(0);
}
int16_t WebRtcIlbcfix_Decoderinit30Ms(iLBC_decinst_t *iLBCdec_inst) {
  WebRtcIlbcfix_InitDecode((iLBC_Dec_Inst_t*) iLBCdec_inst, 30, 1);
  return(0);
}


int16_t WebRtcIlbcfix_Decode(iLBC_decinst_t *iLBCdec_inst,
                             const int16_t *encoded,
                             int16_t len,
                             int16_t *decoded,
                             int16_t *speechType)
{
  int i=0;
  

  if ((len==((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)||
      (len==2*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)||
      (len==3*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)) {
    
  } else {
    
    if (((iLBC_Dec_Inst_t*)iLBCdec_inst)->mode==20) {
      if ((len==NO_OF_BYTES_30MS)||
          (len==2*NO_OF_BYTES_30MS)||
          (len==3*NO_OF_BYTES_30MS)) {
        WebRtcIlbcfix_InitDecode(((iLBC_Dec_Inst_t*)iLBCdec_inst), 30, ((iLBC_Dec_Inst_t*)iLBCdec_inst)->use_enhancer);
      } else {
        
        return(-1);
      }
    } else {
      if ((len==NO_OF_BYTES_20MS)||
          (len==2*NO_OF_BYTES_20MS)||
          (len==3*NO_OF_BYTES_20MS)) {
        WebRtcIlbcfix_InitDecode(((iLBC_Dec_Inst_t*)iLBCdec_inst), 20, ((iLBC_Dec_Inst_t*)iLBCdec_inst)->use_enhancer);
      } else {
        
        return(-1);
      }
    }
  }

  while ((i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)<len) {
    WebRtcIlbcfix_DecodeImpl(&decoded[i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->blockl], (const uint16_t*) &encoded[i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_words], (iLBC_Dec_Inst_t*) iLBCdec_inst, 1);
    i++;
  }
  
  *speechType=1;
  return(i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->blockl);
}

int16_t WebRtcIlbcfix_Decode20Ms(iLBC_decinst_t *iLBCdec_inst,
                                 const int16_t *encoded,
                                 int16_t len,
                                 int16_t *decoded,
                                 int16_t *speechType)
{
  int i=0;
  if ((len==((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)||
      (len==2*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)||
      (len==3*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)) {
    
  } else {
    return(-1);
  }

  while ((i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)<len) {
    WebRtcIlbcfix_DecodeImpl(&decoded[i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->blockl], (const uint16_t*) &encoded[i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_words], (iLBC_Dec_Inst_t*) iLBCdec_inst, 1);
    i++;
  }
  
  *speechType=1;
  return(i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->blockl);
}

int16_t WebRtcIlbcfix_Decode30Ms(iLBC_decinst_t *iLBCdec_inst,
                                 const int16_t *encoded,
                                 int16_t len,
                                 int16_t *decoded,
                                 int16_t *speechType)
{
  int i=0;
  if ((len==((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)||
      (len==2*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)||
      (len==3*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)) {
    
  } else {
    return(-1);
  }

  while ((i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_bytes)<len) {
    WebRtcIlbcfix_DecodeImpl(&decoded[i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->blockl], (const uint16_t*) &encoded[i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->no_of_words], (iLBC_Dec_Inst_t*) iLBCdec_inst, 1);
    i++;
  }
  
  *speechType=1;
  return(i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->blockl);
}

int16_t WebRtcIlbcfix_DecodePlc(iLBC_decinst_t *iLBCdec_inst, int16_t *decoded, int16_t noOfLostFrames) {
  int i;
  uint16_t dummy;

  for (i=0;i<noOfLostFrames;i++) {
    
    WebRtcIlbcfix_DecodeImpl(&decoded[i*((iLBC_Dec_Inst_t*)iLBCdec_inst)->blockl], &dummy, (iLBC_Dec_Inst_t*) iLBCdec_inst, 0);
  }
  return (noOfLostFrames*((iLBC_Dec_Inst_t*)iLBCdec_inst)->blockl);
}

int16_t WebRtcIlbcfix_NetEqPlc(iLBC_decinst_t *iLBCdec_inst, int16_t *decoded, int16_t noOfLostFrames) {

  
  (void)(decoded = NULL);
  (void)(noOfLostFrames = 0);

  WebRtcSpl_MemSetW16(((iLBC_Dec_Inst_t*)iLBCdec_inst)->enh_buf, 0, ENH_BUFL);
  ((iLBC_Dec_Inst_t*)iLBCdec_inst)->prev_enh_pl = 2;

  return (0);
}

void WebRtcIlbcfix_version(char *version)
{
  strcpy((char*)version, "1.1.1");
}
