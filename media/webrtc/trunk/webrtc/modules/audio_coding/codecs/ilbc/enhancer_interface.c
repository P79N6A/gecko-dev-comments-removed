

















#include "defines.h"
#include "constants.h"
#include "xcorr_coef.h"
#include "enhancer.h"
#include "hp_output.h"







int WebRtcIlbcfix_EnhancerInterface( 
    WebRtc_Word16 *out,     
    WebRtc_Word16 *in,      
    iLBC_Dec_Inst_t *iLBCdec_inst 
                                        ){
  int iblock;
  int lag=20, tlag=20;
  int inLen=iLBCdec_inst->blockl+120;
  WebRtc_Word16 scale, scale1, plc_blockl;
  WebRtc_Word16 *enh_buf, *enh_period;
  WebRtc_Word32 tmp1, tmp2, max, new_blocks;
  WebRtc_Word16 *enh_bufPtr1;
  int i, k;
  WebRtc_Word16 EnChange;
  WebRtc_Word16 SqrtEnChange;
  WebRtc_Word16 inc;
  WebRtc_Word16 win;
  WebRtc_Word16 *tmpW16ptr;
  WebRtc_Word16 startPos;
  WebRtc_Word16 *plc_pred;
  WebRtc_Word16 *target, *regressor;
  WebRtc_Word16 max16;
  int shifts;
  WebRtc_Word32 ener;
  WebRtc_Word16 enerSh;
  WebRtc_Word16 corrSh;
  WebRtc_Word16 ind, sh;
  WebRtc_Word16 start, stop;
  
  WebRtc_Word16 totsh[3];
  WebRtc_Word16 downsampled[(BLOCKL_MAX+120)>>1]; 
  WebRtc_Word32 corr32[50];
  WebRtc_Word32 corrmax[3];
  WebRtc_Word16 corr16[3];
  WebRtc_Word16 en16[3];
  WebRtc_Word16 lagmax[3];

  plc_pred = downsampled; 

  enh_buf=iLBCdec_inst->enh_buf;
  enh_period=iLBCdec_inst->enh_period;

  

  WEBRTC_SPL_MEMMOVE_W16(enh_buf, &enh_buf[iLBCdec_inst->blockl],
                         ENH_BUFL-iLBCdec_inst->blockl);

  WEBRTC_SPL_MEMCPY_W16(&enh_buf[ENH_BUFL-iLBCdec_inst->blockl], in,
                        iLBCdec_inst->blockl);

  
  if (iLBCdec_inst->mode==30) {
    plc_blockl=ENH_BLOCKL;
    new_blocks=3;
    startPos=320;  

  } else {
    plc_blockl=40;
    new_blocks=2;
    startPos=440;  

  }

  
  WEBRTC_SPL_MEMMOVE_W16(enh_period, &enh_period[new_blocks],
                         (ENH_NBLOCKS_TOT-new_blocks));

  k=WebRtcSpl_DownsampleFast(
      enh_buf+ENH_BUFL-inLen,    
      (WebRtc_Word16)(inLen+ENH_BUFL_FILTEROVERHEAD),
      downsampled,
      (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W16(inLen, 1),
      (WebRtc_Word16*)WebRtcIlbcfix_kLpFiltCoefs,  
      FILTERORDER_DS_PLUS1,    
      FACTOR_DS,
      DELAY_DS);

  
  for(iblock = 0; iblock<new_blocks; iblock++){

    
    i=60+WEBRTC_SPL_MUL_16_16(iblock,ENH_BLOCKL_HALF);
    target=downsampled+i;
    regressor=downsampled+i-10;

    
    max16=WebRtcSpl_MaxAbsValueW16(&regressor[-50],
                                   (WebRtc_Word16)(ENH_BLOCKL_HALF+50-1));
    shifts = WebRtcSpl_GetSizeInBits(WEBRTC_SPL_MUL_16_16(max16, max16)) - 25;
    shifts = WEBRTC_SPL_MAX(0, shifts);

    
    WebRtcSpl_CrossCorrelation(corr32, target, regressor,
                               ENH_BLOCKL_HALF, 50, (WebRtc_Word16)shifts, -1);

    


    for (i=0;i<2;i++) {
      lagmax[i] = WebRtcSpl_MaxIndexW32(corr32, 50);
      corrmax[i] = corr32[lagmax[i]];
      start = lagmax[i] - 2;
      stop = lagmax[i] + 2;
      start = WEBRTC_SPL_MAX(0,  start);
      stop  = WEBRTC_SPL_MIN(49, stop);
      for (k=start; k<=stop; k++) {
        corr32[k] = 0;
      }
    }
    lagmax[2] = WebRtcSpl_MaxIndexW32(corr32, 50);
    corrmax[2] = corr32[lagmax[2]];

    
    for (i=0;i<3;i++) {
      corrSh = 15-WebRtcSpl_GetSizeInBits(corrmax[i]);
      ener = WebRtcSpl_DotProductWithScale(&regressor[-lagmax[i]],
                                           &regressor[-lagmax[i]],
                                           ENH_BLOCKL_HALF, shifts);
      enerSh = 15-WebRtcSpl_GetSizeInBits(ener);
      corr16[i] = (WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(corrmax[i], corrSh);
      corr16[i] = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(corr16[i],
                                                           corr16[i], 16);
      en16[i] = (WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(ener, enerSh);
      totsh[i] = enerSh - WEBRTC_SPL_LSHIFT_W32(corrSh, 1);
    }

    
    ind = 0;
    for (i=1; i<3; i++) {
      if (totsh[ind] > totsh[i]) {
        sh = WEBRTC_SPL_MIN(31, totsh[ind]-totsh[i]);
        if ( WEBRTC_SPL_MUL_16_16(corr16[ind], en16[i]) <
            WEBRTC_SPL_MUL_16_16_RSFT(corr16[i], en16[ind], sh)) {
          ind = i;
        }
      } else {
        sh = WEBRTC_SPL_MIN(31, totsh[i]-totsh[ind]);
        if (WEBRTC_SPL_MUL_16_16_RSFT(corr16[ind], en16[i], sh) <
            WEBRTC_SPL_MUL_16_16(corr16[i], en16[ind])) {
          ind = i;
        }
      }
    }

    lag = lagmax[ind] + 10;

    
    enh_period[ENH_NBLOCKS_TOT-new_blocks+iblock] =
        (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(lag, 8);

    
    if (iLBCdec_inst->prev_enh_pl==1) {
      if (!iblock) {
        tlag = WEBRTC_SPL_MUL_16_16(lag, 2);
      }
    } else {
      if (iblock==1) {
        tlag = WEBRTC_SPL_MUL_16_16(lag, 2);
      }
    }

    lag = WEBRTC_SPL_MUL_16_16(lag, 2);
  }

  if ((iLBCdec_inst->prev_enh_pl==1)||(iLBCdec_inst->prev_enh_pl==2)) {

    



    
    target=in;
    regressor=in+tlag-1;

    
    max16=WebRtcSpl_MaxAbsValueW16(regressor, (WebRtc_Word16)(plc_blockl+3-1));
    if (max16>5000)
      shifts=2;
    else
      shifts=0;

    
    WebRtcSpl_CrossCorrelation(corr32, target, regressor,
                               plc_blockl, 3, (WebRtc_Word16)shifts, 1);

    
    lag=WebRtcSpl_MaxIndexW32(corr32, 3);
    lag+=tlag-1;

    

    if (iLBCdec_inst->prev_enh_pl==1) {
      if (lag>plc_blockl) {
        WEBRTC_SPL_MEMCPY_W16(plc_pred, &in[lag-plc_blockl], plc_blockl);
      } else {
        WEBRTC_SPL_MEMCPY_W16(&plc_pred[plc_blockl-lag], in, lag);
        WEBRTC_SPL_MEMCPY_W16(
            plc_pred, &enh_buf[ENH_BUFL-iLBCdec_inst->blockl-plc_blockl+lag],
            (plc_blockl-lag));
      }
    } else {
      int pos;

      pos = plc_blockl;

      while (lag<pos) {
        WEBRTC_SPL_MEMCPY_W16(&plc_pred[pos-lag], in, lag);
        pos = pos - lag;
      }
      WEBRTC_SPL_MEMCPY_W16(plc_pred, &in[lag-pos], pos);

    }

    if (iLBCdec_inst->prev_enh_pl==1) {
      










      max=WebRtcSpl_MaxAbsValueW16(
          &enh_buf[ENH_BUFL-iLBCdec_inst->blockl-plc_blockl], plc_blockl);
      max16=WebRtcSpl_MaxAbsValueW16(plc_pred, plc_blockl);
      max = WEBRTC_SPL_MAX(max, max16);
      scale=22-(WebRtc_Word16)WebRtcSpl_NormW32(max);
      scale=WEBRTC_SPL_MAX(scale,0);

      tmp2 = WebRtcSpl_DotProductWithScale(
          &enh_buf[ENH_BUFL-iLBCdec_inst->blockl-plc_blockl],
          &enh_buf[ENH_BUFL-iLBCdec_inst->blockl-plc_blockl],
          plc_blockl, scale);
      tmp1 = WebRtcSpl_DotProductWithScale(plc_pred, plc_pred,
                                           plc_blockl, scale);

      
      if ((tmp1>0)&&((tmp1>>2)>tmp2)) {
        



        scale1=(WebRtc_Word16)WebRtcSpl_NormW32(tmp1);
        tmp1=WEBRTC_SPL_SHIFT_W32(tmp1, (scale1-16)); 

        tmp2=WEBRTC_SPL_SHIFT_W32(tmp2, (scale1));
        EnChange = (WebRtc_Word16)WebRtcSpl_DivW32W16(tmp2,
                                                      (WebRtc_Word16)tmp1);

        
        SqrtEnChange = (WebRtc_Word16)WebRtcSpl_SqrtFloor(
            WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)EnChange, 14));


        
        WebRtcSpl_ScaleVector(plc_pred, plc_pred, SqrtEnChange,
                              (WebRtc_Word16)(plc_blockl-16), 14);

        
        
        inc=(2048-WEBRTC_SPL_RSHIFT_W16(SqrtEnChange, 3));

        win=0;
        tmpW16ptr=&plc_pred[plc_blockl-16];

        for (i=16;i>0;i--) {
          (*tmpW16ptr)=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(
              (*tmpW16ptr), (SqrtEnChange+(win>>1)), 14);
          

          win += inc;
          tmpW16ptr++;
        }
      }

      



      if (plc_blockl==40) {
        inc=400; 
      } else { 
        inc=202; 
      }
      win=0;
      enh_bufPtr1=&enh_buf[ENH_BUFL-1-iLBCdec_inst->blockl];
      for (i=0; i<plc_blockl; i++) {
        win+=inc;
        *enh_bufPtr1 =
            (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT((*enh_bufPtr1), win, 14);
        *enh_bufPtr1 += (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(
                (16384-win), plc_pred[plc_blockl-1-i], 14);
        enh_bufPtr1--;
      }
    } else {
      WebRtc_Word16 *synt = &downsampled[LPC_FILTERORDER];

      enh_bufPtr1=&enh_buf[ENH_BUFL-iLBCdec_inst->blockl-plc_blockl];
      WEBRTC_SPL_MEMCPY_W16(enh_bufPtr1, plc_pred, plc_blockl);

      
      WebRtcSpl_MemSetW16(iLBCdec_inst->syntMem, 0, LPC_FILTERORDER);
      WebRtcSpl_MemSetW16(iLBCdec_inst->hpimemy, 0, 4);
      WebRtcSpl_MemSetW16(iLBCdec_inst->hpimemx, 0, 2);

      
      WEBRTC_SPL_MEMCPY_W16(&synt[-LPC_FILTERORDER], iLBCdec_inst->syntMem,
                            LPC_FILTERORDER);
      WebRtcSpl_FilterARFastQ12(
          enh_bufPtr1,
          synt,
          &iLBCdec_inst->old_syntdenum[
                                       (iLBCdec_inst->nsub-1)*(LPC_FILTERORDER+1)],
                                       LPC_FILTERORDER+1, (WebRtc_Word16)lag);

      WEBRTC_SPL_MEMCPY_W16(&synt[-LPC_FILTERORDER], &synt[lag-LPC_FILTERORDER],
                            LPC_FILTERORDER);
      WebRtcIlbcfix_HpOutput(synt, (WebRtc_Word16*)WebRtcIlbcfix_kHpOutCoefs,
                             iLBCdec_inst->hpimemy, iLBCdec_inst->hpimemx,
                             (WebRtc_Word16)lag);
      WebRtcSpl_FilterARFastQ12(
          enh_bufPtr1, synt,
          &iLBCdec_inst->old_syntdenum[
                                       (iLBCdec_inst->nsub-1)*(LPC_FILTERORDER+1)],
                                       LPC_FILTERORDER+1, (WebRtc_Word16)lag);

      WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->syntMem, &synt[lag-LPC_FILTERORDER],
                            LPC_FILTERORDER);
      WebRtcIlbcfix_HpOutput(synt, (WebRtc_Word16*)WebRtcIlbcfix_kHpOutCoefs,
                             iLBCdec_inst->hpimemy, iLBCdec_inst->hpimemx,
                             (WebRtc_Word16)lag);
    }
  }


  

  for (iblock = 0; iblock<new_blocks; iblock++) {
    WebRtcIlbcfix_Enhancer(out+WEBRTC_SPL_MUL_16_16(iblock, ENH_BLOCKL),
                           enh_buf,
                           ENH_BUFL,
                           (WebRtc_Word16)(WEBRTC_SPL_MUL_16_16(iblock, ENH_BLOCKL)+startPos),
                           enh_period,
                           (WebRtc_Word16*)WebRtcIlbcfix_kEnhPlocs, ENH_NBLOCKS_TOT);
  }

  return (lag);
}
