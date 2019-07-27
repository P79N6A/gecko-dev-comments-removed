









#include "webrtc/modules/audio_coding/main/acm2/audio_coding_module_impl.h"

#include <assert.h>
#include <stdlib.h>
#include <vector>

#include "webrtc/base/checks.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_resampler.h"
#include "webrtc/modules/audio_coding/main/acm2/call_statistics.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/typedefs.h"

namespace webrtc {

namespace acm2 {

enum {
  kACMToneEnd = 999
};


enum {
  kMaxPacketSize = 2560
};




enum {
  kNumRedFragmentationVectors = 2,
  kMaxNumFragmentationVectors = 3
};



enum {
  kNackThresholdPackets = 2
};

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



static int TimestampLessThan(uint32_t t1, uint32_t t2) {
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

AudioCodingModuleImpl::AudioCodingModuleImpl(
    const AudioCodingModule::Config& config)
    : acm_crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      id_(config.id),
      expected_codec_ts_(0xD87F3F9F),
      expected_in_ts_(0xD87F3F9F),
      send_codec_inst_(),
      cng_nb_pltype_(255),
      cng_wb_pltype_(255),
      cng_swb_pltype_(255),
      cng_fb_pltype_(255),
      red_pltype_(255),
      vad_enabled_(false),
      dtx_enabled_(false),
      vad_mode_(VADNormal),
      stereo_send_(false),
      current_send_codec_idx_(-1),
      send_codec_registered_(false),
      receiver_(config),
      is_first_red_(true),
      red_enabled_(false),
      last_red_timestamp_(0),
      codec_fec_enabled_(false),
      previous_pltype_(255),
      aux_rtp_header_(NULL),
      receiver_initialized_(false),
      secondary_send_codec_inst_(),
      codec_timestamp_(expected_codec_ts_),
      first_10ms_data_(false),
      callback_crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      packetization_callback_(NULL),
      vad_callback_(NULL) {

  
  
  const char no_name[] = "noCodecRegistered";
  strncpy(send_codec_inst_.plname, no_name, RTP_PAYLOAD_NAME_SIZE - 1);
  send_codec_inst_.pltype = -1;

  strncpy(secondary_send_codec_inst_.plname, no_name,
          RTP_PAYLOAD_NAME_SIZE - 1);
  secondary_send_codec_inst_.pltype = -1;

  for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++) {
    codecs_[i] = NULL;
    mirror_codec_idx_[i] = -1;
  }

  
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
  WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceAudioCoding, id_, "Created");
}

AudioCodingModuleImpl::~AudioCodingModuleImpl() {
  {
    CriticalSectionScoped lock(acm_crit_sect_);
    current_send_codec_idx_ = -1;

    for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++) {
      if (codecs_[i] != NULL) {
        
        assert(mirror_codec_idx_[i] > -1);
        if (codecs_[mirror_codec_idx_[i]] != NULL) {
          delete codecs_[mirror_codec_idx_[i]];
          codecs_[mirror_codec_idx_[i]] = NULL;
        }

        codecs_[i] = NULL;
      }
    }

    if (red_buffer_ != NULL) {
      delete[] red_buffer_;
      red_buffer_ = NULL;
    }
  }

  if (aux_rtp_header_ != NULL) {
    delete aux_rtp_header_;
    aux_rtp_header_ = NULL;
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

  receiver_.set_id(id_);
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
                                     &last_red_timestamp_,
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
          TimestampLessThan(primary_timestamp, last_red_timestamp_) : 0;
    }

    if (secondary_ready_to_encode) {
      
      
      index_secondary = primary_ready_to_encode ?
          (1 - TimestampLessThan(primary_timestamp, secondary_timestamp)) : 0;
    }

