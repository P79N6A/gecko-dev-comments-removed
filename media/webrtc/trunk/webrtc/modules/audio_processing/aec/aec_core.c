













#include "webrtc/modules/audio_processing/aec/aec_core.h"

#ifdef WEBRTC_AEC_DEBUG_DUMP
#include <stdio.h>
#endif

#include <assert.h>
#include <math.h>
#include <stddef.h>  
#include <stdlib.h>
#include <string.h>

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_processing/aec/aec_common.h"
#include "webrtc/modules/audio_processing/aec/aec_core_internal.h"
#include "webrtc/modules/audio_processing/aec/aec_rdft.h"
#include "webrtc/modules/audio_processing/utility/delay_estimator_wrapper.h"
#include "webrtc/modules/audio_processing/utility/ring_buffer.h"
#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"
#include "webrtc/typedefs.h"

extern int AECDebug();
extern uint32_t AECDebugMaxSize();
extern void AECDebugEnable(uint32_t enable);
extern void AECDebugFilenameBase(char *buffer, size_t size);
static void OpenCoreDebugFiles(AecCore* aec, int *instance_count);


static const size_t kBufSizePartitions = 250;  


static const int subCountLen = 4;
static const int countLen = 50;


static const int flagHbandCn = 1;  
static const float cnScaleHband =
    (float)0.4;  

static const int freqAvgIc = PART_LEN / 2;




ALIGN16_BEG const float ALIGN16_END WebRtcAec_sqrtHanning[65] = {
    0.00000000000000f, 0.02454122852291f, 0.04906767432742f, 0.07356456359967f,
    0.09801714032956f, 0.12241067519922f, 0.14673047445536f, 0.17096188876030f,
    0.19509032201613f, 0.21910124015687f, 0.24298017990326f, 0.26671275747490f,
    0.29028467725446f, 0.31368174039889f, 0.33688985339222f, 0.35989503653499f,
    0.38268343236509f, 0.40524131400499f, 0.42755509343028f, 0.44961132965461f,
    0.47139673682600f, 0.49289819222978f, 0.51410274419322f, 0.53499761988710f,
    0.55557023301960f, 0.57580819141785f, 0.59569930449243f, 0.61523159058063f,
    0.63439328416365f, 0.65317284295378f, 0.67155895484702f, 0.68954054473707f,
    0.70710678118655f, 0.72424708295147f, 0.74095112535496f, 0.75720884650648f,
    0.77301045336274f, 0.78834642762661f, 0.80320753148064f, 0.81758481315158f,
    0.83146961230255f, 0.84485356524971f, 0.85772861000027f, 0.87008699110871f,
    0.88192126434835f, 0.89322430119552f, 0.90398929312344f, 0.91420975570353f,
    0.92387953251129f, 0.93299279883474f, 0.94154406518302f, 0.94952818059304f,
    0.95694033573221f, 0.96377606579544f, 0.97003125319454f, 0.97570213003853f,
    0.98078528040323f, 0.98527764238894f, 0.98917650996478f, 0.99247953459871f,
    0.99518472667220f, 0.99729045667869f, 0.99879545620517f, 0.99969881869620f,
    1.00000000000000f};




ALIGN16_BEG const float ALIGN16_END WebRtcAec_weightCurve[65] = {
    0.0000f, 0.1000f, 0.1378f, 0.1535f, 0.1655f, 0.1756f, 0.1845f, 0.1926f,
    0.2000f, 0.2069f, 0.2134f, 0.2195f, 0.2254f, 0.2309f, 0.2363f, 0.2414f,
    0.2464f, 0.2512f, 0.2558f, 0.2604f, 0.2648f, 0.2690f, 0.2732f, 0.2773f,
    0.2813f, 0.2852f, 0.2890f, 0.2927f, 0.2964f, 0.3000f, 0.3035f, 0.3070f,
    0.3104f, 0.3138f, 0.3171f, 0.3204f, 0.3236f, 0.3268f, 0.3299f, 0.3330f,
    0.3360f, 0.3390f, 0.3420f, 0.3449f, 0.3478f, 0.3507f, 0.3535f, 0.3563f,
    0.3591f, 0.3619f, 0.3646f, 0.3673f, 0.3699f, 0.3726f, 0.3752f, 0.3777f,
    0.3803f, 0.3828f, 0.3854f, 0.3878f, 0.3903f, 0.3928f, 0.3952f, 0.3976f,
    0.4000f};




ALIGN16_BEG const float ALIGN16_END WebRtcAec_overDriveCurve[65] = {
    1.0000f, 1.1250f, 1.1768f, 1.2165f, 1.2500f, 1.2795f, 1.3062f, 1.3307f,
    1.3536f, 1.3750f, 1.3953f, 1.4146f, 1.4330f, 1.4507f, 1.4677f, 1.4841f,
    1.5000f, 1.5154f, 1.5303f, 1.5449f, 1.5590f, 1.5728f, 1.5863f, 1.5995f,
    1.6124f, 1.6250f, 1.6374f, 1.6495f, 1.6614f, 1.6731f, 1.6847f, 1.6960f,
    1.7071f, 1.7181f, 1.7289f, 1.7395f, 1.7500f, 1.7603f, 1.7706f, 1.7806f,
    1.7906f, 1.8004f, 1.8101f, 1.8197f, 1.8292f, 1.8385f, 1.8478f, 1.8570f,
    1.8660f, 1.8750f, 1.8839f, 1.8927f, 1.9014f, 1.9100f, 1.9186f, 1.9270f,
    1.9354f, 1.9437f, 1.9520f, 1.9601f, 1.9682f, 1.9763f, 1.9843f, 1.9922f,
    2.0000f};



static const float kTargetSupp[3] = {-6.9f, -11.5f, -18.4f};


static const float kExtendedMinOverDrive[3] = {3.0f, 6.0f, 15.0f};
static const float kNormalMinOverDrive[3] = {1.0f, 2.0f, 5.0f};
const float WebRtcAec_kExtendedSmoothingCoefficients[2][2] = {{0.9f, 0.1f},
                                                              {0.92f, 0.08f}};
const float WebRtcAec_kNormalSmoothingCoefficients[2][2] = {{0.9f, 0.1f},
                                                            {0.93f, 0.07f}};


enum {
  kPrefBandSize = 24
};

#ifdef WEBRTC_AEC_DEBUG_DUMP
extern int webrtc_aec_instance_count;
#endif

WebRtcAec_FilterFar_t WebRtcAec_FilterFar;
WebRtcAec_ScaleErrorSignal_t WebRtcAec_ScaleErrorSignal;
WebRtcAec_FilterAdaptation_t WebRtcAec_FilterAdaptation;
WebRtcAec_OverdriveAndSuppress_t WebRtcAec_OverdriveAndSuppress;
WebRtcAec_ComfortNoise_t WebRtcAec_ComfortNoise;
WebRtcAec_SubbandCoherence_t WebRtcAec_SubbandCoherence;

__inline static float MulRe(float aRe, float aIm, float bRe, float bIm) {
  return aRe * bRe - aIm * bIm;
}

__inline static float MulIm(float aRe, float aIm, float bRe, float bIm) {
  return aRe * bIm + aIm * bRe;
}

static int CmpFloat(const void* a, const void* b) {
  const float* da = (const float*)a;
  const float* db = (const float*)b;

  return (*da > *db) - (*da < *db);
}

