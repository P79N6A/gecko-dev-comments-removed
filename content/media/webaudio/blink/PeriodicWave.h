



























#ifndef PeriodicWave_h
#define PeriodicWave_h

#include "mozilla/dom/OscillatorNodeBinding.h"
#include <nsAutoPtr.h>
#include <nsTArray.h>

namespace WebCore {

typedef nsTArray<float> AudioFloatArray;

class PeriodicWave {
public:
    static PeriodicWave* createSine(float sampleRate);
    static PeriodicWave* createSquare(float sampleRate);
    static PeriodicWave* createSawtooth(float sampleRate);
    static PeriodicWave* createTriangle(float sampleRate);

    
    
    static PeriodicWave* create(float sampleRate,
                                const float* real,
                                const float* imag,
                                size_t numberOfComponents);

    
    
    
    
    
    
    
    
    void waveDataForFundamentalFrequency(float, float* &lowerWaveData, float* &higherWaveData, float& tableInterpolationFactor);

    
    
    float rateScale() const { return m_rateScale; }

    unsigned periodicWaveSize() const { return m_periodicWaveSize; }
    float sampleRate() const { return m_sampleRate; }

private:
    explicit PeriodicWave(float sampleRate);

    void generateBasicWaveform(mozilla::dom::OscillatorType);

    float m_sampleRate;
    unsigned m_periodicWaveSize;
    unsigned m_numberOfRanges;
    float m_centsPerRange;

    
    
    
    
    float m_lowestFundamentalFrequency;

    float m_rateScale;

    unsigned numberOfRanges() const { return m_numberOfRanges; }

    
    unsigned maxNumberOfPartials() const;

    unsigned numberOfPartialsForRange(unsigned rangeIndex) const;

    
    void createBandLimitedTables(const float* real, const float* imag, unsigned numberOfComponents);
    nsTArray<nsAutoPtr<AudioFloatArray> > m_bandLimitedTables;
};

} 

#endif 
