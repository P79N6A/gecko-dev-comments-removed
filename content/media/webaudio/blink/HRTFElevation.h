



























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

    
    HRTFKernelList* kernelListL() { return m_kernelListL.get(); }
    HRTFKernelList* kernelListR() { return m_kernelListR.get(); }

    double elevationAngle() const { return m_elevationAngle; }
    unsigned numberOfAzimuths() const { return NumberOfTotalAzimuths; }
    float sampleRate() const { return m_sampleRate; }
    
    
    
    void getKernelsFromAzimuth(double azimuthBlend, unsigned azimuthIndex, HRTFKernel* &kernelL, HRTFKernel* &kernelR, double& frameDelayL, double& frameDelayR);
    
    
    static const unsigned AzimuthSpacing;
    
    
    static const unsigned NumberOfRawAzimuths;

    
    static const unsigned InterpolationFactor;
    
    
    static const unsigned NumberOfTotalAzimuths;

    
    
    
    
    static bool calculateKernelsForAzimuthElevation(int azimuth, int elevation, float sampleRate, const String& subjectName,
                                                    RefPtr<HRTFKernel>& kernelL, RefPtr<HRTFKernel>& kernelR);

    
    
    
    static bool calculateSymmetricKernelsForAzimuthElevation(int azimuth, int elevation, float sampleRate, const String& subjectName,
                                                             RefPtr<HRTFKernel>& kernelL, RefPtr<HRTFKernel>& kernelR);

    void reportMemoryUsage(MemoryObjectInfo*) const;

private:
    HRTFElevation(PassOwnPtr<HRTFKernelList> kernelListL, PassOwnPtr<HRTFKernelList> kernelListR, int elevation, float sampleRate)
        : m_kernelListL(kernelListL)
        , m_kernelListR(kernelListR)
        , m_elevationAngle(elevation)
        , m_sampleRate(sampleRate)
    {
    }

    OwnPtr<HRTFKernelList> m_kernelListL;
    OwnPtr<HRTFKernelList> m_kernelListR;
    double m_elevationAngle;
    float m_sampleRate;
};

} 

#endif 
