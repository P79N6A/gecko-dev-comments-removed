





#include "DecodedStream.h"
#include "MediaStreamGraph.h"
#include "AudioSegment.h"
#include "VideoSegment.h"
#include "MediaQueue.h"
#include "MediaData.h"
#include "MediaInfo.h"
#include "SharedBuffer.h"
#include "VideoUtils.h"

namespace mozilla {

class DecodedStreamGraphListener : public MediaStreamListener {
  typedef MediaStreamListener::MediaStreamGraphEvent MediaStreamGraphEvent;
public:
  explicit DecodedStreamGraphListener(MediaStream* aStream)
    : mMutex("DecodedStreamGraphListener::mMutex")
    , mStream(aStream)
    , mLastOutputTime(aStream->StreamTimeToMicroseconds(aStream->GetCurrentTime()))
    , mStreamFinishedOnMainThread(false) {}

  void NotifyOutput(MediaStreamGraph* aGraph, GraphTime aCurrentTime) override
  {
    MutexAutoLock lock(mMutex);
    if (mStream) {
      mLastOutputTime = mStream->StreamTimeToMicroseconds(mStream->GraphTimeToStreamTime(aCurrentTime));
    }
  }

  void NotifyEvent(MediaStreamGraph* aGraph, MediaStreamGraphEvent event) override
  {
    if (event == EVENT_FINISHED) {
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(this, &DecodedStreamGraphListener::DoNotifyFinished);
      aGraph->DispatchToMainThreadAfterStreamStateUpdate(event.forget());
    }
  }

  void DoNotifyFinished()
  {
    MutexAutoLock lock(mMutex);
    mStreamFinishedOnMainThread = true;
  }

  int64_t GetLastOutputTime()
  {
    MutexAutoLock lock(mMutex);
    return mLastOutputTime;
  }

  void Forget()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MutexAutoLock lock(mMutex);
    mStream = nullptr;
  }

  bool IsFinishedOnMainThread()
  {
    MutexAutoLock lock(mMutex);
    return mStreamFinishedOnMainThread;
  }

private:
  Mutex mMutex;
  
  nsRefPtr<MediaStream> mStream;
  int64_t mLastOutputTime; 
  bool mStreamFinishedOnMainThread;
};

static void
UpdateStreamBlocking(MediaStream* aStream, bool aBlocking)
{
  int32_t delta = aBlocking ? 1 : -1;
  if (NS_IsMainThread()) {
    aStream->ChangeExplicitBlockerCount(delta);
  } else {
    nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethodWithArg<int32_t>(
      aStream, &MediaStream::ChangeExplicitBlockerCount, delta);
    AbstractThread::MainThread()->Dispatch(r.forget());
  }
}

DecodedStreamData::DecodedStreamData(SourceMediaStream* aStream)
  : mAudioFramesWritten(0)
  , mNextVideoTime(-1)
  , mNextAudioTime(-1)
  , mStreamInitialized(false)
  , mHaveSentFinish(false)
  , mHaveSentFinishAudio(false)
  , mHaveSentFinishVideo(false)
  , mStream(aStream)
  , mPlaying(false)
  , mEOSVideoCompensation(false)
{
  mListener = new DecodedStreamGraphListener(mStream);
  mStream->AddListener(mListener);
  
  UpdateStreamBlocking(mStream, true);
}

DecodedStreamData::~DecodedStreamData()
{
  mListener->Forget();
  mStream->Destroy();
}

bool
DecodedStreamData::IsFinished() const
{
  return mListener->IsFinishedOnMainThread();
}

int64_t
DecodedStreamData::GetPosition() const
{
  return mListener->GetLastOutputTime();
}

void
DecodedStreamData::SetPlaying(bool aPlaying)
{
  if (mPlaying != aPlaying) {
    mPlaying = aPlaying;
    UpdateStreamBlocking(mStream, !mPlaying);
  }
}

