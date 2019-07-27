



#ifndef MP4_DEMUXER_H_
#define MP4_DEMUXER_H_

#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "mp4_demuxer/DecoderData.h"
#include "mp4_demuxer/Interval.h"
#include "nsISupportsImpl.h"
#include "mozilla/Monitor.h"

namespace mozilla { class MediaByteRange; }

namespace mp4_demuxer
{

using mozilla::Monitor;
struct StageFrightPrivate;
typedef int64_t Microseconds;

class Stream
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Stream);

  virtual bool ReadAt(int64_t offset, void* data, size_t size,
                      size_t* bytes_read) = 0;
  virtual bool CachedReadAt(int64_t offset, void* data, size_t size,
                            size_t* bytes_read) = 0;
  virtual bool Length(int64_t* size) = 0;

  virtual void DiscardBefore(int64_t offset) {}

protected:
  virtual ~Stream() {}
};

enum TrackType { kVideo = 1, kAudio };

class MP4Demuxer
{
public:
  explicit MP4Demuxer(Stream* aSource, Microseconds aTimestampOffset, Monitor* aMonitor);
  ~MP4Demuxer();

  bool Init();
  Microseconds Duration();
  bool CanSeek();

  bool HasValidAudio();
  bool HasValidVideo();

  void SeekAudio(Microseconds aTime);
  void SeekVideo(Microseconds aTime);

  
  
  MP4Sample* DemuxAudioSample();
  MP4Sample* DemuxVideoSample();

  const CryptoFile& Crypto() { return mCrypto; }
  const AudioDecoderConfig& AudioConfig() { return mAudioConfig; }
  const VideoDecoderConfig& VideoConfig() { return mVideoConfig; }

  void UpdateIndex(const nsTArray<mozilla::MediaByteRange>& aByteRanges);

  void ConvertByteRangesToTime(
    const nsTArray<mozilla::MediaByteRange>& aByteRanges,
    nsTArray<Interval<Microseconds>>* aIntervals);

  int64_t GetEvictionOffset(Microseconds aTime);

  
  
  Microseconds GetNextKeyframeTime();

private:
  AudioDecoderConfig mAudioConfig;
  VideoDecoderConfig mVideoConfig;
  CryptoFile mCrypto;

  nsAutoPtr<StageFrightPrivate> mPrivate;
  nsRefPtr<Stream> mSource;
  nsTArray<mozilla::MediaByteRange> mCachedByteRanges;
  nsTArray<Interval<Microseconds>> mCachedTimeRanges;
  Microseconds mTimestampOffset;
  Monitor* mMonitor;
  Microseconds mNextKeyframeTime;
};

} 

#endif 
