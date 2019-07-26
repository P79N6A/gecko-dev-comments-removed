



















#include "bandwidth_estimator.h"
#include "settings.h"




static const WebRtc_Word16 kQRateTable[12] = {
  10000, 11115, 12355, 13733, 15265, 16967,
  18860, 20963, 23301, 25900, 28789, 32000
};



static const WebRtc_Word32 KQRate01[12] = {
  65536000,  72843264,  80969728,  90000589,  100040704, 111194931,
  123600896, 137383117, 152705434, 169738240, 188671590, 209715200
};





static const WebRtc_Word32 kBitsByteSec = 4369000;


static const WebRtc_Word16 kRecHeaderRate[2] = {
  9333, 4666
};





static const WebRtc_Word32 kInvBandwidth[4] = {
  55539, 25978,
  73213, 29284
};


static const WebRtc_Word32 kSamplesIn25msec = 400;












WebRtc_Word32 WebRtcIsacfix_InitBandwidthEstimator(BwEstimatorstr *bweStr)
{
  bweStr->prevFrameSizeMs       = INIT_FRAME_LEN;
  bweStr->prevRtpNumber         = 0;
  bweStr->prevSendTime          = 0;
  bweStr->prevArrivalTime       = 0;
  bweStr->prevRtpRate           = 1;
  bweStr->lastUpdate            = 0;
  bweStr->lastReduction         = 0;
  bweStr->countUpdates          = -9;

  







  bweStr->recBwInv              = 43531;
  bweStr->recBw                 = INIT_BN_EST;
  bweStr->recBwAvgQ             = INIT_BN_EST_Q7;
  bweStr->recBwAvg              = INIT_REC_BN_EST_Q5;
  bweStr->recJitter             = (WebRtc_Word32) 327680;   
  bweStr->recJitterShortTerm    = 0;
  bweStr->recJitterShortTermAbs = (WebRtc_Word32) 40960;    
  bweStr->recMaxDelay           = (WebRtc_Word32) 10;
  bweStr->recMaxDelayAvgQ       = (WebRtc_Word32) 5120;     
  bweStr->recHeaderRate         = INIT_HDR_RATE;
  bweStr->countRecPkts          = 0;
  bweStr->sendBwAvg             = INIT_BN_EST_Q7;
  bweStr->sendMaxDelayAvg       = (WebRtc_Word32) 5120;     

  bweStr->countHighSpeedRec     = 0;
  bweStr->highSpeedRec          = 0;
  bweStr->countHighSpeedSent    = 0;
  bweStr->highSpeedSend         = 0;
  bweStr->inWaitPeriod          = 0;

  



  bweStr->maxBwInv              = kInvBandwidth[3];
  bweStr->minBwInv              = kInvBandwidth[2];

  return 0;
}





















