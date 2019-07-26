

















#include "defines.h"
#include "constants.h"
#include "smooth_out_data.h"





void WebRtcIlbcfix_Smooth(
    WebRtc_Word16 *odata,   
    WebRtc_Word16 *current,  

    WebRtc_Word16 *surround  

                          ) {
  WebRtc_Word16 maxtot, scale, scale1, scale2;
  WebRtc_Word16 A, B, C, denomW16;
  WebRtc_Word32 B_W32, denom, num;
  WebRtc_Word32 errs;
  WebRtc_Word32 w00,w10,w11, endiff, crit;
  WebRtc_Word32 w00prim, w10prim, w11_div_w00;
  WebRtc_Word16 w11prim;
  WebRtc_Word16 bitsw00, bitsw10, bitsw11;
  WebRtc_Word32 w11w00, w10w10, w00w00;
  WebRtc_Word16 max1, max2;

  

  w00 = w10 = w11 = 0;

  max1=WebRtcSpl_MaxAbsValueW16(current, ENH_BLOCKL);
  max2=WebRtcSpl_MaxAbsValueW16(surround, ENH_BLOCKL);
  maxtot=WEBRTC_SPL_MAX(max1, max2);

  scale=WebRtcSpl_GetSizeInBits(maxtot);
  scale = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(2,scale)-26;
  scale=WEBRTC_SPL_MAX(0, scale);

  w00=WebRtcSpl_DotProductWithScale(current,current,ENH_BLOCKL,scale);
  w11=WebRtcSpl_DotProductWithScale(surround,surround,ENH_BLOCKL,scale);
  w10=WebRtcSpl_DotProductWithScale(surround,current,ENH_BLOCKL,scale);

  if (w00<0) w00 = WEBRTC_SPL_WORD32_MAX;
  if (w11<0) w11 = WEBRTC_SPL_WORD32_MAX;

  


  bitsw00 = WebRtcSpl_GetSizeInBits(w00);
  bitsw11 = WebRtcSpl_GetSizeInBits(w11);
  bitsw10 = WebRtcSpl_GetSizeInBits(WEBRTC_SPL_ABS_W32(w10));
  scale1 = 31 - bitsw00;
  scale2 = 15 - bitsw11;

  if (scale2>(scale1-16)) {
    scale2 = scale1 - 16;
  } else {
    scale1 = scale2 + 16;
  }

  w00prim = WEBRTC_SPL_LSHIFT_W32(w00, scale1);
  w11prim = (WebRtc_Word16) WEBRTC_SPL_SHIFT_W32(w11, scale2);

  
  if (w11prim>64) {
    endiff = WEBRTC_SPL_LSHIFT_W32(
        (WebRtc_Word32)WebRtcSpl_DivW32W16(w00prim, w11prim), 6);
    C = (WebRtc_Word16)WebRtcSpl_SqrtFloor(endiff); 
  } else {
    C = 1;
  }

  

  errs = WebRtcIlbcfix_Smooth_odata(odata, current, surround, C);



  

  if ( (6-scale+scale1) > 31) {
    crit=0;
  } else {
    
    crit = WEBRTC_SPL_SHIFT_W32(
        WEBRTC_SPL_MUL(ENH_A0, WEBRTC_SPL_RSHIFT_W32(w00prim, 14)),
        -(6-scale+scale1));
  }

  if (errs > crit) {

    if( w00 < 1) {
      w00=1;
    }

    

    scale1 = bitsw00-15;
    scale2 = bitsw11-15;

    if (scale2>scale1) {
      scale = scale2;
    } else {
      scale = scale1;
    }

    w11w00 = WEBRTC_SPL_MUL_16_16(
        (WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(w11, -scale),
        (WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(w00, -scale));

    w10w10 = WEBRTC_SPL_MUL_16_16(
        (WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(w10, -scale),
        (WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(w10, -scale));

    w00w00 = WEBRTC_SPL_MUL_16_16(
        (WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(w00, -scale),
        (WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(w00, -scale));

    
    if (w00w00>65536) {
      endiff = (w11w00-w10w10);
      endiff = WEBRTC_SPL_MAX(0, endiff);
      
      denom = WebRtcSpl_DivW32W16(endiff, (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(w00w00, 16));
    } else {
      denom = 65536;
    }

    if( denom > 7){ 


      scale=WebRtcSpl_GetSizeInBits(denom)-15;

      if (scale>0) {
        
        denomW16=(WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(denom, scale);

        
        num=WEBRTC_SPL_RSHIFT_W32(ENH_A0_MINUS_A0A0DIV4, scale);
      } else {
        
        denomW16=(WebRtc_Word16)denom;

        
        num=ENH_A0_MINUS_A0A0DIV4;
      }

      
      A = (WebRtc_Word16)WebRtcSpl_SqrtFloor(WebRtcSpl_DivW32W16(num, denomW16));

      
      scale1 = 31-bitsw10;
      scale2 = 21-scale1;
      w10prim = WEBRTC_SPL_LSHIFT_W32(w10, scale1);
      w00prim = WEBRTC_SPL_SHIFT_W32(w00, -scale2);
      scale = bitsw00-scale2-15;

      if (scale>0) {
        w10prim=WEBRTC_SPL_RSHIFT_W32(w10prim, scale);
        w00prim=WEBRTC_SPL_RSHIFT_W32(w00prim, scale);
      }

      if ((w00prim>0)&&(w10prim>0)) {
        w11_div_w00=WebRtcSpl_DivW32W16(w10prim, (WebRtc_Word16)w00prim);

        if (WebRtcSpl_GetSizeInBits(w11_div_w00)+WebRtcSpl_GetSizeInBits(A)>31) {
          B_W32 = 0;
        } else {
          B_W32 = (WebRtc_Word32)1073741824 - (WebRtc_Word32)ENH_A0DIV2 -
              WEBRTC_SPL_MUL(A, w11_div_w00);
        }
        B = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(B_W32, 16); 
      } else {
        
        A = 0;
        B = 16384; 
      }
    }
    else{ 


      A = 0;
      B = 16384; 
    }

    

    WebRtcSpl_ScaleAndAddVectors(surround, A, 9,
                                current, B, 14,
                                odata, ENH_BLOCKL);
  }
  return;
}
