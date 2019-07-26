





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
    ConvertTimeToTickHelper* This = static_cast<ConvertTimeToTickHelper*> (aClosure);
    MOZ_ASSERT(This->mSourceStream->SampleRate() == This->mDestinationStream->SampleRate());
    return WebAudioUtils::ConvertDestinationStreamTimeToSourceStreamTime(
        aTime, This->mSourceStream, This->mDestinationStream);
  }
};

TrackTicks
WebAudioUtils::ConvertDestinationStreamTimeToSourceStreamTime(double aTime,
                                                              AudioNodeStream* aSource,
                                                              MediaStream* aDestination)
{
  StreamTime streamTime = std::max<MediaTime>(0, SecondsToMediaTime(aTime));
  GraphTime graphTime = aDestination->StreamTimeToGraphTime(streamTime);
  StreamTime thisStreamTime = aSource->GraphTimeToStreamTimeOptimistic(graphTime);
  TrackTicks ticks = TimeToTicksRoundUp(aSource->SampleRate(), thisStreamTime);
  return ticks;
}

double
WebAudioUtils::StreamPositionToDestinationTime(TrackTicks aSourcePosition,
                                               AudioNodeStream* aSource,
                                               AudioNodeStream* aDestination)
{
  MOZ_ASSERT(aSource->SampleRate() == aDestination->SampleRate());
  StreamTime sourceTime = TicksToTimeRoundDown(aSource->SampleRate(), aSourcePosition);
  GraphTime graphTime = aSource->StreamTimeToGraphTime(sourceTime);
  StreamTime destinationTime = aDestination->GraphTimeToStreamTimeOptimistic(graphTime);
  return MediaTimeToSeconds(destinationTime);
}

void
WebAudioUtils::ConvertAudioParamToTicks(AudioParamTimeline& aParam,
                                        AudioNodeStream* aSource,
                                        AudioNodeStream* aDest)
{
  MOZ_ASSERT(!aSource || aSource->SampleRate() == aDest->SampleRate());
  ConvertTimeToTickHelper ctth;
  ctth.mSourceStream = aSource;
  ctth.mDestinationStream = aDest;
  aParam.ConvertEventTimesToTicks(ConvertTimeToTickHelper::Convert, &ctth, aDest->SampleRate());
}

}
}
