













#include "webrtc/modules/audio_processing/agc/digital_agc.h"

#include <assert.h>
#include <string.h>
#ifdef WEBRTC_AGC_DEBUG_DUMP
#include <stdio.h>
#endif

#include "webrtc/modules/audio_processing/agc/include/gain_control.h"















enum { kGenFuncTableSize = 128 };
static const uint16_t kGenFuncTable[kGenFuncTableSize] = {
          256,   485,   786,  1126,  1484,  1849,  2217,  2586,
         2955,  3324,  3693,  4063,  4432,  4801,  5171,  5540,
         5909,  6279,  6648,  7017,  7387,  7756,  8125,  8495,
         8864,  9233,  9603,  9972, 10341, 10711, 11080, 11449,
        11819, 12188, 12557, 12927, 13296, 13665, 14035, 14404,
        14773, 15143, 15512, 15881, 16251, 16620, 16989, 17359,
        17728, 18097, 18466, 18836, 19205, 19574, 19944, 20313,
        20682, 21052, 21421, 21790, 22160, 22529, 22898, 23268,
        23637, 24006, 24376, 24745, 25114, 25484, 25853, 26222,
        26592, 26961, 27330, 27700, 28069, 28438, 28808, 29177,
        29546, 29916, 30285, 30654, 31024, 31393, 31762, 32132,
        32501, 32870, 33240, 33609, 33978, 34348, 34717, 35086,
        35456, 35825, 36194, 36564, 36933, 37302, 37672, 38041,
        38410, 38780, 39149, 39518, 39888, 40257, 40626, 40996,
        41365, 41734, 42104, 42473, 42842, 43212, 43581, 43950,
        44320, 44689, 45058, 45428, 45797, 46166, 46536, 46905
};

static const int16_t kAvgDecayTime = 250; 

