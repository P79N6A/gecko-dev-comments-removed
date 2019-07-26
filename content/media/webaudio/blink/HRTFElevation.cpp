



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/HRTFElevation.h"

#include "speex/speex_resampler.h"
#include "mozilla/PodOperations.h"
#include "AudioSampleFormat.h"
#include <math.h>
#include <algorithm>
#include "core/platform/PlatformMemoryInstrumentation.h"
#include "core/platform/audio/AudioBus.h"
#include "core/platform/audio/HRTFPanner.h"
#include <wtf/MemoryInstrumentationVector.h>
#include <wtf/OwnPtr.h>

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

size_t HRTFElevation::fftSizeForSampleRate(float sampleRate)
{
    
    
    
    ASSERT(sampleRate >= 44100 && sampleRate <= 96000.0);
    return (sampleRate < 88200.0) ? 512 : 1024;
}

bool HRTFElevation::calculateKernelForAzimuthElevation(int azimuth, int elevation, SpeexResamplerState* resampler, float sampleRate,
                                                       RefPtr<HRTFKernel>& kernelL)
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
    float floatResponse[ResponseFrameSize];
    ConvertAudioSamples(impulse_response_data, floatResponse,
                        ResponseFrameSize);

    
    const size_t responseLength = fftSizeForSampleRate(sampleRate) / 2;

    float* response;
    nsAutoTArray<float, 2 * ResponseFrameSize> resampled;
    if (sampleRate == rawSampleRate) {
        response = floatResponse;
        MOZ_ASSERT(responseLength == ResponseFrameSize);
    } else {
        resampled.SetLength(responseLength);
        response = resampled.Elements();
        speex_resampler_skip_zeros(resampler);

        
        spx_uint32_t in_len = ResponseFrameSize;
        spx_uint32_t out_len = resampled.Length();
        speex_resampler_process_float(resampler, 0, floatResponse, &in_len,
                                      response, &out_len);

        if (out_len < resampled.Length()) {
            
            MOZ_ASSERT(in_len == ResponseFrameSize);
            
            spx_uint32_t out_index = out_len;
            in_len = speex_resampler_get_input_latency(resampler);
            nsAutoTArray<float, 256> zeros;
            zeros.SetLength(in_len);
            PodZero(zeros.Elements(), in_len);
            out_len = resampled.Length() - out_index;
            speex_resampler_process_float(resampler, 0,
                                          zeros.Elements(), &in_len,
                                          response + out_index, &out_len);
            out_index += out_len;
            
            
            PodZero(response + out_index, resampled.Length() - out_index);
        }

        speex_resampler_reset_mem(resampler);
    }

    kernelL = HRTFKernel::create(response, responseLength, sampleRate);
    
    return true;
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

PassOwnPtr<HRTFElevation> HRTFElevation::createForSubject(const String& subjectName, int elevation, float sampleRate)
{
    if (elevation < firstElevation ||
        elevation > firstElevation + numberOfElevations * elevationSpacing ||
        (elevation / elevationSpacing) * elevationSpacing != elevation)
        return nullptr;
        
    
    
    
    static const unsigned AzimuthSpacing = 15;
    static const unsigned NumberOfRawAzimuths = 360 / AzimuthSpacing;
    static_assert(AzimuthSpacing * NumberOfRawAzimuths == 360,
                  "Not a multiple");
    static const unsigned InterpolationFactor =
        NumberOfTotalAzimuths / NumberOfRawAzimuths;
    static_assert(NumberOfTotalAzimuths ==
                  NumberOfRawAzimuths * InterpolationFactor, "Not a multiple");

    OwnPtr<HRTFKernelList> kernelListL = adoptPtr(new HRTFKernelList(NumberOfTotalAzimuths));

    SpeexResamplerState* resampler = sampleRate == rawSampleRate ? nullptr :
        speex_resampler_init(1, rawSampleRate, sampleRate,
                             SPEEX_RESAMPLER_QUALITY_DEFAULT, nullptr);

    
    int interpolatedIndex = 0;
    for (unsigned rawIndex = 0; rawIndex < NumberOfRawAzimuths; ++rawIndex) {
        
        int maxElevation = maxElevations[rawIndex];
        int actualElevation = min(elevation, maxElevation);

        bool success = calculateKernelForAzimuthElevation(rawIndex * AzimuthSpacing, actualElevation, resampler, sampleRate, kernelListL->at(interpolatedIndex));
        if (!success)
            return nullptr;
            
        interpolatedIndex += InterpolationFactor;
    }

    if (resampler)
        speex_resampler_destroy(resampler);

    
    for (unsigned i = 0; i < NumberOfTotalAzimuths; i += InterpolationFactor) {
        int j = (i + InterpolationFactor) % NumberOfTotalAzimuths;

        
        for (unsigned jj = 1; jj < InterpolationFactor; ++jj) {
            float x = float(jj) / float(InterpolationFactor); 

            (*kernelListL)[i + jj] = HRTFKernel::createInterpolatedKernel(kernelListL->at(i).get(), kernelListL->at(j).get(), x);
        }
    }
    
    OwnPtr<HRTFElevation> hrtfElevation = adoptPtr(new HRTFElevation(kernelListL.release(), elevation, sampleRate));
    return hrtfElevation.release();
}

