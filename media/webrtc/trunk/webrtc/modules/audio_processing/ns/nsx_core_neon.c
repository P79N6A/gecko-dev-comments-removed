









#include "webrtc/modules/audio_processing/ns/nsx_core.h"

#include <arm_neon.h>
#include <assert.h>


const int16_t WebRtcNsx_kLogTable[9] = {
  0, 177, 355, 532, 710, 887, 1065, 1242, 1420
};

const int16_t WebRtcNsx_kCounterDiv[201] = {
  32767, 16384, 10923, 8192, 6554, 5461, 4681, 4096, 3641, 3277, 2979, 2731,
  2521, 2341, 2185, 2048, 1928, 1820, 1725, 1638, 1560, 1489, 1425, 1365, 1311,
  1260, 1214, 1170, 1130, 1092, 1057, 1024, 993, 964, 936, 910, 886, 862, 840,
  819, 799, 780, 762, 745, 728, 712, 697, 683, 669, 655, 643, 630, 618, 607,
  596, 585, 575, 565, 555, 546, 537, 529, 520, 512, 504, 496, 489, 482, 475,
  468, 462, 455, 449, 443, 437, 431, 426, 420, 415, 410, 405, 400, 395, 390,
  386, 381, 377, 372, 368, 364, 360, 356, 352, 349, 345, 341, 338, 334, 331,
  328, 324, 321, 318, 315, 312, 309, 306, 303, 301, 298, 295, 293, 290, 287,
  285, 282, 280, 278, 275, 273, 271, 269, 266, 264, 262, 260, 258, 256, 254,
  252, 250, 248, 246, 245, 243, 241, 239, 237, 236, 234, 232, 231, 229, 228,
  226, 224, 223, 221, 220, 218, 217, 216, 214, 213, 211, 210, 209, 207, 206,
  205, 204, 202, 201, 200, 199, 197, 196, 195, 194, 193, 192, 191, 189, 188,
  187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176, 175, 174, 173,
  172, 172, 171, 170, 169, 168, 167, 166, 165, 165, 164, 163
};

const int16_t WebRtcNsx_kLogTableFrac[256] = {
  0, 1, 3, 4, 6, 7, 9, 10, 11, 13, 14, 16, 17, 18, 20, 21,
  22, 24, 25, 26, 28, 29, 30, 32, 33, 34, 36, 37, 38, 40, 41, 42,
  44, 45, 46, 47, 49, 50, 51, 52, 54, 55, 56, 57, 59, 60, 61, 62,
  63, 65, 66, 67, 68, 69, 71, 72, 73, 74, 75, 77, 78, 79, 80, 81,
  82, 84, 85, 86, 87, 88, 89, 90, 92, 93, 94, 95, 96, 97, 98, 99,
  100, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 116,
  117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146,
  147, 148, 149, 150, 151, 152, 153, 154, 155, 155, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 169, 170, 171, 172, 173, 174,
  175, 176, 177, 178, 178, 179, 180, 181, 182, 183, 184, 185, 185, 186, 187,
  188, 189, 190, 191, 192, 192, 193, 194, 195, 196, 197, 198, 198, 199, 200,
  201, 202, 203, 203, 204, 205, 206, 207, 208, 208, 209, 210, 211, 212, 212,
  213, 214, 215, 216, 216, 217, 218, 219, 220, 220, 221, 222, 223, 224, 224,
  225, 226, 227, 228, 228, 229, 230, 231, 231, 232, 233, 234, 234, 235, 236,
  237, 238, 238, 239, 240, 241, 241, 242, 243, 244, 244, 245, 246, 247, 247,
  248, 249, 249, 250, 251, 252, 252, 253, 254, 255, 255
};


