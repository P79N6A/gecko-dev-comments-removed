



























#ifndef HRTFElevation_h
#define HRTFElevation_h

#include "core/platform/audio/HRTFKernel.h"
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {



class HRTFElevation {
    WTF_MAKE_NONCOPYABLE(HRTFElevation);
public:
    
    
    
    
    static PassOwnPtr<HRTFElevation> createForSubject(const String& subjectName, int elevation, float sampleRate);

    
    static PassOwnPtr<HRTFElevation> createByInterpolatingSlices(HRTFElevation* hrtfElevation1, HRTFElevation* hrtfElevation2, float x, float sampleRate);

    double elevationAngle() const { return m_elevationAngle; }
    unsigned numberOfAzimuths() const { return NumberOfTotalAzimuths; }
    float sampleRate() const { return m_sampleRate; }
    
    
    
    void getKernelsFromAzimuth(double azimuthBlend, unsigned azimuthIndex, HRTFKernel* &kernelL, HRTFKernel* &kernelR, double& frameDelayL, double& frameDelayR);
    
    
    static const unsigned AzimuthSpacing;
    
    
    static const unsigned NumberOfRawAzimuths;

    
    static const unsigned InterpolationFactor;
    
    
    static const unsigned NumberOfTotalAzimuths;

    void reportMemoryUsage(MemoryObjectInfo*) const;

private:
    HRTFElevation(PassOwnPtr<HRTFKernelList> kernelListL, int elevation, float sampleRate)
        : m_kernelListL(kernelListL)
        , m_elevationAngle(elevation)
        , m_sampleRate(sampleRate)
    {
    }

    
    HRTFKernelList* kernelListL() { return m_kernelListL.get(); }

    
    
    
    
    static bool calculateKernelForAzimuthElevation(int azimuth, int elevation, float sampleRate, const String& subjectName,
                                                   RefPtr<HRTFKernel>& kernelL);

    OwnPtr<HRTFKernelList> m_kernelListL;
    double m_elevationAngle;
    float m_sampleRate;
};

} 

#endif 