int32_t WebRtcAgc_CalculateGainTable(int32_t *gainTable, 
                                     int16_t digCompGaindB, 
                                     int16_t targetLevelDbfs,
                                     uint8_t limiterEnable,
                                     int16_t analogTarget) 
{
    
    uint32_t tmpU32no1, tmpU32no2, absInLevel, logApprox;
    int32_t inLevel, limiterLvl;
    int32_t tmp32, tmp32no1, tmp32no2, numFIX, den, y32;
    const uint16_t kLog10 = 54426; 
    const uint16_t kLog10_2 = 49321; 
    const uint16_t kLogE_1 = 23637; 
    uint16_t constMaxGain;
    uint16_t tmpU16, intPart, fracPart;
    const int16_t kCompRatio = 3;
    const int16_t kSoftLimiterLeft = 1;
    int16_t limiterOffset = 0; 
    int16_t limiterIdx, limiterLvlX;
    int16_t constLinApprox, zeroGainLvl, maxGain, diffGain;
    int16_t i, tmp16, tmp16no1;
    int zeros, zerosScale;

    




    
    tmp32no1 = WEBRTC_SPL_MUL_16_16(digCompGaindB - analogTarget, kCompRatio - 1);
    tmp16no1 = analogTarget - targetLevelDbfs;
    tmp16no1 += WebRtcSpl_DivW32W16ResW16(tmp32no1 + (kCompRatio >> 1), kCompRatio);
    maxGain = WEBRTC_SPL_MAX(tmp16no1, (analogTarget - targetLevelDbfs));
    tmp32no1 = WEBRTC_SPL_MUL_16_16(maxGain, kCompRatio);
    zeroGainLvl = digCompGaindB;
    zeroGainLvl -= WebRtcSpl_DivW32W16ResW16(tmp32no1 + ((kCompRatio - 1) >> 1),
                                             kCompRatio - 1);
    if ((digCompGaindB <= analogTarget) && (limiterEnable))
    {
        zeroGainLvl += (analogTarget - digCompGaindB + kSoftLimiterLeft);
        limiterOffset = 0;
    }

    
    
    
    tmp32no1 = WEBRTC_SPL_MUL_16_16(digCompGaindB, kCompRatio - 1);
    diffGain = WebRtcSpl_DivW32W16ResW16(tmp32no1 + (kCompRatio >> 1), kCompRatio);
    if (diffGain < 0 || diffGain >= kGenFuncTableSize)
    {
        assert(0);
        return -1;
    }

    
    
    
    limiterLvlX = analogTarget - limiterOffset;
    limiterIdx =
        2 + WebRtcSpl_DivW32W16ResW16((int32_t)limiterLvlX << 13, kLog10_2 / 2);
    tmp16no1 = WebRtcSpl_DivW32W16ResW16(limiterOffset + (kCompRatio >> 1), kCompRatio);
    limiterLvl = targetLevelDbfs + tmp16no1;

    
    
    constMaxGain = kGenFuncTable[diffGain]; 

    
    
    
    constLinApprox = 22817; 

    
    
    den = WEBRTC_SPL_MUL_16_U16(20, constMaxGain); 

    for (i = 0; i < 32; i++)
    {
        
        
        tmp16 = (int16_t)WEBRTC_SPL_MUL_16_16(kCompRatio - 1, i - 1); 
        tmp32 = WEBRTC_SPL_MUL_16_U16(tmp16, kLog10_2) + 1; 
        inLevel = WebRtcSpl_DivW32W16(tmp32, kCompRatio); 

        
        inLevel = ((int32_t)diffGain << 14) - inLevel;  

        
        absInLevel = (uint32_t)WEBRTC_SPL_ABS_W32(inLevel); 

        
        intPart = (uint16_t)(absInLevel >> 14);
        fracPart = (uint16_t)(absInLevel & 0x00003FFF); 
        tmpU16 = kGenFuncTable[intPart + 1] - kGenFuncTable[intPart]; 
        tmpU32no1 = tmpU16 * fracPart;  
        tmpU32no1 += (uint32_t)kGenFuncTable[intPart] << 14;  
        logApprox = tmpU32no1 >> 8;  
        
        
        if (inLevel < 0)
        {
            zeros = WebRtcSpl_NormU32(absInLevel);
            zerosScale = 0;
            if (zeros < 15)
            {
                
                tmpU32no2 = absInLevel >> (15 - zeros);  
                tmpU32no2 = WEBRTC_SPL_UMUL_32_16(tmpU32no2, kLogE_1); 
                if (zeros < 9)
                {
                    zerosScale = 9 - zeros;
                    tmpU32no1 >>= zerosScale;  
                } else
                {
                    tmpU32no2 >>= zeros - 9;  
                }
            } else
            {
                tmpU32no2 = WEBRTC_SPL_UMUL_32_16(absInLevel, kLogE_1); 
                tmpU32no2 >>= 6;  
            }
            logApprox = 0;
            if (tmpU32no2 < tmpU32no1)
            {
                logApprox = (tmpU32no1 - tmpU32no2) >> (8 - zerosScale);  
            }
        }
        numFIX = (maxGain * constMaxGain) << 6;  
        numFIX -= (int32_t)logApprox * diffGain;  

        
        
        
        if (numFIX > (den >> 8))  
        {
            zeros = WebRtcSpl_NormW32(numFIX);
        } else
        {
            zeros = WebRtcSpl_NormW32(den) + 8;
        }
        numFIX <<= zeros;  

        
        tmp32no1 = WEBRTC_SPL_SHIFT_W32(den, zeros - 8); 
        if (numFIX < 0)
        {
            numFIX -= tmp32no1 / 2;
        } else
        {
            numFIX += tmp32no1 / 2;
        }
        y32 = numFIX / tmp32no1;  
        if (limiterEnable && (i < limiterIdx))
        {
            tmp32 = WEBRTC_SPL_MUL_16_U16(i - 1, kLog10_2); 
            tmp32 -= limiterLvl << 14;  
            y32 = WebRtcSpl_DivW32W16(tmp32 + 10, 20);
        }
        if (y32 > 39000)
        {
            tmp32 = (y32 >> 1) * kLog10 + 4096;  
            tmp32 >>= 13;  
        } else
        {
            tmp32 = y32 * kLog10 + 8192;  
            tmp32 >>= 14;  
        }
        tmp32 += 16 << 14;  

        
        if (tmp32 > 0)
        {
            intPart = (int16_t)(tmp32 >> 14);
            fracPart = (uint16_t)(tmp32 & 0x00003FFF); 
            if ((fracPart >> 13) != 0)
            {
                tmp16 = (2 << 14) - constLinApprox;
                tmp32no2 = (1 << 14) - fracPart;
                tmp32no2 *= tmp16;
                tmp32no2 >>= 13;
                tmp32no2 = (1 << 14) - tmp32no2;
            } else
            {
                tmp16 = constLinApprox - (1 << 14);
                tmp32no2 = (fracPart * tmp16) >> 13;
            }
            fracPart = (uint16_t)tmp32no2;
            gainTable[i] =
                (1 << intPart) + WEBRTC_SPL_SHIFT_W32(fracPart, intPart - 14);
        } else
        {
            gainTable[i] = 0;
        }
    }

    return 0;
}

