









#include "webrtc_cng.h"

#include <string.h>
#include <stdlib.h>

#include "cng_helpfuns.h"
#include "signal_processing_library.h"

typedef struct WebRtcCngDecInst_t_ {
  uint32_t dec_seed;
  int32_t dec_target_energy;
  int32_t dec_used_energy;
  int16_t dec_target_reflCoefs[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int16_t dec_used_reflCoefs[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int16_t dec_filtstate[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int16_t dec_filtstateLow[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int16_t dec_Efiltstate[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int16_t dec_EfiltstateLow[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int16_t dec_order;
  int16_t dec_target_scale_factor;  
  int16_t dec_used_scale_factor;  
  int16_t target_scale_factor;  
  int16_t errorcode;
  int16_t initflag;
} WebRtcCngDecInst_t;

typedef struct WebRtcCngEncInst_t_ {
  int16_t enc_nrOfCoefs;
  uint16_t enc_sampfreq;
  int16_t enc_interval;
  int16_t enc_msSinceSID;
  int32_t enc_Energy;
  int16_t enc_reflCoefs[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int32_t enc_corrVector[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  uint32_t enc_seed;
  int16_t errorcode;
  int16_t initflag;
} WebRtcCngEncInst_t;

const int32_t WebRtcCng_kDbov[94] = {
  1081109975,  858756178,  682134279,  541838517,  430397633,  341876992,
  271562548,  215709799,  171344384,  136103682,  108110997,   85875618,
  68213428,   54183852,   43039763,   34187699,   27156255,   21570980,
  17134438,   13610368,   10811100,    8587562,    6821343,    5418385,
  4303976,    3418770,    2715625,    2157098,    1713444,    1361037,
  1081110,     858756,     682134,     541839,     430398,     341877,
  271563,     215710,     171344,     136104,     108111,      85876,
  68213,      54184,      43040,      34188,      27156,      21571,
  17134,      13610,      10811,       8588,       6821,       5418,
  4304,       3419,       2716,       2157,       1713,       1361,
  1081,        859,        682,        542,        430,        342,
  272,        216,        171,        136,        108,         86,
  68,         54,         43,         34,         27,         22,
  17,         14,         11,          9,          7,          5,
  4,          3,          3,          2,          2,           1,
  1,          1,          1,          1
};

const int16_t WebRtcCng_kCorrWindow[WEBRTC_CNG_MAX_LPC_ORDER] = {
  32702, 32636, 32570, 32505, 32439, 32374,
  32309, 32244, 32179, 32114, 32049, 31985
};












int16_t WebRtcCng_CreateEnc(CNG_enc_inst** cng_inst) {
  if (cng_inst != NULL) {
    *cng_inst = (CNG_enc_inst*) malloc(sizeof(WebRtcCngEncInst_t));
    if (*cng_inst != NULL) {
      (*(WebRtcCngEncInst_t**) cng_inst)->errorcode = 0;
      (*(WebRtcCngEncInst_t**) cng_inst)->initflag = 0;

      
      WebRtcSpl_Init();

      return 0;
    } else {
      
      return -1;
    }
  } else {
    
    return -1;
  }
}

int16_t WebRtcCng_CreateDec(CNG_dec_inst** cng_inst) {
  if (cng_inst != NULL ) {
    *cng_inst = (CNG_dec_inst*) malloc(sizeof(WebRtcCngDecInst_t));
    if (*cng_inst != NULL ) {
      (*(WebRtcCngDecInst_t**) cng_inst)->errorcode = 0;
      (*(WebRtcCngDecInst_t**) cng_inst)->initflag = 0;

      
      WebRtcSpl_Init();

      return 0;
    } else {
      
      return -1;
    }
  } else {
    
    return -1;
  }
}



















int16_t WebRtcCng_InitEnc(CNG_enc_inst* cng_inst, uint16_t fs, int16_t interval,
                          int16_t quality) {
  int i;
  WebRtcCngEncInst_t* inst = (WebRtcCngEncInst_t*) cng_inst;
  memset(inst, 0, sizeof(WebRtcCngEncInst_t));

  
  if (quality > WEBRTC_CNG_MAX_LPC_ORDER || quality <= 0) {
    inst->errorcode = CNG_DISALLOWED_LPC_ORDER;
    return -1;
  }

  inst->enc_sampfreq = fs;
  inst->enc_interval = interval;
  inst->enc_nrOfCoefs = quality;
  inst->enc_msSinceSID = 0;
  inst->enc_seed = 7777;  
  inst->enc_Energy = 0;
  for (i = 0; i < (WEBRTC_CNG_MAX_LPC_ORDER + 1); i++) {
    inst->enc_reflCoefs[i] = 0;
    inst->enc_corrVector[i] = 0;
  }
  inst->initflag = 1;

  return 0;
}

int16_t WebRtcCng_InitDec(CNG_dec_inst* cng_inst) {
  int i;

  WebRtcCngDecInst_t* inst = (WebRtcCngDecInst_t*) cng_inst;

  memset(inst, 0, sizeof(WebRtcCngDecInst_t));
  inst->dec_seed = 7777;  
  inst->dec_order = 5;
  inst->dec_target_scale_factor = 0;
  inst->dec_used_scale_factor = 0;
  for (i = 0; i < (WEBRTC_CNG_MAX_LPC_ORDER + 1); i++) {
    inst->dec_filtstate[i] = 0;
    inst->dec_target_reflCoefs[i] = 0;
    inst->dec_used_reflCoefs[i] = 0;
  }
  inst->dec_target_reflCoefs[0] = 0;
  inst->dec_used_reflCoefs[0] = 0;
  inst->dec_used_energy = 0;
  inst->initflag = 1;

  return 0;
}












int16_t WebRtcCng_FreeEnc(CNG_enc_inst* cng_inst) {
  free(cng_inst);
  return 0;
}

int16_t WebRtcCng_FreeDec(CNG_dec_inst* cng_inst) {
  free(cng_inst);
  return 0;
}















int16_t WebRtcCng_Encode(CNG_enc_inst* cng_inst, int16_t* speech,
                         int16_t nrOfSamples, uint8_t* SIDdata,
                         int16_t* bytesOut, int16_t forceSID) {
  WebRtcCngEncInst_t* inst = (WebRtcCngEncInst_t*) cng_inst;

  int16_t arCoefs[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int32_t corrVector[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int16_t refCs[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int16_t hanningW[WEBRTC_CNG_MAX_OUTSIZE_ORDER];
  int16_t ReflBeta = 19661;  
  int16_t ReflBetaComp = 13107;  
  int32_t outEnergy;
  int outShifts;
  int i, stab;
  int acorrScale;
  int index;
  int16_t ind, factor;
  int32_t* bptr;
  int32_t blo, bhi;
  int16_t negate;
  const int16_t* aptr;
  int16_t speechBuf[WEBRTC_CNG_MAX_OUTSIZE_ORDER];

  
  if (inst->initflag != 1) {
    inst->errorcode = CNG_ENCODER_NOT_INITIATED;
    return -1;
  }

  
  if (nrOfSamples > WEBRTC_CNG_MAX_OUTSIZE_ORDER) {
    inst->errorcode = CNG_DISALLOWED_FRAME_SIZE;
    return -1;
  }

  for (i = 0; i < nrOfSamples; i++) {
    speechBuf[i] = speech[i];
  }

  factor = nrOfSamples;

  
  outEnergy = WebRtcSpl_Energy(speechBuf, nrOfSamples, &outShifts);
  while (outShifts > 0) {
    

    if (outShifts > 5) {
      outEnergy <<= (outShifts - 5);
      outShifts = 5;
    } else {
      factor /= 2;
      outShifts--;
    }
  }
  outEnergy = WebRtcSpl_DivW32W16(outEnergy, factor);

  if (outEnergy > 1) {
    
    WebRtcSpl_GetHanningWindow(hanningW, nrOfSamples / 2);
    for (i = 0; i < (nrOfSamples / 2); i++)
      hanningW[nrOfSamples - i - 1] = hanningW[i];

    WebRtcSpl_ElementwiseVectorMult(speechBuf, hanningW, speechBuf, nrOfSamples,
                                    14);

    WebRtcSpl_AutoCorrelation(speechBuf, nrOfSamples, inst->enc_nrOfCoefs,
                              corrVector, &acorrScale);

    if (*corrVector == 0)
      *corrVector = WEBRTC_SPL_WORD16_MAX;

    
    aptr = WebRtcCng_kCorrWindow;
    bptr = corrVector;

    
    for (ind = 0; ind < inst->enc_nrOfCoefs; ind++) {
      

      negate = *bptr < 0;
      if (negate)
        *bptr = -*bptr;

      blo = (int32_t) * aptr * (*bptr & 0xffff);
      bhi = ((blo >> 16) & 0xffff)
          + ((int32_t)(*aptr++) * ((*bptr >> 16) & 0xffff));
      blo = (blo & 0xffff) | ((bhi & 0xffff) << 16);

      *bptr = (((bhi >> 16) & 0x7fff) << 17) | ((uint32_t) blo >> 15);
      if (negate)
        *bptr = -*bptr;
      bptr++;
    }
    

    stab = WebRtcSpl_LevinsonDurbin(corrVector, arCoefs, refCs,
                                    inst->enc_nrOfCoefs);

    if (!stab) {
      
      *bytesOut = 0;
      return 0;
    }

  } else {
    for (i = 0; i < inst->enc_nrOfCoefs; i++)
      refCs[i] = 0;
  }

  if (forceSID) {
    
    for (i = 0; i < inst->enc_nrOfCoefs; i++)
      inst->enc_reflCoefs[i] = refCs[i];
    inst->enc_Energy = outEnergy;
  } else {
    
    for (i = 0; i < (inst->enc_nrOfCoefs); i++) {
      inst->enc_reflCoefs[i] = (int16_t) WEBRTC_SPL_MUL_16_16_RSFT(
          inst->enc_reflCoefs[i], ReflBeta, 15);
      inst->enc_reflCoefs[i] += (int16_t) WEBRTC_SPL_MUL_16_16_RSFT(
          refCs[i], ReflBetaComp, 15);
    }
    inst->enc_Energy = (outEnergy >> 2) + (inst->enc_Energy >> 1)
        + (inst->enc_Energy >> 2);
  }

  if (inst->enc_Energy < 1) {
    inst->enc_Energy = 1;
  }

  if ((inst->enc_msSinceSID > (inst->enc_interval - 1)) || forceSID) {

    
    index = 0;
    for (i = 1; i < 93; i++) {
      
      if ((inst->enc_Energy - WebRtcCng_kDbov[i]) > 0) {
        index = i;
        break;
      }
    }
    if ((i == 93) && (index == 0))
      index = 94;
    SIDdata[0] = index;

    
    if (inst->enc_nrOfCoefs == WEBRTC_CNG_MAX_LPC_ORDER) {
      for (i = 0; i < inst->enc_nrOfCoefs; i++) {
        
        SIDdata[i + 1] = ((inst->enc_reflCoefs[i] + 128) >> 8);
      }
    } else {
      for (i = 0; i < inst->enc_nrOfCoefs; i++) {
        
        SIDdata[i + 1] = (127 + ((inst->enc_reflCoefs[i] + 128) >> 8));
      }
    }

    inst->enc_msSinceSID = 0;
    *bytesOut = inst->enc_nrOfCoefs + 1;

    inst->enc_msSinceSID += (1000 * nrOfSamples) / inst->enc_sampfreq;
    return inst->enc_nrOfCoefs + 1;
  } else {
    inst->enc_msSinceSID += (1000 * nrOfSamples) / inst->enc_sampfreq;
    *bytesOut = 0;
    return 0;
  }
}














int16_t WebRtcCng_UpdateSid(CNG_dec_inst* cng_inst, uint8_t* SID,
                            int16_t length) {

  WebRtcCngDecInst_t* inst = (WebRtcCngDecInst_t*) cng_inst;
  int16_t refCs[WEBRTC_CNG_MAX_LPC_ORDER];
  int32_t targetEnergy;
  int i;

  if (inst->initflag != 1) {
    inst->errorcode = CNG_DECODER_NOT_INITIATED;
    return -1;
  }

  
  if (length > (WEBRTC_CNG_MAX_LPC_ORDER + 1))
    length = WEBRTC_CNG_MAX_LPC_ORDER + 1;

  inst->dec_order = length - 1;

  if (SID[0] > 93)
    SID[0] = 93;
  targetEnergy = WebRtcCng_kDbov[SID[0]];
  
  targetEnergy = targetEnergy >> 1;
  targetEnergy += targetEnergy >> 2;

  inst->dec_target_energy = targetEnergy;

  
  if (inst->dec_order == WEBRTC_CNG_MAX_LPC_ORDER) {
    for (i = 0; i < (inst->dec_order); i++) {
      refCs[i] = SID[i + 1] << 8; 
      inst->dec_target_reflCoefs[i] = refCs[i];
    }
  } else {
    for (i = 0; i < (inst->dec_order); i++) {
      refCs[i] = (SID[i + 1] - 127) << 8; 
      inst->dec_target_reflCoefs[i] = refCs[i];
    }
  }

  for (i = (inst->dec_order); i < WEBRTC_CNG_MAX_LPC_ORDER; i++) {
    refCs[i] = 0;
    inst->dec_target_reflCoefs[i] = refCs[i];
  }

  return 0;
}














int16_t WebRtcCng_Generate(CNG_dec_inst* cng_inst, int16_t* outData,
                           int16_t nrOfSamples, int16_t new_period) {
  WebRtcCngDecInst_t* inst = (WebRtcCngDecInst_t*) cng_inst;

  int i;
  int16_t excitation[WEBRTC_CNG_MAX_OUTSIZE_ORDER];
  int16_t low[WEBRTC_CNG_MAX_OUTSIZE_ORDER];
  int16_t lpPoly[WEBRTC_CNG_MAX_LPC_ORDER + 1];
  int16_t ReflBetaStd = 26214;  
  int16_t ReflBetaCompStd = 6553;  
  int16_t ReflBetaNewP = 19661;  
  int16_t ReflBetaCompNewP = 13107;  
  int16_t Beta, BetaC, tmp1, tmp2, tmp3;
  int32_t targetEnergy;
  int16_t En;
  int16_t temp16;

  if (nrOfSamples > WEBRTC_CNG_MAX_OUTSIZE_ORDER) {
    inst->errorcode = CNG_DISALLOWED_FRAME_SIZE;
    return -1;
  }

  if (new_period) {
    inst->dec_used_scale_factor = inst->dec_target_scale_factor;
    Beta = ReflBetaNewP;
    BetaC = ReflBetaCompNewP;
  } else {
    Beta = ReflBetaStd;
    BetaC = ReflBetaCompStd;
  }

  
  tmp1 = inst->dec_used_scale_factor << 2; 
  tmp2 = inst->dec_target_scale_factor << 2; 
  tmp3 = (int16_t) WEBRTC_SPL_MUL_16_16_RSFT(tmp1, Beta, 15);
  tmp3 += (int16_t) WEBRTC_SPL_MUL_16_16_RSFT(tmp2, BetaC, 15);
  inst->dec_used_scale_factor = tmp3 >> 2; 

  inst->dec_used_energy = inst->dec_used_energy >> 1;
  inst->dec_used_energy += inst->dec_target_energy >> 1;

  
  for (i = 0; i < WEBRTC_CNG_MAX_LPC_ORDER; i++) {
    inst->dec_used_reflCoefs[i] = (int16_t) WEBRTC_SPL_MUL_16_16_RSFT(
        inst->dec_used_reflCoefs[i], Beta, 15);
    inst->dec_used_reflCoefs[i] += (int16_t) WEBRTC_SPL_MUL_16_16_RSFT(
        inst->dec_target_reflCoefs[i], BetaC, 15);
  }

  
  WebRtcCng_K2a16(inst->dec_used_reflCoefs, WEBRTC_CNG_MAX_LPC_ORDER, lpPoly);


  targetEnergy = inst->dec_used_energy;

  
  En = 8192;  
  for (i = 0; i < (WEBRTC_CNG_MAX_LPC_ORDER); i++) {

    




    
    
    temp16 = (int16_t) WEBRTC_SPL_MUL_16_16_RSFT(
        inst->dec_used_reflCoefs[i], inst->dec_used_reflCoefs[i], 15);
    
    temp16 = 0x7fff - temp16;
    En = (int16_t) WEBRTC_SPL_MUL_16_16_RSFT(En, temp16, 15);
  }

  

  
  targetEnergy = WebRtcSpl_Sqrt(inst->dec_used_energy);

  En = (int16_t) WebRtcSpl_Sqrt(En) << 6;
  En = (En * 3) >> 1;  
  inst->dec_used_scale_factor = (int16_t)((En * targetEnergy) >> 12);

  
  
  for (i = 0; i < nrOfSamples; i++) {
    excitation[i] = WebRtcSpl_RandN(&inst->dec_seed) >> 1;
  }

  
  WebRtcSpl_ScaleVector(excitation, excitation, inst->dec_used_scale_factor,
                        nrOfSamples, 13);

  



  WebRtcSpl_FilterAR(lpPoly, WEBRTC_CNG_MAX_LPC_ORDER + 1, excitation,
                     nrOfSamples, inst->dec_filtstate, WEBRTC_CNG_MAX_LPC_ORDER,
                     inst->dec_filtstateLow, WEBRTC_CNG_MAX_LPC_ORDER, outData,
                     low, nrOfSamples);

  return 0;
}














int16_t WebRtcCng_GetErrorCodeEnc(CNG_enc_inst* cng_inst) {
  
  WebRtcCngEncInst_t* inst = (WebRtcCngEncInst_t*) cng_inst;
  return inst->errorcode;
}

int16_t WebRtcCng_GetErrorCodeDec(CNG_dec_inst* cng_inst) {
  
  WebRtcCngDecInst_t* inst = (WebRtcCngDecInst_t*) cng_inst;
  return inst->errorcode;
}
