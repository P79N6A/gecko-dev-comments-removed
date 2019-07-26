









#include "webrtc/modules/audio_coding/main/source/audio_coding_module_impl.h"

#include <assert.h>
#include <stdlib.h>

#include <algorithm>  

#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/acm2/call_statistics.h"
#include "webrtc/modules/audio_coding/main/source/acm_dtmf_detection.h"
#include "webrtc/modules/audio_coding/main/source/acm_generic_codec.h"
#include "webrtc/modules/audio_coding/main/source/acm_resampler.h"
#include "webrtc/modules/audio_coding/main/acm2/nack.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

namespace webrtc {

namespace acm1 {

enum {
  kACMToneEnd = 999
};


enum {
  kMaxPacketSize = 2560
};




enum {
  kNumFecFragmentationVectors = 2,
  kMaxNumFragmentationVectors = 3
};

static const uint32_t kMaskTimestamp = 0x03ffffff;
static const int kDefaultTimestampDiff = 960;  



static const int kNackThresholdPackets = 2;

namespace {

bool IsCodecRED(const CodecInst* codec) {
  return (STR_CASE_CMP(codec->plname, "RED") == 0);
}

bool IsCodecRED(int index) {
  return (IsCodecRED(&ACMCodecDB::database_[index]));
}

bool IsCodecCN(const CodecInst* codec) {
  return (STR_CASE_CMP(codec->plname, "CN") == 0);
}

bool IsCodecCN(int index) {
  return (IsCodecCN(&ACMCodecDB::database_[index]));
}


int DownMix(const AudioFrame& frame, int length_out_buff, int16_t* out_buff) {
  if (length_out_buff < frame.samples_per_channel_) {
    return -1;
  }
  for (int n = 0; n < frame.samples_per_channel_; ++n)
    out_buff[n] = (frame.data_[2 * n] + frame.data_[2 * n + 1]) >> 1;
  return 0;
}


int UpMix(const AudioFrame& frame, int length_out_buff, int16_t* out_buff) {
  if (length_out_buff < frame.samples_per_channel_) {
    return -1;
  }
  for (int n = frame.samples_per_channel_ - 1; n >= 0; --n) {
    out_buff[2 * n + 1] = frame.data_[n];
    out_buff[2 * n] = frame.data_[n];
  }
  return 0;
}



int TimestampLessThan(uint32_t t1, uint32_t t2) {
  uint32_t kHalfFullRange = static_cast<uint32_t>(0xFFFFFFFF) / 2;
  if (t1 == t2) {
    return 0;
  } else if (t1 < t2) {
    if (t2 - t1 < kHalfFullRange)
      return 1;
    return 0;
  } else {
    if (t1 - t2 < kHalfFullRange)
      return 0;
    return 1;
  }
}

}  

AudioCodingModuleImpl::AudioCodingModuleImpl(const int32_t id, Clock* clock)
    : packetization_callback_(NULL),
      id_(id),
      last_timestamp_(0xD87F3F9F),
      last_in_timestamp_(0xD87F3F9F),
      send_codec_inst_(),
      cng_nb_pltype_(255),
      cng_wb_pltype_(255),
      cng_swb_pltype_(255),
      cng_fb_pltype_(255),
      red_pltype_(255),
      vad_enabled_(false),
      dtx_enabled_(false),
      vad_mode_(VADNormal),
      stereo_receive_registered_(false),
      stereo_send_(false),
      prev_received_channel_(0),
      expected_channels_(1),
      current_send_codec_idx_(-1),
      current_receive_codec_idx_(-1),
      send_codec_registered_(false),
      acm_crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      vad_callback_(NULL),
      last_recv_audio_codec_pltype_(255),
      is_first_red_(true),
      fec_enabled_(false),
      last_fec_timestamp_(0),
      receive_red_pltype_(255),
      previous_pltype_(255),
      dummy_rtp_header_(NULL),
      recv_pl_frame_size_smpls_(0),
      receiver_initialized_(false),
      dtmf_detector_(NULL),
      dtmf_callback_(NULL),
      last_detected_tone_(kACMToneEnd),
      callback_crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      secondary_send_codec_inst_(),
      initial_delay_ms_(0),
      num_packets_accumulated_(0),
      num_bytes_accumulated_(0),
      accumulated_audio_ms_(0),
      first_payload_received_(false),
      last_incoming_send_timestamp_(0),
      track_neteq_buffer_(false),
      playout_ts_(0),
      av_sync_(false),
      last_timestamp_diff_(kDefaultTimestampDiff),
      last_sequence_number_(0),
      last_ssrc_(0),
      last_packet_was_sync_(false),
      clock_(clock),
      nack_(),
      nack_enabled_(false) {

  
  
  const char no_name[] = "noCodecRegistered";
  strncpy(send_codec_inst_.plname, no_name, RTP_PAYLOAD_NAME_SIZE - 1);
  send_codec_inst_.pltype = -1;

  strncpy(secondary_send_codec_inst_.plname, no_name,
          RTP_PAYLOAD_NAME_SIZE - 1);
  secondary_send_codec_inst_.pltype = -1;

  for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++) {
    codecs_[i] = NULL;
    registered_pltypes_[i] = -1;
    stereo_receive_[i] = false;
    slave_codecs_[i] = NULL;
    mirror_codec_idx_[i] = -1;
  }

  neteq_.set_id(id_);

  
  red_buffer_ = new uint8_t[MAX_PAYLOAD_SIZE_BYTE];

  
  
  
  
  
  
  
  
  
  
  
  
  fragmentation_.VerifyAndAllocateFragmentationHeader(
      kMaxNumFragmentationVectors);

  
  
  for (int i = (ACMCodecDB::kNumCodecs - 1); i >= 0; i--) {
    if (IsCodecRED(i)) {
      red_pltype_ = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
    } else if (IsCodecCN(i)) {
      if (ACMCodecDB::database_[i].plfreq == 8000) {
        cng_nb_pltype_ = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
      } else if (ACMCodecDB::database_[i].plfreq == 16000) {
        cng_wb_pltype_ = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
      } else if (ACMCodecDB::database_[i].plfreq == 32000) {
        cng_swb_pltype_ = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
      } else if (ACMCodecDB::database_[i].plfreq == 48000) {
        cng_fb_pltype_ = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
      }
    }
  }

  if (InitializeReceiverSafe() < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Cannot initialize receiver");
  }
  WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceAudioCoding, id, "Created");
}

AudioCodingModuleImpl::~AudioCodingModuleImpl() {
  {
    CriticalSectionScoped lock(acm_crit_sect_);
    current_send_codec_idx_ = -1;

    for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++) {
      if (codecs_[i] != NULL) {
        
        
        
        if (slave_codecs_[i] == codecs_[i]) {
          slave_codecs_[i] = NULL;
        }

        
        assert(mirror_codec_idx_[i] > -1);
        if (codecs_[mirror_codec_idx_[i]] != NULL) {
          delete codecs_[mirror_codec_idx_[i]];
          codecs_[mirror_codec_idx_[i]] = NULL;
        }

        codecs_[i] = NULL;
      }

      if (slave_codecs_[i] != NULL) {
        
        assert(mirror_codec_idx_[i] > -1);
        if (slave_codecs_[mirror_codec_idx_[i]] != NULL) {
          delete slave_codecs_[mirror_codec_idx_[i]];
          slave_codecs_[mirror_codec_idx_[i]] = NULL;
        }
        slave_codecs_[i] = NULL;
      }
    }

    if (dtmf_detector_ != NULL) {
      delete dtmf_detector_;
      dtmf_detector_ = NULL;
    }
    if (dummy_rtp_header_ != NULL) {
      delete dummy_rtp_header_;
      dummy_rtp_header_ = NULL;
    }
    if (red_buffer_ != NULL) {
      delete[] red_buffer_;
      red_buffer_ = NULL;
    }
  }

  delete callback_crit_sect_;
  callback_crit_sect_ = NULL;

  delete acm_crit_sect_;
  acm_crit_sect_ = NULL;
  WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceAudioCoding, id_,
               "Destroyed");
}

int32_t AudioCodingModuleImpl::ChangeUniqueId(const int32_t id) {
  {
    CriticalSectionScoped lock(acm_crit_sect_);
    id_ = id;

    for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++) {
      if (codecs_[i] != NULL) {
        codecs_[i]->SetUniqueID(id);
      }
    }
  }

  neteq_.set_id(id_);
  return 0;
}



int32_t AudioCodingModuleImpl::TimeUntilNextProcess() {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("TimeUntilNextProcess")) {
    return -1;
  }
  return codecs_[current_send_codec_idx_]->SamplesLeftToEncode() /
      (send_codec_inst_.plfreq / 1000);
}

int32_t AudioCodingModuleImpl::Process() {
  bool dual_stream;
  {
    CriticalSectionScoped lock(acm_crit_sect_);
    dual_stream = (secondary_encoder_.get() != NULL);
  }
  if (dual_stream) {
    return ProcessDualStream();
  }
  return ProcessSingleStream();
}

int AudioCodingModuleImpl::EncodeFragmentation(int fragmentation_index,
                                               int payload_type,
                                               uint32_t current_timestamp,
                                               ACMGenericCodec* encoder,
                                               uint8_t* stream) {
  int16_t len_bytes = MAX_PAYLOAD_SIZE_BYTE;
  uint32_t rtp_timestamp;
  WebRtcACMEncodingType encoding_type;
  if (encoder->Encode(stream, &len_bytes, &rtp_timestamp, &encoding_type) < 0) {
    return -1;
  }
  assert(encoding_type == kActiveNormalEncoded);
  assert(len_bytes > 0);

  fragmentation_.fragmentationLength[fragmentation_index] = len_bytes;
  fragmentation_.fragmentationPlType[fragmentation_index] = payload_type;
  fragmentation_.fragmentationTimeDiff[fragmentation_index] =
      static_cast<uint16_t>(current_timestamp - rtp_timestamp);
  fragmentation_.fragmentationVectorSize++;
  return len_bytes;
}






