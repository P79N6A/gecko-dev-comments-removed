



#ifndef MEDIA_MP4_MP4DEMUXER_H
#define MEDIA_MP4_MP4DEMUXER_H

#include "mp4_demuxer/audio_decoder_config.h"
#include "mp4_demuxer/video_decoder_config.h"
#include "mp4_demuxer/decrypt_config.h"
#include "mp4_demuxer/box_definitions.h"


#include "nsAutoPtr.h"
#include <memory>

namespace mp4_demuxer {

class Stream;
class BoxReader;
struct Movie;
class TrackRunIterator;
struct AVCDecoderConfigurationRecord;
class AAC;



struct MP4Sample {
  MP4Sample(Microseconds decode_timestamp,
            Microseconds composition_timestamp,
            Microseconds duration,
            int64_t byte_offset,
            std::vector<uint8_t>* data,
            TrackType type,
            DecryptConfig* decrypt_config,
            bool is_sync_point);
  ~MP4Sample();

  const Microseconds decode_timestamp;

  const Microseconds composition_timestamp;

  const Microseconds duration;

  
  const int64_t byte_offset;

  
  const nsAutoPtr<std::vector<uint8_t>> data;

  
  const TrackType type;

  const nsAutoPtr<DecryptConfig> decrypt_config;

  
  const bool is_sync_point;

  bool is_encrypted() const;
};

class MP4Demuxer {
public:
  MP4Demuxer(Stream* stream);
  ~MP4Demuxer();

  bool Init();

  
  bool Demux(nsAutoPtr<MP4Sample>* sample,
             bool* end_of_stream);

  bool HasAudio() const;
  const AudioDecoderConfig& AudioConfig() const;

  bool HasVideo() const;
  const VideoDecoderConfig& VideoConfig() const;

  Microseconds Duration() const;

  bool CanSeek() const;

private:

  enum State {
    kWaitingForInit,
    kParsingBoxes,
    kEmittingSamples,
    kError
  };

  
  bool Parse(nsAutoPtr<MP4Sample>* sample,
             bool& end_of_stream);

  void ChangeState(State new_state);

  
  bool ParseBox();
  bool ParseMoov(BoxReader* reader);
  bool ParseMoof(BoxReader* reader);

  void Reset();

  bool EmitSample(nsAutoPtr<MP4Sample>* sample);

  bool PrepareAACBuffer(const AAC& aac_config,
                        std::vector<uint8_t>* frame_buf,
                        std::vector<SubsampleEntry>* subsamples) const;

  bool PrepareAVCBuffer(const AVCDecoderConfigurationRecord& avc_config,
                        std::vector<uint8_t>* frame_buf,
                        std::vector<SubsampleEntry>* subsamples) const;

  State state_;
  
  
  
  
  Stream* stream_;
  int64_t stream_offset_;

  Microseconds duration_;

  
  
  
  
  
  int64_t moof_head_;
  
  
  int64_t mdat_tail_;

  nsAutoPtr<Movie> moov_;
  nsAutoPtr<TrackRunIterator> runs_;

  uint32_t audio_track_id_;
  uint32_t video_track_id_;

  uint32_t audio_frameno;
  uint32_t video_frameno;

  AudioDecoderConfig audio_config_;
  VideoDecoderConfig video_config_;

  bool has_audio_;
  bool has_sbr_; 
  bool is_audio_track_encrypted_;

  bool has_video_;
  bool is_video_track_encrypted_;

  bool can_seek_;
};

} 

#endif
