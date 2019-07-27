



























#include "Reverb.h"
#include "ReverbConvolverStage.h"

#include <math.h>
#include "ReverbConvolver.h"
#include "mozilla/FloatingPoint.h"

using namespace mozilla;

namespace WebCore {


const float GainCalibration = -58;
const float GainCalibrationSampleRate = 44100;


const float MinPower = 0.000125f;

static float calculateNormalizationScale(ThreadSharedFloatArrayBufferList* response, size_t aLength, float sampleRate)
{
    
    size_t numberOfChannels = response->GetChannels();

    float power = 0;

    for (size_t i = 0; i < numberOfChannels; ++i) {
        float channelPower = AudioBufferSumOfSquares(static_cast<const float*>(response->GetData(i)), aLength);
        power += channelPower;
    }

    power = sqrt(power / (numberOfChannels * aLength));

    
    if (!IsFinite(power) || IsNaN(power) || power < MinPower)
        power = MinPower;

    float scale = 1 / power;

    scale *= powf(10, GainCalibration * 0.05f); 

    
    if (sampleRate)
        scale *= GainCalibrationSampleRate / sampleRate;

    
    if (response->GetChannels() == 4)
        scale *= 0.5f;

    return scale;
}

Reverb::Reverb(ThreadSharedFloatArrayBufferList* impulseResponse, size_t impulseResponseBufferLength, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads, bool normalize, float sampleRate)
{
    float scale = 1;

    nsAutoTArray<const float*,4> irChannels;
    for (size_t i = 0; i < impulseResponse->GetChannels(); ++i) {
        irChannels.AppendElement(impulseResponse->GetData(i));
    }
    nsAutoTArray<float,1024> tempBuf;

    if (normalize) {
        scale = calculateNormalizationScale(impulseResponse, impulseResponseBufferLength, sampleRate);

        if (scale) {
            tempBuf.SetLength(irChannels.Length()*impulseResponseBufferLength);
            for (uint32_t i = 0; i < irChannels.Length(); ++i) {
                float* buf = &tempBuf[i*impulseResponseBufferLength];
                AudioBufferCopyWithScale(irChannels[i], scale, buf,
                                         impulseResponseBufferLength);
                irChannels[i] = buf;
            }
        }
    }

    initialize(irChannels, impulseResponseBufferLength, renderSliceSize,
               maxFFTSize, numberOfChannels, useBackgroundThreads);
}

size_t Reverb::sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t amount = aMallocSizeOf(this);
    amount += m_convolvers.SizeOfExcludingThis(aMallocSizeOf);
    for (size_t i = 0; i < m_convolvers.Length(); i++) {
        if (m_convolvers[i]) {
            amount += m_convolvers[i]->sizeOfIncludingThis(aMallocSizeOf);
        }
    }

    amount += m_tempBuffer.SizeOfExcludingThis(aMallocSizeOf, false);
    return amount;
}


void Reverb::initialize(const nsTArray<const float*>& impulseResponseBuffer,
                        size_t impulseResponseBufferLength, size_t renderSliceSize,
                        size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads)
{
    m_impulseResponseLength = impulseResponseBufferLength;

    
    size_t numResponseChannels = impulseResponseBuffer.Length();
    m_convolvers.SetCapacity(numberOfChannels);

    int convolverRenderPhase = 0;
    for (size_t i = 0; i < numResponseChannels; ++i) {
        const float* channel = impulseResponseBuffer[i];
        size_t length = impulseResponseBufferLength;

        nsAutoPtr<ReverbConvolver> convolver(new ReverbConvolver(channel, length, renderSliceSize, maxFFTSize, convolverRenderPhase, useBackgroundThreads));
        m_convolvers.AppendElement(convolver.forget());

        convolverRenderPhase += renderSliceSize;
    }

    
    
    if (numResponseChannels == 4) {
        AllocateAudioBlock(2, &m_tempBuffer);
        WriteZeroesToAudioBlock(&m_tempBuffer, 0, WEBAUDIO_BLOCK_SIZE);
    }
}

