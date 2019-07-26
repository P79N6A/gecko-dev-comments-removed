













#include "aec_resampler.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "aec_core.h"

enum { kEstimateLengthFrames = 400 };

typedef struct {
    short buffer[kResamplerBufferSize];
    float position;

    int deviceSampleRateHz;
    int skewData[kEstimateLengthFrames];
    int skewDataIndex;
    float skewEstimate;
} resampler_t;

static int EstimateSkew(const int* rawSkew,
                        int size,
                        int absLimit,
                        float *skewEst);

int WebRtcAec_CreateResampler(void **resampInst)
{
    resampler_t *obj = malloc(sizeof(resampler_t));
    *resampInst = obj;
    if (obj == NULL) {
        return -1;
    }

    return 0;
}

int WebRtcAec_InitResampler(void *resampInst, int deviceSampleRateHz)
{
    resampler_t *obj = (resampler_t*) resampInst;
    memset(obj->buffer, 0, sizeof(obj->buffer));
    obj->position = 0.0;

    obj->deviceSampleRateHz = deviceSampleRateHz;
    memset(obj->skewData, 0, sizeof(obj->skewData));
    obj->skewDataIndex = 0;
    obj->skewEstimate = 0.0;

    return 0;
}

int WebRtcAec_FreeResampler(void *resampInst)
{
    resampler_t *obj = (resampler_t*) resampInst;
    free(obj);

    return 0;
}

void WebRtcAec_ResampleLinear(void *resampInst,
                              const short *inspeech,
                              int size,
                              float skew,
                              short *outspeech,
                              int *size_out)
{
    resampler_t *obj = (resampler_t*) resampInst;

    short *y;
    float be, tnew, interp;
    int tn, mm;

    assert(!(size < 0 || size > 2 * FRAME_LEN));
    assert(resampInst != NULL);
    assert(inspeech != NULL);
    assert(outspeech != NULL);
    assert(size_out != NULL);

    
    memcpy(&obj->buffer[FRAME_LEN + kResamplingDelay],
           inspeech,
           size * sizeof(short));

    
    be = 1 + skew;

    
    mm = 0;
    y = &obj->buffer[FRAME_LEN]; 

    tnew = be * mm + obj->position;
    tn = (int) tnew;

    while (tn < size) {

        
        interp = y[tn] + (tnew - tn) * (y[tn+1] - y[tn]);

        if (interp > 32767) {
            interp = 32767;
        }
        else if (interp < -32768) {
            interp = -32768;
        }

        outspeech[mm] = (short) interp;
        mm++;

        tnew = be * mm + obj->position;
        tn = (int) tnew;
    }

    *size_out = mm;
    obj->position += (*size_out) * be - size;

    
    memmove(obj->buffer,
            &obj->buffer[size],
            (kResamplerBufferSize - size) * sizeof(short));
}

int WebRtcAec_GetSkew(void *resampInst, int rawSkew, float *skewEst)
{
    resampler_t *obj = (resampler_t*)resampInst;
    int err = 0;

    if (obj->skewDataIndex < kEstimateLengthFrames) {
        obj->skewData[obj->skewDataIndex] = rawSkew;
        obj->skewDataIndex++;
    }
    else if (obj->skewDataIndex == kEstimateLengthFrames) {
        err = EstimateSkew(obj->skewData,
                           kEstimateLengthFrames,
                           obj->deviceSampleRateHz,
                           skewEst);
        obj->skewEstimate = *skewEst;
        obj->skewDataIndex++;
    }
    else {
        *skewEst = obj->skewEstimate;
    }

    return err;
}

int EstimateSkew(const int* rawSkew,
                 int size,
                 int deviceSampleRateHz,
                 float *skewEst)
{
    const int absLimitOuter = (int)(0.04f * deviceSampleRateHz);
    const int absLimitInner = (int)(0.0025f * deviceSampleRateHz);
    int i = 0;
    int n = 0;
    float rawAvg = 0;
    float err = 0;
    float rawAbsDev = 0;
    int upperLimit = 0;
    int lowerLimit = 0;
    float cumSum = 0;
    float x = 0;
    float x2 = 0;
    float y = 0;
    float xy = 0;
    float xAvg = 0;
    float denom = 0;
    float skew = 0;

    *skewEst = 0; 
    for (i = 0; i < size; i++) {
      if ((rawSkew[i] < absLimitOuter && rawSkew[i] > -absLimitOuter)) {
        n++;
        rawAvg += rawSkew[i];
      }
    }

    if (n == 0) {
      return -1;
    }
    assert(n > 0);
    rawAvg /= n;

    for (i = 0; i < size; i++) {
      if ((rawSkew[i] < absLimitOuter && rawSkew[i] > -absLimitOuter)) {
        err = rawSkew[i] - rawAvg;
        rawAbsDev += err >= 0 ? err : -err;
      }
    }
    assert(n > 0);
    rawAbsDev /= n;
    upperLimit = (int)(rawAvg + 5 * rawAbsDev + 1); 
    lowerLimit = (int)(rawAvg - 5 * rawAbsDev - 1); 

    n = 0;
    for (i = 0; i < size; i++) {
        if ((rawSkew[i] < absLimitInner && rawSkew[i] > -absLimitInner) ||
            (rawSkew[i] < upperLimit && rawSkew[i] > lowerLimit)) {
            n++;
            cumSum += rawSkew[i];
            x += n;
            x2 += n*n;
            y += cumSum;
            xy += n * cumSum;
        }
    }

    if (n == 0) {
      return -1;
    }
    assert(n > 0);
    xAvg = x / n;
    denom = x2 - xAvg*x;

    if (denom != 0) {
        skew = (xy - xAvg*y) / denom;
    }

    *skewEst = skew;
    return 0;
}
