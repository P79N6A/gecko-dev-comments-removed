









#include "nsx_core.h"

#include <arm_neon.h>
#include <assert.h>


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

  tmp16 = (int16_t) WEBRTC_SPL_RSHIFT_W32(tmp32no2, 21);
  tmp16 -= 21;
  tmp16 += (int16_t) inst->qNoise; 
  if (tmp16 < 0) {
    tmp32no1 = WEBRTC_SPL_RSHIFT_W32(tmp32no1, -tmp16);
  } else {
    tmp32no1 = WEBRTC_SPL_LSHIFT_W32(tmp32no1, tmp16);
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
      tmp16no1 = WEBRTC_SPL_RSHIFT_W16(tmp16, 2);
      inst->noiseEstLogQuantile[offset + i] += tmp16no1;
    } else {
      tmp16 += 1;
      tmp16no1 = WEBRTC_SPL_RSHIFT_W16(tmp16, 1);
      
      tmp16no2 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(tmp16no1, 3, 1);
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

  

  
  
  
  
  
  
  

  int16_t* ptr_real = &inst->real[0];
  int16_t* ptr_imag = &inst->imag[0];
  uint16_t* ptr_noiseSupFilter = &inst->noiseSupFilter[0];

  
  for (; ptr_real < &inst->real[inst->magnLen - 1];) {
    
    __asm__ __volatile__(
      "vld1.16 d20, [%[ptr_real]]\n\t"
      "vld1.16 d22, [%[ptr_imag]]\n\t"
      "vld1.16 d23, [%[ptr_noiseSupFilter]]!\n\t"
      "vmull.s16 q10, d20, d23\n\t"
      "vmull.s16 q11, d22, d23\n\t"
      "vshrn.s32 d20, q10, #14\n\t"
      "vshrn.s32 d22, q11, #14\n\t"
      "vst1.16 d20, [%[ptr_real]]!\n\t"
      "vst1.16 d22, [%[ptr_imag]]!\n\t"

      "vld1.16 d18, [%[ptr_real]]\n\t"
      "vld1.16 d24, [%[ptr_imag]]\n\t"
      "vld1.16 d25, [%[ptr_noiseSupFilter]]!\n\t"
      "vmull.s16 q9, d18, d25\n\t"
      "vmull.s16 q12, d24, d25\n\t"
      "vshrn.s32 d18, q9, #14\n\t"
      "vshrn.s32 d24, q12, #14\n\t"
      "vst1.16 d18, [%[ptr_real]]!\n\t"
      "vst1.16 d24, [%[ptr_imag]]!\n\t"

      
      :[ptr_imag]"+r"(ptr_imag),
       [ptr_real]"+r"(ptr_real),
       [ptr_noiseSupFilter]"+r"(ptr_noiseSupFilter)
      :
      :"d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25",
       "q9", "q10", "q11", "q12"
    );
  }

  
  *ptr_real = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(*ptr_real,
      (int16_t)(*ptr_noiseSupFilter), 14); 
  *ptr_imag = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(*ptr_imag,
      (int16_t)(*ptr_noiseSupFilter), 14); 

  

  
  
  
  
  
  
  
  
  
  
  
  

  freq_buf[0] = inst->real[0];
  freq_buf[1] = -inst->imag[0];

  int offset = -16;
  int16_t* ptr_realImag1 = &freq_buf[2];
  int16_t* ptr_realImag2 = ptr_realImag2 = &freq_buf[(inst->anaLen << 1) - 8];
  ptr_real = &inst->real[1];
  ptr_imag = &inst->imag[1];
  for (; ptr_real < &inst->real[inst->anaLen2 - 11];) {
    
    __asm__ __volatile__(
      "vld1.16 d22, [%[ptr_real]]!\n\t"
      "vld1.16 d23, [%[ptr_imag]]!\n\t"
      
      "vmov.s16 d20, d22\n\t"
      "vneg.s16 d21, d23\n\t"
      "vzip.16 d20, d21\n\t"
      
      "vst1.16 {d20, d21}, [%[ptr_realImag1]]!\n\t"
      
      "vzip.16 d22, d23\n\t"
      "vrev64.32 d18, d23\n\t"
      "vrev64.32 d19, d22\n\t"
      
      "vst1.16 {d18, d19}, [%[ptr_realImag2]], %[offset]\n\t"

      "vld1.16 d22, [%[ptr_real]]!\n\t"
      "vld1.16 d23, [%[ptr_imag]]!\n\t"
      
      "vmov.s16 d20, d22\n\t"
      "vneg.s16 d21, d23\n\t"
      "vzip.16 d20, d21\n\t"
      
      "vst1.16 {d20, d21}, [%[ptr_realImag1]]!\n\t"
      
      "vzip.16 d22, d23\n\t"
      "vrev64.32 d18, d23\n\t"
      "vrev64.32 d19, d22\n\t"
      
      "vst1.16 {d18, d19}, [%[ptr_realImag2]], %[offset]\n\t"

      
      :[ptr_imag]"+r"(ptr_imag),
       [ptr_real]"+r"(ptr_real),
       [ptr_realImag1]"+r"(ptr_realImag1),
       [ptr_realImag2]"+r"(ptr_realImag2)
      :[offset]"r"(offset)
      :"d18", "d19", "d20", "d21", "d22", "d23"
    );
  }
  for (ptr_realImag2 += 6;
       ptr_real <= &inst->real[inst->anaLen2];
       ptr_real += 1, ptr_imag += 1, ptr_realImag1 += 2, ptr_realImag2 -= 2) {
    *ptr_realImag1 = *ptr_real;
    *(ptr_realImag1 + 1) = -(*ptr_imag);
    *ptr_realImag2 = *ptr_real;
    *(ptr_realImag2 + 1) = *ptr_imag;
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
  int16_t* ptr_real = &inst->real[0];
  int16_t* ptr_syn = &inst->synthesisBuffer[0];
  const int16_t* ptr_window = &inst->window[0];

  
  __asm__ __volatile__("vdup.16 d24, %0" : : "r"(gain_factor) : "d24");
  
  for (; ptr_syn < &inst->synthesisBuffer[inst->anaLen];) {
    __asm__ __volatile__(
      
      "vld1.16 d22, [%[ptr_real]]!\n\t"
      "vld1.16 d23, [%[ptr_window]]!\n\t"
      "vld1.16 d25, [%[ptr_syn]]\n\t"
      
      
      "vmull.s16 q11, d22, d23\n\t"
      "vrshrn.i32 d22, q11, #14\n\t"
      
      "vmull.s16 q11, d24, d22\n\t"
      
      "vqrshrn.s32 d22, q11, #13\n\t"
      
      
      "vqadd.s16 d25, d22\n\t"
      "vst1.16 d25, [%[ptr_syn]]!\n\t"

      
      "vld1.16 d26, [%[ptr_real]]!\n\t"
      "vld1.16 d27, [%[ptr_window]]!\n\t"
      "vld1.16 d28, [%[ptr_syn]]\n\t"
      
      
      "vmull.s16 q13, d26, d27\n\t"
      "vrshrn.i32 d26, q13, #14\n\t"
      
      "vmull.s16 q13, d24, d26\n\t"
      
      "vqrshrn.s32 d26, q13, #13\n\t"
      
      
      "vqadd.s16 d28, d26\n\t"
      "vst1.16 d28, [%[ptr_syn]]!\n\t"

      
      :[ptr_real]"+r"(ptr_real),
       [ptr_window]"+r"(ptr_window),
       [ptr_syn]"+r"(ptr_syn)
      :
      :"d22", "d23", "d24", "d25", "d26", "d27", "d28", "q11", "q12", "q13"
    );
  }

  int16_t* ptr_out = &out_frame[0];
  ptr_syn = &inst->synthesisBuffer[0];
  
  for (; ptr_syn < &inst->synthesisBuffer[inst->blockLen10ms];) {
    
    __asm__ __volatile__(
      
      "vld1.16 {d22, d23}, [%[ptr_syn]]!\n\t"
      "vld1.16 {d24, d25}, [%[ptr_syn]]!\n\t"
      "vst1.16 {d22, d23}, [%[ptr_out]]!\n\t"
      "vst1.16 {d24, d25}, [%[ptr_out]]!\n\t"
      :[ptr_syn]"+r"(ptr_syn),
       [ptr_out]"+r"(ptr_out)
      :
      :"d22", "d23", "d24", "d25"
    );
  }

  
  
  
  
  
  ptr_out = &inst->synthesisBuffer[0],
  ptr_syn = &inst->synthesisBuffer[inst->blockLen10ms];
  for (; ptr_syn < &inst->synthesisBuffer[inst->anaLen];) {
    
    __asm__ __volatile__(
      "vld1.16 {d22, d23}, [%[ptr_syn]]!\n\t"
      "vld1.16 {d24, d25}, [%[ptr_syn]]!\n\t"
      "vst1.16 {d22, d23}, [%[ptr_out]]!\n\t"
      "vst1.16 {d24, d25}, [%[ptr_out]]!\n\t"
      :[ptr_syn]"+r"(ptr_syn),
       [ptr_out]"+r"(ptr_out)
      :
      :"d22", "d23", "d24", "d25"
    );
  }

  
  
  
  __asm__ __volatile__("vdup.16 q10, %0" : : "r"(0) : "q10");
  for (; ptr_out < &inst->synthesisBuffer[inst->anaLen];) {
    
    __asm__ __volatile__(
      "vst1.16 {d20, d21}, [%[ptr_out]]!\n\t"
      "vst1.16 {d20, d21}, [%[ptr_out]]!\n\t"
      :[ptr_out]"+r"(ptr_out)
      :
      :"d20", "d21"
    );
  }
}


void WebRtcNsx_AnalysisUpdateNeon(NsxInst_t* inst,
                                  int16_t* out,
                                  int16_t* new_speech) {

  int16_t* ptr_ana = &inst->analysisBuffer[inst->blockLen10ms];
  int16_t* ptr_out = &inst->analysisBuffer[0];

  
  
  
  
  for (; ptr_out < &inst->analysisBuffer[inst->anaLen - inst->blockLen10ms];) {
    
    __asm__ __volatile__(
      "vld1.16 {d20, d21}, [%[ptr_ana]]!\n\t"
      "vst1.16 {d20, d21}, [%[ptr_out]]!\n\t"
      "vld1.16 {d22, d23}, [%[ptr_ana]]!\n\t"
      "vst1.16 {d22, d23}, [%[ptr_out]]!\n\t"
      :[ptr_ana]"+r"(ptr_ana),
       [ptr_out]"+r"(ptr_out)
      :
      :"d20", "d21", "d22", "d23"
    );
  }

  
  
  for (ptr_ana = new_speech; ptr_out < &inst->analysisBuffer[inst->anaLen];) {
    
    __asm__ __volatile__(
      "vld1.16 {d20, d21}, [%[ptr_ana]]!\n\t"
      "vst1.16 {d20, d21}, [%[ptr_out]]!\n\t"
      "vld1.16 {d22, d23}, [%[ptr_ana]]!\n\t"
      "vst1.16 {d22, d23}, [%[ptr_out]]!\n\t"
      :[ptr_ana]"+r"(ptr_ana),
       [ptr_out]"+r"(ptr_out)
      :
      :"d20", "d21", "d22", "d23"
    );
  }

  
  const int16_t* ptr_window = &inst->window[0];
  ptr_out = &out[0];
  ptr_ana = &inst->analysisBuffer[0];
  for (; ptr_out < &out[inst->anaLen];) {

    
    __asm__ __volatile__(
      "vld1.16 d20, [%[ptr_ana]]!\n\t"
      "vld1.16 d21, [%[ptr_window]]!\n\t"
      
      
      "vmull.s16 q10, d20, d21\n\t"
      "vrshrn.i32 d20, q10, #14\n\t"
      "vst1.16 d20, [%[ptr_out]]!\n\t"

      "vld1.16 d22, [%[ptr_ana]]!\n\t"
      "vld1.16 d23, [%[ptr_window]]!\n\t"
      
      
      "vmull.s16 q11, d22, d23\n\t"
      "vrshrn.i32 d22, q11, #14\n\t"
      "vst1.16 d22, [%[ptr_out]]!\n\t"

      
      :[ptr_ana]"+r"(ptr_ana),
       [ptr_window]"+r"(ptr_window),
       [ptr_out]"+r"(ptr_out)
      :
      :"d20", "d21", "d22", "d23", "q10", "q11"
    );
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
