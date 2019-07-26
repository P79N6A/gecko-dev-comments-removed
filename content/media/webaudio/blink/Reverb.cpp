



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/Reverb.h"

#include <math.h>
#include "core/platform/audio/AudioBus.h"
#include "core/platform/audio/AudioFileReader.h"
#include "core/platform/audio/ReverbConvolver.h"
#include "core/platform/audio/VectorMath.h"
#include <wtf/MathExtras.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#if OS(DARWIN)
using namespace std;
#endif

namespace WebCore {

using namespace VectorMath;


const float GainCalibration = -58;
const float GainCalibrationSampleRate = 44100;


const float MinPower = 0.000125f;
    
static float calculateNormalizationScale(AudioBus* response)
{
    
    size_t numberOfChannels = response->numberOfChannels();
    size_t length = response->length();

    float power = 0;

    for (size_t i = 0; i < numberOfChannels; ++i) {
        float channelPower = 0;
        vsvesq(response->channel(i)->data(), 1, &channelPower, length);
        power += channelPower;
    }

    power = sqrt(power / (numberOfChannels * length));

    
    if (std::isinf(power) || std::isnan(power) || power < MinPower)
        power = MinPower;

    float scale = 1 / power;

    scale *= powf(10, GainCalibration * 0.05f); 

    
    if (response->sampleRate())
        scale *= GainCalibrationSampleRate / response->sampleRate();

    
    if (response->numberOfChannels() == 4)
        scale *= 0.5f;

    return scale;
}

Reverb::Reverb(AudioBus* impulseResponse, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads, bool normalize)
{
    float scale = 1;

    if (normalize) {
        scale = calculateNormalizationScale(impulseResponse);

        if (scale)
            impulseResponse->scale(scale);
    }

    initialize(impulseResponse, renderSliceSize, maxFFTSize, numberOfChannels, useBackgroundThreads);

    
    
    
    if (normalize && scale)
        impulseResponse->scale(1 / scale);
}

void Reverb::initialize(AudioBus* impulseResponseBuffer, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads)
{
    m_impulseResponseLength = impulseResponseBuffer->length();

    
    size_t numResponseChannels = impulseResponseBuffer->numberOfChannels();
    m_convolvers.reserveCapacity(numberOfChannels);

    int convolverRenderPhase = 0;
    for (size_t i = 0; i < numResponseChannels; ++i) {
        AudioChannel* channel = impulseResponseBuffer->channel(i);

        OwnPtr<ReverbConvolver> convolver = adoptPtr(new ReverbConvolver(channel, renderSliceSize, maxFFTSize, convolverRenderPhase, useBackgroundThreads));
        m_convolvers.append(convolver.release());

        convolverRenderPhase += renderSliceSize;
    }

    
    
    if (numResponseChannels == 4)
        m_tempBuffer = AudioBus::create(2, MaxFrameSize);
}

void Reverb::process(const AudioBus* sourceBus, AudioBus* destinationBus, size_t framesToProcess)
{
    
    
    bool isSafeToProcess = sourceBus && destinationBus && sourceBus->numberOfChannels() > 0 && destinationBus->numberOfChannels() > 0
        && framesToProcess <= MaxFrameSize && framesToProcess <= sourceBus->length() && framesToProcess <= destinationBus->length(); 
    
    ASSERT(isSafeToProcess);
    if (!isSafeToProcess)
        return;

    
    if (destinationBus->numberOfChannels() > 2) {
        destinationBus->zero();
        return;
    }

    AudioChannel* destinationChannelL = destinationBus->channel(0);
    const AudioChannel* sourceChannelL = sourceBus->channel(0);

    
    size_t numInputChannels = sourceBus->numberOfChannels();
    size_t numOutputChannels = destinationBus->numberOfChannels();
    size_t numReverbChannels = m_convolvers.size();

    if (numInputChannels == 2 && numReverbChannels == 2 && numOutputChannels == 2) {
        
        const AudioChannel* sourceChannelR = sourceBus->channel(1);
        AudioChannel* destinationChannelR = destinationBus->channel(1);
        m_convolvers[0]->process(sourceChannelL, destinationChannelL, framesToProcess);
        m_convolvers[1]->process(sourceChannelR, destinationChannelR, framesToProcess);
    } else  if (numInputChannels == 1 && numOutputChannels == 2 && numReverbChannels == 2) {
        
        for (int i = 0; i < 2; ++i) {
            AudioChannel* destinationChannel = destinationBus->channel(i);
            m_convolvers[i]->process(sourceChannelL, destinationChannel, framesToProcess);
        }
    } else if (numInputChannels == 1 && numReverbChannels == 1 && numOutputChannels == 2) {
        
        m_convolvers[0]->process(sourceChannelL, destinationChannelL, framesToProcess);

        
        AudioChannel* destinationChannelR = destinationBus->channel(1);
        bool isCopySafe = destinationChannelL->data() && destinationChannelR->data() && destinationChannelL->length() >= framesToProcess && destinationChannelR->length() >= framesToProcess;
        ASSERT(isCopySafe);
        if (!isCopySafe)
            return;
        memcpy(destinationChannelR->mutableData(), destinationChannelL->data(), sizeof(float) * framesToProcess);
    } else if (numInputChannels == 1 && numReverbChannels == 1 && numOutputChannels == 1) {
        
        m_convolvers[0]->process(sourceChannelL, destinationChannelL, framesToProcess);
    } else if (numInputChannels == 2 && numReverbChannels == 4 && numOutputChannels == 2) {
        
        const AudioChannel* sourceChannelR = sourceBus->channel(1);
        AudioChannel* destinationChannelR = destinationBus->channel(1);

        AudioChannel* tempChannelL = m_tempBuffer->channel(0);
        AudioChannel* tempChannelR = m_tempBuffer->channel(1);

        
        m_convolvers[0]->process(sourceChannelL, destinationChannelL, framesToProcess);
        m_convolvers[1]->process(sourceChannelL, destinationChannelR, framesToProcess);

        
        m_convolvers[2]->process(sourceChannelR, tempChannelL, framesToProcess);
        m_convolvers[3]->process(sourceChannelR, tempChannelR, framesToProcess);

        destinationBus->sumFrom(*m_tempBuffer);
    } else if (numInputChannels == 1 && numReverbChannels == 4 && numOutputChannels == 2) {
        
        
        AudioChannel* destinationChannelR = destinationBus->channel(1);

        AudioChannel* tempChannelL = m_tempBuffer->channel(0);
        AudioChannel* tempChannelR = m_tempBuffer->channel(1);

        
        m_convolvers[0]->process(sourceChannelL, destinationChannelL, framesToProcess);
        m_convolvers[1]->process(sourceChannelL, destinationChannelR, framesToProcess);

        
        m_convolvers[2]->process(sourceChannelL, tempChannelL, framesToProcess);
        m_convolvers[3]->process(sourceChannelL, tempChannelR, framesToProcess);

        destinationBus->sumFrom(*m_tempBuffer);
    } else {
        
        
        destinationBus->zero();
    }
}

void Reverb::reset()
{
    for (size_t i = 0; i < m_convolvers.size(); ++i)
        m_convolvers[i]->reset();
}

size_t Reverb::latencyFrames() const
{
    return !m_convolvers.isEmpty() ? m_convolvers.first()->latencyFrames() : 0;
}

} 

#endif 
