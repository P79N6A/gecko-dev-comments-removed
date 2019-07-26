



























#ifndef HRTFDatabase_h
#define HRTFDatabase_h

#include "HRTFElevation.h"
#include "nsAutoRef.h"
#include "nsTArray.h"
#include "mozilla/MemoryReporting.h"

namespace WebCore {

class HRTFKernel;

class HRTFDatabase {
public:
    static nsReturnRef<HRTFDatabase> create(float sampleRate);

    
    
    
    
    void getKernelsFromAzimuthElevation(double azimuthBlend, unsigned azimuthIndex, double elevationAngle, HRTFKernel* &kernelL, HRTFKernel* &kernelR, double& frameDelayL, double& frameDelayR);

    
    static unsigned numberOfAzimuths() { return HRTFElevation::NumberOfTotalAzimuths; }

    float sampleRate() const { return m_sampleRate; }

    
    static const unsigned NumberOfRawElevations;

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
    HRTFDatabase(const HRTFDatabase& other) MOZ_DELETE;
    void operator=(const HRTFDatabase& other) MOZ_DELETE;

    explicit HRTFDatabase(float sampleRate);

    
    static const int MinElevation;
    static const int MaxElevation;
    static const unsigned RawElevationAngleSpacing;

    
    static const unsigned InterpolationFactor;
    
    
    static const unsigned NumberOfTotalElevations;

    
    static unsigned indexFromElevationAngle(double);

    nsTArray<nsAutoRef<HRTFElevation> > m_elevations;
    float m_sampleRate;
};

} 

template <>
class nsAutoRefTraits<WebCore::HRTFDatabase> :
    public nsPointerRefTraits<WebCore::HRTFDatabase> {
public:
    static void Release(WebCore::HRTFDatabase* ptr) { delete(ptr); }
};

#endif 