int AudioCodingModuleImpl::ProcessDualStream() {
  uint8_t stream[kMaxNumFragmentationVectors * MAX_PAYLOAD_SIZE_BYTE];
  uint32_t current_timestamp;
  int16_t length_bytes = 0;
  RTPFragmentationHeader my_fragmentation;

  uint8_t my_red_payload_type;

  {
    CriticalSectionScoped lock(acm_crit_sect_);
    
    if (!HaveValidEncoder("ProcessDualStream") ||
        secondary_encoder_.get() == NULL) {
      return -1;
    }
    ACMGenericCodec* primary_encoder = codecs_[current_send_codec_idx_];
    
    bool primary_ready_to_encode = primary_encoder->HasFrameToEncode();
    
    bool secondary_ready_to_encode = secondary_encoder_->HasFrameToEncode();

    if (!primary_ready_to_encode && !secondary_ready_to_encode) {
      
      return 0;
    }
    int len_bytes_previous_secondary = static_cast<int>(
        fragmentation_.fragmentationLength[2]);
    assert(len_bytes_previous_secondary <= MAX_PAYLOAD_SIZE_BYTE);
    bool has_previous_payload = len_bytes_previous_secondary > 0;

    uint32_t primary_timestamp = primary_encoder->EarliestTimestamp();
    uint32_t secondary_timestamp = secondary_encoder_->EarliestTimestamp();

    if (!has_previous_payload && !primary_ready_to_encode &&
        secondary_ready_to_encode) {
      
      
      int16_t len_bytes = MAX_PAYLOAD_SIZE_BYTE;
      WebRtcACMEncodingType encoding_type;
      if (secondary_encoder_->Encode(red_buffer_, &len_bytes,
                                     &last_fec_timestamp_,
                                     &encoding_type) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "ProcessDual(): Encoding of secondary encoder Failed");
        return -1;
      }
      assert(len_bytes > 0);
      assert(encoding_type == kActiveNormalEncoded);
      assert(len_bytes <= MAX_PAYLOAD_SIZE_BYTE);
      fragmentation_.fragmentationLength[2] = len_bytes;
      return 0;
    }

    
    
    int index_primary = -1;
    int index_secondary = -2;
    int index_previous_secondary = -3;

    if (primary_ready_to_encode) {
      index_primary = secondary_ready_to_encode ?
          TimestampLessThan(primary_timestamp, secondary_timestamp) : 0;
      index_primary += has_previous_payload ?
          TimestampLessThan(primary_timestamp, last_fec_timestamp_) : 0;
    }

    if (secondary_ready_to_encode) {
      
      
      index_secondary = primary_ready_to_encode ?
          (1 - TimestampLessThan(primary_timestamp, secondary_timestamp)) : 0;
    }

    if (has_previous_payload) {
      index_previous_secondary = primary_ready_to_encode ?
          (1 - TimestampLessThan(primary_timestamp, last_fec_timestamp_)) : 0;
      
      
      index_previous_secondary += secondary_ready_to_encode ? 1 : 0;
    }

    
    assert(index_primary != index_secondary);
    assert(index_primary != index_previous_secondary);
    assert(index_secondary != index_previous_secondary);

    
    assert(index_primary == 0 || index_secondary == 0 ||
           index_previous_secondary == 0);

    
    if (index_primary == 0) {
      current_timestamp = primary_timestamp;
    } else if (index_secondary == 0) {
      current_timestamp = secondary_timestamp;
    } else {
      current_timestamp = last_fec_timestamp_;
    }

    fragmentation_.fragmentationVectorSize = 0;
    if (has_previous_payload) {
      assert(index_previous_secondary >= 0 &&
             index_previous_secondary < kMaxNumFragmentationVectors);
      assert(len_bytes_previous_secondary <= MAX_PAYLOAD_SIZE_BYTE);
      memcpy(&stream[index_previous_secondary * MAX_PAYLOAD_SIZE_BYTE],
             red_buffer_, sizeof(stream[0]) * len_bytes_previous_secondary);
      fragmentation_.fragmentationLength[index_previous_secondary] =
          len_bytes_previous_secondary;
      fragmentation_.fragmentationPlType[index_previous_secondary] =
          secondary_send_codec_inst_.pltype;
      fragmentation_.fragmentationTimeDiff[index_previous_secondary] =
          static_cast<uint16_t>(current_timestamp - last_fec_timestamp_);
      fragmentation_.fragmentationVectorSize++;
    }

    if (primary_ready_to_encode) {
      assert(index_primary >= 0 && index_primary < kMaxNumFragmentationVectors);
      int i = index_primary * MAX_PAYLOAD_SIZE_BYTE;
      if (EncodeFragmentation(index_primary, send_codec_inst_.pltype,
                              current_timestamp, primary_encoder,
                              &stream[i]) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "ProcessDualStream(): Encoding of primary encoder Failed");
        return -1;
      }
    }

    if (secondary_ready_to_encode) {
      assert(index_secondary >= 0 &&
             index_secondary < kMaxNumFragmentationVectors - 1);
      int i = index_secondary * MAX_PAYLOAD_SIZE_BYTE;
      if (EncodeFragmentation(index_secondary,
                              secondary_send_codec_inst_.pltype,
                              current_timestamp, secondary_encoder_.get(),
                              &stream[i]) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "ProcessDualStream(): Encoding of secondary encoder "
                     "Failed");
        return -1;
      }
    }
    
    my_fragmentation.CopyFrom(fragmentation_);
    my_red_payload_type = red_pltype_;
    length_bytes = 0;
    for (int n = 0; n < fragmentation_.fragmentationVectorSize; n++) {
      length_bytes += fragmentation_.fragmentationLength[n];
    }
  }

  {
    CriticalSectionScoped lock(callback_crit_sect_);
    if (packetization_callback_ != NULL) {
      
      if (packetization_callback_->SendData(kAudioFrameSpeech,
                                            my_red_payload_type,
                                            current_timestamp, stream,
                                            length_bytes,
                                            &my_fragmentation) < 0) {
        return -1;
      }
    }
  }

  {
    CriticalSectionScoped lock(acm_crit_sect_);
    
    ResetFragmentation(0);
  }
  return 0;
}


int AudioCodingModuleImpl::ProcessSingleStream() {
  
  uint8_t stream[2 * MAX_PAYLOAD_SIZE_BYTE];
  int16_t length_bytes = 2 * MAX_PAYLOAD_SIZE_BYTE;
  int16_t red_length_bytes = length_bytes;
  uint32_t rtp_timestamp;
  int16_t status;
  WebRtcACMEncodingType encoding_type;
  FrameType frame_type = kAudioFrameSpeech;
  uint8_t current_payload_type = 0;
  bool has_data_to_send = false;
  bool fec_active = false;
  RTPFragmentationHeader my_fragmentation;

  
  {
    CriticalSectionScoped lock(acm_crit_sect_);
    
    if (!HaveValidEncoder("ProcessSingleStream")) {
      return -1;
    }
    status = codecs_[current_send_codec_idx_]->Encode(stream, &length_bytes,
                                                      &rtp_timestamp,
                                                      &encoding_type);
    if (status < 0) {
      
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "ProcessSingleStream(): Encoding Failed");
      length_bytes = 0;
      return -1;
    } else if (status == 0) {
      
      return 0;
    } else {
      switch (encoding_type) {
        case kNoEncoding: {
          current_payload_type = previous_pltype_;
          frame_type = kFrameEmpty;
          length_bytes = 0;
          break;
        }
        case kActiveNormalEncoded:
        case kPassiveNormalEncoded: {
          current_payload_type = static_cast<uint8_t>(send_codec_inst_.pltype);
          frame_type = kAudioFrameSpeech;
          break;
        }
        case kPassiveDTXNB: {
          current_payload_type = cng_nb_pltype_;
          frame_type = kAudioFrameCN;
          is_first_red_ = true;
          break;
        }
        case kPassiveDTXWB: {
          current_payload_type = cng_wb_pltype_;
          frame_type = kAudioFrameCN;
          is_first_red_ = true;
          break;
        }
        case kPassiveDTXSWB: {
          current_payload_type = cng_swb_pltype_;
          frame_type = kAudioFrameCN;
          is_first_red_ = true;
          break;
        }
        case kPassiveDTXFB: {
          current_payload_type = cng_fb_pltype_;
          frame_type = kAudioFrameCN;
          is_first_red_ = true;
          break;
        }
      }
      has_data_to_send = true;
      previous_pltype_ = current_payload_type;

      
      
      
      if ((fec_enabled_) &&
          ((encoding_type == kActiveNormalEncoded) ||
              (encoding_type == kPassiveNormalEncoded))) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        fec_active = true;

        has_data_to_send = false;
        
        if (!is_first_red_) {
          
          
          memcpy(stream + fragmentation_.fragmentationOffset[1], red_buffer_,
                 fragmentation_.fragmentationLength[1]);
          
          
          uint16_t time_since_last = static_cast<uint16_t>(rtp_timestamp -
                                                           last_fec_timestamp_);

          
          fragmentation_.fragmentationPlType[1] =
              fragmentation_.fragmentationPlType[0];
          fragmentation_.fragmentationTimeDiff[1] = time_since_last;
          has_data_to_send = true;
        }

        
        fragmentation_.fragmentationLength[0] = length_bytes;

        
        fragmentation_.fragmentationPlType[0] = current_payload_type;
        last_fec_timestamp_ = rtp_timestamp;

        
        red_length_bytes = length_bytes;

        
        
        
        
        length_bytes = static_cast<int16_t>(
            fragmentation_.fragmentationLength[0] +
            fragmentation_.fragmentationLength[1]);

        
        
        
        
        if (codecs_[current_send_codec_idx_]->GetRedPayload(
            red_buffer_,
            &red_length_bytes) == -1) {
          
          
          memcpy(red_buffer_, stream, red_length_bytes);
        }

        is_first_red_ = false;
        
        current_payload_type = red_pltype_;
        
        fragmentation_.fragmentationVectorSize = kNumFecFragmentationVectors;

        
        my_fragmentation.CopyFrom(fragmentation_);
        
        fragmentation_.fragmentationLength[1] = red_length_bytes;
      }
    }
  }

  if (has_data_to_send) {
    CriticalSectionScoped lock(callback_crit_sect_);

    if (packetization_callback_ != NULL) {
      if (fec_active) {
        
        packetization_callback_->SendData(frame_type, current_payload_type,
                                          rtp_timestamp, stream,
                                          length_bytes,
                                          &my_fragmentation);
      } else {
        
        packetization_callback_->SendData(frame_type, current_payload_type,
                                          rtp_timestamp, stream,
                                          length_bytes, NULL);
      }
    }

    if (vad_callback_ != NULL) {
      
      vad_callback_->InFrameType(static_cast<int16_t>(encoding_type));
    }
  }
  return length_bytes;
}






int32_t AudioCodingModuleImpl::InitializeSender() {
  CriticalSectionScoped lock(acm_crit_sect_);

  
  send_codec_registered_ = false;
  current_send_codec_idx_ = -1;
  send_codec_inst_.plname[0] = '\0';

  
  for (int id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if (codecs_[id] != NULL) {
      codecs_[id]->DestructEncoder();
    }
  }

  
  is_first_red_ = true;
  if (fec_enabled_ || secondary_encoder_.get() != NULL) {
    if (red_buffer_ != NULL) {
      memset(red_buffer_, 0, MAX_PAYLOAD_SIZE_BYTE);
    }
    if (fec_enabled_) {
      ResetFragmentation(kNumFecFragmentationVectors);
    } else {
      ResetFragmentation(0);
    }
  }

  return 0;
}

