




#include <stdio.h>
#include <math.h>
#include "prlog.h"
#include "prdtoa.h"
#include "AudioStream.h"
#include "VideoUtils.h"
#include "mozilla/Monitor.h"
#include "mozilla/Mutex.h"
#include <algorithm>
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "soundtouch/SoundTouch.h"
#include "Latency.h"

namespace mozilla {

#ifdef LOG
#undef LOG
#endif

#ifdef PR_LOGGING
PRLogModuleInfo* gAudioStreamLog = nullptr;

#define LOG(x) PR_LOG(gAudioStreamLog, PR_LOG_DEBUG, x)
#else
#define LOG(x)
#endif







static int gDumpedAudioCount = 0;

#define PREF_VOLUME_SCALE "media.volume_scale"
#define PREF_CUBEB_LATENCY "media.cubeb_latency_ms"

static const uint32_t CUBEB_NORMAL_LATENCY_MS = 100;

StaticMutex AudioStream::sMutex;
cubeb* AudioStream::sCubebContext;
uint32_t AudioStream::sPreferredSampleRate;
double AudioStream::sVolumeScale;
uint32_t AudioStream::sCubebLatency;
bool AudioStream::sCubebLatencyPrefSet;

 void AudioStream::PrefChanged(const char* aPref, void* aClosure)
{
  if (strcmp(aPref, PREF_VOLUME_SCALE) == 0) {
    nsAdoptingString value = Preferences::GetString(aPref);
    StaticMutexAutoLock lock(sMutex);
    if (value.IsEmpty()) {
      sVolumeScale = 1.0;
    } else {
      NS_ConvertUTF16toUTF8 utf8(value);
      sVolumeScale = std::max<double>(0, PR_strtod(utf8.get(), nullptr));
    }
  } else if (strcmp(aPref, PREF_CUBEB_LATENCY) == 0) {
    
    
    
    sCubebLatencyPrefSet = Preferences::HasUserValue(aPref);
    uint32_t value = Preferences::GetUint(aPref, CUBEB_NORMAL_LATENCY_MS);
    StaticMutexAutoLock lock(sMutex);
    sCubebLatency = std::min<uint32_t>(std::max<uint32_t>(value, 1), 1000);
  }
}

 bool AudioStream::GetFirstStream()
{
  static bool sFirstStream = true;

  StaticMutexAutoLock lock(sMutex);
  bool result = sFirstStream;
  sFirstStream = false;
  return result;
}

 double AudioStream::GetVolumeScale()
{
  StaticMutexAutoLock lock(sMutex);
  return sVolumeScale;
}

 cubeb* AudioStream::GetCubebContext()
{
  StaticMutexAutoLock lock(sMutex);
  return GetCubebContextUnlocked();
}

 void AudioStream::InitPreferredSampleRate()
{
  StaticMutexAutoLock lock(sMutex);
  if (sPreferredSampleRate == 0 &&
      cubeb_get_preferred_sample_rate(GetCubebContextUnlocked(),
                                      &sPreferredSampleRate) != CUBEB_OK) {
    sPreferredSampleRate = 44100;
  }
}

 cubeb* AudioStream::GetCubebContextUnlocked()
{
  sMutex.AssertCurrentThreadOwns();
  if (sCubebContext ||
      cubeb_init(&sCubebContext, "AudioStream") == CUBEB_OK) {
    return sCubebContext;
  }
  NS_WARNING("cubeb_init failed");
  return nullptr;
}

 uint32_t AudioStream::GetCubebLatency()
{
  StaticMutexAutoLock lock(sMutex);
  return sCubebLatency;
}

 bool AudioStream::CubebLatencyPrefSet()
{
  StaticMutexAutoLock lock(sMutex);
  return sCubebLatencyPrefSet;
}

#if defined(__ANDROID__) && defined(MOZ_B2G)
static cubeb_stream_type ConvertChannelToCubebType(dom::AudioChannel aChannel)
{
  switch(aChannel) {
    case dom::AudioChannel::Normal:
      return CUBEB_STREAM_TYPE_SYSTEM;
    case dom::AudioChannel::Content:
      return CUBEB_STREAM_TYPE_MUSIC;
    case dom::AudioChannel::Notification:
      return CUBEB_STREAM_TYPE_NOTIFICATION;
    case dom::AudioChannel::Alarm:
      return CUBEB_STREAM_TYPE_ALARM;
    case dom::AudioChannel::Telephony:
      return CUBEB_STREAM_TYPE_VOICE_CALL;
    case dom::AudioChannel::Ringer:
      return CUBEB_STREAM_TYPE_RING;
    
    case dom::AudioChannel::Publicnotification:
    default:
      NS_ERROR("The value of AudioChannel is invalid");
      return CUBEB_STREAM_TYPE_MAX;
  }
}
#endif

AudioStream::AudioStream()
  : mMonitor("AudioStream")
  , mInRate(0)
  , mOutRate(0)
  , mChannels(0)
  , mOutChannels(0)
  , mWritten(0)
  , mAudioClock(MOZ_THIS_IN_INITIALIZER_LIST())
  , mLatencyRequest(HighLatency)
  , mReadPoint(0)
  , mWrittenFramesPast(0)
  , mLostFramesPast(0)
  , mWrittenFramesLast(0)
  , mLostFramesLast(0)
  , mDumpFile(nullptr)
  , mVolume(1.0)
  , mBytesPerFrame(0)
  , mState(INITIALIZED)
  , mNeedsStart(false)
{
  
  mLatencyLog = AsyncLatencyLogger::Get(true);
}

AudioStream::~AudioStream()
{
  LOG(("AudioStream: delete %p, state %d", this, mState));
  Shutdown();
  if (mDumpFile) {
    fclose(mDumpFile);
  }
}

size_t
AudioStream::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t amount = aMallocSizeOf(this);

  
  
  
  