static void UpdateNoiseEstimateNeon(NsxInst_t* inst, int offset) {
  const int16_t kExp2Const = 11819; 
  int16_t* ptr_noiseEstLogQuantile = NULL;
  int16_t* ptr_noiseEstQuantile = NULL;
  int16x4_t kExp2Const16x4 = vdup_n_s16(kExp2Const);
  int32x4_t twentyOne32x4 = vdupq_n_s32(21);
  int32x4_t constA32x4 = vdupq_n_s32(0x1fffff);
  int32x4_t constB32x4 = vdupq_n_s32(0x200000);

  int16_t tmp16 = WebRtcSpl_MaxValueW16(inst->noiseEstLogQuantile + offset,
                                        inst->magnLen);

  
  inst->qNoise = 14 - (int) WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(kExp2Const,
                                                                 tmp16,
                                                                 21);

  int32x4_t qNoise32x4 = vdupq_n_s32(inst->qNoise);

  for (ptr_noiseEstLogQuantile = &inst->noiseEstLogQuantile[offset],
       ptr_noiseEstQuantile = &inst->noiseEstQuantile[0];
       ptr_noiseEstQuantile < &inst->noiseEstQuantile[inst->magnLen - 3];
       ptr_noiseEstQuantile += 4, ptr_noiseEstLogQuantile += 4) {

    
    
    int16x4_t v16x4 = vld1_s16(ptr_noiseEstLogQuantile);
    int32x4_t v32x4B = vmull_s16(v16x4, kExp2Const16x4);

    
    int32x4_t v32x4A = vandq_s32(v32x4B, constA32x4);
    v32x4A = vorrq_s32(v32x4A, constB32x4);

    
    v32x4B = vshrq_n_s32(v32x4B, 21);

    
    v32x4B = vsubq_s32(v32x4B, twentyOne32x4);

    
    
    v32x4B = vaddq_s32(v32x4B, qNoise32x4);

    
    
    
    
    
    v32x4B = vshlq_s32(v32x4A, v32x4B);

    
    v16x4 = vqmovn_s32(v32x4B);

    
    vst1_s16(ptr_noiseEstQuantile, v16x4);
  }

  

  
  
  int32_t tmp32no2 = WEBRTC_SPL_MUL_16_16(kExp2Const,
                                          *ptr_noiseEstLogQuantile);
  int32_t tmp32no1 = (0x00200000 | (tmp32no2 & 0x001FFFFF)); 

  tmp16 = (int16_t)(tmp32no2 >> 21);
  tmp16 -= 21;
  tmp16 += (int16_t) inst->qNoise; 
  if (tmp16 < 0) {
    tmp32no1 >>= -tmp16;
  } else {
    tmp32no1 <<= tmp16;
  }
  *ptr_noiseEstQuantile = WebRtcSpl_SatW32ToW16(tmp32no1);
}


