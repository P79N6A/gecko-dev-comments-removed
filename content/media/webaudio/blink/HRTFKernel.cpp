



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/HRTFKernel.h"

#include "core/platform/FloatConversion.h"
#include "core/platform/PlatformMemoryInstrumentation.h"
#include "core/platform/audio/AudioChannel.h"
#include "core/platform/audio/FFTFrame.h"
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {





static float extractAverageGroupDelay(AudioChannel* channel, size_t analysisFFTSize)
{
    ASSERT(channel);
        
    float* impulseP = channel->mutableData();
    
    bool isSizeGood = channel->length() >= analysisFFTSize;
    ASSERT(isSizeGood);
    if (!isSizeGood)
        return 0;
    
    
    ASSERT(1UL << static_cast<unsigned>(log2(analysisFFTSize)) == analysisFFTSize);

    FFTFrame estimationFrame(analysisFFTSize);
    estimationFrame.doFFT(impulseP);

    float frameDelay = narrowPrecisionToFloat(estimationFrame.extractAverageGroupDelay());
    estimationFrame.doInverseFFT(impulseP);

    return frameDelay;
}

HRTFKernel::HRTFKernel(AudioChannel* channel, size_t fftSize, float sampleRate)
    : m_frameDelay(0)
    , m_sampleRate(sampleRate)
{
    ASSERT(channel);

    
    m_frameDelay = extractAverageGroupDelay(channel, fftSize / 2);

    float* impulseResponse = channel->mutableData();
    size_t responseLength = channel->length();

    
    size_t truncatedResponseLength = min(responseLength, fftSize / 2); 

    
    unsigned numberOfFadeOutFrames = static_cast<unsigned>(sampleRate / 4410); 
    ASSERT(numberOfFadeOutFrames < truncatedResponseLength);
    if (numberOfFadeOutFrames < truncatedResponseLength) {
        for (unsigned i = truncatedResponseLength - numberOfFadeOutFrames; i < truncatedResponseLength; ++i) {
            float x = 1.0f - static_cast<float>(i - (truncatedResponseLength - numberOfFadeOutFrames)) / numberOfFadeOutFrames;
            impulseResponse[i] *= x;
        }
    }

    m_fftFrame = adoptPtr(new FFTFrame(fftSize));
    m_fftFrame->doPaddedFFT(impulseResponse, truncatedResponseLength);
}

PassOwnPtr<AudioChannel> HRTFKernel::createImpulseResponse()
{
    OwnPtr<AudioChannel> channel = adoptPtr(new AudioChannel(fftSize()));
    FFTFrame fftFrame(*m_fftFrame);

    
    fftFrame.addConstantGroupDelay(m_frameDelay);
    fftFrame.doInverseFFT(channel->mutableData());

    return channel.release();
}


PassRefPtr<HRTFKernel> HRTFKernel::createInterpolatedKernel(HRTFKernel* kernel1, HRTFKernel* kernel2, float x)
{
    ASSERT(kernel1 && kernel2);
    if (!kernel1 || !kernel2)
        return 0;
 
    ASSERT(x >= 0.0 && x < 1.0);
    x = min(1.0f, max(0.0f, x));
    
    float sampleRate1 = kernel1->sampleRate();
    float sampleRate2 = kernel2->sampleRate();
    ASSERT(sampleRate1 == sampleRate2);
    if (sampleRate1 != sampleRate2)
        return 0;
    
    float frameDelay = (1 - x) * kernel1->frameDelay() + x * kernel2->frameDelay();
    
    OwnPtr<FFTFrame> interpolatedFrame = FFTFrame::createInterpolatedFrame(*kernel1->fftFrame(), *kernel2->fftFrame(), x);
    return HRTFKernel::create(interpolatedFrame.release(), frameDelay, sampleRate1);
}

void HRTFKernel::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, PlatformMemoryTypes::AudioSharedData);
    info.addMember(m_fftFrame, "fftFrame");
}

} 

#endif 
