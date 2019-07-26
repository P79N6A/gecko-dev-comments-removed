



























#include "DirectConvolver.h"
#include "mozilla/PodOperations.h"

using namespace mozilla;

namespace WebCore {

DirectConvolver::DirectConvolver(size_t inputBlockSize)
    : m_inputBlockSize(inputBlockSize)
{
  m_buffer.SetLength(inputBlockSize * 2);
  PodZero(m_buffer.Elements(), inputBlockSize * 2);
}

void DirectConvolver::process(const nsTArray<float>* convolutionKernel, const float* sourceP, float* destP, size_t framesToProcess)
{
    MOZ_ASSERT(framesToProcess == m_inputBlockSize);
    if (framesToProcess != m_inputBlockSize)
        return;

    
    size_t kernelSize = convolutionKernel->Length();
    MOZ_ASSERT(kernelSize <= m_inputBlockSize);
    if (kernelSize > m_inputBlockSize)
        return;

    const float* kernelP = convolutionKernel->Elements();

    
    bool isCopyGood = kernelP && sourceP && destP && m_buffer.Elements();
    MOZ_ASSERT(isCopyGood);
    if (!isCopyGood)
        return;

    float* inputP = m_buffer.Elements() + m_inputBlockSize;

    
    memcpy(inputP, sourceP, sizeof(float) * framesToProcess);

    
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

    
    memcpy(m_buffer.Elements(), inputP, sizeof(float) * framesToProcess);
}

void DirectConvolver::reset()
{
    PodZero(m_buffer.Elements(), m_buffer.Length());
}

} 
