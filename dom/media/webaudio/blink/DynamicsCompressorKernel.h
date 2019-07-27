



























#ifndef DynamicsCompressorKernel_h
#define DynamicsCompressorKernel_h

#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "mozilla/MemoryReporting.h"

namespace WebCore {

class DynamicsCompressorKernel {
public:
    DynamicsCompressorKernel(float sampleRate, unsigned numberOfChannels);

    void setNumberOfChannels(unsigned);

    
    void process(float* sourceChannels[],
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
                 );

    void reset();

    unsigned latencyFrames() const { return m_lastPreDelayFrames; }

    float sampleRate() const { return m_sampleRate; }

    float meteringGain() const { return m_meteringGain; }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

protected:
    float m_sampleRate;

    float m_detectorAverage;
    float m_compressorGain;

    
    float m_meteringReleaseK;
    float m_meteringGain;

    
    enum { MaxPreDelayFrames = 1024 };
    enum { MaxPreDelayFramesMask = MaxPreDelayFrames - 1 };
    enum { DefaultPreDelayFrames = 256 }; 
    unsigned m_lastPreDelayFrames;
    void setPreDelayTime(float);

    nsTArray<nsAutoArrayPtr<float> > m_preDelayBuffers;
    int m_preDelayReadIndex;
    int m_preDelayWriteIndex;

    float m_maxAttackCompressionDiffDb;

    
    float kneeCurve(float x, float k);
    float saturate(float x, float k);
    float slopeAt(float x, float k);
    float kAtSlope(float desiredSlope);

    float updateStaticCurveParameters(float dbThreshold, float dbKnee, float ratio);

    
    
    float m_ratio;
    float m_slope; 

    
    float m_linearThreshold;
    float m_dbThreshold;

    
    
    
    
    float m_dbKnee;
    float m_kneeThreshold;
    float m_kneeThresholdDb;
    float m_ykneeThresholdDb;

    
    float m_K;
};

} 

#endif 