void WebRtcNsx_NoiseEstimationNeon(NsxInst_t* inst,
                                   uint16_t* magn,
                                   uint32_t* noise,
                                   int16_t* q_noise) {
  int16_t lmagn[HALF_ANAL_BLOCKL], counter, countDiv;
  int16_t countProd, delta, zeros, frac;
  int16_t log2, tabind, logval, tmp16, tmp16no1, tmp16no2;
  const int16_t log2_const = 22713;
  const int16_t width_factor = 21845;

  int i, s, offset;

  tabind = inst->stages - inst->normData;
  assert(tabind < 9);
  assert(tabind > -9);
  if (tabind < 0) {
    logval = -WebRtcNsx_kLogTable[-tabind];
  } else {
    logval = WebRtcNsx_kLogTable[tabind];
  }

  int16x8_t logval_16x8 = vdupq_n_s16(logval);

  
  
  
  
  for (i = 0; i < inst->magnLen; i++) {
    if (magn[i]) {
      zeros = WebRtcSpl_NormU32((uint32_t)magn[i]);
      frac = (int16_t)((((uint32_t)magn[i] << zeros)
                        & 0x7FFFFFFF) >> 23);
      assert(frac < 256);
      
      log2 = (int16_t)(((31 - zeros) << 8)
                       + WebRtcNsx_kLogTableFrac[frac]);
      
      lmagn[i] = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(log2, log2_const, 15);
      
      lmagn[i] += logval;
    } else {
      lmagn[i] = logval;
    }
  }

  int16x4_t Q3_16x4  = vdup_n_s16(3);
  int16x8_t WIDTHQ8_16x8 = vdupq_n_s16(WIDTH_Q8);
  int16x8_t WIDTHFACTOR_16x8 = vdupq_n_s16(width_factor);

  int16_t factor = FACTOR_Q7;
  if (inst->blockIndex < END_STARTUP_LONG)
    factor = FACTOR_Q7_STARTUP;

  
  for (s = 0; s < SIMULT; s++) {
    offset = s * inst->magnLen;

    
    counter = inst->noiseEstCounter[s];
    assert(counter < 201);
    countDiv = WebRtcNsx_kCounterDiv[counter];
    countProd = (int16_t)WEBRTC_SPL_MUL_16_16(counter, countDiv);

    
    int16_t deltaBuff[8];
    int16x4_t tmp16x4_0;
    int16x4_t tmp16x4_1;
    int16x4_t countDiv_16x4 = vdup_n_s16(countDiv);
    int16x8_t countProd_16x8 = vdupq_n_s16(countProd);
    int16x8_t tmp16x8_0 = vdupq_n_s16(countDiv);
    int16x8_t prod16x8 = vqrdmulhq_s16(WIDTHFACTOR_16x8, tmp16x8_0);
    int16x8_t tmp16x8_1;
    int16x8_t tmp16x8_2;
    int16x8_t tmp16x8_3;
    
    int16x8_t tmp16x8_4 = vdupq_n_s16(0);
    int16x8_t tmp16x8_5;
    int32x4_t tmp32x4;

    for (i = 0; i < inst->magnLen - 7; i += 8) {
      
      
      
      tmp16x8_0 = vdupq_n_s16(factor);
      vst1q_s16(deltaBuff, tmp16x8_0);

      int j;
      for (j = 0; j < 8; j++) {
        if (inst->noiseEstDensity[offset + i + j] > 512) {
          
          int factor = WebRtcSpl_NormW16(inst->noiseEstDensity[offset + i + j]);
          deltaBuff[j] = (int16_t)(FACTOR_Q16 >> (14 - factor));
        }
      }

      

      
      tmp32x4 = vmull_s16(vld1_s16(&deltaBuff[0]), countDiv_16x4);
      tmp16x4_1 = vshrn_n_s32(tmp32x4, 14);
      tmp32x4 = vmull_s16(vld1_s16(&deltaBuff[4]), countDiv_16x4);
      tmp16x4_0 = vshrn_n_s32(tmp32x4, 14);
      tmp16x8_0 = vcombine_s16(tmp16x4_1, tmp16x4_0); 

      
      
      
      tmp16x8_1 = vrshrq_n_s16(tmp16x8_0, 2);

      
      tmp16x8_2 = vld1q_s16(&inst->noiseEstLogQuantile[offset + i]); 
      tmp16x8_1 = vaddq_s16(tmp16x8_2, tmp16x8_1); 

      
      
      
      tmp16x8_0 = vrshrq_n_s16(tmp16x8_0, 1);

      
      tmp32x4 = vmull_s16(vget_low_s16(tmp16x8_0), Q3_16x4);
      tmp16x4_1 = vshrn_n_s32(tmp32x4, 1);

      
      tmp32x4 = vmull_s16(vget_high_s16(tmp16x8_0), Q3_16x4);
      tmp16x4_0 = vshrn_n_s32(tmp32x4, 1);

      
      tmp16x8_0 = vcombine_s16(tmp16x4_1, tmp16x4_0); 
      tmp16x8_0 = vsubq_s16(tmp16x8_2, tmp16x8_0);

      
      
      
      tmp16x8_0 = vmaxq_s16(tmp16x8_0, logval_16x8);

      
      tmp16x8_3 = vld1q_s16(&lmagn[i]); 
      tmp16x8_5 = vsubq_s16(tmp16x8_3, tmp16x8_2);
      __asm__("vcgt.s16 %q0, %q1, #0"::"w"(tmp16x8_4), "w"(tmp16x8_5));
      __asm__("vbit %q0, %q1, %q2"::
              "w"(tmp16x8_2), "w"(tmp16x8_1), "w"(tmp16x8_4));
      __asm__("vbif %q0, %q1, %q2"::
              "w"(tmp16x8_2), "w"(tmp16x8_0), "w"(tmp16x8_4));
      vst1q_s16(&inst->noiseEstLogQuantile[offset + i], tmp16x8_2);

      
      
      tmp16x8_1 = vld1q_s16(&inst->noiseEstDensity[offset + i]);
      tmp16x8_0 = vqrdmulhq_s16(tmp16x8_1, countProd_16x8);
      tmp16x8_0 = vaddq_s16(tmp16x8_0, prod16x8);

      
      tmp16x8_3 = vsubq_s16(tmp16x8_3, tmp16x8_2);
      tmp16x8_3 = vabsq_s16(tmp16x8_3);
      tmp16x8_4 = vcgtq_s16(WIDTHQ8_16x8, tmp16x8_3);
      __asm__("vbit %q0, %q1, %q2"::
              "w"(tmp16x8_1), "w"(tmp16x8_0), "w"(tmp16x8_4));
      vst1q_s16(&inst->noiseEstDensity[offset + i], tmp16x8_1);
    }  

    
    
    if (inst->noiseEstDensity[offset + i] > 512) {
      
      int factor = WebRtcSpl_NormW16(inst->noiseEstDensity[offset + i]);
      delta = (int16_t)(FACTOR_Q16 >> (14 - factor));
    } else {
      delta = FACTOR_Q7;
      if (inst->blockIndex < END_STARTUP_LONG) {
        
        
        delta = FACTOR_Q7_STARTUP;
      }
    }
    
    tmp16 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(delta, countDiv, 14);
    if (lmagn[i] > inst->noiseEstLogQuantile[offset + i]) {
      
      
      tmp16 += 2;
      inst->noiseEstLogQuantile[offset + i] += tmp16 / 4;
    } else {
      tmp16 += 1;
      
      tmp16no2 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(tmp16 / 2, 3, 1);
      inst->noiseEstLogQuantile[offset + i] -= tmp16no2;
      if (inst->noiseEstLogQuantile[offset + i] < logval) {
        
        
        
        inst->noiseEstLogQuantile[offset + i] = logval;
      }
    }

    
    if (WEBRTC_SPL_ABS_W16(lmagn[i] - inst->noiseEstLogQuantile[offset + i])
        < WIDTH_Q8) {
      tmp16no1 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                   inst->noiseEstDensity[offset + i], countProd, 15);
      tmp16no2 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                   width_factor, countDiv, 15);
      inst->noiseEstDensity[offset + i] = tmp16no1 + tmp16no2;
    }


    if (counter >= END_STARTUP_LONG) {
      inst->noiseEstCounter[s] = 0;
      if (inst->blockIndex >= END_STARTUP_LONG) {
        UpdateNoiseEstimateNeon(inst, offset);
      }
    }
    inst->noiseEstCounter[s]++;

  }  

  
  if (inst->blockIndex < END_STARTUP_LONG) {
    UpdateNoiseEstimateNeon(inst, offset);
  }

  for (i = 0; i < inst->magnLen; i++) {
    noise[i] = (uint32_t)(inst->noiseEstQuantile[i]); 
  }
  (*q_noise) = (int16_t)inst->qNoise;
}


