



























#ifndef Reverb_h
#define Reverb_h

#include "ReverbConvolver.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "AudioSegment.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {
class ThreadSharedFloatArrayBufferList;
}

namespace WebCore {



class Reverb {
public:
    enum { MaxFrameSize = 256 };

    
    Reverb(mozilla::ThreadSharedFloatArrayBufferList* impulseResponseBuffer, size_t impulseResponseBufferLength, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads, bool normalize, float sampleRate);

    void process(const mozilla::AudioChunk* sourceBus, mozilla::AudioChunk* destinationBus, size_t framesToProcess);
    void reset();

    size_t impulseResponseLength() const { return m_impulseResponseLength; }
    size_t latencyFrames() const;

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
    void initialize(const nsTArray<const float*>& impulseResponseBuffer, size_t impulseResponseBufferLength, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads);

    size_t m_impulseResponseLength;

    nsTArray<nsAutoPtr<ReverbConvolver> > m_convolvers;

    
    mozilla::AudioChunk m_tempBuffer;
};

} 

#endif