    if (has_previous_payload) {
      index_previous_secondary = primary_ready_to_encode ?
          (1 - TimestampLessThan(primary_timestamp, last_red_timestamp_)) : 0;
      
      
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
      current_timestamp = last_red_timestamp_;
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
          static_cast<uint16_t>(current_timestamp - last_red_timestamp_);
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
  int status;
  WebRtcACMEncodingType encoding_type;
  FrameType frame_type = kAudioFrameSpeech;
  uint8_t current_payload_type = 0;
  bool has_data_to_send = false;
  bool red_active = false;
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

      
      
      
      if ((red_enabled_) &&
          ((encoding_type == kActiveNormalEncoded) ||
              (encoding_type == kPassiveNormalEncoded))) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        red_active = true;

        has_data_to_send = false;
        
        if (!is_first_red_) {
          
          
          memcpy(stream + fragmentation_.fragmentationOffset[1], red_buffer_,
                 fragmentation_.fragmentationLength[1]);
          
          
          uint16_t time_since_last = static_cast<uint16_t>(
              rtp_timestamp - last_red_timestamp_);

          
          fragmentation_.fragmentationPlType[1] =
              fragmentation_.fragmentationPlType[0];
          fragmentation_.fragmentationTimeDiff[1] = time_since_last;
          has_data_to_send = true;
        }

        
        fragmentation_.fragmentationLength[0] = length_bytes;

        
        fragmentation_.fragmentationPlType[0] = current_payload_type;
        last_red_timestamp_ = rtp_timestamp;

        
        red_length_bytes = length_bytes;

        
        
        
        
        length_bytes = static_cast<int16_t>(
            fragmentation_.fragmentationLength[0] +
            fragmentation_.fragmentationLength[1]);

        
        
        
        
        if (codecs_[current_send_codec_idx_]->GetRedPayload(
            red_buffer_, &red_length_bytes) == -1) {
          
          
          memcpy(red_buffer_, stream, red_length_bytes);
        }

        is_first_red_ = false;
        
        current_payload_type = red_pltype_;
        
        fragmentation_.fragmentationVectorSize = kNumRedFragmentationVectors;

        
        my_fragmentation.CopyFrom(fragmentation_);
        
        fragmentation_.fragmentationLength[1] = red_length_bytes;
      }
    }
  }

  if (has_data_to_send) {
    CriticalSectionScoped lock(callback_crit_sect_);

    if (packetization_callback_ != NULL) {
      if (red_active) {
        
        packetization_callback_->SendData(frame_type, current_payload_type,
                                          rtp_timestamp, stream, length_bytes,
                                          &my_fragmentation);
      } else {
        
        packetization_callback_->SendData(frame_type, current_payload_type,
                                          rtp_timestamp, stream, length_bytes,
                                          NULL);
      }
    }

    if (vad_callback_ != NULL) {
      
      vad_callback_->InFrameType(static_cast<int16_t>(encoding_type));
    }
  }
  return length_bytes;
}






int AudioCodingModuleImpl::InitializeSender() {
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
  if (red_enabled_ || secondary_encoder_.get() != NULL) {
    if (red_buffer_ != NULL) {
      memset(red_buffer_, 0, MAX_PAYLOAD_SIZE_BYTE);
    }
    if (red_enabled_) {
      ResetFragmentation(kNumRedFragmentationVectors);
    } else {
      ResetFragmentation(0);
    }
  }

  return 0;
}

int AudioCodingModuleImpl::ResetEncoder() {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (!HaveValidEncoder("ResetEncoder")) {
    return -1;
  }
  return codecs_[current_send_codec_idx_]->ResetEncoder();
}

ACMGenericCodec* AudioCodingModuleImpl::CreateCodec(const CodecInst& codec) {
  ACMGenericCodec* my_codec = NULL;

  my_codec = ACMCodecDB::CreateCodecInstance(codec);
  if (my_codec == NULL) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "ACMCodecDB::CreateCodecInstance() failed in CreateCodec()");
    return my_codec;
  }
  my_codec->SetUniqueID(id_);

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

  int codec_id = ACMCodecDB::CodecNumber(send_codec, mirror_id);
  if (codec_id < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, acm_id,
                 "Invalid codec setting for the send codec.");
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


int AudioCodingModuleImpl::RegisterSendCodec(const CodecInst& send_codec) {
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
    int send_codec_id = ACMCodecDB::CodecNumber(send_codec_inst_,
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

      if (!codec_ptr->HasInternalFEC()) {
        codec_fec_enabled_ = false;
      } else {
        if (codec_ptr->SetFEC(codec_fec_enabled_) < 0) {
          WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                       "Cannot set codec FEC");
          return -1;
        }
      }
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

    if (!codecs_[codec_id]->HasInternalFEC()) {
      codec_fec_enabled_ = false;
    } else {
      if (codecs_[codec_id]->SetFEC(codec_fec_enabled_) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Cannot set codec FEC");
        return -1;
      }
    }

    previous_pltype_ = send_codec_inst_.pltype;
    return 0;
  }
}


