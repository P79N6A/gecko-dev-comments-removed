



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/HRTFElevation.h"

#include <math.h>
#include <algorithm>
#include "core/platform/PlatformMemoryInstrumentation.h"
#include "core/platform/audio/AudioBus.h"
#include "core/platform/audio/HRTFPanner.h"
#include <wtf/MemoryInstrumentationVector.h>
#include <wtf/OwnPtr.h>

using namespace std;
 
namespace WebCore {

const unsigned HRTFElevation::AzimuthSpacing = 15;
const unsigned HRTFElevation::NumberOfRawAzimuths = 360 / AzimuthSpacing;
const unsigned HRTFElevation::InterpolationFactor = 8;
const unsigned HRTFElevation::NumberOfTotalAzimuths = NumberOfRawAzimuths * InterpolationFactor;


const size_t ResponseFrameSize = 256;

bool HRTFElevation::calculateKernelForAzimuthElevation(int azimuth, int elevation, float sampleRate, const String& subjectName,
                                                       RefPtr<HRTFKernel>& kernelL)
{
    
    

    bool isAzimuthGood = azimuth >= 0 && azimuth <= 345 && (azimuth / 15) * 15 == azimuth;
    ASSERT(isAzimuthGood);
    if (!isAzimuthGood)
        return false;

    bool isElevationGood = elevation >= -45 && elevation <= 90 && (elevation / 15) * 15 == elevation;
    ASSERT(isElevationGood);
    if (!isElevationGood)
        return false;
    
    
    
    
    
    int positiveElevation = elevation < 0 ? elevation + 360 : elevation;

    String resourceName = String::format("IRC_%s_C_R0195_T%03d_P%03d", subjectName.utf8().data(), azimuth, positiveElevation);

    RefPtr<AudioBus> impulseResponse(AudioBus::loadPlatformResource(resourceName.utf8().data(), sampleRate));

    ASSERT(impulseResponse.get());
    if (!impulseResponse.get())
        return false;
    
    size_t responseLength = impulseResponse->length();
    size_t expectedLength = static_cast<size_t>(256 * (sampleRate / 44100.0));

    
    bool isBusGood = responseLength == expectedLength && impulseResponse->numberOfChannels() == 2;
    ASSERT(isBusGood);
    if (!isBusGood)
        return false;
    
    AudioChannel* leftEarImpulseResponse = impulseResponse->channelByType(AudioBus::ChannelLeft);

    
    const size_t fftSize = HRTFPanner::fftSizeForSampleRate(sampleRate);
    MOZ_ASSERT(responseLength >= fftSize / 2);
    if (responseLength < fftSize / 2)
        return false;

    kernelL = HRTFKernel::create(leftEarImpulseResponse, fftSize / 2, sampleRate);
    
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
    bool isElevationGood = elevation >= -45 && elevation <= 90 && (elevation / 15) * 15 == elevation;
    ASSERT(isElevationGood);
    if (!isElevationGood)
        return nullptr;
        
    OwnPtr<HRTFKernelList> kernelListL = adoptPtr(new HRTFKernelList(NumberOfTotalAzimuths));

    
    int interpolatedIndex = 0;
    for (unsigned rawIndex = 0; rawIndex < NumberOfRawAzimuths; ++rawIndex) {
        
        int maxElevation = maxElevations[rawIndex];
        int actualElevation = min(elevation, maxElevation);

        bool success = calculateKernelForAzimuthElevation(rawIndex * AzimuthSpacing, actualElevation, sampleRate, subjectName, kernelListL->at(interpolatedIndex));
        if (!success)
            return nullptr;
            
        interpolatedIndex += InterpolationFactor;
    }

    
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
