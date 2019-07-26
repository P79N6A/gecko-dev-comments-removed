



























#ifndef HRTFKernel_h
#define HRTFKernel_h

#include "core/platform/audio/FFTFrame.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class AudioChannel;
    






class HRTFKernel : public RefCounted<HRTFKernel> {
public:
    
    
    static PassRefPtr<HRTFKernel> create(AudioChannel* channel, size_t fftSize, float sampleRate)
    {
        return adoptRef(new HRTFKernel(channel, fftSize, sampleRate));
    }

    static PassRefPtr<HRTFKernel> create(PassOwnPtr<FFTFrame> fftFrame, float frameDelay, float sampleRate)
    {
        return adoptRef(new HRTFKernel(fftFrame, frameDelay, sampleRate));
    }

    
    static PassRefPtr<HRTFKernel> createInterpolatedKernel(HRTFKernel* kernel1, HRTFKernel* kernel2, float x);
  
    FFTFrame* fftFrame() { return m_fftFrame.get(); }
    
    size_t fftSize() const { return m_fftFrame->fftSize(); }
    float frameDelay() const { return m_frameDelay; }

    float sampleRate() const { return m_sampleRate; }
    double nyquist() const { return 0.5 * sampleRate(); }

    
    PassOwnPtr<AudioChannel> createImpulseResponse();

    void reportMemoryUsage(MemoryObjectInfo*) const;

private:
    
    HRTFKernel(AudioChannel*, size_t fftSize, float sampleRate);
    
    HRTFKernel(PassOwnPtr<FFTFrame> fftFrame, float frameDelay, float sampleRate)
        : m_fftFrame(fftFrame)
        , m_frameDelay(frameDelay)
        , m_sampleRate(sampleRate)
    {
    }
    
    OwnPtr<FFTFrame> m_fftFrame;
    float m_frameDelay;
    float m_sampleRate;
};

typedef Vector<RefPtr<HRTFKernel> > HRTFKernelList;

} 

#endif 
