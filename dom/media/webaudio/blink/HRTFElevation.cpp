



























#include "HRTFElevation.h"

#include <speex/speex_resampler.h>
#include "mozilla/PodOperations.h"
#include "AudioSampleFormat.h"

#include "IRC_Composite_C_R0195-incl.cpp"

using namespace std;
using namespace mozilla;
 
namespace WebCore {

const int elevationSpacing = irc_composite_c_r0195_elevation_interval;
const int firstElevation = irc_composite_c_r0195_first_elevation;
const int numberOfElevations = MOZ_ARRAY_LENGTH(irc_composite_c_r0195);

const unsigned HRTFElevation::NumberOfTotalAzimuths = 360 / 15 * 8;

const int rawSampleRate = irc_composite_c_r0195_sample_rate;


const size_t ResponseFrameSize = 256;

size_t HRTFElevation::sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t amount = aMallocSizeOf(this);

    amount += m_kernelListL.SizeOfExcludingThis(aMallocSizeOf);
    for (size_t i = 0; i < m_kernelListL.Length(); i++) {
        amount += m_kernelListL[i]->sizeOfIncludingThis(aMallocSizeOf);
    }

    return amount;
}

size_t HRTFElevation::fftSizeForSampleRate(float sampleRate)
{
    
    
    
    
    
    
    
    MOZ_ASSERT(sampleRate > 1.0 && sampleRate < 1048576.0);

    
    unsigned resampledLength =
        floorf(ResponseFrameSize * sampleRate / rawSampleRate);
    
    
    
    unsigned size = min(resampledLength, 1023U);
    size |= 3;
    
    
    
    
    
    size |= (size >> 1);
    size |= (size >> 2);
    size |= (size >> 4);
    size++;
    MOZ_ASSERT((size & (size - 1)) == 0);

    return size;
}

nsReturnRef<HRTFKernel> HRTFElevation::calculateKernelForAzimuthElevation(int azimuth, int elevation, SpeexResamplerState* resampler, float sampleRate)
{
    int elevationIndex = (elevation - firstElevation) / elevationSpacing;
    MOZ_ASSERT(elevationIndex >= 0 && elevationIndex <= numberOfElevations);

    int numberOfAzimuths = irc_composite_c_r0195[elevationIndex].count;
    int azimuthSpacing = 360 / numberOfAzimuths;
    MOZ_ASSERT(numberOfAzimuths * azimuthSpacing == 360);

    int azimuthIndex = azimuth / azimuthSpacing;
    MOZ_ASSERT(azimuthIndex * azimuthSpacing == azimuth);

    const int16_t (&impulse_response_data)[ResponseFrameSize] =
        irc_composite_c_r0195[elevationIndex].azimuths[azimuthIndex];

    
    
    
    
#ifdef MOZ_SAMPLE_TYPE_S16
#  define RESAMPLER_PROCESS speex_resampler_process_int
    const int16_t* response = impulse_response_data;
    const int16_t* resampledResponse;
#else
#  define RESAMPLER_PROCESS speex_resampler_process_float
    float response[ResponseFrameSize];
    ConvertAudioSamples(impulse_response_data, response, ResponseFrameSize);
    float* resampledResponse;
#endif

    
    const size_t resampledResponseLength = fftSizeForSampleRate(sampleRate) / 2;

    nsAutoTArray<AudioDataValue, 2 * ResponseFrameSize> resampled;
    if (sampleRate == rawSampleRate) {
        resampledResponse = response;
        MOZ_ASSERT(resampledResponseLength == ResponseFrameSize);
    } else {
        resampled.SetLength(resampledResponseLength);
        resampledResponse = resampled.Elements();
        speex_resampler_skip_zeros(resampler);

        
        spx_uint32_t in_len = ResponseFrameSize;
        spx_uint32_t out_len = resampled.Length();
        RESAMPLER_PROCESS(resampler, 0, response, &in_len,
                          resampled.Elements(), &out_len);

        if (out_len < resampled.Length()) {
            
            MOZ_ASSERT(in_len == ResponseFrameSize);
            
            spx_uint32_t out_index = out_len;
            in_len = speex_resampler_get_input_latency(resampler);
            out_len = resampled.Length() - out_index;
            RESAMPLER_PROCESS(resampler, 0, nullptr, &in_len,
                              resampled.Elements() + out_index, &out_len);
            out_index += out_len;
            
            
            PodZero(resampled.Elements() + out_index,
                    resampled.Length() - out_index);
        }

        speex_resampler_reset_mem(resampler);
    }

#ifdef MOZ_SAMPLE_TYPE_S16
    nsAutoTArray<float, 2 * ResponseFrameSize> floatArray;
    floatArray.SetLength(resampledResponseLength);
    float *floatResponse = floatArray.Elements();
    ConvertAudioSamples(resampledResponse,
                        floatResponse, resampledResponseLength);
#else
    float *floatResponse = resampledResponse;
#endif
#undef RESAMPLER_PROCESS

    return HRTFKernel::create(floatResponse, resampledResponseLength, sampleRate);
}




static int maxElevations[] = {
        
        
    90, 
    45, 
    60, 
    45, 
    75, 
    45, 
    60, 
    45, 
    75, 
    45, 
    60, 
    45, 
    75, 
    45, 
    60, 
    45, 
    75, 
    45, 
    60, 
    45, 
    75, 
    45, 
    60, 
    45 
};