int32_t AudioCodingModuleImpl::ResetEncoder() {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (!HaveValidEncoder("ResetEncoder")) {
    return -1;
  }
  return codecs_[current_send_codec_idx_]->ResetEncoder();
}

void AudioCodingModuleImpl::UnregisterSendCodec() {
  CriticalSectionScoped lock(acm_crit_sect_);
  send_codec_registered_ = false;
  current_send_codec_idx_ = -1;
  
  if (secondary_encoder_.get() != NULL)
    secondary_encoder_.reset();
  return;
}

ACMGenericCodec* AudioCodingModuleImpl::CreateCodec(const CodecInst& codec) {
  ACMGenericCodec* my_codec = NULL;

  my_codec = ACMCodecDB::CreateCodecInstance(&codec);
  if (my_codec == NULL) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "ACMCodecDB::CreateCodecInstance() failed in CreateCodec()");
    return my_codec;
  }
  my_codec->SetUniqueID(id_);
  my_codec->SetNetEqDecodeLock(neteq_.DecodeLock());

  return my_codec;
}


static int IsValidSendCodec(const CodecInst& send_codec,
                            bool is_primary_encoder,
                            int acm_id,
                            int* mirror_id) {
  if ((send_codec.channels != 1) && (send_codec.channels != 2)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, acm_id,
                 "Wrong number of channels (%d, only mono and stereo are "
                 "supported) for %s encoder", send_codec.channels,
                 is_primary_encoder ? "primary" : "secondary");
    return -1;
  }

  int codec_id = ACMCodecDB::CodecNumber(&send_codec, mirror_id);
  if (codec_id < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, acm_id,
                 "Invalid settings for the send codec.");
    return -1;
  }

  
  
  
  if (!ACMCodecDB::ValidPayloadType(send_codec.pltype)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, acm_id,
                 "Invalid payload-type %d for %s.", send_codec.pltype,
                 send_codec.plname);
    return -1;
  }

  
  if (!STR_CASE_CMP(send_codec.plname, "telephone-event")) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, acm_id,
                 "telephone-event cannot be a send codec");
    *mirror_id = -1;
    return -1;
  }

  if (ACMCodecDB::codec_settings_[codec_id].channel_support
      < send_codec.channels) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, acm_id,
                 "%d number of channels not supportedn for %s.",
                 send_codec.channels, send_codec.plname);
    *mirror_id = -1;
    return -1;
  }

  if (!is_primary_encoder) {
    
    
    if (IsCodecRED(&send_codec)) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, acm_id,
                   "RED cannot be secondary codec");
      *mirror_id = -1;
      return -1;
    }

    if (IsCodecCN(&send_codec)) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, acm_id,
                   "DTX cannot be secondary codec");
      *mirror_id = -1;
      return -1;
    }
  }
  return codec_id;
}

int AudioCodingModuleImpl::RegisterSecondarySendCodec(
    const CodecInst& send_codec) {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (!send_codec_registered_) {
    return -1;
  }
  
  if (send_codec.plfreq != send_codec_inst_.plfreq) {
    return -1;
  }
  int mirror_id;
  int codec_id = IsValidSendCodec(send_codec, false, id_, &mirror_id);
  if (codec_id < 0) {
    return -1;
  }
  ACMGenericCodec* encoder = CreateCodec(send_codec);
  WebRtcACMCodecParams codec_params;
  
  
  memcpy(&(codec_params.codec_inst), &send_codec, sizeof(CodecInst));
  codec_params.enable_vad = false;
  codec_params.enable_dtx = false;
  codec_params.vad_mode = VADNormal;
  
  if (encoder->InitEncoder(&codec_params, true) < 0) {
    
    delete encoder;
    return -1;
  }
  secondary_encoder_.reset(encoder);
  memcpy(&secondary_send_codec_inst_, &send_codec, sizeof(send_codec));

  
  SetVADSafe(false, false, VADNormal);

  
  if (red_buffer_) {
    memset(red_buffer_, 0, MAX_PAYLOAD_SIZE_BYTE);
  }
  ResetFragmentation(0);
  return 0;
}

void AudioCodingModuleImpl::UnregisterSecondarySendCodec() {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (secondary_encoder_.get() == NULL) {
    return;
  }
  secondary_encoder_.reset();
  ResetFragmentation(0);
}

int AudioCodingModuleImpl::SecondarySendCodec(
    CodecInst* secondary_codec) const {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (secondary_encoder_.get() == NULL) {
    return -1;
  }
  memcpy(secondary_codec, &secondary_send_codec_inst_,
         sizeof(secondary_send_codec_inst_));
  return 0;
}


int32_t AudioCodingModuleImpl::RegisterSendCodec(
    const CodecInst& send_codec) {
  int mirror_id;
  int codec_id = IsValidSendCodec(send_codec, true, id_, &mirror_id);

  CriticalSectionScoped lock(acm_crit_sect_);

  
  if (codec_id < 0) {
    if (!send_codec_registered_) {
      
      current_send_codec_idx_ = -1;
    }
    return -1;
  }

  
  
  if (IsCodecRED(&send_codec)) {
    
    
    
    if (!ACMCodecDB::ValidPayloadType(send_codec.pltype)) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "Invalid payload-type %d for %s.", send_codec.pltype,
                   send_codec.plname);
      return -1;
    }
    
    red_pltype_ = static_cast<uint8_t>(send_codec.pltype);
    return 0;
  }

  
  
  if (IsCodecCN(&send_codec)) {
    
    switch (send_codec.plfreq) {
      case 8000: {
        cng_nb_pltype_ = static_cast<uint8_t>(send_codec.pltype);
        break;
      }
      case 16000: {
        cng_wb_pltype_ = static_cast<uint8_t>(send_codec.pltype);
        break;
      }
      case 32000: {
        cng_swb_pltype_ = static_cast<uint8_t>(send_codec.pltype);
        break;
      }
      case 48000: {
        cng_fb_pltype_ = static_cast<uint8_t>(send_codec.pltype);
        break;
      }
      default: {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "RegisterSendCodec() failed, invalid frequency for CNG "
                     "registration");
        return -1;
      }
    }
    return 0;
  }

  
  if (send_codec.channels == 2) {
    stereo_send_ = true;
    if (vad_enabled_ || dtx_enabled_) {
      WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, id_,
                   "VAD/DTX is turned off, not supported when sending stereo.");
    }
    vad_enabled_ = false;
    dtx_enabled_ = false;
  } else {
    stereo_send_ = false;
  }

  
  bool is_send_codec;
  if (send_codec_registered_) {
    int send_codec_mirror_id;
    int send_codec_id = ACMCodecDB::CodecNumber(&send_codec_inst_,
                                                &send_codec_mirror_id);
    assert(send_codec_id >= 0);
    is_send_codec = (send_codec_id == codec_id) ||
        (mirror_id == send_codec_mirror_id);
  } else {
    is_send_codec = false;
  }

  
  
  
  if (secondary_encoder_.get() != NULL &&
      secondary_send_codec_inst_.plfreq != send_codec.plfreq) {
    secondary_encoder_.reset();
    ResetFragmentation(0);
  }

  
  if (!is_send_codec) {
    if (codecs_[mirror_id] == NULL) {
      codecs_[mirror_id] = CreateCodec(send_codec);
      if (codecs_[mirror_id] == NULL) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Cannot Create the codec");
        return -1;
      }
      mirror_codec_idx_[mirror_id] = mirror_id;
    }

    if (mirror_id != codec_id) {
      codecs_[codec_id] = codecs_[mirror_id];
      mirror_codec_idx_[codec_id] = mirror_id;
    }

    ACMGenericCodec* codec_ptr = codecs_[codec_id];
    WebRtcACMCodecParams codec_params;

    memcpy(&(codec_params.codec_inst), &send_codec, sizeof(CodecInst));
    codec_params.enable_vad = vad_enabled_;
    codec_params.enable_dtx = dtx_enabled_;
    codec_params.vad_mode = vad_mode_;
    
    if (codec_ptr->InitEncoder(&codec_params, true) < 0) {
      

      
      
      if (!send_codec_registered_) {
        current_send_codec_idx_ = -1;
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Cannot Initialize the encoder No Encoder is registered");
      } else {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Cannot Initialize the encoder, continue encoding with "
                     "the previously registered codec");
      }
      return -1;
    }

    
    dtx_enabled_ = codec_params.enable_dtx;
    vad_enabled_ = codec_params.enable_vad;
    vad_mode_ = codec_params.vad_mode;

    
    if (send_codec_registered_) {
      
      
      is_first_red_ = true;

      codec_ptr->SetVAD(&dtx_enabled_, &vad_enabled_, &vad_mode_);
    }

    current_send_codec_idx_ = codec_id;
    send_codec_registered_ = true;
    memcpy(&send_codec_inst_, &send_codec, sizeof(CodecInst));
    previous_pltype_ = send_codec_inst_.pltype;
    return 0;
  } else {
    
    
    
    bool force_init = false;

    if (mirror_id != codec_id) {
      codecs_[codec_id] = codecs_[mirror_id];
      mirror_codec_idx_[codec_id] = mirror_id;
    }

    
    if (send_codec.pltype != send_codec_inst_.pltype) {
      
      
      
      if (!ACMCodecDB::ValidPayloadType(send_codec.pltype)) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Out of range payload type");
        return -1;
      }
    }

    
    
    
    
    
    if (send_codec_inst_.plfreq != send_codec.plfreq) {
      force_init = true;

      
      is_first_red_ = true;
    }

    
    
    if (send_codec_inst_.pacsize != send_codec.pacsize) {
      force_init = true;
    }
    if (send_codec_inst_.channels != send_codec.channels) {
      force_init = true;
    }

    if (force_init) {
      WebRtcACMCodecParams codec_params;

      memcpy(&(codec_params.codec_inst), &send_codec, sizeof(CodecInst));
      codec_params.enable_vad = vad_enabled_;
      codec_params.enable_dtx = dtx_enabled_;
      codec_params.vad_mode = vad_mode_;

      
      if (codecs_[current_send_codec_idx_]->InitEncoder(&codec_params,
                                                        true) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Could not change the codec packet-size.");
        return -1;
      }

      send_codec_inst_.plfreq = send_codec.plfreq;
      send_codec_inst_.pacsize = send_codec.pacsize;
      send_codec_inst_.channels = send_codec.channels;
    }

    
    
    send_codec_inst_.pltype = send_codec.pltype;

    
    if (send_codec.rate != send_codec_inst_.rate) {
      if (codecs_[codec_id]->SetBitRate(send_codec.rate) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Could not change the codec rate.");
        return -1;
      }
      send_codec_inst_.rate = send_codec.rate;
    }
    previous_pltype_ = send_codec_inst_.pltype;

    return 0;
  }
}