class OutputStreamListener : public MediaStreamListener {
  typedef MediaStreamListener::MediaStreamGraphEvent MediaStreamGraphEvent;
public:
  OutputStreamListener(DecodedStream* aDecodedStream, MediaStream* aStream)
    : mDecodedStream(aDecodedStream), mStream(aStream) {}

  void NotifyEvent(MediaStreamGraph* aGraph, MediaStreamGraphEvent event) override
  {
    if (event == EVENT_FINISHED) {
      nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethod(
        this, &OutputStreamListener::DoNotifyFinished);
      aGraph->DispatchToMainThreadAfterStreamStateUpdate(r.forget());
    }
  }

  void Forget()
  {
    MOZ_ASSERT(NS_IsMainThread());
    mDecodedStream = nullptr;
  }

private:
  void DoNotifyFinished()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (mDecodedStream) {
      
      mDecodedStream->Remove(mStream);
    }
  }

  
  DecodedStream* mDecodedStream;
  nsRefPtr<MediaStream> mStream;
};

OutputStreamData::~OutputStreamData()
{
  mListener->Forget();
}

void
OutputStreamData::Init(DecodedStream* aDecodedStream, ProcessedMediaStream* aStream)
{
  mStream = aStream;
  mListener = new OutputStreamListener(aDecodedStream, aStream);
  aStream->AddListener(mListener);
}

DecodedStream::DecodedStream()
  : mMonitor("DecodedStream::mMonitor")
{
  
}

DecodedStreamData*
DecodedStream::GetData() const
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  return mData.get();
}

void
DecodedStream::DestroyData()
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  
  if (!mData) {
    return;
  }

  
  
  auto& outputStreams = OutputStreams();
  for (int32_t i = outputStreams.Length() - 1; i >= 0; --i) {
    OutputStreamData& os = outputStreams[i];
    
    
    MOZ_ASSERT(os.mPort, "Double-delete of the ports!");
    os.mPort->Destroy();
    os.mPort = nullptr;
    
    
    
    if (os.mStream->IsDestroyed()) {
      
      outputStreams.RemoveElementAt(i);
    } else {
      os.mStream->ChangeExplicitBlockerCount(1);
    }
  }

  mData = nullptr;
}

void
DecodedStream::RecreateData(MediaStreamGraph* aGraph)
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  MOZ_ASSERT((aGraph && !mData && OutputStreams().IsEmpty()) || 
             (!aGraph && mData)); 

  if (!aGraph) {
    aGraph = mData->mStream->Graph();
  }
  auto source = aGraph->CreateSourceStream(nullptr);
  DestroyData();
  mData.reset(new DecodedStreamData(source));

  
  
  
  auto& outputStreams = OutputStreams();
  for (int32_t i = outputStreams.Length() - 1; i >= 0; --i) {
    OutputStreamData& os = outputStreams[i];
    MOZ_ASSERT(!os.mStream->IsDestroyed(), "Should've been removed in DestroyData()");
    Connect(&os);
  }
}

nsTArray<OutputStreamData>&
DecodedStream::OutputStreams()
{
  MOZ_ASSERT(NS_IsMainThread());
  GetReentrantMonitor().AssertCurrentThreadIn();
  return mOutputStreams;
}

ReentrantMonitor&
DecodedStream::GetReentrantMonitor() const
{
  return mMonitor;
}

void
DecodedStream::Connect(OutputStreamData* aStream)
{
  MOZ_ASSERT(NS_IsMainThread());
  GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(!aStream->mPort, "Already connected?");

  
  
  aStream->mPort = aStream->mStream->AllocateInputPort(mData->mStream,
      MediaInputPort::FLAG_BLOCK_INPUT | MediaInputPort::FLAG_BLOCK_OUTPUT);
  
  
  aStream->mStream->ChangeExplicitBlockerCount(-1);
}

