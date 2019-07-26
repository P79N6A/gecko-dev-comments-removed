



























#include "DynamicsCompressorKernel.h"

#include "DenormalDisabler.h"
#include <algorithm>

#include "mozilla/FloatingPoint.h"
#include "mozilla/Constants.h"
#include "WebAudioUtils.h"

using namespace std;

using namespace mozilla::dom; 
using mozilla::IsInfinite;
using mozilla::IsNaN;

namespace WebCore {



const float meteringReleaseTimeConstant = 0.325f;

const float uninitializedValue = -1;

DynamicsCompressorKernel::DynamicsCompressorKernel(float sampleRate, unsigned numberOfChannels)
    : m_sampleRate(sampleRate)
    , m_lastPreDelayFrames(DefaultPreDelayFrames)
    , m_preDelayReadIndex(0)
    , m_preDelayWriteIndex(DefaultPreDelayFrames)
    , m_ratio(uninitializedValue)
    , m_slope(uninitializedValue)
    , m_linearThreshold(uninitializedValue)
    , m_dbThreshold(uninitializedValue)
    , m_dbKnee(uninitializedValue)
    , m_kneeThreshold(uninitializedValue)
    , m_kneeThresholdDb(uninitializedValue)
    , m_ykneeThresholdDb(uninitializedValue)
    , m_K(uninitializedValue)
{
    setNumberOfChannels(numberOfChannels);

    
    reset();

    m_meteringReleaseK =
        static_cast<float>(WebAudioUtils::DiscreteTimeConstantForSampleRate(meteringReleaseTimeConstant, sampleRate));
}

size_t DynamicsCompressorKernel::sizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t amount = 0;
    amount += m_preDelayBuffers.SizeOfExcludingThis(aMallocSizeOf);
    for (size_t i = 0; i < m_preDelayBuffers.Length(); i++) {
        amount += m_preDelayBuffers[i].SizeOfExcludingThis(aMallocSizeOf);
    }

    return amount;
}

void DynamicsCompressorKernel::setNumberOfChannels(unsigned numberOfChannels)
{
    if (m_preDelayBuffers.Length() == numberOfChannels)
        return;

    m_preDelayBuffers.Clear();
    for (unsigned i = 0; i < numberOfChannels; ++i)
        m_preDelayBuffers.AppendElement(new float[MaxPreDelayFrames]);
}

void DynamicsCompressorKernel::setPreDelayTime(float preDelayTime)
{
    
    unsigned preDelayFrames = preDelayTime * sampleRate();
    if (preDelayFrames > MaxPreDelayFrames - 1)
        preDelayFrames = MaxPreDelayFrames - 1;

    if (m_lastPreDelayFrames != preDelayFrames) {
        m_lastPreDelayFrames = preDelayFrames;
        for (unsigned i = 0; i < m_preDelayBuffers.Length(); ++i)
            memset(m_preDelayBuffers[i], 0, sizeof(float) * MaxPreDelayFrames);

        m_preDelayReadIndex = 0;
        m_preDelayWriteIndex = preDelayFrames;
    }
}



float DynamicsCompressorKernel::kneeCurve(float x, float k)
{
    
    if (x < m_linearThreshold)
        return x;

    return m_linearThreshold + (1 - expf(-k * (x - m_linearThreshold))) / k;
}


float DynamicsCompressorKernel::saturate(float x, float k)
{
    float y;

    if (x < m_kneeThreshold)
        y = kneeCurve(x, k);
    else {
        
        float xDb = WebAudioUtils::ConvertLinearToDecibels(x, -1000.0f);
        float yDb = m_ykneeThresholdDb + m_slope * (xDb - m_kneeThresholdDb);

        y = WebAudioUtils::ConvertDecibelsToLinear(yDb);
    }

    return y;
}




float DynamicsCompressorKernel::slopeAt(float x, float k)
{
    if (x < m_linearThreshold)
        return 1;

    float x2 = x * 1.001;

    float xDb = WebAudioUtils::ConvertLinearToDecibels(x, -1000.0f);
    float x2Db = WebAudioUtils::ConvertLinearToDecibels(x2, -1000.0f);

    float yDb = WebAudioUtils::ConvertLinearToDecibels(kneeCurve(x, k), -1000.0f);
    float y2Db = WebAudioUtils::ConvertLinearToDecibels(kneeCurve(x2, k), -1000.0f);

    float m = (y2Db - yDb) / (x2Db - xDb);

    return m;
}

float DynamicsCompressorKernel::kAtSlope(float desiredSlope)
{
    float xDb = m_dbThreshold + m_dbKnee;
    float x = WebAudioUtils::ConvertDecibelsToLinear(xDb);

    
    float minK = 0.1;
    float maxK = 10000;
    float k = 5;

    for (int i = 0; i < 15; ++i) {
        
        float slope = slopeAt(x, k);

        if (slope < desiredSlope) {
            
            maxK = k;
        } else {
            
            minK = k;
        }

        
        k = sqrtf(minK * maxK);
    }

    return k;
}

