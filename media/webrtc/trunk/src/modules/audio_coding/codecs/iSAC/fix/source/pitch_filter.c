
















#include <string.h>

#include "pitch_estimator.h"



static const WebRtc_Word16 kDampFilter[PITCH_DAMPORDER] = {
  -2294, 8192, 20972, 8192, -2294
};




static const WebRtc_Word16 kIntrpCoef[PITCH_FRACS][PITCH_FRACORDER] = {
  {-367, 1090, -2706,  9945, 10596, -3318,  1626, -781,  287},
  {-325,  953, -2292,  7301, 12963, -3320,  1570, -743,  271},
  {-240,  693, -1622,  4634, 14809, -2782,  1262, -587,  212},
  {-125,  358,  -817,  2144, 15982, -1668,   721, -329,  118},
  {   0,    0,    -1,     1, 16380,     1,    -1,    0,    0},
  { 118, -329,   721, -1668, 15982,  2144,  -817,  358, -125},
  { 212, -587,  1262, -2782, 14809,  4634, -1622,  693, -240},
  { 271, -743,  1570, -3320, 12963,  7301, -2292,  953, -325}
};




static __inline WebRtc_Word32 CalcLrIntQ(WebRtc_Word32 fixVal, WebRtc_Word16 qDomain) {
  WebRtc_Word32 intgr;
  WebRtc_Word32 roundVal;

  roundVal = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)1,  qDomain-1);
  intgr = WEBRTC_SPL_RSHIFT_W32(fixVal+roundVal, qDomain);

  return intgr;
}