void
DecodedStream::Connect(ProcessedMediaStream* aStream, bool aFinishWhenEnded)
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  OutputStreamData* os = OutputStreams().AppendElement();
  os->Init(this, aStream);
  Connect(os);
  if (aFinishWhenEnded) {
    
    aStream->SetAutofinish(true);
  }
}

void
DecodedStream::Remove(MediaStream* aStream)
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  auto& streams = OutputStreams();
  for (int32_t i = streams.Length() - 1; i >= 0; --i) {
    auto& os = streams[i];
    MediaStream* p = os.mStream.get();
    if (p == aStream) {
      if (os.mPort) {
        os.mPort->Destroy();
        os.mPort = nullptr;
      }
      streams.RemoveElementAt(i);
      break;
    }
  }
}

void
DecodedStream::SetPlaying(bool aPlaying)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  MOZ_ASSERT(mData);
  mData->SetPlaying(aPlaying);
}

bool
DecodedStream::HaveEnoughAudio(const MediaInfo& aInfo) const
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  if (mData->mStreamInitialized && !mData->mHaveSentFinishAudio) {
    MOZ_ASSERT(aInfo.HasAudio());
    TrackID audioTrackId = aInfo.mAudio.mTrackId;
    if (!mData->mStream->HaveEnoughBuffered(audioTrackId)) {
      return false;
    }
  }
  return true;
}

bool
DecodedStream::HaveEnoughVideo(const MediaInfo& aInfo) const
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  if (mData->mStreamInitialized && !mData->mHaveSentFinishVideo) {
    MOZ_ASSERT(aInfo.HasVideo());
    TrackID videoTrackId = aInfo.mVideo.mTrackId;
    if (!mData->mStream->HaveEnoughBuffered(videoTrackId)) {
      return false;
    }
  }
  return true;
}

void
DecodedStream::InitTracks(int64_t aStartTime, const MediaInfo& aInfo)
{
  GetReentrantMonitor().AssertCurrentThreadIn();

  if (mData->mStreamInitialized) {
    return;
  }

  SourceMediaStream* sourceStream = mData->mStream;

  if (aInfo.HasAudio()) {
    TrackID audioTrackId = aInfo.mAudio.mTrackId;
    AudioSegment* audio = new AudioSegment();
    sourceStream->AddAudioTrack(audioTrackId, aInfo.mAudio.mRate, 0, audio,
                                SourceMediaStream::ADDTRACK_QUEUED);
    mData->mNextAudioTime = aStartTime;
  }

  if (aInfo.HasVideo()) {
    TrackID videoTrackId = aInfo.mVideo.mTrackId;
    VideoSegment* video = new VideoSegment();
    sourceStream->AddTrack(videoTrackId, 0, video,
                           SourceMediaStream::ADDTRACK_QUEUED);
    mData->mNextVideoTime = aStartTime;
  }

  sourceStream->FinishAddTracks();
  mData->mStreamInitialized = true;
}

static void
SendStreamAudio(DecodedStreamData* aStream, int64_t aStartTime,
                AudioData* aAudio, AudioSegment* aOutput,
                uint32_t aRate, double aVolume)
{
  
  
  CheckedInt64 audioWrittenOffset = aStream->mAudioFramesWritten +
                                    UsecsToFrames(aStartTime, aRate);
  CheckedInt64 frameOffset = UsecsToFrames(aAudio->mTime, aRate);

  if (!audioWrittenOffset.isValid() ||
      !frameOffset.isValid() ||
      
      frameOffset.value() + aAudio->mFrames <= audioWrittenOffset.value()) {
    return;
  }

  if (audioWrittenOffset.value() < frameOffset.value()) {
    int64_t silentFrames = frameOffset.value() - audioWrittenOffset.value();
    
    AudioSegment silence;
    silence.InsertNullDataAtStart(silentFrames);
    aStream->mAudioFramesWritten += silentFrames;
    audioWrittenOffset += silentFrames;
    aOutput->AppendFrom(&silence);
  }

  MOZ_ASSERT(audioWrittenOffset.value() >= frameOffset.value());

  int64_t offset = audioWrittenOffset.value() - frameOffset.value();
  size_t framesToWrite = aAudio->mFrames - offset;

  aAudio->EnsureAudioBuffer();
  nsRefPtr<SharedBuffer> buffer = aAudio->mAudioBuffer;
  AudioDataValue* bufferData = static_cast<AudioDataValue*>(buffer->Data());
  nsAutoTArray<const AudioDataValue*, 2> channels;
  for (uint32_t i = 0; i < aAudio->mChannels; ++i) {
    channels.AppendElement(bufferData + i * aAudio->mFrames + offset);
  }
  aOutput->AppendFrames(buffer.forget(), channels, framesToWrite);
  aStream->mAudioFramesWritten += framesToWrite;
  aOutput->ApplyVolume(aVolume);

  aStream->mNextAudioTime = aAudio->GetEndTime();
}