WebRtc_Word32 WebRtcIsacfix_UpdateUplinkBwImpl(BwEstimatorstr *bweStr,
                                               const WebRtc_UWord16 rtpNumber,
                                               const WebRtc_Word16  frameSize,
                                               const WebRtc_UWord32 sendTime,
                                               const WebRtc_UWord32 arrivalTime,
                                               const WebRtc_Word16  pksize,
                                               const WebRtc_UWord16 Index)
{
  WebRtc_UWord16  weight = 0;
  WebRtc_UWord32  currBwInv = 0;
  WebRtc_UWord16  recRtpRate;
  WebRtc_UWord32  arrTimeProj;
  WebRtc_Word32   arrTimeDiff;
  WebRtc_Word32   arrTimeNoise;
  WebRtc_Word32   arrTimeNoiseAbs;
  WebRtc_Word32   sendTimeDiff;

  WebRtc_Word32 delayCorrFactor = DELAY_CORRECTION_MED;
  WebRtc_Word32 lateDiff = 0;
  WebRtc_Word16 immediateSet = 0;
  WebRtc_Word32 frameSizeSampl;

  WebRtc_Word32  temp;
  WebRtc_Word32  msec;
  WebRtc_UWord32 exponent;
  WebRtc_UWord32 reductionFactor;
  WebRtc_UWord32 numBytesInv;
  WebRtc_Word32  sign;

  WebRtc_UWord32 byteSecondsPerBit;
  WebRtc_UWord32 tempLower;
  WebRtc_UWord32 tempUpper;
  WebRtc_Word32 recBwAvgInv;
  WebRtc_Word32 numPktsExpected;

  WebRtc_Word16 errCode;

  

  
  errCode = WebRtcIsacfix_UpdateUplinkBwRec(bweStr, Index);
  if (errCode <0) {
    return(errCode);
  }


  

  
  if (frameSize == 60) {
    
    if ( (frameSize != bweStr->prevFrameSizeMs) && (bweStr->countUpdates > 0)) {
      bweStr->countUpdates = 10;
      bweStr->recHeaderRate = kRecHeaderRate[1];

      bweStr->maxBwInv = kInvBandwidth[3];
      bweStr->minBwInv = kInvBandwidth[2];
      bweStr->recBwInv = WEBRTC_SPL_UDIV(1073741824, (bweStr->recBw + bweStr->recHeaderRate));
    }

    
    recRtpRate = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL(kBitsByteSec,
                                                                     (WebRtc_Word32)pksize), 15) + bweStr->recHeaderRate;

  } else {
    
    if ( (frameSize != bweStr->prevFrameSizeMs) && (bweStr->countUpdates > 0)) {
      bweStr->countUpdates = 10;
      bweStr->recHeaderRate = kRecHeaderRate[0];

      bweStr->maxBwInv = kInvBandwidth[1];
      bweStr->minBwInv = kInvBandwidth[0];
      bweStr->recBwInv = WEBRTC_SPL_UDIV(1073741824, (bweStr->recBw + bweStr->recHeaderRate));
    }

    
    recRtpRate = (WebRtc_UWord16)WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL(kBitsByteSec,
                                                                      (WebRtc_Word32)pksize), 14) + bweStr->recHeaderRate;
  }


  
  if (arrivalTime < bweStr->prevArrivalTime) {
    bweStr->prevArrivalTime = arrivalTime;
    bweStr->lastUpdate      = arrivalTime;
    bweStr->lastReduction   = arrivalTime + FS3;

    bweStr->countRecPkts      = 0;

    
    bweStr->prevFrameSizeMs = frameSize;

    
    bweStr->prevRtpRate = recRtpRate;

    
    bweStr->prevRtpNumber = rtpNumber;

    return 0;
  }

  bweStr->countRecPkts++;

  
  frameSizeSampl = WEBRTC_SPL_MUL_16_16((WebRtc_Word16)SAMPLES_PER_MSEC, frameSize);

  
  if ( bweStr->countUpdates > 0 ) {

    
    if(bweStr->inWaitPeriod) {
      if ((arrivalTime - bweStr->startWaitPeriod)> FS_1_HALF) {
        bweStr->inWaitPeriod = 0;
      }
    }

    

    
    sendTimeDiff = sendTime - bweStr->prevSendTime;
    if (sendTimeDiff <= WEBRTC_SPL_LSHIFT_W32(frameSizeSampl, 1)) {

      
      if ((arrivalTime - bweStr->lastUpdate) > FS3) {

        
        numPktsExpected =  WEBRTC_SPL_UDIV(arrivalTime - bweStr->lastUpdate, frameSizeSampl);

        
        
        if(WEBRTC_SPL_LSHIFT_W32(bweStr->countRecPkts, 10)  > WEBRTC_SPL_MUL_16_16(922, numPktsExpected)) {
          
          msec = (arrivalTime - bweStr->lastReduction);

          

          if (msec > 208000) {
            msec = 208000;
          }

          

          exponent = WEBRTC_SPL_UMUL(0x0000004C, msec);

          

          reductionFactor = WEBRTC_SPL_RSHIFT_U32(0x01000000 | (exponent & 0x00FFFFFF),
                                                  WEBRTC_SPL_RSHIFT_U32(exponent, 24));

          
          reductionFactor = WEBRTC_SPL_RSHIFT_U32(reductionFactor, 11);

          if ( reductionFactor != 0 ) {
            bweStr->recBwInv = WEBRTC_SPL_MUL((WebRtc_Word32)bweStr->recBwInv, (WebRtc_Word32)reductionFactor);
            bweStr->recBwInv = WEBRTC_SPL_RSHIFT_W32((WebRtc_Word32)bweStr->recBwInv, 13);

          } else {
            
            bweStr->recBwInv = WEBRTC_SPL_DIV((1073741824 +
                                               WEBRTC_SPL_LSHIFT_W32(((WebRtc_Word32)INIT_BN_EST + INIT_HDR_RATE), 1)), INIT_BN_EST + INIT_HDR_RATE);
          }

          
          bweStr->lastReduction = arrivalTime;
        } else {
          
          bweStr->lastReduction = arrivalTime + FS3;
          bweStr->lastUpdate    = arrivalTime;
          bweStr->countRecPkts  = 0;
        }
      }
    } else {
      bweStr->lastReduction = arrivalTime + FS3;
      bweStr->lastUpdate    = arrivalTime;
      bweStr->countRecPkts  = 0;
    }


    
    if ( rtpNumber == bweStr->prevRtpNumber + 1 ) {
      arrTimeDiff = arrivalTime - bweStr->prevArrivalTime;

      if (!(bweStr->highSpeedSend && bweStr->highSpeedRec)) {
        if (arrTimeDiff > frameSizeSampl) {
          if (sendTimeDiff > 0) {
            lateDiff = arrTimeDiff - sendTimeDiff -
                WEBRTC_SPL_LSHIFT_W32(frameSizeSampl, 1);
          } else {
            lateDiff = arrTimeDiff - frameSizeSampl;
          }

          
          if (lateDiff > 8000) {
            delayCorrFactor = (WebRtc_Word32) DELAY_CORRECTION_MAX;
            bweStr->inWaitPeriod = 1;
            bweStr->startWaitPeriod = arrivalTime;
            immediateSet = 1;
          } else if (lateDiff > 5120) {
            delayCorrFactor = (WebRtc_Word32) DELAY_CORRECTION_MED;
            immediateSet = 1;
            bweStr->inWaitPeriod = 1;
            bweStr->startWaitPeriod = arrivalTime;
          }
        }
      }

      if ((bweStr->prevRtpRate > WEBRTC_SPL_RSHIFT_W32((WebRtc_Word32) bweStr->recBwAvg, 5)) &&
          (recRtpRate > WEBRTC_SPL_RSHIFT_W32((WebRtc_Word32)bweStr->recBwAvg, 5)) &&
          !bweStr->inWaitPeriod) {

        
        if (bweStr->countUpdates++ > 99) {
          
          weight = (WebRtc_UWord16) 82;
        } else {
          
          weight = (WebRtc_UWord16) WebRtcSpl_DivW32W16(
              (WebRtc_Word32)(8192 + WEBRTC_SPL_RSHIFT_W32((WebRtc_Word32) bweStr->countUpdates, 1)),
              (WebRtc_Word16)bweStr->countUpdates);
        }

        

        
        if (arrTimeDiff > frameSizeSampl + kSamplesIn25msec) {
          arrTimeDiff = frameSizeSampl + kSamplesIn25msec;
        }

        
        if (arrTimeDiff < frameSizeSampl - FRAMESAMPLES_10ms) {
          arrTimeDiff = frameSizeSampl - FRAMESAMPLES_10ms;
        }

        
        numBytesInv = (WebRtc_UWord16) WebRtcSpl_DivW32W16(
            (WebRtc_Word32)(524288 + WEBRTC_SPL_RSHIFT_W32(((WebRtc_Word32)pksize + HEADER_SIZE), 1)),
            (WebRtc_Word16)(pksize + HEADER_SIZE));

        
        byteSecondsPerBit = WEBRTC_SPL_MUL_16_16(arrTimeDiff, 8389);

        
        tempUpper = WEBRTC_SPL_RSHIFT_U32(byteSecondsPerBit, 15);

        
        tempLower = byteSecondsPerBit & 0x00007FFF;

        tempUpper = WEBRTC_SPL_MUL(tempUpper, numBytesInv);
        tempLower = WEBRTC_SPL_MUL(tempLower, numBytesInv);
        tempLower = WEBRTC_SPL_RSHIFT_U32(tempLower, 15);

        currBwInv = tempUpper + tempLower;
        currBwInv = WEBRTC_SPL_RSHIFT_U32(currBwInv, 4);

        
        if(currBwInv < bweStr->maxBwInv) {
          currBwInv = bweStr->maxBwInv;
        } else if(currBwInv > bweStr->minBwInv) {
          currBwInv = bweStr->minBwInv;
        }

        
        bweStr->recBwInv = WEBRTC_SPL_UMUL(weight, currBwInv) +
            WEBRTC_SPL_UMUL((WebRtc_UWord32) 8192 - weight, bweStr->recBwInv);

        

        bweStr->recBwInv = WEBRTC_SPL_RSHIFT_U32(bweStr->recBwInv, 13);

        
        bweStr->lastUpdate    = arrivalTime;
        bweStr->lastReduction = arrivalTime + FS3;
        bweStr->countRecPkts  = 0;

        



        recBwAvgInv = WEBRTC_SPL_UDIV((WebRtc_UWord32)(0x80000000 + WEBRTC_SPL_RSHIFT_U32(bweStr->recBwAvg, 1)),
                                      bweStr->recBwAvg);

        

        

        arrTimeProj = WEBRTC_SPL_MUL((WebRtc_Word32)8000, recBwAvgInv);
        
        arrTimeProj = WEBRTC_SPL_RSHIFT_U32(arrTimeProj, 4);
        
        arrTimeProj = WEBRTC_SPL_MUL(((WebRtc_Word32)pksize + HEADER_SIZE), arrTimeProj);
        
        arrTimeProj = WEBRTC_SPL_RSHIFT_U32(arrTimeProj, 12);

        
        
        if (WEBRTC_SPL_LSHIFT_W32(arrTimeDiff, 6) > (WebRtc_Word32)arrTimeProj) {
          arrTimeNoise = WEBRTC_SPL_LSHIFT_W32(arrTimeDiff, 6) -  arrTimeProj;
          sign = 1;
        } else {
          arrTimeNoise = arrTimeProj - WEBRTC_SPL_LSHIFT_W32(arrTimeDiff, 6);
          sign = -1;
        }

        
        arrTimeNoiseAbs = arrTimeNoise;

        
        weight = WEBRTC_SPL_RSHIFT_W32(weight, 3);
        bweStr->recJitter = WEBRTC_SPL_MUL(weight, WEBRTC_SPL_LSHIFT_W32(arrTimeNoiseAbs, 5))
            +  WEBRTC_SPL_MUL(1024 - weight, bweStr->recJitter);

        
        bweStr->recJitter = WEBRTC_SPL_RSHIFT_W32(bweStr->recJitter, 10);

        
        if (bweStr->recJitter > (WebRtc_Word32)327680) {
          bweStr->recJitter = (WebRtc_Word32)327680;
        }

        
        
        bweStr->recJitterShortTermAbs = WEBRTC_SPL_MUL(51, WEBRTC_SPL_LSHIFT_W32(arrTimeNoiseAbs, 3)) +
            WEBRTC_SPL_MUL(973, bweStr->recJitterShortTermAbs);
        bweStr->recJitterShortTermAbs = WEBRTC_SPL_RSHIFT_W32(bweStr->recJitterShortTermAbs , 10);

        
        
        bweStr->recJitterShortTerm = WEBRTC_SPL_MUL(205, WEBRTC_SPL_LSHIFT_W32(arrTimeNoise, 3)) * sign +
            WEBRTC_SPL_MUL(3891, bweStr->recJitterShortTerm);

        if (bweStr->recJitterShortTerm < 0) {
          temp = -bweStr->recJitterShortTerm;
          temp = WEBRTC_SPL_RSHIFT_W32(temp, 12);
          bweStr->recJitterShortTerm = -temp;
        } else {
          bweStr->recJitterShortTerm = WEBRTC_SPL_RSHIFT_W32(bweStr->recJitterShortTerm, 12);
        }
      }
    }
  } else {
    
    bweStr->lastUpdate    = arrivalTime;
    bweStr->lastReduction = arrivalTime + FS3;
    bweStr->countRecPkts  = 0;
    bweStr->countUpdates++;
  }

  
  if (bweStr->recBwInv > bweStr->minBwInv) {
    bweStr->recBwInv = bweStr->minBwInv;
  } else if (bweStr->recBwInv < bweStr->maxBwInv) {
    bweStr->recBwInv = bweStr->maxBwInv;
  }


  
  bweStr->prevFrameSizeMs = frameSize;

  
  bweStr->prevRtpRate = recRtpRate;

  
  bweStr->prevRtpNumber = rtpNumber;

  
  if (bweStr->prevArrivalTime != 0xffffffff) {
    bweStr->recMaxDelay = WEBRTC_SPL_MUL(3, bweStr->recJitter);
  }

  
  bweStr->prevArrivalTime = arrivalTime;
  bweStr->prevSendTime = sendTime;

  
  bweStr->recBw = WEBRTC_SPL_UDIV(1073741824, bweStr->recBwInv) - bweStr->recHeaderRate;

  if (immediateSet) {
    
    bweStr->recBw = WEBRTC_SPL_UMUL(delayCorrFactor, bweStr->recBw);
    bweStr->recBw = WEBRTC_SPL_RSHIFT_U32(bweStr->recBw, 10);

    if (bweStr->recBw < (WebRtc_Word32) MIN_ISAC_BW) {
      bweStr->recBw = (WebRtc_Word32) MIN_ISAC_BW;
    }

    bweStr->recBwAvg = WEBRTC_SPL_LSHIFT_U32(bweStr->recBw + bweStr->recHeaderRate, 5);

    bweStr->recBwAvgQ = WEBRTC_SPL_LSHIFT_U32(bweStr->recBw, 7);

    bweStr->recJitterShortTerm = 0;

    bweStr->recBwInv = WEBRTC_SPL_UDIV(1073741824, bweStr->recBw + bweStr->recHeaderRate);

    immediateSet = 0;
  }


  return 0;
}