int AudioCodingModuleImpl::SendCodec(
    CodecInst* current_codec) const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
               "SendCodec()");
  CriticalSectionScoped lock(acm_crit_sect_);

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


int AudioCodingModuleImpl::SendFrequency() const {
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




int AudioCodingModuleImpl::SendBitrate() const {
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



int AudioCodingModuleImpl::SetReceivedEstimatedBandwidth(int bw) {
  CriticalSectionScoped lock(acm_crit_sect_);
  return codecs_[current_send_codec_idx_]->SetEstimatedBandwidth(bw);
}



int AudioCodingModuleImpl::RegisterTransportCallback(
    AudioPacketizationCallback* transport) {
  CriticalSectionScoped lock(callback_crit_sect_);
  packetization_callback_ = transport;
  return 0;
}


int AudioCodingModuleImpl::Add10MsData(
    const AudioFrame& audio_frame) {
  if (audio_frame.samples_per_channel_ <= 0) {
    assert(false);
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Cannot Add 10 ms audio, payload length is negative or "
                 "zero");
    return -1;
  }

  if (audio_frame.sample_rate_hz_ > 48000) {
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

  bool resample = (in_frame.sample_rate_hz_ != send_codec_inst_.plfreq);

  
  
  bool down_mix;
  if (secondary_encoder_.get() != NULL) {
    down_mix = (in_frame.num_channels_ == 2) &&
        (send_codec_inst_.channels == 1) &&
        (secondary_send_codec_inst_.channels == 1);
  } else {
    down_mix = (in_frame.num_channels_ == 2) &&
        (send_codec_inst_.channels == 1);
  }

  if (!first_10ms_data_) {
    expected_in_ts_ = in_frame.timestamp_;
    expected_codec_ts_ = in_frame.timestamp_;
    first_10ms_data_ = true;
  } else if (in_frame.timestamp_ != expected_in_ts_) {
    
    expected_codec_ts_ += (in_frame.timestamp_ - expected_in_ts_) *
        static_cast<uint32_t>((static_cast<double>(send_codec_inst_.plfreq) /
                    static_cast<double>(in_frame.sample_rate_hz_)));
    expected_in_ts_ = in_frame.timestamp_;
  }


  if (!down_mix && !resample) {
    
    expected_in_ts_ += in_frame.samples_per_channel_;
    expected_codec_ts_ += in_frame.samples_per_channel_;
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

  preprocess_frame_.timestamp_ = expected_codec_ts_;
  preprocess_frame_.samples_per_channel_ = in_frame.samples_per_channel_;
  preprocess_frame_.sample_rate_hz_ = in_frame.sample_rate_hz_;
  
  if (resample) {
    
    dest_ptr_audio = preprocess_frame_.data_;

    preprocess_frame_.samples_per_channel_ =
        resampler_.Resample10Msec(src_ptr_audio,
                                  in_frame.sample_rate_hz_,
                                  send_codec_inst_.plfreq,
                                  preprocess_frame_.num_channels_,
                                  AudioFrame::kMaxDataSizeSamples,
                                  dest_ptr_audio);

    if (preprocess_frame_.samples_per_channel_ < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "Cannot add 10 ms audio, resampling failed");
      return -1;
    }
    preprocess_frame_.sample_rate_hz_ = send_codec_inst_.plfreq;
  }

  expected_codec_ts_ += preprocess_frame_.samples_per_channel_;
  expected_in_ts_ += in_frame.samples_per_channel_;

  return 0;
}





bool AudioCodingModuleImpl::REDStatus() const {
  CriticalSectionScoped lock(acm_crit_sect_);

  return red_enabled_;
}


int AudioCodingModuleImpl::SetREDStatus(bool enable_red) {
  CriticalSectionScoped lock(acm_crit_sect_);

#ifdef WEBRTC_CODEC_RED
  if (enable_red == true && codec_fec_enabled_ == true) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, id_,
                 "Codec internal FEC and RED cannot be co-enabled.");
    return -1;
  }

  if (red_enabled_ != enable_red) {
    
    memset(red_buffer_, 0, MAX_PAYLOAD_SIZE_BYTE);

    
    ResetFragmentation(kNumRedFragmentationVectors);
    
    red_enabled_ = enable_red;
  }
  is_first_red_ = true;  
  return 0;
#else
  red_enabled_ = false;
  WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, id_,
               "  WEBRTC_CODEC_RED is undefined => red_enabled_ = %d",
               red_enabled_);
  return -1;
#endif
}