void
DecodedStream::SendAudio(int64_t aStartTime,
                         const MediaInfo& aInfo,
                         MediaQueue<AudioData>& aQueue,
                         double aVolume, bool aIsSameOrigin)
{
  GetReentrantMonitor().AssertCurrentThreadIn();

  if (!aInfo.HasAudio()) {
    return;
  }

  AudioSegment output;
  uint32_t rate = aInfo.mAudio.mRate;
  nsAutoTArray<nsRefPtr<AudioData>,10> audio;
  TrackID audioTrackId = aInfo.mAudio.mTrackId;
  SourceMediaStream* sourceStream = mData->mStream;

  
  
  aQueue.GetElementsAfter(mData->mNextAudioTime, &audio);
  for (uint32_t i = 0; i < audio.Length(); ++i) {
    SendStreamAudio(mData.get(), aStartTime, audio[i], &output, rate, aVolume);
  }

  if (!aIsSameOrigin) {
    output.ReplaceWithDisabled();
  }

  
  
  
  if (output.GetDuration() > 0) {
    sourceStream->AppendToTrack(audioTrackId, &output);
  }

  if (aQueue.IsFinished() && !mData->mHaveSentFinishAudio) {
    sourceStream->EndTrack(audioTrackId);
    mData->mHaveSentFinishAudio = true;
  }
}

static void
WriteVideoToMediaStream(MediaStream* aStream,
                        layers::Image* aImage,
                        int64_t aEndMicroseconds,
                        int64_t aStartMicroseconds,
                        const mozilla::gfx::IntSize& aIntrinsicSize,
                        VideoSegment* aOutput)
{
  nsRefPtr<layers::Image> image = aImage;
  StreamTime duration =
      aStream->MicrosecondsToStreamTimeRoundDown(aEndMicroseconds) -
      aStream->MicrosecondsToStreamTimeRoundDown(aStartMicroseconds);
  aOutput->AppendFrame(image.forget(), duration, aIntrinsicSize);
}

static bool
ZeroDurationAtLastChunk(VideoSegment& aInput)
{
  
  
  
  StreamTime lastVideoStratTime;
  aInput.GetLastFrame(&lastVideoStratTime);
  return lastVideoStratTime == aInput.GetDuration();
}