PassOwnPtr<HRTFElevation> HRTFElevation::createByInterpolatingSlices(HRTFElevation* hrtfElevation1, HRTFElevation* hrtfElevation2, float x, float sampleRate)
{
    ASSERT(hrtfElevation1 && hrtfElevation2);
    if (!hrtfElevation1 || !hrtfElevation2)
        return nullptr;
        
    ASSERT(x >= 0.0 && x < 1.0);
    
    OwnPtr<HRTFKernelList> kernelListL = adoptPtr(new HRTFKernelList(NumberOfTotalAzimuths));

    HRTFKernelList* kernelListL1 = hrtfElevation1->kernelListL();
    HRTFKernelList* kernelListL2 = hrtfElevation2->kernelListL();
    
    
    for (unsigned i = 0; i < NumberOfTotalAzimuths; ++i) {
        (*kernelListL)[i] = HRTFKernel::createInterpolatedKernel(kernelListL1->at(i).get(), kernelListL2->at(i).get(), x);
    }

    
    double angle = (1.0 - x) * hrtfElevation1->elevationAngle() + x * hrtfElevation2->elevationAngle();
    
    OwnPtr<HRTFElevation> hrtfElevation = adoptPtr(new HRTFElevation(kernelListL.release(), static_cast<int>(angle), sampleRate));
    return hrtfElevation.release();  
}

void HRTFElevation::getKernelsFromAzimuth(double azimuthBlend, unsigned azimuthIndex, HRTFKernel* &kernelL, HRTFKernel* &kernelR, double& frameDelayL, double& frameDelayR)
{
    bool checkAzimuthBlend = azimuthBlend >= 0.0 && azimuthBlend < 1.0;
    ASSERT(checkAzimuthBlend);
    if (!checkAzimuthBlend)
        azimuthBlend = 0.0;
    
    unsigned numKernels = m_kernelListL->size();

    bool isIndexGood = azimuthIndex < numKernels;
    ASSERT(isIndexGood);
    if (!isIndexGood) {
        kernelL = 0;
        kernelR = 0;
        return;
    }
    
    
    
    kernelL = m_kernelListL->at(azimuthIndex).get();
    int azimuthIndexR = (numKernels - azimuthIndex) % numKernels;
    kernelR = m_kernelListL->at(azimuthIndexR).get();

    frameDelayL = kernelL->frameDelay();
    frameDelayR = kernelR->frameDelay();

    int azimuthIndex2L = (azimuthIndex + 1) % numKernels;
    double frameDelay2L = m_kernelListL->at(azimuthIndex2L)->frameDelay();
    int azimuthIndex2R = (numKernels - azimuthIndex2L) % numKernels;
    double frameDelay2R = m_kernelListL->at(azimuthIndex2R)->frameDelay();

    
    frameDelayL = (1.0 - azimuthBlend) * frameDelayL + azimuthBlend * frameDelay2L;
    frameDelayR = (1.0 - azimuthBlend) * frameDelayR + azimuthBlend * frameDelay2R;
}

void HRTFElevation::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, PlatformMemoryTypes::AudioSharedData);
    info.addMember(m_kernelListL, "kernelListL");
}

} 

#endif 
