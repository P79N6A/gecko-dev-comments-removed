



























#ifndef HRTFDatabase_h
#define HRTFDatabase_h

#include "core/platform/audio/HRTFElevation.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class HRTFKernel;

class HRTFDatabase {
    WTF_MAKE_NONCOPYABLE(HRTFDatabase);
public:
    static PassOwnPtr<HRTFDatabase> create(float sampleRate);

    
    
    
    
    void getKernelsFromAzimuthElevation(double azimuthBlend, unsigned azimuthIndex, double elevationAngle, HRTFKernel* &kernelL, HRTFKernel* &kernelR, double& frameDelayL, double& frameDelayR);

    
    static unsigned numberOfAzimuths() { return HRTFElevation::NumberOfTotalAzimuths; }

    float sampleRate() const { return m_sampleRate; }

    
    static const unsigned NumberOfRawElevations;

    void reportMemoryUsage(MemoryObjectInfo*) const;

private:
    explicit HRTFDatabase(float sampleRate);

    
    static const int MinElevation;
    static const int MaxElevation;
    static const unsigned RawElevationAngleSpacing;

    
    static const unsigned InterpolationFactor;
    
    
    static const unsigned NumberOfTotalElevations;

    
    static unsigned indexFromElevationAngle(double);

    Vector<OwnPtr<HRTFElevation> > m_elevations;                                            
    float m_sampleRate;
};

} 

#endif 