int32_t WebRtcAgc_InitDigital(DigitalAgc_t *stt, int16_t agcMode)
{

    if (agcMode == kAgcModeFixedDigital)
    {
        
        stt->capacitorSlow = 0;
    } else
    {
        
        stt->capacitorSlow = 134217728; 
    }
    stt->capacitorFast = 0;
    stt->gain = 65536;
    stt->gatePrevious = 0;
    stt->agcMode = agcMode;
#ifdef WEBRTC_AGC_DEBUG_DUMP
    stt->frameCounter = 0;
#endif

    
    WebRtcAgc_InitVad(&stt->vadNearend);
    WebRtcAgc_InitVad(&stt->vadFarend);

    return 0;
}

int32_t WebRtcAgc_AddFarendToDigital(DigitalAgc_t *stt, const int16_t *in_far,
                                     int16_t nrSamples)
{
    assert(stt != NULL);
    
    WebRtcAgc_ProcessVad(&stt->vadFarend, in_far, nrSamples);

    return 0;
}

int32_t WebRtcAgc_ProcessDigital(DigitalAgc_t *stt, const int16_t *in_near,
                                 const int16_t *in_near_H, int16_t *out,
                                 int16_t *out_H, uint32_t FS,
                                 int16_t lowlevelSignal)
{
    
    int32_t gains[11];

    int32_t out_tmp, tmp32;
    int32_t env[10];
    int32_t nrg, max_nrg;
    int32_t cur_level;
    int32_t gain32, delta;
    int16_t logratio;
    int16_t lower_thr, upper_thr;
    int16_t zeros = 0, zeros_fast, frac = 0;
    int16_t decay;
    int16_t gate, gain_adj;
    int16_t k, n;
    int16_t L, L2; 

    
    if (FS == 8000)
    {
        L = 8;
        L2 = 3;
    } else if (FS == 16000)
    {
        L = 16;
        L2 = 4;
    } else if (FS == 32000)
    {
        L = 16;
        L2 = 4;
    } else
    {
        return -1;
    }

    
    if (in_near != out)
    {
        
        memcpy(out, in_near, 10 * L * sizeof(int16_t));
    }
    if (FS == 32000)
    {
        if (in_near_H != out_H)
        {
            memcpy(out_H, in_near_H, 10 * L * sizeof(int16_t));
        }
    }
    
    logratio = WebRtcAgc_ProcessVad(&stt->vadNearend, out, L * 10);

    
    if (stt->vadFarend.counter > 10)
    {
        tmp32 = WEBRTC_SPL_MUL_16_16(3, logratio);
        logratio = (int16_t)((tmp32 - stt->vadFarend.logRatio) >> 2);
    }

    
    
    
    upper_thr = 1024; 
    lower_thr = 0; 
    if (logratio > upper_thr)
    {
        
        decay = -65;
    } else if (logratio < lower_thr)
    {
        decay = 0;
    } else
    {
        
        
        
        tmp32 = WEBRTC_SPL_MUL_16_16((lower_thr - logratio), 65);
        decay = (int16_t)(tmp32 >> 10);
    }

    
    
    if (stt->agcMode != kAgcModeFixedDigital)
    {
        if (stt->vadNearend.stdLongTerm < 4000)
        {
            decay = 0;
        } else if (stt->vadNearend.stdLongTerm < 8096)
        {
            
            tmp32 = WEBRTC_SPL_MUL_16_16((stt->vadNearend.stdLongTerm - 4000), decay);
            decay = (int16_t)(tmp32 >> 12);
        }

        if (lowlevelSignal != 0)
        {
            decay = 0;
        }
    }
#ifdef WEBRTC_AGC_DEBUG_DUMP
    stt->frameCounter++;
    fprintf(stt->logFile,
            "%5.2f\t%d\t%d\t%d\t",
            (float)(stt->frameCounter) / 100,
            logratio,
            decay,
            stt->vadNearend.stdLongTerm);
#endif
    
    
    for (k = 0; k < 10; k++)
    {
        
        max_nrg = 0;
        for (n = 0; n < L; n++)
        {
            nrg = WEBRTC_SPL_MUL_16_16(out[k * L + n], out[k * L + n]);
            if (nrg > max_nrg)
            {
                max_nrg = nrg;
            }
        }
        env[k] = max_nrg;
    }

    
    gains[0] = stt->gain;
    for (k = 0; k < 10; k++)
    {
        
        
        stt->capacitorFast = AGC_SCALEDIFF32(-1000, stt->capacitorFast, stt->capacitorFast);
        if (env[k] > stt->capacitorFast)
        {
            stt->capacitorFast = env[k];
        }
        
        if (env[k] > stt->capacitorSlow)
        {
            
            stt->capacitorSlow
                    = AGC_SCALEDIFF32(500, (env[k] - stt->capacitorSlow), stt->capacitorSlow);
        } else
        {
            
            stt->capacitorSlow
                    = AGC_SCALEDIFF32(decay, stt->capacitorSlow, stt->capacitorSlow);
        }

        
        if (stt->capacitorFast > stt->capacitorSlow)
        {
            cur_level = stt->capacitorFast;
        } else
        {
            cur_level = stt->capacitorSlow;
        }
        
        
        zeros = WebRtcSpl_NormU32((uint32_t)cur_level);
        if (cur_level == 0)
        {
            zeros = 31;
        }
        tmp32 = (cur_level << zeros) & 0x7FFFFFFF;
        frac = (int16_t)(tmp32 >> 19);  
        tmp32 = (stt->gainTable[zeros-1] - stt->gainTable[zeros]) * frac;
        gains[k + 1] = stt->gainTable[zeros] + (tmp32 >> 12);
#ifdef WEBRTC_AGC_DEBUG_DUMP
        if (k == 0) {
          fprintf(stt->logFile,
                  "%d\t%d\t%d\t%d\t%d\n",
                  env[0],
                  cur_level,
                  stt->capacitorFast,
                  stt->capacitorSlow,
                  zeros);
        }
#endif
    }

    
    zeros = (zeros << 9) - (frac >> 3);
    
    zeros_fast = WebRtcSpl_NormU32((uint32_t)stt->capacitorFast);
    if (stt->capacitorFast == 0)
    {
        zeros_fast = 31;
    }
    tmp32 = (stt->capacitorFast << zeros_fast) & 0x7FFFFFFF;
    zeros_fast <<= 9;
    zeros_fast -= (int16_t)(tmp32 >> 22);

    gate = 1000 + zeros_fast - zeros - stt->vadNearend.stdShortTerm;

    if (gate < 0)
    {
        stt->gatePrevious = 0;
    } else
    {
        tmp32 = WEBRTC_SPL_MUL_16_16(stt->gatePrevious, 7);
        gate = (int16_t)((gate + tmp32) >> 3);
        stt->gatePrevious = gate;
    }
    
    
    if (gate > 0)
    {
        if (gate < 2500)
        {
            gain_adj = (2500 - gate) >> 5;
        } else
        {
            gain_adj = 0;
        }
        for (k = 0; k < 10; k++)
        {
            if ((gains[k + 1] - stt->gainTable[0]) > 8388608)
            {
                
                tmp32 = (gains[k + 1] - stt->gainTable[0]) >> 8;
                tmp32 *= 178 + gain_adj;
            } else
            {
                tmp32 = (gains[k+1] - stt->gainTable[0]) * (178 + gain_adj);
                tmp32 >>= 8;
            }
            gains[k + 1] = stt->gainTable[0] + tmp32;
        }
    }

    
    for (k = 0; k < 10; k++)
    {
        
        zeros = 10;
        if (gains[k + 1] > 47453132)
        {
            zeros = 16 - WebRtcSpl_NormW32(gains[k + 1]);
        }
        gain32 = (gains[k + 1] >> zeros) + 1;
        gain32 *= gain32;
        
        while (AGC_MUL32((env[k] >> 12) + 1, gain32)
                > WEBRTC_SPL_SHIFT_W32((int32_t)32767, 2 * (1 - zeros + 10)))
        {
            
            if (gains[k + 1] > 8388607)
            {
                
                gains[k + 1] = (gains[k+1] / 256) * 253;
            } else
            {
                gains[k + 1] = (gains[k+1] * 253) / 256;
            }
            gain32 = (gains[k + 1] >> zeros) + 1;
            gain32 *= gain32;
        }
    }
    
    for (k = 1; k < 10; k++)
    {
        if (gains[k] > gains[k + 1])
        {
            gains[k] = gains[k + 1];
        }
    }
    
    stt->gain = gains[10];

    
    
    delta = (gains[1] - gains[0]) << (4 - L2);
    gain32 = gains[0] << 4;
    
    for (n = 0; n < L; n++)
    {
        
        tmp32 = out[n] * ((gain32 + 127) >> 7);
        out_tmp = tmp32 >> 16;
        if (out_tmp > 4095)
        {
            out[n] = (int16_t)32767;
        } else if (out_tmp < -4096)
        {
            out[n] = (int16_t)-32768;
        } else
        {
            tmp32 = out[n] * (gain32 >> 4);
            out[n] = (int16_t)(tmp32 >> 16);
        }
        
        if (FS == 32000)
        {
            tmp32 = out_H[n] * ((gain32 + 127) >> 7);
            out_tmp = tmp32 >> 16;
            if (out_tmp > 4095)
            {
                out_H[n] = (int16_t)32767;
            } else if (out_tmp < -4096)
            {
                out_H[n] = (int16_t)-32768;
            } else
            {
                tmp32 = out_H[n] * (gain32 >> 4);
                out_H[n] = (int16_t)(tmp32 >> 16);
            }
        }
        

        gain32 += delta;
    }
    
    for (k = 1; k < 10; k++)
    {
        delta = (gains[k+1] - gains[k]) << (4 - L2);
        gain32 = gains[k] << 4;
        
        for (n = 0; n < L; n++)
        {
            
            tmp32 = out[k * L + n] * (gain32 >> 4);
            out[k * L + n] = (int16_t)(tmp32 >> 16);
            
            if (FS == 32000)
            {
                tmp32 = out_H[k * L + n] * (gain32 >> 4);
                out_H[k * L + n] = (int16_t)(tmp32 >> 16);
            }
            gain32 += delta;
        }
    }

    return 0;
}

