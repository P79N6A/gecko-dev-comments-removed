




#ifndef MOZILLA_AUDIONODEEXTERNALINPUTSTREAM_H_
#define MOZILLA_AUDIONODEEXTERNALINPUTSTREAM_H_

#include "MediaStreamGraph.h"
#include "AudioNodeStream.h"
#include "mozilla/Atomics.h"

namespace mozilla {







class AudioNodeExternalInputStream final : public AudioNodeStream
{
public:
  AudioNodeExternalInputStream(AudioNodeEngine* aEngine, TrackRate aSampleRate,
                               uint32_t aContextId);
protected:
  ~AudioNodeExternalInputStream();

public:
  virtual void ProcessInput(GraphTime aFrom, GraphTime aTo,
                            uint32_t aFlags) override;

private:
  




  bool IsEnabled();
};

}

#endif 