static void FilterFar(AecCore* aec, float yf[2][PART_LEN1]) {
  int i;
  for (i = 0; i < aec->num_partitions; i++) {
    int j;
    int xPos = (i + aec->xfBufBlockPos) * PART_LEN1;
    int pos = i * PART_LEN1;
    
    if (i + aec->xfBufBlockPos >= aec->num_partitions) {
      xPos -= aec->num_partitions * (PART_LEN1);
    }

    for (j = 0; j < PART_LEN1; j++) {
      yf[0][j] += MulRe(aec->xfBuf[0][xPos + j],
                        aec->xfBuf[1][xPos + j],
                        aec->wfBuf[0][pos + j],
                        aec->wfBuf[1][pos + j]);
      yf[1][j] += MulIm(aec->xfBuf[0][xPos + j],
                        aec->xfBuf[1][xPos + j],
                        aec->wfBuf[0][pos + j],
                        aec->wfBuf[1][pos + j]);
    }
  }
}

static void ScaleErrorSignal(AecCore* aec, float ef[2][PART_LEN1]) {
  const float mu = aec->extended_filter_enabled ? kExtendedMu : aec->normal_mu;
  const float error_threshold = aec->extended_filter_enabled
                                    ? kExtendedErrorThreshold
                                    : aec->normal_error_threshold;
  int i;
  float abs_ef;
  for (i = 0; i < (PART_LEN1); i++) {
    ef[0][i] /= (aec->xPow[i] + 1e-10f);
    ef[1][i] /= (aec->xPow[i] + 1e-10f);
    abs_ef = sqrtf(ef[0][i] * ef[0][i] + ef[1][i] * ef[1][i]);

    if (abs_ef > error_threshold) {
      abs_ef = error_threshold / (abs_ef + 1e-10f);
      ef[0][i] *= abs_ef;
      ef[1][i] *= abs_ef;
    }

    
    ef[0][i] *= mu;
    ef[1][i] *= mu;
  }
}



























static void FilterAdaptation(AecCore* aec, float* fft, float ef[2][PART_LEN1]) {
  int i, j;
  for (i = 0; i < aec->num_partitions; i++) {
    int xPos = (i + aec->xfBufBlockPos) * (PART_LEN1);
    int pos;
    
    if (i + aec->xfBufBlockPos >= aec->num_partitions) {
      xPos -= aec->num_partitions * PART_LEN1;
    }

    pos = i * PART_LEN1;

    for (j = 0; j < PART_LEN; j++) {

      fft[2 * j] = MulRe(aec->xfBuf[0][xPos + j],
                         -aec->xfBuf[1][xPos + j],
                         ef[0][j],
                         ef[1][j]);
      fft[2 * j + 1] = MulIm(aec->xfBuf[0][xPos + j],
                             -aec->xfBuf[1][xPos + j],
                             ef[0][j],
                             ef[1][j]);
    }
    fft[1] = MulRe(aec->xfBuf[0][xPos + PART_LEN],
                   -aec->xfBuf[1][xPos + PART_LEN],
                   ef[0][PART_LEN],
                   ef[1][PART_LEN]);

    aec_rdft_inverse_128(fft);
    memset(fft + PART_LEN, 0, sizeof(float) * PART_LEN);

    
    {
      float scale = 2.0f / PART_LEN2;
      for (j = 0; j < PART_LEN; j++) {
        fft[j] *= scale;
      }
    }
    aec_rdft_forward_128(fft);

    aec->wfBuf[0][pos] += fft[0];
    aec->wfBuf[0][pos + PART_LEN] += fft[1];

    for (j = 1; j < PART_LEN; j++) {
      aec->wfBuf[0][pos + j] += fft[2 * j];
      aec->wfBuf[1][pos + j] += fft[2 * j + 1];
    }
  }
}

static void OverdriveAndSuppress(AecCore* aec,
                                 float hNl[PART_LEN1],
                                 const float hNlFb,
                                 float efw[2][PART_LEN1]) {
  int i;
  for (i = 0; i < PART_LEN1; i++) {
    
    if (hNl[i] > hNlFb) {
      hNl[i] = WebRtcAec_weightCurve[i] * hNlFb +
               (1 - WebRtcAec_weightCurve[i]) * hNl[i];
    }
    hNl[i] = powf(hNl[i], aec->overDriveSm * WebRtcAec_overDriveCurve[i]);

    
    efw[0][i] *= hNl[i];
    efw[1][i] *= hNl[i];

    
    
    efw[1][i] *= -1;
  }
}

static int PartitionDelay(const AecCore* aec) {
  
  
  
  
  float wfEnMax = 0;
  int i;
  int delay = 0;

  for (i = 0; i < aec->num_partitions; i++) {
    int j;
    int pos = i * PART_LEN1;
    float wfEn = 0;
    for (j = 0; j < PART_LEN1; j++) {
      wfEn += aec->wfBuf[0][pos + j] * aec->wfBuf[0][pos + j] +
          aec->wfBuf[1][pos + j] * aec->wfBuf[1][pos + j];
    }

    if (wfEn > wfEnMax) {
      wfEnMax = wfEn;
      delay = i;
    }
  }
  return delay;
}


const float WebRtcAec_kMinFarendPSD = 15;










static void SmoothedPSD(AecCore* aec,
                        float efw[2][PART_LEN1],
                        float dfw[2][PART_LEN1],
                        float xfw[2][PART_LEN1]) {
  
  const float* ptrGCoh = aec->extended_filter_enabled
      ? WebRtcAec_kExtendedSmoothingCoefficients[aec->mult - 1]
      : WebRtcAec_kNormalSmoothingCoefficients[aec->mult - 1];
  int i;
  float sdSum = 0, seSum = 0;

  for (i = 0; i < PART_LEN1; i++) {
    aec->sd[i] = ptrGCoh[0] * aec->sd[i] +
                 ptrGCoh[1] * (dfw[0][i] * dfw[0][i] + dfw[1][i] * dfw[1][i]);
    aec->se[i] = ptrGCoh[0] * aec->se[i] +
                 ptrGCoh[1] * (efw[0][i] * efw[0][i] + efw[1][i] * efw[1][i]);
    
    
    
    
    aec->sx[i] =
        ptrGCoh[0] * aec->sx[i] +
        ptrGCoh[1] * WEBRTC_SPL_MAX(
            xfw[0][i] * xfw[0][i] + xfw[1][i] * xfw[1][i],
            WebRtcAec_kMinFarendPSD);

    aec->sde[i][0] =
        ptrGCoh[0] * aec->sde[i][0] +
        ptrGCoh[1] * (dfw[0][i] * efw[0][i] + dfw[1][i] * efw[1][i]);
    aec->sde[i][1] =
        ptrGCoh[0] * aec->sde[i][1] +
        ptrGCoh[1] * (dfw[0][i] * efw[1][i] - dfw[1][i] * efw[0][i]);

    aec->sxd[i][0] =
        ptrGCoh[0] * aec->sxd[i][0] +
        ptrGCoh[1] * (dfw[0][i] * xfw[0][i] + dfw[1][i] * xfw[1][i]);
    aec->sxd[i][1] =
        ptrGCoh[0] * aec->sxd[i][1] +
        ptrGCoh[1] * (dfw[0][i] * xfw[1][i] - dfw[1][i] * xfw[0][i]);

    sdSum += aec->sd[i];
    seSum += aec->se[i];
  }

  
  aec->divergeState = (aec->divergeState ? 1.05f : 1.0f) * seSum > sdSum;

  if (aec->divergeState)
    memcpy(efw, dfw, sizeof(efw[0][0]) * 2 * PART_LEN1);

  
  if (!aec->extended_filter_enabled && seSum > (19.95f * sdSum))
    memset(aec->wfBuf, 0, sizeof(aec->wfBuf));
}


__inline static void WindowData(float* x_windowed, const float* x) {
  int i;
  for (i = 0; i < PART_LEN; i++) {
    x_windowed[i] = x[i] * WebRtcAec_sqrtHanning[i];
    x_windowed[PART_LEN + i] =
        x[PART_LEN + i] * WebRtcAec_sqrtHanning[PART_LEN - i];
  }
}


