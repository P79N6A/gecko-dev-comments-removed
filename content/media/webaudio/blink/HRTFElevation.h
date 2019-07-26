



























#ifndef HRTFElevation_h
#define HRTFElevation_h

#include "HRTFKernel.h"
#include "nsAutoRef.h"
#include "mozilla/MemoryReporting.h"

struct SpeexResamplerState_;
typedef struct SpeexResamplerState_ SpeexResamplerState;

namespace WebCore {



class HRTFElevation {
public:
    
    
    
    
    static nsReturnRef<HRTFElevation> createBuiltin(int elevation, float sampleRate);

    
    static nsReturnRef<HRTFElevation> createByInterpolatingSlices(HRTFElevation* hrtfElevation1, HRTFElevation* hrtfElevation2, float x, float sampleRate);

    double elevationAngle() const { return m_elevationAngle; }
    unsigned numberOfAzimuths() const { return NumberOfTotalAzimuths; }
    float sampleRate() const { return m_sampleRate; }
    
    
    
    void getKernelsFromAzimuth(double azimuthBlend, unsigned azimuthIndex, HRTFKernel* &kernelL, HRTFKernel* &kernelR, double& frameDelayL, double& frameDelayR);
    
    
    static const unsigned NumberOfTotalAzimuths;

    static size_t fftSizeForSampleRate(float sampleRate);

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
    HRTFElevation(const HRTFElevation& other) MOZ_DELETE;
    void operator=(const HRTFElevation& other) MOZ_DELETE;

    HRTFElevation(HRTFKernelList *kernelListL, int elevation, float sampleRate)
        : m_elevationAngle(elevation)
        , m_sampleRate(sampleRate)
    {
        m_kernelListL.SwapElements(*kernelListL);
    }

    
    const HRTFKernelList& kernelListL() { return m_kernelListL; }

    
    
    
    
    static nsReturnRef<HRTFKernel> calculateKernelForAzimuthElevation(int azimuth, int elevation, SpeexResamplerState* resampler, float sampleRate);

    HRTFKernelList m_kernelListL;
    double m_elevationAngle;
    float m_sampleRate;
};

} 

template <>
class nsAutoRefTraits<WebCore::HRTFElevation> :
    public nsPointerRefTraits<WebCore::HRTFElevation> {
public:
    static void Release(WebCore::HRTFElevation* ptr) { delete(ptr); }
};

#endif 
