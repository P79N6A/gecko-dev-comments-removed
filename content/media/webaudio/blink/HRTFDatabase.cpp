



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/HRTFDatabase.h"

#include "core/platform/PlatformMemoryInstrumentation.h"
#include "core/platform/audio/HRTFElevation.h"
#include <wtf/MemoryInstrumentationVector.h>

using namespace std;

namespace WebCore {

const int HRTFDatabase::MinElevation = -45;
const int HRTFDatabase::MaxElevation = 90;
const unsigned HRTFDatabase::RawElevationAngleSpacing = 15;
const unsigned HRTFDatabase::NumberOfRawElevations = 10; 
const unsigned HRTFDatabase::InterpolationFactor = 1;
const unsigned HRTFDatabase::NumberOfTotalElevations = NumberOfRawElevations * InterpolationFactor;

PassOwnPtr<HRTFDatabase> HRTFDatabase::create(float sampleRate)
{
    OwnPtr<HRTFDatabase> hrtfDatabase = adoptPtr(new HRTFDatabase(sampleRate));
    return hrtfDatabase.release();
}

HRTFDatabase::HRTFDatabase(float sampleRate)
    : m_elevations(NumberOfTotalElevations)
    , m_sampleRate(sampleRate)
{
    unsigned elevationIndex = 0;
    for (int elevation = MinElevation; elevation <= MaxElevation; elevation += RawElevationAngleSpacing) {
        OwnPtr<HRTFElevation> hrtfElevation = HRTFElevation::createForSubject("Composite", elevation, sampleRate);
        ASSERT(hrtfElevation.get());
        if (!hrtfElevation.get())
            return;
        
        m_elevations[elevationIndex] = hrtfElevation.release();
        elevationIndex += InterpolationFactor;
    }

    
    if (InterpolationFactor > 1) {
        for (unsigned i = 0; i < NumberOfTotalElevations; i += InterpolationFactor) {
            unsigned j = (i + InterpolationFactor);
            if (j >= NumberOfTotalElevations)
                j = i; 

            
            for (unsigned jj = 1; jj < InterpolationFactor; ++jj) {
                float x = static_cast<float>(jj) / static_cast<float>(InterpolationFactor);
                m_elevations[i + jj] = HRTFElevation::createByInterpolatingSlices(m_elevations[i].get(), m_elevations[j].get(), x, sampleRate);
                ASSERT(m_elevations[i + jj].get());
            }
        }
    }
}

void HRTFDatabase::getKernelsFromAzimuthElevation(double azimuthBlend, unsigned azimuthIndex, double elevationAngle, HRTFKernel* &kernelL, HRTFKernel* &kernelR,
                                                  double& frameDelayL, double& frameDelayR)
{
    unsigned elevationIndex = indexFromElevationAngle(elevationAngle);
    ASSERT_WITH_SECURITY_IMPLICATION(elevationIndex < m_elevations.size() && m_elevations.size() > 0);
    
    if (!m_elevations.size()) {
        kernelL = 0;
        kernelR = 0;
        return;
    }
    
    if (elevationIndex > m_elevations.size() - 1)
        elevationIndex = m_elevations.size() - 1;    
    
    HRTFElevation* hrtfElevation = m_elevations[elevationIndex].get();
    ASSERT(hrtfElevation);
    if (!hrtfElevation) {
        kernelL = 0;
        kernelR = 0;
        return;
    }
    
    hrtfElevation->getKernelsFromAzimuth(azimuthBlend, azimuthIndex, kernelL, kernelR, frameDelayL, frameDelayR);
}                                                     

unsigned HRTFDatabase::indexFromElevationAngle(double elevationAngle)
{
    
    elevationAngle = max(static_cast<double>(MinElevation), elevationAngle);
    elevationAngle = min(static_cast<double>(MaxElevation), elevationAngle);

    unsigned elevationIndex = static_cast<int>(InterpolationFactor * (elevationAngle - MinElevation) / RawElevationAngleSpacing);    
    return elevationIndex;
}

void HRTFDatabase::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, PlatformMemoryTypes::AudioSharedData);
    info.addMember(m_elevations, "elevations");
}

} 

#endif 