WebRtc_Word16 WebRtcIsacfix_UpdateUplinkBwRec(BwEstimatorstr *bweStr,
                                              const WebRtc_Word16 Index)
{
  WebRtc_UWord16 RateInd;

  if ( (Index < 0) || (Index > 23) ) {
    return -ISAC_RANGE_ERROR_BW_ESTIMATOR;
  }

  

  if ( Index > 11 ) {
    RateInd = Index - 12;
    
    
    bweStr->sendMaxDelayAvg = WEBRTC_SPL_MUL(461, bweStr->sendMaxDelayAvg) +
        WEBRTC_SPL_MUL(51, WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)MAX_ISAC_MD, 9));
    bweStr->sendMaxDelayAvg = WEBRTC_SPL_RSHIFT_W32(bweStr->sendMaxDelayAvg, 9);

  } else {
    RateInd = Index;
    
    
    bweStr->sendMaxDelayAvg = WEBRTC_SPL_MUL(461, bweStr->sendMaxDelayAvg) +
        WEBRTC_SPL_MUL(51, WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)MIN_ISAC_MD,9));
    bweStr->sendMaxDelayAvg = WEBRTC_SPL_RSHIFT_W32(bweStr->sendMaxDelayAvg, 9);

  }


  
  
  bweStr->sendBwAvg = WEBRTC_SPL_UMUL(461, bweStr->sendBwAvg) +
      WEBRTC_SPL_UMUL(51, WEBRTC_SPL_LSHIFT_U32(kQRateTable[RateInd], 7));
  bweStr->sendBwAvg = WEBRTC_SPL_RSHIFT_U32(bweStr->sendBwAvg, 9);


  if (WEBRTC_SPL_RSHIFT_U32(bweStr->sendBwAvg, 7) > 28000 && !bweStr->highSpeedSend) {
    bweStr->countHighSpeedSent++;

    
    if (bweStr->countHighSpeedSent >= 66) {
      bweStr->highSpeedSend = 1;
    }
  } else if (!bweStr->highSpeedSend) {
    bweStr->countHighSpeedSent = 0;
  }

  return 0;
}













