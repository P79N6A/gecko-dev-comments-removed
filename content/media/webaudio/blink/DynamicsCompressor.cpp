



























#include "DynamicsCompressor.h"
#include "AudioSegment.h"

#include <cmath>
#include "AudioNodeEngine.h"
#include "nsDebug.h"

using mozilla::WEBAUDIO_BLOCK_SIZE;
using mozilla::AudioBlockCopyChannelWithScale;

namespace WebCore {

DynamicsCompressor::DynamicsCompressor(float sampleRate, unsigned numberOfChannels)
    : m_numberOfChannels(numberOfChannels)
    , m_sampleRate(sampleRate)
    , m_compressor(sampleRate, numberOfChannels)
{
    
    m_lastFilterStageRatio = -1;
    m_lastAnchor = -1;
    m_lastFilterStageGain = -1;

    setNumberOfChannels(numberOfChannels);
    initializeParameters();
}

void DynamicsCompressor::setParameterValue(unsigned parameterID, float value)
{
    MOZ_ASSERT(parameterID < ParamLast);
    if (parameterID < ParamLast)
        m_parameters[parameterID] = value;
}

void DynamicsCompressor::initializeParameters()
{
    
    
    m_parameters[ParamThreshold] = -24; 
    m_parameters[ParamKnee] = 30; 
    m_parameters[ParamRatio] = 12; 
    m_parameters[ParamAttack] = 0.003f; 
    m_parameters[ParamRelease] = 0.250f; 
    m_parameters[ParamPreDelay] = 0.006f; 

    
    m_parameters[ParamReleaseZone1] = 0.09f;
    m_parameters[ParamReleaseZone2] = 0.16f;
    m_parameters[ParamReleaseZone3] = 0.42f;
    m_parameters[ParamReleaseZone4] = 0.98f;

    m_parameters[ParamFilterStageGain] = 4.4f; 
    m_parameters[ParamFilterStageRatio] = 2;
    m_parameters[ParamFilterAnchor] = 15000 / nyquist();
    
    m_parameters[ParamPostGain] = 0; 
    m_parameters[ParamReduction] = 0; 

    
    m_parameters[ParamEffectBlend] = 1;
}

float DynamicsCompressor::parameterValue(unsigned parameterID)
{
    MOZ_ASSERT(parameterID < ParamLast);
    return m_parameters[parameterID];
}

void DynamicsCompressor::setEmphasisStageParameters(unsigned stageIndex, float gain, float normalizedFrequency )
{
    float gk = 1 - gain / 20;
    float f1 = normalizedFrequency * gk;
    float f2 = normalizedFrequency / gk;
    float r1 = expf(-f1 * M_PI);
    float r2 = expf(-f2 * M_PI);

    MOZ_ASSERT(m_numberOfChannels == m_preFilterPacks.Length());

    for (unsigned i = 0; i < m_numberOfChannels; ++i) {
        
        ZeroPole& preFilter = m_preFilterPacks[i]->filters[stageIndex];
        preFilter.setZero(r1);
        preFilter.setPole(r2);

        
        
        ZeroPole& postFilter = m_postFilterPacks[i]->filters[stageIndex];
        postFilter.setZero(r2);
        postFilter.setPole(r1);
    }
}

void DynamicsCompressor::setEmphasisParameters(float gain, float anchorFreq, float filterStageRatio)
{
    setEmphasisStageParameters(0, gain, anchorFreq);
    setEmphasisStageParameters(1, gain, anchorFreq / filterStageRatio);
    setEmphasisStageParameters(2, gain, anchorFreq / (filterStageRatio * filterStageRatio));
    setEmphasisStageParameters(3, gain, anchorFreq / (filterStageRatio * filterStageRatio * filterStageRatio));
}

void DynamicsCompressor::process(const AudioChunk* sourceChunk, AudioChunk* destinationChunk, unsigned framesToProcess)
{
    
    
    

    unsigned numberOfChannels = destinationChunk->mChannelData.Length();
    unsigned numberOfSourceChannels = sourceChunk->mChannelData.Length();

    MOZ_ASSERT(numberOfChannels == m_numberOfChannels && numberOfSourceChannels);

    if (numberOfChannels != m_numberOfChannels || !numberOfSourceChannels) {
        destinationChunk->SetNull(WEBAUDIO_BLOCK_SIZE);
        return;
    }

    switch (numberOfChannels) {
    case 2: 
        m_sourceChannels[0] = static_cast<const float*>(sourceChunk->mChannelData[0]);

        if (numberOfSourceChannels > 1)
            m_sourceChannels[1] = static_cast<const float*>(sourceChunk->mChannelData[1]);
        else
            
            m_sourceChannels[1] = m_sourceChannels[0];

        break;
    default:
        
        NS_WARNING("Support other number of channels");
        destinationChunk->SetNull(WEBAUDIO_BLOCK_SIZE);
        return;
    }

    for (unsigned i = 0; i < numberOfChannels; ++i)
        m_destinationChannels[i] = const_cast<float*>(static_cast<const float*>(
            destinationChunk->mChannelData[i]));

    float filterStageGain = parameterValue(ParamFilterStageGain);
    float filterStageRatio = parameterValue(ParamFilterStageRatio);
    float anchor = parameterValue(ParamFilterAnchor);

    if (filterStageGain != m_lastFilterStageGain || filterStageRatio != m_lastFilterStageRatio || anchor != m_lastAnchor) {
        m_lastFilterStageGain = filterStageGain;
        m_lastFilterStageRatio = filterStageRatio;
        m_lastAnchor = anchor;

        setEmphasisParameters(filterStageGain, anchor, filterStageRatio);
    }

    float sourceWithVolume[WEBAUDIO_BLOCK_SIZE];

    
    
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        const float* sourceData;
        if (sourceChunk->mVolume == 1.0f) {
          
          sourceData = m_sourceChannels[i];
        } else {
          AudioBlockCopyChannelWithScale(m_sourceChannels[i],
                                         sourceChunk->mVolume,
                                         sourceWithVolume);
          sourceData = sourceWithVolume;
        }

        float* destinationData = m_destinationChannels[i];
        ZeroPole* preFilters = m_preFilterPacks[i]->filters;

        preFilters[0].process(sourceData, destinationData, framesToProcess);
        preFilters[1].process(destinationData, destinationData, framesToProcess);
        preFilters[2].process(destinationData, destinationData, framesToProcess);
        preFilters[3].process(destinationData, destinationData, framesToProcess);
    }