  amount += mInserts.SizeOfExcludingThis(aMallocSizeOf);
  amount += mBuffer.SizeOfExcludingThis(aMallocSizeOf);

  return amount;
}

 void AudioStream::InitLibrary()
{
#ifdef PR_LOGGING
  gAudioStreamLog = PR_NewLogModule("AudioStream");
#endif
  PrefChanged(PREF_VOLUME_SCALE, nullptr);
  Preferences::RegisterCallback(PrefChanged, PREF_VOLUME_SCALE);
  PrefChanged(PREF_CUBEB_LATENCY, nullptr);
  Preferences::RegisterCallback(PrefChanged, PREF_CUBEB_LATENCY);
}

 void AudioStream::ShutdownLibrary()
{
  Preferences::UnregisterCallback(PrefChanged, PREF_VOLUME_SCALE);
  Preferences::UnregisterCallback(PrefChanged, PREF_CUBEB_LATENCY);

  StaticMutexAutoLock lock(sMutex);
  if (sCubebContext) {
    cubeb_destroy(sCubebContext);
    sCubebContext = nullptr;
  }
}

nsresult AudioStream::EnsureTimeStretcherInitializedUnlocked()
{
  mMonitor.AssertCurrentThreadOwns();
  if (!mTimeStretcher) {
    mTimeStretcher = new soundtouch::SoundTouch();
    mTimeStretcher->setSampleRate(mInRate);
    mTimeStretcher->setChannels(mOutChannels);
    mTimeStretcher->setPitch(1.0);
  }
  return NS_OK;
}

nsresult AudioStream::SetPlaybackRate(double aPlaybackRate)
{
  NS_ASSERTION(aPlaybackRate > 0.0,
               "Can't handle negative or null playbackrate in the AudioStream.");
  
  
  if (aPlaybackRate == mAudioClock.GetPlaybackRate()) {
    return NS_OK;
  }

  
  
  MonitorAutoLock mon(mMonitor);
  if (EnsureTimeStretcherInitializedUnlocked() != NS_OK) {
    return NS_ERROR_FAILURE;
  }

  mAudioClock.SetPlaybackRateUnlocked(aPlaybackRate);
  mOutRate = mInRate / aPlaybackRate;

  if (mAudioClock.GetPreservesPitch()) {
    mTimeStretcher->setTempo(aPlaybackRate);
    mTimeStretcher->setRate(1.0f);
  } else {
    mTimeStretcher->setTempo(1.0f);
    mTimeStretcher->setRate(aPlaybackRate);
  }
  return NS_OK;
}

nsresult AudioStream::SetPreservesPitch(bool aPreservesPitch)
{
  
  if (aPreservesPitch == mAudioClock.GetPreservesPitch()) {
    return NS_OK;
  }

  
  
  MonitorAutoLock mon(mMonitor);
  if (EnsureTimeStretcherInitializedUnlocked() != NS_OK) {
    return NS_ERROR_FAILURE;
  }

  if (aPreservesPitch == true) {
    mTimeStretcher->setTempo(mAudioClock.GetPlaybackRate());
    mTimeStretcher->setRate(1.0f);
  } else {
    mTimeStretcher->setTempo(1.0f);
    mTimeStretcher->setRate(mAudioClock.GetPlaybackRate());
  }

  mAudioClock.SetPreservesPitch(aPreservesPitch);

  return NS_OK;
}

