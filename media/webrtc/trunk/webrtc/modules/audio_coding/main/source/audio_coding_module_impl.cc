









#include "webrtc/modules/audio_coding/main/source/audio_coding_module_impl.h"

#include <assert.h>
#include <stdlib.h>

#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_dtmf_detection.h"
#include "webrtc/modules/audio_coding/main/source/acm_generic_codec.h"
#include "webrtc/modules/audio_coding/main/source/acm_resampler.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

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

AudioCodingModuleImpl::AudioCodingModuleImpl(const WebRtc_Word32 id)
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
      secondary_encoder_(NULL) {

  
  
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

  
  red_buffer_ = new WebRtc_UWord8[MAX_PAYLOAD_SIZE_BYTE];

  
  
  
  
  
  
  
  
  
  
  
  
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

WebRtc_Word32 AudioCodingModuleImpl::ChangeUniqueId(const WebRtc_Word32 id) {
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



WebRtc_Word32 AudioCodingModuleImpl::TimeUntilNextProcess() {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("TimeUntilNextProcess")) {
    return -1;
  }
  return codecs_[current_send_codec_idx_]->SamplesLeftToEncode() /
      (send_codec_inst_.plfreq / 1000);
}

WebRtc_Word32 AudioCodingModuleImpl::Process() {
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
      static_cast<WebRtc_UWord16>(current_timestamp - rtp_timestamp);
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
          static_cast<WebRtc_UWord16>(current_timestamp - last_fec_timestamp_);
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
  
  WebRtc_UWord8 stream[2 * MAX_PAYLOAD_SIZE_BYTE];
  WebRtc_Word16 length_bytes = 2 * MAX_PAYLOAD_SIZE_BYTE;
  WebRtc_Word16 red_length_bytes = length_bytes;
  WebRtc_UWord32 rtp_timestamp;
  WebRtc_Word16 status;
  WebRtcACMEncodingType encoding_type;
  FrameType frame_type = kAudioFrameSpeech;
  WebRtc_UWord8 current_payload_type = 0;
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
          current_payload_type = (WebRtc_UWord8) send_codec_inst_.pltype;
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
          
          
          WebRtc_UWord16 time_since_last = WebRtc_UWord16(
              rtp_timestamp - last_fec_timestamp_);

          
          fragmentation_.fragmentationPlType[1] =
              fragmentation_.fragmentationPlType[0];
          fragmentation_.fragmentationTimeDiff[1] = time_since_last;
          has_data_to_send = true;
        }

        
        fragmentation_.fragmentationLength[0] = length_bytes;

        
        fragmentation_.fragmentationPlType[0] = current_payload_type;
        last_fec_timestamp_ = rtp_timestamp;

        
        red_length_bytes = length_bytes;

        
        
        
        
        length_bytes = static_cast<WebRtc_Word16>(
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
                                          rtp_timestamp, stream, length_bytes,
                                          &my_fragmentation);
      } else {
        
        packetization_callback_->SendData(frame_type, current_payload_type,
                                          rtp_timestamp, stream, length_bytes,
                                          NULL);
      }
    }

    if (vad_callback_ != NULL) {
      
      vad_callback_->InFrameType(((WebRtc_Word16) encoding_type));
    }
  }
  return length_bytes;
}