bool AudioCodingModuleImpl::CodecFEC() const {
  CriticalSectionScoped lock(acm_crit_sect_);
  return codec_fec_enabled_;
}

int AudioCodingModuleImpl::SetCodecFEC(bool enable_codec_fec) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (enable_codec_fec == true && red_enabled_ == true) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, id_,
                 "Codec internal FEC and RED cannot be co-enabled.");
    return -1;
  }

  
  if (HaveValidEncoder("SetCodecFEC") &&
      codecs_[current_send_codec_idx_]->SetFEC(enable_codec_fec) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "Set codec internal FEC failed.");
    return -1;
  }
  codec_fec_enabled_ = enable_codec_fec;
  return 0;
}

int AudioCodingModuleImpl::SetPacketLossRate(int loss_rate) {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (HaveValidEncoder("SetPacketLossRate") &&
      codecs_[current_send_codec_idx_]->SetPacketLossRate(loss_rate) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "Set packet loss rate failed.");
    return -1;
  }
  return 0;
}




int AudioCodingModuleImpl::SetVAD(bool enable_dtx,
                                  bool enable_vad,
                                  ACMVADMode mode) {
  CriticalSectionScoped lock(acm_crit_sect_);
  return SetVADSafe(enable_dtx, enable_vad, mode);
}

int AudioCodingModuleImpl::SetVADSafe(bool enable_dtx,
                                      bool enable_vad,
                                      ACMVADMode mode) {
  
  if ((mode != VADNormal) && (mode != VADLowBitrate)
      && (mode != VADAggr) && (mode != VADVeryAggr)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Invalid VAD Mode %d, no change is made to VAD/DTX status",
                 mode);
    return -1;
  }

  
  
  if ((enable_dtx || enable_vad) && stereo_send_) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "VAD/DTX not supported for stereo sending");
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

  
  if (HaveValidEncoder("SetVAD") && codecs_[current_send_codec_idx_]->SetVAD(
      &dtx_enabled_, &vad_enabled_,  &vad_mode_) < 0) {
      
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "SetVAD failed");
      vad_enabled_ = false;
      dtx_enabled_ = false;
      return -1;
  }
  return 0;
}


int AudioCodingModuleImpl::VAD(bool* dtx_enabled, bool* vad_enabled,
                               ACMVADMode* mode) const {
  CriticalSectionScoped lock(acm_crit_sect_);

  *dtx_enabled = dtx_enabled_;
  *vad_enabled = vad_enabled_;
  *mode = vad_mode_;

  return 0;
}





int AudioCodingModuleImpl::InitializeReceiver() {
  CriticalSectionScoped lock(acm_crit_sect_);
  return InitializeReceiverSafe();
}


int AudioCodingModuleImpl::InitializeReceiverSafe() {
  
  
  
  if (receiver_initialized_) {
    if (receiver_.RemoveAllCodecs() < 0)
      return -1;
  }
  receiver_.set_id(id_);
  receiver_.ResetInitialDelay();
  receiver_.SetMinimumDelay(0);
  receiver_.SetMaximumDelay(0);
  receiver_.FlushBuffers();

  
  for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
    if (IsCodecRED(i) || IsCodecCN(i)) {
      uint8_t pl_type = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
      if (receiver_.AddCodec(i, pl_type, 1, NULL) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "Cannot register master codec.");
        return -1;
      }
    }
  }
  receiver_initialized_ = true;
  return 0;
}





int AudioCodingModuleImpl::ResetDecoder() {
  return 0;
}


int AudioCodingModuleImpl::ReceiveFrequency() const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
               "ReceiveFrequency()");

  CriticalSectionScoped lock(acm_crit_sect_);

  int codec_id = receiver_.last_audio_codec_id();

  return codec_id < 0 ? receiver_.current_sample_rate_hz() :
                        ACMCodecDB::database_[codec_id].plfreq;
}


int AudioCodingModuleImpl::PlayoutFrequency() const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
               "PlayoutFrequency()");

  CriticalSectionScoped lock(acm_crit_sect_);

  return receiver_.current_sample_rate_hz();
}