int64_t AudioStream::GetWritten()
{
  return mWritten;
}

 int AudioStream::MaxNumberOfChannels()
{
  cubeb* cubebContext = GetCubebContext();
  uint32_t maxNumberOfChannels;
  if (cubebContext &&
      cubeb_get_max_channel_count(cubebContext,
                                  &maxNumberOfChannels) == CUBEB_OK) {
    return static_cast<int>(maxNumberOfChannels);
  }

  return 0;
}

 int AudioStream::PreferredSampleRate()
{
  MOZ_ASSERT(sPreferredSampleRate,
             "sPreferredSampleRate has not been initialized!");
  return sPreferredSampleRate;
}

static void SetUint16LE(uint8_t* aDest, uint16_t aValue)
{
  aDest[0] = aValue & 0xFF;
  aDest[1] = aValue >> 8;
}

static void SetUint32LE(uint8_t* aDest, uint32_t aValue)
{
  SetUint16LE(aDest, aValue & 0xFFFF);
  SetUint16LE(aDest + 2, aValue >> 16);
}

static FILE*
OpenDumpFile(AudioStream* aStream)
{
  if (!getenv("MOZ_DUMP_AUDIO"))
    return nullptr;
  char buf[100];
  sprintf(buf, "dumped-audio-%d.wav", gDumpedAudioCount);
  FILE* f = fopen(buf, "wb");
  if (!f)
    return nullptr;
  ++gDumpedAudioCount;

  uint8_t header[] = {
    
    0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45,
    
    0x66, 0x6d, 0x74, 0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x10, 0x00,
    
    0x64, 0x61, 0x74, 0x61, 0xFE, 0xFF, 0xFF, 0x7F
  };
  static const int CHANNEL_OFFSET = 22;
  static const int SAMPLE_RATE_OFFSET = 24;
  static const int BLOCK_ALIGN_OFFSET = 32;
  SetUint16LE(header + CHANNEL_OFFSET, aStream->GetChannels());
  SetUint32LE(header + SAMPLE_RATE_OFFSET, aStream->GetRate());
  SetUint16LE(header + BLOCK_ALIGN_OFFSET, aStream->GetChannels()*2);
  fwrite(header, sizeof(header), 1, f);

  return f;
}

static void
WriteDumpFile(FILE* aDumpFile, AudioStream* aStream, uint32_t aFrames,
              void* aBuffer)
{
  if (!aDumpFile)
    return;

  uint32_t samples = aStream->GetOutChannels()*aFrames;
  if (AUDIO_OUTPUT_FORMAT == AUDIO_FORMAT_S16) {
    fwrite(aBuffer, 2, samples, aDumpFile);
    return;
  }

  NS_ASSERTION(AUDIO_OUTPUT_FORMAT == AUDIO_FORMAT_FLOAT32, "bad format");
  nsAutoTArray<uint8_t, 1024*2> buf;
  buf.SetLength(samples*2);
  float* input = static_cast<float*>(aBuffer);
  uint8_t* output = buf.Elements();
  for (uint32_t i = 0; i < samples; ++i) {
    SetUint16LE(output + i*2, int16_t(input[i]*32767.0f));
  }
  fwrite(output, 2, samples, aDumpFile);
  fflush(aDumpFile);
}



nsresult
AudioStream::Init(int32_t aNumChannels, int32_t aRate,
                  const dom::AudioChannel aAudioChannel,
                  LatencyRequest aLatencyRequest)
{
  mStartTime = TimeStamp::Now();
  mIsFirst = GetFirstStream();

  if (!GetCubebContext() || aNumChannels < 0 || aRate < 0) {
    return NS_ERROR_FAILURE;
  }

  PR_LOG(gAudioStreamLog, PR_LOG_DEBUG,
    ("%s  channels: %d, rate: %d for %p", __FUNCTION__, aNumChannels, aRate, this));
  mInRate = mOutRate = aRate;
  mChannels = aNumChannels;
  mOutChannels = (aNumChannels > 2) ? 2 : aNumChannels;
  mLatencyRequest = aLatencyRequest;

  mDumpFile = OpenDumpFile(this);

  cubeb_stream_params params;
  params.rate = aRate;
  params.channels = mOutChannels;
#if defined(__ANDROID__)
#if defined(MOZ_B2G)
  params.stream_type = ConvertChannelToCubebType(aAudioChannel);
#else
  params.stream_type = CUBEB_STREAM_TYPE_MUSIC;
#endif

  if (params.stream_type == CUBEB_STREAM_TYPE_MAX) {
    return NS_ERROR_INVALID_ARG;
  }
#endif
  if (AUDIO_OUTPUT_FORMAT == AUDIO_FORMAT_S16) {
    params.format = CUBEB_SAMPLE_S16NE;
  } else {
    params.format = CUBEB_SAMPLE_FLOAT32NE;
  }
  mBytesPerFrame = sizeof(AudioDataValue) * mOutChannels;

  mAudioClock.Init();

  
  
  
  uint32_t bufferLimit = FramesToBytes(aRate);
  NS_ABORT_IF_FALSE(bufferLimit % mBytesPerFrame == 0, "Must buffer complete frames");
  mBuffer.SetCapacity(bufferLimit);

  if (aLatencyRequest == LowLatency) {
    
    
    
    
    
    RefPtr<AudioInitTask> init = new AudioInitTask(this, aLatencyRequest, params);
    init->Dispatch();
    return NS_OK;
  }
  
  nsresult rv = OpenCubeb(params, aLatencyRequest);
  
  
  CheckForStart();
  return rv;
}