WebRtc_Word32 AudioCodingModuleImpl::InitializeSender() {
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

WebRtc_Word32 AudioCodingModuleImpl::ResetEncoder() {
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

  char error_message[500];
  int codec_id = ACMCodecDB::CodecNumber(&send_codec, mirror_id, error_message,
                                         sizeof(error_message));
  if (codec_id < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, acm_id,
                 error_message);
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


WebRtc_Word32 AudioCodingModuleImpl::RegisterSendCodec(
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
    WebRtc_Word16 status;
    WebRtcACMCodecParams codec_params;

    memcpy(&(codec_params.codec_inst), &send_codec, sizeof(CodecInst));
    codec_params.enable_vad = vad_enabled_;
    codec_params.enable_dtx = dtx_enabled_;
    codec_params.vad_mode = vad_mode_;
    
    status = codec_ptr->InitEncoder(&codec_params, true);

    
    if (status == 1) {
      vad_enabled_ = true;
    } else if (status < 0) {
      

      
      
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

    
    if (send_codec_registered_) {
      
      
      is_first_red_ = true;

      if (codec_ptr->SetVAD(dtx_enabled_, vad_enabled_, vad_mode_) < 0) {
        
        vad_enabled_ = false;
        dtx_enabled_ = false;
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
    previous_pltype_ = send_codec_inst_.pltype;

    return 0;
  }
}


WebRtc_Word32 AudioCodingModuleImpl::SendCodec(
    CodecInst& current_codec) const {
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
  memcpy(&current_codec, &(encoder_param.codec_inst), sizeof(CodecInst));

  return 0;
}


WebRtc_Word32 AudioCodingModuleImpl::SendFrequency() const {
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




WebRtc_Word32 AudioCodingModuleImpl::SendBitrate() const {
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



WebRtc_Word32 AudioCodingModuleImpl::SetReceivedEstimatedBandwidth(
    const WebRtc_Word32 bw) {
  return codecs_[current_send_codec_idx_]->SetEstimatedBandwidth(bw);
}



WebRtc_Word32 AudioCodingModuleImpl::RegisterTransportCallback(
    AudioPacketizationCallback* transport) {
  CriticalSectionScoped lock(callback_crit_sect_);
  packetization_callback_ = transport;
  return 0;
}



WebRtc_Word32 AudioCodingModuleImpl::RegisterIncomingMessagesCallback(
#ifndef WEBRTC_DTMF_DETECTION
    AudioCodingFeedback* ,
    const ACMCountries ) {
  return -1;
#else
    AudioCodingFeedback* incoming_message,
    const ACMCountries cpt) {
  WebRtc_Word16 status = 0;

  
  {
    CriticalSectionScoped lock(callback_crit_sect_);
    dtmf_callback_ = incoming_message;
  }
  
  {
    CriticalSectionScoped lock(acm_crit_sect_);
    
    if (incoming_message == NULL) {
      
      if (dtmf_detector_ != NULL) {
        delete dtmf_detector_;
        dtmf_detector_ = NULL;
      }
      status = 0;
    } else {
      status = 0;
      if (dtmf_detector_ == NULL) {
        dtmf_detector_ = new ACMDTMFDetection;
        if (dtmf_detector_ == NULL) {
          status = -1;
        }
      }
      if (status >= 0) {
        status = dtmf_detector_->Enable(cpt);
        if (status < 0) {
          
          
          delete dtmf_detector_;
          dtmf_detector_ = NULL;
        }
      }
    }
  }
  
  if ((status < 0)) {
    
    CriticalSectionScoped lock(callback_crit_sect_);
    dtmf_callback_ = NULL;
  }

  return status;
#endif
}


WebRtc_Word32 AudioCodingModuleImpl::Add10MsData(
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

  bool resample = ((WebRtc_Word32) in_frame.sample_rate_hz_
      != send_codec_inst_.plfreq);

  
  
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
      
      timestamp_diff = ((WebRtc_UWord32) 0xFFFFFFFF - last_in_timestamp_)
                                                 + in_frame.timestamp_;
    } else {
      timestamp_diff = in_frame.timestamp_ - last_in_timestamp_;
    }
    preprocess_frame_.timestamp_ = last_timestamp_ +
        (WebRtc_UWord32)(timestamp_diff * ((double) send_codec_inst_.plfreq /
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


WebRtc_Word32
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




WebRtc_Word32 AudioCodingModuleImpl::SetVAD(const bool enable_dtx,
                                            const bool enable_vad,
                                            const ACMVADMode mode) {
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
                 (int) mode);
    return -1;
  }

  
  
  if ((enable_dtx || enable_vad) && stereo_send_) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "VAD/DTX not supported for stereo sending");
    return -1;
  }

  
  
  if ((enable_dtx || enable_vad) && secondary_encoder_.get() != NULL) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "VAD/DTX not supported when dual-streaming is enabled.");
    return -1;
  }

  
  if (HaveValidEncoder("SetVAD")) {
    WebRtc_Word16 status = codecs_[current_send_codec_idx_]->SetVAD(enable_dtx,
                                                                    enable_vad,
                                                                    mode);
    if (status == 1) {
      
      vad_enabled_ = true;
      dtx_enabled_ = enable_dtx;
      vad_mode_ = mode;

      return 0;
    } else if (status < 0) {
      
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                   "SetVAD failed");

      vad_enabled_ = false;
      dtx_enabled_ = false;

      return -1;
    }
  }

  vad_enabled_ = enable_vad;
  dtx_enabled_ = enable_dtx;
  vad_mode_ = mode;

  return 0;
}


WebRtc_Word32 AudioCodingModuleImpl::VAD(bool& dtx_enabled, bool& vad_enabled,
                                         ACMVADMode& mode) const {
  CriticalSectionScoped lock(acm_crit_sect_);

  dtx_enabled = dtx_enabled_;
  vad_enabled = vad_enabled_;
  mode = vad_mode_;

  return 0;
}





WebRtc_Word32 AudioCodingModuleImpl::InitializeReceiver() {
  CriticalSectionScoped lock(acm_crit_sect_);
  return InitializeReceiverSafe();
}


WebRtc_Word32 AudioCodingModuleImpl::InitializeReceiverSafe() {
  
  
  
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


WebRtc_Word32 AudioCodingModuleImpl::ResetDecoder() {
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


WebRtc_Word32 AudioCodingModuleImpl::ReceiveFrequency() const {
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


WebRtc_Word32 AudioCodingModuleImpl::PlayoutFrequency() const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
               "PlayoutFrequency()");

  CriticalSectionScoped lock(acm_crit_sect_);

  return neteq_.CurrentSampFreqHz();
}



WebRtc_Word32 AudioCodingModuleImpl::RegisterReceiveCodec(
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

WebRtc_Word32 AudioCodingModuleImpl::RegisterRecCodecMSSafe(
    const CodecInst& receive_codec, WebRtc_Word16 codec_id,
    WebRtc_Word16 mirror_id, ACMNetEQ::JitterBuffer jitter_buffer) {
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

  WebRtc_Word16 status = 0;
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


WebRtc_Word32 AudioCodingModuleImpl::ReceiveCodec(
    CodecInst& current_codec) const {
  WebRtcACMCodecParams decoder_param;
  CriticalSectionScoped lock(acm_crit_sect_);

  for (int id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if (codecs_[id] != NULL) {
      if (codecs_[id]->DecoderInitialized()) {
        if (codecs_[id]->DecoderParams(&decoder_param,
                                       last_recv_audio_codec_pltype_)) {
          memcpy(&current_codec, &decoder_param.codec_inst,
                 sizeof(CodecInst));
          return 0;
        }
      }
    }
  }

  
  
  current_codec.pltype = -1;
  return -1;
}


WebRtc_Word32 AudioCodingModuleImpl::IncomingPacket(
    const WebRtc_UWord8* incoming_payload,
    const WebRtc_Word32 payload_length,
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
    WebRtc_UWord8 my_payload_type;

    
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
      }
      last_recv_audio_codec_pltype_ = my_payload_type;
    }
  }

  
  
  if (expected_channels_ == 2) {
    if (!rtp_info.type.Audio.isCNG) {
      
      WebRtc_Word32 length = payload_length;
      WebRtc_UWord8 payload[kMaxPacketSize];
      assert(payload_length <= kMaxPacketSize);
      memcpy(payload, incoming_payload, payload_length);
      codecs_[current_receive_codec_idx_]->SplitStereoPacket(payload, &length);
      rtp_header.type.Audio.channel = 2;
      
      return neteq_.RecIn(payload, length, rtp_header);
    } else {
      
      
      return 0;
    }
  } else {
    return neteq_.RecIn(incoming_payload, payload_length, rtp_header);
  }
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


WebRtc_Word32 AudioCodingModuleImpl::SetMinimumPlayoutDelay(
    const WebRtc_Word32 time_ms) {
  if ((time_ms < 0) || (time_ms > 1000)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "Delay must be in the range of 0-1000 milliseconds.");
    return -1;
  }
  return neteq_.SetExtraDelay(time_ms);
}


bool AudioCodingModuleImpl::DtmfPlayoutStatus() const {
#ifndef WEBRTC_CODEC_AVT
  return false;
#else
  return neteq_.avt_playout();
#endif
}



WebRtc_Word32 AudioCodingModuleImpl::SetDtmfPlayoutStatus(
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




WebRtc_Word32 AudioCodingModuleImpl::DecoderEstimatedBandwidth() const {
  CodecInst codec;
  WebRtc_Word16 codec_id = -1;
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


WebRtc_Word32 AudioCodingModuleImpl::SetPlayoutMode(
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



WebRtc_Word32 AudioCodingModuleImpl::PlayoutData10Ms(
    const WebRtc_Word32 desired_freq_hz, AudioFrame& audio_frame) {
  bool stereo_mode;

  
  if (neteq_.RecOut(audio_frame_) != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "PlayoutData failed, RecOut Failed");
    return -1;
  }

  audio_frame.num_channels_ = audio_frame_.num_channels_;
  audio_frame.vad_activity_ = audio_frame_.vad_activity_;
  audio_frame.speech_type_ = audio_frame_.speech_type_;

  stereo_mode = (audio_frame_.num_channels_ > 1);
  
  

  const WebRtc_UWord16 receive_freq =
      static_cast<WebRtc_UWord16>(audio_frame_.sample_rate_hz_);
  bool tone_detected = false;
  WebRtc_Word16 last_detected_tone;
  WebRtc_Word16 tone;

  
  {
    CriticalSectionScoped lock(acm_crit_sect_);

    if ((receive_freq != desired_freq_hz) && (desired_freq_hz != -1)) {
      
      WebRtc_Word16 temp_len = output_resampler_.Resample10Msec(
          audio_frame_.data_, receive_freq, audio_frame.data_,
          desired_freq_hz, audio_frame_.num_channels_);

      if (temp_len < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                     "PlayoutData failed, resampler failed");
        return -1;
      }

      
      audio_frame.samples_per_channel_ = (WebRtc_UWord16) temp_len;
      
      audio_frame.sample_rate_hz_ = desired_freq_hz;
    } else {
      memcpy(audio_frame.data_, audio_frame_.data_,
             audio_frame_.samples_per_channel_ * audio_frame.num_channels_
             * sizeof(WebRtc_Word16));
      
      audio_frame.samples_per_channel_ =
          audio_frame_.samples_per_channel_;
      
      audio_frame.sample_rate_hz_ = receive_freq;
    }

    
    if (dtmf_detector_ != NULL) {
      
      if (audio_frame.sample_rate_hz_ == 8000) {
        
        
        if (!stereo_mode) {
          dtmf_detector_->Detect(audio_frame.data_,
                                 audio_frame.samples_per_channel_,
                                 audio_frame.sample_rate_hz_, tone_detected,
                                 tone);
        } else {
          
          WebRtc_Word16 master_channel[80];
          for (int n = 0; n < 80; n++) {
            master_channel[n] = audio_frame.data_[n << 1];
          }
          dtmf_detector_->Detect(master_channel,
                                 audio_frame.samples_per_channel_,
                                 audio_frame.sample_rate_hz_, tone_detected,
                                 tone);
        }
      } else {
        
        if (!stereo_mode) {
          dtmf_detector_->Detect(audio_frame_.data_,
                                 audio_frame_.samples_per_channel_,
                                 receive_freq, tone_detected, tone);
        } else {
          WebRtc_Word16 master_channel[WEBRTC_10MS_PCM_AUDIO];
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
        
        dtmf_callback_->IncomingDtmf((WebRtc_UWord8) tone, false);
      } else if ((tone == kACMToneEnd) && (last_detected_tone != kACMToneEnd)) {
        
        
        dtmf_callback_->IncomingDtmf((WebRtc_UWord8) last_detected_tone, true);
      }
    }
  }

  audio_frame.id_ = id_;
  audio_frame.energy_ = -1;
  audio_frame.timestamp_ = 0;

  return 0;
}







ACMVADMode AudioCodingModuleImpl::ReceiveVADMode() const {
  return neteq_.vad_mode();
}


WebRtc_Word16 AudioCodingModuleImpl::SetReceiveVADMode(const ACMVADMode mode) {
  return neteq_.SetVADMode(mode);
}





WebRtc_Word32 AudioCodingModuleImpl::NetworkStatistics(
    ACMNetworkStatistics& statistics) const {
  WebRtc_Word32 status;
  status = neteq_.NetworkStatistics(&statistics);
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

WebRtc_Word16 AudioCodingModuleImpl::AudioBuffer(
    WebRtcACMAudioBuff& buffer) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, id_,
               "AudioBuffer()");
  if (!HaveValidEncoder("AudioBuffer")) {
    return -1;
  }
  buffer.last_in_timestamp = last_in_timestamp_;
  return codecs_[current_send_codec_idx_]->AudioBuffer(buffer);
}

WebRtc_Word16 AudioCodingModuleImpl::SetAudioBuffer(
    WebRtcACMAudioBuff& buffer) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, id_,
               "SetAudioBuffer()");
  if (!HaveValidEncoder("SetAudioBuffer")) {
    return -1;
  }
  return codecs_[current_send_codec_idx_]->SetAudioBuffer(buffer);
}