int32_t AudioCodingModuleImpl::SendCodec(
    CodecInst* current_codec) const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
               "SendCodec()");
  CriticalSectionScoped lock(acm_crit_sect_);

  assert(current_codec);
  if (!send_codec_registered_) {
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
                 "SendCodec Failed, no codec is registered");

    return -1;
  }
  WebRtcACMCodecParams encoder_param;
  codecs_[current_send_codec_idx_]->EncoderParams(&encoder_param);
  encoder_param.codec_inst.pltype = send_codec_inst_.pltype;
  memcpy(current_codec, &(encoder_param.codec_inst), sizeof(CodecInst));

  return 0;
}


int32_t AudioCodingModuleImpl::SendFrequency() const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
               "SendFrequency()");
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!send_codec_registered_) {
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
                 "SendFrequency Failed, no codec is registered");

    return -1;
  }

  return send_codec_inst_.plfreq;
}




int32_t AudioCodingModuleImpl::SendBitrate() const {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!send_codec_registered_) {
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
                 "SendBitrate Failed, no codec is registered");

    return -1;
  }

  WebRtcACMCodecParams encoder_param;
  codecs_[current_send_codec_idx_]->EncoderParams(&encoder_param);

  return encoder_param.codec_inst.rate;
}



int32_t AudioCodingModuleImpl::SetReceivedEstimatedBandwidth(
    const int32_t bw) {
  return codecs_[current_send_codec_idx_]->SetEstimatedBandwidth(bw);
}



int32_t AudioCodingModuleImpl::RegisterTransportCallback(
    AudioPacketizationCallback* transport) {
  CriticalSectionScoped lock(callback_crit_sect_);
  packetization_callback_ = transport;
  return 0;
}


int32_t AudioCodingModuleImpl::Add10MsData(
    const AudioFrame& audio_frame) {
  if (audio_frame.samples_per_channel_ <= 0) {
    assert(false);
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Cannot Add 10 ms audio, payload length is negative or "
                 "zero");
    return -1;
  }

  
  if ((audio_frame.sample_rate_hz_ != 8000)
      && (audio_frame.sample_rate_hz_ != 16000)
      && (audio_frame.sample_rate_hz_ != 32000)
      && (audio_frame.sample_rate_hz_ != 48000)) {
    assert(false);
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Cannot Add 10 ms audio, input frequency not valid");
    return -1;
  }

  
  if ((audio_frame.sample_rate_hz_ / 100)
      != audio_frame.samples_per_channel_) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Cannot Add 10 ms audio, input frequency and length doesn't"
                 " match");
    return -1;
  }

  if (audio_frame.num_channels_ != 1 && audio_frame.num_channels_ != 2) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Cannot Add 10 ms audio, invalid number of channels.");
    return -1;
  }

  CriticalSectionScoped lock(acm_crit_sect_);
  
  if (!HaveValidEncoder("Add10MsData")) {
    return -1;
  }

  const AudioFrame* ptr_frame;
  
  
  
  
  if (PreprocessToAddData(audio_frame, &ptr_frame) < 0) {
    return -1;
  }
  TRACE_EVENT_ASYNC_BEGIN1("webrtc", "Audio", ptr_frame->timestamp_,
                           "now", clock_->TimeInMilliseconds());

  
  bool remix = ptr_frame->num_channels_ != send_codec_inst_.channels;
  if (secondary_encoder_.get() != NULL) {
    remix = remix ||
        (ptr_frame->num_channels_ != secondary_send_codec_inst_.channels);
  }

  
  
  int16_t buffer[WEBRTC_10MS_PCM_AUDIO];
  if (remix) {
    if (ptr_frame->num_channels_ == 1) {
      if (UpMix(*ptr_frame, WEBRTC_10MS_PCM_AUDIO, buffer) < 0)
        return -1;
    } else {
      if (DownMix(*ptr_frame, WEBRTC_10MS_PCM_AUDIO, buffer) < 0)
        return -1;
    }
  }

  
  
  const int16_t* ptr_audio = ptr_frame->data_;

  
  if (send_codec_inst_.channels != ptr_frame->num_channels_)
    ptr_audio = buffer;

  if (codecs_[current_send_codec_idx_]->Add10MsData(
      ptr_frame->timestamp_, ptr_audio, ptr_frame->samples_per_channel_,
      send_codec_inst_.channels) < 0)
    return -1;

  if (secondary_encoder_.get() != NULL) {
    
    ptr_audio = ptr_frame->data_;
    if (secondary_send_codec_inst_.channels != ptr_frame->num_channels_)
      ptr_audio = buffer;

    if (secondary_encoder_->Add10MsData(
        ptr_frame->timestamp_, ptr_audio, ptr_frame->samples_per_channel_,
        secondary_send_codec_inst_.channels) < 0)
      return -1;
  }

  return 0;
}






int AudioCodingModuleImpl::PreprocessToAddData(const AudioFrame& in_frame,
                                               const AudioFrame** ptr_out) {
  
  assert((secondary_encoder_.get() != NULL) ?
      secondary_send_codec_inst_.plfreq == send_codec_inst_.plfreq : true);

  bool resample = static_cast<int32_t>(in_frame.sample_rate_hz_) !=
      send_codec_inst_.plfreq;

  
  
  bool down_mix;
  if (secondary_encoder_.get() != NULL) {
    down_mix = (in_frame.num_channels_ == 2) &&
        (send_codec_inst_.channels == 1) &&
        (secondary_send_codec_inst_.channels == 1);
  } else {
    down_mix = (in_frame.num_channels_ == 2) &&
        (send_codec_inst_.channels == 1);
  }

  if (!down_mix && !resample) {
    
    last_in_timestamp_ = in_frame.timestamp_;
    last_timestamp_ = in_frame.timestamp_;
    *ptr_out = &in_frame;
    return 0;
  }

  *ptr_out = &preprocess_frame_;
  preprocess_frame_.num_channels_ = in_frame.num_channels_;
  int16_t audio[WEBRTC_10MS_PCM_AUDIO];
  const int16_t* src_ptr_audio = in_frame.data_;
  int16_t* dest_ptr_audio = preprocess_frame_.data_;
  if (down_mix) {
    
    
    if (resample)
      dest_ptr_audio = audio;
    if (DownMix(in_frame, WEBRTC_10MS_PCM_AUDIO, dest_ptr_audio) < 0)
      return -1;
    preprocess_frame_.num_channels_ = 1;
    
    src_ptr_audio = audio;
  }

  preprocess_frame_.timestamp_ = in_frame.timestamp_;
  preprocess_frame_.samples_per_channel_ = in_frame.samples_per_channel_;
  preprocess_frame_.sample_rate_hz_ = in_frame.sample_rate_hz_;
  
  if (resample) {
    
    dest_ptr_audio = preprocess_frame_.data_;

    uint32_t timestamp_diff;

    
    if (last_in_timestamp_ > in_frame.timestamp_) {
      
      timestamp_diff = (static_cast<uint32_t>(0xFFFFFFFF) - last_in_timestamp_)
          + in_frame.timestamp_;
    } else {
      timestamp_diff = in_frame.timestamp_ - last_in_timestamp_;
    }
    preprocess_frame_.timestamp_ = last_timestamp_ +
        static_cast<uint32_t>(timestamp_diff *
            (static_cast<double>(send_codec_inst_.plfreq) /
            static_cast<double>(in_frame.sample_rate_hz_)));

    preprocess_frame_.samples_per_channel_ = input_resampler_.Resample10Msec(
        src_ptr_audio, in_frame.sample_rate_hz_, dest_ptr_audio,
        send_codec_inst_.plfreq, preprocess_frame_.num_channels_);

    if (preprocess_frame_.samples_per_channel_ < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "Cannot add 10 ms audio, resmapling failed");
      return -1;
    }
    preprocess_frame_.sample_rate_hz_ = send_codec_inst_.plfreq;
  }
  last_in_timestamp_ = in_frame.timestamp_;
  last_timestamp_ = preprocess_frame_.timestamp_;

  return 0;
}





bool AudioCodingModuleImpl::FECStatus() const {
  CriticalSectionScoped lock(acm_crit_sect_);
  return fec_enabled_;
}


int32_t
AudioCodingModuleImpl::SetFECStatus(
#ifdef WEBRTC_CODEC_RED
    const bool enable_fec) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (fec_enabled_ != enable_fec) {
    
    memset(red_buffer_, 0, MAX_PAYLOAD_SIZE_BYTE);

    
    ResetFragmentation(kNumFecFragmentationVectors);
    
    fec_enabled_ = enable_fec;
  }
  is_first_red_ = true;  
  return 0;
#else
    const bool /* enable_fec */) {
  fec_enabled_ = false;
  WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, id_,
               "  WEBRTC_CODEC_RED is undefined => fec_enabled_ = %d",
               fec_enabled_);
  return -1;
#endif
}




int32_t AudioCodingModuleImpl::SetVAD(bool enable_dtx, bool enable_vad,
                                      ACMVADMode mode) {
  CriticalSectionScoped lock(acm_crit_sect_);
  return SetVADSafe(enable_dtx, enable_vad, mode);
}

int AudioCodingModuleImpl::SetVADSafe(bool enable_dtx, bool enable_vad,
                                      ACMVADMode mode) {
  
  if ((mode != VADNormal) && (mode != VADLowBitrate)
      && (mode != VADAggr) && (mode != VADVeryAggr)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Invalid VAD Mode %d, no change is made to VAD/DTX status",
                 static_cast<int>(mode));
    return -1;
  }

  
  
  if ((enable_dtx || enable_vad) && stereo_send_) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "VAD/DTX not supported for stereo sending.");
    dtx_enabled_ = false;
    vad_enabled_ = false;
    vad_mode_ = mode;
    return -1;
  }

  
  
  if ((enable_dtx || enable_vad) && secondary_encoder_.get() != NULL) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "VAD/DTX not supported when dual-streaming is enabled.");
    dtx_enabled_ = false;
    vad_enabled_ = false;
    vad_mode_ = mode;
    return -1;
  }

  
  
  dtx_enabled_ = enable_dtx;
  vad_enabled_ = enable_vad;
  vad_mode_ = mode;

  
  if (HaveValidEncoder("SetVAD")) {
    if (codecs_[current_send_codec_idx_]->SetVAD(&dtx_enabled_, &vad_enabled_,
                                                 &vad_mode_) < 0) {
      
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "SetVAD failed");
      dtx_enabled_ = false;
      vad_enabled_ = false;
      return -1;
    }
  }

  return 0;
}



int32_t AudioCodingModuleImpl::VAD(bool* dtx_enabled, bool* vad_enabled,
                                   ACMVADMode* mode) const {
  CriticalSectionScoped lock(acm_crit_sect_);

  *dtx_enabled = dtx_enabled_;
  *vad_enabled = vad_enabled_;
  *mode = vad_mode_;

  return 0;
}





int32_t AudioCodingModuleImpl::InitializeReceiver() {
  CriticalSectionScoped lock(acm_crit_sect_);
  return InitializeReceiverSafe();
}