__inline static void StoreAsComplex(const float* data,
                                    float data_complex[2][PART_LEN1]) {
  int i;
  data_complex[0][0] = data[0];
  data_complex[1][0] = 0;
  for (i = 1; i < PART_LEN; i++) {
    data_complex[0][i] = data[2 * i];
    data_complex[1][i] = data[2 * i + 1];
  }
  data_complex[0][PART_LEN] = data[1];
  data_complex[1][PART_LEN] = 0;
}

static void SubbandCoherence(AecCore* aec,
                             float efw[2][PART_LEN1],
                             float xfw[2][PART_LEN1],
                             float* fft,
                             float* cohde,
                             float* cohxd) {
  float dfw[2][PART_LEN1];
  int i;

  if (aec->delayEstCtr == 0)
    aec->delayIdx = PartitionDelay(aec);

  
  memcpy(xfw,
         aec->xfwBuf + aec->delayIdx * PART_LEN1,
         sizeof(xfw[0][0]) * 2 * PART_LEN1);

  
  WindowData(fft, aec->dBuf);
  aec_rdft_forward_128(fft);
  StoreAsComplex(fft, dfw);

  
  WindowData(fft, aec->eBuf);
  aec_rdft_forward_128(fft);
  StoreAsComplex(fft, efw);

  SmoothedPSD(aec, efw, dfw, xfw);

  
  for (i = 0; i < PART_LEN1; i++) {
    cohde[i] =
        (aec->sde[i][0] * aec->sde[i][0] + aec->sde[i][1] * aec->sde[i][1]) /
        (aec->sd[i] * aec->se[i] + 1e-10f);
    cohxd[i] =
        (aec->sxd[i][0] * aec->sxd[i][0] + aec->sxd[i][1] * aec->sxd[i][1]) /
        (aec->sx[i] * aec->sd[i] + 1e-10f);
  }
}

static void GetHighbandGain(const float* lambda, float* nlpGainHband) {
  int i;

  nlpGainHband[0] = (float)0.0;
  for (i = freqAvgIc; i < PART_LEN1 - 1; i++) {
    nlpGainHband[0] += lambda[i];
  }
  nlpGainHband[0] /= (float)(PART_LEN1 - 1 - freqAvgIc);
}

static void ComfortNoise(AecCore* aec,
                         float efw[2][PART_LEN1],
                         complex_t* comfortNoiseHband,
                         const float* noisePow,
                         const float* lambda) {
  int i, num;
  float rand[PART_LEN];
  float noise, noiseAvg, tmp, tmpAvg;
  int16_t randW16[PART_LEN];
  complex_t u[PART_LEN1];

  const float pi2 = 6.28318530717959f;

  
  WebRtcSpl_RandUArray(randW16, PART_LEN, &aec->seed);
  for (i = 0; i < PART_LEN; i++) {
    rand[i] = ((float)randW16[i]) / 32768;
  }

  
  u[0][0] = 0;
  u[0][1] = 0;
  for (i = 1; i < PART_LEN1; i++) {
    tmp = pi2 * rand[i - 1];

    noise = sqrtf(noisePow[i]);
    u[i][0] = noise * cosf(tmp);
    u[i][1] = -noise * sinf(tmp);
  }
  u[PART_LEN][1] = 0;

  for (i = 0; i < PART_LEN1; i++) {
    
    tmp = sqrtf(WEBRTC_SPL_MAX(1 - lambda[i] * lambda[i], 0));
    
    efw[0][i] += tmp * u[i][0];
    efw[1][i] += tmp * u[i][1];
  }

  
  
  noiseAvg = 0.0;
  tmpAvg = 0.0;
  num = 0;
  if (aec->sampFreq == 32000 && flagHbandCn == 1) {

    
    
    
    for (i = PART_LEN1 >> 1; i < PART_LEN1; i++) {
      num++;
      noiseAvg += sqrtf(noisePow[i]);
    }
    noiseAvg /= (float)num;

    
    
    
    num = 0;
    for (i = PART_LEN1 >> 1; i < PART_LEN1; i++) {
      num++;
      tmpAvg += sqrtf(WEBRTC_SPL_MAX(1 - lambda[i] * lambda[i], 0));
    }
    tmpAvg /= (float)num;

    
    
    
    u[0][0] = 0;
    u[0][1] = 0;
    for (i = 1; i < PART_LEN1; i++) {
      tmp = pi2 * rand[i - 1];

      
      u[i][0] = noiseAvg * (float)cos(tmp);
      u[i][1] = -noiseAvg * (float)sin(tmp);
    }
    u[PART_LEN][1] = 0;

    for (i = 0; i < PART_LEN1; i++) {
      
      comfortNoiseHband[i][0] = tmpAvg * u[i][0];
      comfortNoiseHband[i][1] = tmpAvg * u[i][1];
    }
  }
}

static void InitLevel(PowerLevel* level) {
  const float kBigFloat = 1E17f;

  level->averagelevel = 0;
  level->framelevel = 0;
  level->minlevel = kBigFloat;
  level->frsum = 0;
  level->sfrsum = 0;
  level->frcounter = 0;
  level->sfrcounter = 0;
}

static void InitStats(Stats* stats) {
  stats->instant = kOffsetLevel;
  stats->average = kOffsetLevel;
  stats->max = kOffsetLevel;
  stats->min = kOffsetLevel * (-1);
  stats->sum = 0;
  stats->hisum = 0;
  stats->himean = kOffsetLevel;
  stats->counter = 0;
  stats->hicounter = 0;
}

static void InitMetrics(AecCore* self) {
  self->stateCounter = 0;
  InitLevel(&self->farlevel);
  InitLevel(&self->nearlevel);
  InitLevel(&self->linoutlevel);
  InitLevel(&self->nlpoutlevel);

  InitStats(&self->erl);
  InitStats(&self->erle);
  InitStats(&self->aNlp);
  InitStats(&self->rerl);
}

static void UpdateLevel(PowerLevel* level, float in[2][PART_LEN1]) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  int k = 1;
  
  float energy = (in[0][0] * in[0][0]) / 2;
  energy += (in[0][PART_LEN] * in[0][PART_LEN]) / 2;

  for (k = 1; k < PART_LEN; k++) {
    energy += (in[0][k] * in[0][k] + in[1][k] * in[1][k]);
  }
  energy /= PART_LEN2;

  level->sfrsum += energy;
  level->sfrcounter++;

  if (level->sfrcounter > subCountLen) {
    level->framelevel = level->sfrsum / (subCountLen * PART_LEN);
    level->sfrsum = 0;
    level->sfrcounter = 0;
    if (level->framelevel > 0) {
      if (level->framelevel < level->minlevel) {
        level->minlevel = level->framelevel;  
      } else {
        level->minlevel *= (1 + 0.001f);  
      }
    }
    level->frcounter++;
    level->frsum += level->framelevel;
    if (level->frcounter > countLen) {
      level->averagelevel = level->frsum / countLen;
      level->frsum = 0;
      level->frcounter = 0;
    }
  }
}