void WebRtcNsx_PrepareSpectrumNeon(NsxInst_t* inst, int16_t* freq_buf) {
  assert(inst->magnLen % 8 == 1);
  assert(inst->anaLen2 % 16 == 0);

  

  
  
  
  
  
  
  

  int16_t* preal = &inst->real[0];
  int16_t* pimag = &inst->imag[0];
  int16_t* pns_filter = (int16_t*)&inst->noiseSupFilter[0];
  int16_t* pimag_end = pimag + inst->magnLen - 4;

  while (pimag < pimag_end) {
    int16x8_t real = vld1q_s16(preal);
    int16x8_t imag = vld1q_s16(pimag);
    int16x8_t ns_filter = vld1q_s16(pns_filter);

    int32x4_t tmp_r_0 = vmull_s16(vget_low_s16(real), vget_low_s16(ns_filter));
    int32x4_t tmp_i_0 = vmull_s16(vget_low_s16(imag), vget_low_s16(ns_filter));
    int32x4_t tmp_r_1 = vmull_s16(vget_high_s16(real),
                                  vget_high_s16(ns_filter));
    int32x4_t tmp_i_1 = vmull_s16(vget_high_s16(imag),
                                  vget_high_s16(ns_filter));

    int16x4_t result_r_0 = vshrn_n_s32(tmp_r_0, 14);
    int16x4_t result_i_0 = vshrn_n_s32(tmp_i_0, 14);
    int16x4_t result_r_1 = vshrn_n_s32(tmp_r_1, 14);
    int16x4_t result_i_1 = vshrn_n_s32(tmp_i_1, 14);

    vst1q_s16(preal, vcombine_s16(result_r_0, result_r_1));
    vst1q_s16(pimag, vcombine_s16(result_i_0, result_i_1));
    preal += 8;
    pimag += 8;
    pns_filter += 8;
  }

  
  *preal = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(*preal, *pns_filter, 14);
  *pimag = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(*pimag, *pns_filter, 14);

  

  
  
  
  
  
  
  
  
  

  preal = &inst->real[0];
  pimag = &inst->imag[0];
  pimag_end = pimag + inst->anaLen2;
  int16_t * freq_buf_start = freq_buf;
  while (pimag < pimag_end) {
    
    int16x8x2_t real_imag_0;
    int16x8x2_t real_imag_1;
    real_imag_0.val[1] = vld1q_s16(pimag);
    real_imag_0.val[0] = vld1q_s16(preal);
    preal += 8;
    pimag += 8;
    real_imag_1.val[1] = vld1q_s16(pimag);
    real_imag_1.val[0] = vld1q_s16(preal);
    preal += 8;
    pimag += 8;

    real_imag_0.val[1] = vnegq_s16(real_imag_0.val[1]);
    real_imag_1.val[1] = vnegq_s16(real_imag_1.val[1]);
    vst2q_s16(freq_buf_start, real_imag_0);
    freq_buf_start += 16;
    vst2q_s16(freq_buf_start, real_imag_1);
    freq_buf_start += 16;
  }
  freq_buf[inst->anaLen] = inst->real[inst->anaLen2];
  freq_buf[inst->anaLen + 1] = -inst->imag[inst->anaLen2];
}


