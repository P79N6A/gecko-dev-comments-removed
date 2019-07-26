























#ifndef HRTFPanner_h
#define HRTFPanner_h

#include "core/platform/audio/FFTConvolver.h"
#include "core/platform/audio/HRTFDatabaseLoader.h"
#include "core/platform/audio/Panner.h"
#include "modules/webaudio/DelayDSPKernel.h"

namespace WebCore {

class HRTFPanner : public Panner {
public:
    HRTFPanner(float sampleRate, HRTFDatabaseLoader*);
    virtual ~HRTFPanner();

    
    virtual void pan(double azimuth, double elevation, const AudioBus* inputBus, AudioBus* outputBus, size_t framesToProcess);
    virtual void reset();

    size_t fftSize() const { return m_convolverL1.fftSize(); }

    float sampleRate() const { return m_sampleRate; }

    virtual double tailTime() const OVERRIDE;
    virtual double latencyTime() const OVERRIDE;

private:
    
    
    int calculateDesiredAzimuthIndexAndBlend(double azimuth, double& azimuthBlend);

    RefPtr<HRTFDatabaseLoader> m_databaseLoader;

    float m_sampleRate;

    
    
    
    
    
    
    
    
    

    
    enum CrossfadeSelection {
        CrossfadeSelection1,
        CrossfadeSelection2
    };

    CrossfadeSelection m_crossfadeSelection;

    
    int m_azimuthIndex1;
    double m_elevation1;

    
    int m_azimuthIndex2;
    double m_elevation2;

    
    float m_crossfadeX;

    
    float m_crossfadeIncr;

    FFTConvolver m_convolverL1;
    FFTConvolver m_convolverR1;
    FFTConvolver m_convolverL2;
    FFTConvolver m_convolverR2;

    DelayDSPKernel m_delayLineL;
    DelayDSPKernel m_delayLineR;

    AudioFloatArray m_tempL1;
    AudioFloatArray m_tempR1;
    AudioFloatArray m_tempL2;
    AudioFloatArray m_tempR2;
};

} 

#endif 
