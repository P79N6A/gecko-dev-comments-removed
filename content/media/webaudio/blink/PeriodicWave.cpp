



























#include "PeriodicWave.h"
#include <algorithm>
#include <cmath>
#include "mozilla/FFTBlock.h"

const unsigned PeriodicWaveSize = 4096; 
const unsigned NumberOfRanges = 36; 
const float CentsPerRange = 1200 / 3; 

using namespace mozilla;
using mozilla::dom::OscillatorType;

namespace WebCore {

PeriodicWave* PeriodicWave::create(float sampleRate,
                                   const float* real,
                                   const float* imag,
                                   size_t numberOfComponents)
{
    bool isGood = real && imag && numberOfComponents > 0 &&
         numberOfComponents <= PeriodicWaveSize;
    MOZ_ASSERT(isGood);
    if (isGood) {
        PeriodicWave* periodicWave = new PeriodicWave(sampleRate);
        periodicWave->createBandLimitedTables(real, imag, numberOfComponents);
        return periodicWave;
    }
    return 0;
}

PeriodicWave* PeriodicWave::createSine(float sampleRate)
{
      PeriodicWave* periodicWave = new PeriodicWave(sampleRate);
          periodicWave->generateBasicWaveform(OscillatorType::Sine);
              return periodicWave;
}

PeriodicWave* PeriodicWave::createSquare(float sampleRate)
{
      PeriodicWave* periodicWave = new PeriodicWave(sampleRate);
          periodicWave->generateBasicWaveform(OscillatorType::Square);
              return periodicWave;
}

PeriodicWave* PeriodicWave::createSawtooth(float sampleRate)
{
      PeriodicWave* periodicWave = new PeriodicWave(sampleRate);
          periodicWave->generateBasicWaveform(OscillatorType::Sawtooth);
              return periodicWave;
}

PeriodicWave* PeriodicWave::createTriangle(float sampleRate)
{
      PeriodicWave* periodicWave = new PeriodicWave(sampleRate);
          periodicWave->generateBasicWaveform(OscillatorType::Triangle);
              return periodicWave;
}

PeriodicWave::PeriodicWave(float sampleRate)
    : m_sampleRate(sampleRate)
    , m_periodicWaveSize(PeriodicWaveSize)
    , m_numberOfRanges(NumberOfRanges)
    , m_centsPerRange(CentsPerRange)
{
    float nyquist = 0.5 * m_sampleRate;
    m_lowestFundamentalFrequency = nyquist / maxNumberOfPartials();
    m_rateScale = m_periodicWaveSize / m_sampleRate;
}

void PeriodicWave::waveDataForFundamentalFrequency(float fundamentalFrequency, float* &lowerWaveData, float* &higherWaveData, float& tableInterpolationFactor)
{
    
    
    fundamentalFrequency = fabsf(fundamentalFrequency);

    
    float ratio = fundamentalFrequency > 0 ? fundamentalFrequency / m_lowestFundamentalFrequency : 0.5;
    float centsAboveLowestFrequency = logf(ratio)/logf(2.0f) * 1200;

    
    
    float pitchRange = 1 + centsAboveLowestFrequency / m_centsPerRange;

    pitchRange = std::max(pitchRange, 0.0f);
    pitchRange = std::min(pitchRange, static_cast<float>(m_numberOfRanges - 1));

    
    
    
    
    unsigned rangeIndex1 = static_cast<unsigned>(pitchRange);
    unsigned rangeIndex2 = rangeIndex1 < m_numberOfRanges - 1 ? rangeIndex1 + 1 : rangeIndex1;

    lowerWaveData = m_bandLimitedTables[rangeIndex2]->Elements();
    higherWaveData = m_bandLimitedTables[rangeIndex1]->Elements();

    
    tableInterpolationFactor = pitchRange - rangeIndex1;
}

unsigned PeriodicWave::maxNumberOfPartials() const
{
    return m_periodicWaveSize / 2;
}

unsigned PeriodicWave::numberOfPartialsForRange(unsigned rangeIndex) const
{
    
    float centsToCull = rangeIndex * m_centsPerRange;

    
    float cullingScale = pow(2, -centsToCull / 1200);

    
    unsigned numberOfPartials = cullingScale * maxNumberOfPartials();

    return numberOfPartials;
}





void PeriodicWave::createBandLimitedTables(const float* realData, const float* imagData, unsigned numberOfComponents)
{
    float normalizationScale = 1;

    unsigned fftSize = m_periodicWaveSize;
    unsigned halfSize = fftSize / 2 + 1;
    unsigned i;

    numberOfComponents = std::min(numberOfComponents, halfSize);

    m_bandLimitedTables.SetCapacity(m_numberOfRanges);

    for (unsigned rangeIndex = 0; rangeIndex < m_numberOfRanges; ++rangeIndex) {
        
        FFTBlock frame(fftSize);
        float* realP = new float[halfSize];
        float* imagP = new float[halfSize];

        
        float scale = fftSize;
        AudioBufferCopyWithScale(realData, scale, realP, numberOfComponents);
        AudioBufferCopyWithScale(imagData, scale, imagP, numberOfComponents);

        
        
        for (i = numberOfComponents; i < halfSize; ++i) {
            realP[i] = 0;
            imagP[i] = 0;
        }

        
        
        float minusOne = -1;
        AudioBufferInPlaceScale(imagP, 1, minusOne, halfSize);

        
        
        
        unsigned numberOfPartials = numberOfPartialsForRange(rangeIndex);

        
        for (i = numberOfPartials + 1; i < halfSize; ++i) {
            realP[i] = 0;
            imagP[i] = 0;
        }
        
        if (numberOfPartials < halfSize)
            realP[halfSize-1] = 0;

        
        realP[0] = 0;

        
        imagP[0] = 0;
        imagP[halfSize-1] = 0;

        
        AudioFloatArray* table = new AudioFloatArray(m_periodicWaveSize);
        m_bandLimitedTables.AppendElement(table);

        
        float* data = m_bandLimitedTables[rangeIndex]->Elements();
        frame.PerformInverseFFT(realP, imagP, data);

        
        
        if (!rangeIndex) {
            float maxValue;
            maxValue = AudioBufferPeakValue(data, m_periodicWaveSize);

            if (maxValue)
                normalizationScale = 1.0f / maxValue;
        }

        
        AudioBufferInPlaceScale(data, 1, normalizationScale, m_periodicWaveSize);
    }
}

void PeriodicWave::generateBasicWaveform(OscillatorType shape)
{
    const float piFloat = M_PI;
    unsigned fftSize = periodicWaveSize();
    unsigned halfSize = fftSize / 2 + 1;

    AudioFloatArray real(halfSize);
    AudioFloatArray imag(halfSize);
    float* realP = real.Elements();
    float* imagP = imag.Elements();

    
    realP[0] = 0;
    imagP[0] = 0;
    realP[halfSize-1] = 0;
    imagP[halfSize-1] = 0;

    for (unsigned n = 1; n < halfSize; ++n) {
        float omega = 2 * piFloat * n;
        float invOmega = 1 / omega;

        
        float a; 
        float b; 

        
        
        
        switch (shape) {
        case OscillatorType::Sine:
            
            a = 0;
            b = (n == 1) ? 1 : 0;
            break;
        case OscillatorType::Square:
            
            
            a = 0;
            b = invOmega * ((n & 1) ? 2 : 0);
            break;
        case OscillatorType::Sawtooth:
            
            
            a = 0;
            b = -invOmega * cos(0.5 * omega);
            break;
        case OscillatorType::Triangle:
            
            
            a = (4 - 4 * cos(0.5 * omega)) / (n * n * piFloat * piFloat);
            b = 0;
            break;
        default:
            NS_NOTREACHED("invalid oscillator type");
            a = 0;
            b = 0;
            break;
        }

        realP[n] = a;
        imagP[n] = b;
    }

    createBandLimitedTables(realP, imagP, halfSize);
}

} 
