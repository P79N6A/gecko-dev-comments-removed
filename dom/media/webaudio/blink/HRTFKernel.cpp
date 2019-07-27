



























#include "HRTFKernel.h"
namespace WebCore {





static float extractAverageGroupDelay(float* impulseP, size_t length)
{
    
    MOZ_ASSERT(length && (length & (length - 1)) == 0);

    FFTBlock estimationFrame(length);
    estimationFrame.PerformFFT(impulseP);

    float frameDelay = static_cast<float>(estimationFrame.ExtractAverageGroupDelay());
    estimationFrame.GetInverse(impulseP);

    return frameDelay;
}

HRTFKernel::HRTFKernel(float* impulseResponse, size_t length, float sampleRate)
    : m_frameDelay(0)
    , m_sampleRate(sampleRate)
{
    
    m_frameDelay = extractAverageGroupDelay(impulseResponse, length);

    
    
    unsigned fftSize = 2 * length;

    
    
    unsigned numberOfFadeOutFrames = static_cast<unsigned>(sampleRate / 4410); 
    MOZ_ASSERT(numberOfFadeOutFrames < length);
    if (numberOfFadeOutFrames < length) {
        for (unsigned i = length - numberOfFadeOutFrames; i < length; ++i) {
            float x = 1.0f - static_cast<float>(i - (length - numberOfFadeOutFrames)) / numberOfFadeOutFrames;
            impulseResponse[i] *= x;
        }
    }

    m_fftFrame = new FFTBlock(fftSize);
    m_fftFrame->PadAndMakeScaledDFT(impulseResponse, length);
}


nsReturnRef<HRTFKernel> HRTFKernel::createInterpolatedKernel(HRTFKernel* kernel1, HRTFKernel* kernel2, float x)
{
    MOZ_ASSERT(kernel1 && kernel2);
    if (!kernel1 || !kernel2)
        return nsReturnRef<HRTFKernel>();
 
    MOZ_ASSERT(x >= 0.0 && x < 1.0);
    x = mozilla::clamped(x, 0.0f, 1.0f);
    
    float sampleRate1 = kernel1->sampleRate();
    float sampleRate2 = kernel2->sampleRate();
    MOZ_ASSERT(sampleRate1 == sampleRate2);
    if (sampleRate1 != sampleRate2)
        return nsReturnRef<HRTFKernel>();
    
    float frameDelay = (1 - x) * kernel1->frameDelay() + x * kernel2->frameDelay();
    
    nsAutoPtr<FFTBlock> interpolatedFrame(
        FFTBlock::CreateInterpolatedBlock(*kernel1->fftFrame(), *kernel2->fftFrame(), x));
    return HRTFKernel::create(interpolatedFrame, frameDelay, sampleRate1);
}

} 