int AudioCodingModuleImpl::RegisterReceiveCodec(const CodecInst& codec) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (codec.channels > 2 || codec.channels < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Unsupported number of channels, %d.", codec.channels);
    return -1;
  }

  
  if (!receiver_initialized_) {
    if (InitializeReceiverSafe() < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "Cannot initialize receiver, failed registering codec.");
      return -1;
    }
  }

  int mirror_id;
  int codec_id = ACMCodecDB::ReceiverCodecNumber(codec, &mirror_id);

  if (codec_id < 0 || codec_id >= ACMCodecDB::kNumCodecs) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Wrong codec params to be registered as receive codec");
    return -1;
  }

  
  if (!ACMCodecDB::ValidPayloadType(codec.pltype)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Invalid payload-type %d for %s.", codec.pltype,
                 codec.plname);
    return -1;
  }

  AudioDecoder* decoder = NULL;
  
  
  if (GetAudioDecoder(codec, codec_id, mirror_id, &decoder) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Wrong codec params to be registered as receive codec");
    return -1;
  }
  uint8_t payload_type = static_cast<uint8_t>(codec.pltype);
  return receiver_.AddCodec(codec_id, payload_type, codec.channels, decoder);
}


int AudioCodingModuleImpl::ReceiveCodec(CodecInst* current_codec) const {
  return receiver_.LastAudioCodec(current_codec);
}


int AudioCodingModuleImpl::IncomingPacket(const uint8_t* incoming_payload,
                                          const int payload_length,
                                          const WebRtcRTPHeader& rtp_header) {
  if (payload_length < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "IncomingPacket() Error, payload-length cannot be negative");
    return -1;
  }
  int last_audio_pltype = receiver_.last_audio_payload_type();
  if (receiver_.InsertPacket(rtp_header, incoming_payload, payload_length) <
      0) {
    return -1;
  }
  if (receiver_.last_audio_payload_type() != last_audio_pltype) {
    int index = receiver_.last_audio_codec_id();
    assert(index >= 0);
    CriticalSectionScoped lock(acm_crit_sect_);

    
    
    
    
    if (codecs_[index] != NULL)
      codecs_[index]->UpdateDecoderSampFreq(index);
  }
  return 0;
}


int AudioCodingModuleImpl::SetMinimumPlayoutDelay(int time_ms) {
  if ((time_ms < 0) || (time_ms > 10000)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Delay must be in the range of 0-1000 milliseconds.");
    return -1;
  }
  return receiver_.SetMinimumDelay(time_ms);
}

int AudioCodingModuleImpl::SetMaximumPlayoutDelay(int time_ms) {
  if ((time_ms < 0) || (time_ms > 10000)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Delay must be in the range of 0-1000 milliseconds.");
    return -1;
  }
  return receiver_.SetMaximumDelay(time_ms);
}




int AudioCodingModuleImpl::DecoderEstimatedBandwidth() const {
  
  
  int last_audio_codec_id = receiver_.last_audio_codec_id();
  if (last_audio_codec_id >= 0 &&
      STR_CASE_CMP("ISAC", ACMCodecDB::database_[last_audio_codec_id].plname)) {
    CriticalSectionScoped lock(acm_crit_sect_);
    return codecs_[last_audio_codec_id]->GetEstimatedBandwidth();
  }
  return -1;
}


int AudioCodingModuleImpl::SetPlayoutMode(AudioPlayoutMode mode) {
  receiver_.SetPlayoutMode(mode);
  return 0;  
}


AudioPlayoutMode AudioCodingModuleImpl::PlayoutMode() const {
  return receiver_.PlayoutMode();
}



int AudioCodingModuleImpl::PlayoutData10Ms(int desired_freq_hz,
                                           AudioFrame* audio_frame) {
  
  if (receiver_.GetAudio(desired_freq_hz, audio_frame) != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "PlayoutData failed, RecOut Failed");
    return -1;
  }

  audio_frame->id_ = id_;
  return 0;
}







int AudioCodingModuleImpl::NetworkStatistics(ACMNetworkStatistics* statistics) {
  receiver_.NetworkStatistics(statistics);
  return 0;
}

int AudioCodingModuleImpl::RegisterVADCallback(ACMVADCallback* vad_callback) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, id_,
               "RegisterVADCallback()");
  CriticalSectionScoped lock(callback_crit_sect_);
  vad_callback_ = vad_callback;
  return 0;
}


