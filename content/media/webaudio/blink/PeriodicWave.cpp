



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "modules/webaudio/PeriodicWave.h"

#include "core/platform/audio/FFTFrame.h"
#include "core/platform/audio/VectorMath.h"
#include "modules/webaudio/OscillatorNode.h"
#include "wtf/OwnPtr.h"
#include <algorithm>

const unsigned PeriodicWaveSize = 4096; 
const unsigned NumberOfRanges = 36; 
const float CentsPerRange = 1200 / 3; 

namespace WebCore {

using namespace VectorMath;

PassRefPtr<PeriodicWave> PeriodicWave::create(float sampleRate, Float32Array* real, Float32Array* imag)
{
    bool isGood = real && imag && real->length() == imag->length();
    ASSERT(isGood);
    if (isGood) {
        RefPtr<PeriodicWave> periodicWave = adoptRef(new PeriodicWave(sampleRate));
        size_t numberOfComponents = real->length();
        periodicWave->createBandLimitedTables(real->data(), imag->data(), numberOfComponents);
        return periodicWave;
    }
    return 0;
}

PassRefPtr<PeriodicWave> PeriodicWave::createSine(float sampleRate)
{
    RefPtr<PeriodicWave> periodicWave = adoptRef(new PeriodicWave(sampleRate));
    periodicWave->generateBasicWaveform(OscillatorNode::SINE);
    return periodicWave;
}

PassRefPtr<PeriodicWave> PeriodicWave::createSquare(float sampleRate)
{
    RefPtr<PeriodicWave> periodicWave = adoptRef(new PeriodicWave(sampleRate));
    periodicWave->generateBasicWaveform(OscillatorNode::SQUARE);
    return periodicWave;
}

PassRefPtr<PeriodicWave> PeriodicWave::createSawtooth(float sampleRate)
{
    RefPtr<PeriodicWave> periodicWave = adoptRef(new PeriodicWave(sampleRate));
    periodicWave->generateBasicWaveform(OscillatorNode::SAWTOOTH);
    return periodicWave;
}

PassRefPtr<PeriodicWave> PeriodicWave::createTriangle(float sampleRate)
{
    RefPtr<PeriodicWave> periodicWave = adoptRef(new PeriodicWave(sampleRate));
    periodicWave->generateBasicWaveform(OscillatorNode::TRIANGLE);
    return periodicWave;
}

PeriodicWave::PeriodicWave(float sampleRate)
    : m_sampleRate(sampleRate)
    , m_periodicWaveSize(PeriodicWaveSize)
    , m_numberOfRanges(NumberOfRanges)
    , m_centsPerRange(CentsPerRange)
{
    ScriptWrappable::init(this);
    float nyquist = 0.5 * m_sampleRate;
    m_lowestFundamentalFrequency = nyquist / maxNumberOfPartials();
    m_rateScale = m_periodicWaveSize / m_sampleRate;
}

void PeriodicWave::waveDataForFundamentalFrequency(float fundamentalFrequency, float* &lowerWaveData, float* &higherWaveData, float& tableInterpolationFactor)
{
    
    fundamentalFrequency = fabsf(fundamentalFrequency);

    
    float ratio = fundamentalFrequency > 0 ? fundamentalFrequency / m_lowestFundamentalFrequency : 0.5;
    float centsAboveLowestFrequency = log2f(ratio) * 1200;

    
    float pitchRange = 1 + centsAboveLowestFrequency / m_centsPerRange;

    pitchRange = std::max(pitchRange, 0.0f);
    pitchRange = std::min(pitchRange, static_cast<float>(m_numberOfRanges - 1));

    
    
    
    unsigned rangeIndex1 = static_cast<unsigned>(pitchRange);
    unsigned rangeIndex2 = rangeIndex1 < m_numberOfRanges - 1 ? rangeIndex1 + 1 : rangeIndex1;

    lowerWaveData = m_bandLimitedTables[rangeIndex2]->data();
    higherWaveData = m_bandLimitedTables[rangeIndex1]->data();

    
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
    unsigned halfSize = fftSize / 2;
    unsigned i;

    numberOfComponents = std::min(numberOfComponents, halfSize);

    m_bandLimitedTables.reserveCapacity(m_numberOfRanges);

    for (unsigned rangeIndex = 0; rangeIndex < m_numberOfRanges; ++rangeIndex) {
        
        FFTFrame frame(fftSize);
        float* realP = frame.realData();
        float* imagP = frame.imagData();

        
        float scale = fftSize;
        vsmul(realData, 1, &scale, realP, 1, numberOfComponents);
        vsmul(imagData, 1, &scale, imagP, 1, numberOfComponents);

        
        for (i = numberOfComponents; i < halfSize; ++i) {
            realP[i] = 0;
            imagP[i] = 0;
        }

        
        float minusOne = -1;
        vsmul(imagP, 1, &minusOne, imagP, 1, halfSize);

        
        
        unsigned numberOfPartials = numberOfPartialsForRange(rangeIndex);

        
        for (i = numberOfPartials + 1; i < halfSize; ++i) {
            realP[i] = 0;
            imagP[i] = 0;
        }
        
        if (numberOfPartials < halfSize)
            imagP[0] = 0;

        
        realP[0] = 0;

        
        OwnPtr<AudioFloatArray> table = adoptPtr(new AudioFloatArray(m_periodicWaveSize));
        m_bandLimitedTables.append(table.release());

        
        float* data = m_bandLimitedTables[rangeIndex]->data();
        frame.doInverseFFT(data);

        
        if (!rangeIndex) {
            float maxValue;
            vmaxmgv(data, 1, &maxValue, m_periodicWaveSize);

            if (maxValue)
                normalizationScale = 1.0f / maxValue;
        }

        
        vsmul(data, 1, &normalizationScale, data, 1, m_periodicWaveSize);
    }
}

void PeriodicWave::generateBasicWaveform(int shape)
{
    unsigned fftSize = periodicWaveSize();
    unsigned halfSize = fftSize / 2;

    AudioFloatArray real(halfSize);
    AudioFloatArray imag(halfSize);
    float* realP = real.data();
    float* imagP = imag.data();

    
    realP[0] = 0;
    imagP[0] = 0;

    for (unsigned n = 1; n < halfSize; ++n) {
        float omega = 2 * piFloat * n;
        float invOmega = 1 / omega;

        
        float a; 
        float b; 

        
        
        switch (shape) {
        case OscillatorNode::SINE:
            
            a = 0;
            b = (n == 1) ? 1 : 0;
            break;
        case OscillatorNode::SQUARE:
            
            a = 0;
            b = invOmega * ((n & 1) ? 2 : 0);
            break;
        case OscillatorNode::SAWTOOTH:
            
            a = 0;
            b = -invOmega * cos(0.5 * omega);
            break;
        case OscillatorNode::TRIANGLE:
            
            a = (4 - 4 * cos(0.5 * omega)) / (n * n * piFloat * piFloat);
            b = 0;
            break;
        default:
            ASSERT_NOT_REACHED();
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

#endif 