    float dbThreshold = parameterValue(ParamThreshold);
    float dbKnee = parameterValue(ParamKnee);
    float ratio = parameterValue(ParamRatio);
    float attackTime = parameterValue(ParamAttack);
    float releaseTime = parameterValue(ParamRelease);
    float preDelayTime = parameterValue(ParamPreDelay);

    
    float dbPostGain = parameterValue(ParamPostGain);

    
    
    
    float effectBlend = parameterValue(ParamEffectBlend);

    float releaseZone1 = parameterValue(ParamReleaseZone1);
    float releaseZone2 = parameterValue(ParamReleaseZone2);
    float releaseZone3 = parameterValue(ParamReleaseZone3);
    float releaseZone4 = parameterValue(ParamReleaseZone4);

    
    
    m_compressor.process(m_destinationChannels.get(),
                         m_destinationChannels.get(),
                         numberOfChannels,
                         framesToProcess,

                         dbThreshold,
                         dbKnee,
                         ratio,
                         attackTime,
                         releaseTime,
                         preDelayTime,
                         dbPostGain,
                         effectBlend,

                         releaseZone1,
                         releaseZone2,
                         releaseZone3,
                         releaseZone4
                         );
                         
    
    setParameterValue(ParamReduction, m_compressor.meteringGain());

    
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        float* destinationData = m_destinationChannels[i];
        ZeroPole* postFilters = m_postFilterPacks[i]->filters;

        postFilters[0].process(destinationData, destinationData, framesToProcess);
        postFilters[1].process(destinationData, destinationData, framesToProcess);
        postFilters[2].process(destinationData, destinationData, framesToProcess);
        postFilters[3].process(destinationData, destinationData, framesToProcess);
    }
}

void DynamicsCompressor::reset()
{
    m_lastFilterStageRatio = -1; 
    m_lastAnchor = -1;
    m_lastFilterStageGain = -1;

    for (unsigned channel = 0; channel < m_numberOfChannels; ++channel) {
        for (unsigned stageIndex = 0; stageIndex < 4; ++stageIndex) {
            m_preFilterPacks[channel]->filters[stageIndex].reset();
            m_postFilterPacks[channel]->filters[stageIndex].reset();
        }
    }

    m_compressor.reset();
}

void DynamicsCompressor::setNumberOfChannels(unsigned numberOfChannels)
{
    if (m_preFilterPacks.Length() == numberOfChannels)
        return;

    m_preFilterPacks.Clear();
    m_postFilterPacks.Clear();
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        m_preFilterPacks.AppendElement(new ZeroPoleFilterPack4());
        m_postFilterPacks.AppendElement(new ZeroPoleFilterPack4());
    }

    m_sourceChannels = new const float* [numberOfChannels];
    m_destinationChannels = new float* [numberOfChannels];

    m_compressor.setNumberOfChannels(numberOfChannels);
    m_numberOfChannels = numberOfChannels;
}

} 