int AudioCodingModuleImpl::IncomingPayload(const uint8_t* incoming_payload,
                                           int payload_length,
                                           uint8_t payload_type,
                                           uint32_t timestamp) {
  if (payload_length < 0) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "IncomingPacket() Error, payload-length cannot be negative");
    return -1;
  }

  
  
  if (aux_rtp_header_ == NULL) {
    
    
    aux_rtp_header_ = new WebRtcRTPHeader;
    aux_rtp_header_->header.payloadType = payload_type;
    
    aux_rtp_header_->header.ssrc = 0;
    aux_rtp_header_->header.markerBit = false;
    
    aux_rtp_header_->header.sequenceNumber = 0x1234;  
    aux_rtp_header_->type.Audio.channel = 1;
  }

  aux_rtp_header_->header.timestamp = timestamp;
  IncomingPacket(incoming_payload, payload_length, *aux_rtp_header_);
  
  aux_rtp_header_->header.sequenceNumber++;
  return 0;
}

int AudioCodingModuleImpl::ReplaceInternalDTXWithWebRtc(bool use_webrtc_dtx) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("ReplaceInternalDTXWithWebRtc")) {
    WEBRTC_TRACE(
        webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
        "Cannot replace codec internal DTX when no send codec is registered.");
    return -1;
  }

  int res = codecs_[current_send_codec_idx_]->ReplaceInternalDTX(
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

int AudioCodingModuleImpl::IsInternalDTXReplacedWithWebRtc(
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

int AudioCodingModuleImpl::ConfigISACBandwidthEstimator(
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


int AudioCodingModuleImpl::SetOpusMaxPlaybackRate(int frequency_hz) {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (!HaveValidEncoder("SetOpusMaxPlaybackRate")) {
    return -1;
  }
  return codecs_[current_send_codec_idx_]->SetOpusMaxPlaybackRate(frequency_hz);
}

int AudioCodingModuleImpl::PlayoutTimestamp(uint32_t* timestamp) {
  return receiver_.GetPlayoutTimestamp(timestamp) ? 0 : -1;
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
  return receiver_.RemoveCodec(payload_type);
}



int AudioCodingModuleImpl::REDPayloadISAC(int isac_rate,
                                          int isac_bw_estimate,
                                          uint8_t* payload,
                                          int16_t* length_bytes) {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (!HaveValidEncoder("EncodeData")) {
    return -1;
  }
  int status;
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
  memset(fragmentation_.fragmentationPlType,
         0,
         kMaxNumFragmentationVectors *
             sizeof(fragmentation_.fragmentationPlType[0]));
  fragmentation_.fragmentationVectorSize = static_cast<uint16_t>(vector_size);
}

int AudioCodingModuleImpl::GetAudioDecoder(const CodecInst& codec, int codec_id,
                                           int mirror_id,
                                           AudioDecoder** decoder) {
  if (ACMCodecDB::OwnsDecoder(codec_id)) {
    
    
    
    
    
    
    if (codecs_[mirror_id] == NULL) {
      codecs_[mirror_id] = CreateCodec(codec);
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
    *decoder = codecs_[codec_id]->Decoder(codec_id);
    if (!*decoder) {
      assert(false);
      return -1;
    }
  } else {
    *decoder = NULL;
  }

  return 0;
}

int AudioCodingModuleImpl::SetInitialPlayoutDelay(int delay_ms) {
  {
    CriticalSectionScoped lock(acm_crit_sect_);
    
    
    if (!receiver_initialized_)
      InitializeReceiverSafe();
  }
  return receiver_.SetInitialDelay(delay_ms);
}

int AudioCodingModuleImpl::EnableNack(size_t max_nack_list_size) {
  return receiver_.EnableNack(max_nack_list_size);
}

void AudioCodingModuleImpl::DisableNack() {
  receiver_.DisableNack();
}

std::vector<uint16_t> AudioCodingModuleImpl::GetNackList(
    int round_trip_time_ms) const {
  return receiver_.GetNackList(round_trip_time_ms);
}

int AudioCodingModuleImpl::LeastRequiredDelayMs() const {
  return receiver_.LeastRequiredDelayMs();
}

void AudioCodingModuleImpl::GetDecodingCallStatistics(
      AudioDecodingCallStats* call_stats) const {
  receiver_.GetDecodingCallStatistics(call_stats);
}

}  

bool AudioCodingImpl::RegisterSendCodec(AudioEncoder* send_codec) {
  FATAL() << "Not implemented yet.";
  return false;
}

bool AudioCodingImpl::RegisterSendCodec(int encoder_type,
                                        uint8_t payload_type,
                                        int frame_size_samples) {
  std::string codec_name;
  int sample_rate_hz;
  int channels;
  if (!MapCodecTypeToParameters(
          encoder_type, &codec_name, &sample_rate_hz, &channels)) {
    return false;
  }
  webrtc::CodecInst codec;
  AudioCodingModule::Codec(
      codec_name.c_str(), &codec, sample_rate_hz, channels);
  codec.pltype = payload_type;
  if (frame_size_samples > 0) {
    codec.pacsize = frame_size_samples;
  }
  return acm_old_->RegisterSendCodec(codec) == 0;
}

const AudioEncoder* AudioCodingImpl::GetSenderInfo() const {
  FATAL() << "Not implemented yet.";
  return reinterpret_cast<const AudioEncoder*>(NULL);
}

const CodecInst* AudioCodingImpl::GetSenderCodecInst() {
  if (acm_old_->SendCodec(&current_send_codec_) != 0) {
    return NULL;
  }
  return &current_send_codec_;
}

int AudioCodingImpl::Add10MsAudio(const AudioFrame& audio_frame) {
  if (acm_old_->Add10MsData(audio_frame) != 0) {
    return -1;
  }
  return acm_old_->Process();
}

const ReceiverInfo* AudioCodingImpl::GetReceiverInfo() const {
  FATAL() << "Not implemented yet.";
  return reinterpret_cast<const ReceiverInfo*>(NULL);
}

bool AudioCodingImpl::RegisterReceiveCodec(AudioDecoder* receive_codec) {
  FATAL() << "Not implemented yet.";
  return false;
}

bool AudioCodingImpl::RegisterReceiveCodec(int decoder_type,
                                           uint8_t payload_type) {
  std::string codec_name;
  int sample_rate_hz;
  int channels;
  if (!MapCodecTypeToParameters(
          decoder_type, &codec_name, &sample_rate_hz, &channels)) {
    return false;
  }
  webrtc::CodecInst codec;
  AudioCodingModule::Codec(
      codec_name.c_str(), &codec, sample_rate_hz, channels);
  codec.pltype = payload_type;
  return acm_old_->RegisterReceiveCodec(codec) == 0;
}

bool AudioCodingImpl::InsertPacket(const uint8_t* incoming_payload,
                                   int32_t payload_len_bytes,
                                   const WebRtcRTPHeader& rtp_info) {
  return acm_old_->IncomingPacket(
             incoming_payload, payload_len_bytes, rtp_info) == 0;
}

bool AudioCodingImpl::InsertPayload(const uint8_t* incoming_payload,
                                    int32_t payload_len_byte,
                                    uint8_t payload_type,
                                    uint32_t timestamp) {
  FATAL() << "Not implemented yet.";
  return false;
}

bool AudioCodingImpl::SetMinimumPlayoutDelay(int time_ms) {
  FATAL() << "Not implemented yet.";
  return false;
}

bool AudioCodingImpl::SetMaximumPlayoutDelay(int time_ms) {
  FATAL() << "Not implemented yet.";
  return false;
}

int AudioCodingImpl::LeastRequiredDelayMs() const {
  FATAL() << "Not implemented yet.";
  return -1;
}

bool AudioCodingImpl::PlayoutTimestamp(uint32_t* timestamp) {
  FATAL() << "Not implemented yet.";
  return false;
}

bool AudioCodingImpl::Get10MsAudio(AudioFrame* audio_frame) {
  return acm_old_->PlayoutData10Ms(playout_frequency_hz_, audio_frame) == 0;
}

bool AudioCodingImpl::NetworkStatistics(
    ACMNetworkStatistics* network_statistics) {
  FATAL() << "Not implemented yet.";
  return false;
}

bool AudioCodingImpl::EnableNack(size_t max_nack_list_size) {
  FATAL() << "Not implemented yet.";
  return false;
}

void AudioCodingImpl::DisableNack() {
  
  
  
}

bool AudioCodingImpl::SetVad(bool enable_dtx,
                             bool enable_vad,
                             ACMVADMode vad_mode) {
  return acm_old_->SetVAD(enable_dtx, enable_vad, vad_mode) == 0;
}

std::vector<uint16_t> AudioCodingImpl::GetNackList(
    int round_trip_time_ms) const {
  return acm_old_->GetNackList(round_trip_time_ms);
}

void AudioCodingImpl::GetDecodingCallStatistics(
    AudioDecodingCallStats* call_stats) const {
  acm_old_->GetDecodingCallStatistics(call_stats);
}

bool AudioCodingImpl::MapCodecTypeToParameters(int codec_type,
                                               std::string* codec_name,
                                               int* sample_rate_hz,
                                               int* channels) {
  switch (codec_type) {
#ifdef WEBRTC_CODEC_PCM16
    case acm2::ACMCodecDB::kPCM16B:
      *codec_name = "L16";
      *sample_rate_hz = 8000;
      *channels = 1;
      break;
    case acm2::ACMCodecDB::kPCM16Bwb:
      *codec_name = "L16";
      *sample_rate_hz = 16000;
      *channels = 1;
      break;
    case acm2::ACMCodecDB::kPCM16Bswb32kHz:
      *codec_name = "L16";
      *sample_rate_hz = 32000;
      *channels = 1;
      break;
    case acm2::ACMCodecDB::kPCM16B_2ch:
      *codec_name = "L16";
      *sample_rate_hz = 8000;
      *channels = 2;
      break;
    case acm2::ACMCodecDB::kPCM16Bwb_2ch:
      *codec_name = "L16";
      *sample_rate_hz = 16000;
      *channels = 2;
      break;
    case acm2::ACMCodecDB::kPCM16Bswb32kHz_2ch:
      *codec_name = "L16";
      *sample_rate_hz = 32000;
      *channels = 2;
      break;
#endif
#if (defined(WEBRTC_CODEC_ISAC) || defined(WEBRTC_CODEC_ISACFX))
    case acm2::ACMCodecDB::kISAC:
      *codec_name = "ISAC";
      *sample_rate_hz = 16000;
      *channels = 1;
      break;
#endif
#ifdef WEBRTC_CODEC_ISAC
    case acm2::ACMCodecDB::kISACSWB:
      *codec_name = "ISAC";
      *sample_rate_hz = 32000;
      *channels = 1;
      break;
    case acm2::ACMCodecDB::kISACFB:
      *codec_name = "ISAC";
      *sample_rate_hz = 48000;
      *channels = 1;
      break;
#endif
#ifdef WEBRTC_CODEC_ILBC
    case acm2::ACMCodecDB::kILBC:
      *codec_name = "ILBC";
      *sample_rate_hz = 8000;
      *channels = 1;
      break;
#endif
    case acm2::ACMCodecDB::kPCMA:
      *codec_name = "PCMA";
      *sample_rate_hz = 8000;
      *channels = 1;
      break;
    case acm2::ACMCodecDB::kPCMA_2ch:
      *codec_name = "PCMA";
      *sample_rate_hz = 8000;
      *channels = 2;
      break;
    case acm2::ACMCodecDB::kPCMU:
      *codec_name = "PCMU";
      *sample_rate_hz = 8000;
      *channels = 1;
      break;
    case acm2::ACMCodecDB::kPCMU_2ch:
      *codec_name = "PCMU";
      *sample_rate_hz = 8000;
      *channels = 2;
      break;
#ifdef WEBRTC_CODEC_G722
    case acm2::ACMCodecDB::kG722:
      *codec_name = "G722";
      *sample_rate_hz = 16000;
      *channels = 1;
      break;
    case acm2::ACMCodecDB::kG722_2ch:
      *codec_name = "G722";
      *sample_rate_hz = 16000;
      *channels = 2;
      break;
#endif
#ifdef WEBRTC_CODEC_OPUS
    case acm2::ACMCodecDB::kOpus:
      *codec_name = "opus";
      *sample_rate_hz = 48000;
      *channels = 2;
      break;
#endif
    case acm2::ACMCodecDB::kCNNB:
      *codec_name = "CN";
      *sample_rate_hz = 8000;
      *channels = 1;
      break;
    case acm2::ACMCodecDB::kCNWB:
      *codec_name = "CN";
      *sample_rate_hz = 16000;
      *channels = 1;
      break;
    case acm2::ACMCodecDB::kCNSWB:
      *codec_name = "CN";
      *sample_rate_hz = 32000;
      *channels = 1;
      break;
    case acm2::ACMCodecDB::kRED:
      *codec_name = "red";
      *sample_rate_hz = 8000;
      *channels = 1;
      break;
#ifdef WEBRTC_CODEC_AVT
    case acm2::ACMCodecDB::kAVT:
      *codec_name = "telephone-event";
      *sample_rate_hz = 8000;
      *channels = 1;
      break;
#endif
    default:
      FATAL() << "Codec type " << codec_type << " not supported.";
  }
  return true;
}

}  