WebRtc_UWord16 WebRtcIsacfix_GetDownlinkBwIndexImpl(BwEstimatorstr *bweStr)
{
  WebRtc_Word32  rate;
  WebRtc_Word32  maxDelay;
  WebRtc_UWord16 rateInd;
  WebRtc_UWord16 maxDelayBit;
  WebRtc_Word32  tempTerm1;
  WebRtc_Word32  tempTerm2;
  WebRtc_Word32  tempTermX;
  WebRtc_Word32  tempTermY;
  WebRtc_Word32  tempMin;
  WebRtc_Word32  tempMax;

  

  
  rate = WebRtcIsacfix_GetDownlinkBandwidth(bweStr);

  

  
  bweStr->recBwAvg = WEBRTC_SPL_UMUL(922, bweStr->recBwAvg) +
      WEBRTC_SPL_UMUL(102, WEBRTC_SPL_LSHIFT_U32((WebRtc_UWord32)rate + bweStr->recHeaderRate, 5));
  bweStr->recBwAvg = WEBRTC_SPL_RSHIFT_U32(bweStr->recBwAvg, 10);

  


  for (rateInd = 1; rateInd < 11; rateInd++) {
    if (rate <= kQRateTable[rateInd]){
      break;
    }
  }

  
  

  
  
  tempTerm1 = WEBRTC_SPL_MUL(bweStr->recBwAvgQ, 25);
  tempTerm1 = WEBRTC_SPL_RSHIFT_W32(tempTerm1, 7);
  tempTermX = WEBRTC_SPL_UMUL(461, bweStr->recBwAvgQ) - tempTerm1;

  
  tempTermY = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)rate, 16);

  
  tempTerm1 = tempTermX + KQRate01[rateInd] - tempTermY;
  tempTerm2 = tempTermY - tempTermX - KQRate01[rateInd-1];

  

  if (tempTerm1  > tempTerm2) {
    rateInd--;
  }

  
  

  
  tempTermX += KQRate01[rateInd];

  
  bweStr->recBwAvgQ = WEBRTC_SPL_RSHIFT_W32(tempTermX, 9);

  
  
  
  if ((bweStr->recBwAvgQ > 3584000) && !bweStr->highSpeedRec) {
    bweStr->countHighSpeedRec++;
    if (bweStr->countHighSpeedRec >= 66) {
      bweStr->highSpeedRec = 1;
    }
  } else if (!bweStr->highSpeedRec)    {
    bweStr->countHighSpeedRec = 0;
  }

  

  
  maxDelay = WebRtcIsacfix_GetDownlinkMaxDelay(bweStr);

  
  tempMax = 652800; 
  tempMin = 130560; 
  tempTermX = WEBRTC_SPL_MUL((WebRtc_Word32)bweStr->recMaxDelayAvgQ, (WebRtc_Word32)461);
  tempTermY = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)maxDelay, 18);

  tempTerm1 = tempTermX + tempMax - tempTermY;
  tempTerm2 = tempTermY - tempTermX - tempMin;

  if ( tempTerm1 > tempTerm2) {
    maxDelayBit = 0;
    tempTerm1 = tempTermX + tempMin;

    
    bweStr->recMaxDelayAvgQ = WEBRTC_SPL_RSHIFT_W32(tempTerm1, 9);
  } else {
    maxDelayBit = 12;
    tempTerm1 =  tempTermX + tempMax;

    
    bweStr->recMaxDelayAvgQ = WEBRTC_SPL_RSHIFT_W32(tempTerm1, 9);
  }

  
  return (WebRtc_UWord16)(rateInd + maxDelayBit);
}