static void UpdateMetrics(AecCore* aec) {
  float dtmp, dtmp2;

  const float actThresholdNoisy = 8.0f;
  const float actThresholdClean = 40.0f;
  const float safety = 0.99995f;
  const float noisyPower = 300000.0f;

  float actThreshold;
  float echo, suppressedEcho;

  if (aec->echoState) {  
    aec->stateCounter++;
  }

  if (aec->farlevel.frcounter == 0) {

    if (aec->farlevel.minlevel < noisyPower) {
      actThreshold = actThresholdClean;
    } else {
      actThreshold = actThresholdNoisy;
    }

    if ((aec->stateCounter > (0.5f * countLen * subCountLen)) &&
        (aec->farlevel.sfrcounter == 0)

        
        &&
        (aec->farlevel.averagelevel >
         (actThreshold * aec->farlevel.minlevel))) {

      
      echo = aec->nearlevel.averagelevel - safety * aec->nearlevel.minlevel;

      
      dtmp = 10 * (float)log10(aec->farlevel.averagelevel /
                                   aec->nearlevel.averagelevel +
                               1e-10f);
      dtmp2 = 10 * (float)log10(aec->farlevel.averagelevel / echo + 1e-10f);

      aec->erl.instant = dtmp;
      if (dtmp > aec->erl.max) {
        aec->erl.max = dtmp;
      }

      if (dtmp < aec->erl.min) {
        aec->erl.min = dtmp;
      }

      aec->erl.counter++;
      aec->erl.sum += dtmp;
      aec->erl.average = aec->erl.sum / aec->erl.counter;

      
      if (dtmp > aec->erl.average) {
        aec->erl.hicounter++;
        aec->erl.hisum += dtmp;
        aec->erl.himean = aec->erl.hisum / aec->erl.hicounter;
      }

      
      dtmp = 10 * (float)log10(aec->nearlevel.averagelevel /
                                   (2 * aec->linoutlevel.averagelevel) +
                               1e-10f);

      
      suppressedEcho = 2 * (aec->linoutlevel.averagelevel -
                            safety * aec->linoutlevel.minlevel);

      dtmp2 = 10 * (float)log10(echo / suppressedEcho + 1e-10f);

      aec->aNlp.instant = dtmp2;
      if (dtmp > aec->aNlp.max) {
        aec->aNlp.max = dtmp;
      }

      if (dtmp < aec->aNlp.min) {
        aec->aNlp.min = dtmp;
      }

      aec->aNlp.counter++;
      aec->aNlp.sum += dtmp;
      aec->aNlp.average = aec->aNlp.sum / aec->aNlp.counter;

      
      if (dtmp > aec->aNlp.average) {
        aec->aNlp.hicounter++;
        aec->aNlp.hisum += dtmp;
        aec->aNlp.himean = aec->aNlp.hisum / aec->aNlp.hicounter;
      }

      

      
      suppressedEcho = 2 * (aec->nlpoutlevel.averagelevel -
                            safety * aec->nlpoutlevel.minlevel);

      dtmp = 10 * (float)log10(aec->nearlevel.averagelevel /
                                   (2 * aec->nlpoutlevel.averagelevel) +
                               1e-10f);
      dtmp2 = 10 * (float)log10(echo / suppressedEcho + 1e-10f);

      dtmp = dtmp2;
      aec->erle.instant = dtmp;
      if (dtmp > aec->erle.max) {
        aec->erle.max = dtmp;
      }

      if (dtmp < aec->erle.min) {
        aec->erle.min = dtmp;
      }

      aec->erle.counter++;
      aec->erle.sum += dtmp;
      aec->erle.average = aec->erle.sum / aec->erle.counter;

      
      if (dtmp > aec->erle.average) {
        aec->erle.hicounter++;
        aec->erle.hisum += dtmp;
        aec->erle.himean = aec->erle.hisum / aec->erle.hicounter;
      }
    }

    aec->stateCounter = 0;
  }
}

static void TimeToFrequency(float time_data[PART_LEN2],
                            float freq_data[2][PART_LEN1],
                            int window) {
  int i = 0;

  
  if (window) {
    for (i = 0; i < PART_LEN; i++) {
      time_data[i] *= WebRtcAec_sqrtHanning[i];
      time_data[PART_LEN + i] *= WebRtcAec_sqrtHanning[PART_LEN - i];
    }
  }

  aec_rdft_forward_128(time_data);
  
  freq_data[1][0] = 0;
  freq_data[1][PART_LEN] = 0;
  freq_data[0][0] = time_data[0];
  freq_data[0][PART_LEN] = time_data[1];
  for (i = 1; i < PART_LEN; i++) {
    freq_data[0][i] = time_data[2 * i];
    freq_data[1][i] = time_data[2 * i + 1];
  }
}

#ifdef WEBRTC_AEC_DEBUG_DUMP


static void ReopenWav(rtc_WavWriter** wav_file,
                      const char* name,
                      int seq1,
                      int seq2,
                      int sample_rate) {
  int written UNUSED;
  char path[1024];
  char *filename;
  if (*wav_file) {
    if (rtc_WavSampleRate(*wav_file) == sample_rate)
      return;
    rtc_WavClose(*wav_file);
    *wav_file = NULL;
  }
  AECDebugFilenameBase(path, sizeof(path));
  filename = path + strlen(path);
  if (filename > path) {
#ifdef XP_WIN
    if (*(filename-1) != '\\') {
      *filename++ = '\\';
    }
#else
    if (*(filename-1) != '/') {
      *filename++ = '/';
    }
#endif
  }
  written = snprintf(filename, sizeof(path) - (filename-path), "%s%d-%d.wav",
                     name, seq1, seq2);
  assert(written >= 0);  
  assert(filename+written < path + sizeof(path)-1);  
  *wav_file = rtc_WavOpen(path, sample_rate, 1);
}

static void
OpenCoreDebugFiles(AecCore* aec, int *aec_instance_count)
{
  if (AECDebug())
  {
    if (!aec->farFile)
    {
      int process_rate = aec->sampFreq > 16000 ? 16000 : aec->sampFreq;
      ReopenWav(&aec->farFile, "aec_far",
                aec->instance_index, aec->debug_dump_count, process_rate);
      ReopenWav(&aec->nearFile, "aec_near",
                aec->instance_index, aec->debug_dump_count, process_rate);
      ReopenWav(&aec->outFile, "aec_out",
                aec->instance_index, aec->debug_dump_count, process_rate);
      ReopenWav(&aec->outLinearFile, "aec_out_linear",
                aec->instance_index, aec->debug_dump_count, process_rate);
      ++aec->debug_dump_count;
    }
  } else {
    if (aec->farFile) {
      rtc_WavClose(aec->farFile);
    }
    if (aec->nearFile) {
      rtc_WavClose(aec->nearFile);
    }
    if (aec->outFile) {
      rtc_WavClose(aec->outFile);
    }
    if (aec->outLinearFile) {
      rtc_WavClose(aec->outLinearFile);
    }
    aec->outLinearFile = aec->outFile = aec->nearFile = aec->farFile = NULL;
    aec->debugWritten = 0;
  }
}
#endif