void WebRtcAgc_InitVad(AgcVad_t *state)
{
    int16_t k;

    state->HPstate = 0; 
    state->logRatio = 0; 
    
    state->meanLongTerm = 15 << 10;

    
    state->varianceLongTerm = 500 << 8;

    state->stdLongTerm = 0; 
    
    state->meanShortTerm = 15 << 10;

    
    state->varianceShortTerm = 500 << 8;

    state->stdShortTerm = 0; 
    state->counter = 3; 
    for (k = 0; k < 8; k++)
    {
        
        state->downState[k] = 0;
    }
}

int16_t WebRtcAgc_ProcessVad(AgcVad_t *state, 
                                   const int16_t *in, 
                                   int16_t nrSamples) 
{
    int32_t out, nrg, tmp32, tmp32b;
    uint16_t tmpU16;
    int16_t k, subfr, tmp16;
    int16_t buf1[8];
    int16_t buf2[4];
    int16_t HPstate;
    int16_t zeros, dB;

    
    nrg = 0;
    HPstate = state->HPstate;
    for (subfr = 0; subfr < 10; subfr++)
    {
        
        if (nrSamples == 160)
        {
            for (k = 0; k < 8; k++)
            {
                tmp32 = (int32_t)in[2 * k] + (int32_t)in[2 * k + 1];
                tmp32 >>= 1;
                buf1[k] = (int16_t)tmp32;
            }
            in += 16;

            WebRtcSpl_DownsampleBy2(buf1, 8, buf2, state->downState);
        } else
        {
            WebRtcSpl_DownsampleBy2(in, 8, buf2, state->downState);
            in += 8;
        }

        
        for (k = 0; k < 4; k++)
        {
            out = buf2[k] + HPstate;
            tmp32 = 600 * out;
            HPstate = (int16_t)((tmp32 >> 10) - buf2[k]);
            nrg += (out * out) >> 6;
        }
    }
    state->HPstate = HPstate;

    
    if (!(0xFFFF0000 & nrg))
    {
        zeros = 16;
    } else
    {
        zeros = 0;
    }
    if (!(0xFF000000 & (nrg << zeros)))
    {
        zeros += 8;
    }
    if (!(0xF0000000 & (nrg << zeros)))
    {
        zeros += 4;
    }
    if (!(0xC0000000 & (nrg << zeros)))
    {
        zeros += 2;
    }
    if (!(0x80000000 & (nrg << zeros)))
    {
        zeros += 1;
    }

    
    dB = (15 - zeros) << 11;

    

    if (state->counter < kAvgDecayTime)
    {
        
        state->counter++;
    }

    
    tmp32 = (WEBRTC_SPL_MUL_16_16(state->meanShortTerm, 15) + (int32_t)dB);
    state->meanShortTerm = (int16_t)(tmp32 >> 4);

    
    tmp32 = (dB * dB) >> 12;
    tmp32 += state->varianceShortTerm * 15;
    state->varianceShortTerm = tmp32 / 16;

    
    tmp32 = WEBRTC_SPL_MUL_16_16(state->meanShortTerm, state->meanShortTerm);
    tmp32 = (state->varianceShortTerm << 12) - tmp32;
    state->stdShortTerm = (int16_t)WebRtcSpl_Sqrt(tmp32);

    
    tmp32 = WEBRTC_SPL_MUL_16_16(state->meanLongTerm, state->counter) + (int32_t)dB;
    state->meanLongTerm = WebRtcSpl_DivW32W16ResW16(
        tmp32, WebRtcSpl_AddSatW16(state->counter, 1));

    
    tmp32 = (dB * dB) >> 12;
    tmp32 += state->varianceLongTerm * state->counter;
    state->varianceLongTerm = WebRtcSpl_DivW32W16(
        tmp32, WebRtcSpl_AddSatW16(state->counter, 1));

    
    tmp32 = WEBRTC_SPL_MUL_16_16(state->meanLongTerm, state->meanLongTerm);
    tmp32 = (state->varianceLongTerm << 12) - tmp32;
    state->stdLongTerm = (int16_t)WebRtcSpl_Sqrt(tmp32);

    
    tmp16 = 3 << 12;
    tmp32 = WEBRTC_SPL_MUL_16_16(tmp16, (dB - state->meanLongTerm));
    tmp32 = WebRtcSpl_DivW32W16(tmp32, state->stdLongTerm);
    tmpU16 = (13 << 12);
    tmp32b = WEBRTC_SPL_MUL_16_U16(state->logRatio, tmpU16);
    tmp32 += tmp32b >> 10;

    state->logRatio = (int16_t)(tmp32 >> 6);

    
    if (state->logRatio > 2048)
    {
        state->logRatio = 2048;
    }
    if (state->logRatio < -2048)
    {
        state->logRatio = -2048;
    }

    return state->logRatio; 
}