WebRtc_UWord32 AudioCodingModuleImpl::EarliestTimestamp() const {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, id_,
               "EarliestTimestamp()");
  if (!HaveValidEncoder("EarliestTimestamp")) {
    return -1;
  }
  return codecs_[current_send_codec_idx_]->EarliestTimestamp();
}

WebRtc_Word32 AudioCodingModuleImpl::RegisterVADCallback(
    ACMVADCallback* vad_callback) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, id_,
               "RegisterVADCallback()");
  CriticalSectionScoped lock(callback_crit_sect_);
  vad_callback_ = vad_callback;
  return 0;
}


WebRtc_Word32 AudioCodingModuleImpl::IncomingPayload(
    const WebRtc_UWord8* incoming_payload, const WebRtc_Word32 payload_length,
    const WebRtc_UWord8 payload_type, const WebRtc_UWord32 timestamp) {
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
        (static_cast<WebRtc_UWord32>(rand()) << 16) +
        static_cast<WebRtc_UWord32>(rand());
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

  
  if (neteq_.RecIn(incoming_payload, payload_length, *dummy_rtp_header_) < 0) {
    return -1;
  }

  
  dummy_rtp_header_->header.sequenceNumber++;
  dummy_rtp_header_->header.timestamp += recv_pl_frame_size_smpls_;
  return 0;
}

