









#include "webrtc/modules/audio_coding/codecs/isac/fix/source/pitch_estimator.h"

#ifdef WEBRTC_ARCH_ARM_NEON
#include <arm_neon.h>
#endif

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/system_wrappers/interface/compile_assert_c.h"


static const int16_t kLogLagWinQ8[3] = {
  -594, -256, -7
};


static const int16_t kACoefQ12[3] = {
  4096, -3072, 1024
};



static __inline int32_t Log2Q8( uint32_t x ) {

  int32_t zeros, lg2;
  int16_t frac;

  zeros=WebRtcSpl_NormU32(x);
  frac=(int16_t)WEBRTC_SPL_RSHIFT_W32(((uint32_t)(WEBRTC_SPL_LSHIFT_W32(x, zeros))&0x7FFFFFFF), 23);
  

  lg2= (WEBRTC_SPL_LSHIFT_W32((31-zeros), 8)+frac);
  return lg2;

}

static __inline int16_t Exp2Q10(int16_t x) { 

  int16_t tmp16_1, tmp16_2;

  tmp16_2=(int16_t)(0x0400|(x&0x03FF));
  tmp16_1=-(int16_t)WEBRTC_SPL_RSHIFT_W16(x,10);
  if(tmp16_1>0)
    return (int16_t) WEBRTC_SPL_RSHIFT_W16(tmp16_2, tmp16_1);
  else
    return (int16_t) WEBRTC_SPL_LSHIFT_W16(tmp16_2, -tmp16_1);

}




static __inline void Intrp1DQ8(int32_t *x, int32_t *fx, int32_t *y, int32_t *fy) {

  int16_t sign1=1, sign2=1;
  int32_t r32, q32, t32, nom32, den32;
  int16_t t16, tmp16, tmp16_1;

  if ((fx[0]>0) && (fx[2]>0)) {
    r32=fx[1]-fx[2];
    q32=fx[0]-fx[1];
    nom32=q32+r32;
    den32=WEBRTC_SPL_MUL_32_16((q32-r32), 2);
    if (nom32<0)
      sign1=-1;
    if (den32<0)
      sign2=-1;

    
    
    t32=WebRtcSpl_DivResultInQ31(WEBRTC_SPL_MUL_32_16(nom32, sign1),WEBRTC_SPL_MUL_32_16(den32, sign2)); 

    t16=(int16_t)WEBRTC_SPL_RSHIFT_W32(t32, 23);  
    t16=t16*sign1*sign2;        

    *y = x[0]+t16;          
    

    
    

    
    tmp16_1=(int16_t)WEBRTC_SPL_MUL_16_16(t16,t16); 
    tmp16_1 = WEBRTC_SPL_RSHIFT_W16(tmp16_1,2);  
    t16 = (int16_t)WEBRTC_SPL_MUL_16_16(t16, 64);           
    tmp16 = tmp16_1-t16;
    *fy = WEBRTC_SPL_MUL_16_32_RSFT15(tmp16, fx[0]); 

    
    tmp16 = 16384-tmp16_1;        
    *fy += WEBRTC_SPL_MUL_16_32_RSFT14(tmp16, fx[1]);

    
    tmp16 = tmp16_1+t16;
    *fy += WEBRTC_SPL_MUL_16_32_RSFT15(tmp16, fx[2]);
  } else {
    *y = x[0];
    *fy= fx[1];
  }
}


static void FindFour32(int32_t *in, int16_t length, int16_t *bestind)
{
  int32_t best[4]= {-100, -100, -100, -100};
  int16_t k;

  for (k=0; k<length; k++) {
    if (in[k] > best[3]) {
      if (in[k] > best[2]) {
        if (in[k] > best[1]) {
          if (in[k] > best[0]) { 
            best[3] = best[2];
            bestind[3] = bestind[2];
            best[2] = best[1];
            bestind[2] = bestind[1];
            best[1] = best[0];
            bestind[1] = bestind[0];
            best[0] = in[k];
            bestind[0] = k;
          } else { 
            best[3] = best[2];
            bestind[3] = bestind[2];
            best[2] = best[1];
            bestind[2] = bestind[1];
            best[1] = in[k];
            bestind[1] = k;
          }
        } else { 
          best[3] = best[2];
          bestind[3] = bestind[2];
          best[2] = in[k];
          bestind[2] = k;
        }
      } else {  
        best[3] = in[k];
        bestind[3] = k;
      }
    }
  }
}