void WebRtcNsx_DenormalizeNeon(NsxInst_t* inst, int16_t* in, int factor) {
  int16_t* ptr_real = &inst->real[0];
  int16_t* ptr_in = &in[0];

  __asm__ __volatile__("vdup.32 q10, %0" ::
                       "r"((int32_t)(factor - inst->normData)) : "q10");
  for (; ptr_real < &inst->real[inst->anaLen];) {

    
    __asm__ __volatile__(
      
      
      "vld2.16 {d24, d25}, [%[ptr_in]]!\n\t"
      "vmovl.s16 q12, d24\n\t"
      "vshl.s32 q12, q10\n\t"
      
      "vqmovn.s32 d24, q12\n\t"
      "vst1.16 d24, [%[ptr_real]]!\n\t"

      
      
      "vld2.16 {d22, d23}, [%[ptr_in]]!\n\t"
      "vmovl.s16 q11, d22\n\t"
      "vshl.s32 q11, q10\n\t"
      
      "vqmovn.s32 d22, q11\n\t"
      "vst1.16 d22, [%[ptr_real]]!\n\t"

      
      :[ptr_in]"+r"(ptr_in),
       [ptr_real]"+r"(ptr_real)
      :
      :"d22", "d23", "d24", "d25"
    );
  }
}



