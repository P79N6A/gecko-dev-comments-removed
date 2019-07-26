




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
#include "soundtouch/SoundTouch.h"
#include "Latency.h"

#if defined(MOZ_CUBEB)
#include "nsAutoRef.h"
#include "cubeb/cubeb.h"

template <>
class nsAutoRefTraits<cubeb_stream> : public nsPointerRefTraits<cubeb_stream>
{
public:
  static void Release(cubeb_stream* aStream) { cubeb_stream_destroy(aStream); }
};

#endif

namespace mozilla {

#ifdef PR_LOGGING
PRLogModuleInfo* gAudioStreamLog = nullptr;
#endif

#define PREF_VOLUME_SCALE "media.volume_scale"
#define PREF_CUBEB_LATENCY "media.cubeb_latency_ms"

static Mutex* gAudioPrefsLock = nullptr;
static double gVolumeScale;
static uint32_t gCubebLatency;







static int gDumpedAudioCount = 0;

static int PrefChanged(const char* aPref, void* aClosure)
{
  if (strcmp(aPref, PREF_VOLUME_SCALE) == 0) {
    nsAdoptingString value = Preferences::GetString(aPref);
    MutexAutoLock lock(*gAudioPrefsLock);
    if (value.IsEmpty()) {
      gVolumeScale = 1.0;
    } else {
      NS_ConvertUTF16toUTF8 utf8(value);
      gVolumeScale = std::max<double>(0, PR_strtod(utf8.get(), nullptr));
    }
  } else if (strcmp(aPref, PREF_CUBEB_LATENCY) == 0) {
    
    
    
    uint32_t value = Preferences::GetUint(aPref, 100);
    MutexAutoLock lock(*gAudioPrefsLock);
    gCubebLatency = std::min<uint32_t>(std::max<uint32_t>(value, 20), 1000);
  }
  return 0;
}

static double GetVolumeScale()
{
  MutexAutoLock lock(*gAudioPrefsLock);
  return gVolumeScale;
}

#if defined(MOZ_CUBEB)
static cubeb* gCubebContext;

static cubeb* GetCubebContext()
{
  MutexAutoLock lock(*gAudioPrefsLock);
  if (gCubebContext ||
      cubeb_init(&gCubebContext, "AudioStream") == CUBEB_OK) {
    return gCubebContext;
  }
  NS_WARNING("cubeb_init failed");
  return nullptr;
}

static uint32_t GetCubebLatency()
{
  MutexAutoLock lock(*gAudioPrefsLock);
  return gCubebLatency;
}
#endif

#if defined(MOZ_CUBEB) && defined(__ANDROID__) && defined(MOZ_B2G)
static cubeb_stream_type ConvertChannelToCubebType(dom::AudioChannelType aType)
{
  switch(aType) {
    case dom::AUDIO_CHANNEL_NORMAL:
      return CUBEB_STREAM_TYPE_SYSTEM;
    case dom::AUDIO_CHANNEL_CONTENT:
      return CUBEB_STREAM_TYPE_MUSIC;
    case dom::AUDIO_CHANNEL_NOTIFICATION:
      return CUBEB_STREAM_TYPE_NOTIFICATION;
    case dom::AUDIO_CHANNEL_ALARM:
      return CUBEB_STREAM_TYPE_ALARM;
    case dom::AUDIO_CHANNEL_TELEPHONY:
      return CUBEB_STREAM_TYPE_VOICE_CALL;
    case dom::AUDIO_CHANNEL_RINGER:
      return CUBEB_STREAM_TYPE_RING;
    
    case dom::AUDIO_CHANNEL_PUBLICNOTIFICATION:
    default:
      NS_ERROR("The value of AudioChannelType is invalid");
      return CUBEB_STREAM_TYPE_MAX;
  }
}
#endif

AudioStream::AudioStream()
: mInRate(0),
  mOutRate(0),
  mChannels(0),
  mWritten(0),
  mAudioClock(MOZ_THIS_IN_INITIALIZER_LIST())
{}

void AudioStream::InitLibrary()
{
#ifdef PR_LOGGING
  gAudioStreamLog = PR_NewLogModule("AudioStream");
#endif
  gAudioPrefsLock = new Mutex("AudioStream::gAudioPrefsLock");
  PrefChanged(PREF_VOLUME_SCALE, nullptr);
  Preferences::RegisterCallback(PrefChanged, PREF_VOLUME_SCALE);
#if defined(MOZ_CUBEB)
  PrefChanged(PREF_CUBEB_LATENCY, nullptr);
  Preferences::RegisterCallback(PrefChanged, PREF_CUBEB_LATENCY);
#endif
}

void AudioStream::ShutdownLibrary()
{
  Preferences::UnregisterCallback(PrefChanged, PREF_VOLUME_SCALE);
#if defined(MOZ_CUBEB)
  Preferences::UnregisterCallback(PrefChanged, PREF_CUBEB_LATENCY);
#endif
  delete gAudioPrefsLock;
  gAudioPrefsLock = nullptr;

#if defined(MOZ_CUBEB)
  if (gCubebContext) {
    cubeb_destroy(gCubebContext);
    gCubebContext = nullptr;
  }
#endif
}

AudioStream::~AudioStream()
{
}

nsresult AudioStream::EnsureTimeStretcherInitialized()
{
  if (!mTimeStretcher) {
    
    if (mChannels > 2) {
      return NS_ERROR_FAILURE;
    }
    mTimeStretcher = new soundtouch::SoundTouch();
    mTimeStretcher->setSampleRate(mInRate);
    mTimeStretcher->setChannels(mChannels);
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

  if (EnsureTimeStretcherInitialized() != NS_OK) {
    return NS_ERROR_FAILURE;
  }

  mAudioClock.SetPlaybackRate(aPlaybackRate);
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

  if (EnsureTimeStretcherInitialized() != NS_OK) {
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

#if defined(MOZ_CUBEB)
class nsCircularByteBuffer
{
public:
  nsCircularByteBuffer()
    : mBuffer(nullptr), mCapacity(0), mStart(0), mCount(0)
  {}

  
  
  void SetCapacity(uint32_t aCapacity) {
    NS_ABORT_IF_FALSE(!mBuffer, "Buffer allocated.");
    mCapacity = aCapacity;
    mBuffer = new uint8_t[mCapacity];
  }

  uint32_t Length() {
    return mCount;
  }

  uint32_t Capacity() {
    return mCapacity;
  }

  uint32_t Available() {
    return Capacity() - Length();
  }

  
  
  void AppendElements(const uint8_t* aSrc, uint32_t aLength) {
    NS_ABORT_IF_FALSE(mBuffer && mCapacity, "Buffer not initialized.");
    NS_ABORT_IF_FALSE(aLength <= Available(), "Buffer full.");

    uint32_t end = (mStart + mCount) % mCapacity;

    uint32_t toCopy = std::min(mCapacity - end, aLength);
    memcpy(&mBuffer[end], aSrc, toCopy);
    memcpy(&mBuffer[0], aSrc + toCopy, aLength - toCopy);
    mCount += aLength;
  }

  
  
  
  void PopElements(uint32_t aSize, void** aData1, uint32_t* aSize1,
                   void** aData2, uint32_t* aSize2) {
    NS_ABORT_IF_FALSE(mBuffer && mCapacity, "Buffer not initialized.");
    NS_ABORT_IF_FALSE(aSize <= Length(), "Request too large.");

    *aData1 = &mBuffer[mStart];
    *aSize1 = std::min(mCapacity - mStart, aSize);
    *aData2 = &mBuffer[0];
    *aSize2 = aSize - *aSize1;
    mCount -= *aSize1 + *aSize2;
    mStart += *aSize1 + *aSize2;
    mStart %= mCapacity;
  }

private:
  nsAutoArrayPtr<uint8_t> mBuffer;
  uint32_t mCapacity;
  uint32_t mStart;
  uint32_t mCount;
};

class BufferedAudioStream : public AudioStream
{
 public:
  BufferedAudioStream();
  ~BufferedAudioStream();

  nsresult Init(int32_t aNumChannels, int32_t aRate,
                const dom::AudioChannelType aAudioChannelType);
  void Shutdown();
  nsresult Write(const AudioDataValue* aBuf, uint32_t aFrames);
  uint32_t Available();
  void SetVolume(double aVolume);
  void Drain();
  void Start();
  void Pause();
  void Resume();
  int64_t GetPosition();
  int64_t GetPositionInFrames();
  int64_t GetPositionInFramesInternal();
  int64_t GetLatencyInFrames();
  bool IsPaused();
  
  
  
  nsresult EnsureTimeStretcherInitialized();

private:
  static long DataCallback_S(cubeb_stream*, void* aThis, void* aBuffer, long aFrames)
  {
    return static_cast<BufferedAudioStream*>(aThis)->DataCallback(aBuffer, aFrames);
  }

  static void StateCallback_S(cubeb_stream*, void* aThis, cubeb_state aState)
  {
    static_cast<BufferedAudioStream*>(aThis)->StateCallback(aState);
  }

  long DataCallback(void* aBuffer, long aFrames);
  void StateCallback(cubeb_state aState);

  long GetUnprocessed(void* aBuffer, long aFrames);

  long GetTimeStretched(void* aBuffer, long aFrames);


  
  
  int64_t GetPositionInFramesUnlocked();

  void StartUnlocked();

  
  
  
  
  Monitor mMonitor;

  
  
  uint64_t mLostFrames;

  
  FILE* mDumpFile;

  
  
  
  
  nsCircularByteBuffer mBuffer;

  
  double mVolume;

  
  
  nsAutoRef<cubeb_stream> mCubebStream;

  uint32_t mBytesPerFrame;

  uint32_t BytesToFrames(uint32_t aBytes) {
    NS_ASSERTION(aBytes % mBytesPerFrame == 0,
                 "Byte count not aligned on frames size.");
    return aBytes / mBytesPerFrame;
  }

  uint32_t FramesToBytes(uint32_t aFrames) {
    return aFrames * mBytesPerFrame;
  }


  enum StreamState {
    INITIALIZED, 
    STARTED,     
    STOPPED,     
    DRAINING,    
                 
                 
                 
    DRAINED,     
    ERRORED      
  };

  StreamState mState;
};
#endif

AudioStream* AudioStream::AllocateStream()
{
#if defined(MOZ_CUBEB)
  return new BufferedAudioStream();
#endif
  return nullptr;
}

int AudioStream::MaxNumberOfChannels()
{
  uint32_t maxNumberOfChannels, rv;

  rv = cubeb_get_max_channel_count(GetCubebContext(), &maxNumberOfChannels);

  if (rv != CUBEB_OK) {
    return 0;
  }

  return static_cast<int>(maxNumberOfChannels);
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

  uint32_t samples = aStream->GetChannels()*aFrames;
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

#if defined(MOZ_CUBEB)
BufferedAudioStream::BufferedAudioStream()
  : mMonitor("BufferedAudioStream"), mLostFrames(0), mDumpFile(nullptr),
    mVolume(1.0), mBytesPerFrame(0), mState(INITIALIZED)
{
  AsyncLatencyLogger::Get(true)->AddRef();
}

BufferedAudioStream::~BufferedAudioStream()
{
  Shutdown();
  if (mDumpFile) {
    fclose(mDumpFile);
  }
  AsyncLatencyLogger::Get()->Release();
}

nsresult
BufferedAudioStream::EnsureTimeStretcherInitialized()
{
  MonitorAutoLock mon(mMonitor);
  return AudioStream::EnsureTimeStretcherInitialized();
}

nsresult
BufferedAudioStream::Init(int32_t aNumChannels, int32_t aRate,
                            const dom::AudioChannelType aAudioChannelType)
{
  cubeb* cubebContext = GetCubebContext();

  if (!cubebContext || aNumChannels < 0 || aRate < 0) {
    return NS_ERROR_FAILURE;
  }

  mInRate = mOutRate = aRate;
  mChannels = aNumChannels;

  mDumpFile = OpenDumpFile(this);

  cubeb_stream_params params;
  params.rate = aRate;
  params.channels = aNumChannels;
#if defined(__ANDROID__)
#if defined(MOZ_B2G)
  params.stream_type = ConvertChannelToCubebType(aAudioChannelType);
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
  mBytesPerFrame = sizeof(AudioDataValue) * aNumChannels;

  mAudioClock.Init();

  {
    cubeb_stream* stream;
    if (cubeb_stream_init(cubebContext, &stream, "BufferedAudioStream", params,
                          GetCubebLatency(), DataCallback_S, StateCallback_S, this) == CUBEB_OK) {
      mCubebStream.own(stream);
    }
  }

  if (!mCubebStream) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  uint32_t bufferLimit = FramesToBytes(aRate);
  NS_ABORT_IF_FALSE(bufferLimit % mBytesPerFrame == 0, "Must buffer complete frames");
  mBuffer.SetCapacity(bufferLimit);

  return NS_OK;
}

void
BufferedAudioStream::Shutdown()
{
  if (mState == STARTED) {
    Pause();
  }
  if (mCubebStream) {
    mCubebStream.reset();
  }
}

nsresult
BufferedAudioStream::Write(const AudioDataValue* aBuf, uint32_t aFrames)
{
  MonitorAutoLock mon(mMonitor);
  if (!mCubebStream || mState == ERRORED) {
    return NS_ERROR_FAILURE;
  }
  NS_ASSERTION(mState == INITIALIZED || mState == STARTED,
    "Stream write in unexpected state.");

  const uint8_t* src = reinterpret_cast<const uint8_t*>(aBuf);
  uint32_t bytesToCopy = FramesToBytes(aFrames);

  while (bytesToCopy > 0) {
    uint32_t available = std::min(bytesToCopy, mBuffer.Available());
    NS_ABORT_IF_FALSE(available % mBytesPerFrame == 0,
        "Must copy complete frames.");

    mBuffer.AppendElements(src, available);
    src += available;
    bytesToCopy -= available;

    if (bytesToCopy > 0) {
      
      
      if (mState != STARTED) {
        StartUnlocked();
        if (mState != STARTED) {
          return NS_ERROR_FAILURE;
        }
      }
      mon.Wait();
    }
  }

  mWritten += aFrames;
  return NS_OK;
}

uint32_t
BufferedAudioStream::Available()
{
  MonitorAutoLock mon(mMonitor);
  NS_ABORT_IF_FALSE(mBuffer.Length() % mBytesPerFrame == 0, "Buffer invariant violated.");
  return BytesToFrames(mBuffer.Available());
}

void
BufferedAudioStream::SetVolume(double aVolume)
{
  MonitorAutoLock mon(mMonitor);
  NS_ABORT_IF_FALSE(aVolume >= 0.0 && aVolume <= 1.0, "Invalid volume");
  mVolume = aVolume;
}

void
BufferedAudioStream::Drain()
{
  MonitorAutoLock mon(mMonitor);
  if (mState != STARTED) {
    NS_ASSERTION(mBuffer.Available() == 0, "Draining with unplayed audio");
    return;
  }
  mState = DRAINING;
  while (mState == DRAINING) {
    mon.Wait();
  }
}

void
BufferedAudioStream::Start()
{
  MonitorAutoLock mon(mMonitor);
  StartUnlocked();
}

void
BufferedAudioStream::StartUnlocked()
{
  mMonitor.AssertCurrentThreadOwns();
  if (!mCubebStream || mState != INITIALIZED) {
    return;
  }
  if (mState != STARTED) {
    int r;
    {
      MonitorAutoUnlock mon(mMonitor);
      r = cubeb_stream_start(mCubebStream);
    }
    if (mState != ERRORED) {
      mState = r == CUBEB_OK ? STARTED : ERRORED;
    }
  }
}

void
BufferedAudioStream::Pause()
{
  MonitorAutoLock mon(mMonitor);
  if (!mCubebStream || mState != STARTED) {
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
BufferedAudioStream::Resume()
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

int64_t
BufferedAudioStream::GetPosition()
{
  return mAudioClock.GetPosition();
}


#ifdef _MSC_VER
#pragma optimize("", off)
#endif
int64_t
BufferedAudioStream::GetPositionInFrames()
{
  return mAudioClock.GetPositionInFrames();
}
#ifdef _MSC_VER
#pragma optimize("", on)
#endif

int64_t
BufferedAudioStream::GetPositionInFramesInternal()
{
  MonitorAutoLock mon(mMonitor);
  return GetPositionInFramesUnlocked();
}

int64_t
BufferedAudioStream::GetPositionInFramesUnlocked()
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
  if (position >= mLostFrames) {
    adjustedPosition = position - mLostFrames;
  }
  return std::min<uint64_t>(adjustedPosition, INT64_MAX);
}

int64_t
BufferedAudioStream::GetLatencyInFrames()
{
  uint32_t latency;
  if(cubeb_stream_get_latency(mCubebStream, &latency)) {
    NS_WARNING("Could not get cubeb latency.");
    return 0;
  }
  return static_cast<int64_t>(latency);
}

bool
BufferedAudioStream::IsPaused()
{
  MonitorAutoLock mon(mMonitor);
  return mState == STOPPED;
}

long
BufferedAudioStream::GetUnprocessed(void* aBuffer, long aFrames)
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
  return BytesToFrames(available) + flushedFrames;
}

long
BufferedAudioStream::GetTimeStretched(void* aBuffer, long aFrames)
{
  long processedFrames = 0;

  
  if (AudioStream::EnsureTimeStretcherInitialized() != NS_OK) {
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
      for(uint32_t i = 0; i < 2; i++) {
        mTimeStretcher->putSamples(reinterpret_cast<AudioDataValue*>(input[i]), BytesToFrames(input_size[i]));
      }
    }
    uint32_t receivedFrames = mTimeStretcher->receiveSamples(reinterpret_cast<AudioDataValue*>(wpos), aFrames - processedFrames);
    wpos += FramesToBytes(receivedFrames);
    processedFrames += receivedFrames;
  } while (processedFrames < aFrames && !lowOnBufferedData);

  return processedFrames;
}

long
BufferedAudioStream::DataCallback(void* aBuffer, long aFrames)
{
  MonitorAutoLock mon(mMonitor);
  uint32_t available = std::min(static_cast<uint32_t>(FramesToBytes(aFrames)), mBuffer.Length());
  NS_ABORT_IF_FALSE(available % mBytesPerFrame == 0, "Must copy complete frames");
  uint32_t underrunFrames = 0;
  uint32_t servicedFrames = 0;

  if (available) {
    AudioDataValue* output = reinterpret_cast<AudioDataValue*>(aBuffer);
    if (mInRate == mOutRate) {
      servicedFrames = GetUnprocessed(output, aFrames);
    } else {
      servicedFrames = GetTimeStretched(output, aFrames);
    }
    float scaled_volume = float(GetVolumeScale() * mVolume);

    ScaleAudioSamples(output, aFrames * mChannels, scaled_volume);

    NS_ABORT_IF_FALSE(mBuffer.Length() % mBytesPerFrame == 0, "Must copy complete frames");

    
    mon.NotifyAll();
  }

  underrunFrames = aFrames - servicedFrames;

  if (mState != DRAINING) {
    uint8_t* rpos = static_cast<uint8_t*>(aBuffer) + FramesToBytes(aFrames - underrunFrames);
    memset(rpos, 0, FramesToBytes(underrunFrames));
#ifdef PR_LOGGING
    if (underrunFrames) {
      PR_LOG(gAudioStreamLog, PR_LOG_WARNING,
             ("AudioStream %p lost %d frames", this, underrunFrames));
    }
#endif
    mLostFrames += underrunFrames;
    servicedFrames += underrunFrames;
  }

  WriteDumpFile(mDumpFile, this, aFrames, aBuffer);
  if (PR_LOG_TEST(GetLatencyLog(), PR_LOG_DEBUG)) {
    uint32_t latency = UINT32_MAX;
    if (cubeb_stream_get_latency(mCubebStream, &latency)) {
      NS_WARNING("Could not get latency from cubeb.");
    }
    LogLatency(AsyncLatencyLogger::AudioStream, 0, (mBuffer.Length() * 1000) / mOutRate);
    LogLatency(AsyncLatencyLogger::Cubeb, 0, (latency * 1000) / mOutRate);
  }

  mAudioClock.UpdateWritePosition(servicedFrames);
  return servicedFrames;
}

void
BufferedAudioStream::StateCallback(cubeb_state aState)
{
  MonitorAutoLock mon(mMonitor);
  if (aState == CUBEB_STATE_DRAINED) {
    mState = DRAINED;
  } else if (aState == CUBEB_STATE_ERROR) {
    mState = ERRORED;
  }
  mon.NotifyAll();
}

#endif

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

uint64_t AudioClock::GetPosition()
{
  int64_t position = mAudioStream->GetPositionInFramesInternal();
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
  return (GetPosition() * mOutRate) / USECS_PER_S;
}

void AudioClock::SetPlaybackRate(double aPlaybackRate)
{
  int64_t position = mAudioStream->GetPositionInFramesInternal();
  if (position > mPlaybackRateChangeOffset) {
    mOldBasePosition = mBasePosition;
    mBasePosition = GetPosition();
    mOldBaseOffset = mPlaybackRateChangeOffset;
    mBaseOffset = position;
    mPlaybackRateChangeOffset = mWritten;
    mOldOutRate = mOutRate;
    mOutRate = static_cast<int>(mInRate / aPlaybackRate);
  } else {
    
    
    
    mBasePosition = GetPosition();
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
