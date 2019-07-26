



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/DirectConvolver.h"

#if OS(DARWIN)
#include <Accelerate/Accelerate.h>
#endif

#include "core/platform/audio/VectorMath.h"

namespace WebCore {

using namespace VectorMath;
    
DirectConvolver::DirectConvolver(size_t inputBlockSize)
    : m_inputBlockSize(inputBlockSize)
#if USE(WEBAUDIO_IPP)
    , m_overlayBuffer(inputBlockSize)
#endif 
    , m_buffer(inputBlockSize * 2)
{
}

void DirectConvolver::process(AudioFloatArray* convolutionKernel, const float* sourceP, float* destP, size_t framesToProcess)
{
    ASSERT(framesToProcess == m_inputBlockSize);
    if (framesToProcess != m_inputBlockSize)
        return;

    
    size_t kernelSize = convolutionKernel->size();
    ASSERT(kernelSize <= m_inputBlockSize);
    if (kernelSize > m_inputBlockSize)
        return;

    float* kernelP = convolutionKernel->data();

    
    bool isCopyGood = kernelP && sourceP && destP && m_buffer.data();
    ASSERT(isCopyGood);
    if (!isCopyGood)
        return;

#if USE(WEBAUDIO_IPP)
    float* outputBuffer = m_buffer.data();
    float* overlayBuffer = m_overlayBuffer.data();
    bool isCopyGood2 = overlayBuffer && m_overlayBuffer.size() >= kernelSize && m_buffer.size() == m_inputBlockSize * 2;
    ASSERT(isCopyGood2);
    if (!isCopyGood2)
        return;

    ippsConv_32f(static_cast<const Ipp32f*>(sourceP), framesToProcess, static_cast<Ipp32f*>(kernelP), kernelSize, static_cast<Ipp32f*>(outputBuffer));

    vadd(outputBuffer, 1, overlayBuffer, 1, destP, 1, framesToProcess);
    memcpy(overlayBuffer, outputBuffer + m_inputBlockSize, sizeof(float) * kernelSize);
#else
    float* inputP = m_buffer.data() + m_inputBlockSize;

    
    memcpy(inputP, sourceP, sizeof(float) * framesToProcess);

#if OS(DARWIN)
#if defined(__ppc__) || defined(__i386__)
    conv(inputP - kernelSize + 1, 1, kernelP + kernelSize - 1, -1, destP, 1, framesToProcess, kernelSize);
#else
    vDSP_conv(inputP - kernelSize + 1, 1, kernelP + kernelSize - 1, -1, destP, 1, framesToProcess, kernelSize);
#endif 
#else
    
#define CONVOLVE_ONE_SAMPLE             \
    sum += inputP[i - j] * kernelP[j];  \
    j++;

    size_t i = 0;
    while (i < framesToProcess) {
        size_t j = 0;
        float sum = 0;
        
        
        if (kernelSize == 32) {
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

        } else if (kernelSize == 64) {
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

        } else if (kernelSize == 128) {
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 

            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
            CONVOLVE_ONE_SAMPLE 
        } else {
            while (j < kernelSize) {
                
                CONVOLVE_ONE_SAMPLE
            }
        }
        destP[i++] = sum;
    }
#endif 

    
    memcpy(m_buffer.data(), inputP, sizeof(float) * framesToProcess);
#endif
}

void DirectConvolver::reset()
{
    m_buffer.zero();
#if USE(WEBAUDIO_IPP)
    m_overlayBuffer.zero();
#endif 
}

} 

#endif 