int32_t AudioCodingModuleImpl::InitializeReceiverSafe() {
  initial_delay_ms_ = 0;
  num_packets_accumulated_ = 0;
  num_bytes_accumulated_ = 0;
  accumulated_audio_ms_ = 0;
  first_payload_received_ = 0;
  last_incoming_send_timestamp_ = 0;
  track_neteq_buffer_ = false;
  playout_ts_ = 0;
  
  
  
  if (receiver_initialized_) {
    for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
      if (UnregisterReceiveCodecSafe(i) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "InitializeReceiver() failed, Could not unregister codec");
        return -1;
      }
    }
  }
  if (neteq_.Init() != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "InitializeReceiver() failed, Could not initialize NetEQ");
    return -1;
  }
  neteq_.set_id(id_);
  if (neteq_.AllocatePacketBuffer(ACMCodecDB::NetEQDecoders(),
                                   ACMCodecDB::kNumCodecs) != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "NetEQ cannot allocate_packet Buffer");
    return -1;
  }

  
  for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
    if (IsCodecRED(i) || IsCodecCN(i)) {
      if (RegisterRecCodecMSSafe(ACMCodecDB::database_[i], i, i,
                                 ACMNetEQ::kMasterJb) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Cannot register master codec.");
        return -1;
      }
      registered_pltypes_[i] = ACMCodecDB::database_[i].pltype;
    }
  }

  receiver_initialized_ = true;
  return 0;
}


int32_t AudioCodingModuleImpl::ResetDecoder() {
  CriticalSectionScoped lock(acm_crit_sect_);

  for (int id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if ((codecs_[id] != NULL) && (registered_pltypes_[id] != -1)) {
      if (codecs_[id]->ResetDecoder(registered_pltypes_[id]) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "ResetDecoder failed:");
        return -1;
      }
    }
  }
  return neteq_.FlushBuffers();
}


int32_t AudioCodingModuleImpl::ReceiveFrequency() const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
               "ReceiveFrequency()");
  WebRtcACMCodecParams codec_params;

  CriticalSectionScoped lock(acm_crit_sect_);
  if (DecoderParamByPlType(last_recv_audio_codec_pltype_, codec_params) < 0) {
    return neteq_.CurrentSampFreqHz();
  } else if (codec_params.codec_inst.plfreq == 48000) {
    
    return 32000;
  } else {
    return codec_params.codec_inst.plfreq;
  }
}


int32_t AudioCodingModuleImpl::PlayoutFrequency() const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
               "PlayoutFrequency()");

  CriticalSectionScoped lock(acm_crit_sect_);

  return neteq_.CurrentSampFreqHz();
}



int32_t AudioCodingModuleImpl::RegisterReceiveCodec(
    const CodecInst& receive_codec) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (receive_codec.channels > 2) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "More than 2 audio channel is not supported.");
    return -1;
  }

  int mirror_id;
  int codec_id = ACMCodecDB::ReceiverCodecNumber(&receive_codec, &mirror_id);

  if (codec_id < 0 || codec_id >= ACMCodecDB::kNumCodecs) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Wrong codec params to be registered as receive codec");
    return -1;
  }
  
  if (!ACMCodecDB::ValidPayloadType(receive_codec.pltype)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Invalid payload-type %d for %s.", receive_codec.pltype,
                 receive_codec.plname);
    return -1;
  }

  if (!receiver_initialized_) {
    if (InitializeReceiverSafe() < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "Cannot initialize reciver, so failed registering a codec.");
      return -1;
    }
  }

  
  
  if ((registered_pltypes_[codec_id] == receive_codec.pltype)
      && IsCodecCN(&receive_codec)) {
    
    
    return 0;
  } else if (registered_pltypes_[codec_id] != -1) {
    if (UnregisterReceiveCodecSafe(codec_id) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "Cannot register master codec.");
      return -1;
    }
  }

  if (RegisterRecCodecMSSafe(receive_codec, codec_id, mirror_id,
                             ACMNetEQ::kMasterJb) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Cannot register master codec.");
    return -1;
  }

  
  
  
  

  
  
  if (receive_codec.channels == 2 ||
      (stereo_receive_registered_ && (IsCodecCN(&receive_codec) ||
          IsCodecRED(&receive_codec)))) {
    

    if (!stereo_receive_registered_) {
      
      

      
      assert(neteq_.num_slaves() == 0);
      if (neteq_.AddSlave(ACMCodecDB::NetEQDecoders(),
                           ACMCodecDB::kNumCodecs) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Cannot add slave jitter buffer to NetEQ.");
        return -1;
      }

      
      for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
        if (registered_pltypes_[i] != -1 && (IsCodecRED(i) || IsCodecCN(i))) {
          stereo_receive_[i] = true;

          CodecInst codec;
          memcpy(&codec, &ACMCodecDB::database_[i], sizeof(CodecInst));
          codec.pltype = registered_pltypes_[i];
          if (RegisterRecCodecMSSafe(codec, i, i, ACMNetEQ::kSlaveJb) < 0) {
            WEBRTC_TRACE(kTraceError, kTraceAudioCoding, id_,
                         "Cannot register slave codec.");
            return -1;
          }
        }
      }
    }

    if (RegisterRecCodecMSSafe(receive_codec, codec_id, mirror_id,
                               ACMNetEQ::kSlaveJb) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "Cannot register slave codec.");
      return -1;
    }

    if (!stereo_receive_[codec_id] &&
        (last_recv_audio_codec_pltype_ == receive_codec.pltype)) {
      
      
      
      
      
      
      
      last_recv_audio_codec_pltype_ = -1;
    }

    stereo_receive_[codec_id] = true;
    stereo_receive_registered_ = true;
  } else {
    if (last_recv_audio_codec_pltype_ == receive_codec.pltype &&
        expected_channels_ == 2) {
      
      
      
      
      
      
      
      last_recv_audio_codec_pltype_ = -1;
    }
    stereo_receive_[codec_id] = false;
  }

  registered_pltypes_[codec_id] = receive_codec.pltype;

  if (IsCodecRED(&receive_codec)) {
    receive_red_pltype_ = receive_codec.pltype;
  }
  return 0;
}

int32_t AudioCodingModuleImpl::RegisterRecCodecMSSafe(
    const CodecInst& receive_codec, int16_t codec_id,
    int16_t mirror_id, ACMNetEQ::JitterBuffer jitter_buffer) {
  ACMGenericCodec** codecs;
  if (jitter_buffer == ACMNetEQ::kMasterJb) {
    codecs = &codecs_[0];
  } else if (jitter_buffer == ACMNetEQ::kSlaveJb) {
    codecs = &slave_codecs_[0];
    if (codecs_[codec_id]->IsTrueStereoCodec()) {
      
      
      slave_codecs_[mirror_id] = codecs_[mirror_id];
      mirror_codec_idx_[mirror_id] = mirror_id;
    }
  } else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "RegisterReceiveCodecMSSafe failed, jitter_buffer is neither "
                 "master or slave ");
    return -1;
  }

  if (codecs[mirror_id] == NULL) {
    codecs[mirror_id] = CreateCodec(receive_codec);
    if (codecs[mirror_id] == NULL) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "Cannot create codec to register as receive codec");
      return -1;
    }
    mirror_codec_idx_[mirror_id] = mirror_id;
  }
  if (mirror_id != codec_id) {
    codecs[codec_id] = codecs[mirror_id];
    mirror_codec_idx_[codec_id] = mirror_id;
  }

  codecs[codec_id]->SetIsMaster(jitter_buffer == ACMNetEQ::kMasterJb);

  int16_t status = 0;
  WebRtcACMCodecParams codec_params;
  memcpy(&(codec_params.codec_inst), &receive_codec, sizeof(CodecInst));
  codec_params.enable_vad = false;
  codec_params.enable_dtx = false;
  codec_params.vad_mode = VADNormal;
  if (!codecs[codec_id]->DecoderInitialized()) {
    
    status = codecs[codec_id]->InitDecoder(&codec_params, true);
    if (status < 0) {
      
      
      WEBRTC_TRACE(
          webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
          "could not initialize the receive codec, codec not registered");

      return -1;
    }
  } else if (mirror_id != codec_id) {
    
    
    codecs[codec_id]->SaveDecoderParam(&codec_params);
  }

  if (codecs[codec_id]->RegisterInNetEq(&neteq_, receive_codec) != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Receive codec could not be registered in NetEQ");
    return -1;
  }
  
  
  codecs[codec_id]->SaveDecoderParam(&codec_params);

  return status;
}


int32_t AudioCodingModuleImpl::ReceiveCodec(
    CodecInst* current_codec) const {
  WebRtcACMCodecParams decoder_param;
  CriticalSectionScoped lock(acm_crit_sect_);

  for (int id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if (codecs_[id] != NULL) {
      if (codecs_[id]->DecoderInitialized()) {
        if (codecs_[id]->DecoderParams(&decoder_param,
                                       last_recv_audio_codec_pltype_)) {
          memcpy(current_codec, &decoder_param.codec_inst,
                 sizeof(CodecInst));
          return 0;
        }
      }
    }
  }

  
  
  current_codec->pltype = -1;
  return -1;
}


