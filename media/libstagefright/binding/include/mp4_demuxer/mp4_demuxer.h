



#ifndef MP4_DEMUXER_H_
#define MP4_DEMUXER_H_

#include "nsAutoPtr.h"
#include "mozilla/gfx/Rect.h"
#include "mp4_demuxer/DecoderData.h"

namespace mp4_demuxer
{

struct StageFrightPrivate;
typedef int64_t Microseconds;

class Stream
{

public:
  virtual bool ReadAt(int64_t offset, void* data, size_t size,
                      size_t* bytes_read) = 0;
  virtual bool Length(int64_t* size) = 0;

  virtual ~Stream() {}
};

enum TrackType { kVideo = 1, kAudio };

class MP4Demuxer
{
public:
  MP4Demuxer(Stream* aSource);
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

  const AudioDecoderConfig& AudioConfig() { return mAudioConfig; }
  const VideoDecoderConfig& VideoConfig() { return mVideoConfig; }

private:
  AudioDecoderConfig mAudioConfig;
  VideoDecoderConfig mVideoConfig;

  nsAutoPtr<StageFrightPrivate> mPrivate;
};
}

#endif
