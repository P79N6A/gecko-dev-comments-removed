




#include "mp4_demuxer/mp4_demuxer.h"

#include "mp4_demuxer/Streams.h"
#include "mp4_demuxer/box_reader.h"
#include "mp4_demuxer/box_definitions.h"
#include "mp4_demuxer/basictypes.h"
#include "mp4_demuxer/es_descriptor.h"
#include "mp4_demuxer/video_util.h"
#include "mp4_demuxer/track_run_iterator.h"
#include "mp4_demuxer/audio_decoder_config.h"
#include "mp4_demuxer/video_decoder_config.h"

#include <assert.h>

using namespace std;

namespace mp4_demuxer {


MP4Sample::MP4Sample(Microseconds _decode_timestamp,
                     Microseconds _composition_timestamp,
                     Microseconds _duration,
                     int64_t _byte_offset,
                     std::vector<uint8_t>* _data,
                     TrackType _type,
                     DecryptConfig* _decrypt_config,
                     bool _is_sync_point)
  : decode_timestamp(_decode_timestamp),
    composition_timestamp(_composition_timestamp),
    duration(_duration),
    byte_offset(_byte_offset),
    data(_data),
    type(_type),
    decrypt_config(_decrypt_config),
    is_sync_point(_is_sync_point)
{
}

MP4Sample::~MP4Sample()
{
}

bool MP4Sample::is_encrypted() const {
  return decrypt_config != nullptr;
};




MP4Demuxer::MP4Demuxer(Stream* stream)
  : state_(kWaitingForInit),
    stream_(stream),
    stream_offset_(0),
    duration_(InfiniteMicroseconds),
    moof_head_(0),
    mdat_tail_(0),
    audio_track_id_(0),
    video_track_id_(0),
    audio_frameno(0),
    video_frameno(0),
    has_audio_(false),
    has_sbr_(false),
    is_audio_track_encrypted_(false),
    has_video_(false),
    is_video_track_encrypted_(false),
    can_seek_(false)
{
}

MP4Demuxer::~MP4Demuxer()
{
}

bool MP4Demuxer::Init()
{
  ChangeState(kParsingBoxes);

  
  
  bool ok = true;
  const int64_t length = stream_->Length();
  while (ok &&
         stream_offset_ < length &&
         !moov_ &&
         state_ == kParsingBoxes) {
    ok = ParseBox();
  }
  return state_  >= kParsingBoxes &&
         state_ < kError;
}

void MP4Demuxer::Reset() {
  moov_ = nullptr;
  runs_ = nullptr;
  moof_head_ = 0;
  mdat_tail_ = 0;
}


static const char kMp4InitDataType[] = "video/mp4";

bool MP4Demuxer::ParseMoov(BoxReader* reader) {
  RCHECK(state_ < kError);
  moov_ = new Movie();
  RCHECK(moov_->Parse(reader));
  runs_ = new TrackRunIterator(moov_.get());

  has_audio_ = false;
  has_video_ = false;

  for (std::vector<Track>::const_iterator track = moov_->tracks.begin();
       track != moov_->tracks.end(); ++track) {
    
    
    
    const SampleDescription& samp_descr =
        track->media.information.sample_table.description;

    
    
    
    size_t desc_idx = 0;
    for (size_t t = 0; t < moov_->extends.tracks.size(); t++) {
      const TrackExtends& trex = moov_->extends.tracks[t];
      if (trex.track_id == track->header.track_id) {
        desc_idx = trex.default_sample_description_index;
        break;
      }
    }
    RCHECK(desc_idx > 0);
    desc_idx -= 1;  

    if (track->media.handler.type == kAudio && !audio_config_.IsValidConfig()) {
      RCHECK(!samp_descr.audio_entries.empty());

      
      
      if (desc_idx >= samp_descr.audio_entries.size())
        desc_idx = 0;
      const AudioSampleEntry& entry = samp_descr.audio_entries[desc_idx];
      const AAC& aac = entry.esds.aac;

      if (!(entry.format == FOURCC_MP4A ||
            (entry.format == FOURCC_ENCA &&
             entry.sinf.format.format == FOURCC_MP4A))) {
        DMX_LOG("Unsupported audio format 0x%x in stsd box\n", entry.format);
        return false;
      }

      int audio_type = entry.esds.object_type;
      DMX_LOG("audio_type 0x%x\n", audio_type);

      const std::vector<uint8_t>& asc = aac.AudioSpecificConfig();
      if (asc.size() > 0) {
        DMX_LOG("audio specific config:");
        for (unsigned i=0; i<asc.size(); ++i) {
          DMX_LOG(" 0x%x", asc[i]);
        }
        DMX_LOG("\n");
      }

      
      
      if (audio_type != kISO_14496_3 && audio_type != kISO_13818_7_AAC_LC) {
        DMX_LOG("Unsupported audio object type 0x%x  in esds.", audio_type);
        return false;
      }

      SampleFormat sample_format;
      if (entry.samplesize == 8) {
        sample_format = kSampleFormatU8;
      } else if (entry.samplesize == 16) {
        sample_format = kSampleFormatS16;
      } else if (entry.samplesize == 32) {
        sample_format = kSampleFormatS32;
      } else {
        DMX_LOG("Unsupported sample size.\n");
        return false;
      }

      is_audio_track_encrypted_ = entry.sinf.info.track_encryption.is_encrypted;
      DMX_LOG("is_audio_track_encrypted_: %d\n", is_audio_track_encrypted_);
      
      
      
      audio_config_.Initialize(kCodecAAC, sample_format,
                               aac.GetChannelLayout(has_sbr_),
                               aac.GetOutputSamplesPerSecond(has_sbr_),
                               &asc.front(),
                               asc.size(),
                               is_audio_track_encrypted_);
      has_audio_ = true;
      audio_track_id_ = track->header.track_id;
    }
    if (track->media.handler.type == kVideo && !video_config_.IsValidConfig()) {
      RCHECK(!samp_descr.video_entries.empty());
      if (desc_idx >= samp_descr.video_entries.size())
        desc_idx = 0;
      const VideoSampleEntry& entry = samp_descr.video_entries[desc_idx];

      if (!(entry.format == FOURCC_AVC1 ||
            (entry.format == FOURCC_ENCV &&
             entry.sinf.format.format == FOURCC_AVC1))) {
        DMX_LOG("Unsupported video format 0x%x in stsd box.\n", entry.format);
        return false;
      }

      
      IntSize coded_size(entry.width, entry.height);
      IntRect visible_rect(0, 0, coded_size.width(), coded_size.height());
      IntSize natural_size = GetNaturalSize(visible_rect.size(),
                                            entry.pixel_aspect.h_spacing,
                                            entry.pixel_aspect.v_spacing);
      is_video_track_encrypted_ = entry.sinf.info.track_encryption.is_encrypted;
      DMX_LOG("is_video_track_encrypted_: %d\n", is_video_track_encrypted_);
      video_config_.Initialize(kCodecH264, H264PROFILE_MAIN,  VideoFrameFormat::YV12,
                              coded_size, visible_rect, natural_size,
                              
                              
                              NULL, 0, is_video_track_encrypted_, true);
      has_video_ = true;
      video_track_id_ = track->header.track_id;
    }
  }

  

  if (moov_->extends.header.fragment_duration > 0) {
    duration_ = MicrosecondsFromRational(moov_->extends.header.fragment_duration,
                                     moov_->header.timescale);
  } else if (moov_->header.duration > 0 &&
             moov_->header.duration != kuint64max) {
    duration_ = MicrosecondsFromRational(moov_->header.duration,
                                     moov_->header.timescale);
  } else {
    duration_ = InfiniteMicroseconds;
  }

  
  

  return true;
}

Microseconds
MP4Demuxer::Duration() const {
  return duration_;
}

bool MP4Demuxer::ParseMoof(BoxReader* reader) {
  RCHECK(state_ < kError);
  RCHECK(moov_.get());  
  MovieFragment moof;
  RCHECK(moof.Parse(reader));
  RCHECK(runs_->Init(moof));
  
  ChangeState(kEmittingSamples);
  return true;
}

bool MP4Demuxer::ParseBox() {
  RCHECK(state_ < kError);
  bool err = false;
  nsAutoPtr<BoxReader> reader(BoxReader::ReadTopLevelBox(stream_,
                                                         stream_offset_,
                                                         &err));
  if (!reader || err) {
    DMX_LOG("Failed to read box at offset=%lld", stream_offset_);
    return false;
  }
  string type = FourCCToString(reader->type());

  DMX_LOG("offset=%lld version=0x%x flags=0x%x size=%d",
      stream_offset_, (uint32_t)reader->version(),
      reader->flags(), reader->size());

  if (reader->type() == FOURCC_MOOV) {
    DMX_LOG("ParseMoov\n");
    if (!ParseMoov(reader.get())) {
      DMX_LOG("ParseMoov failed\n");
      return false;
    }
  } else if (reader->type() == FOURCC_MOOF) {
    DMX_LOG("MOOF encountered\n.");
    moof_head_ = stream_offset_;
    if (!ParseMoof(reader.get())) {
      DMX_LOG("ParseMoof failed\n");
      return false;
    }
    mdat_tail_ = stream_offset_ + reader->size();
  }

  stream_offset_ += reader->size();

  return true;
}

bool MP4Demuxer::EmitSample(nsAutoPtr<MP4Sample>* sample) {
  if (!runs_->IsRunValid()) {
    ChangeState(kParsingBoxes);
    
    return true;
  }

  if (!runs_->IsSampleValid()) {
    runs_->AdvanceRun();
    return true;
  }

  bool audio = has_audio_ && audio_track_id_ == runs_->track_id();
  bool video = has_video_ && video_track_id_ == runs_->track_id();

  
  if (!audio && !video)
    runs_->AdvanceRun();

  
  
  
  
  
  
  
  if (runs_->AuxInfoNeedsToBeCached()) {
    int64_t aux_info_offset = runs_->aux_info_offset() + moof_head_;
    if (stream_->Length() - aux_info_offset < runs_->aux_info_size()) {
      return false;
    }

    return runs_->CacheAuxInfo(stream_, moof_head_);
  }

  nsAutoPtr<DecryptConfig> decrypt_config;
  std::vector<SubsampleEntry> subsamples;
  if (runs_->is_encrypted()) {
    runs_->GetDecryptConfig(decrypt_config);
    subsamples = decrypt_config->subsamples();
  }

  nsAutoPtr<vector<uint8_t>> frame_buf(new vector<uint8_t>());
  const int64_t sample_offset = runs_->sample_offset() + moof_head_;
  StreamReader reader(stream_, sample_offset, runs_->sample_size());
  reader.ReadVec(frame_buf, runs_->sample_size());

  if (video) {
    if (!PrepareAVCBuffer(runs_->video_description().avcc,
                          frame_buf, &subsamples)) {
      DMX_LOG("Failed to prepare AVC sample for decode\n");
      return false;
    }
  }

  if (audio) {
    if (!PrepareAACBuffer(runs_->audio_description().esds.aac,
                          frame_buf, &subsamples)) {
      DMX_LOG("Failed to prepare AAC sample for decode\n");
      return false;
    }
  }

  const bool is_encrypted = (audio && is_audio_track_encrypted_) ||
                            (video && is_video_track_encrypted_);
  assert(runs_->is_encrypted() == is_encrypted);
  if (decrypt_config) {
    if (!subsamples.empty()) {
      
      decrypt_config = new DecryptConfig(decrypt_config->key_id(),
                                         decrypt_config->iv(),
                                         decrypt_config->data_offset(),
                                         subsamples);
    }
    
  } else if (is_encrypted) {
    
    
    decrypt_config = new DecryptConfig("1", "", 0, std::vector<SubsampleEntry>());
  }

  assert(audio || video);
  *sample = new MP4Sample(runs_->dts(),
                          runs_->cts(),
                          runs_->duration(),
                          sample_offset,
                          frame_buf.forget(),
                          audio ? kAudio : kVideo,
                          decrypt_config.forget(),
                          runs_->is_keyframe());
  runs_->AdvanceSample();
  return true;
}

bool MP4Demuxer::PrepareAVCBuffer(
    const AVCDecoderConfigurationRecord& avc_config,
    std::vector<uint8_t>* frame_buf,
    std::vector<SubsampleEntry>* subsamples) const {
  
  
  
  
  
  RCHECK(AVC::ConvertFrameToAnnexB(avc_config.length_size, frame_buf));
  if (!subsamples->empty()) {
    const int nalu_size_diff = 4 - avc_config.length_size;
    size_t expected_size = runs_->sample_size() +
        subsamples->size() * nalu_size_diff;
    RCHECK(frame_buf->size() == expected_size);
    for (size_t i = 0; i < subsamples->size(); i++)
      (*subsamples)[i].clear_bytes += nalu_size_diff;
  }

  if (runs_->is_keyframe()) {
    
    
    
    std::vector<uint8_t> param_sets;
    RCHECK(AVC::ConvertConfigToAnnexB(avc_config, &param_sets));
    frame_buf->insert(frame_buf->begin(),
                      param_sets.begin(), param_sets.end());
    if (!subsamples->empty())
      (*subsamples)[0].clear_bytes += param_sets.size();
  }
  return true;
}

bool MP4Demuxer::PrepareAACBuffer(const AAC& aac_config,
                                  std::vector<uint8_t>* frame_buf,
                                  std::vector<SubsampleEntry>* subsamples) const {
  
  RCHECK(aac_config.ConvertEsdsToADTS(frame_buf));

  
  
  if (subsamples->empty()) {
    SubsampleEntry entry;
    entry.clear_bytes = AAC::kADTSHeaderSize;
    entry.cypher_bytes = frame_buf->size() - AAC::kADTSHeaderSize;
    subsamples->push_back(entry);
  } else {
    (*subsamples)[0].clear_bytes += AAC::kADTSHeaderSize;
  }
  return true;
}


bool MP4Demuxer::Demux(nsAutoPtr<MP4Sample>* sample,
                       bool* end_of_stream)
{
  RCHECK(state_ < kError);
  assert(state_ > kWaitingForInit);
  *end_of_stream = false;

  const int64_t length = stream_->Length();
  bool ok = true;
  while (ok) {
    if (state_ == kParsingBoxes) {
      if (stream_offset_ < length) {
        ok = ParseBox();
      } else {
        DMX_LOG("End of stream reached.\n");
        *end_of_stream = true;
        break;
      }
    } else {
      DCHECK_EQ(kEmittingSamples, state_);
      ok = EmitSample(sample);
      if (ok && *sample) {
        
        break;
      }
    }
  }

  if (!ok) {
    DMX_LOG("Error demuxing stream\n");
    ChangeState(kError);
    return false;
  }

  return true;
}

void MP4Demuxer::ChangeState(State new_state) {
  DMX_LOG("Demuxer changing state: %d\n", new_state);
  state_ = new_state;
  if (state_ == kError) {
    Reset();
  }
}

const AudioDecoderConfig&
MP4Demuxer::AudioConfig() const
{
  return audio_config_;
}

const VideoDecoderConfig&
MP4Demuxer::VideoConfig() const
{
  return video_config_;
}

bool
MP4Demuxer::HasAudio() const
{
  return has_audio_;
}

bool
MP4Demuxer::HasVideo() const
{
  return has_video_;
}

bool
MP4Demuxer::CanSeek() const
{
  return can_seek_;
}

} 