nsReturnRef<HRTFElevation> HRTFElevation::createBuiltin(int elevation, float sampleRate)
{
    if (elevation < firstElevation ||
        elevation > firstElevation + numberOfElevations * elevationSpacing ||
        (elevation / elevationSpacing) * elevationSpacing != elevation)
        return nsReturnRef<HRTFElevation>();
        
    
    
    
    static const unsigned AzimuthSpacing = 15;
    static const unsigned NumberOfRawAzimuths = 360 / AzimuthSpacing;
    static_assert(AzimuthSpacing * NumberOfRawAzimuths == 360,
                  "Not a multiple");
    static const unsigned InterpolationFactor =
        NumberOfTotalAzimuths / NumberOfRawAzimuths;
    static_assert(NumberOfTotalAzimuths ==
                  NumberOfRawAzimuths * InterpolationFactor, "Not a multiple");

    HRTFKernelList kernelListL;
    kernelListL.SetLength(NumberOfTotalAzimuths);

    SpeexResamplerState* resampler = sampleRate == rawSampleRate ? nullptr :
        speex_resampler_init(1, rawSampleRate, sampleRate,
                             SPEEX_RESAMPLER_QUALITY_DEFAULT, nullptr);

    
    int interpolatedIndex = 0;
    for (unsigned rawIndex = 0; rawIndex < NumberOfRawAzimuths; ++rawIndex) {
        
        int maxElevation = maxElevations[rawIndex];
        int actualElevation = min(elevation, maxElevation);

        kernelListL[interpolatedIndex] = calculateKernelForAzimuthElevation(rawIndex * AzimuthSpacing, actualElevation, resampler, sampleRate);
            
        interpolatedIndex += InterpolationFactor;
    }

    if (resampler)
        speex_resampler_destroy(resampler);

    
    for (unsigned i = 0; i < NumberOfTotalAzimuths; i += InterpolationFactor) {
        int j = (i + InterpolationFactor) % NumberOfTotalAzimuths;

        
        for (unsigned jj = 1; jj < InterpolationFactor; ++jj) {
            float x = float(jj) / float(InterpolationFactor); 

            kernelListL[i + jj] = HRTFKernel::createInterpolatedKernel(kernelListL[i], kernelListL[j], x);
        }
    }
    
    return nsReturnRef<HRTFElevation>(new HRTFElevation(&kernelListL, elevation, sampleRate));
}

nsReturnRef<HRTFElevation> HRTFElevation::createByInterpolatingSlices(HRTFElevation* hrtfElevation1, HRTFElevation* hrtfElevation2, float x, float sampleRate)
{
    MOZ_ASSERT(hrtfElevation1 && hrtfElevation2);
    if (!hrtfElevation1 || !hrtfElevation2)
        return nsReturnRef<HRTFElevation>();
        
    MOZ_ASSERT(x >= 0.0 && x < 1.0);
    
    HRTFKernelList kernelListL;
    kernelListL.SetLength(NumberOfTotalAzimuths);

    const HRTFKernelList& kernelListL1 = hrtfElevation1->kernelListL();
    const HRTFKernelList& kernelListL2 = hrtfElevation2->kernelListL();
    
    
    for (unsigned i = 0; i < NumberOfTotalAzimuths; ++i) {
        kernelListL[i] = HRTFKernel::createInterpolatedKernel(kernelListL1[i], kernelListL2[i], x);
    }

    
    double angle = (1.0 - x) * hrtfElevation1->elevationAngle() + x * hrtfElevation2->elevationAngle();
    
    return nsReturnRef<HRTFElevation>(new HRTFElevation(&kernelListL, static_cast<int>(angle), sampleRate));
}

void HRTFElevation::getKernelsFromAzimuth(double azimuthBlend, unsigned azimuthIndex, HRTFKernel* &kernelL, HRTFKernel* &kernelR, double& frameDelayL, double& frameDelayR)
{
    bool checkAzimuthBlend = azimuthBlend >= 0.0 && azimuthBlend < 1.0;
    MOZ_ASSERT(checkAzimuthBlend);
    if (!checkAzimuthBlend)
        azimuthBlend = 0.0;
    
    unsigned numKernels = m_kernelListL.Length();

    bool isIndexGood = azimuthIndex < numKernels;
    MOZ_ASSERT(isIndexGood);
    if (!isIndexGood) {
        kernelL = 0;
        kernelR = 0;
        return;
    }
    
    
    
    kernelL = m_kernelListL[azimuthIndex];
    int azimuthIndexR = (numKernels - azimuthIndex) % numKernels;
    kernelR = m_kernelListL[azimuthIndexR];

    frameDelayL = kernelL->frameDelay();
    frameDelayR = kernelR->frameDelay();

    int azimuthIndex2L = (azimuthIndex + 1) % numKernels;
    double frameDelay2L = m_kernelListL[azimuthIndex2L]->frameDelay();
    int azimuthIndex2R = (numKernels - azimuthIndex2L) % numKernels;
    double frameDelay2R = m_kernelListL[azimuthIndex2R]->frameDelay();

    
    frameDelayL = (1.0 - azimuthBlend) * frameDelayL + azimuthBlend * frameDelay2L;
    frameDelayR = (1.0 - azimuthBlend) * frameDelayR + azimuthBlend * frameDelay2R;
}

} 