static void NonLinearProcessing(AecCore* aec, float* output, float* outputH) {
  float efw[2][PART_LEN1], xfw[2][PART_LEN1];
  complex_t comfortNoiseHband[PART_LEN1];
  float fft[PART_LEN2];
  float scale, dtmp;
  float nlpGainHband;
  int i;

  
  float cohde[PART_LEN1], cohxd[PART_LEN1];
  float hNlDeAvg, hNlXdAvg;
  float hNl[PART_LEN1];
  float hNlPref[kPrefBandSize];
  float hNlFb = 0, hNlFbLow = 0;
  const float prefBandQuant = 0.75f, prefBandQuantLow = 0.5f;
  const int prefBandSize = kPrefBandSize / aec->mult;
  const int minPrefBand = 4 / aec->mult;
  
  const float* min_overdrive = aec->extended_filter_enabled
                                   ? kExtendedMinOverDrive
                                   : kNormalMinOverDrive;

  
  const int delayEstInterval = 10 * aec->mult;

  float* xfw_ptr = NULL;

  aec->delayEstCtr++;
  if (aec->delayEstCtr == delayEstInterval) {
    aec->delayEstCtr = 0;
  }

  
  memset(comfortNoiseHband, 0, sizeof(comfortNoiseHband));
  nlpGainHband = (float)0.0;
  dtmp = (float)0.0;

  
  assert(WebRtc_available_read(aec->far_buf_windowed) > 0);
  
  WebRtc_ReadBuffer(aec->far_buf_windowed, (void**)&xfw_ptr, &xfw[0][0], 1);

  
  
  
  memcpy(aec->xfwBuf, xfw_ptr, sizeof(float) * 2 * PART_LEN1);

  WebRtcAec_SubbandCoherence(aec, efw, xfw, fft, cohde, cohxd);

  hNlXdAvg = 0;
  for (i = minPrefBand; i < prefBandSize + minPrefBand; i++) {
    hNlXdAvg += cohxd[i];
  }
  hNlXdAvg /= prefBandSize;
  hNlXdAvg = 1 - hNlXdAvg;

  hNlDeAvg = 0;
  for (i = minPrefBand; i < prefBandSize + minPrefBand; i++) {
    hNlDeAvg += cohde[i];
  }
  hNlDeAvg /= prefBandSize;

  if (hNlXdAvg < 0.75f && hNlXdAvg < aec->hNlXdAvgMin) {
    aec->hNlXdAvgMin = hNlXdAvg;
  }

  if (hNlDeAvg > 0.98f && hNlXdAvg > 0.9f) {
    aec->stNearState = 1;
  } else if (hNlDeAvg < 0.95f || hNlXdAvg < 0.8f) {
    aec->stNearState = 0;
  }

  if (aec->hNlXdAvgMin == 1) {
    aec->echoState = 0;
    aec->overDrive = min_overdrive[aec->nlp_mode];

    if (aec->stNearState == 1) {
      memcpy(hNl, cohde, sizeof(hNl));
      hNlFb = hNlDeAvg;
      hNlFbLow = hNlDeAvg;
    } else {
      for (i = 0; i < PART_LEN1; i++) {
        hNl[i] = 1 - cohxd[i];
      }
      hNlFb = hNlXdAvg;
      hNlFbLow = hNlXdAvg;
    }
  } else {

    if (aec->stNearState == 1) {
      aec->echoState = 0;
      memcpy(hNl, cohde, sizeof(hNl));
      hNlFb = hNlDeAvg;
      hNlFbLow = hNlDeAvg;
    } else {
      aec->echoState = 1;
      for (i = 0; i < PART_LEN1; i++) {
        hNl[i] = WEBRTC_SPL_MIN(cohde[i], 1 - cohxd[i]);
      }

      
      
      memcpy(hNlPref, &hNl[minPrefBand], sizeof(float) * prefBandSize);
      qsort(hNlPref, prefBandSize, sizeof(float), CmpFloat);
      hNlFb = hNlPref[(int)floor(prefBandQuant * (prefBandSize - 1))];
      hNlFbLow = hNlPref[(int)floor(prefBandQuantLow * (prefBandSize - 1))];
    }
  }

  
  if (hNlFbLow < 0.6f && hNlFbLow < aec->hNlFbLocalMin) {
    aec->hNlFbLocalMin = hNlFbLow;
    aec->hNlFbMin = hNlFbLow;
    aec->hNlNewMin = 1;
    aec->hNlMinCtr = 0;
  }
  aec->hNlFbLocalMin =
      WEBRTC_SPL_MIN(aec->hNlFbLocalMin + 0.0008f / aec->mult, 1);
  aec->hNlXdAvgMin = WEBRTC_SPL_MIN(aec->hNlXdAvgMin + 0.0006f / aec->mult, 1);

  if (aec->hNlNewMin == 1) {
    aec->hNlMinCtr++;
  }
  if (aec->hNlMinCtr == 2) {
    aec->hNlNewMin = 0;
    aec->hNlMinCtr = 0;
    aec->overDrive =
        WEBRTC_SPL_MAX(kTargetSupp[aec->nlp_mode] /
                           ((float)log(aec->hNlFbMin + 1e-10f) + 1e-10f),
                       min_overdrive[aec->nlp_mode]);
  }

  
  if (aec->overDrive < aec->overDriveSm) {
    aec->overDriveSm = 0.99f * aec->overDriveSm + 0.01f * aec->overDrive;
  } else {
    aec->overDriveSm = 0.9f * aec->overDriveSm + 0.1f * aec->overDrive;
  }

  WebRtcAec_OverdriveAndSuppress(aec, hNl, hNlFb, efw);

  
  WebRtcAec_ComfortNoise(aec, efw, comfortNoiseHband, aec->noisePow, hNl);

  
  
  if (aec->metricsMode == 1) {
    
    
    
    
    UpdateLevel(&aec->nlpoutlevel, efw);
  }
  
  fft[0] = efw[0][0];
  fft[1] = efw[0][PART_LEN];
  for (i = 1; i < PART_LEN; i++) {
    fft[2 * i] = efw[0][i];
    
    fft[2 * i + 1] = -efw[1][i];
  }
  aec_rdft_inverse_128(fft);

  
  scale = 2.0f / PART_LEN2;
  for (i = 0; i < PART_LEN; i++) {
    fft[i] *= scale;  
    fft[i] = fft[i] * WebRtcAec_sqrtHanning[i] + aec->outBuf[i];

    fft[PART_LEN + i] *= scale;  
    aec->outBuf[i] = fft[PART_LEN + i] * WebRtcAec_sqrtHanning[PART_LEN - i];

    
    output[i] = WEBRTC_SPL_SAT(
        WEBRTC_SPL_WORD16_MAX, fft[i], WEBRTC_SPL_WORD16_MIN);
  }

  
  if (aec->sampFreq == 32000) {

    
    
    
    GetHighbandGain(hNl, &nlpGainHband);

    
    if (flagHbandCn == 1) {
      fft[0] = comfortNoiseHband[0][0];
      fft[1] = comfortNoiseHband[PART_LEN][0];
      for (i = 1; i < PART_LEN; i++) {
        fft[2 * i] = comfortNoiseHband[i][0];
        fft[2 * i + 1] = comfortNoiseHband[i][1];
      }
      aec_rdft_inverse_128(fft);
      scale = 2.0f / PART_LEN2;
    }

    
    for (i = 0; i < PART_LEN; i++) {
      dtmp = aec->dBufH[i];
      dtmp = dtmp * nlpGainHband;  

      
      if (flagHbandCn == 1) {
        fft[i] *= scale;  
        dtmp += cnScaleHband * fft[i];
      }

      
      outputH[i] = WEBRTC_SPL_SAT(
          WEBRTC_SPL_WORD16_MAX, dtmp, WEBRTC_SPL_WORD16_MIN);
    }
  }

  
  memcpy(aec->dBuf, aec->dBuf + PART_LEN, sizeof(float) * PART_LEN);
  memcpy(aec->eBuf, aec->eBuf + PART_LEN, sizeof(float) * PART_LEN);

  
  if (aec->sampFreq == 32000) {
    memcpy(aec->dBufH, aec->dBufH + PART_LEN, sizeof(float) * PART_LEN);
  }

  memmove(aec->xfwBuf + PART_LEN1,
          aec->xfwBuf,
          sizeof(aec->xfwBuf) - sizeof(complex_t) * PART_LEN1);
}

