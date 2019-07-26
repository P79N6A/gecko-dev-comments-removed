





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

    ConvertTimeToTickHelper* This = static_cast<ConvertTimeToTickHelper*> (aClosure);
    TrackTicks tick = This->mSourceStream->GetCurrentPosition();
    StreamTime streamTime = TicksToTimeRoundDown(sampleRate, tick);
    GraphTime graphTime = This->mSourceStream->StreamTimeToGraphTime(streamTime);
    StreamTime destinationStreamTime = This->mDestinationStream->GraphTimeToStreamTime(graphTime);
    return TimeToTicksRoundDown(sampleRate, destinationStreamTime + SecondsToMediaTime(aTime));
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