int32_t AudioCodingModuleImpl::IncomingPacket(
    const uint8_t* incoming_payload,
    const int32_t payload_length,
    const WebRtcRTPHeader& rtp_info) {
  WebRtcRTPHeader rtp_header;

  memcpy(&rtp_header, &rtp_info, sizeof(WebRtcRTPHeader));

  if (payload_length < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "IncomingPacket() Error, payload-length cannot be negative");
    return -1;
  }

  {
    
    
    CriticalSectionScoped lock(acm_crit_sect_);

    
    
    
    if (av_sync_ && first_payload_received_ &&
        rtp_info.header.sequenceNumber > last_sequence_number_ + 1) {
      
      
      if (last_packet_was_sync_) {
        while (rtp_info.header.sequenceNumber > last_sequence_number_ + 2) {
          PushSyncPacketSafe();
        }
      } else {
        
        if (rtp_info.header.sequenceNumber > last_sequence_number_ + 3) {
          last_sequence_number_ += 2;
          last_incoming_send_timestamp_ += last_timestamp_diff_ * 2;
          last_receive_timestamp_ += 2 * last_timestamp_diff_;
          while (rtp_info.header.sequenceNumber > last_sequence_number_ + 1)
            PushSyncPacketSafe();
        }
      }
    }

    uint8_t my_payload_type;

    
    if (rtp_info.header.payloadType == receive_red_pltype_) {
      
      my_payload_type = incoming_payload[0] & 0x7F;
    } else {
      my_payload_type = rtp_info.header.payloadType;
    }

    
    
    if (!rtp_info.type.Audio.isCNG) {
      

      if (my_payload_type != last_recv_audio_codec_pltype_) {
        
        
        
        
        for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++) {
          if (registered_pltypes_[i] == my_payload_type) {
            if (UpdateUponReceivingCodec(i) != 0)
              return -1;
            break;
          }
        }
        
        
        if (track_neteq_buffer_ || av_sync_) {
          last_incoming_send_timestamp_ = rtp_info.header.timestamp;
        }

        if (nack_enabled_) {
          assert(nack_.get());
          
          nack_->Reset();
          nack_->UpdateSampleRate(
              ACMCodecDB::database_[current_receive_codec_idx_].plfreq);
        }
      }
      last_recv_audio_codec_pltype_ = my_payload_type;
    }

    
    last_receive_timestamp_ = NowTimestamp(current_receive_codec_idx_);

    if (nack_enabled_) {
      assert(nack_.get());
      nack_->UpdateLastReceivedPacket(rtp_header.header.sequenceNumber,
                                      rtp_header.header.timestamp);
    }
  }

  int per_neteq_payload_length = payload_length;
  
  
  if (expected_channels_ == 2) {
    if (!rtp_info.type.Audio.isCNG) {
      
      int32_t length = payload_length;
      uint8_t payload[kMaxPacketSize];
      assert(payload_length <= kMaxPacketSize);
      memcpy(payload, incoming_payload, payload_length);
      codecs_[current_receive_codec_idx_]->SplitStereoPacket(payload, &length);
      rtp_header.type.Audio.channel = 2;
      per_neteq_payload_length = length / 2;
      
      if (neteq_.RecIn(payload, length, rtp_header,
                       last_receive_timestamp_) < 0)
        return -1;
    } else {
      
      
      return 0;
    }
  } else {
    if (neteq_.RecIn(incoming_payload, payload_length, rtp_header,
                     last_receive_timestamp_) < 0)
      return -1;
  }

  {
    CriticalSectionScoped lock(acm_crit_sect_);

    
    
    if (track_neteq_buffer_)
      UpdateBufferingSafe(rtp_header, per_neteq_payload_length);

    if (av_sync_) {
      if (rtp_info.header.sequenceNumber == last_sequence_number_ + 1) {
        last_timestamp_diff_ = rtp_info.header.timestamp -
            last_incoming_send_timestamp_;
      }
      last_sequence_number_ = rtp_info.header.sequenceNumber;
      last_ssrc_ = rtp_info.header.ssrc;
      last_packet_was_sync_ = false;
    }

    if (av_sync_ || track_neteq_buffer_) {
      last_incoming_send_timestamp_ = rtp_info.header.timestamp;
    }

    
    
    if (!rtp_info.type.Audio.isCNG)
      first_payload_received_ = true;
  }
  return 0;
}

int AudioCodingModuleImpl::UpdateUponReceivingCodec(int index) {
  if (codecs_[index] == NULL) {
    WEBRTC_TRACE(kTraceError, kTraceAudioCoding, id_,
                 "IncomingPacket() error: payload type found but "
                 "corresponding codec is NULL");
    return -1;
  }
  codecs_[index]->UpdateDecoderSampFreq(index);
  neteq_.set_received_stereo(stereo_receive_[index]);
  current_receive_codec_idx_ = index;

  
  
  if ((stereo_receive_[index] && (expected_channels_ == 1)) ||
      (!stereo_receive_[index] && (expected_channels_ == 2))) {
    neteq_.FlushBuffers();
    codecs_[index]->ResetDecoder(registered_pltypes_[index]);
  }

  if (stereo_receive_[index] && (expected_channels_ == 1)) {
    
    if (InitStereoSlave() != 0)
      return -1;
  }

  
  if (stereo_receive_[index]) {
    expected_channels_ = 2;
  } else {
    expected_channels_ = 1;
  }

  
  prev_received_channel_ = 0;
  return 0;
}

bool AudioCodingModuleImpl::IsCodecForSlave(int index) const {
  return (registered_pltypes_[index] != -1 && stereo_receive_[index]);
}

int AudioCodingModuleImpl::InitStereoSlave() {
  neteq_.RemoveSlaves();

  if (neteq_.AddSlave(ACMCodecDB::NetEQDecoders(),
                       ACMCodecDB::kNumCodecs) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Cannot add slave jitter buffer to NetEQ.");
    return -1;
  }

  
  for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
    if (codecs_[i] != NULL && IsCodecForSlave(i)) {
      WebRtcACMCodecParams decoder_params;
      if (codecs_[i]->DecoderParams(&decoder_params, registered_pltypes_[i])) {
        if (RegisterRecCodecMSSafe(decoder_params.codec_inst,
                                   i, ACMCodecDB::MirrorID(i),
                                   ACMNetEQ::kSlaveJb) < 0) {
          WEBRTC_TRACE(kTraceError, kTraceAudioCoding, id_,
                       "Cannot register slave codec.");
          return -1;
        }
      }
    }
  }
  return 0;
}

int AudioCodingModuleImpl::SetMinimumPlayoutDelay(int time_ms) {
  {
    CriticalSectionScoped lock(acm_crit_sect_);
    
    if (track_neteq_buffer_ && first_payload_received_)
      return 0;
  }
  return neteq_.SetMinimumDelay(time_ms);
}

int AudioCodingModuleImpl::SetMaximumPlayoutDelay(int time_ms) {
  return neteq_.SetMaximumDelay(time_ms);
}


bool AudioCodingModuleImpl::DtmfPlayoutStatus() const {
#ifndef WEBRTC_CODEC_AVT
  return false;
#else
  return neteq_.avt_playout();
#endif
}



int32_t AudioCodingModuleImpl::SetDtmfPlayoutStatus(
#ifndef WEBRTC_CODEC_AVT
    const bool ) {
  WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, id_,
              "SetDtmfPlayoutStatus() failed: AVT is not supported.");
  return -1;
#else
    const bool enable) {
  return neteq_.SetAVTPlayout(enable);
#endif
}




int32_t AudioCodingModuleImpl::DecoderEstimatedBandwidth() const {
  CodecInst codec;
  int16_t codec_id = -1;
  int pltype_wb;
  int pltype_swb;

  
  for (int id = 0; id < ACMCodecDB::kNumCodecs; id++) {
    
    ACMCodecDB::Codec(id, &codec);

    if (!STR_CASE_CMP(codec.plname, "isac")) {
      codec_id = 1;
      pltype_wb = codec.pltype;

      ACMCodecDB::Codec(id + 1, &codec);
      pltype_swb = codec.pltype;

      break;
    }
  }

  if (codec_id < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "DecoderEstimatedBandwidth failed");
    return -1;
  }

  if ((last_recv_audio_codec_pltype_ == pltype_wb) ||
      (last_recv_audio_codec_pltype_ == pltype_swb)) {
    return codecs_[codec_id]->GetEstimatedBandwidth();
  } else {
    return -1;
  }
}


int32_t AudioCodingModuleImpl::SetPlayoutMode(
    const AudioPlayoutMode mode) {
  if ((mode != voice) && (mode != fax) && (mode != streaming) &&
      (mode != off)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Invalid playout mode.");
    return -1;
  }
  return neteq_.SetPlayoutMode(mode);
}


AudioPlayoutMode AudioCodingModuleImpl::PlayoutMode() const {
  return neteq_.playout_mode();
}



int32_t AudioCodingModuleImpl::PlayoutData10Ms(
    int32_t desired_freq_hz, AudioFrame* audio_frame) {
  TRACE_EVENT_ASYNC_BEGIN0("webrtc", "ACM::PlayoutData10Ms", this);
  bool stereo_mode;

  if (GetSilence(desired_freq_hz, audio_frame)) {
     TRACE_EVENT_ASYNC_END1("webrtc", "ACM::PlayoutData10Ms", this,
                            "silence", true);
     return 0;  
  }

  
  if (neteq_.RecOut(audio_frame_) != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "PlayoutData failed, RecOut Failed");
    return -1;
  }
  int decoded_seq_num;
  uint32_t decoded_timestamp;
  bool update_nack =
      neteq_.DecodedRtpInfo(&decoded_seq_num, &decoded_timestamp) &&
      nack_enabled_;  
  audio_frame->num_channels_ = audio_frame_.num_channels_;
  audio_frame->vad_activity_ = audio_frame_.vad_activity_;
  audio_frame->speech_type_ = audio_frame_.speech_type_;

  stereo_mode = (audio_frame_.num_channels_ > 1);

  
  
  const uint16_t receive_freq =
      static_cast<uint16_t>(audio_frame_.sample_rate_hz_);
  bool tone_detected = false;
  int16_t last_detected_tone;
  int16_t tone;

  
  {
    CriticalSectionScoped lock(acm_crit_sect_);

    
    call_stats_.DecodedByNetEq(audio_frame->speech_type_);

    if (update_nack) {
      assert(nack_.get());
      nack_->UpdateLastDecodedPacket(decoded_seq_num, decoded_timestamp);
    }

    
    
    if (av_sync_ && first_payload_received_ &&
        NowTimestamp(current_receive_codec_idx_) > 5 * last_timestamp_diff_ +
        last_receive_timestamp_) {
      if (!last_packet_was_sync_) {
        
        
        last_incoming_send_timestamp_ += 2 * last_timestamp_diff_;
        last_sequence_number_ += 2;
        last_receive_timestamp_ += 2 * last_timestamp_diff_;
      }

      
      if (PushSyncPacketSafe() < 0)
        return -1;
    }

    if ((receive_freq != desired_freq_hz) && (desired_freq_hz != -1)) {
      TRACE_EVENT_ASYNC_END2("webrtc", "ACM::PlayoutData10Ms", this,
                             "seqnum", decoded_seq_num,
                             "now", clock_->TimeInMilliseconds());
      
      int16_t temp_len = output_resampler_.Resample10Msec(
          audio_frame_.data_, receive_freq, audio_frame->data_,
          desired_freq_hz, audio_frame_.num_channels_);

      if (temp_len < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "PlayoutData failed, resampler failed");
        return -1;
      }

      
      audio_frame->samples_per_channel_ = static_cast<uint16_t>(temp_len);
      
      audio_frame->sample_rate_hz_ = desired_freq_hz;
    } else {
      TRACE_EVENT_ASYNC_END2("webrtc", "ACM::PlayoutData10Ms", this,
                             "seqnum", decoded_seq_num,
                             "now", clock_->TimeInMilliseconds());
      memcpy(audio_frame->data_, audio_frame_.data_,
             audio_frame_.samples_per_channel_ * audio_frame->num_channels_
             * sizeof(int16_t));
      
      audio_frame->samples_per_channel_ =
          audio_frame_.samples_per_channel_;
      
      audio_frame->sample_rate_hz_ = receive_freq;
    }

    
    if (dtmf_detector_ != NULL) {
      
      if (audio_frame->sample_rate_hz_ == 8000) {
        
        
        if (!stereo_mode) {
          dtmf_detector_->Detect(audio_frame->data_,
                                 audio_frame->samples_per_channel_,
                                 audio_frame->sample_rate_hz_, tone_detected,
                                 tone);
        } else {
          
          int16_t master_channel[80];
          for (int n = 0; n < 80; n++) {
            master_channel[n] = audio_frame->data_[n << 1];
          }
          dtmf_detector_->Detect(master_channel,
                                 audio_frame->samples_per_channel_,
                                 audio_frame->sample_rate_hz_, tone_detected,
                                 tone);
        }
      } else {
        
        if (!stereo_mode) {
          dtmf_detector_->Detect(audio_frame_.data_,
                                 audio_frame_.samples_per_channel_,
                                 receive_freq, tone_detected, tone);
        } else {
          int16_t master_channel[WEBRTC_10MS_PCM_AUDIO];
          for (int n = 0; n < audio_frame_.samples_per_channel_; n++) {
            master_channel[n] = audio_frame_.data_[n << 1];
          }
          dtmf_detector_->Detect(master_channel,
                                 audio_frame_.samples_per_channel_,
                                 receive_freq, tone_detected, tone);
        }
      }
    }

    
    
    
    last_detected_tone = kACMToneEnd;
    if (tone_detected) {
      last_detected_tone = last_detected_tone_;
      last_detected_tone_ = tone;
    }
  }

  if (tone_detected) {
    
    CriticalSectionScoped lock(callback_crit_sect_);

    if (dtmf_callback_ != NULL) {
      if (tone != kACMToneEnd) {
        
        dtmf_callback_->IncomingDtmf(static_cast<uint8_t>(tone), false);
      } else if ((tone == kACMToneEnd) && (last_detected_tone != kACMToneEnd)) {
        
        
        dtmf_callback_->IncomingDtmf(static_cast<uint8_t>(last_detected_tone),
                                     true);
      }
    }
  }

  audio_frame->id_ = id_;
  audio_frame->energy_ = -1;
  audio_frame->timestamp_ = 0;

  return 0;
}