void WebRtcNsx_SynthesisUpdateNeon(NsxInst_t* inst,
                                   int16_t* out_frame,
                                   int16_t gain_factor) {
  assert(inst->anaLen % 16 == 0);
  assert(inst->blockLen10ms % 16 == 0);

  int16_t* preal_start = inst->real;
  const int16_t* pwindow = inst->window;
  int16_t* preal_end = preal_start + inst->anaLen;
  int16_t* psynthesis_buffer = inst->synthesisBuffer;

  while (preal_start < preal_end) {
    
    int16x8_t window_0 = vld1q_s16(pwindow);
    int16x8_t real_0 = vld1q_s16(preal_start);
    int16x8_t synthesis_buffer_0 = vld1q_s16(psynthesis_buffer);

    int16x8_t window_1 = vld1q_s16(pwindow + 8);
    int16x8_t real_1 = vld1q_s16(preal_start + 8);
    int16x8_t synthesis_buffer_1 = vld1q_s16(psynthesis_buffer + 8);

    int32x4_t tmp32a_0_low = vmull_s16(vget_low_s16(real_0),
                                       vget_low_s16(window_0));
    int32x4_t tmp32a_0_high = vmull_s16(vget_high_s16(real_0),
                                        vget_high_s16(window_0));

    int32x4_t tmp32a_1_low = vmull_s16(vget_low_s16(real_1),
                                       vget_low_s16(window_1));
    int32x4_t tmp32a_1_high = vmull_s16(vget_high_s16(real_1),
                                        vget_high_s16(window_1));

    int16x4_t tmp16a_0_low = vqrshrn_n_s32(tmp32a_0_low, 14);
    int16x4_t tmp16a_0_high = vqrshrn_n_s32(tmp32a_0_high, 14);

    int16x4_t tmp16a_1_low = vqrshrn_n_s32(tmp32a_1_low, 14);
    int16x4_t tmp16a_1_high = vqrshrn_n_s32(tmp32a_1_high, 14);

    int32x4_t tmp32b_0_low = vmull_n_s16(tmp16a_0_low, gain_factor);
    int32x4_t tmp32b_0_high = vmull_n_s16(tmp16a_0_high, gain_factor);

    int32x4_t tmp32b_1_low = vmull_n_s16(tmp16a_1_low, gain_factor);
    int32x4_t tmp32b_1_high = vmull_n_s16(tmp16a_1_high, gain_factor);

    int16x4_t tmp16b_0_low = vqrshrn_n_s32(tmp32b_0_low, 13);
    int16x4_t tmp16b_0_high = vqrshrn_n_s32(tmp32b_0_high, 13);

    int16x4_t tmp16b_1_low = vqrshrn_n_s32(tmp32b_1_low, 13);
    int16x4_t tmp16b_1_high = vqrshrn_n_s32(tmp32b_1_high, 13);

    synthesis_buffer_0 = vqaddq_s16(vcombine_s16(tmp16b_0_low, tmp16b_0_high),
                                    synthesis_buffer_0);
    synthesis_buffer_1 = vqaddq_s16(vcombine_s16(tmp16b_1_low, tmp16b_1_high),
                                    synthesis_buffer_1);
    vst1q_s16(psynthesis_buffer, synthesis_buffer_0);
    vst1q_s16(psynthesis_buffer + 8, synthesis_buffer_1);

    pwindow += 16;
    preal_start += 16;
    psynthesis_buffer += 16;
  }

  
  int16_t * p_start = inst->synthesisBuffer;
  int16_t * p_end = inst->synthesisBuffer + inst->blockLen10ms;
  int16_t * p_frame = out_frame;
  while (p_start < p_end) {
    int16x8_t frame_0 = vld1q_s16(p_start);
    vst1q_s16(p_frame, frame_0);
    p_start += 8;
    p_frame += 8;
  }

  
  int16_t* p_start_src = inst->synthesisBuffer + inst->blockLen10ms;
  int16_t* p_end_src = inst->synthesisBuffer + inst->anaLen;
  int16_t* p_start_dst = inst->synthesisBuffer;
  while (p_start_src < p_end_src) {
    int16x8_t frame = vld1q_s16(p_start_src);
    vst1q_s16(p_start_dst, frame);
    p_start_src += 8;
    p_start_dst += 8;
  }

  p_start = inst->synthesisBuffer + inst->anaLen - inst->blockLen10ms;
  p_end = p_start + inst->blockLen10ms;
  int16x8_t zero = vdupq_n_s16(0);
  for (;p_start < p_end; p_start += 8) {
    vst1q_s16(p_start, zero);
  }
}


