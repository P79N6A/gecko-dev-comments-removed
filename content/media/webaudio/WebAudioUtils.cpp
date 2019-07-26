





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
    if (This->mSourceStream) {
      return WebAudioUtils::ConvertDestinationStreamTimeToSourceStreamTime(
          aTime, This->mSourceStream, This->mDestinationStream);
    } else {
      StreamTime streamTime = This->mDestinationStream->GetCurrentPosition();
      return TimeToTicksRoundUp(IdealAudioRate(), streamTime + SecondsToMediaTime(aTime));
    }
  }
};

TrackTicks
WebAudioUtils::ConvertDestinationStreamTimeToSourceStreamTime(double aTime,
                                                              MediaStream* aSource,
                                                              MediaStream* aDestination)
{
  StreamTime streamTime = std::max<MediaTime>(0, SecondsToMediaTime(aTime));
  GraphTime graphTime = aDestination->StreamTimeToGraphTime(streamTime);
  StreamTime thisStreamTime = aSource->GraphTimeToStreamTimeOptimistic(graphTime);
  TrackTicks ticks = TimeToTicksRoundUp(IdealAudioRate(), thisStreamTime);
  return ticks;
}

double
WebAudioUtils::StreamPositionToDestinationTime(TrackTicks aSourcePosition,
                                               AudioNodeStream* aSource,
                                               AudioNodeStream* aDestination)
{
  StreamTime sourceTime = TicksToTimeRoundDown(IdealAudioRate(), aSourcePosition);
  GraphTime graphTime = aSource->StreamTimeToGraphTime(sourceTime);
  StreamTime destinationTime = aDestination->GraphTimeToStreamTimeOptimistic(graphTime);
  return MediaTimeToSeconds(destinationTime);
}

void
WebAudioUtils::ConvertAudioParamToTicks(AudioParamTimeline& aParam,
                                        AudioNodeStream* aSource,
                                        AudioNodeStream* aDest)
{
  ConvertTimeToTickHelper ctth;
  ctth.mSourceStream = aSource;
  ctth.mDestinationStream = aDest;
  aParam.ConvertEventTimesToTicks(ConvertTimeToTickHelper::Convert, &ctth, IdealAudioRate());
}

}
}