float DynamicsCompressorKernel::updateStaticCurveParameters(float dbThreshold, float dbKnee, float ratio)
{
    if (dbThreshold != m_dbThreshold || dbKnee != m_dbKnee || ratio != m_ratio) {
        
        m_dbThreshold = dbThreshold;
        m_linearThreshold = WebAudioUtils::ConvertDecibelsToLinear(dbThreshold);
        m_dbKnee = dbKnee;

        
        m_ratio = ratio;
        m_slope = 1 / m_ratio;

        float k = kAtSlope(1 / m_ratio);

        m_kneeThresholdDb = dbThreshold + dbKnee;
        m_kneeThreshold = WebAudioUtils::ConvertDecibelsToLinear(m_kneeThresholdDb);

        m_ykneeThresholdDb = WebAudioUtils::ConvertLinearToDecibels(kneeCurve(m_kneeThreshold, k), -1000.0f);

        m_K = k;
    }
    return m_K;
}

void DynamicsCompressorKernel::process(float* sourceChannels[],
                                       float* destinationChannels[],
                                       unsigned numberOfChannels,
                                       unsigned framesToProcess,

                                       float dbThreshold,
                                       float dbKnee,
                                       float ratio,
                                       float attackTime,
                                       float releaseTime,
                                       float preDelayTime,
                                       float dbPostGain,
                                       float effectBlend, 

                                       float releaseZone1,
                                       float releaseZone2,
                                       float releaseZone3,
                                       float releaseZone4
                                       )
{
    MOZ_ASSERT(m_preDelayBuffers.Length() == numberOfChannels);

    float sampleRate = this->sampleRate();

    float dryMix = 1 - effectBlend;
    float wetMix = effectBlend;

    float k = updateStaticCurveParameters(dbThreshold, dbKnee, ratio);

    
    float fullRangeGain = saturate(1, k);
    float fullRangeMakeupGain = 1 / fullRangeGain;

    
    fullRangeMakeupGain = powf(fullRangeMakeupGain, 0.6f);

    float masterLinearGain = WebAudioUtils::ConvertDecibelsToLinear(dbPostGain) * fullRangeMakeupGain;

    
    attackTime = max(0.001f, attackTime);
    float attackFrames = attackTime * sampleRate;

    
    float releaseFrames = sampleRate * releaseTime;

    
    float satReleaseTime = 0.0025f;
    float satReleaseFrames = satReleaseTime * sampleRate;

    

    
    

    float y1 = releaseFrames * releaseZone1;
    float y2 = releaseFrames * releaseZone2;
    float y3 = releaseFrames * releaseZone3;
    float y4 = releaseFrames * releaseZone4;

    
    
    float kA = 0.9999999999999998f*y1 + 1.8432219684323923e-16f*y2 - 1.9373394351676423e-16f*y3 + 8.824516011816245e-18f*y4;
    float kB = -1.5788320352845888f*y1 + 2.3305837032074286f*y2 - 0.9141194204840429f*y3 + 0.1623677525612032f*y4;
    float kC = 0.5334142869106424f*y1 - 1.272736789213631f*y2 + 0.9258856042207512f*y3 - 0.18656310191776226f*y4;
    float kD = 0.08783463138207234f*y1 - 0.1694162967925622f*y2 + 0.08588057951595272f*y3 - 0.00429891410546283f*y4;
    float kE = -0.042416883008123074f*y1 + 0.1115693827987602f*y2 - 0.09764676325265872f*y3 + 0.028494263462021576f*y4;

    
    

    

    setPreDelayTime(preDelayTime);

    const int nDivisionFrames = 32;

    const int nDivisions = framesToProcess / nDivisionFrames;

    unsigned frameIndex = 0;
    for (int i = 0; i < nDivisions; ++i) {
        
        
        

        
        if (IsNaN(m_detectorAverage))
            m_detectorAverage = 1;
        if (IsInfinite(m_detectorAverage))
            m_detectorAverage = 1;

        float desiredGain = m_detectorAverage;

        
        float scaledDesiredGain = asinf(desiredGain) / (0.5f * M_PI);

        
        
        

        
        
        float envelopeRate;

        bool isReleasing = scaledDesiredGain > m_compressorGain;

        
        float compressionDiffDb = WebAudioUtils::ConvertLinearToDecibels(m_compressorGain / scaledDesiredGain, -1000.0f);

        if (isReleasing) {
            
            m_maxAttackCompressionDiffDb = -1;

            
            if (IsNaN(compressionDiffDb))
                compressionDiffDb = -1;
            if (IsInfinite(compressionDiffDb))
                compressionDiffDb = -1;

            

            
            float x = compressionDiffDb;
            x = max(-12.0f, x);
            x = min(0.0f, x);
            x = 0.25f * (x + 12);

            
            
            float x2 = x * x;
            float x3 = x2 * x;
            float x4 = x2 * x2;
            float releaseFrames = kA + kB * x + kC * x2 + kD * x3 + kE * x4;

#define kSpacingDb 5
            float dbPerFrame = kSpacingDb / releaseFrames;

            envelopeRate = WebAudioUtils::ConvertDecibelsToLinear(dbPerFrame);
        } else {
            

            
            if (IsNaN(compressionDiffDb))
                compressionDiffDb = 1;
            if (IsInfinite(compressionDiffDb))
                compressionDiffDb = 1;

            
            
            if (m_maxAttackCompressionDiffDb == -1 || m_maxAttackCompressionDiffDb < compressionDiffDb)
                m_maxAttackCompressionDiffDb = compressionDiffDb;

            float effAttenDiffDb = max(0.5f, m_maxAttackCompressionDiffDb);

            float x = 0.25f / effAttenDiffDb;
            envelopeRate = 1 - powf(x, 1 / attackFrames);
        }

        
        
        

        {
            int preDelayReadIndex = m_preDelayReadIndex;
            int preDelayWriteIndex = m_preDelayWriteIndex;
            float detectorAverage = m_detectorAverage;
            float compressorGain = m_compressorGain;

            int loopFrames = nDivisionFrames;
            while (loopFrames--) {
                float compressorInput = 0;

                
                for (unsigned i = 0; i < numberOfChannels; ++i) {
                    float* delayBuffer = m_preDelayBuffers[i];
                    float undelayedSource = sourceChannels[i][frameIndex];
                    delayBuffer[preDelayWriteIndex] = undelayedSource;

                    float absUndelayedSource = undelayedSource > 0 ? undelayedSource : -undelayedSource;
                    if (compressorInput < absUndelayedSource)
                        compressorInput = absUndelayedSource;
                }

                

                float scaledInput = compressorInput;
                float absInput = scaledInput > 0 ? scaledInput : -scaledInput;

                
                
                
                
                float shapedInput = saturate(absInput, k);

                float attenuation = absInput <= 0.0001f ? 1 : shapedInput / absInput;

                float attenuationDb = -WebAudioUtils::ConvertLinearToDecibels(attenuation, -1000.0f);
                attenuationDb = max(2.0f, attenuationDb);

                float dbPerFrame = attenuationDb / satReleaseFrames;

                float satReleaseRate = WebAudioUtils::ConvertDecibelsToLinear(dbPerFrame) - 1;

                bool isRelease = (attenuation > detectorAverage);
                float rate = isRelease ? satReleaseRate : 1;

                detectorAverage += (attenuation - detectorAverage) * rate;
                detectorAverage = min(1.0f, detectorAverage);

                
                if (IsNaN(detectorAverage))
                    detectorAverage = 1;
                if (IsInfinite(detectorAverage))
                    detectorAverage = 1;

                
                if (envelopeRate < 1) {
                    
                    compressorGain += (scaledDesiredGain - compressorGain) * envelopeRate;
                } else {
                    
                    compressorGain *= envelopeRate;
                    compressorGain = min(1.0f, compressorGain);
                }

                
                float postWarpCompressorGain = sinf(0.5f * M_PI * compressorGain);

                
                float totalGain = dryMix + wetMix * masterLinearGain * postWarpCompressorGain;

                
                float dbRealGain = 20 * log10(postWarpCompressorGain);
                if (dbRealGain < m_meteringGain)
                    m_meteringGain = dbRealGain;
                else
                    m_meteringGain += (dbRealGain - m_meteringGain) * m_meteringReleaseK;

                
                for (unsigned i = 0; i < numberOfChannels; ++i) {
                    float* delayBuffer = m_preDelayBuffers[i];
                    destinationChannels[i][frameIndex] = delayBuffer[preDelayReadIndex] * totalGain;
                }

                frameIndex++;
                preDelayReadIndex = (preDelayReadIndex + 1) & MaxPreDelayFramesMask;
                preDelayWriteIndex = (preDelayWriteIndex + 1) & MaxPreDelayFramesMask;
            }

            
            m_preDelayReadIndex = preDelayReadIndex;
            m_preDelayWriteIndex = preDelayWriteIndex;
            m_detectorAverage = DenormalDisabler::flushDenormalFloatToZero(detectorAverage);
            m_compressorGain = DenormalDisabler::flushDenormalFloatToZero(compressorGain);
        }
    }
}

void DynamicsCompressorKernel::reset()
{
    m_detectorAverage = 0;
    m_compressorGain = 1;
    m_meteringGain = 1;

    
    for (unsigned i = 0; i < m_preDelayBuffers.Length(); ++i)
        memset(m_preDelayBuffers[i], 0, sizeof(float) * MaxPreDelayFrames);

    m_preDelayReadIndex = 0;
    m_preDelayWriteIndex = DefaultPreDelayFrames;

    m_maxAttackCompressionDiffDb = -1; 
}

} 
