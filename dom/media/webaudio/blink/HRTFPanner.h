























#ifndef HRTFPanner_h
#define HRTFPanner_h

#include "FFTConvolver.h"
#include "DelayBuffer.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {
struct AudioChunk;
}

namespace WebCore {

typedef nsTArray<float> AudioFloatArray;

class HRTFDatabaseLoader;

using mozilla::AudioChunk;

class HRTFPanner {
public:
    HRTFPanner(float sampleRate, already_AddRefed<HRTFDatabaseLoader> databaseLoader);
    ~HRTFPanner();

    
    void pan(double azimuth, double elevation, const AudioChunk* inputBus, AudioChunk* outputBus);
    void reset();

    size_t fftSize() const { return m_convolverL1.fftSize(); }

    float sampleRate() const { return m_sampleRate; }

    int maxTailFrames() const;

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
    
    
    int calculateDesiredAzimuthIndexAndBlend(double azimuth, double& azimuthBlend);

    mozilla::RefPtr<HRTFDatabaseLoader> m_databaseLoader;

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

    mozilla::DelayBuffer m_delayLine;

    AudioFloatArray m_tempL1;
    AudioFloatArray m_tempR1;
    AudioFloatArray m_tempL2;
    AudioFloatArray m_tempR2;
};

} 

#endif 