int32_t AudioCodingModuleImpl::NetworkStatistics(
    ACMNetworkStatistics* statistics) {
  int32_t status;
  status = neteq_.NetworkStatistics(statistics);
  return status;
}

void AudioCodingModuleImpl::DestructEncoderInst(void* inst) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, id_,
               "DestructEncoderInst()");
  if (!HaveValidEncoder("DestructEncoderInst")) {
    return;
  }

  codecs_[current_send_codec_idx_]->DestructEncoderInst(inst);
}

int16_t AudioCodingModuleImpl::AudioBuffer(
    WebRtcACMAudioBuff& buffer) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, id_,
               "AudioBuffer()");
  if (!HaveValidEncoder("AudioBuffer")) {
    return -1;
  }
  buffer.last_in_timestamp = last_in_timestamp_;
  return codecs_[current_send_codec_idx_]->AudioBuffer(buffer);
}

int16_t AudioCodingModuleImpl::SetAudioBuffer(
    WebRtcACMAudioBuff& buffer) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, id_,
               "SetAudioBuffer()");
  if (!HaveValidEncoder("SetAudioBuffer")) {
    return -1;
  }
  return codecs_[current_send_codec_idx_]->SetAudioBuffer(buffer);
}

uint32_t AudioCodingModuleImpl::EarliestTimestamp() const {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, id_,
               "EarliestTimestamp()");
  if (!HaveValidEncoder("EarliestTimestamp")) {
    return -1;
  }
  return codecs_[current_send_codec_idx_]->EarliestTimestamp();
}

int32_t AudioCodingModuleImpl::RegisterVADCallback(
    ACMVADCallback* vad_callback) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, id_,
               "RegisterVADCallback()");
  CriticalSectionScoped lock(callback_crit_sect_);
  vad_callback_ = vad_callback;
  return 0;
}






int32_t AudioCodingModuleImpl::IncomingPayload(
    const uint8_t* incoming_payload, const int32_t payload_length,
    const uint8_t payload_type, const uint32_t timestamp) {
  if (payload_length < 0) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "IncomingPacket() Error, payload-length cannot be negative");
    return -1;
  }

  if (dummy_rtp_header_ == NULL) {
    
    
    WebRtcACMCodecParams codec_params;
    dummy_rtp_header_ = new WebRtcRTPHeader;
    if (dummy_rtp_header_ == NULL) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "IncomingPayload() Error, out of memory");
      return -1;
    }
    dummy_rtp_header_->header.payloadType = payload_type;
    
    dummy_rtp_header_->header.ssrc = 0;
    dummy_rtp_header_->header.markerBit = false;
    
    dummy_rtp_header_->header.sequenceNumber = rand();
    dummy_rtp_header_->header.timestamp =
        (static_cast<uint32_t>(rand()) << 16) +
        static_cast<uint32_t>(rand());
    dummy_rtp_header_->type.Audio.channel = 1;

    if (DecoderParamByPlType(payload_type, codec_params) < 0) {
      
      
      
      delete dummy_rtp_header_;
      dummy_rtp_header_ = NULL;
      return -1;
    }
    recv_pl_frame_size_smpls_ = codec_params.codec_inst.pacsize;
  }

  if (payload_type != dummy_rtp_header_->header.payloadType) {
    
    
    WebRtcACMCodecParams codec_params;
    if (DecoderParamByPlType(payload_type, codec_params) < 0) {
      
      return -1;
    }
    recv_pl_frame_size_smpls_ = codec_params.codec_inst.pacsize;
    dummy_rtp_header_->header.payloadType = payload_type;
  }

  if (timestamp > 0) {
    dummy_rtp_header_->header.timestamp = timestamp;
  }

  
  
  last_recv_audio_codec_pltype_ = payload_type;

  last_receive_timestamp_ += recv_pl_frame_size_smpls_;
  
  if (neteq_.RecIn(incoming_payload, payload_length, *dummy_rtp_header_,
                   last_receive_timestamp_) < 0) {
    return -1;
  }

  
  dummy_rtp_header_->header.sequenceNumber++;
  dummy_rtp_header_->header.timestamp += recv_pl_frame_size_smpls_;
  return 0;
}

int16_t AudioCodingModuleImpl::DecoderParamByPlType(
    const uint8_t payload_type,
    WebRtcACMCodecParams& codec_params) const {
  CriticalSectionScoped lock(acm_crit_sect_);
  for (int16_t id = 0; id < ACMCodecDB::kMaxNumCodecs;
      id++) {
    if (codecs_[id] != NULL) {
      if (codecs_[id]->DecoderInitialized()) {
        if (codecs_[id]->DecoderParams(&codec_params, payload_type)) {
          return 0;
        }
      }
    }
  }
  
  
  
  codec_params.codec_inst.plname[0] = '\0';
  codec_params.codec_inst.pacsize = 0;
  codec_params.codec_inst.rate = 0;
  codec_params.codec_inst.pltype = -1;
  return -1;
}

int16_t AudioCodingModuleImpl::DecoderListIDByPlName(
    const char* name, const uint16_t frequency) const {
  WebRtcACMCodecParams codec_params;
  CriticalSectionScoped lock(acm_crit_sect_);
  for (int16_t id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if ((codecs_[id] != NULL)) {
      if (codecs_[id]->DecoderInitialized()) {
        assert(registered_pltypes_[id] >= 0);
        assert(registered_pltypes_[id] <= 255);
        codecs_[id]->DecoderParams(
            &codec_params, static_cast<uint8_t>(registered_pltypes_[id]));
        if (!STR_CASE_CMP(codec_params.codec_inst.plname, name)) {
          
          
          
          
          
          
          if ((frequency == 0)||
              (codec_params.codec_inst.plfreq == frequency)) {
            return id;
          }
        }
      }
    }
  }
  
  
  return -1;
}

int32_t AudioCodingModuleImpl::LastEncodedTimestamp(
    uint32_t& timestamp) const {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (!HaveValidEncoder("LastEncodedTimestamp")) {
    return -1;
  }
  timestamp = codecs_[current_send_codec_idx_]->LastEncodedTimestamp();
  return 0;
}

int32_t AudioCodingModuleImpl::ReplaceInternalDTXWithWebRtc(
    bool use_webrtc_dtx) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("ReplaceInternalDTXWithWebRtc")) {
    WEBRTC_TRACE(
        webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
        "Cannot replace codec internal DTX when no send codec is registered.");
    return -1;
  }

  int32_t res = codecs_[current_send_codec_idx_]->ReplaceInternalDTX(
      use_webrtc_dtx);
  
  if (res == 1) {
    vad_enabled_ = true;
  } else if (res < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Failed to set ReplaceInternalDTXWithWebRtc(%d)",
                 use_webrtc_dtx);
    return res;
  }

  return 0;
}

int32_t AudioCodingModuleImpl::IsInternalDTXReplacedWithWebRtc(
    bool* uses_webrtc_dtx) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("IsInternalDTXReplacedWithWebRtc")) {
    return -1;
  }
  if (codecs_[current_send_codec_idx_]->IsInternalDTXReplaced(uses_webrtc_dtx)
      < 0) {
    return -1;
  }
  return 0;
}

int AudioCodingModuleImpl::SetISACMaxRate(int max_bit_per_sec) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("SetISACMaxRate")) {
    return -1;
  }

  return codecs_[current_send_codec_idx_]->SetISACMaxRate(max_bit_per_sec);
}

int AudioCodingModuleImpl::SetISACMaxPayloadSize(int max_size_bytes) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("SetISACMaxPayloadSize")) {
    return -1;
  }

  return codecs_[current_send_codec_idx_]->SetISACMaxPayloadSize(
      max_size_bytes);
}

int32_t AudioCodingModuleImpl::ConfigISACBandwidthEstimator(
    int frame_size_ms,
    int rate_bit_per_sec,
    bool enforce_frame_size) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("ConfigISACBandwidthEstimator")) {
    return -1;
  }

  return codecs_[current_send_codec_idx_]->ConfigISACBandwidthEstimator(
      frame_size_ms, rate_bit_per_sec, enforce_frame_size);
}

int32_t AudioCodingModuleImpl::PlayoutTimestamp(
    uint32_t* timestamp) {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
               "PlayoutTimestamp()");
  {
    CriticalSectionScoped lock(acm_crit_sect_);
    if (track_neteq_buffer_) {
      *timestamp = playout_ts_;
      return 0;
    }
  }
  return neteq_.PlayoutTimestamp(*timestamp);
}

bool AudioCodingModuleImpl::HaveValidEncoder(const char* caller_name) const {
  if ((!send_codec_registered_) || (current_send_codec_idx_ < 0) ||
      (current_send_codec_idx_ >= ACMCodecDB::kNumCodecs)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "%s failed: No send codec is registered.", caller_name);
    return false;
  }
  if ((current_send_codec_idx_ < 0) ||
      (current_send_codec_idx_ >= ACMCodecDB::kNumCodecs)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "%s failed: Send codec index out of range.", caller_name);
    return false;
  }
  if (codecs_[current_send_codec_idx_] == NULL) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "%s failed: Send codec is NULL pointer.", caller_name);
    return false;
  }
  return true;
}