void Reverb::process(const AudioChunk* sourceBus, AudioChunk* destinationBus, size_t framesToProcess)
{
    
    
    bool isSafeToProcess = sourceBus && destinationBus && sourceBus->mChannelData.Length() > 0 && destinationBus->mChannelData.Length() > 0
        && framesToProcess <= MaxFrameSize && framesToProcess <= size_t(sourceBus->mDuration) && framesToProcess <= size_t(destinationBus->mDuration);

    MOZ_ASSERT(isSafeToProcess);
    if (!isSafeToProcess)
        return;

    
    MOZ_ASSERT(destinationBus->mChannelData.Length() <= 2);

    float* destinationChannelL = static_cast<float*>(const_cast<void*>(destinationBus->mChannelData[0]));
    const float* sourceBusL = static_cast<const float*>(sourceBus->mChannelData[0]);

    
    size_t numInputChannels = sourceBus->mChannelData.Length();
    size_t numOutputChannels = destinationBus->mChannelData.Length();
    size_t numReverbChannels = m_convolvers.Length();

    if (numInputChannels == 2 && numReverbChannels == 2 && numOutputChannels == 2) {
        
        const float* sourceBusR = static_cast<const float*>(sourceBus->mChannelData[1]);
        float* destinationChannelR = static_cast<float*>(const_cast<void*>(destinationBus->mChannelData[1]));
        m_convolvers[0]->process(sourceBusL, sourceBus->mDuration, destinationChannelL, destinationBus->mDuration, framesToProcess);
        m_convolvers[1]->process(sourceBusR, sourceBus->mDuration, destinationChannelR, destinationBus->mDuration, framesToProcess);
    } else  if (numInputChannels == 1 && numOutputChannels == 2 && numReverbChannels == 2) {
        
        for (int i = 0; i < 2; ++i) {
            float* destinationChannel = static_cast<float*>(const_cast<void*>(destinationBus->mChannelData[i]));
            m_convolvers[i]->process(sourceBusL, sourceBus->mDuration, destinationChannel, destinationBus->mDuration, framesToProcess);
        }
    } else if (numInputChannels == 1 && numReverbChannels == 1 && numOutputChannels == 2) {
        
        m_convolvers[0]->process(sourceBusL, sourceBus->mDuration, destinationChannelL, destinationBus->mDuration, framesToProcess);

        
        float* destinationChannelR = static_cast<float*>(const_cast<void*>(destinationBus->mChannelData[1]));
        bool isCopySafe = destinationChannelL && destinationChannelR && size_t(destinationBus->mDuration) >= framesToProcess && size_t(destinationBus->mDuration) >= framesToProcess;
        MOZ_ASSERT(isCopySafe);
        if (!isCopySafe)
            return;
        PodCopy(destinationChannelR, destinationChannelL, framesToProcess);
    } else if (numInputChannels == 1 && numReverbChannels == 1 && numOutputChannels == 1) {
        
        m_convolvers[0]->process(sourceBusL, sourceBus->mDuration, destinationChannelL, destinationBus->mDuration, framesToProcess);
    } else if (numInputChannels == 2 && numReverbChannels == 4 && numOutputChannels == 2) {
        
        const float* sourceBusR = static_cast<const float*>(sourceBus->mChannelData[1]);
        float* destinationChannelR = static_cast<float*>(const_cast<void*>(destinationBus->mChannelData[1]));

        float* tempChannelL = static_cast<float*>(const_cast<void*>(m_tempBuffer.mChannelData[0]));
        float* tempChannelR = static_cast<float*>(const_cast<void*>(m_tempBuffer.mChannelData[1]));

        
        m_convolvers[0]->process(sourceBusL, sourceBus->mDuration, destinationChannelL, destinationBus->mDuration, framesToProcess);
        m_convolvers[1]->process(sourceBusL, sourceBus->mDuration, destinationChannelR, destinationBus->mDuration, framesToProcess);

        
        m_convolvers[2]->process(sourceBusR, sourceBus->mDuration, tempChannelL, m_tempBuffer.mDuration, framesToProcess);
        m_convolvers[3]->process(sourceBusR, sourceBus->mDuration, tempChannelR, m_tempBuffer.mDuration, framesToProcess);

        AudioBufferAddWithScale(tempChannelL, 1.0f, destinationChannelL, sourceBus->mDuration);
        AudioBufferAddWithScale(tempChannelR, 1.0f, destinationChannelR, sourceBus->mDuration);
    } else if (numInputChannels == 1 && numReverbChannels == 4 && numOutputChannels == 2) {
        
        
        float* destinationChannelR = static_cast<float*>(const_cast<void*>(destinationBus->mChannelData[1]));

        float* tempChannelL = static_cast<float*>(const_cast<void*>(m_tempBuffer.mChannelData[0]));
        float* tempChannelR = static_cast<float*>(const_cast<void*>(m_tempBuffer.mChannelData[1]));

        
        m_convolvers[0]->process(sourceBusL, sourceBus->mDuration, destinationChannelL, destinationBus->mDuration, framesToProcess);
        m_convolvers[1]->process(sourceBusL, sourceBus->mDuration, destinationChannelR, destinationBus->mDuration, framesToProcess);

        
        m_convolvers[2]->process(sourceBusL, sourceBus->mDuration, tempChannelL, m_tempBuffer.mDuration, framesToProcess);
        m_convolvers[3]->process(sourceBusL, sourceBus->mDuration, tempChannelR, m_tempBuffer.mDuration, framesToProcess);

        AudioBufferAddWithScale(tempChannelL, 1.0f, destinationChannelL, sourceBus->mDuration);
        AudioBufferAddWithScale(tempChannelR, 1.0f, destinationChannelR, sourceBus->mDuration);
    } else {
        
        
        destinationBus->SetNull(destinationBus->mDuration);
    }
}

void Reverb::reset()
{
    for (size_t i = 0; i < m_convolvers.Length(); ++i)
        m_convolvers[i]->reset();
}

size_t Reverb::latencyFrames() const
{
    return !m_convolvers.IsEmpty() ? m_convolvers[0]->latencyFrames() : 0;
}

} 