WebRtc_UWord16 WebRtcIsacfix_GetDownlinkBandwidth(const BwEstimatorstr *bweStr)
{
  WebRtc_UWord32  recBw;
  WebRtc_Word32   jitter_sign; 
  WebRtc_Word32   bw_adjust;   
  WebRtc_Word32   rec_jitter_short_term_abs_inv; 
  WebRtc_Word32   temp;

  

  rec_jitter_short_term_abs_inv = WEBRTC_SPL_UDIV(0x80000000, bweStr->recJitterShortTermAbs);

  
  jitter_sign = WEBRTC_SPL_MUL(WEBRTC_SPL_RSHIFT_W32(bweStr->recJitterShortTerm, 4), (WebRtc_Word32)rec_jitter_short_term_abs_inv);

  if (jitter_sign < 0) {
    temp = -jitter_sign;
    temp = WEBRTC_SPL_RSHIFT_W32(temp, 19);
    jitter_sign = -temp;
  } else {
    jitter_sign = WEBRTC_SPL_RSHIFT_W32(jitter_sign, 19);
  }

  
  
  
  
  temp = 9830  + WEBRTC_SPL_RSHIFT_W32((WEBRTC_SPL_MUL(38, WEBRTC_SPL_MUL(jitter_sign, jitter_sign))), 8);

  if (jitter_sign < 0) {
    temp = WEBRTC_SPL_MUL(jitter_sign, temp);
    temp = -temp;
    temp = WEBRTC_SPL_RSHIFT_W32(temp, 8);
    bw_adjust = (WebRtc_UWord32)65536 + temp; 
  } else {
    bw_adjust = (WebRtc_UWord32)65536 - WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL(jitter_sign, temp), 8);
  }

  
  
  bw_adjust = WEBRTC_SPL_RSHIFT_W32(bw_adjust, 2);

  
  recBw = WEBRTC_SPL_UMUL(bweStr->recBw, bw_adjust);

  recBw = WEBRTC_SPL_RSHIFT_W32(recBw, 14);

  
  if (recBw < MIN_ISAC_BW) {
    recBw = MIN_ISAC_BW;
  } else if (recBw > MAX_ISAC_BW) {
    recBw = MAX_ISAC_BW;
  }

  return  (WebRtc_UWord16) recBw;
}