nsresult
AudioStream::OpenCubeb(cubeb_stream_params &aParams,
                       LatencyRequest aLatencyRequest)
{
  cubeb* cubebContext = GetCubebContext();
  if (!cubebContext) {
    MonitorAutoLock mon(mMonitor);
    mState = AudioStream::ERRORED;
    return NS_ERROR_FAILURE;
  }

  
  
  
  uint32_t latency;
  if (aLatencyRequest == LowLatency && !CubebLatencyPrefSet()) {
    if (cubeb_get_min_latency(cubebContext, aParams, &latency) != CUBEB_OK) {
      latency = GetCubebLatency();
    }
  } else {
    latency = GetCubebLatency();
  }

  {
    cubeb_stream* stream;
    if (cubeb_stream_init(cubebContext, &stream, "AudioStream", aParams,
                          latency, DataCallback_S, StateCallback_S, this) == CUBEB_OK) {
      MonitorAutoLock mon(mMonitor);
      mCubebStream.own(stream);
      
      if (mState == SHUTDOWN) {
        mCubebStream.reset();
        LOG(("AudioStream::OpenCubeb() %p Shutdown while opening cubeb", this));
        return NS_ERROR_FAILURE;
      }

      
      
      
    } else {
      MonitorAutoLock mon(mMonitor);
      mState = ERRORED;
      LOG(("AudioStream::OpenCubeb() %p failed to init cubeb", this));
      return NS_ERROR_FAILURE;
    }
  }

  if (!mStartTime.IsNull()) {
		TimeDuration timeDelta = TimeStamp::Now() - mStartTime;
    LOG(("AudioStream creation time %sfirst: %u ms", mIsFirst ? "" : "not ",
         (uint32_t) timeDelta.ToMilliseconds()));
		Telemetry::Accumulate(mIsFirst ? Telemetry::AUDIOSTREAM_FIRST_OPEN_MS :
                          Telemetry::AUDIOSTREAM_LATER_OPEN_MS, timeDelta.ToMilliseconds());
  }

  return NS_OK;
}

void
AudioStream::CheckForStart()
{
  if (mState == INITIALIZED) {
    
    
    
    if (mLatencyRequest == LowLatency || mNeedsStart) {
      StartUnlocked(); 
      mNeedsStart = false;
      PR_LOG(gAudioStreamLog, PR_LOG_WARNING,
             ("Started waiting %s-latency stream",
              mLatencyRequest == LowLatency ? "low" : "high"));
    } else {
      
      PR_LOG(gAudioStreamLog, PR_LOG_DEBUG,
             ("Not starting waiting %s-latency stream",
              mLatencyRequest == LowLatency ? "low" : "high"));
    }
  }
}

NS_IMETHODIMP
AudioInitTask::Run()
{
  MOZ_ASSERT(mThread);
  if (NS_IsMainThread()) {
    mThread->Shutdown(); 
    
    
    
    
    
    return NS_OK;
  }

  nsresult rv = mAudioStream->OpenCubeb(mParams, mLatencyRequest);

  
  NS_DispatchToMainThread(this);
  return rv;
}


