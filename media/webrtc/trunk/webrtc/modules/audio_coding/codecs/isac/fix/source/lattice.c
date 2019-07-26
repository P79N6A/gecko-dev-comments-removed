
















#include "codec.h"
#include "settings.h"

#define LATTICE_MUL_32_32_RSFT16(a32a, a32b, b32)                  \
  ((WebRtc_Word32)(WEBRTC_SPL_MUL(a32a, b32) + (WEBRTC_SPL_MUL_16_32_RSFT16(a32b, b32))))



















void WebRtcIsacfix_FilterArLoop(int16_t* ar_g_Q0,
                                int16_t* ar_f_Q0,
                                int16_t* cth_Q15,
                                int16_t* sth_Q15,
                                int16_t order_coef);








void WebRtcIsacfix_FilterMaLoopC(int16_t input0,  
                                 int16_t input1,  
                                 int32_t input2,  
                                 int32_t* ptr0,   
                                 int32_t* ptr1,   
                                 int32_t* ptr2) { 
  int n = 0;

  
  
  int16_t t16a = (int16_t)(input2 >> 16);
  int16_t t16b = (int16_t)input2;
  if (t16b < 0) t16a++;

  
  
  for(n = 0; n < HALF_SUBFRAMELEN - 1; n++, ptr0++, ptr1++, ptr2++) {
    int32_t tmp32a = 0;
    int32_t tmp32b = 0;

    
    tmp32a = WEBRTC_SPL_MUL_16_32_RSFT15(input0, *ptr0); 
    tmp32b = *ptr2 + tmp32a; 
    *ptr2 = LATTICE_MUL_32_32_RSFT16(t16a, t16b, tmp32b);

    
    tmp32a = WEBRTC_SPL_MUL_16_32_RSFT15(input1, *ptr0); 
    tmp32b = WEBRTC_SPL_MUL_16_32_RSFT15(input0, *ptr2); 
    *ptr1 = tmp32a + tmp32b; 
  }
}


FilterMaLoopFix WebRtcIsacfix_FilterMaLoopFix;



void WebRtcIsacfix_NormLatticeFilterMa(WebRtc_Word16 orderCoef,
                                       WebRtc_Word32 *stateGQ15,
                                       WebRtc_Word16 *lat_inQ0,
                                       WebRtc_Word16 *filt_coefQ15,
                                       WebRtc_Word32 *gain_lo_hiQ17,
                                       WebRtc_Word16 lo_hi,
                                       WebRtc_Word16 *lat_outQ9)
{
  WebRtc_Word16 sthQ15[MAX_AR_MODEL_ORDER];
  WebRtc_Word16 cthQ15[MAX_AR_MODEL_ORDER];

  int u, i, k, n;
  WebRtc_Word16 temp2,temp3;
  WebRtc_Word16 ord_1 = orderCoef+1;
  WebRtc_Word32 inv_cthQ16[MAX_AR_MODEL_ORDER];

  WebRtc_Word32 gain32, fQtmp;
  WebRtc_Word16 gain16;
  WebRtc_Word16 gain_sh;

  WebRtc_Word32 tmp32, tmp32b;
  WebRtc_Word32 fQ15vec[HALF_SUBFRAMELEN];
  WebRtc_Word32 gQ15[MAX_AR_MODEL_ORDER+1][HALF_SUBFRAMELEN];
  WebRtc_Word16 sh;
  WebRtc_Word16 t16a;
  WebRtc_Word16 t16b;

  for (u=0;u<SUBFRAMES;u++)
  {
    int32_t temp1 = WEBRTC_SPL_MUL_16_16(u, HALF_SUBFRAMELEN);

    
    temp2 = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(u, orderCoef);
    temp3 = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(2, u)+lo_hi;

    
    memcpy(sthQ15, &filt_coefQ15[temp2], orderCoef * sizeof(WebRtc_Word16));

    WebRtcSpl_SqrtOfOneMinusXSquared(sthQ15, orderCoef, cthQ15);

    
    gain32 = gain_lo_hiQ17[temp3];
    gain_sh = WebRtcSpl_NormW32(gain32);
    gain32 = WEBRTC_SPL_LSHIFT_W32(gain32, gain_sh); 

    for (k=0;k<orderCoef;k++)
    {
      gain32 = WEBRTC_SPL_MUL_16_32_RSFT15(cthQ15[k], gain32); 
      inv_cthQ16[k] = WebRtcSpl_DivW32W16((WebRtc_Word32)2147483647, cthQ15[k]); 
    }
    gain16 = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(gain32, 16); 

    
    

    
    for (i=0;i<HALF_SUBFRAMELEN;i++)
    {
      fQ15vec[i] = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)lat_inQ0[i + temp1], 15); 
      gQ15[0][i] = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)lat_inQ0[i + temp1], 15); 
    }


    fQtmp = fQ15vec[0];

    
    for (i=1;i<ord_1;i++)
    {
      
      tmp32 = WEBRTC_SPL_MUL_16_32_RSFT15(sthQ15[i-1], stateGQ15[i-1]);
      tmp32b= fQtmp + tmp32; 
      tmp32 = inv_cthQ16[i-1]; 
      t16a = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(tmp32, 16);
      t16b = (WebRtc_Word16) (tmp32-WEBRTC_SPL_LSHIFT_W32(((WebRtc_Word32)t16a), 16));
      if (t16b<0) t16a++;
      tmp32 = LATTICE_MUL_32_32_RSFT16(t16a, t16b, tmp32b);
      fQtmp = tmp32; 

      
      tmp32  = WEBRTC_SPL_MUL_16_32_RSFT15(cthQ15[i-1], stateGQ15[i-1]); 
      tmp32b = WEBRTC_SPL_MUL_16_32_RSFT15(sthQ15[i-1], fQtmp); 
      tmp32  = tmp32 + tmp32b;
      gQ15[i][0] = tmp32; 
    }

    
    
    for(k=0;k<orderCoef;k++)
    {
      
      
      
      WebRtcIsacfix_FilterMaLoopFix(sthQ15[k], cthQ15[k], inv_cthQ16[k],
                                    &gQ15[k][0], &gQ15[k+1][1], &fQ15vec[1]);
    }

    fQ15vec[0] = fQtmp;

    for(n=0;n<HALF_SUBFRAMELEN;n++)
    {
      
      tmp32 = WEBRTC_SPL_MUL_16_32_RSFT16(gain16, fQ15vec[n]); 
      sh = 9-gain_sh; 
      t16a = (WebRtc_Word16) WEBRTC_SPL_SHIFT_W32(tmp32, sh);
      lat_outQ9[n + temp1] = t16a;
    }

    
    for (i=0;i<ord_1;i++)
    {
      stateGQ15[i] = gQ15[i][HALF_SUBFRAMELEN-1];
    }
    
  }

  return;
}







