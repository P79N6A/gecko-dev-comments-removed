





#ifndef AudioParamTimeline_h_
#define AudioParamTimeline_h_




#include "AudioEventTimeline.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {

namespace dom {

typedef AudioEventTimeline<ErrorResult> AudioParamTimeline;

}
}

#endif

