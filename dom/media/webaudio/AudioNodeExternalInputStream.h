




#ifndef MOZILLA_AUDIONODEEXTERNALINPUTSTREAM_H_
#define MOZILLA_AUDIONODEEXTERNALINPUTSTREAM_H_

#include "MediaStreamGraph.h"
#include "AudioNodeStream.h"
#include "mozilla/Atomics.h"

namespace mozilla {







class AudioNodeExternalInputStream : public AudioNodeStream {
public:
  AudioNodeExternalInputStream(AudioNodeEngine* aEngine, TrackRate aSampleRate);
protected:
  ~AudioNodeExternalInputStream();

public:
  virtual void ProcessInput(GraphTime aFrom, GraphTime aTo, uint32_t aFlags) MOZ_OVERRIDE;

private:
  




  bool IsEnabled();
};

}

#endif 