void WebRtcIsacfix_PitchFilter(WebRtc_Word16 *indatQQ, 
                               WebRtc_Word16 *outdatQQ,
                               PitchFiltstr *pfp,
                               WebRtc_Word16 *lagsQ7,
                               WebRtc_Word16 *gainsQ12,
                               WebRtc_Word16 type)
{
  int    k, n, m, ind;
  WebRtc_Word16 sign = 1;
  WebRtc_Word16 inystateQQ[PITCH_DAMPORDER];
  WebRtc_Word16 ubufQQ[PITCH_INTBUFFSIZE+QLOOKAHEAD];
  WebRtc_Word16 Gain = 21299;     
  WebRtc_Word16 DivFactor = 6553; 
  WebRtc_Word16 oldLagQ7, oldGainQ12,
      lagdeltaQ7, curLagQ7,
      gaindeltaQ12, curGainQ12;
  WebRtc_Word16 tmpW16, indW16=0, frcQQ, cnt=0, pos, pos2;
  const WebRtc_Word16 *fracoeffQQ=NULL;
  WebRtc_Word32 tmpW32;

  if (type==4)
    sign = -1;

  
  memcpy(ubufQQ, pfp->ubufQQ, WEBRTC_SPL_MUL_16_16(sizeof(WebRtc_Word16), PITCH_BUFFSIZE));
  memcpy(inystateQQ, pfp->ystateQQ, WEBRTC_SPL_MUL_16_16(sizeof(WebRtc_Word16), PITCH_DAMPORDER));

  
  oldLagQ7 = pfp->oldlagQ7;
  oldGainQ12 = pfp->oldgainQ12;

  if (type==4) {
    
    
    for (k=0;k<PITCH_SUBFRAMES;k++)
      gainsQ12[k] = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(gainsQ12[k], Gain, 14);
  }

  
  if ((WEBRTC_SPL_RSHIFT_W16(WEBRTC_SPL_MUL_16_16(lagsQ7[0], 3), 1) < oldLagQ7) ||
      (lagsQ7[0] > WEBRTC_SPL_RSHIFT_W16(WEBRTC_SPL_MUL_16_16(oldLagQ7, 3), 1))) {
    oldLagQ7 = lagsQ7[0];
    oldGainQ12 = gainsQ12[0];
  }

  ind=0;
  for (k=0;k<PITCH_SUBFRAMES;k++) {

    
    lagdeltaQ7 = lagsQ7[k]-oldLagQ7;
    lagdeltaQ7 = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(lagdeltaQ7,DivFactor,15);
    curLagQ7 = oldLagQ7;
    gaindeltaQ12 = gainsQ12[k]-oldGainQ12;
    gaindeltaQ12 = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(gaindeltaQ12,DivFactor,15);

    curGainQ12 = oldGainQ12;
    oldLagQ7 = lagsQ7[k];
    oldGainQ12 = gainsQ12[k];


    for (n=0;n<PITCH_SUBFRAME_LEN;n++) {

      if (cnt == 0) {   

        curGainQ12 += gaindeltaQ12;
        curLagQ7 += lagdeltaQ7;
        indW16 = (WebRtc_Word16)CalcLrIntQ(curLagQ7,7);
        tmpW16 = WEBRTC_SPL_LSHIFT_W16(indW16,7);
        tmpW16 -= curLagQ7;
        frcQQ = WEBRTC_SPL_RSHIFT_W16(tmpW16,4);
        frcQQ += 4;

        if(frcQQ==PITCH_FRACS)
          frcQQ=0;
        fracoeffQQ = kIntrpCoef[frcQQ];

        cnt=12;
      }

      
      for (m=PITCH_DAMPORDER-1;m>0;m--)
        inystateQQ[m] = inystateQQ[m-1];

      
      pos = ind + PITCH_BUFFSIZE;
      pos2 = pos - (indW16 + 2);

      tmpW32=0;
      for (m=0;m<PITCH_FRACORDER;m++)
        tmpW32 += WEBRTC_SPL_MUL_16_16(ubufQQ[pos2+m], fracoeffQQ[m]);

      
      tmpW32 = WEBRTC_SPL_SAT(536862719, tmpW32, -536879104);
      tmpW32 += 8192;
      tmpW16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmpW32,14);

      inystateQQ[0] = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(curGainQ12, tmpW16,12); 

      
      tmpW32=0;
      for (m=0;m<PITCH_DAMPORDER;m++)
        tmpW32 += WEBRTC_SPL_MUL_16_16(inystateQQ[m], kDampFilter[m]);

      
      tmpW32 = WEBRTC_SPL_SAT(1073725439, tmpW32, -1073758208);
      tmpW32 += 16384;
      tmpW16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmpW32,15);

      
      tmpW32 = indatQQ[ind] - WEBRTC_SPL_MUL_16_16(sign, tmpW16);
      outdatQQ[ind] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmpW32);
      tmpW32 = indatQQ[ind] + (WebRtc_Word32)outdatQQ[ind];
      ubufQQ[pos] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmpW32);

      ind++;
      cnt--;
    }
  }


  
  memcpy(pfp->ubufQQ, ubufQQ+PITCH_FRAME_LEN, WEBRTC_SPL_MUL_16_16(sizeof(WebRtc_Word16), PITCH_BUFFSIZE));
  memcpy(pfp->ystateQQ, inystateQQ, WEBRTC_SPL_MUL_16_16(sizeof(WebRtc_Word16), PITCH_DAMPORDER));

  pfp->oldlagQ7 = oldLagQ7;
  pfp->oldgainQ12 = oldGainQ12;

  if (type==2) {
    
    for (n=0;n<QLOOKAHEAD;n++) {
      
      for (m=PITCH_DAMPORDER-1;m>0;m--)
        inystateQQ[m] = inystateQQ[m-1];

      
      pos = ind + PITCH_BUFFSIZE;
      pos2= pos - (indW16 + 2);

      tmpW32=0;
      for (m=0;m<PITCH_FRACORDER;m++)
        tmpW32 += WEBRTC_SPL_MUL_16_16(ubufQQ[pos2+m], fracoeffQQ[m]);

      if (tmpW32<536862720) {
        tmpW32 += 8192;
        tmpW16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmpW32,14);
      } else
        tmpW16= 32767;
      inystateQQ[0] = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(curGainQ12, tmpW16,12);  

      
      tmpW32=0;
      for (m=0;m<PITCH_DAMPORDER;m++)
        tmpW32 += WEBRTC_SPL_MUL_16_16(inystateQQ[m], kDampFilter[m]);
      if (tmpW32<1073725440) { 
        tmpW32 += 16384;
        tmpW16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmpW32,15);
      } else
        tmpW16 = 32767;

      
      tmpW32 = indatQQ[ind] - (WebRtc_Word32)tmpW16;
      outdatQQ[ind] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmpW32);
      tmpW32 = indatQQ[ind] + (WebRtc_Word32)outdatQQ[ind];
      ubufQQ[pos] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmpW32);

      ind++;
    }

  }


}