WebRtc_Word16 AudioCodingModuleImpl::DecoderParamByPlType(
    const WebRtc_UWord8 payload_type,
    WebRtcACMCodecParams& codec_params) const {
  CriticalSectionScoped lock(acm_crit_sect_);
  for (WebRtc_Word16 id = 0; id < ACMCodecDB::kMaxNumCodecs;
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

WebRtc_Word16 AudioCodingModuleImpl::DecoderListIDByPlName(
    const char* name, const WebRtc_UWord16 frequency) const {
  WebRtcACMCodecParams codec_params;
  CriticalSectionScoped lock(acm_crit_sect_);
  for (WebRtc_Word16 id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if ((codecs_[id] != NULL)) {
      if (codecs_[id]->DecoderInitialized()) {
        assert(registered_pltypes_[id] >= 0);
        assert(registered_pltypes_[id] <= 255);
        codecs_[id]->DecoderParams(
            &codec_params, (WebRtc_UWord8) registered_pltypes_[id]);
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

WebRtc_Word32 AudioCodingModuleImpl::LastEncodedTimestamp(
    WebRtc_UWord32& timestamp) const {
  CriticalSectionScoped lock(acm_crit_sect_);
  if (!HaveValidEncoder("LastEncodedTimestamp")) {
    return -1;
  }
  timestamp = codecs_[current_send_codec_idx_]->LastEncodedTimestamp();
  return 0;
}

WebRtc_Word32 AudioCodingModuleImpl::ReplaceInternalDTXWithWebRtc(
    bool use_webrtc_dtx) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("ReplaceInternalDTXWithWebRtc")) {
    WEBRTC_TRACE(
        webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
        "Cannot replace codec internal DTX when no send codec is registered.");
    return -1;
  }

  WebRtc_Word32 res = codecs_[current_send_codec_idx_]->ReplaceInternalDTX(
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

WebRtc_Word32 AudioCodingModuleImpl::IsInternalDTXReplacedWithWebRtc(
    bool& uses_webrtc_dtx) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("IsInternalDTXReplacedWithWebRtc")) {
    return -1;
  }
  if (codecs_[current_send_codec_idx_]->IsInternalDTXReplaced(&uses_webrtc_dtx)
      < 0) {
    return -1;
  }
  return 0;
}

WebRtc_Word32 AudioCodingModuleImpl::SetISACMaxRate(
    const WebRtc_UWord32 max_bit_per_sec) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("SetISACMaxRate")) {
    return -1;
  }

  return codecs_[current_send_codec_idx_]->SetISACMaxRate(max_bit_per_sec);
}

WebRtc_Word32 AudioCodingModuleImpl::SetISACMaxPayloadSize(
    const WebRtc_UWord16 max_size_bytes) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("SetISACMaxPayloadSize")) {
    return -1;
  }

  return codecs_[current_send_codec_idx_]->SetISACMaxPayloadSize(
      max_size_bytes);
}

