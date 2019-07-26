



























#ifndef DynamicsCompressor_h
#define DynamicsCompressor_h

#include "DynamicsCompressorKernel.h"
#include "ZeroPole.h"

#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {
struct AudioChunk;
}

namespace WebCore {

using mozilla::AudioChunk;






class DynamicsCompressor {
public:
    enum {
        ParamThreshold,
        ParamKnee,
        ParamRatio,
        ParamAttack,
        ParamRelease,
        ParamPreDelay,
        ParamReleaseZone1,
        ParamReleaseZone2,
        ParamReleaseZone3,
        ParamReleaseZone4,
        ParamPostGain,
        ParamFilterStageGain,
        ParamFilterStageRatio,
        ParamFilterAnchor,
        ParamEffectBlend,
        ParamReduction,
        ParamLast
    };

    DynamicsCompressor(float sampleRate, unsigned numberOfChannels);

    void process(const AudioChunk* sourceChunk, AudioChunk* destinationChunk, unsigned framesToProcess);
    void reset();
    void setNumberOfChannels(unsigned);
    unsigned numberOfChannels() const { return m_numberOfChannels; }

    void setParameterValue(unsigned parameterID, float value);
    float parameterValue(unsigned parameterID);

    float sampleRate() const { return m_sampleRate; }
    float nyquist() const { return m_sampleRate / 2; }

    double tailTime() const { return 0; }
    double latencyTime() const { return m_compressor.latencyFrames() / static_cast<double>(sampleRate()); }

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

protected:
    unsigned m_numberOfChannels;

    
    float m_parameters[ParamLast];
    void initializeParameters();

    float m_sampleRate;

    
    float m_lastFilterStageRatio;
    float m_lastAnchor;
    float m_lastFilterStageGain;

    typedef struct {
        ZeroPole filters[4];
        size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
        {
            return aMallocSizeOf(this);
        }
    } ZeroPoleFilterPack4;

    
    nsTArray<nsAutoPtr<ZeroPoleFilterPack4> > m_preFilterPacks;
    nsTArray<nsAutoPtr<ZeroPoleFilterPack4> > m_postFilterPacks;

    nsAutoArrayPtr<const float*> m_sourceChannels;
    nsAutoArrayPtr<float*> m_destinationChannels;

    void setEmphasisStageParameters(unsigned stageIndex, float gain, float normalizedFrequency );
    void setEmphasisParameters(float gain, float anchorFreq, float filterStageRatio);

    
    DynamicsCompressorKernel m_compressor;
};

} 

#endif
