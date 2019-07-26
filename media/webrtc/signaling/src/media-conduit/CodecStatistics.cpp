



#include "CodecStatistics.h"

#include "CSFLog.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;
using namespace webrtc;


static const char* logTag ="WebrtcVideoSessionConduit";

VideoCodecStatistics::VideoCodecStatistics(int channel,
                                           ViECodec* codec,
                                           bool encoder) :
  mChannel(channel),
  mSentRawFrames(0),
  mPtrViECodec(codec),
  mEncoderDroppedFrames(0),
  mDecoderDiscardedPackets(0),
  mEncoderMode(encoder),
  mReceiveState(kReceiveStateInitial),
  mRecoveredBeforeLoss(0),
  mRecoveredLosses(0)
{
  MOZ_ASSERT(mPtrViECodec);
  if (mEncoderMode) {
    mPtrViECodec->RegisterEncoderObserver(mChannel, *this);
  } else {
    mPtrViECodec->RegisterDecoderObserver(mChannel, *this);
  }
}

VideoCodecStatistics::~VideoCodecStatistics()
{
  if (mEncoderMode) {
    mPtrViECodec->DeregisterEncoderObserver(mChannel);
  } else {
    mPtrViECodec->DeregisterDecoderObserver(mChannel);
  }
}

void VideoCodecStatistics::OutgoingRate(const int video_channel,
                                        const uint32_t framerate,
                                        const uint32_t bitrate)
{
  unsigned int keyFrames, deltaFrames;
  mPtrViECodec->GetSendCodecStatistics(video_channel, keyFrames, deltaFrames);
  uint32_t dropped = mSentRawFrames - (keyFrames + deltaFrames);
  CSFLogDebug(logTag,
              "encoder statistics - framerate: %u, bitrate: %u, dropped frames: %u",
              framerate, bitrate, dropped);
  mEncoderBitRate.Push(bitrate);
  mEncoderFps.Push(framerate);
  mEncoderDroppedFrames += dropped;
}

void VideoCodecStatistics::IncomingCodecChanged(const int video_channel,
                                                const VideoCodec& video_codec)
{
  CSFLogDebug(logTag,
              "channel %d change codec to \"%s\" ",
              video_channel, video_codec.plName);
}

void VideoCodecStatistics::IncomingRate(const int video_channel,
                                        const unsigned int framerate,
                                        const unsigned int bitrate)
{
  unsigned int discarded = mPtrViECodec->GetDiscardedPackets(video_channel);
  CSFLogDebug(logTag,
      "decoder statistics - framerate: %u, bitrate: %u, discarded packets %u",
      framerate, bitrate, discarded);
  mDecoderBitRate.Push(bitrate);
  mDecoderFps.Push(framerate);
  mDecoderDiscardedPackets += discarded;
}

void VideoCodecStatistics::ReceiveStateChange(const int aChannel,
                                              VideoReceiveState aState)
{
  CSFLogDebug(logTag,"New state for %d: %d (was %d)", aChannel, aState, mReceiveState);
#ifdef MOZILLA_INTERNAL_API
  if (mFirstDecodeTime.IsNull()) {
    mFirstDecodeTime = TimeStamp::Now();
  }
  





  switch (mReceiveState) {
    case kReceiveStateNormal:
    case kReceiveStateInitial:
      
      if (aState != kReceiveStateNormal && aState != kReceiveStateInitial) {
        
        if (aState != kReceiveStatePreemptiveNACK) {
          mReceiveFailureTime = TimeStamp::Now();
        }
      } 
      break;
    default:
      
      if (aState == kReceiveStateNormal || aState == kReceiveStateInitial) {

        if (mReceiveState == kReceiveStatePreemptiveNACK) {
          mRecoveredBeforeLoss++;
          CSFLogError(logTag, "Video error avoided by NACK recovery");
        } else if (!mReceiveFailureTime.IsNull()) { 
          TimeDuration timeDelta = TimeStamp::Now() - mReceiveFailureTime;
          CSFLogError(logTag, "Video error duration: %u ms",
                      static_cast<uint32_t>(timeDelta.ToMilliseconds()));
          Telemetry::Accumulate(Telemetry::WEBRTC_VIDEO_ERROR_RECOVERY_MS,
                                static_cast<uint32_t>(timeDelta.ToMilliseconds()));

          mRecoveredLosses++; 
          mTotalLossTime += timeDelta;  
        }
      } 
      break;
  }

#endif

  mReceiveState = aState;
}

void VideoCodecStatistics::EndOfCallStats()
{
#ifdef MOZILLA_INTERNAL_API
  if (!mFirstDecodeTime.IsNull()) {
    TimeDuration callDelta = TimeStamp::Now() - mFirstDecodeTime;
    if (callDelta.ToSeconds() != 0) {
      uint32_t recovered_per_min = mRecoveredBeforeLoss/(callDelta.ToSeconds()/60);
      CSFLogError(logTag, "Video recovery before error per min %f", recovered_per_min);
      Telemetry::Accumulate(Telemetry::WEBRTC_VIDEO_RECOVERY_BEFORE_ERROR_PER_MIN,
                            recovered_per_min);
      uint32_t err_per_min = mRecoveredLosses/(callDelta.ToSeconds()/60);
      CSFLogError(logTag, "Video recovery after error per min %u", err_per_min);
      Telemetry::Accumulate(Telemetry::WEBRTC_VIDEO_RECOVERY_AFTER_ERROR_PER_MIN,
                            err_per_min);
      float percent = (mTotalLossTime.ToSeconds()*100)/callDelta.ToSeconds();
      CSFLogError(logTag, "Video error time percentage %f%%", percent);
      Telemetry::Accumulate(Telemetry::WEBRTC_VIDEO_DECODE_ERROR_TIME_PERMILLE,
                            static_cast<uint32_t>(percent*10));
    }
  }
#endif
}

void VideoCodecStatistics::SentFrame()
{
  mSentRawFrames++;
}

void VideoCodecStatistics::Dump()
{
  Dump(mEncoderBitRate, "encoder bitrate");
  Dump(mEncoderFps, "encoder fps");
  Dump(mDecoderBitRate, "decoder bitrate");
  Dump(mDecoderFps, "decoder fps");
}

void VideoCodecStatistics::Dump(RunningStat& s, const char *name)
{
  CSFLogDebug(logTag,
              "%s, mean: %f, variance: %f, standard deviation: %f",
              name, s.Mean(), s.Variance(), s.StandardDeviation());
}