void WebRtcIsacfix_NormLatticeFilterAr(WebRtc_Word16 orderCoef,
                                       WebRtc_Word16 *stateGQ0,
                                       WebRtc_Word32 *lat_inQ25,
                                       WebRtc_Word16 *filt_coefQ15,
                                       WebRtc_Word32 *gain_lo_hiQ17,
                                       WebRtc_Word16 lo_hi,
                                       WebRtc_Word16 *lat_outQ0)
{
  int ii,n,k,i,u;
  WebRtc_Word16 sthQ15[MAX_AR_MODEL_ORDER];
  WebRtc_Word16 cthQ15[MAX_AR_MODEL_ORDER];
  WebRtc_Word32 tmp32;


  WebRtc_Word16 tmpAR;
  WebRtc_Word16 ARfQ0vec[HALF_SUBFRAMELEN];
  WebRtc_Word16 ARgQ0vec[MAX_AR_MODEL_ORDER+1];

  WebRtc_Word32 inv_gain32;
  WebRtc_Word16 inv_gain16;
  WebRtc_Word16 den16;
  WebRtc_Word16 sh;

  WebRtc_Word16 temp2,temp3;
  WebRtc_Word16 ord_1 = orderCoef+1;

  for (u=0;u<SUBFRAMES;u++)
  {
    int32_t temp1 = WEBRTC_SPL_MUL_16_16(u, HALF_SUBFRAMELEN);

    
    temp2 = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(u, orderCoef);
    temp3 = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(2, u) + lo_hi;

    for (ii=0; ii<orderCoef; ii++) {
      sthQ15[ii] = filt_coefQ15[temp2+ii];
    }

    WebRtcSpl_SqrtOfOneMinusXSquared(sthQ15, orderCoef, cthQ15);

    





    tmp32 = WEBRTC_SPL_LSHIFT_W32(gain_lo_hiQ17[temp3], 10); 

    for (k=0;k<orderCoef;k++) {
      tmp32 = WEBRTC_SPL_MUL_16_32_RSFT15(cthQ15[k], tmp32); 
    }

    sh = WebRtcSpl_NormW32(tmp32); 
    den16 = (WebRtc_Word16) WEBRTC_SPL_SHIFT_W32(tmp32, sh-16); 
    inv_gain32 = WebRtcSpl_DivW32W16((WebRtc_Word32)2147483647, den16); 

    
    inv_gain16 = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(inv_gain32, 2); 

    for (i=0;i<HALF_SUBFRAMELEN;i++)
    {

      tmp32 = WEBRTC_SPL_LSHIFT_W32(lat_inQ25[i + temp1], 1); 
      tmp32 = WEBRTC_SPL_MUL_16_32_RSFT16(inv_gain16, tmp32); 
      tmp32 = WEBRTC_SPL_SHIFT_W32(tmp32, -(28-sh)); 

      ARfQ0vec[i] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmp32); 
    }

    for (i=orderCoef-1;i>=0;i--) 
    {
      tmp32 = WEBRTC_SPL_RSHIFT_W32(((WEBRTC_SPL_MUL_16_16(cthQ15[i],ARfQ0vec[0])) - (WEBRTC_SPL_MUL_16_16(sthQ15[i],stateGQ0[i])) + 16384), 15);
      tmpAR = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmp32); 

      tmp32 = WEBRTC_SPL_RSHIFT_W32(((WEBRTC_SPL_MUL_16_16(sthQ15[i],ARfQ0vec[0])) + (WEBRTC_SPL_MUL_16_16(cthQ15[i], stateGQ0[i])) + 16384), 15);
      ARgQ0vec[i+1] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmp32); 
      ARfQ0vec[0] = tmpAR;
    }
    ARgQ0vec[0] = ARfQ0vec[0];

    
    WebRtcIsacfix_FilterArLoop(ARgQ0vec, ARfQ0vec, cthQ15, sthQ15, orderCoef);

    for(n=0;n<HALF_SUBFRAMELEN;n++)
    {
      lat_outQ0[n + temp1] = ARfQ0vec[n];
    }


    

    for (i=0;i<ord_1;i++)
    {
      stateGQ0[i] = ARgQ0vec[i];
    }
  }

  return;
}
