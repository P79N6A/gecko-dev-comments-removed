





#include "WebAudioUtils.h"
#include "AudioNodeStream.h"

namespace mozilla {

namespace dom {

struct ConvertTimeToTickHelper
{
  AudioNodeStream* mSourceStream;
  AudioNodeStream* mDestinationStream;

  static int64_t Convert(double aTime, void* aClosure)
  {
    TrackRate sampleRate = IdealAudioRate();
    StreamTime streamTime;

    ConvertTimeToTickHelper* This = static_cast<ConvertTimeToTickHelper*> (aClosure);
    if (This->mSourceStream) {
      TrackTicks tick = This->mDestinationStream->GetCurrentPosition();
      StreamTime destinationStreamTime = TicksToTimeRoundDown(sampleRate, tick);
      GraphTime graphTime = This->mDestinationStream->StreamTimeToGraphTime(destinationStreamTime);
      streamTime = This->mSourceStream->GraphTimeToStreamTime(graphTime);
    } else {
      streamTime = This->mDestinationStream->GetCurrentPosition();
    }
    return TimeToTicksRoundDown(sampleRate, streamTime + SecondsToMediaTime(aTime));
  }
};

void
WebAudioUtils::ConvertAudioParamToTicks(AudioParamTimeline& aParam,
                                        AudioNodeStream* aSource,
                                        AudioNodeStream* aDest)
{
  ConvertTimeToTickHelper ctth;
  ctth.mSourceStream = aSource;
  ctth.mDestinationStream = aDest;
  aParam.ConvertEventTimesToTicks(ConvertTimeToTickHelper::Convert, &ctth);
}

}
}
