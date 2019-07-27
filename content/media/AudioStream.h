




#if !defined(AudioStream_h_)
#define AudioStream_h_

#include "AudioSampleFormat.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "Latency.h"
#include "mozilla/dom/AudioChannelBinding.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"
#include "CubebUtils.h"

namespace soundtouch {
class SoundTouch;
}

namespace mozilla {

template<>
struct DefaultDelete<cubeb_stream>
{
  void operator()(cubeb_stream* aStream) const
  {
    cubeb_stream_destroy(aStream);
  }
};

class AudioStream;
class FrameHistory;

class AudioClock
{
public:
  explicit AudioClock(AudioStream* aStream);
  
  
  void Init();
  
  
  void UpdateFrameHistory(uint32_t aServiced, uint32_t aUnderrun);
  
  
  
  
  int64_t GetPositionUnlocked() const;
  
  
  int64_t GetPositionInFrames() const;
  
  
  
  
  void SetPlaybackRateUnlocked(double aPlaybackRate);
  
  
  double GetPlaybackRate() const;
  
  
  void SetPreservesPitch(bool aPreservesPitch);
  
  
  bool GetPreservesPitch() const;
private:
  
  
  AudioStream* const mAudioStream;
  
  int mOutRate;
  
  int mInRate;
  
  bool mPreservesPitch;
  
  const nsAutoPtr<FrameHistory> mFrameHistory;
};

class CircularByteBuffer
{
public:
  CircularByteBuffer()
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

  
  
  uint32_t ContractTo(uint32_t aSize) {
    NS_ABORT_IF_FALSE(mBuffer && mCapacity, "Buffer not initialized.");
    if (aSize >= mCount) {
      return mCount;
    }
    mStart += (mCount - aSize);
    mCount = aSize;
    mStart %= mCapacity;
    return mCount;
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    size_t amount = 0;
    amount += mBuffer.SizeOfExcludingThis(aMallocSizeOf);
    return amount;
  }

  void Reset()
  {
    mBuffer = nullptr;
    mCapacity = 0;
    mStart = 0;
    mCount = 0;
  }

private:
  nsAutoArrayPtr<uint8_t> mBuffer;
  uint32_t mCapacity;
  uint32_t mStart;
  uint32_t mCount;
};

class AudioInitTask;





class AudioStream MOZ_FINAL
{
  virtual ~AudioStream();

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AudioStream)
  AudioStream();

  enum LatencyRequest {
    HighLatency,
    LowLatency
  };

  
  
  
  nsresult Init(int32_t aNumChannels, int32_t aRate,
                const dom::AudioChannel aAudioStreamChannel,
                LatencyRequest aLatencyRequest);

  
  void Shutdown();

  void Reset();

  
  
  
  
  
  nsresult Write(const AudioDataValue* aBuf, uint32_t aFrames, TimeStamp* aTime = nullptr);

  
  uint32_t Available();

  
  
  void SetVolume(double aVolume);

  
  
  void SetMicrophoneActive(bool aActive);
  void PanOutputIfNeeded(bool aMicrophoneActive);
  void ResetStreamIfNeeded();

  
  void Drain();

  
  void Cancel();

  
  void Start();

  
  
  int64_t GetWritten();

  
  void Pause();

  
  void Resume();

  
  
  int64_t GetPosition();

  
  
  int64_t GetPositionInFrames();

  
  bool IsPaused();

  int GetRate() { return mOutRate; }
  int GetChannels() { return mChannels; }
  int GetOutChannels() { return mOutChannels; }

  
  
  nsresult SetPlaybackRate(double aPlaybackRate);
  
  nsresult SetPreservesPitch(bool aPreservesPitch);

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

protected:
  friend class AudioClock;

  
  
  
  
  int64_t GetPositionInFramesUnlocked();

private:
  friend class AudioInitTask;

  
  nsresult OpenCubeb(cubeb_stream_params &aParams,
                     LatencyRequest aLatencyRequest);
  void AudioInitTaskFinished();

  void CheckForStart();

  static long DataCallback_S(cubeb_stream*, void* aThis, void* aBuffer, long aFrames)
  {
    return static_cast<AudioStream*>(aThis)->DataCallback(aBuffer, aFrames);
  }

  static void StateCallback_S(cubeb_stream*, void* aThis, cubeb_state aState)
  {
    static_cast<AudioStream*>(aThis)->StateCallback(aState);
  }


  static void DeviceChangedCallback_s(void * aThis) {
    static_cast<AudioStream*>(aThis)->DeviceChangedCallback();
  }

  long DataCallback(void* aBuffer, long aFrames);
  void StateCallback(cubeb_state aState);
  void DeviceChangedCallback();

  nsresult EnsureTimeStretcherInitializedUnlocked();

  
  long GetUnprocessed(void* aBuffer, long aFrames, int64_t &aTime);
  long GetTimeStretched(void* aBuffer, long aFrames, int64_t &aTime);
  long GetUnprocessedWithSilencePadding(void* aBuffer, long aFrames, int64_t &aTime);

  int64_t GetLatencyInFrames();
  void GetBufferInsertTime(int64_t &aTimeMs);

  void StartUnlocked();

  
  
  
  
  Monitor mMonitor;

  
  int mInRate;
  
  int mOutRate;
  int mChannels;
  int mOutChannels;
#if defined(__ANDROID__)
  dom::AudioChannel mAudioChannel;
#endif
  
  int64_t mWritten;
  AudioClock mAudioClock;
  nsAutoPtr<soundtouch::SoundTouch> mTimeStretcher;
  nsRefPtr<AsyncLatencyLogger> mLatencyLog;

  
  TimeStamp mStartTime;
  
  LatencyRequest mLatencyRequest;
  
  int64_t mReadPoint;
  
  
  
  struct Inserts {
    int64_t mTimeMs;
    int64_t mFrames;
  };
  nsAutoTArray<Inserts, 8> mInserts;

  
  FILE* mDumpFile;

  
  
  
  
  CircularByteBuffer mBuffer;

  
  UniquePtr<cubeb_stream> mCubebStream;

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
    RUNNING,     
    STOPPED,     
    DRAINING,    
                 
                 
                 
    DRAINED,     
    ERRORED,     
    SHUTDOWN     
  };

  StreamState mState;
  bool mNeedsStart; 
  bool mIsFirst;
  
  bool mMicrophoneActive;
  
  
  
  bool mShouldDropFrames;
  
  
  bool mPendingAudioInitTask;
};

class AudioInitTask : public nsRunnable
{
public:
  AudioInitTask(AudioStream *aStream,
                AudioStream::LatencyRequest aLatencyRequest,
                const cubeb_stream_params &aParams)
    : mAudioStream(aStream)
    , mLatencyRequest(aLatencyRequest)
    , mParams(aParams)
  {}

  nsresult Dispatch()
  {
    
    nsresult rv = NS_NewNamedThread("CubebInit", getter_AddRefs(mThread));
    if (NS_SUCCEEDED(rv)) {
      
      rv = mThread->Dispatch(this, NS_DISPATCH_NORMAL);
    }
    return rv;
  }

protected:
  virtual ~AudioInitTask() {};

private:
  NS_IMETHOD Run() MOZ_OVERRIDE MOZ_FINAL;

  RefPtr<AudioStream> mAudioStream;
  AudioStream::LatencyRequest mLatencyRequest;
  cubeb_stream_params mParams;

  nsCOMPtr<nsIThread> mThread;
};

} 

#endif
