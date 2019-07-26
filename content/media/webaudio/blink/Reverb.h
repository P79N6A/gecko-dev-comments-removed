



























#ifndef Reverb_h
#define Reverb_h

#include "core/platform/audio/ReverbConvolver.h"
#include <wtf/Vector.h>

namespace WebCore {

class AudioBus;
    


class Reverb {
public:
    enum { MaxFrameSize = 256 };

    
    Reverb(AudioBus* impulseResponseBuffer, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads, bool normalize);

    void process(const AudioBus* sourceBus, AudioBus* destinationBus, size_t framesToProcess);
    void reset();

    size_t impulseResponseLength() const { return m_impulseResponseLength; }
    size_t latencyFrames() const;

private:
    void initialize(AudioBus* impulseResponseBuffer, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads);

    size_t m_impulseResponseLength;

    Vector<OwnPtr<ReverbConvolver> > m_convolvers;

    
    RefPtr<AudioBus> m_tempBuffer;
};

} 

#endif