nsresult
AudioStream::Write(const AudioDataValue* aBuf, uint32_t aFrames, TimeStamp *aTime)
{
  MonitorAutoLock mon(mMonitor);
  if (mState == ERRORED) {
    return NS_ERROR_FAILURE;
  }
  NS_ASSERTION(mState == INITIALIZED || mState == STARTED || mState == RUNNING,
    "Stream write in unexpected state.");

  
  CheckForStart();

  
  if (mChannels > 2 && mChannels <= 8) {
    DownmixAudioToStereo(const_cast<AudioDataValue*> (aBuf), mChannels, aFrames);
  }
  else if (mChannels > 8) {
    return NS_ERROR_FAILURE;
  }

  const uint8_t* src = reinterpret_cast<const uint8_t*>(aBuf);
  uint32_t bytesToCopy = FramesToBytes(aFrames);

  
  if (PR_LOG_TEST(GetLatencyLog(), PR_LOG_DEBUG)) {
    
    int64_t timeMs;
    if (aTime && !aTime->IsNull()) {
      if (mStartTime.IsNull()) {
        AsyncLatencyLogger::Get(true)->GetStartTime(mStartTime);
      }
      timeMs = (*aTime - mStartTime).ToMilliseconds();
    } else {
      timeMs = 0;
    }
    struct Inserts insert = { timeMs, aFrames};
    mInserts.AppendElement(insert);
  }

  while (bytesToCopy > 0) {
    uint32_t available = std::min(bytesToCopy, mBuffer.Available());
    NS_ABORT_IF_FALSE(available % mBytesPerFrame == 0,
        "Must copy complete frames.");

    mBuffer.AppendElements(src, available);
    src += available;
    bytesToCopy -= available;

    if (bytesToCopy > 0) {
      
      if ((mState == INITIALIZED || mState == STARTED) && mLatencyRequest == LowLatency) {
        
        uint32_t remains = 0; 
        if (mBuffer.Length() > bytesToCopy) {
          remains = mBuffer.Length() - bytesToCopy; 
        }
        
        PR_LOG(gAudioStreamLog, PR_LOG_WARNING, ("Stream %p dropping %u bytes (%u frames)in Write()",
            this, mBuffer.Length() - remains, BytesToFrames(mBuffer.Length() - remains)));
        mReadPoint += BytesToFrames(mBuffer.Length() - remains);
        mBuffer.ContractTo(remains);
      } else { 
        
        
        if (mState != STARTED && mState != RUNNING) {
          PR_LOG(gAudioStreamLog, PR_LOG_WARNING, ("Starting stream %p in Write (%u waiting)",
                                                 this, bytesToCopy));
          StartUnlocked();
          if (mState == ERRORED) {
            return NS_ERROR_FAILURE;
          }
        }
        PR_LOG(gAudioStreamLog, PR_LOG_WARNING, ("Stream %p waiting in Write() (%u waiting)",
                                                 this, bytesToCopy));
        mon.Wait();
      }
    }
  }

  mWritten += aFrames;
  return NS_OK;
}

uint32_t
AudioStream::Available()
{
  MonitorAutoLock mon(mMonitor);
  NS_ABORT_IF_FALSE(mBuffer.Length() % mBytesPerFrame == 0, "Buffer invariant violated.");
  return BytesToFrames(mBuffer.Available());
}

void
AudioStream::SetVolume(double aVolume)
{
  MonitorAutoLock mon(mMonitor);
  NS_ABORT_IF_FALSE(aVolume >= 0.0 && aVolume <= 1.0, "Invalid volume");
  mVolume = aVolume;
}

void
AudioStream::Drain()
{
  MonitorAutoLock mon(mMonitor);
  LOG(("AudioStream::Drain() for %p, state %d, avail %u", this, mState, mBuffer.Available()));
  if (mState != STARTED && mState != RUNNING) {
    NS_ASSERTION(mState == ERRORED || mBuffer.Available() == 0, "Draining without full buffer of unplayed audio");
    return;
  }
  mState = DRAINING;
  while (mState == DRAINING) {
    mon.Wait();
  }
}

void
AudioStream::Start()
{
  MonitorAutoLock mon(mMonitor);
  StartUnlocked();
}

void
AudioStream::StartUnlocked()
{
  mMonitor.AssertCurrentThreadOwns();
  if (!mCubebStream) {
    mNeedsStart = true;
    return;
  }
  MonitorAutoUnlock mon(mMonitor);
  if (mState == INITIALIZED) {
    int r = cubeb_stream_start(mCubebStream);
    mState = r == CUBEB_OK ? STARTED : ERRORED;
    LOG(("AudioStream: started %p, state %s", this, mState == STARTED ? "STARTED" : "ERRORED"));
  }
}

void
AudioStream::Pause()
{
  MonitorAutoLock mon(mMonitor);
  if (!mCubebStream || (mState != STARTED && mState != RUNNING)) {
    mNeedsStart = false;
    mState = STOPPED; 
    return;
  }

  int r;
  {
    MonitorAutoUnlock mon(mMonitor);
    r = cubeb_stream_stop(mCubebStream);
  }
  if (mState != ERRORED && r == CUBEB_OK) {
    mState = STOPPED;
  }
}

void
AudioStream::Resume()
{
  MonitorAutoLock mon(mMonitor);
  if (!mCubebStream || mState != STOPPED) {
    return;
  }

  int r;
  {
    MonitorAutoUnlock mon(mMonitor);
    r = cubeb_stream_start(mCubebStream);
  }
  if (mState != ERRORED && r == CUBEB_OK) {
    mState = STARTED;
  }
}

void
AudioStream::Shutdown()
{
  LOG(("AudioStream: Shutdown %p, state %d", this, mState));
  {
    MonitorAutoLock mon(mMonitor);
    if (mState == STARTED || mState == RUNNING) {
      MonitorAutoUnlock mon(mMonitor);
      Pause();
    }
    MOZ_ASSERT(mState != STARTED && mState != RUNNING); 
    mState = SHUTDOWN;
  }
  
  
  if (mCubebStream) {
    mCubebStream.reset();
  }
}