static void ProcessBlock(AecCore* aec) {
  int i;
  float y[PART_LEN], e[PART_LEN];
  float scale;

  float fft[PART_LEN2];
  float xf[2][PART_LEN1], yf[2][PART_LEN1], ef[2][PART_LEN1];
  float df[2][PART_LEN1];
  float far_spectrum = 0.0f;
  float near_spectrum = 0.0f;
  float abs_far_spectrum[PART_LEN1];
  float abs_near_spectrum[PART_LEN1];

  const float gPow[2] = {0.9f, 0.1f};

  
  const int noiseInitBlocks = 500 * aec->mult;
  const float step = 0.1f;
  const float ramp = 1.0002f;
  const float gInitNoise[2] = {0.999f, 0.001f};

  float nearend[PART_LEN];
  float* nearend_ptr = NULL;
  float output[PART_LEN];
  float outputH[PART_LEN];

  float* xf_ptr = NULL;

  
  if (aec->sampFreq == 32000) {
    WebRtc_ReadBuffer(aec->nearFrBufH, (void**)&nearend_ptr, nearend, PART_LEN);
    memcpy(aec->dBufH + PART_LEN, nearend_ptr, sizeof(nearend));
  }
  WebRtc_ReadBuffer(aec->nearFrBuf, (void**)&nearend_ptr, nearend, PART_LEN);
  memcpy(aec->dBuf + PART_LEN, nearend_ptr, sizeof(nearend));

  

#ifdef WEBRTC_AEC_DEBUG_DUMP
  {
    float farend[PART_LEN];
    float* farend_ptr = NULL;
    WebRtc_ReadBuffer(aec->far_time_buf, (void**)&farend_ptr, farend, 1);
    OpenCoreDebugFiles(aec, &webrtc_aec_instance_count);
    if (aec->farFile) {
      rtc_WavWriteSamples(aec->farFile, farend_ptr, PART_LEN);
      rtc_WavWriteSamples(aec->nearFile, nearend_ptr, PART_LEN);
      aec->debugWritten += sizeof(int16_t) * PART_LEN;
      if (aec->debugWritten >= AECDebugMaxSize()) {
        AECDebugEnable(0);
      }
    }
  }
#endif

  
  assert(WebRtc_available_read(aec->far_buf) > 0);
  WebRtc_ReadBuffer(aec->far_buf, (void**)&xf_ptr, &xf[0][0], 1);

  
  memcpy(fft, aec->dBuf, sizeof(float) * PART_LEN2);
  TimeToFrequency(fft, df, 0);

  
  for (i = 0; i < PART_LEN1; i++) {
    far_spectrum = (xf_ptr[i] * xf_ptr[i]) +
                   (xf_ptr[PART_LEN1 + i] * xf_ptr[PART_LEN1 + i]);
    aec->xPow[i] =
        gPow[0] * aec->xPow[i] + gPow[1] * aec->num_partitions * far_spectrum;
    
    abs_far_spectrum[i] = sqrtf(far_spectrum);

    near_spectrum = df[0][i] * df[0][i] + df[1][i] * df[1][i];
    aec->dPow[i] = gPow[0] * aec->dPow[i] + gPow[1] * near_spectrum;
    
    abs_near_spectrum[i] = sqrtf(near_spectrum);
  }

  
  if (aec->noiseEstCtr > 50) {
    for (i = 0; i < PART_LEN1; i++) {
      if (aec->dPow[i] < aec->dMinPow[i]) {
        aec->dMinPow[i] =
            (aec->dPow[i] + step * (aec->dMinPow[i] - aec->dPow[i])) * ramp;
      } else {
        aec->dMinPow[i] *= ramp;
      }
    }
  }

  
  
  if (aec->noiseEstCtr < noiseInitBlocks) {
    aec->noiseEstCtr++;
    for (i = 0; i < PART_LEN1; i++) {
      if (aec->dMinPow[i] > aec->dInitMinPow[i]) {
        aec->dInitMinPow[i] = gInitNoise[0] * aec->dInitMinPow[i] +
                              gInitNoise[1] * aec->dMinPow[i];
      } else {
        aec->dInitMinPow[i] = aec->dMinPow[i];
      }
    }
    aec->noisePow = aec->dInitMinPow;
  } else {
    aec->noisePow = aec->dMinPow;
  }

  
  if (aec->delay_logging_enabled) {
    int delay_estimate = 0;
    if (WebRtc_AddFarSpectrumFloat(
            aec->delay_estimator_farend, abs_far_spectrum, PART_LEN1) == 0) {
      delay_estimate = WebRtc_DelayEstimatorProcessFloat(
          aec->delay_estimator, abs_near_spectrum, PART_LEN1);
      if (delay_estimate >= 0) {
        
        aec->delay_histogram[delay_estimate]++;
      }
    }
  }

  
  aec->xfBufBlockPos--;
  if (aec->xfBufBlockPos == -1) {
    aec->xfBufBlockPos = aec->num_partitions - 1;
  }

  
  memcpy(aec->xfBuf[0] + aec->xfBufBlockPos * PART_LEN1,
         xf_ptr,
         sizeof(float) * PART_LEN1);
  memcpy(aec->xfBuf[1] + aec->xfBufBlockPos * PART_LEN1,
         &xf_ptr[PART_LEN1],
         sizeof(float) * PART_LEN1);

  memset(yf, 0, sizeof(yf));

  
  WebRtcAec_FilterFar(aec, yf);

  
  fft[0] = yf[0][0];
  fft[1] = yf[0][PART_LEN];
  for (i = 1; i < PART_LEN; i++) {
    fft[2 * i] = yf[0][i];
    fft[2 * i + 1] = yf[1][i];
  }
  aec_rdft_inverse_128(fft);

  scale = 2.0f / PART_LEN2;
  for (i = 0; i < PART_LEN; i++) {
    y[i] = fft[PART_LEN + i] * scale;  
  }

  for (i = 0; i < PART_LEN; i++) {
    e[i] = nearend_ptr[i] - y[i];
  }

  
  memcpy(aec->eBuf + PART_LEN, e, sizeof(float) * PART_LEN);
  memset(fft, 0, sizeof(float) * PART_LEN);
  memcpy(fft + PART_LEN, e, sizeof(float) * PART_LEN);
  
  aec_rdft_forward_128(fft);

  ef[1][0] = 0;
  ef[1][PART_LEN] = 0;
  ef[0][0] = fft[0];
  ef[0][PART_LEN] = fft[1];
  for (i = 1; i < PART_LEN; i++) {
    ef[0][i] = fft[2 * i];
    ef[1][i] = fft[2 * i + 1];
  }

  if (aec->metricsMode == 1) {
    
    
    
    UpdateLevel(&aec->linoutlevel, ef);
  }

  
  WebRtcAec_ScaleErrorSignal(aec, ef);
  WebRtcAec_FilterAdaptation(aec, fft, ef);
  NonLinearProcessing(aec, output, outputH);

  if (aec->metricsMode == 1) {
    
    UpdateLevel(&aec->farlevel, (float(*)[PART_LEN1])xf_ptr);
    UpdateLevel(&aec->nearlevel, df);
    UpdateMetrics(aec);
  }

  
  WebRtc_WriteBuffer(aec->outFrBuf, output, PART_LEN);
  
  if (aec->sampFreq == 32000) {
    WebRtc_WriteBuffer(aec->outFrBufH, outputH, PART_LEN);
  }

#ifdef WEBRTC_AEC_DEBUG_DUMP
  OpenCoreDebugFiles(aec, &webrtc_aec_instance_count);
  if (aec->outLinearFile) {
    rtc_WavWriteSamples(aec->outLinearFile, e, PART_LEN);
    rtc_WavWriteSamples(aec->outFile, output, PART_LEN);
  }
#endif
}

