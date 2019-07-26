



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "DynamicsCompressor.h"

#include "AudioBus.h"
#include "AudioUtilities.h"
#include <wtf/MathExtras.h>

namespace WebCore {

using namespace AudioUtilities;
    
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
    ASSERT(parameterID < ParamLast);
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
    ASSERT(parameterID < ParamLast);
    return m_parameters[parameterID];
}

void DynamicsCompressor::setEmphasisStageParameters(unsigned stageIndex, float gain, float normalizedFrequency )
{
    float gk = 1 - gain / 20;
    float f1 = normalizedFrequency * gk;
    float f2 = normalizedFrequency / gk;
    float r1 = expf(-f1 * piFloat);
    float r2 = expf(-f2 * piFloat);

    ASSERT(m_numberOfChannels == m_preFilterPacks.size());

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

void DynamicsCompressor::process(const AudioBus* sourceBus, AudioBus* destinationBus, unsigned framesToProcess)
{
    
    
    

    unsigned numberOfChannels = destinationBus->numberOfChannels();
    unsigned numberOfSourceChannels = sourceBus->numberOfChannels();

    ASSERT(numberOfChannels == m_numberOfChannels && numberOfSourceChannels);

    if (numberOfChannels != m_numberOfChannels || !numberOfSourceChannels) {
        destinationBus->zero();
        return;
    }

    switch (numberOfChannels) {
    case 2: 
        m_sourceChannels[0] = sourceBus->channel(0)->data();

        if (numberOfSourceChannels > 1)
            m_sourceChannels[1] = sourceBus->channel(1)->data();
        else
            
            m_sourceChannels[1] = m_sourceChannels[0];

        break;
    default:
        
        ASSERT_NOT_REACHED();
        destinationBus->zero();
        return;
    }

    for (unsigned i = 0; i < numberOfChannels; ++i)
        m_destinationChannels[i] = destinationBus->channel(i)->mutableData();

    float filterStageGain = parameterValue(ParamFilterStageGain);
    float filterStageRatio = parameterValue(ParamFilterStageRatio);
    float anchor = parameterValue(ParamFilterAnchor);

    if (filterStageGain != m_lastFilterStageGain || filterStageRatio != m_lastFilterStageRatio || anchor != m_lastAnchor) {
        m_lastFilterStageGain = filterStageGain;
        m_lastFilterStageRatio = filterStageRatio;
        m_lastAnchor = anchor;

        setEmphasisParameters(filterStageGain, anchor, filterStageRatio);
    }

    
    
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        const float* sourceData = m_sourceChannels[i];
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
    if (m_preFilterPacks.size() == numberOfChannels)
        return;

    m_preFilterPacks.clear();
    m_postFilterPacks.clear();
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        m_preFilterPacks.append(adoptPtr(new ZeroPoleFilterPack4()));
        m_postFilterPacks.append(adoptPtr(new ZeroPoleFilterPack4()));
    }

    m_sourceChannels = adoptArrayPtr(new const float* [numberOfChannels]);
    m_destinationChannels = adoptArrayPtr(new float* [numberOfChannels]);

    m_compressor.setNumberOfChannels(numberOfChannels);
    m_numberOfChannels = numberOfChannels;
}

} 

#endif 