int64_t
AudioStream::GetPosition()
{
  MonitorAutoLock mon(mMonitor);
  return mAudioClock.GetPositionUnlocked();
}


#ifdef _MSC_VER
#pragma optimize("", off)
#endif
int64_t
AudioStream::GetPositionInFrames()
{
  return mAudioClock.GetPositionInFrames();
}
#ifdef _MSC_VER
#pragma optimize("", on)
#endif

int64_t
AudioStream::GetPositionInFramesInternal()
{
  MonitorAutoLock mon(mMonitor);
  return GetPositionInFramesUnlocked();
}

int64_t
AudioStream::GetPositionInFramesUnlocked()
{
  mMonitor.AssertCurrentThreadOwns();

  if (!mCubebStream || mState == ERRORED) {
    return -1;
  }

  uint64_t position = 0;
  {
    MonitorAutoUnlock mon(mMonitor);
    if (cubeb_stream_get_position(mCubebStream, &position) != CUBEB_OK) {
      return -1;
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  uint64_t adjustedPosition = 0;
  if (position <= mWrittenFramesPast) {
    adjustedPosition = position;
  } else if (position <= mWrittenFramesPast + mLostFramesPast) {
    adjustedPosition = mWrittenFramesPast;
  } else if (position <= mWrittenFramesPast + mLostFramesPast + mWrittenFramesLast) {
    adjustedPosition = position - mLostFramesPast;
  } else {
    adjustedPosition = mWrittenFramesPast + mWrittenFramesLast;
  }
  return std::min<uint64_t>(adjustedPosition, INT64_MAX);
}

int64_t
AudioStream::GetLatencyInFrames()
{
  uint32_t latency;
  if (cubeb_stream_get_latency(mCubebStream, &latency)) {
    NS_WARNING("Could not get cubeb latency.");
    return 0;
  }
  return static_cast<int64_t>(latency);
}

bool
AudioStream::IsPaused()
{
  MonitorAutoLock mon(mMonitor);
  return mState == STOPPED;
}

void
AudioStream::GetBufferInsertTime(int64_t &aTimeMs)
{
  if (mInserts.Length() > 0) {
    
    while (mInserts.Length() > 1 && mReadPoint >= mInserts[0].mFrames) {
      mReadPoint -= mInserts[0].mFrames;
      mInserts.RemoveElementAt(0);
    }
    
    
    aTimeMs = mInserts[0].mTimeMs + ((mReadPoint * 1000) / mOutRate);
  } else {
    aTimeMs = INT64_MAX;
  }
}

long
AudioStream::GetUnprocessed(void* aBuffer, long aFrames, int64_t &aTimeMs)
{
  uint8_t* wpos = reinterpret_cast<uint8_t*>(aBuffer);

  
  
  uint32_t flushedFrames = 0;
  if (mTimeStretcher && mTimeStretcher->numSamples()) {
    flushedFrames = mTimeStretcher->receiveSamples(reinterpret_cast<AudioDataValue*>(wpos), aFrames);
    wpos += FramesToBytes(flushedFrames);
  }
  uint32_t toPopBytes = FramesToBytes(aFrames - flushedFrames);
  uint32_t available = std::min(toPopBytes, mBuffer.Length());

  void* input[2];
  uint32_t input_size[2];
  mBuffer.PopElements(available, &input[0], &input_size[0], &input[1], &input_size[1]);
  memcpy(wpos, input[0], input_size[0]);
  wpos += input_size[0];
  memcpy(wpos, input[1], input_size[1]);

  
  mReadPoint += BytesToFrames(available);
  GetBufferInsertTime(aTimeMs);

  return BytesToFrames(available) + flushedFrames;
}



long
AudioStream::GetUnprocessedWithSilencePadding(void* aBuffer, long aFrames, int64_t& aTimeMs)
{
  uint32_t toPopBytes = FramesToBytes(aFrames);
  uint32_t available = std::min(toPopBytes, mBuffer.Length());
  uint32_t silenceOffset = toPopBytes - available;

  uint8_t* wpos = reinterpret_cast<uint8_t*>(aBuffer);

  memset(wpos, 0, silenceOffset);
  wpos += silenceOffset;

  void* input[2];
  uint32_t input_size[2];
  mBuffer.PopElements(available, &input[0], &input_size[0], &input[1], &input_size[1]);
  memcpy(wpos, input[0], input_size[0]);
  wpos += input_size[0];
  memcpy(wpos, input[1], input_size[1]);

  GetBufferInsertTime(aTimeMs);

  return aFrames;
}

long
AudioStream::GetTimeStretched(void* aBuffer, long aFrames, int64_t &aTimeMs)
{
  long processedFrames = 0;

  
  if (EnsureTimeStretcherInitializedUnlocked() != NS_OK) {
    return 0;
  }

  uint8_t* wpos = reinterpret_cast<uint8_t*>(aBuffer);
  double playbackRate = static_cast<double>(mInRate) / mOutRate;
  uint32_t toPopBytes = FramesToBytes(ceil(aFrames / playbackRate));
  uint32_t available = 0;
  bool lowOnBufferedData = false;
  do {
    
    if (mTimeStretcher->numSamples() <= static_cast<uint32_t>(aFrames)) {
      void* input[2];
      uint32_t input_size[2];
      available = std::min(mBuffer.Length(), toPopBytes);
      if (available != toPopBytes) {
        lowOnBufferedData = true;
      }
      mBuffer.PopElements(available, &input[0], &input_size[0],
                                     &input[1], &input_size[1]);
      mReadPoint += BytesToFrames(available);
      for(uint32_t i = 0; i < 2; i++) {
        mTimeStretcher->putSamples(reinterpret_cast<AudioDataValue*>(input[i]), BytesToFrames(input_size[i]));
      }
    }
    uint32_t receivedFrames = mTimeStretcher->receiveSamples(reinterpret_cast<AudioDataValue*>(wpos), aFrames - processedFrames);
    wpos += FramesToBytes(receivedFrames);
    processedFrames += receivedFrames;
  } while (processedFrames < aFrames && !lowOnBufferedData);

  GetBufferInsertTime(aTimeMs);

  return processedFrames;
}

long
AudioStream::DataCallback(void* aBuffer, long aFrames)
{
  MonitorAutoLock mon(mMonitor);
  uint32_t available = std::min(static_cast<uint32_t>(FramesToBytes(aFrames)), mBuffer.Length());
  NS_ABORT_IF_FALSE(available % mBytesPerFrame == 0, "Must copy complete frames");
  AudioDataValue* output = reinterpret_cast<AudioDataValue*>(aBuffer);
  uint32_t underrunFrames = 0;
  uint32_t servicedFrames = 0;
  int64_t insertTime;

  mWrittenFramesPast += mWrittenFramesLast;
  mLostFramesPast += mLostFramesLast;

  
  

  
  if (mState == STARTED) {
    
    
    
    if (mLatencyRequest == LowLatency) {
#ifdef PR_LOGGING
      uint32_t old_len = mBuffer.Length();
#endif
      available = mBuffer.ContractTo(FramesToBytes(aFrames));
#ifdef PR_LOGGING
      TimeStamp now = TimeStamp::Now();
      if (!mStartTime.IsNull()) {
        int64_t timeMs = (now - mStartTime).ToMilliseconds();
        PR_LOG(gAudioStreamLog, PR_LOG_WARNING,
               ("Stream took %lldms to start after first Write() @ %u", timeMs, mOutRate));
      } else {
        PR_LOG(gAudioStreamLog, PR_LOG_WARNING,
          ("Stream started before Write() @ %u", mOutRate));
      }

      if (old_len != available) {
        
        PR_LOG(gAudioStreamLog, PR_LOG_WARNING,
               ("AudioStream %p dropped %u + %u initial frames @ %u", this,
                 mReadPoint, BytesToFrames(old_len - available), mOutRate));
        mReadPoint += BytesToFrames(old_len - available);
      }
#endif
    }
    mState = RUNNING;
  }

  if (available) {
    
    
    
    
    if (mInRate == mOutRate) {
      if (mLatencyRequest == LowLatency && !mWritten) {
        servicedFrames = GetUnprocessedWithSilencePadding(output, aFrames, insertTime);
      } else {
        servicedFrames = GetUnprocessed(output, aFrames, insertTime);
      }
    } else {
      servicedFrames = GetTimeStretched(output, aFrames, insertTime);
    }
    float scaled_volume = float(GetVolumeScale() * mVolume);

    ScaleAudioSamples(output, aFrames * mOutChannels, scaled_volume);

    NS_ABORT_IF_FALSE(mBuffer.Length() % mBytesPerFrame == 0, "Must copy complete frames");

    
    mon.NotifyAll();
  } else {
    GetBufferInsertTime(insertTime);
  }

  underrunFrames = aFrames - servicedFrames;
  mWrittenFramesLast = servicedFrames;
  mLostFramesLast = underrunFrames;

  if (mState != DRAINING) {
    uint8_t* rpos = static_cast<uint8_t*>(aBuffer) + FramesToBytes(aFrames - underrunFrames);
    memset(rpos, 0, FramesToBytes(underrunFrames));
    if (underrunFrames) {
      PR_LOG(gAudioStreamLog, PR_LOG_WARNING,
             ("AudioStream %p lost %d frames", this, underrunFrames));
    }
    servicedFrames += underrunFrames;
  }

  WriteDumpFile(mDumpFile, this, aFrames, aBuffer);
  
  if (PR_LOG_TEST(GetLatencyLog(), PR_LOG_DEBUG) &&
      mState != SHUTDOWN &&
      insertTime != INT64_MAX && servicedFrames > underrunFrames) {
    uint32_t latency = UINT32_MAX;
    if (cubeb_stream_get_latency(mCubebStream, &latency)) {
      NS_WARNING("Could not get latency from cubeb.");
    }
    TimeStamp now = TimeStamp::Now();

    mLatencyLog->Log(AsyncLatencyLogger::AudioStream, reinterpret_cast<uint64_t>(this),
                     insertTime, now);
    mLatencyLog->Log(AsyncLatencyLogger::Cubeb, reinterpret_cast<uint64_t>(mCubebStream.get()),
                     (latency * 1000) / mOutRate, now);
  }

  mAudioClock.UpdateWritePosition(servicedFrames);
  return servicedFrames;
}

void
AudioStream::StateCallback(cubeb_state aState)
{
  MonitorAutoLock mon(mMonitor);
  if (aState == CUBEB_STATE_DRAINED) {
    mState = DRAINED;
  } else if (aState == CUBEB_STATE_ERROR) {
    LOG(("AudioStream::StateCallback() state %d cubeb error", mState));
    mState = ERRORED;
  }
  mon.NotifyAll();
}

AudioClock::AudioClock(AudioStream* aStream)
 :mAudioStream(aStream),
  mOldOutRate(0),
  mBasePosition(0),
  mBaseOffset(0),
  mOldBaseOffset(0),
  mOldBasePosition(0),
  mPlaybackRateChangeOffset(0),
  mPreviousPosition(0),
  mWritten(0),
  mOutRate(0),
  mInRate(0),
  mPreservesPitch(true),
  mCompensatingLatency(false)
{}

void AudioClock::Init()
{
  mOutRate = mAudioStream->GetRate();
  mInRate = mAudioStream->GetRate();
  mOldOutRate = mOutRate;
}

void AudioClock::UpdateWritePosition(uint32_t aCount)
{
  mWritten += aCount;
}

uint64_t AudioClock::GetPositionUnlocked()
{
  
  int64_t position = mAudioStream->GetPositionInFramesUnlocked();
  int64_t diffOffset;
  NS_ASSERTION(position < 0 || (mInRate != 0 && mOutRate != 0), "AudioClock not initialized.");
  if (position >= 0) {
    if (position < mPlaybackRateChangeOffset) {
      
      
      
      mCompensatingLatency = true;
      diffOffset = position - mOldBaseOffset;
      position = static_cast<uint64_t>(mOldBasePosition +
        static_cast<float>(USECS_PER_S * diffOffset) / mOldOutRate);
      mPreviousPosition = position;
      return position;
    }

    if (mCompensatingLatency) {
      diffOffset = position - mPlaybackRateChangeOffset;
      mCompensatingLatency = false;
      mBasePosition = mPreviousPosition;
    } else {
      diffOffset = position - mPlaybackRateChangeOffset;
    }
    position =  static_cast<uint64_t>(mBasePosition +
      (static_cast<float>(USECS_PER_S * diffOffset) / mOutRate));
    return position;
  }
  return UINT64_MAX;
}

uint64_t AudioClock::GetPositionInFrames()
{
  return (GetPositionUnlocked() * mOutRate) / USECS_PER_S;
}

void AudioClock::SetPlaybackRateUnlocked(double aPlaybackRate)
{
  
  int64_t position = mAudioStream->GetPositionInFramesUnlocked();
  if (position > mPlaybackRateChangeOffset) {
    mOldBasePosition = mBasePosition;
    mBasePosition = GetPositionUnlocked();
    mOldBaseOffset = mPlaybackRateChangeOffset;
    mBaseOffset = position;
    mPlaybackRateChangeOffset = mWritten;
    mOldOutRate = mOutRate;
    mOutRate = static_cast<int>(mInRate / aPlaybackRate);
  } else {
    
    
    
    mBasePosition = GetPositionUnlocked();
    mBaseOffset = position;
    mPlaybackRateChangeOffset = mWritten;
    mOutRate = static_cast<int>(mInRate / aPlaybackRate);
  }
}

double AudioClock::GetPlaybackRate()
{
  return static_cast<double>(mInRate) / mOutRate;
}

void AudioClock::SetPreservesPitch(bool aPreservesPitch)
{
  mPreservesPitch = aPreservesPitch;
}

bool AudioClock::GetPreservesPitch()
{
  return mPreservesPitch;
}
} 