int WebRtcAec_CreateAec(AecCore** aecInst) {
  AecCore* aec = malloc(sizeof(AecCore));
  *aecInst = aec;
  if (aec == NULL) {
    return -1;
  }

  aec->nearFrBuf = WebRtc_CreateBuffer(FRAME_LEN + PART_LEN, sizeof(float));
  if (!aec->nearFrBuf) {
    WebRtcAec_FreeAec(aec);
    aec = NULL;
    return -1;
  }

  aec->outFrBuf = WebRtc_CreateBuffer(FRAME_LEN + PART_LEN, sizeof(float));
  if (!aec->outFrBuf) {
    WebRtcAec_FreeAec(aec);
    aec = NULL;
    return -1;
  }

  aec->nearFrBufH = WebRtc_CreateBuffer(FRAME_LEN + PART_LEN, sizeof(float));
  if (!aec->nearFrBufH) {
    WebRtcAec_FreeAec(aec);
    aec = NULL;
    return -1;
  }

  aec->outFrBufH = WebRtc_CreateBuffer(FRAME_LEN + PART_LEN, sizeof(float));
  if (!aec->outFrBufH) {
    WebRtcAec_FreeAec(aec);
    aec = NULL;
    return -1;
  }

  
  aec->far_buf =
      WebRtc_CreateBuffer(kBufSizePartitions, sizeof(float) * 2 * PART_LEN1);
  if (!aec->far_buf) {
    WebRtcAec_FreeAec(aec);
    aec = NULL;
    return -1;
  }
  aec->far_buf_windowed =
      WebRtc_CreateBuffer(kBufSizePartitions, sizeof(float) * 2 * PART_LEN1);
  if (!aec->far_buf_windowed) {
    WebRtcAec_FreeAec(aec);
    aec = NULL;
    return -1;
  }
#ifdef WEBRTC_AEC_DEBUG_DUMP
  aec->instance_index = webrtc_aec_instance_count;
  aec->far_time_buf =
      WebRtc_CreateBuffer(kBufSizePartitions, sizeof(float) * PART_LEN);
  if (!aec->far_time_buf) {
    WebRtcAec_FreeAec(aec);
    aec = NULL;
    return -1;
  }
  aec->farFile = aec->nearFile = aec->outFile = aec->outLinearFile = NULL;
  aec->debug_dump_count = 0;
  aec->debugWritten = 0;
  OpenCoreDebugFiles(aec, &webrtc_aec_instance_count);
#endif
  aec->delay_estimator_farend =
      WebRtc_CreateDelayEstimatorFarend(PART_LEN1, kHistorySizeBlocks);
  if (aec->delay_estimator_farend == NULL) {
    WebRtcAec_FreeAec(aec);
    aec = NULL;
    return -1;
  }
  aec->delay_estimator = WebRtc_CreateDelayEstimator(
      aec->delay_estimator_farend, kLookaheadBlocks);
  if (aec->delay_estimator == NULL) {
    WebRtcAec_FreeAec(aec);
    aec = NULL;
    return -1;
  }

  
  WebRtcAec_FilterFar = FilterFar;
  WebRtcAec_ScaleErrorSignal = ScaleErrorSignal;
  WebRtcAec_FilterAdaptation = FilterAdaptation;
  WebRtcAec_OverdriveAndSuppress = OverdriveAndSuppress;
  WebRtcAec_ComfortNoise = ComfortNoise;
  WebRtcAec_SubbandCoherence = SubbandCoherence;

#if defined(WEBRTC_ARCH_X86_FAMILY)
  if (WebRtc_GetCPUInfo(kSSE2)) {
    WebRtcAec_InitAec_SSE2();
  }
#endif

#if defined(MIPS_FPU_LE)
  WebRtcAec_InitAec_mips();
#endif

#if defined(WEBRTC_DETECT_ARM_NEON)
  if ((WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON) != 0) {
    WebRtcAec_InitAec_neon();
  }
#elif defined(WEBRTC_ARCH_ARM_NEON)
  WebRtcAec_InitAec_neon();
#endif

  aec_rdft_init();

  return 0;
}

int WebRtcAec_FreeAec(AecCore* aec) {
  if (aec == NULL) {
    return -1;
  }

  WebRtc_FreeBuffer(aec->nearFrBuf);
  WebRtc_FreeBuffer(aec->outFrBuf);

  WebRtc_FreeBuffer(aec->nearFrBufH);
  WebRtc_FreeBuffer(aec->outFrBufH);

  WebRtc_FreeBuffer(aec->far_buf);
  WebRtc_FreeBuffer(aec->far_buf_windowed);
#ifdef WEBRTC_AEC_DEBUG_DUMP
  WebRtc_FreeBuffer(aec->far_time_buf);
  if (aec->farFile) {
    
    rtc_WavClose(aec->farFile);
    rtc_WavClose(aec->nearFile);
    rtc_WavClose(aec->outFile);
    rtc_WavClose(aec->outLinearFile);
  }
#endif
  WebRtc_FreeDelayEstimator(aec->delay_estimator);
  WebRtc_FreeDelayEstimatorFarend(aec->delay_estimator_farend);

  free(aec);
  return 0;
}

int WebRtcAec_InitAec(AecCore* aec, int sampFreq) {
  int i;

  aec->sampFreq = sampFreq;

  if (sampFreq == 8000) {
    aec->normal_mu = 0.6f;
    aec->normal_error_threshold = 2e-6f;
  } else {
    aec->normal_mu = 0.5f;
    aec->normal_error_threshold = 1.5e-6f;
  }

  if (WebRtc_InitBuffer(aec->nearFrBuf) == -1) {
    return -1;
  }

  if (WebRtc_InitBuffer(aec->outFrBuf) == -1) {
    return -1;
  }

  if (WebRtc_InitBuffer(aec->nearFrBufH) == -1) {
    return -1;
  }

  if (WebRtc_InitBuffer(aec->outFrBufH) == -1) {
    return -1;
  }

  
  if (WebRtc_InitBuffer(aec->far_buf) == -1) {
    return -1;
  }
  if (WebRtc_InitBuffer(aec->far_buf_windowed) == -1) {
    return -1;
  }
#ifdef WEBRTC_AEC_DEBUG_DUMP
  if (WebRtc_InitBuffer(aec->far_time_buf) == -1) {
    return -1;
  }
  aec->instance_index = webrtc_aec_instance_count;
  OpenCoreDebugFiles(aec, &webrtc_aec_instance_count);
#endif
  aec->system_delay = 0;

  if (WebRtc_InitDelayEstimatorFarend(aec->delay_estimator_farend) != 0) {
    return -1;
  }
  if (WebRtc_InitDelayEstimator(aec->delay_estimator) != 0) {
    return -1;
  }
  aec->delay_logging_enabled = 0;
  memset(aec->delay_histogram, 0, sizeof(aec->delay_histogram));

  aec->reported_delay_enabled = 1;
  aec->extended_filter_enabled = 0;
  aec->num_partitions = kNormalNumPartitions;

  
  
  
  
  WebRtc_set_allowed_offset(aec->delay_estimator, aec->num_partitions / 2);
  
  
  
  
  WebRtc_enable_robust_validation(aec->delay_estimator, 1);

  
  aec->nlp_mode = 1;

  
  
  if (aec->sampFreq == 32000) {
    aec->mult = (short)aec->sampFreq / 16000;
  } else {
    aec->mult = (short)aec->sampFreq / 8000;
  }

  aec->farBufWritePos = 0;
  aec->farBufReadPos = 0;

  aec->inSamples = 0;
  aec->outSamples = 0;
  aec->knownDelay = 0;

  
  memset(aec->dBuf, 0, sizeof(aec->dBuf));
  memset(aec->eBuf, 0, sizeof(aec->eBuf));
  
  memset(aec->dBufH, 0, sizeof(aec->dBufH));

  memset(aec->xPow, 0, sizeof(aec->xPow));
  memset(aec->dPow, 0, sizeof(aec->dPow));
  memset(aec->dInitMinPow, 0, sizeof(aec->dInitMinPow));
  aec->noisePow = aec->dInitMinPow;
  aec->noiseEstCtr = 0;

  
  for (i = 0; i < PART_LEN1; i++) {
    aec->dMinPow[i] = 1.0e6f;
  }

  
  aec->xfBufBlockPos = 0;
  
  
  memset(aec->xfBuf, 0, sizeof(complex_t) * kExtendedNumPartitions * PART_LEN1);
  memset(aec->wfBuf, 0, sizeof(complex_t) * kExtendedNumPartitions * PART_LEN1);
  memset(aec->sde, 0, sizeof(complex_t) * PART_LEN1);
  memset(aec->sxd, 0, sizeof(complex_t) * PART_LEN1);
  memset(
      aec->xfwBuf, 0, sizeof(complex_t) * kExtendedNumPartitions * PART_LEN1);
  memset(aec->se, 0, sizeof(float) * PART_LEN1);

  
  for (i = 0; i < PART_LEN1; i++) {
    aec->sd[i] = 1;
  }
  for (i = 0; i < PART_LEN1; i++) {
    aec->sx[i] = 1;
  }

  memset(aec->hNs, 0, sizeof(aec->hNs));
  memset(aec->outBuf, 0, sizeof(float) * PART_LEN);

  aec->hNlFbMin = 1;
  aec->hNlFbLocalMin = 1;
  aec->hNlXdAvgMin = 1;
  aec->hNlNewMin = 0;
  aec->hNlMinCtr = 0;
  aec->overDrive = 2;
  aec->overDriveSm = 2;
  aec->delayIdx = 0;
  aec->stNearState = 0;
  aec->echoState = 0;
  aec->divergeState = 0;

  aec->seed = 777;
  aec->delayEstCtr = 0;

  
  aec->metricsMode = 0;
  InitMetrics(aec);

  return 0;
}

