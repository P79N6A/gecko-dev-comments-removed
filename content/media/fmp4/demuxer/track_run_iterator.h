



#ifndef MEDIA_MP4_TRACK_RUN_ITERATOR_H_
#define MEDIA_MP4_TRACK_RUN_ITERATOR_H_

#include <vector>
#include <memory>

#include "mp4_demuxer/box_definitions.h"
#include "mp4_demuxer/cenc.h"
#include "nsAutoPtr.h"

namespace mp4_demuxer {

class DecryptConfig;

Microseconds MicrosecondsFromRational(int64_t numer, int64_t denom);

struct SampleInfo;
struct TrackRunInfo;

class TrackRunIterator {
 public:
  
  
  TrackRunIterator(const Movie* moov);
  ~TrackRunIterator();

  void Reset();

  
  bool Init(const MovieFragment& moof);

  
  bool IsRunValid() const;
  bool IsSampleValid() const;

  
  
  void AdvanceRun();
  void AdvanceSample();

  
  
  bool AuxInfoNeedsToBeCached();

  
  
  
  
  bool CacheAuxInfo(Stream* stream, int64_t moof_offset);

  
  
  
  
  
  int64_t GetMaxClearOffset();

  
  Microseconds GetMinDecodeTimestamp();

  
  uint32_t track_id() const;
  int64_t aux_info_offset() const;
  int aux_info_size() const;
  bool is_encrypted() const;
  bool is_audio() const;
  
  const AudioSampleEntry& audio_description() const;
  const VideoSampleEntry& video_description() const;

  
  int64_t sample_offset() const;
  int sample_size() const;
  Microseconds dts() const;
  Microseconds cts() const;
  Microseconds duration() const;
  bool is_keyframe() const;

  
  
  void GetDecryptConfig(nsAutoPtr<DecryptConfig>& config);

 private:
  void ResetRun();
  const TrackEncryption& track_encryption() const;

  const Movie* moov_;

  std::vector<TrackRunInfo> runs_;
  std::vector<TrackRunInfo>::const_iterator run_itr_;
  std::vector<SampleInfo>::const_iterator sample_itr_;

  std::vector<FrameCENCInfo> cenc_info_;

  int64_t sample_dts_;
  int64_t sample_offset_;

  DISALLOW_COPY_AND_ASSIGN(TrackRunIterator);
};

}  

#endif  