WebRtc_Word16 WebRtcIsacfix_GetDownlinkMaxDelay(const BwEstimatorstr *bweStr)
{
  WebRtc_Word16 recMaxDelay;

  recMaxDelay = (WebRtc_Word16)  WEBRTC_SPL_RSHIFT_W32(bweStr->recMaxDelay, 15);

  
  if (recMaxDelay < MIN_ISAC_MD) {
    recMaxDelay = MIN_ISAC_MD;
  } else if (recMaxDelay > MAX_ISAC_MD) {
    recMaxDelay = MAX_ISAC_MD;
  }

  return recMaxDelay;
}


WebRtc_Word16 WebRtcIsacfix_GetUplinkBandwidth(const BwEstimatorstr *bweStr)
{
  WebRtc_Word16 send_bw;

  send_bw = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_U32(bweStr->sendBwAvg, 7);

  
  if (send_bw < MIN_ISAC_BW) {
    send_bw = MIN_ISAC_BW;
  } else if (send_bw > MAX_ISAC_BW) {
    send_bw = MAX_ISAC_BW;
  }

  return send_bw;
}




WebRtc_Word16 WebRtcIsacfix_GetUplinkMaxDelay(const BwEstimatorstr *bweStr)
{
  WebRtc_Word16 send_max_delay;

  send_max_delay = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(bweStr->sendMaxDelayAvg, 9);

  
  if (send_max_delay < MIN_ISAC_MD) {
    send_max_delay = MIN_ISAC_MD;
  } else if (send_max_delay > MAX_ISAC_MD) {
    send_max_delay = MAX_ISAC_MD;
  }

  return send_max_delay;
}