void WebRtcAec_BufferFarendPartition(AecCore* aec, const float* farend) {
  float fft[PART_LEN2];
  float xf[2][PART_LEN1];

  
  if (WebRtc_available_write(aec->far_buf) < 1) {
    WebRtcAec_MoveFarReadPtr(aec, 1);
  }
  
  memcpy(fft, farend, sizeof(float) * PART_LEN2);
  TimeToFrequency(fft, xf, 0);
  WebRtc_WriteBuffer(aec->far_buf, &xf[0][0], 1);

  
  memcpy(fft, farend, sizeof(float) * PART_LEN2);
  TimeToFrequency(fft, xf, 1);
  WebRtc_WriteBuffer(aec->far_buf_windowed, &xf[0][0], 1);
}

int WebRtcAec_MoveFarReadPtr(AecCore* aec, int elements) {
  int elements_moved = WebRtc_MoveReadPtr(aec->far_buf_windowed, elements);
  WebRtc_MoveReadPtr(aec->far_buf, elements);
#ifdef WEBRTC_AEC_DEBUG_DUMP
  WebRtc_MoveReadPtr(aec->far_time_buf, elements);
#endif
  aec->system_delay -= elements_moved * PART_LEN;
  return elements_moved;
}

void WebRtcAec_ProcessFrame(AecCore* aec,
                            const float* nearend,
                            const float* nearendH,
                            int knownDelay,
                            float* out,
                            float* outH) {
  int out_elements = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  int move_elements = (aec->knownDelay - knownDelay - 32) / PART_LEN;
  int moved_elements = 0;

  
  
  
  WebRtc_WriteBuffer(aec->nearFrBuf, nearend, FRAME_LEN);
  
  if (aec->sampFreq == 32000) {
    WebRtc_WriteBuffer(aec->nearFrBufH, nearendH, FRAME_LEN);
  }

  
  
  
  if (aec->system_delay < FRAME_LEN) {
    
    WebRtcAec_MoveFarReadPtr(aec, -(aec->mult + 1));
  }

  
  WebRtc_MoveReadPtr(aec->far_buf_windowed, move_elements);
  moved_elements = WebRtc_MoveReadPtr(aec->far_buf, move_elements);
  aec->knownDelay -= moved_elements * PART_LEN;
#ifdef WEBRTC_AEC_DEBUG_DUMP
  WebRtc_MoveReadPtr(aec->far_time_buf, move_elements);
#endif

  
  while (WebRtc_available_read(aec->nearFrBuf) >= PART_LEN) {
    ProcessBlock(aec);
  }

  
  aec->system_delay -= FRAME_LEN;

  
  
  
  out_elements = (int)WebRtc_available_read(aec->outFrBuf);
  if (out_elements < FRAME_LEN) {
    WebRtc_MoveReadPtr(aec->outFrBuf, out_elements - FRAME_LEN);
    if (aec->sampFreq == 32000) {
      WebRtc_MoveReadPtr(aec->outFrBufH, out_elements - FRAME_LEN);
    }
  }
  
  WebRtc_ReadBuffer(aec->outFrBuf, NULL, out, FRAME_LEN);
  
  if (aec->sampFreq == 32000) {
    WebRtc_ReadBuffer(aec->outFrBufH, NULL, outH, FRAME_LEN);
  }
}

int WebRtcAec_GetDelayMetricsCore(AecCore* self, int* median, int* std) {
  int i = 0;
  int delay_values = 0;
  int num_delay_values = 0;
  int my_median = 0;
  const int kMsPerBlock = PART_LEN / (self->mult * 8);
  float l1_norm = 0;

  assert(self != NULL);
  assert(median != NULL);
  assert(std != NULL);

  if (self->delay_logging_enabled == 0) {
    
    return -1;
  }

  
  for (i = 0; i < kHistorySizeBlocks; i++) {
    num_delay_values += self->delay_histogram[i];
  }
  if (num_delay_values == 0) {
    
    
    
    *median = -1;
    *std = -1;
    return 0;
  }

  delay_values = num_delay_values >> 1;  
  
  for (i = 0; i < kHistorySizeBlocks; i++) {
    delay_values -= self->delay_histogram[i];
    if (delay_values < 0) {
      my_median = i;
      break;
    }
  }
  
  *median = (my_median - kLookaheadBlocks) * kMsPerBlock;

  
  for (i = 0; i < kHistorySizeBlocks; i++) {
    l1_norm += (float)abs(i - my_median) * self->delay_histogram[i];
  }
  *std = (int)(l1_norm / (float)num_delay_values + 0.5f) * kMsPerBlock;

  
  memset(self->delay_histogram, 0, sizeof(self->delay_histogram));

  return 0;
}

int WebRtcAec_echo_state(AecCore* self) { return self->echoState; }

void WebRtcAec_GetEchoStats(AecCore* self,
                            Stats* erl,
                            Stats* erle,
                            Stats* a_nlp) {
  assert(erl != NULL);
  assert(erle != NULL);
  assert(a_nlp != NULL);
  *erl = self->erl;
  *erle = self->erle;
  *a_nlp = self->aNlp;
}

#ifdef WEBRTC_AEC_DEBUG_DUMP
void* WebRtcAec_far_time_buf(AecCore* self) { return self->far_time_buf; }
#endif

void WebRtcAec_SetConfigCore(AecCore* self,
                             int nlp_mode,
                             int metrics_mode,
                             int delay_logging) {
  assert(nlp_mode >= 0 && nlp_mode < 3);
  self->nlp_mode = nlp_mode;
  self->metricsMode = metrics_mode;
  if (self->metricsMode) {
    InitMetrics(self);
  }
  self->delay_logging_enabled = delay_logging;
  if (self->delay_logging_enabled) {
    memset(self->delay_histogram, 0, sizeof(self->delay_histogram));
  }
}

void WebRtcAec_enable_reported_delay(AecCore* self, int enable) {
  self->reported_delay_enabled = enable;
}

int WebRtcAec_reported_delay_enabled(AecCore* self) {
  return self->reported_delay_enabled;
}

void WebRtcAec_enable_delay_correction(AecCore* self, int enable) {
  self->extended_filter_enabled = enable;
  self->num_partitions = enable ? kExtendedNumPartitions : kNormalNumPartitions;
  
  WebRtc_set_allowed_offset(self->delay_estimator, self->num_partitions / 2);
}

int WebRtcAec_delay_correction_enabled(AecCore* self) {
  return self->extended_filter_enabled;
}

int WebRtcAec_system_delay(AecCore* self) { return self->system_delay; }

void WebRtcAec_SetSystemDelay(AecCore* self, int delay) {
  assert(delay >= 0);
  self->system_delay = delay;
}