int AudioCodingModuleImpl::UnregisterReceiveCodec(uint8_t payload_type) {
  CriticalSectionScoped lock(acm_crit_sect_);
  int id;

  
  for (id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if (registered_pltypes_[id] == payload_type) {
      
      break;
    }
  }

  if (id >= ACMCodecDB::kNumCodecs) {
    
    return 0;
  }

  
  return UnregisterReceiveCodecSafe(id);
}

int32_t AudioCodingModuleImpl::UnregisterReceiveCodecSafe(
    const int16_t codec_id) {
  const WebRtcNetEQDecoder *neteq_decoder = ACMCodecDB::NetEQDecoders();
  int16_t mirror_id = ACMCodecDB::MirrorID(codec_id);
  bool stereo_receiver = false;

  if (codecs_[codec_id] != NULL) {
    if (registered_pltypes_[codec_id] != -1) {
      
      stereo_receiver = stereo_receive_[codec_id];

      
      if (neteq_.RemoveCodec(neteq_decoder[codec_id],
                              stereo_receive_[codec_id])  < 0) {
        CodecInst codec;
        ACMCodecDB::Codec(codec_id, &codec);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Unregistering %s-%d from NetEQ failed.", codec.plname,
                     codec.plfreq);
        return -1;
      }

      
      
      if (IsCodecCN(codec_id)) {
        for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
          if (IsCodecCN(i)) {
            stereo_receive_[i] = false;
            registered_pltypes_[i] = -1;
          }
        }
      } else {
        if (codec_id == mirror_id) {
          codecs_[codec_id]->DestructDecoder();
          if (stereo_receive_[codec_id]) {
            slave_codecs_[codec_id]->DestructDecoder();
            stereo_receive_[codec_id] = false;
          }
        }
      }

      
      if (stereo_receiver) {
        bool no_stereo = true;

        for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
          if (stereo_receive_[i]) {
            
            no_stereo = false;
            break;
          }
        }

        
        if (no_stereo) {
          neteq_.RemoveSlaves();  
          stereo_receive_registered_ = false;
        }
      }
    }
  }

  if (registered_pltypes_[codec_id] == receive_red_pltype_) {
    
    receive_red_pltype_ = 255;
  }
  registered_pltypes_[codec_id] = -1;

  return 0;
}

int32_t AudioCodingModuleImpl::REDPayloadISAC(
    const int32_t isac_rate, const int16_t isac_bw_estimate,
    uint8_t* payload, int16_t* length_bytes) {
  if (!HaveValidEncoder("EncodeData")) {
    return -1;
  }
  int16_t status;
  status = codecs_[current_send_codec_idx_]->REDPayloadISAC(isac_rate,
                                                            isac_bw_estimate,
                                                            payload,
                                                            length_bytes);
  return status;
}

void AudioCodingModuleImpl::ResetFragmentation(int vector_size) {
  for (int n = 0; n < kMaxNumFragmentationVectors; n++) {
    fragmentation_.fragmentationOffset[n] = n * MAX_PAYLOAD_SIZE_BYTE;
  }
  memset(fragmentation_.fragmentationLength, 0, kMaxNumFragmentationVectors *
         sizeof(fragmentation_.fragmentationLength[0]));
  memset(fragmentation_.fragmentationTimeDiff, 0, kMaxNumFragmentationVectors *
         sizeof(fragmentation_.fragmentationTimeDiff[0]));
  memset(fragmentation_.fragmentationPlType, 0, kMaxNumFragmentationVectors *
         sizeof(fragmentation_.fragmentationPlType[0]));
  fragmentation_.fragmentationVectorSize =
      static_cast<uint16_t>(vector_size);
}


int AudioCodingModuleImpl::SetInitialPlayoutDelay(int delay_ms) {
  if (delay_ms < 0 || delay_ms > 10000) {
    return -1;
  }

  CriticalSectionScoped lock(acm_crit_sect_);

  
  if (!receiver_initialized_) {
    InitializeReceiverSafe();
  }

  if (first_payload_received_) {
    
    return -1;
  }
  initial_delay_ms_ = delay_ms;

  
  
  track_neteq_buffer_ = delay_ms > 0;
  av_sync_ = delay_ms > 0;

  neteq_.EnableAVSync(av_sync_);
  return neteq_.SetMinimumDelay(delay_ms);
}

bool AudioCodingModuleImpl::GetSilence(int desired_sample_rate_hz,
                                       AudioFrame* frame) {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (initial_delay_ms_ == 0 || !track_neteq_buffer_) {
    return false;
  }

  if (accumulated_audio_ms_ >= initial_delay_ms_) {
    
    track_neteq_buffer_ = false;
    return false;
  }

  
  call_stats_.DecodedBySilenceGenerator();

  
  
  int max_num_packets;
  int buffer_size_bytes;
  int per_payload_overhead_bytes;
  neteq_.BufferSpec(max_num_packets, buffer_size_bytes,
                     per_payload_overhead_bytes);
  int total_bytes_accumulated = num_bytes_accumulated_ +
      num_packets_accumulated_ * per_payload_overhead_bytes;
  if (num_packets_accumulated_ > max_num_packets * 0.9 ||
      total_bytes_accumulated > buffer_size_bytes * 0.9) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, id_,
                 "GetSilence: Initial delay couldn't be achieved."
                 " num_packets_accumulated=%d, total_bytes_accumulated=%d",
                 num_packets_accumulated_, num_bytes_accumulated_);
    track_neteq_buffer_ = false;
    return false;
  }

  if (desired_sample_rate_hz > 0) {
    frame->sample_rate_hz_ = desired_sample_rate_hz;
  } else {
    frame->sample_rate_hz_ = 0;
    if (current_receive_codec_idx_ >= 0) {
      frame->sample_rate_hz_ =
          ACMCodecDB::database_[current_receive_codec_idx_].plfreq;
    } else {
      
      frame->sample_rate_hz_ = neteq_.CurrentSampFreqHz();
    }
  }
  frame->num_channels_ = expected_channels_;
  frame->samples_per_channel_ = frame->sample_rate_hz_ / 100;  
  frame->speech_type_ = AudioFrame::kCNG;
  frame->vad_activity_ = AudioFrame::kVadPassive;
  frame->energy_ = 0;
  int samples = frame->samples_per_channel_ * frame->num_channels_;
  memset(frame->data_, 0, samples * sizeof(int16_t));
  return true;
}


int AudioCodingModuleImpl::PushSyncPacketSafe() {
  assert(av_sync_);
  last_sequence_number_++;
  last_incoming_send_timestamp_ += last_timestamp_diff_;
  last_receive_timestamp_ += last_timestamp_diff_;

  WebRtcRTPHeader rtp_info;
  rtp_info.header.payloadType = last_recv_audio_codec_pltype_;
  rtp_info.header.ssrc = last_ssrc_;
  rtp_info.header.markerBit = false;
  rtp_info.header.sequenceNumber = last_sequence_number_;
  rtp_info.header.timestamp = last_incoming_send_timestamp_;
  rtp_info.type.Audio.channel = stereo_receive_[current_receive_codec_idx_] ?
      2 : 1;
  last_packet_was_sync_ = true;
  int payload_len_bytes = neteq_.RecIn(rtp_info, last_receive_timestamp_);

  if (payload_len_bytes < 0)
    return -1;

  
  if (track_neteq_buffer_)
    UpdateBufferingSafe(rtp_info, payload_len_bytes);

  return 0;
}


void AudioCodingModuleImpl::UpdateBufferingSafe(const WebRtcRTPHeader& rtp_info,
                                                int payload_len_bytes) {
  const int in_sample_rate_khz =
      (ACMCodecDB::database_[current_receive_codec_idx_].plfreq / 1000);
  if (first_payload_received_ &&
      rtp_info.header.timestamp > last_incoming_send_timestamp_ &&
      in_sample_rate_khz > 0) {
      accumulated_audio_ms_ += (rtp_info.header.timestamp -
          last_incoming_send_timestamp_) / in_sample_rate_khz;
  }

  num_packets_accumulated_++;
  num_bytes_accumulated_ += payload_len_bytes;

  playout_ts_ = static_cast<uint32_t>(
      rtp_info.header.timestamp - static_cast<uint32_t>(
          initial_delay_ms_ * in_sample_rate_khz));
}

uint32_t AudioCodingModuleImpl::NowTimestamp(int codec_id) {
  
  
  
  
  int sample_rate_khz = ACMCodecDB::database_[codec_id].plfreq / 1000;
  const uint32_t now_in_ms = static_cast<uint32_t>(
      clock_->TimeInMilliseconds() & kMaskTimestamp);
  return static_cast<uint32_t>(sample_rate_khz * now_in_ms);
}

std::vector<uint16_t> AudioCodingModuleImpl::GetNackList(
    int round_trip_time_ms) const {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (round_trip_time_ms < 0) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, id_,
                 "GetNackList: round trip time cannot be negative."
                 " round_trip_time_ms=%d", round_trip_time_ms);
  }
  if (nack_enabled_ && round_trip_time_ms >= 0) {
    assert(nack_.get());
    return nack_->GetNackList(round_trip_time_ms);
  }
  std::vector<uint16_t> empty_list;
  return empty_list;
}

int AudioCodingModuleImpl::LeastRequiredDelayMs() const {
  return std::max(neteq_.LeastRequiredDelayMs(), initial_delay_ms_);
}

int AudioCodingModuleImpl::EnableNack(size_t max_nack_list_size) {
  
  if (max_nack_list_size == 0 ||
      max_nack_list_size > acm2::Nack::kNackListSizeLimit)
    return -1;

  CriticalSectionScoped lock(acm_crit_sect_);
  if (!nack_enabled_) {
    nack_.reset(acm2::Nack::Create(kNackThresholdPackets));
    nack_enabled_ = true;

    
    
    if (current_receive_codec_idx_ >= 0) {
      nack_->UpdateSampleRate(
          ACMCodecDB::database_[current_receive_codec_idx_].plfreq);
    }
  }
  return nack_->SetMaxNackListSize(max_nack_list_size);
}

void AudioCodingModuleImpl::DisableNack() {
  CriticalSectionScoped lock(acm_crit_sect_);
  nack_.reset();  
  nack_enabled_ = false;
}

const char* AudioCodingModuleImpl::Version() const {
  return kLegacyAcmVersion;
}

void AudioCodingModuleImpl::GetDecodingCallStatistics(
      AudioDecodingCallStats* call_stats) const {
  CriticalSectionScoped lock(acm_crit_sect_);
  *call_stats = call_stats_.GetDecodingStatistics();
}

}  

}  