void WebRtcIsacfix_PitchFilterGains(const WebRtc_Word16 *indatQ0,
                                    PitchFiltstr *pfp,
                                    WebRtc_Word16 *lagsQ7,
                                    WebRtc_Word16 *gainsQ12)
{
  int  k, n, m, ind;

  WebRtc_Word16 ubufQQ[PITCH_INTBUFFSIZE];
  WebRtc_Word16 oldLagQ7,lagdeltaQ7, curLagQ7;
  WebRtc_Word16 DivFactor = 6553;
  const WebRtc_Word16 *fracoeffQQ = NULL;
  WebRtc_Word16 scale;
  WebRtc_Word16 cnt=0, pos, pos3QQ, frcQQ, indW16 = 0, tmpW16;
  WebRtc_Word32 tmpW32, tmp2W32, csum1QQ, esumxQQ;

  
  memcpy(ubufQQ, pfp->ubufQQ, WEBRTC_SPL_MUL_16_16(sizeof(WebRtc_Word16), PITCH_BUFFSIZE));
  oldLagQ7 = pfp->oldlagQ7;

  
  if ((WEBRTC_SPL_RSHIFT_W16(WEBRTC_SPL_MUL_16_16(lagsQ7[0], 3), 1) < oldLagQ7) ||
      (lagsQ7[0] > WEBRTC_SPL_RSHIFT_W16(WEBRTC_SPL_MUL_16_16(oldLagQ7, 3), 1))) {
    oldLagQ7 = lagsQ7[0];
  }

  ind=0;
  scale=0;
  for (k=0;k<PITCH_SUBFRAMES;k++) {

    
    lagdeltaQ7 = lagsQ7[k]-oldLagQ7;
    lagdeltaQ7 = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(lagdeltaQ7,DivFactor,15);
    curLagQ7 = oldLagQ7;
    oldLagQ7 = lagsQ7[k];

    csum1QQ=1;
    esumxQQ=1;

    for (n=0;n<PITCH_SUBFRAME_LEN;n++) {

      if (cnt == 0) {   
        curLagQ7 += lagdeltaQ7;
        indW16 = (WebRtc_Word16)CalcLrIntQ(curLagQ7,7);
        tmpW16 = WEBRTC_SPL_LSHIFT_W16(indW16,7);
        tmpW16 -= curLagQ7;
        frcQQ = WEBRTC_SPL_RSHIFT_W16(tmpW16,4);
        frcQQ += 4;

        if(frcQQ==PITCH_FRACS)
          frcQQ=0;
        fracoeffQQ = kIntrpCoef[frcQQ];

        cnt=12;
      }

      
      pos = ind + PITCH_BUFFSIZE;
      pos3QQ = pos - (indW16 + 4);

      tmpW32=0;
      for (m=0;m<PITCH_FRACORDER;m++){
        tmpW32 += WEBRTC_SPL_MUL_16_16(ubufQQ[pos3QQ+m], fracoeffQQ[m]);
      }

      
      ubufQQ[pos] = indatQ0[ind];

      tmp2W32 = WEBRTC_SPL_MUL_16_32_RSFT14(indatQ0[ind], tmpW32);
      tmpW32 += 8192;
      tmpW16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmpW32,14);
      tmpW32 = WEBRTC_SPL_MUL_16_16(tmpW16, tmpW16);

      if ((tmp2W32>1073700000) || (csum1QQ>1073700000) || (tmpW32>1073700000) || (esumxQQ>1073700000)) {
        scale++;
        csum1QQ = WEBRTC_SPL_RSHIFT_W32(csum1QQ,1);
        esumxQQ = WEBRTC_SPL_RSHIFT_W32(esumxQQ,1);
      }
      tmp2W32 = WEBRTC_SPL_RSHIFT_W32(tmp2W32,scale);
      csum1QQ += tmp2W32;
      tmpW32 = WEBRTC_SPL_RSHIFT_W32(tmpW32,scale);
      esumxQQ += tmpW32;

      ind++;
      cnt--;
    }

    if (csum1QQ<esumxQQ) {
      tmp2W32=WebRtcSpl_DivResultInQ31(csum1QQ,esumxQQ);

      
      tmpW32=WEBRTC_SPL_RSHIFT_W32(tmp2W32,20);
    } else
      tmpW32=4096;
    gainsQ12[k]=(WebRtc_Word16)WEBRTC_SPL_SAT(PITCH_MAX_GAIN_Q12, tmpW32, 0);

  }

  
  memcpy(pfp->ubufQQ, ubufQQ+PITCH_FRAME_LEN, WEBRTC_SPL_MUL_16_16(sizeof(WebRtc_Word16), PITCH_BUFFSIZE));
  pfp->oldlagQ7 = lagsQ7[PITCH_SUBFRAMES-1];
  pfp->oldgainQ12 = gainsQ12[PITCH_SUBFRAMES-1];

}
