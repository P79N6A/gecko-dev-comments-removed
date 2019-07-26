





#ifndef AudioParamTimeline_h_
#define AudioParamTimeline_h_

#include "AudioEventTimeline.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"
#include "MediaStreamGraph.h"

namespace mozilla {

namespace dom {








class AudioParamTimeline : public AudioEventTimeline<ErrorResult>
{
public:
  explicit AudioParamTimeline(float aDefaultValue)
    : AudioEventTimeline<ErrorResult>(aDefaultValue)
  {
  }

  MediaStream* Stream() const
  {
    return mStream;
  }

protected:
  
  nsRefPtr<MediaStream> mStream;
};

}
}

#endif