static void PCorr2Q32(const int16_t *in, int32_t *logcorQ8)
{
  int16_t scaling,n,k;
  int32_t ysum32,csum32, lys, lcs;
  int32_t oneQ8;


  const int16_t *x, *inptr;

  oneQ8 = WEBRTC_SPL_LSHIFT_W32((int32_t)1, 8);  

  x = in + PITCH_MAX_LAG/2 + 2;
  scaling = WebRtcSpl_GetScalingSquare ((int16_t *) in, PITCH_CORR_LEN2, PITCH_CORR_LEN2);
  ysum32 = 1;
  csum32 = 0;
  x = in + PITCH_MAX_LAG/2 + 2;
  for (n = 0; n < PITCH_CORR_LEN2; n++) {
    ysum32 += WEBRTC_SPL_MUL_16_16_RSFT( (int16_t) in[n],(int16_t) in[n], scaling);  
    csum32 += WEBRTC_SPL_MUL_16_16_RSFT((int16_t) x[n],(int16_t) in[n], scaling); 
  }

  logcorQ8 += PITCH_LAG_SPAN2 - 1;

  lys=Log2Q8((uint32_t) ysum32); 
  lys=WEBRTC_SPL_RSHIFT_W32(lys, 1); 

  if (csum32>0) {

    lcs=Log2Q8((uint32_t) csum32);   

    if (lcs>(lys + oneQ8) ){ 
      *logcorQ8 = lcs - lys;  
    } else {
      *logcorQ8 = oneQ8;  
    }

  } else {
    *logcorQ8 = 0;
  }


  for (k = 1; k < PITCH_LAG_SPAN2; k++) {
    inptr = &in[k];
    ysum32 -= WEBRTC_SPL_MUL_16_16_RSFT( (int16_t) in[k-1],(int16_t) in[k-1], scaling);
    ysum32 += WEBRTC_SPL_MUL_16_16_RSFT( (int16_t) in[PITCH_CORR_LEN2 + k - 1],(int16_t) in[PITCH_CORR_LEN2 + k - 1], scaling);

#ifdef WEBRTC_ARCH_ARM_NEON
    {
      int32_t vbuff[4];
      int32x4_t int_32x4_sum = vmovq_n_s32(0);
      
      int32x4_t int_32x4_scale = vdupq_n_s32(-scaling);
      
      COMPILE_ASSERT(PITCH_CORR_LEN2 %4 == 0);

      for (n = 0; n < PITCH_CORR_LEN2; n += 4) {
        int16x4_t int_16x4_x = vld1_s16(&x[n]);
        int16x4_t int_16x4_in = vld1_s16(&inptr[n]);
        int32x4_t int_32x4 = vmull_s16(int_16x4_x, int_16x4_in);
        int_32x4 = vshlq_s32(int_32x4, int_32x4_scale);
        int_32x4_sum = vaddq_s32(int_32x4_sum, int_32x4);
      }

      
      
      vst1q_s32(vbuff, int_32x4_sum);
      csum32 = vbuff[0] + vbuff[1];
      csum32 += vbuff[2];
      csum32 += vbuff[3];
    }
#else
    csum32 = 0;
    if(scaling == 0) {
      for (n = 0; n < PITCH_CORR_LEN2; n++) {
        csum32 += x[n] * inptr[n];
      }
    } else {
      for (n = 0; n < PITCH_CORR_LEN2; n++) {
        csum32 += (x[n] * inptr[n]) >> scaling;
      }
    }
#endif

    logcorQ8--;

    lys=Log2Q8((uint32_t)ysum32); 
    lys=WEBRTC_SPL_RSHIFT_W32(lys, 1); 

    if (csum32>0) {

      lcs=Log2Q8((uint32_t) csum32);   

      if (lcs>(lys + oneQ8) ){ 
        *logcorQ8 = lcs - lys;  
      } else {
        *logcorQ8 = oneQ8;  
      }

    } else {
      *logcorQ8 = 0;
    }
  }
}