WebRtc_UWord16 WebRtcIsacfix_GetMinBytes(RateModel *State,
                                         WebRtc_Word16 StreamSize,                    
                                         const WebRtc_Word16 FrameSamples,            
                                         const WebRtc_Word16 BottleNeck,        
                                         const WebRtc_Word16 DelayBuildUp)      
{
  WebRtc_Word32 MinRate = 0;
  WebRtc_UWord16    MinBytes;
  WebRtc_Word16 TransmissionTime;
  WebRtc_Word32 inv_Q12;
  WebRtc_Word32 den;


  
  if (State->InitCounter > 0) {
    if (State->InitCounter-- <= INIT_BURST_LEN) {
      MinRate = INIT_RATE;
    } else {
      MinRate = 0;
    }
  } else {
    
    if (State->BurstCounter) {
      if (State->StillBuffered < WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL((512 - WEBRTC_SPL_DIV(512, BURST_LEN)), DelayBuildUp), 9)) {
        
        inv_Q12 = WEBRTC_SPL_DIV(4096, WEBRTC_SPL_MUL(BURST_LEN, FrameSamples));
        MinRate = WEBRTC_SPL_MUL(512 + WEBRTC_SPL_MUL(SAMPLES_PER_MSEC, WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL(DelayBuildUp, inv_Q12), 3)), BottleNeck);
      } else {
        
        inv_Q12 = WEBRTC_SPL_DIV(4096, FrameSamples);
        if (DelayBuildUp > State->StillBuffered) {
          MinRate = WEBRTC_SPL_MUL(512 + WEBRTC_SPL_MUL(SAMPLES_PER_MSEC, WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL(DelayBuildUp - State->StillBuffered, inv_Q12), 3)), BottleNeck);
        } else if ((den = WEBRTC_SPL_MUL(SAMPLES_PER_MSEC, (State->StillBuffered - DelayBuildUp))) >= FrameSamples) {
          
          MinRate = 0;
        } else {
          MinRate = WEBRTC_SPL_MUL((512 - WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL(den, inv_Q12), 3)), BottleNeck);
        }
        
        
        
        if (MinRate < WEBRTC_SPL_MUL(532, BottleNeck)) {
          MinRate += WEBRTC_SPL_MUL(22, BottleNeck);
        }
      }

      State->BurstCounter--;
    }
  }


  
  
  MinRate += 256;
  MinRate = WEBRTC_SPL_RSHIFT_W32(MinRate, 9);
  MinBytes = (WebRtc_UWord16)WEBRTC_SPL_UDIV(WEBRTC_SPL_MUL(MinRate, FrameSamples), FS8);

  
  if (StreamSize < MinBytes) {
    StreamSize = MinBytes;
  }

  
  
  if (WEBRTC_SPL_DIV(WEBRTC_SPL_MUL(StreamSize, FS8), FrameSamples) > (WEBRTC_SPL_MUL(517, BottleNeck) >> 9)) {
    if (State->PrevExceed) {
      
      State->ExceedAgo -= WEBRTC_SPL_DIV(BURST_INTERVAL, BURST_LEN - 1);
      if (State->ExceedAgo < 0) {
        State->ExceedAgo = 0;
      }
    } else {
      State->ExceedAgo += (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W16(FrameSamples, 4);       
      State->PrevExceed = 1;
    }
  } else {
    State->PrevExceed = 0;
    State->ExceedAgo += (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W16(FrameSamples, 4);           
  }

  
  if ((State->ExceedAgo > BURST_INTERVAL) && (State->BurstCounter == 0)) {
    if (State->PrevExceed) {
      State->BurstCounter = BURST_LEN - 1;
    } else {
      State->BurstCounter = BURST_LEN;
    }
  }


  
  TransmissionTime = (WebRtc_Word16)WEBRTC_SPL_DIV(WEBRTC_SPL_MUL(StreamSize, 8000), BottleNeck);    
  State->StillBuffered += TransmissionTime;
  State->StillBuffered -= (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W16(FrameSamples, 4);  
  if (State->StillBuffered < 0) {
    State->StillBuffered = 0;
  }

  if (State->StillBuffered > 2000) {
    State->StillBuffered = 2000;
  }

  return MinBytes;
}





void WebRtcIsacfix_UpdateRateModel(RateModel *State,
                                   WebRtc_Word16 StreamSize,                    
                                   const WebRtc_Word16 FrameSamples,            
                                   const WebRtc_Word16 BottleNeck)        
{
  WebRtc_Word16 TransmissionTime;

  
  State->InitCounter = 0;

  
  TransmissionTime = (WebRtc_Word16)WEBRTC_SPL_DIV(WEBRTC_SPL_MUL(WEBRTC_SPL_MUL(StreamSize, 8), 1000), BottleNeck);    
  State->StillBuffered += TransmissionTime;
  State->StillBuffered -= (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W16(FrameSamples, 4);            
  if (State->StillBuffered < 0) {
    State->StillBuffered = 0;
  }

}


void WebRtcIsacfix_InitRateModel(RateModel *State)
{
  State->PrevExceed      = 0;                        
  State->ExceedAgo       = 0;                        
  State->BurstCounter    = 0;                        
  State->InitCounter     = INIT_BURST_LEN + 10;    
  State->StillBuffered   = 1;                    
}





WebRtc_Word16 WebRtcIsacfix_GetNewFrameLength(WebRtc_Word16 bottle_neck, WebRtc_Word16 current_framesamples)
{
  WebRtc_Word16 new_framesamples;

  new_framesamples = current_framesamples;

  
  switch(current_framesamples) {
    case 480:
      if (bottle_neck < Thld_30_60) {
        new_framesamples = 960;
      }
      break;
    case 960:
      if (bottle_neck >= Thld_60_30) {
        new_framesamples = 480;
      }
      break;
    default:
      new_framesamples = -1; 
  }

  return new_framesamples;
}

WebRtc_Word16 WebRtcIsacfix_GetSnr(WebRtc_Word16 bottle_neck, WebRtc_Word16 framesamples)
{
  WebRtc_Word16 s2nr = 0;

  
  
  switch(framesamples) {
    case 480:
      
      s2nr = -22500 + (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(500, bottle_neck, 10); 
      break;
    case 960:
      
      s2nr = -22500 + (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(500, bottle_neck, 10); 
      break;
    default:
      s2nr = -1; 
  }

  return s2nr; 

}