void WebRtcNsx_AnalysisUpdateNeon(NsxInst_t* inst,
                                  int16_t* out,
                                  int16_t* new_speech) {
  assert(inst->blockLen10ms % 16 == 0);
  assert(inst->anaLen % 16 == 0);

  
  
  
  
  int16_t* p_start_src = inst->analysisBuffer + inst->blockLen10ms;
  int16_t* p_end_src = inst->analysisBuffer + inst->anaLen;
  int16_t* p_start_dst = inst->analysisBuffer;
  while (p_start_src < p_end_src) {
    int16x8_t frame = vld1q_s16(p_start_src);
    vst1q_s16(p_start_dst, frame);

    p_start_src += 8;
    p_start_dst += 8;
  }

  
  
  p_start_src = new_speech;
  p_end_src = new_speech + inst->blockLen10ms;
  p_start_dst = inst->analysisBuffer + inst->anaLen - inst->blockLen10ms;
  while (p_start_src < p_end_src) {
    int16x8_t frame = vld1q_s16(p_start_src);
    vst1q_s16(p_start_dst, frame);

    p_start_src += 8;
    p_start_dst += 8;
  }

  
  int16_t* p_start_window = (int16_t*) inst->window;
  int16_t* p_start_buffer = inst->analysisBuffer;
  int16_t* p_start_out = out;
  const int16_t* p_end_out = out + inst->anaLen;

  
  int16x8_t window = vld1q_s16(p_start_window);
  int16x8_t buffer = vld1q_s16(p_start_buffer);
  p_start_window += 8;
  p_start_buffer += 8;

  while (p_start_out < p_end_out) {
    
    int32x4_t tmp32_low = vmull_s16(vget_low_s16(window), vget_low_s16(buffer));
    int32x4_t tmp32_high = vmull_s16(vget_high_s16(window),
                                     vget_high_s16(buffer));
    window = vld1q_s16(p_start_window);
    buffer = vld1q_s16(p_start_buffer);

    int16x4_t result_low = vrshrn_n_s32(tmp32_low, 14);
    int16x4_t result_high = vrshrn_n_s32(tmp32_high, 14);
    vst1q_s16(p_start_out, vcombine_s16(result_low, result_high));

    p_start_buffer += 8;
    p_start_window += 8;
    p_start_out += 8;
  }
}



void WebRtcNsx_CreateComplexBufferNeon(NsxInst_t* inst,
                                       int16_t* in,
                                       int16_t* out) {
  int16_t* ptr_out = &out[0];
  int16_t* ptr_in = &in[0];

  __asm__ __volatile__("vdup.16 d25, %0" : : "r"(0) : "d25");
  __asm__ __volatile__("vdup.16 q10, %0" : : "r"(inst->normData) : "q10");
  for (; ptr_in < &in[inst->anaLen];) {

    
    
    __asm__ __volatile__(
      
      "vld1.16 {d22, d23}, [%[ptr_in]]!\n\t"
      "vshl.s16 q11, q10\n\t"
      "vmov d24, d23\n\t"

      
      "vmov d23, d25\n\t"
      "vst2.16 {d22, d23}, [%[ptr_out]]!\n\t"
      "vst2.16 {d24, d25}, [%[ptr_out]]!\n\t"

      
      "vld1.16 {d22, d23}, [%[ptr_in]]!\n\t"
      "vshl.s16 q11, q10\n\t"
      "vmov d24, d23\n\t"

      
      "vmov d23, d25\n\t"
      "vst2.16 {d22, d23}, [%[ptr_out]]!\n\t"
      "vst2.16 {d24, d25}, [%[ptr_out]]!\n\t"

      
      :[ptr_in]"+r"(ptr_in),
       [ptr_out]"+r"(ptr_out)
      :
      :"d22", "d23", "d24", "d25", "q10", "q11"
    );
  }
}
