



























#ifndef PeriodicWave_h
#define PeriodicWave_h

#include "mozilla/dom/OscillatorNodeBinding.h"
#include <nsAutoPtr.h>
#include <nsTArray.h>
#include "AlignedTArray.h"
#include "mozilla/MemoryReporting.h"

namespace WebCore {

typedef AlignedTArray<float> AlignedAudioFloatArray;
typedef nsTArray<float> AudioFloatArray;

class PeriodicWave {
public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(WebCore::PeriodicWave);

    static already_AddRefed<PeriodicWave> createSine(float sampleRate);
    static already_AddRefed<PeriodicWave> createSquare(float sampleRate);
    static already_AddRefed<PeriodicWave> createSawtooth(float sampleRate);
    static already_AddRefed<PeriodicWave> createTriangle(float sampleRate);

    
    
    static already_AddRefed<PeriodicWave> create(float sampleRate,
                                                 const float* real,
                                                 const float* imag,
                                                 size_t numberOfComponents);

    
    
    
    
    
    
    
    
    void waveDataForFundamentalFrequency(float, float* &lowerWaveData, float* &higherWaveData, float& tableInterpolationFactor);

    
    
    float rateScale() const { return m_rateScale; }

    unsigned periodicWaveSize() const { return m_periodicWaveSize; }
    float sampleRate() const { return m_sampleRate; }

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
    explicit PeriodicWave(float sampleRate);
    ~PeriodicWave() {}

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
    nsTArray<nsAutoPtr<AlignedAudioFloatArray> > m_bandLimitedTables;
};

} 

#endif 
