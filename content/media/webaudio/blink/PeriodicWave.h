



























#ifndef PeriodicWave_h
#define PeriodicWave_h

#include "bindings/v8/ScriptWrappable.h"
#include "core/platform/audio/AudioArray.h"
#include "wtf/Float32Array.h"
#include "wtf/OwnPtr.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"

namespace WebCore {

class PeriodicWave : public ScriptWrappable, public RefCounted<PeriodicWave> {
public:
    static PassRefPtr<PeriodicWave> createSine(float sampleRate);
    static PassRefPtr<PeriodicWave> createSquare(float sampleRate);
    static PassRefPtr<PeriodicWave> createSawtooth(float sampleRate);
    static PassRefPtr<PeriodicWave> createTriangle(float sampleRate);

    
    static PassRefPtr<PeriodicWave> create(float sampleRate, Float32Array* real, Float32Array* imag);

    
    
    
    
    
    
    void waveDataForFundamentalFrequency(float, float* &lowerWaveData, float* &higherWaveData, float& tableInterpolationFactor);

    
    float rateScale() const { return m_rateScale; }

    unsigned periodicWaveSize() const { return m_periodicWaveSize; }
    float sampleRate() const { return m_sampleRate; }

private:
    explicit PeriodicWave(float sampleRate);

    void generateBasicWaveform(int);

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
    Vector<OwnPtr<AudioFloatArray> > m_bandLimitedTables;
};

} 

#endif 
