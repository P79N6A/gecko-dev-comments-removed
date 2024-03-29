




#include "AudioSink.h"
#include "MediaDecoderStateMachine.h"
#include "AudioStream.h"
#include "prenv.h"

namespace mozilla {

extern PRLogModuleInfo* gMediaDecoderLog;
#define SINK_LOG(msg, ...) \
  MOZ_LOG(gMediaDecoderLog, LogLevel::Debug, ("AudioSink=%p " msg, this, ##__VA_ARGS__))
#define SINK_LOG_V(msg, ...) \
  MOZ_LOG(gMediaDecoderLog, LogLevel::Verbose, ("AudioSink=%p " msg, this, ##__VA_ARGS__))


static const int64_t AUDIO_FUZZ_FRAMES = 1;

AudioSink::AudioSink(MediaDecoderStateMachine* aStateMachine,
                     int64_t aStartTime, AudioInfo aInfo, dom::AudioChannel aChannel)
  : mStateMachine(aStateMachine)
  , mStartTime(aStartTime)
  , mWritten(0)
  , mLastGoodPosition(0)
  , mInfo(aInfo)
  , mChannel(aChannel)
  , mVolume(1.0)
  , mPlaybackRate(1.0)
  , mPreservesPitch(false)
  , mStopAudioThread(false)
  , mSetVolume(false)
  , mSetPlaybackRate(false)
  , mSetPreservesPitch(false)
  , mPlaying(true)
{
}

nsresult
AudioSink::Init()
{
  nsresult rv = NS_NewNamedThread("Media Audio",
                                  getter_AddRefs(mThread),
                                  nullptr,
                                  MEDIA_THREAD_STACK_SIZE);
  if (NS_FAILED(rv)) {
    mStateMachine->OnAudioSinkError();
    return rv;
  }

  nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(this, &AudioSink::AudioLoop);
  rv =  mThread->Dispatch(event, NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    mStateMachine->OnAudioSinkError();
    return rv;
  }

  return NS_OK;
}

int64_t
AudioSink::GetPosition()
{
  AssertCurrentThreadInMonitor();

  int64_t pos;
  if (mAudioStream &&
      (pos = mAudioStream->GetPosition()) >= 0) {
    
    mLastGoodPosition = pos;
  }

  return mStartTime + mLastGoodPosition;
}

bool
AudioSink::HasUnplayedFrames()
{
  AssertCurrentThreadInMonitor();
  
  
  return mAudioStream && mAudioStream->GetPositionInFrames() + 1 < mWritten;
}

void
AudioSink::PrepareToShutdown()
{
  AssertCurrentThreadInMonitor();
  mStopAudioThread = true;
  if (mAudioStream) {
    mAudioStream->Cancel();
  }
  GetReentrantMonitor().NotifyAll();
}

void
AudioSink::Shutdown()
{
  mThread->Shutdown();
  mThread = nullptr;
  MOZ_ASSERT(!mAudioStream);
}

void
AudioSink::SetVolume(double aVolume)
{
  AssertCurrentThreadInMonitor();
  mVolume = aVolume;
  mSetVolume = true;
}

void
AudioSink::SetPlaybackRate(double aPlaybackRate)
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(mPlaybackRate != 0, "Don't set the playbackRate to 0 on AudioStream");
  mPlaybackRate = aPlaybackRate;
  mSetPlaybackRate = true;
}

void
AudioSink::SetPreservesPitch(bool aPreservesPitch)
{
  AssertCurrentThreadInMonitor();
  mPreservesPitch = aPreservesPitch;
  mSetPreservesPitch = true;
}

void
AudioSink::SetPlaying(bool aPlaying)
{
  AssertCurrentThreadInMonitor();
  mPlaying = aPlaying;
  GetReentrantMonitor().NotifyAll();
}

void
AudioSink::AudioLoop()
{
  AssertOnAudioThread();
  SINK_LOG("AudioLoop started");

  if (NS_FAILED(InitializeAudioStream())) {
    NS_WARNING("Initializing AudioStream failed.");
    mStateMachine->DispatchOnAudioSinkError();
    return;
  }

  while (1) {
    {
      ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
      WaitForAudioToPlay();
      if (!IsPlaybackContinuing()) {
        break;
      }
    }
    
    
    
    
    NS_ASSERTION(AudioQueue().GetSize() > 0, "Should have data to play");
    CheckedInt64 sampleTime = UsecsToFrames(AudioQueue().PeekFront()->mTime, mInfo.mRate);

    
    CheckedInt64 playedFrames = UsecsToFrames(mStartTime, mInfo.mRate) +
                                static_cast<int64_t>(mWritten);

    CheckedInt64 missingFrames = sampleTime - playedFrames;
    if (!missingFrames.isValid() || !sampleTime.isValid()) {
      NS_WARNING("Int overflow adding in AudioLoop");
      break;
    }

    if (missingFrames.value() > AUDIO_FUZZ_FRAMES) {
      
      
      
      
      missingFrames = std::min<int64_t>(UINT32_MAX, missingFrames.value());
      mWritten += PlaySilence(static_cast<uint32_t>(missingFrames.value()));
    } else {
      mWritten += PlayFromAudioQueue();
    }
  }
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  MOZ_ASSERT(mStopAudioThread || AudioQueue().AtEndOfStream());
  if (!mStopAudioThread && mPlaying) {
    Drain();
  }
  SINK_LOG("AudioLoop complete");
  Cleanup();
  SINK_LOG("AudioLoop exit");
}

nsresult
AudioSink::InitializeAudioStream()
{
  
  
  
  RefPtr<AudioStream> audioStream(new AudioStream());
  nsresult rv = audioStream->Init(mInfo.mChannels, mInfo.mRate,
                                  mChannel, AudioStream::HighLatency);
  if (NS_FAILED(rv)) {
    audioStream->Shutdown();
    return rv;
  }

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  mAudioStream = audioStream;
  UpdateStreamSettings();

  return NS_OK;
}

void
AudioSink::Drain()
{
  MOZ_ASSERT(mPlaying && !mAudioStream->IsPaused());
  AssertCurrentThreadInMonitor();
  
  
  mAudioStream->Start();
  {
    ReentrantMonitorAutoExit exit(GetReentrantMonitor());
    mAudioStream->Drain();
  }
}

void
AudioSink::Cleanup()
{
  AssertCurrentThreadInMonitor();
  nsRefPtr<AudioStream> audioStream;
  audioStream.swap(mAudioStream);
  
  
  if (!mStopAudioThread) {
    mStateMachine->DispatchOnAudioSinkComplete();
  }

  ReentrantMonitorAutoExit exit(GetReentrantMonitor());
  audioStream->Shutdown();
}

bool
AudioSink::ExpectMoreAudioData()
{
  return AudioQueue().GetSize() == 0 && !AudioQueue().IsFinished();
}

void
AudioSink::WaitForAudioToPlay()
{
  
  
  AssertCurrentThreadInMonitor();
  while (!mStopAudioThread && (!mPlaying || ExpectMoreAudioData())) {
    if (!mPlaying && !mAudioStream->IsPaused()) {
      mAudioStream->Pause();
    }
    GetReentrantMonitor().Wait();
  }
}

bool
AudioSink::IsPlaybackContinuing()
{
  AssertCurrentThreadInMonitor();
  if (mPlaying && mAudioStream->IsPaused()) {
    mAudioStream->Resume();
  }

  
  
  if (mStopAudioThread || AudioQueue().AtEndOfStream()) {
    return false;
  }

  UpdateStreamSettings();

  return true;
}

uint32_t
AudioSink::PlaySilence(uint32_t aFrames)
{
  
  
  
  
  
  const uint32_t SILENCE_BYTES_CHUNK = 32 * 1024;

  AssertOnAudioThread();
  NS_ASSERTION(!mAudioStream->IsPaused(), "Don't play when paused");
  uint32_t maxFrames = SILENCE_BYTES_CHUNK / mInfo.mChannels / sizeof(AudioDataValue);
  uint32_t frames = std::min(aFrames, maxFrames);
  SINK_LOG_V("playing %u frames of silence", aFrames);
  WriteSilence(frames);
  return frames;
}

uint32_t
AudioSink::PlayFromAudioQueue()
{
  AssertOnAudioThread();
  NS_ASSERTION(!mAudioStream->IsPaused(), "Don't play when paused");
  nsRefPtr<AudioData> audio(AudioQueue().PopFront());

  SINK_LOG_V("playing %u frames of audio at time %lld",
             audio->mFrames, audio->mTime);
  if (audio->mRate == mInfo.mRate && audio->mChannels == mInfo.mChannels) {
    mAudioStream->Write(audio->mAudioData, audio->mFrames);
  } else {
    SINK_LOG_V("mismatched sample format mInfo=[%uHz/%u channels] audio=[%uHz/%u channels]",
               mInfo.mRate, mInfo.mChannels, audio->mRate, audio->mChannels);
    PlaySilence(audio->mFrames);
  }

  StartAudioStreamPlaybackIfNeeded();

  if (audio->mOffset != -1) {
    mStateMachine->DispatchOnPlaybackOffsetUpdate(audio->mOffset);
  }
  return audio->mFrames;
}

void
AudioSink::UpdateStreamSettings()
{
  AssertCurrentThreadInMonitor();

  bool setVolume = mSetVolume;
  bool setPlaybackRate = mSetPlaybackRate;
  bool setPreservesPitch = mSetPreservesPitch;
  double volume = mVolume;
  double playbackRate = mPlaybackRate;
  bool preservesPitch = mPreservesPitch;

  mSetVolume = false;
  mSetPlaybackRate = false;
  mSetPreservesPitch = false;

  {
    ReentrantMonitorAutoExit exit(GetReentrantMonitor());
    if (setVolume) {
      mAudioStream->SetVolume(volume);
    }

    if (setPlaybackRate &&
        NS_FAILED(mAudioStream->SetPlaybackRate(playbackRate))) {
      NS_WARNING("Setting the playback rate failed in AudioSink.");
    }

    if (setPreservesPitch &&
      NS_FAILED(mAudioStream->SetPreservesPitch(preservesPitch))) {
      NS_WARNING("Setting the pitch preservation failed in AudioSink.");
    }
  }
}

void
AudioSink::StartAudioStreamPlaybackIfNeeded()
{
  
  const uint32_t MIN_WRITE_BEFORE_START_USECS = 200000;

  
  if (static_cast<double>(mAudioStream->GetWritten()) / mAudioStream->GetRate() >=
      static_cast<double>(MIN_WRITE_BEFORE_START_USECS) / USECS_PER_S) {
    mAudioStream->Start();
  }
}

void
AudioSink::WriteSilence(uint32_t aFrames)
{
  uint32_t numSamples = aFrames * mInfo.mChannels;
  nsAutoTArray<AudioDataValue, 1000> buf;
  buf.SetLength(numSamples);
  memset(buf.Elements(), 0, numSamples * sizeof(AudioDataValue));
  mAudioStream->Write(buf.Elements(), aFrames);

  StartAudioStreamPlaybackIfNeeded();
}

int64_t
AudioSink::GetEndTime() const
{
  CheckedInt64 playedUsecs = FramesToUsecs(mWritten, mInfo.mRate) + mStartTime;
  if (!playedUsecs.isValid()) {
    NS_WARNING("Int overflow calculating audio end time");
    return -1;
  }
  return playedUsecs.value();
}

MediaQueue<AudioData>&
AudioSink::AudioQueue()
{
  return mStateMachine->AudioQueue();
}

ReentrantMonitor&
AudioSink::GetReentrantMonitor()
{
  return mStateMachine->mDecoder->GetReentrantMonitor();
}

void
AudioSink::AssertCurrentThreadInMonitor()
{
  return mStateMachine->AssertCurrentThreadInMonitor();
}

void
AudioSink::AssertOnAudioThread()
{
  MOZ_ASSERT(IsCurrentThread(mThread));
}

} 