void
DecodedStream::SendVideo(int64_t aStartTime,
                         const MediaInfo& aInfo,
                         MediaQueue<VideoData>& aQueue,
                         bool aIsSameOrigin)
{
  GetReentrantMonitor().AssertCurrentThreadIn();

  if (!aInfo.HasVideo()) {
    return;
  }

  VideoSegment output;
  TrackID videoTrackId = aInfo.mVideo.mTrackId;
  nsAutoTArray<nsRefPtr<VideoData>, 10> video;
  SourceMediaStream* sourceStream = mData->mStream;

  
  
  aQueue.GetElementsAfter(mData->mNextVideoTime, &video);

  for (uint32_t i = 0; i < video.Length(); ++i) {
    VideoData* v = video[i];

    if (mData->mNextVideoTime < v->mTime) {
      
      

      
      
      
      
      
      
      WriteVideoToMediaStream(sourceStream, mData->mLastVideoImage, v->mTime,
          mData->mNextVideoTime, mData->mLastVideoImageDisplaySize, &output);
      mData->mNextVideoTime = v->mTime;
    }

    if (mData->mNextVideoTime < v->GetEndTime()) {
      WriteVideoToMediaStream(sourceStream, v->mImage,
          v->GetEndTime(), mData->mNextVideoTime, v->mDisplay, &output);
      mData->mNextVideoTime = v->GetEndTime();
      mData->mLastVideoImage = v->mImage;
      mData->mLastVideoImageDisplaySize = v->mDisplay;
    }
  }

  
  if (output.GetLastFrame()) {
    mData->mEOSVideoCompensation = ZeroDurationAtLastChunk(output);
  }

  if (!aIsSameOrigin) {
    output.ReplaceWithDisabled();
  }

  if (output.GetDuration() > 0) {
    sourceStream->AppendToTrack(videoTrackId, &output);
  }

  if (aQueue.IsFinished() && !mData->mHaveSentFinishVideo) {
    if (mData->mEOSVideoCompensation) {
      VideoSegment endSegment;
      
      int64_t deviation_usec = sourceStream->StreamTimeToMicroseconds(1);
      WriteVideoToMediaStream(sourceStream, mData->mLastVideoImage,
          mData->mNextVideoTime + deviation_usec, mData->mNextVideoTime,
          mData->mLastVideoImageDisplaySize, &endSegment);
      mData->mNextVideoTime += deviation_usec;
      MOZ_ASSERT(endSegment.GetDuration() > 0);
      if (!aIsSameOrigin) {
        endSegment.ReplaceWithDisabled();
      }
      sourceStream->AppendToTrack(videoTrackId, &endSegment);
    }
    sourceStream->EndTrack(videoTrackId);
    mData->mHaveSentFinishVideo = true;
  }
}

void
DecodedStream::AdvanceTracks(int64_t aStartTime, const MediaInfo& aInfo)
{
  GetReentrantMonitor().AssertCurrentThreadIn();

  StreamTime endPosition = 0;

  if (aInfo.HasAudio()) {
    StreamTime audioEnd = mData->mStream->TicksToTimeRoundDown(
        aInfo.mAudio.mRate, mData->mAudioFramesWritten);
    endPosition = std::max(endPosition, audioEnd);
  }

  if (aInfo.HasVideo()) {
    StreamTime videoEnd = mData->mStream->MicrosecondsToStreamTimeRoundDown(
        mData->mNextVideoTime - aStartTime);
    endPosition = std::max(endPosition, videoEnd);
  }

  if (!mData->mHaveSentFinish) {
    mData->mStream->AdvanceKnownTracksTime(endPosition);
  }
}

bool
DecodedStream::SendData(int64_t aStartTime,
                        const MediaInfo& aInfo,
                        MediaQueue<AudioData>& aAudioQueue,
                        MediaQueue<VideoData>& aVideoQueue,
                        double aVolume, bool aIsSameOrigin)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  InitTracks(aStartTime, aInfo);
  SendAudio(aStartTime, aInfo, aAudioQueue, aVolume, aIsSameOrigin);
  SendVideo(aStartTime, aInfo, aVideoQueue, aIsSameOrigin);
  AdvanceTracks(aStartTime, aInfo);

  bool finished = (!aInfo.HasAudio() || aAudioQueue.IsFinished()) &&
                  (!aInfo.HasVideo() || aVideoQueue.IsFinished());

  if (finished && !mData->mHaveSentFinish) {
    mData->mHaveSentFinish = true;
    mData->mStream->Finish();
  }

  return finished;
}

CheckedInt64
DecodedStream::AudioEndTime(int64_t aStartTime, uint32_t aRate) const
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  return aStartTime + FramesToUsecs(mData->mAudioFramesWritten, aRate);
}

} 