WebRtc_Word32 AudioCodingModuleImpl::ConfigISACBandwidthEstimator(
    const WebRtc_UWord8 frame_size_ms,
    const WebRtc_UWord16 rate_bit_per_sec,
    const bool enforce_frame_size) {
  CriticalSectionScoped lock(acm_crit_sect_);

  if (!HaveValidEncoder("ConfigISACBandwidthEstimator")) {
    return -1;
  }

  return codecs_[current_send_codec_idx_]->ConfigISACBandwidthEstimator(
      frame_size_ms, rate_bit_per_sec, enforce_frame_size);
}

WebRtc_Word32 AudioCodingModuleImpl::SetBackgroundNoiseMode(
    const ACMBackgroundNoiseMode mode) {
  if ((mode < On) || (mode > Off)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, id_,
                 "The specified background noise is out of range.\n");
    return -1;
  }
  return neteq_.SetBackgroundNoiseMode(mode);
}

WebRtc_Word32 AudioCodingModuleImpl::BackgroundNoiseMode(
    ACMBackgroundNoiseMode& mode) {
  return neteq_.BackgroundNoiseMode(mode);
}

WebRtc_Word32 AudioCodingModuleImpl::PlayoutTimestamp(
    WebRtc_UWord32& timestamp) {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, id_,
               "PlayoutTimestamp()");
  return neteq_.PlayoutTimestamp(timestamp);
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

WebRtc_Word32 AudioCodingModuleImpl::UnregisterReceiveCodec(
    const WebRtc_Word16 payload_type) {
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

WebRtc_Word32 AudioCodingModuleImpl::UnregisterReceiveCodecSafe(
    const WebRtc_Word16 codec_id) {
  const WebRtcNetEQDecoder *neteq_decoder = ACMCodecDB::NetEQDecoders();
  WebRtc_Word16 mirror_id = ACMCodecDB::MirrorID(codec_id);
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

WebRtc_Word32 AudioCodingModuleImpl::REDPayloadISAC(
    const WebRtc_Word32 isac_rate, const WebRtc_Word16 isac_bw_estimate,
    WebRtc_UWord8* payload, WebRtc_Word16* length_bytes) {
  if (!HaveValidEncoder("EncodeData")) {
    return -1;
  }
  WebRtc_Word16 status;
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
      static_cast<WebRtc_UWord16>(vector_size);
}

}  
