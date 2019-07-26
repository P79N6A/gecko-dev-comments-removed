



























#ifndef HRTFKernel_h
#define HRTFKernel_h

#include "nsAutoPtr.h"
#include "nsAutoRef.h"
#include "nsTArray.h"
#include "mozilla/FFTBlock.h"
#include "mozilla/MemoryReporting.h"

namespace WebCore {

using mozilla::FFTBlock;







class HRTFKernel {
public:
    
    
    
    static nsReturnRef<HRTFKernel> create(float* impulseResponse, size_t length, float sampleRate);

    static nsReturnRef<HRTFKernel> create(nsAutoPtr<FFTBlock> fftFrame, float frameDelay, float sampleRate);

    
    static nsReturnRef<HRTFKernel> createInterpolatedKernel(HRTFKernel* kernel1, HRTFKernel* kernel2, float x);
  
    FFTBlock* fftFrame() { return m_fftFrame.get(); }
    
    size_t fftSize() const { return m_fftFrame->FFTSize(); }
    float frameDelay() const { return m_frameDelay; }

    float sampleRate() const { return m_sampleRate; }
    double nyquist() const { return 0.5 * sampleRate(); }

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
    {
        size_t amount = aMallocSizeOf(this);
        amount += m_fftFrame->SizeOfIncludingThis(aMallocSizeOf);
        return amount;
    }

private:
    HRTFKernel(const HRTFKernel& other) MOZ_DELETE;
    void operator=(const HRTFKernel& other) MOZ_DELETE;

    
    HRTFKernel(float* impulseResponse, size_t fftSize, float sampleRate);
    
    HRTFKernel(nsAutoPtr<FFTBlock> fftFrame, float frameDelay, float sampleRate)
        : m_fftFrame(fftFrame)
        , m_frameDelay(frameDelay)
        , m_sampleRate(sampleRate)
    {
    }
    
    nsAutoPtr<FFTBlock> m_fftFrame;
    float m_frameDelay;
    float m_sampleRate;
};

typedef nsTArray<nsAutoRef<HRTFKernel> > HRTFKernelList;

} 

template <>
class nsAutoRefTraits<WebCore::HRTFKernel> :
    public nsPointerRefTraits<WebCore::HRTFKernel> {
public:
    static void Release(WebCore::HRTFKernel* ptr) { delete(ptr); }
};

namespace WebCore {

inline nsReturnRef<HRTFKernel> HRTFKernel::create(float* impulseResponse, size_t length, float sampleRate)
{
    return nsReturnRef<HRTFKernel>(new HRTFKernel(impulseResponse, length, sampleRate));
}

inline nsReturnRef<HRTFKernel> HRTFKernel::create(nsAutoPtr<FFTBlock> fftFrame, float frameDelay, float sampleRate)
{
    return nsReturnRef<HRTFKernel>(new HRTFKernel(fftFrame, frameDelay, sampleRate));
}

} 
#endif 