void WebRtcIsacfix_InitialPitch(const int16_t *in, 
                                PitchAnalysisStruct *State,
                                int16_t *lagsQ7                   
                                )
{
  int16_t buf_dec16[PITCH_CORR_LEN2+PITCH_CORR_STEP2+PITCH_MAX_LAG/2+2];
  int32_t *crrvecQ8_1,*crrvecQ8_2;
  int32_t cv1q[PITCH_LAG_SPAN2+2],cv2q[PITCH_LAG_SPAN2+2], peakvq[PITCH_LAG_SPAN2+2];
  int k;
  int16_t peaks_indq;
  int16_t peakiq[PITCH_LAG_SPAN2];
  int32_t corr;
  int32_t corr32, corr_max32, corr_max_o32;
  int16_t npkq;
  int16_t best4q[4]={0,0,0,0};
  int32_t xq[3],yq[1],fyq[1];
  int32_t *fxq;
  int32_t best_lag1q, best_lag2q;
  int32_t tmp32a,tmp32b,lag32,ratq;
  int16_t start;
  int16_t oldgQ12, tmp16a, tmp16b, gain_bias16,tmp16c, tmp16d, bias16;
  int32_t tmp32c,tmp32d, tmp32e;
  int16_t old_lagQ;
  int32_t old_lagQ8;
  int32_t lagsQ8[4];

  old_lagQ = State->PFstr_wght.oldlagQ7; 
  old_lagQ8= WEBRTC_SPL_LSHIFT_W32((int32_t)old_lagQ,1); 

  oldgQ12= State->PFstr_wght.oldgainQ12;

  crrvecQ8_1=&cv1q[1];
  crrvecQ8_2=&cv2q[1];


  
  memcpy(buf_dec16, State->dec_buffer16, WEBRTC_SPL_MUL_16_16(sizeof(int16_t), (PITCH_CORR_LEN2+PITCH_CORR_STEP2+PITCH_MAX_LAG/2-PITCH_FRAME_LEN/2+2)));

  
  WebRtcIsacfix_DecimateAllpass32(in, State->decimator_state32, PITCH_FRAME_LEN,
                                  &buf_dec16[PITCH_CORR_LEN2+PITCH_CORR_STEP2+PITCH_MAX_LAG/2-PITCH_FRAME_LEN/2+2]);

  
  start= PITCH_CORR_LEN2+PITCH_CORR_STEP2+PITCH_MAX_LAG/2-PITCH_FRAME_LEN/2+2;
  WebRtcSpl_FilterARFastQ12(&buf_dec16[start],&buf_dec16[start],(int16_t*)kACoefQ12,3, PITCH_FRAME_LEN/2);

  
  for (k = 0; k < (PITCH_CORR_LEN2+PITCH_CORR_STEP2+PITCH_MAX_LAG/2-PITCH_FRAME_LEN/2+2); k++)
    State->dec_buffer16[k] = buf_dec16[k+PITCH_FRAME_LEN/2];


  
  PCorr2Q32(buf_dec16, crrvecQ8_1);
  PCorr2Q32(buf_dec16 + PITCH_CORR_STEP2, crrvecQ8_2);


  
  tmp32a = Log2Q8((uint32_t) old_lagQ8) - 2304; 
  tmp32b = WEBRTC_SPL_MUL_16_16_RSFT(oldgQ12,oldgQ12, 10); 
  gain_bias16 = (int16_t) tmp32b;  
  if (gain_bias16 > 3276) gain_bias16 = 3276; 


  for (k = 0; k < PITCH_LAG_SPAN2; k++)
  {
    if (crrvecQ8_1[k]>0) {
      tmp32b = Log2Q8((uint32_t) (k + (PITCH_MIN_LAG/2-2)));
      tmp16a = (int16_t) (tmp32b - tmp32a); 
      tmp32c = WEBRTC_SPL_MUL_16_16_RSFT(tmp16a,tmp16a, 6); 
      tmp16b = (int16_t) tmp32c; 
      tmp32d = WEBRTC_SPL_MUL_16_16_RSFT(tmp16b, 177 , 8); 
      tmp16c = (int16_t) tmp32d; 
      tmp16d = Exp2Q10((int16_t) -tmp16c); 
      tmp32c = WEBRTC_SPL_MUL_16_16_RSFT(gain_bias16,tmp16d,13); 
      bias16 = (int16_t) (1024 + tmp32c); 
      tmp32b = Log2Q8((uint32_t) bias16) - 2560; 
      crrvecQ8_1[k] += tmp32b ; 
    }
  }

  
  for (k = 0; k < 3; k++) {
    crrvecQ8_1[k] += kLogLagWinQ8[k];
    crrvecQ8_2[k] += kLogLagWinQ8[k];

    crrvecQ8_1[PITCH_LAG_SPAN2-1-k] += kLogLagWinQ8[k];
    crrvecQ8_2[PITCH_LAG_SPAN2-1-k] += kLogLagWinQ8[k];
  }


  
  cv1q[0]=0;
  cv2q[0]=0;
  cv1q[PITCH_LAG_SPAN2+1]=0;
  cv2q[PITCH_LAG_SPAN2+1]=0;
  corr_max32 = 0;

  for (k = 1; k <= PITCH_LAG_SPAN2; k++)
  {


    corr32=crrvecQ8_1[k-1];
    if (corr32 > corr_max32)
      corr_max32 = corr32;

    corr32=crrvecQ8_2[k-1];
    corr32 += -4; 

    if (corr32 > corr_max32)
      corr_max32 = corr32;

  }

  
  
  corr_max32 += -1000; 
  corr_max_o32 = corr_max32;


  
  peaks_indq = 0;
  for (k = 1; k <= PITCH_LAG_SPAN2; k++)
  {
    corr32=cv1q[k];
    if (corr32>corr_max32) { 
      if ((corr32>=cv1q[k-1]) && (corr32>cv1q[k+1])) { 
        peakvq[peaks_indq] = corr32;
        peakiq[peaks_indq++] = k;
      }
    }
  }


  
  corr_max32=0;
  best_lag1q =0;
  if (peaks_indq > 0) {
    FindFour32(peakvq, (int16_t) peaks_indq, best4q);
    npkq = WEBRTC_SPL_MIN(peaks_indq, 4);

    for (k=0;k<npkq;k++) {

      lag32 =  peakiq[best4q[k]];
      fxq = &cv1q[peakiq[best4q[k]]-1];
      xq[0]= lag32;
      xq[0] = WEBRTC_SPL_LSHIFT_W32(xq[0], 8);
      Intrp1DQ8(xq, fxq, yq, fyq);

      tmp32a= Log2Q8((uint32_t) *yq) - 2048; 
      
      
      tmp32b= WEBRTC_SPL_MUL_16_16_RSFT((int16_t) tmp32a, -42, 8);
      tmp32c= tmp32b + 256;
      *fyq += tmp32c;
      if (*fyq > corr_max32) {
        corr_max32 = *fyq;
        best_lag1q = *yq;
      }
    }
    tmp32a = best_lag1q - OFFSET_Q8;
    tmp32b = WEBRTC_SPL_LSHIFT_W32(tmp32a, 1);
    lagsQ8[0] = tmp32b + PITCH_MIN_LAG_Q8;
    lagsQ8[1] = lagsQ8[0];
  } else {
    lagsQ8[0] = old_lagQ8;
    lagsQ8[1] = lagsQ8[0];
  }

  
  tmp32a = lagsQ8[0] - PITCH_MIN_LAG_Q8;
  ratq = WEBRTC_SPL_RSHIFT_W32(tmp32a, 1) + OFFSET_Q8;

  for (k = 1; k <= PITCH_LAG_SPAN2; k++)
  {
    tmp32a = WEBRTC_SPL_LSHIFT_W32(k, 7); 
    tmp32b = (int32_t) (WEBRTC_SPL_LSHIFT_W32(tmp32a, 1)) - ratq; 
    tmp32c = WEBRTC_SPL_MUL_16_16_RSFT((int16_t) tmp32b, (int16_t) tmp32b, 8); 

    tmp32b = (int32_t) tmp32c + (int32_t)  WEBRTC_SPL_RSHIFT_W32(ratq, 1); 
    tmp32c = Log2Q8((uint32_t) tmp32a) - 2048; 
    tmp32d = Log2Q8((uint32_t) tmp32b) - 2048; 
    tmp32e =  tmp32c -tmp32d;

    cv2q[k] += WEBRTC_SPL_RSHIFT_W32(tmp32e, 1);

  }

  
  corr_max32 = corr_max_o32;
  peaks_indq = 0;

  for (k = 1; k <= PITCH_LAG_SPAN2; k++)
  {
    corr=cv2q[k];
    if (corr>corr_max32) { 
      if ((corr>=cv2q[k-1]) && (corr>cv2q[k+1])) { 
        peakvq[peaks_indq] = corr;
        peakiq[peaks_indq++] = k;
      }
    }
  }



  
  corr_max32 = 0;
  best_lag2q =0;
  if (peaks_indq > 0) {

    FindFour32(peakvq, (int16_t) peaks_indq, best4q);
    npkq = WEBRTC_SPL_MIN(peaks_indq, 4);
    for (k=0;k<npkq;k++) {

      lag32 =  peakiq[best4q[k]];
      fxq = &cv2q[peakiq[best4q[k]]-1];

      xq[0]= lag32;
      xq[0] = WEBRTC_SPL_LSHIFT_W32(xq[0], 8);
      Intrp1DQ8(xq, fxq, yq, fyq);

      
      
      tmp32a= Log2Q8((uint32_t) *yq) - 2048; 
      tmp32b= WEBRTC_SPL_MUL_16_16_RSFT((int16_t) tmp32a, -82, 8);
      tmp32c= tmp32b + 256;
      *fyq += tmp32c;
      if (*fyq > corr_max32) {
        corr_max32 = *fyq;
        best_lag2q = *yq;
      }
    }

    tmp32a = best_lag2q - OFFSET_Q8;
    tmp32b = WEBRTC_SPL_LSHIFT_W32(tmp32a, 1);
    lagsQ8[2] = tmp32b + PITCH_MIN_LAG_Q8;
    lagsQ8[3] = lagsQ8[2];
  } else {
    lagsQ8[2] = lagsQ8[0];
    lagsQ8[3] = lagsQ8[0];
  }

  lagsQ7[0]=(int16_t) WEBRTC_SPL_RSHIFT_W32(lagsQ8[0], 1);
  lagsQ7[1]=(int16_t) WEBRTC_SPL_RSHIFT_W32(lagsQ8[1], 1);
  lagsQ7[2]=(int16_t) WEBRTC_SPL_RSHIFT_W32(lagsQ8[2], 1);
  lagsQ7[3]=(int16_t) WEBRTC_SPL_RSHIFT_W32(lagsQ8[3], 1);


}



void WebRtcIsacfix_PitchAnalysis(const int16_t *inn,               
                                 int16_t *outQ0,                  
                                 PitchAnalysisStruct *State,
                                 int16_t *PitchLags_Q7,
                                 int16_t *PitchGains_Q12)
{
  int16_t inbufQ0[PITCH_FRAME_LEN + QLOOKAHEAD];
  int16_t k;

  
  WebRtcIsacfix_InitialPitch(inn, State,  PitchLags_Q7);


  
  WebRtcIsacfix_PitchFilterGains(inn, &(State->PFstr_wght), PitchLags_Q7, PitchGains_Q12);

  
  for (k = 0; k < QLOOKAHEAD; k++) {
    inbufQ0[k] = State->inbuf[k];
  }
  for (k = 0; k < PITCH_FRAME_LEN; k++) {
    inbufQ0[k+QLOOKAHEAD] = (int16_t) inn[k];
  }

  
  WebRtcIsacfix_PitchFilter(inbufQ0, outQ0, &(State->PFstr), PitchLags_Q7,PitchGains_Q12, 2);


  
  for (k = 0; k < QLOOKAHEAD; k++) {
    State->inbuf[k] = inbufQ0[k + PITCH_FRAME_LEN];
  }
}
